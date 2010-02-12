#include <foobar2000.h>

#include "../helpers/window_placement_helper.h"

#include "config.h"
#include "resource.h"

#include "monitor.h"

#include <gme/Music_Emu.h>

critical_section           lock;

class monitor_dialog *     dialog           = 0;

gme_t const*           emu              = 0;

bool                       changed_info     = false;
pfc::string8               path;
pfc::array_t<const char *> voice_names;

bool                       changed_controls = false;
int                        mute_mask        = 0;

static const GUID guid_cfg_placement = { 0x2ea726fb, 0x60b, 0x471c, { 0x99, 0x40, 0xdc, 0x9a, 0xf1, 0x4e, 0x51, 0x88 } };
static cfg_window_placement cfg_placement(guid_cfg_placement);

void monitor_start( gme_t * p_emu, const char * p_path )
{
	insync( lock );

	changed_info = true;

	emu = p_emu;
	path = p_path;

	voice_names.set_size( p_emu->voice_count() );
	for ( unsigned i = 0, j = p_emu->voice_count(); i < j; i++ )
		voice_names [i] = p_emu->voice_name( i );

	if ( cfg_control_override )
	{
		p_emu->set_tempo( static_cast<double> (cfg_control_tempo) / 10000 );
		p_emu->mute_voices( mute_mask );
		p_emu->ignore_silence();
	}
}

void monitor_update( gme_t * p_emu )
{
	insync( lock );

	if ( emu == p_emu )
	{
		if ( changed_controls )
		{
			changed_controls = false;

			bool enabled = !!cfg_control_override;
			double t = enabled ? ( static_cast<double> (cfg_control_tempo) / 10000 ) : 1;
			int mask = enabled ? mute_mask : 0;

			p_emu->set_tempo( t );
			p_emu->mute_voices( mask );
			p_emu->ignore_silence( enabled );
		}
	}
}

void monitor_stop( const gme_t * p_emu )
{
	insync( lock );

	if ( emu == p_emu )
	{
		emu = 0;

		changed_info = true;
		path = "";
		voice_names.set_size( 0 );
	}
}

class monitor_dialog
{
	HWND wnd;

	static BOOL CALLBACK g_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
	{
		monitor_dialog * ptr;

		if ( msg == WM_INITDIALOG )
		{
			ptr = reinterpret_cast<monitor_dialog *> (lp);
			uSetWindowLong( wnd, DWL_USER, lp );
		}
		else
		{
			ptr = reinterpret_cast<monitor_dialog *> ( uGetWindowLong( wnd, DWL_USER ) );
		}

		if ( ptr ) return ptr->dialog_proc( wnd, msg, wp, lp );
		else return 0;
	}

	BOOL dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
	{
		switch ( msg )
		{
		case WM_INITDIALOG:
			{
				this->wnd = wnd;

				uSendDlgItemMessage( wnd, IDC_OVERRIDE, BM_SETCHECK, cfg_control_override, 0 );

				HWND w = GetDlgItem( wnd, IDC_TEMPO_SLIDER );
				uSendMessage( w, TBM_SETRANGEMIN, FALSE, 2 );
				uSendMessage( w, TBM_SETRANGEMAX, FALSE, 400 * 100 );
				uSendMessage( w, TBM_SETPAGESIZE, 0, 1250 );
				uSendMessage( w, TBM_SETPOS, TRUE, cfg_control_tempo );
				/*for ( unsigned tick = 25 * 100; tick <= 400 * 100; tick += 25 * 100 )
					uSendMessage( w, TBM_SETTIC, 0, tick );*/

				{
					insync( lock );
					changed_info = false;
					changed_controls = false;
					update();
					update_tempo();
				}

				SetTimer( wnd, 0, 100, 0 );

				cfg_placement.on_window_creation(wnd);
			}
			return 1;

		case WM_TIMER:
			{
				insync( lock );
				if ( changed_info )
				{
					changed_info = false;
					update();
				}
			}
			break;

		case WM_DESTROY:
			{
				cfg_placement.on_window_destruction( wnd );
				KillTimer( wnd, 0 );
				uSetWindowLong( wnd, DWL_USER, 0 );
				delete this;
				dialog = 0;
			}
			break;

		case WM_COMMAND:
			if ( wp == IDCANCEL )
			{
				DestroyWindow( wnd );
			}
			else if ( wp == IDC_OVERRIDE )
			{
				insync( lock );

				cfg_control_override = uSendMessage((HWND)lp,BM_GETCHECK,0,0);

				BOOL enable = emu != 0 && cfg_control_override;

				EnableWindow( GetDlgItem( wnd, IDC_TEMPO_CAPTION ), enable );
				EnableWindow( GetDlgItem( wnd, IDC_TEMPO ), enable );
				EnableWindow( GetDlgItem( wnd, IDC_TEMPO_SLIDER ), enable );

				for ( unsigned i = 0, j = voice_names.get_size(); i < j; ++i )
				{
					EnableWindow( GetDlgItem( wnd, IDC_VOICE1 + i ), enable );
				}

				changed_controls = true;
			}
			else if ( wp == IDC_RESET )
			{
				insync( lock );

				changed_controls = ( cfg_control_tempo != 100 * 100 ) || ( mute_mask != 0 );
				cfg_control_tempo = 100 * 100;
				mute_mask = 0;

				if ( changed_controls )
				{
					uSendDlgItemMessage( wnd, IDC_TEMPO_SLIDER, TBM_SETPOS, TRUE, cfg_control_tempo );
					update();
					update_tempo();
				}
			}
			else if ( wp - IDC_VOICE1 < 30 )
			{
				unsigned voice = wp - IDC_VOICE1;
				unsigned mask = ~(1 << voice);
				unsigned bit = uSendMessage((HWND)lp,BM_GETCHECK,0,0) ? 0 : ( 1 << voice );

				insync( lock );

				changed_controls = true;
				mute_mask = ( mute_mask & mask ) | bit;
			}
			break;

		case WM_HSCROLL:
			{
				unsigned t = uSendMessage((HWND)lp,TBM_GETPOS,0,0);
				if ( t < 2 ) t = 2;
				else if ( t > 400 * 100 ) t = 400 * 100;

				insync( lock );
				
				changed_controls = true;
				cfg_control_tempo = t;
				update_tempo();
			}
			break;
		}

		return 0;
	}

	void update()
	{
		pfc::string8 title;
		if ( path.length() )
		{
			title = pfc::string_filename_ext( path );
			title += " - ";
		}
		title += "Game Emu Player";
		uSetWindowText( wnd, title );

		BOOL enable = emu != 0 && cfg_control_override;

		EnableWindow( GetDlgItem( wnd, IDC_TEMPO_CAPTION ), enable );
		EnableWindow( GetDlgItem( wnd, IDC_TEMPO ), enable );
		EnableWindow( GetDlgItem( wnd, IDC_TEMPO_SLIDER ), enable );

		HWND w;
		unsigned count = voice_names.get_size();
		for ( unsigned i = 0; i < count; ++i )
		{
			w = GetDlgItem( wnd, IDC_VOICE1 + i );
			uSendMessage( w, BM_SETCHECK, ! ( ( mute_mask >> i ) & 1 ) , 0 );
			uSetWindowText( w, voice_names[ i ] );
			EnableWindow( w, enable );
			ShowWindow( w, SW_SHOWNA );
		}
		for ( unsigned i = count; i < 30; ++i )
		{
			w = GetDlgItem( wnd, IDC_VOICE1 + i );
			uSendMessage( w, BM_SETCHECK, ! ( ( mute_mask >> i ) & 1 ) , 0 );
			uSetWindowText( w, "" );
			EnableWindow( w, 0 );
			ShowWindow( w, SW_HIDE );
		}
	}

	void update_tempo()
	{
		pfc::string8 text;
		int tempo = cfg_control_tempo;
		text << pfc::format_int( tempo / 100 ) << "." << pfc::format_int( tempo % 100, 2 ) << "%";
		uSetDlgItemText( wnd, IDC_TEMPO, text );
	}

public:
	monitor_dialog( HWND parent )
	{
		wnd = 0;
		if ( ! CreateDialogParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_MONITOR), parent, g_dialog_proc, reinterpret_cast<LPARAM> (this) ) )
			throw exception_win32( GetLastError() );
	}

	~monitor_dialog()
	{
		DestroyWindow( wnd );
	}
};

class monitor_menu : public mainmenu_commands
{
	virtual t_uint32 get_command_count()
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index)
	{
		// {ED5CA903-E477-48dd-9421-7E530B48FAF5}
		static const GUID guid = 
		{ 0xed5ca903, 0xe477, 0x48dd, { 0x94, 0x21, 0x7e, 0x53, 0xb, 0x48, 0xfa, 0xf5 } };
		return guid;
	}
	
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "GEP control";
	}

	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "Activates the Game Emu Player advanced controls window.";
		return true;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::view;
	}

	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags)
	{
		p_flags = 0;
		get_name(p_index,p_text);
		return true;
	}

	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		if ( p_index == 0 && core_api::assert_main_thread() )
		{
			if ( !dialog )
			{
				try
				{
					dialog = new monitor_dialog( core_api::get_main_window() );
				}
				catch ( const std::exception & e )
				{
					dialog = 0;
					console::error( e.what() );
				}
			}
		}
	}
};

static mainmenu_commands_factory_t <monitor_menu> g_mainmenu_commands_monitor_factory;