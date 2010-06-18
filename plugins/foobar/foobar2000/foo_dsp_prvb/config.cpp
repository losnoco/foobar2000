#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../ATLHelpers/ATLHelpers.h"

#include "reverb.h"
#include "config.h"

#include "resource.h"

enum
{
	delay = 0,

	mVolLeft,
	mVolRight,

	VolLeft,
	VolRight,

	FB_SRC_A,
	FB_SRC_B,

	IIR_ALPHA,
	ACC_COEF_A,
	ACC_COEF_B,
	ACC_COEF_C,
	ACC_COEF_D,
	IIR_COEF,
	FB_ALPHA,
	FB_X,

	IIR_DEST_A0,
	IIR_DEST_A1,
	ACC_SRC_A0,
	ACC_SRC_A1,
	ACC_SRC_B0,
	ACC_SRC_B1,
	IIR_SRC_A0,
	IIR_SRC_A1,
	IIR_DEST_B0,
	IIR_DEST_B1,
	ACC_SRC_C0,
	ACC_SRC_C1,
	ACC_SRC_D0,
	ACC_SRC_D1,
	IIR_SRC_B1,
	IIR_SRC_B0,
	MIX_DEST_A0,
	MIX_DEST_A1,
	MIX_DEST_B0,
	MIX_DEST_B1,

	IN_COEF_L,
	IN_COEF_R
};

#define SLIDER_NAMED(a,b) { a, IDC_SLIDER_##b, IDC_SLIDER_LABEL_##b }
#define SLIDER(a) { a, IDC_SLIDER_##a, IDC_SLIDER_LABEL_##a }

struct slider
{
	unsigned index, id, label_id;
};

static const slider sliders_percent[] =
{
	SLIDER_NAMED( mVolLeft, MVOLLEFT ),
	SLIDER_NAMED( mVolRight, MVOLRIGHT ),
	SLIDER_NAMED( VolLeft, VOLLEFT ),
	SLIDER_NAMED( VolRight, VOLRIGHT ),
	SLIDER( IIR_ALPHA ),
	SLIDER( ACC_COEF_A ),
	SLIDER( ACC_COEF_B ),
	SLIDER( ACC_COEF_C ),
	SLIDER( ACC_COEF_D ),
	SLIDER( IIR_COEF ),
	SLIDER( FB_ALPHA ),
	SLIDER( FB_X ),
	SLIDER( IN_COEF_L ),
	SLIDER( IN_COEF_R )
};

static const slider sliders_delay[] =
{
	SLIDER( FB_SRC_A ),
	SLIDER( FB_SRC_B ),
	SLIDER( IIR_DEST_A0 ),
	SLIDER( IIR_DEST_A1 ),
	SLIDER( ACC_SRC_A0 ),
	SLIDER( ACC_SRC_A1 ),
	SLIDER( ACC_SRC_B0 ),
	SLIDER( ACC_SRC_B1 ),
	SLIDER( IIR_SRC_A0 ),
	SLIDER( IIR_SRC_A1 ),
	SLIDER( IIR_DEST_B0 ),
	SLIDER( IIR_DEST_B1 ),
	SLIDER( ACC_SRC_C0 ),
	SLIDER( ACC_SRC_C1 ),
	SLIDER( ACC_SRC_D0 ),
	SLIDER( ACC_SRC_D1 ),
	SLIDER( IIR_SRC_B1 ),
	SLIDER( IIR_SRC_B0 ),
	SLIDER( MIX_DEST_A0 ),
	SLIDER( MIX_DEST_A1 ),
	SLIDER( MIX_DEST_B0 ),
	SLIDER( MIX_DEST_B1 )
};

class CMyDSPPopup : public CDialogImpl<CMyDSPPopup>
{
public:
	CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }

	enum { IDD = IDD_DSP };

	enum
	{
		DelayRangeMin = 1,
		DelayRangeMax = 22050 * 6,

		DelayRangeTotal = DelayRangeMax - DelayRangeMin,

		PercentRangeMin = -32768,
		PercentRangeMax = 32767,

		PercentRangeTotal = PercentRangeMax - PercentRangeMin
	};

	BEGIN_MSG_MAP(CMyDSPPopup)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_COMBO, CBN_SELCHANGE, OnSelect)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		m_combo = GetDlgItem( IDC_COMBO );
		for ( unsigned i = 0, j = _countof( defaults ); i < j; i++ )
		{
			m_combo.AddString( defaults[ i ].name );
		}
		m_combo.AddString( _T("Custom") );

		m_slider[ delay ] = GetDlgItem( IDC_SLIDER_DELAY );
		m_slider[ delay ].SetRangeMin( 0 );
		m_slider[ delay ].SetRangeMax( DelayRangeTotal );

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			sl = GetDlgItem( s.id );
			sl.SetRangeMin( 0 );
			sl.SetRangeMax( PercentRangeTotal, TRUE );
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			m_slider[ s.index ] = GetDlgItem( s.id );
		}

		{
			ReverbConfig r;
			dsp_reverb::parse_preset( r, m_initData );

			m_custom = r;

			RefreshDelayRanges( r.delay );

			FindPreset( r );

			RefreshSliders( r );

			RefreshLabels( r );
		}
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnSelect(UINT, int id, CWindow)
	{
		unsigned idx = m_combo.GetCurSel();
		ReverbConfig r;
		if ( idx < _countof( defaults ) ) r = defaults[ idx ].config;
		else r = m_custom;
		RefreshDelayRanges( r.delay );
		RefreshSliders( r );
		{
			dsp_preset_impl preset;
			dsp_reverb::make_preset( r, preset, m_initData.get_owner() );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabels( r );
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		ReverbConfig r;

		CollectPreset( r );

		FindPreset( r );
		
		if ( pScrollBar.m_hWnd == m_slider[ delay ].m_hWnd ) RefreshDelayRanges( r.delay );

		{
			dsp_preset_impl preset;
			dsp_reverb::make_preset( r, preset, m_initData.get_owner() );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabels( r );
	}

	void RefreshLabels(const ReverbConfig & r)
	{
		pfc::string_formatter msg; msg << pfc::format_int( MulDiv( r.delay, 1000, 22050 ) ) << " ms";
		::uSetDlgItemText( *this, IDC_SLIDER_LABEL_DELAY, msg );
		
		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			msg.reset(); msg << pfc::format_float( ( double ) ( *val ) * 100. / 32768., 0, 2 ) << "%";
			::uSetDlgItemText( *this, s.label_id, msg );
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			msg.reset(); msg << pfc::format_int( MulDiv( *val, 1000 * 4, 22050 ) ) << " ms";
			::uSetDlgItemText( *this, s.label_id, msg );
		}
	}

	void RefreshDelayRanges(unsigned delay)
	{
		unsigned delay_max = delay / 4;
		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			sl.SetRangeMin( 0 );
			sl.SetRangeMax( delay_max, TRUE );
		}
	}

	void RefreshSliders( const ReverbConfig & r )
	{
		m_slider[ delay ].SetPos( pfc::clip_t<t_int32>( r.delay, DelayRangeMin, DelayRangeMax ) - DelayRangeMin );

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			sl.SetPos( pfc::clip_t<t_int32>( *val, PercentRangeMin, PercentRangeMax ) - PercentRangeMin );
		}

		unsigned delay_max = r.delay / 4;

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			sl.SetPos( pfc::clip_t<t_int32>( *val, 0, delay_max ) );
		}
	}

	void CollectPreset( ReverbConfig & r )
	{
		r.delay = m_slider[ delay ].GetPos() + DelayRangeMin;

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			int * val = reinterpret_cast<int*>( &r ) + s.index;
			*val = sl.GetPos() + PercentRangeMin;
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			int * val = reinterpret_cast<int*>( &r ) + s.index;
			*val = sl.GetPos();
		}
	}

	void FindPreset( const ReverbConfig & r )
	{
		unsigned i, j;
		for ( i = 0, j = _countof( defaults ); i < j; i++ )
		{
			const ReverbConfig & rc = defaults[ i ].config;
			if ( !memcmp( &rc, &r, sizeof( r ) ) ) break;
		}
		m_combo.SetCurSel( i );
		if ( i == j ) m_custom = r;
	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;

	CTrackBarCtrl m_slider[37];
	CComboBox m_combo;

	ReverbConfig m_custom;
};

void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
{
	CMyDSPPopup popup( p_data, p_callback );
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}
