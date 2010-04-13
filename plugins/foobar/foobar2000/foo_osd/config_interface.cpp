#include "../ATLHelpers/ATLHelpers.h"

#include "config_interface.h"

#include "resource.h"

#include "config.h"
#include "main.h"

// {A2A5EF33-2C0E-405d-B7E5-360BB4A413C7}
static const GUID guid_cfg_enable = 
{ 0xa2a5ef33, 0x2c0e, 0x405d, { 0xb7, 0xe5, 0x36, 0xb, 0xb4, 0xa4, 0x13, 0xc7 } };
// {EC7D206E-9CD5-4632-B0D2-04150AF7AB9B}
static const GUID guid_cfg_placement = 
{ 0xec7d206e, 0x9cd5, 0x4632, { 0xb0, 0xd2, 0x4, 0x15, 0xa, 0xf7, 0xab, 0x9b } };
// {A138AA8F-826F-48df-ACFF-4A0662E259B2}
static const GUID guid_cfg_customcolors = 
{ 0xa138aa8f, 0x826f, 0x48df, { 0xac, 0xff, 0x4a, 0x6, 0x62, 0xe2, 0x59, 0xb2 } };

cfg_int cfg_enable(guid_cfg_enable, 0);

cfg_window_placement cfg_placement(guid_cfg_placement);

inline static CUSTOMCOLORS get_def_colors()
{
	CUSTOMCOLORS foo;
	for (int i=16;i--;) foo.colors[i] = 0xFFFFFF;
	return foo;
}

cfg_struct_t<CUSTOMCOLORS> cfg_customcolors(guid_cfg_customcolors, get_def_colors());

static const char * tab_names[] = { "General", "Effects", "Format", "Volume" };

#define MAKEID(X,Y) ( ( (DWORD)(X)<<16) | (DWORD)(Y))
#define WM_CMDNOTIFY (WM_USER+6)
#define WM_HSCNOTIFY (WM_USER+7)

enum
{
	IDC_CONFIG1 = 1100,
	IDC_CONFIG2,
	IDC_CONFIG3,
	IDC_CONFIG4
};

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.
	~CMyPreferences();

	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_NOTIFY(OnNotify)
		COMMAND_HANDLER_EX(IDC_ENABLE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_OVERLAY_ADD, BN_CLICKED, OnOverlayAdd)
		COMMAND_HANDLER_EX(IDC_OVERLAY_REMOVE, BN_CLICKED, OnOverlayRemove)
		COMMAND_HANDLER_EX(IDC_OVERLAY_RENAME, BN_CLICKED, OnOverlayRename)
		COMMAND_HANDLER_EX(IDC_OVERLAY_APPLY, BN_CLICKED, OnOverlayApply)
		COMMAND_HANDLER_EX(IDC_OVERLAY_TEST, BN_CLICKED, OnOverlayTest)
		COMMAND_HANDLER_EX(IDC_OVERLAY, CBN_SELCHANGE, OnOverlayChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
	void OnButtonClick(UINT, int, CWindow);
	void OnOverlayAdd(UINT, int, CWindow);
	void OnOverlayRemove(UINT, int, CWindow);
	void OnOverlayRename(UINT, int, CWindow);
	void OnOverlayApply(UINT, int, CWindow);
	void OnOverlayTest(UINT, int, CWindow);
	void OnOverlayChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	void on_count( unsigned count, bool modified );
	void on_modified();
	void load(const osd_config & c);
	void save(osd_config & c);
	bool rename(pfc::string_base & param);

	static BOOL CALLBACK RenameProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	static BOOL CALLBACK ChildProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

	const preferences_page_callback::ptr m_callback;

	struct config_info
	{
		unsigned   n_preset;
		osd_config preset;
		bool       initialized;
		bool       modified;

		config_info() : initialized(false), modified(false) {}
	};

	config_info ctx;

	pfc::array_t<osd_config> backup;

	bool modified, enabled;
};

void CMyPreferences::on_count( unsigned count, bool modified )
{
	BOOL status = (count > 0) ? TRUE : FALSE;
	CWindow hTab;
	GetDlgItem( IDC_OVERLAY_REMOVE ).EnableWindow( status );
	GetDlgItem( IDC_OVERLAY_RENAME ).EnableWindow( status );
	GetDlgItem( IDC_OVERLAY_APPLY ).EnableWindow( status && modified );
	GetDlgItem( IDC_OVERLAY_TEST ).EnableWindow( status );
	hTab = GetDlgItem( IDC_TAB );
	hTab.EnableWindow( status );
	hTab.GetDlgItem( IDC_CONFIG1 ).EnableWindow( status );
	hTab.GetDlgItem( IDC_CONFIG2 ).EnableWindow( status );
	hTab.GetDlgItem( IDC_CONFIG3 ).EnableWindow( status );
	hTab.GetDlgItem( IDC_CONFIG4 ).EnableWindow( status );
}

void CMyPreferences::on_modified()
{
	if (!ctx.modified)
	{
		ctx.modified = true;
		GetDlgItem( IDC_OVERLAY_APPLY ).EnableWindow( TRUE );
		OnChanged();
	}
}

void CMyPreferences::load(const osd_config & c)
{
	CWindow hTab, w, w2;
	
	hTab = GetDlgItem( IDC_TAB );
	
	w = hTab.GetDlgItem( IDC_CONFIG1 );
	w.SendDlgItemMessage( IDC_POP, BM_SETCHECK, !!(c.flags & osd_pop) );
	w.SendDlgItemMessage( IDC_POP_PLAY, BM_SETCHECK, !!(c.flags & osd_play) );
	w.SendDlgItemMessage( IDC_POP_PAUSE, BM_SETCHECK, !!(c.flags & osd_pause) );
	w.SendDlgItemMessage( IDC_POP_SEEK, BM_SETCHECK, !!(c.flags & osd_seek) );
	w.SendDlgItemMessage( IDC_POP_SWITCH, BM_SETCHECK, !!(c.flags & osd_switch) );
	w.SendDlgItemMessage( IDC_POP_DYNAMIC, BM_SETCHECK, !!(c.flags & osd_dynamic) );
	w.SendDlgItemMessage( IDC_POP_DYNAMIC_ALL, BM_SETCHECK, !!(c.flags & osd_dynamic_all) );
	w.SendDlgItemMessage( IDC_POP_VOLUME, BM_SETCHECK, !!(c.flags & osd_volume) );
	w.SendDlgItemMessage( IDC_POP_STOP, BM_SETCHECK, !!(c.flags & osd_hide_on_stop) );
	
	w.SendDlgItemMessage( IDC_PERMANENT, BM_SETCHECK, !!(c.flags & osd_permanent) );
	w.SendDlgItemMessage( IDC_INTERVAL, BM_SETCHECK, !!(c.flags & osd_interval) );
	w.SendDlgItemMessage( IDC_OUTLINE, BM_SETCHECK, !!(c.flags & osd_outline) );
	// FUCKO
#ifdef GDIPLUS
	w.uSendDlgItemMessage( IDC_ANTIALIAS, BM_SETCHECK, !!(c.flags & osd_antialias) );
#else
	w.GetDlgItem( IDC_ANTIALIAS ).EnableWindow( FALSE );
#endif
	
	w2 = w.GetDlgItem( IDC_TIMESPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(99, 1) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG((c.displaytime / 1000), 0) );
	w2 = w.GetDlgItem( IDC_XSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(100, 0) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.x, 0) );
	w2 = w.GetDlgItem( IDC_YSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(0, 100) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.y, 0) );
	
	w.CheckRadioButton( IDC_PLEFT, IDC_PRIGHT, IDC_PLEFT + c.pos );
	w.CheckRadioButton( IDC_ALEFT, IDC_ARIGHT, IDC_ALEFT + c.align );
	
	
	w = hTab.GetDlgItem( IDC_CONFIG2 );
	w.SendDlgItemMessage( IDC_ALPHA, BM_SETCHECK, !!(c.flags & osd_alpha) );
	w.SendDlgItemMessage( IDC_FADEINOUT, BM_SETCHECK, !!(c.flags & osd_fadeinout) );
	w.SendDlgItemMessage( IDC_DISSOLVE, BM_SETCHECK, !!(c.flags & osd_dissolve) );
	
	w2 = w.GetDlgItem( IDC_ALPHATEXT );
	::SendMessage( w2, TBM_SETRANGE, 0, MAKELONG(0, 255) );
	::SendMessage( w2, TBM_SETPOS, 1, 255 - c.alphalev );
	w2 = w.GetDlgItem( IDC_ALPHABACK );
	::SendMessage( w2, TBM_SETRANGE, 0, MAKELONG(0, 255) );
	::SendMessage( w2, TBM_SETPOS, 1, 255 - c.alphaback );
	
	w2 = w.GetDlgItem( IDC_FADESPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(5000, 1) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.fadetime, 0) );
	w2 = w.GetDlgItem( IDC_DECAYSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(255, 1) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.dissolve_decay, 0) );
	
	
	w = hTab.GetDlgItem( IDC_CONFIG3 );
	uSetDlgItemText( w, IDC_FORMAT, c.format );
	//uSetDlgItemText( w, IDC_FORMATNEXT, c.formatnext );
	SetDlgItemTextA( w, IDC_CCF, "$rgb(0,0,0)" );
	SetDlgItemTextA( w, IDC_CC, "$rgb()" );
	
	
	w = hTab.GetDlgItem( IDC_CONFIG4 );
	w2 = w.GetDlgItem( IDC_VWIDTHSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(100, 0) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.vwidth,0) );
	w2 = w.GetDlgItem( IDC_VHEIGHTSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(100, 0) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.vheight,0) );
	w2 = w.GetDlgItem( IDC_VSTEPSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(100,1) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(c.vsteps,0) );
	w2 = w.GetDlgItem( IDC_VMINSPIN );
	::SendMessage( w2, UDM_SETRANGE, 0L, MAKELONG(1,100) );
	::SendMessage( w2, UDM_SETPOS, 0L, MAKELONG(abs(c.vmin) / 100,0) );
}

static void check_flag(unsigned & flags, unsigned flag, int state)
{
	if (state) flags |= flag;
	else flags &= ~flag;
}

void CMyPreferences::save(osd_config & c)
{
	CWindow hTab, w;
	unsigned n;
	
	hTab = GetDlgItem( IDC_TAB );
	
	w = hTab.GetDlgItem( IDC_CONFIG1 );
	
	check_flag( c.flags, osd_pop, w.SendDlgItemMessage( IDC_POP, BM_GETCHECK ) );
	check_flag( c.flags, osd_play, w.SendDlgItemMessage( IDC_POP_PLAY, BM_GETCHECK ) );
	check_flag( c.flags, osd_pause, w.SendDlgItemMessage( IDC_POP_PAUSE, BM_GETCHECK ) );
	check_flag( c.flags, osd_seek, w.SendDlgItemMessage( IDC_POP_SEEK, BM_GETCHECK ) );
	check_flag( c.flags, osd_switch, w.SendDlgItemMessage( IDC_POP_SWITCH, BM_GETCHECK ) );
	check_flag( c.flags, osd_dynamic, w.SendDlgItemMessage( IDC_POP_DYNAMIC, BM_GETCHECK ) );
	check_flag( c.flags, osd_dynamic_all, w.SendDlgItemMessage( IDC_POP_DYNAMIC_ALL, BM_GETCHECK ) );
	check_flag( c.flags, osd_volume, w.SendDlgItemMessage( IDC_POP_VOLUME, BM_GETCHECK ) );
	check_flag( c.flags, osd_hide_on_stop, w.SendDlgItemMessage( IDC_POP_STOP, BM_GETCHECK ) );
	
	check_flag( c.flags, osd_permanent, w.SendDlgItemMessage( IDC_PERMANENT, BM_GETCHECK ) );
	check_flag( c.flags, osd_interval, w.SendDlgItemMessage( IDC_INTERVAL, BM_GETCHECK ) );
	check_flag( c.flags, osd_outline, w.SendDlgItemMessage( IDC_OUTLINE, BM_GETCHECK ) );
	// FUCKO
#ifdef GDIPLUS
	check_flag( c.flags, osd_antialias, w.SendDlgItemMessage( IDC_ANTIALIAS, BM_GETCHECK ) );
#endif
	
	n = w.GetDlgItemInt( IDC_TIME, NULL, FALSE );
	if (n < 1) n = 1;
	else if (n > 99) n = 99;
	c.displaytime = n * 1000;
	
	n = w.GetDlgItemInt( IDC_POSX, NULL, FALSE );
	if (n > 100) n = 100;
	c.x = n;
	
	n = w.GetDlgItemInt( IDC_POSY, NULL, FALSE );
	if (n > 100) n = 100;
	c.y = n;
	
	if (w.SendDlgItemMessage( IDC_PLEFT, BM_GETCHECK ) ) n = DT_LEFT;
	else if (w.SendDlgItemMessage( IDC_PCENTER, BM_GETCHECK ) ) n = DT_CENTER;
	else n = DT_RIGHT;
	c.pos = n;
	
	if (w.SendDlgItemMessage( IDC_ALEFT, BM_GETCHECK ) ) n = DT_LEFT;
	else if (w.SendDlgItemMessage( IDC_ACENTER, BM_GETCHECK ) ) n = DT_CENTER;
	else n = DT_RIGHT;
	c.align = n;
	
	
	w = hTab.GetDlgItem( IDC_CONFIG2 );
	check_flag( c.flags, osd_alpha, w.SendDlgItemMessage( IDC_ALPHA, BM_GETCHECK ) );
	check_flag( c.flags, osd_fadeinout, w.SendDlgItemMessage( IDC_FADEINOUT, BM_GETCHECK ) );
	check_flag( c.flags, osd_dissolve, w.SendDlgItemMessage( IDC_DISSOLVE, BM_GETCHECK ) );
	
	c.alphalev = 255 - w.SendDlgItemMessage( IDC_ALPHATEXT, TBM_GETPOS );
	c.alphaback = 255 - w.SendDlgItemMessage( IDC_ALPHABACK, TBM_GETPOS );
	
	n = w.GetDlgItemInt( IDC_FADETIME, NULL, FALSE );
	if (n < 1) n = 1;
	else if (n > 5000) n = 5000;
	c.fadetime = n;
	
	n = w.GetDlgItemInt( IDC_DECAY, NULL, FALSE );
	if (n < 1) n = 1;
	else if (n > 255) n = 255;
	c.dissolve_decay = n;
	
	
	w = hTab.GetDlgItem( IDC_CONFIG3 );
	c.format = string_utf8_from_window( w, IDC_FORMAT );
	//c.formatnext = string_utf8_from_window( w, IDC_FORMATNEXT );
	
	
	w = hTab.GetDlgItem( IDC_CONFIG4 );
	
	n = w.GetDlgItemInt( IDC_VWIDTH, NULL, FALSE );
	if (n > 100) n = 100;
	c.vwidth = n;
	
	n = w.GetDlgItemInt( IDC_VHEIGHT, NULL, FALSE );
	if (n > 100) n = 100;
	c.vheight = n;

	n = w.GetDlgItemInt( IDC_VSTEPS, NULL, FALSE );
	if (n < 1) n = 1;
	else if (n > 100) n = 100;
	c.vsteps = n;

	n = w.GetDlgItemInt( IDC_VMIN, NULL, FALSE );
	if (n < 1) n = 1;
	else if (n > 100) n = 100;
	c.vmin = - (int(n) * 100);
}

BOOL CALLBACK CMyPreferences::RenameProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		::SetWindowLong( wnd, DWL_USER, lp );
		{
			pfc::string_base * ptr = reinterpret_cast<pfc::string_base*>(lp);
			uSetWindowText( wnd, pfc::string8() << "Rename preset: \"" << ptr->get_ptr() << "\"" );
			uSetDlgItemText( wnd, IDC_EDIT, ptr->get_ptr() );
			cfg_placement.on_window_creation( wnd );
		}
		return 1;

	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			uGetDlgItemText( wnd, IDC_EDIT, *reinterpret_cast<pfc::string_base*>( ::GetWindowLong( wnd, DWL_USER ) ) );
			::EndDialog( wnd, 1 );
			break;

		case IDCANCEL:
			cfg_placement.on_window_destruction( wnd );
			::EndDialog(wnd, 0);
			break;
		}
		break;

	case WM_CLOSE:
		cfg_placement.on_window_destruction( wnd );
		::EndDialog( wnd, 0 );
		break;
	}

	return 0;
}

bool CMyPreferences::rename( pfc::string_base & param )
{
	return !!DialogBoxParam( core_api::get_my_instance(), MAKEINTRESOURCE(IDD_RENAME), m_hWnd, RenameProc, reinterpret_cast<LPARAM>( &param ) );
}

BOOL CALLBACK CMyPreferences::ChildProc( HWND wnd,UINT msg,WPARAM wp,LPARAM lp )
{
	if ( msg == WM_COMMAND )
	{
		NMHDR nm;
		nm.hwndFrom = wnd;
		nm.idFrom = MAKEID( ::GetMenu( wnd ), LOWORD( wp ) );
		nm.code = MAKEID( HIWORD(wp), msg );
		::SendMessage( ::GetParent( ::GetParent( wnd ) ), WM_NOTIFY, (WPARAM) ::GetMenu( wnd ), (LPARAM)&nm );
	}
	else if ( msg == WM_HSCROLL )
	{
		NMHDR nm;
		nm.hwndFrom = wnd;
		nm.idFrom = lp;
		nm.code = msg;
		::SendMessage( ::GetParent( ::GetParent( wnd ) ), WM_NOTIFY, (WPARAM) ::GetMenu( wnd ), (LPARAM)&nm );
	}

	return 0;
}

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	uSendDlgItemMessage( IDC_ENABLE, BM_SETCHECK, cfg_enable );

	CWindow hTab = GetDlgItem( IDC_TAB );
	CWindow w;
	UINT n, count;
	uTCITEM it =
	{
		TCIF_TEXT,
		0, 0,
		0,
		0,
		-1, 0
	};

	for(n = 0; n < tabsize(tab_names); n++)
	{
		it.pszText = (LPSTR)tab_names[n];
		uTabCtrl_InsertItem( hTab, n, &it );
	}
	::SendMessage( hTab, TCM_SETCURFOCUS, 0, 0 );

	RECT r;
	hTab.GetClientRect( &r );
	TabCtrl_AdjustRect( hTab, 0, &r );
	if ( r.left == 4 ) r.left = 2; // bleh

	for (n = 0; n < tabsize(tab_names); n++)
	{
		w = ::CreateDialog( core_api::get_my_instance(), MAKEINTRESOURCE(IDD_CONFIG1 + n), hTab, ChildProc);
		w.SetWindowPos( 0, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER );
		w.SetWindowLong( GWL_ID, (LONG)( IDC_CONFIG1 + n ) );
	}

	hTab.GetDlgItem( IDC_CONFIG1 ).ShowWindow( SW_SHOW );

	pfc::array_t<pfc::string_simple> names;
	g_osd.get_names(names);

	w = GetDlgItem( IDC_OVERLAY );

	for (n = 0, count = names.get_size(); n < count; n++)
	{
		uSendMessageText( w, CB_ADDSTRING, 0, names[n] );
	}
	::SendMessage( w, CB_SETCURSEL, 0, 0 );

	backup.set_count( names.get_size() );

	for (n = 0; n < count; n++)
	{
		g_osd.get( n, backup [n] );
	}
	ctx.n_preset = 0;
	ctx.preset = backup [0];

	on_count( names.get_size(), false );

	load( ctx.preset );

	ctx.initialized = true;

	modified = false;

	enabled = !!cfg_enable;

	return FALSE;
}

LRESULT CMyPreferences::OnNotify(int idCtrl, LPNMHDR pnmh)
{
	if ( idCtrl == IDC_TAB )
	{
		if ( pnmh->code == TCN_SELCHANGE )
		{
			UINT n, cur_tab;
			HWND hTab = pnmh->hwndFrom;
			cur_tab = ::SendMessage( hTab, TCM_GETCURSEL, 0, 0 );
			for( n = 0; n < tabsize(tab_names); n++ )
			{
				HWND w = ::GetDlgItem( hTab, IDC_CONFIG1 + n );
				::ShowWindow( w, (n == cur_tab) ? SW_SHOW : SW_HIDE );
			}
		}
	}
	else if ( idCtrl >= IDC_CONFIG1 && idCtrl <= IDC_CONFIG4 )
	{
		if ( LOWORD(pnmh->code) == WM_COMMAND )
		{
			if (ctx.initialized)
			{
				switch ( pnmh->idFrom )
				{
				case MAKEID(IDC_CONFIG1, IDC_FONT):
					{
						if ( ctx.preset.font.popup_dialog( m_hWnd ) )
						{
							on_modified();
						}
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_COLOR):
					{
						DWORD temp = ctx.preset.color;
						CUSTOMCOLORS meh = cfg_customcolors;
						if ( uChooseColor( &temp, m_hWnd, (DWORD*)&meh.colors ) )
						{
							ctx.preset.color = temp;
							on_modified();
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_BGCOLOR):
					{
						DWORD temp = ctx.preset.bgcolor;
						CUSTOMCOLORS meh = cfg_customcolors;
						if ( uChooseColor( &temp, m_hWnd, (DWORD*)&meh.colors ) )
						{
							ctx.preset.bgcolor = temp;
							on_modified();
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG3, IDC_TAGZHELP):
					standard_commands::main_titleformat_help();
					break;

				case MAKEID(IDC_CONFIG3, IDC_RESET):
					{
						CWindow w = GetDlgItem(IDC_TAB).GetDlgItem(IDC_CONFIG3).GetDlgItem(IDC_FORMAT);
						string_utf8_from_window fmt(w);
						if ( strcmp( fmt, default_format ) )
						{
							uSetWindowText( w, default_format );
							ctx.preset.format = default_format;
							on_modified();
						}
					}
					break;

					/*case MAKEID(IDC_CONFIG3, IDC_RESET_NEXT):
					{
						CWindow w = GetDlgItem(IDC_TAB).GetDlgItem(IDC_CONFIG3).GetDlgItem(IDC_FORMATNEXT);
						string_utf8_from_window fmt(w);
						if ( strcmp( fmt, default_format_next ) )
						{
							uSetWindowText( w, default_format_next );
							ctx.preset.formatnext = default_format_next;
							on_modified();
						}
					}
					break;*/

				case MAKEID(IDC_CONFIG3, IDC_CCP):
					{
						CWindow w = GetDlgItem(IDC_TAB).GetDlgItem(IDC_CONFIG3).GetDlgItem(IDC_CCF);
						pfc::string8_fastalloc temp;
						temp = string_utf8_from_window(w);
						const char * ptr = temp.get_ptr() + 5;
						DWORD color = strtoul(ptr, (char **) &ptr, 10);
						color |= strtoul(ptr + 1, (char **) &ptr, 10) << 8;
						color |= strtoul(ptr + 1, 0, 10) << 16;
						CUSTOMCOLORS meh = cfg_customcolors;
						if ( uChooseColor( &color, m_hWnd, (DWORD*)&meh.colors ) )
						{
							temp = "$rgb(";
							temp << pfc::format_int( color & 255 );
							temp.add_byte(',');
							temp << pfc::format_int( (color >> 8) & 255 );
							temp.add_byte(',');
							temp << pfc::format_int( (color >> 16) & 255 );
							temp.add_byte(')');
							uSetWindowText( w, temp );
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_POP):
				case MAKEID(IDC_CONFIG1, IDC_POP_PLAY):
				case MAKEID(IDC_CONFIG1, IDC_POP_PAUSE):
				case MAKEID(IDC_CONFIG1, IDC_POP_SEEK):
				case MAKEID(IDC_CONFIG1, IDC_POP_SWITCH):
				case MAKEID(IDC_CONFIG1, IDC_POP_DYNAMIC):
				case MAKEID(IDC_CONFIG1, IDC_POP_DYNAMIC_ALL):
				case MAKEID(IDC_CONFIG1, IDC_POP_VOLUME):
				case MAKEID(IDC_CONFIG1, IDC_POP_STOP):
				case MAKEID(IDC_CONFIG1, IDC_PERMANENT):
				case MAKEID(IDC_CONFIG1, IDC_INTERVAL):
				case MAKEID(IDC_CONFIG1, IDC_OUTLINE):
				case MAKEID(IDC_CONFIG1, IDC_ANTIALIAS):
				case MAKEID(IDC_CONFIG1, IDC_PLEFT):
				case MAKEID(IDC_CONFIG1, IDC_PCENTER):
				case MAKEID(IDC_CONFIG1, IDC_PRIGHT):
				case MAKEID(IDC_CONFIG1, IDC_ALEFT):
				case MAKEID(IDC_CONFIG1, IDC_ACENTER):
				case MAKEID(IDC_CONFIG1, IDC_ARIGHT):
				case MAKEID(IDC_CONFIG2, IDC_ALPHA):
				case MAKEID(IDC_CONFIG2, IDC_FADEINOUT):
				case MAKEID(IDC_CONFIG2, IDC_DISSOLVE):
					on_modified();
					break;

				case MAKEID(IDC_CONFIG1, IDC_TIME):
				case MAKEID(IDC_CONFIG1, IDC_POSX):
				case MAKEID(IDC_CONFIG1, IDC_POSY):
				case MAKEID(IDC_CONFIG2, IDC_FADETIME):
				case MAKEID(IDC_CONFIG2, IDC_DECAY):
				case MAKEID(IDC_CONFIG3, IDC_FORMAT):
				//case MAKEID(IDC_CONFIG3, IDC_FORMATNEXT):
				case MAKEID(IDC_CONFIG4, IDC_VWIDTH):
				case MAKEID(IDC_CONFIG4, IDC_VHEIGHT):
				case MAKEID(IDC_CONFIG4, IDC_VSTEPS):
				case MAKEID(IDC_CONFIG4, IDC_VMIN):
					if (HIWORD(pnmh->code) == EN_CHANGE) on_modified();
					break;
				}
			}
		}
		else if ( pnmh->code == WM_HSCROLL )
		{
			if ( ctx.initialized ) on_modified();
		}
	}

	return FALSE;
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	cfg_enable = SendDlgItemMessage( IDC_ENABLE, BM_GETCHECK );
	OnChanged();
}

void CMyPreferences::OnOverlayAdd(UINT, int, CWindow) {
	CWindow w;
	unsigned count;

	if (ctx.modified)
	{
		switch ( uMessageBox( m_hWnd, "The current preset has been modified. Do you want to save the changes (yes),\napply them to the new preset (no) or cancel?", "On-Screen Display configuration", MB_YESNOCANCEL | MB_ICONEXCLAMATION ) )
		{
		case IDYES:
			save( ctx.preset );
			g_osd.set( ctx.n_preset, ctx.preset );
			modified = true;
			ctx.preset = osd_config();
			break;

		case IDCANCEL:
			return;
		}
		GetDlgItem( IDC_OVERLAY_APPLY ).EnableWindow( FALSE );
		ctx.modified = false;
	}
	else ctx.preset = osd_config();

	count = g_osd.add( ctx.preset );

	w = GetDlgItem( IDC_OVERLAY );
	uSendMessageText( w, CB_ADDSTRING, 0, ctx.preset.name );
	::SendMessage( w, CB_SETCURSEL, ctx.n_preset = count - 1, 0 );
	on_count( count, ctx.modified );

	modified = true;

	OnChanged();
}

void CMyPreferences::OnOverlayRemove(UINT, int, CWindow) {
	unsigned count;

	if (ctx.modified)
	{
		GetDlgItem( IDC_OVERLAY_APPLY ).EnableWindow( FALSE );
		ctx.modified = false;
	}

	CWindow w = GetDlgItem( IDC_OVERLAY );
	::SendMessage( w, CB_DELETESTRING, ctx.n_preset, 0 );

	count = g_osd.del( ctx.n_preset );
	if ( count )
	{
		if (ctx.n_preset == count) ctx.n_preset--;
		g_osd.get( ctx.n_preset, ctx.preset );

		::SendMessage( w, CB_SETCURSEL, ctx.n_preset, 0 );

		ctx.initialized = false;
		load(ctx.preset);
		ctx.initialized = true;
	}

	on_count(count, ctx.modified);

	modified = true;

	OnChanged();
}

void CMyPreferences::OnOverlayRename(UINT, int, CWindow) {
	pfc::string8_fastalloc temp;
	temp = ctx.preset.name;

	if ( rename( temp ) )
	{
		ctx.preset.name = temp;
		g_osd.rename( ctx.n_preset, temp );

		CWindow w = GetDlgItem( IDC_OVERLAY );
		::SendMessage( w, CB_DELETESTRING, ctx.n_preset, 0 );
		uSendMessageText( w, CB_INSERTSTRING, ctx.n_preset, ctx.preset.name );
		::uSendMessage( w, CB_SETCURSEL, ctx.n_preset, 0 );

		modified = true;

		OnChanged();
	}
}

void CMyPreferences::OnOverlayApply(UINT, int, CWindow w) {
	w.EnableWindow( FALSE );
	if (ctx.modified)
	{
		save(ctx.preset);
		g_osd.set(ctx.n_preset, ctx.preset);
		ctx.modified = false;
		modified = true;
		OnChanged();
	}
}

void CMyPreferences::OnOverlayTest(UINT, int, CWindow) {
	g_osd.test( ctx.n_preset );
}

void CMyPreferences::OnOverlayChange(UINT, int, CWindow w) {
	unsigned n = w.SendMessage( CB_GETCURSEL );

	if (n != ctx.n_preset)
	{
		if (ctx.modified)
		{
			switch ( uMessageBox( m_hWnd, "The current preset has been modified. Do you want to save the changes?", "On-Screen Display configuration", MB_YESNOCANCEL | MB_ICONEXCLAMATION ) )
			{
			case IDCANCEL:
				w.SendMessage( CB_SETCURSEL, ctx.n_preset, 0 );
				return;

			case IDYES:
				save( ctx.preset );
				g_osd.set( ctx.n_preset, ctx.preset );
				modified = true;
				OnChanged();
				break;
			}
			GetDlgItem( IDC_OVERLAY_APPLY ).EnableWindow( FALSE );
			ctx.modified = false;
		}

		ctx.n_preset = n;
		g_osd.get( ctx.n_preset, ctx.preset );

		ctx.initialized = false;
		load(ctx.preset);
		ctx.initialized = true;
	}
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SendDlgItemMessage( IDC_ENABLE, BM_SETCHECK, 0 );
	g_osd.quit();
	cfg_enable = 0;
	g_osd.reset();
	modified = true;

	OnChanged();
}

void CMyPreferences::apply() {
	if ( ctx.modified )
	{
		GetDlgItem(IDC_OVERLAY_APPLY).EnableWindow( FALSE );
		save(ctx.preset);
		g_osd.set(ctx.n_preset, ctx.preset);
		ctx.modified = false;
	}

	modified = false;

	pfc::array_t<pfc::string_simple> names;
	g_osd.get_names( names );

	backup.set_count( names.get_size() );

	for (unsigned n = 0, count = names.get_size(); n < count; n++)
	{
		g_osd.get( n, backup [n] );
	}

	if ( enabled != cfg_enable )
	{
		if ( cfg_enable )
		{
			if (!g_osd.init()) SendDlgItemMessage( IDC_ENABLE, BM_SETCHECK, 0 );
			else enabled = true;
		}
		else
		{
			g_osd.quit();
			enabled = false;
		}
	}

	OnChanged();
}

bool CMyPreferences::HasChanged() {
	return modified || ctx.modified || enabled != cfg_enable;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

CMyPreferences::~CMyPreferences() {
	if ( !enabled && cfg_enable ) g_osd.quit();
	if ( modified )
	{
		while ( g_osd.del( 0 ) );
		for ( unsigned n = 0, count = backup.get_count(); n < count; n++ )
		{
			g_osd.add( backup[n] );
		}
	}
	if ( enabled && !cfg_enable )
	{
		if (!g_osd.init()) enabled = false;
	}
	cfg_enable = enabled;
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "On-Screen Display";}
	GUID get_guid() {
		// {4D3CC458-020A-409c-8DA2-A760D5E01E73}
		static const GUID guid = { 0x4d3cc458, 0x20a, 0x409c, { 0x8d, 0xa2, 0xa7, 0x60, 0xd5, 0xe0, 0x1e, 0x73 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_display;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_config_osd_factory;
