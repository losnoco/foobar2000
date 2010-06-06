#define MYVERSION "1.0"

/*
	changelog

2010-06-06 02:06 UTC - kode54
- Initial release
- Version is now 1.0

*/

#define _WIN32_WINNT 0x0501
#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

#include "echo.h"

static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );

class dsp_echo : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	
	int m_ms, m_amp;

	pfc::array_t<Echo> m_buffers;

public:
	dsp_echo( dsp_preset const & in ) : m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), m_ms( 200 ), m_amp( 128 )
	{
		parse_preset( m_ms, m_amp, in );
	}

	static GUID g_get_guid()
	{
		static const GUID guid = { 0xc2794c27, 0x2091, 0x460a, { 0xa7, 0x5c, 0x1, 0x6, 0xc6, 0x6b, 0xa7, 0x96 } };
		return guid;
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "Echo"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			m_buffers.set_count( 0 );
			m_buffers.set_count( m_ch );
			for ( unsigned i = 0; i < m_ch; i++ )
			{
				Echo & e = m_buffers[ i ];
				e.SetSampleRate( m_rate );
				e.SetDelay( m_ms );
				e.SetAmp( m_amp );
			}
		}

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			Echo & e = m_buffers[ i ];
			audio_sample * data = chunk->get_data() + i;
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				*data = e.Process( *data );
				data += m_ch;
			}
		}

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_buffers.set_count( 0 );
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}

	double get_latency()
	{
		return 0;
	}

	bool need_track_change_mark()
	{
		return false;
	}
	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset( 200, 128, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunDSPConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( int ms, int amp, dsp_preset & out )
	{
		dsp_preset_builder builder; builder << ms; builder << amp; builder.finish( g_get_guid(), out );
	}
	static void parse_preset( int & ms, int & amp, const dsp_preset & in )
	{
		try
		{
			dsp_preset_parser parser(in); parser >> ms; parser >> amp;
		}
		catch(exception_io_data) {ms = 200; amp = 128;}
	}
};

static dsp_factory_t<dsp_echo> g_dsp_echo_factory;


class CMyDSPPopup : public CDialogImpl<CMyDSPPopup>
{
public:
	CMyDSPPopup( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }

	enum { IDD = IDD_DSP };

	enum
	{
		MSRangeMin = 10,
		MSRangeMax = 5000,

		MSRangeTotal = MSRangeMax - MSRangeMin,

		AmpRangeMin = 0,
		AmpRangeMax = 256,

		AmpRangeTotal = AmpRangeMax - AmpRangeMin
	};

	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDOK, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		MSG_WM_HSCROLL( OnHScroll )
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		m_slider_ms = GetDlgItem(IDC_SLIDER_MS);
		m_slider_ms.SetRange(0, MSRangeTotal);

		m_slider_amp = GetDlgItem(IDC_SLIDER_AMP);
		m_slider_amp.SetRange(0, AmpRangeTotal);

		{
			int ms, amp;
			dsp_echo::parse_preset( ms, amp, m_initData );
			m_slider_ms.SetPos( pfc::clip_t<t_int32>( ms, MSRangeMin, MSRangeMax ) - MSRangeMin );
			m_slider_amp.SetPos( pfc::clip_t<t_int32>( amp, AmpRangeMin, AmpRangeMax ) - AmpRangeMin );
			RefreshLabel( ms, amp );
		}
		return TRUE;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar pScrollBar )
	{
		int ms, amp;
		ms = m_slider_ms.GetPos() + MSRangeMin;
		amp = m_slider_amp.GetPos() + AmpRangeMin;

		{
			dsp_preset_impl preset;
			dsp_echo::make_preset( ms, amp, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabel( ms, amp );
	}

	void RefreshLabel( int ms, int amp )
	{
		pfc::string_formatter msg; msg << pfc::format_int( ms ) << " ms";
		::uSetDlgItemText( *this, IDC_SLIDER_LABEL_MS, msg );
		msg.reset(); msg << pfc::format_int(amp * 100 / 256) << "%";
		::uSetDlgItemText( *this, IDC_SLIDER_LABEL_AMP, msg );
	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;

	CTrackBarCtrl m_slider_ms, m_slider_amp;
};

static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopup popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}

DECLARE_COMPONENT_VERSION( "Echo DSP", MYVERSION, "" );

VALIDATE_COMPONENT_FILENAME("foo_dsp_echo.dll");
