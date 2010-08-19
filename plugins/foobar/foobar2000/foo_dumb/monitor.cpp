#include <foobar2000.h>

#include "../helpers/window_placement_helper.h"

#include "resource.h"

#include "monitor.h"

#include "dumb.h"

extern "C" {
#include "internal/it.h"
}

critical_section           lock;

class monitor_dialog *     dialog           = 0;

DUMB_IT_SIGDATA const*     song_data        = 0;
DUMB_IT_SIGRENDERER *      song_renderer    = 0;

bool                       changed_info     = false;
pfc::string8               path;
t_uint64                   channels_allowed = 0;

bool                       changed_controls = false;
t_uint64                   mute_mask        = 0;

static const GUID guid_cfg_placement = { 0xfdbfa6fe, 0xb913, 0x4390, { 0x91, 0x37, 0xc0, 0x2d, 0xd5, 0x74, 0x63, 0xe9 } };
static cfg_window_placement cfg_placement(guid_cfg_placement);

static const GUID guid_cfg_control_override = { 0x1a93ede5, 0xb2d1, 0x4a35, { 0xac, 0xa6, 0x7a, 0x29, 0xe2, 0xf2, 0xca, 0x33 } };
static cfg_int cfg_control_override( guid_cfg_control_override, 0 );

static void initialize_channels()
{
	channels_allowed = 0;

	if ( song_data )
	for ( int order = 0; order < song_data->n_orders; order++ )
	{
		int pattern = song_data->order[ order ];
		if ( pattern < song_data->n_patterns )
		{
			IT_PATTERN * the_pattern = &song_data->pattern[ pattern ];
			IT_ENTRY * entry = the_pattern->entry;
			for ( int entry_number = 0; entry_number < the_pattern->n_entries; entry_number++ )
			{
				if ( entry[ entry_number ].mask & IT_ENTRY_NOTE )
				{
					channels_allowed |= t_uint64(1) << entry[ entry_number ].channel;
				}
			}
		}
	}
}

static void mute_channels( t_uint64 mask )
{
	if ( song_renderer )
	for ( int channel = 0; channel < DUMB_IT_N_CHANNELS; channel++ )
	{
		t_uint64 current_mask = 1 << channel;
		if ( current_mask & channels_allowed )
		{
			int muted = !!(mask & current_mask);
			dumb_it_sr_set_channel_muted( song_renderer, channel, muted );
		}
	}
}

void monitor_start( DUMB_IT_SIGDATA * p_sigdata, DUMB_IT_SIGRENDERER * p_sigrenderer, const char * p_path )
{
	insync( lock );

	changed_info = true;

	song_data = p_sigdata;
	song_renderer = p_sigrenderer;
	path = p_path;

	if ( cfg_control_override )
	{
		mute_channels( mute_mask );
	}
}

void monitor_update( DUMB_IT_SIGRENDERER * p_sigrenderer )
{
	insync( lock );

	if ( song_renderer == p_sigrenderer )
	{
		if ( changed_controls )
		{
			changed_controls = false;

			bool enabled = !!cfg_control_override;
			t_uint64 mask = enabled ? mute_mask : 0;

			mute_channels( mask );
		}
	}
}

void monitor_stop( const DUMB_IT_SIGRENDERER * p_sigrenderer )
{
	insync( lock );

	if ( song_renderer == p_sigrenderer )
	{
		song_data = 0;
		song_renderer = 0;

		changed_info = true;
		path = "";
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

				{
					insync( lock );
					changed_info = false;
					changed_controls = false;
					update();
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

				BOOL enable = song_renderer != 0 && cfg_control_override;

				for ( unsigned i = 0, j = DUMB_IT_N_CHANNELS; i < j; ++i )
				{
					EnableWindow( GetDlgItem( wnd, IDC_VOICE1 + i ), enable );
				}

				changed_controls = true;
			}
			else if ( wp == IDC_RESET )
			{
				insync( lock );

				changed_controls = mute_mask != 0;
				mute_mask = 0;

				if ( changed_controls )
				{
					update();
				}
			}
			else if ( wp - IDC_VOICE1 < DUMB_IT_N_CHANNELS )
			{
				unsigned voice = wp - IDC_VOICE1;
				t_uint64 mask = ~(1 << voice);
				t_uint64 bit = uSendMessage((HWND)lp,BM_GETCHECK,0,0) ? 0 : ( 1 << voice );

				insync( lock );

				changed_controls = true;
				mute_mask = ( mute_mask & mask ) | bit;
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
		title += "DUMB";
		uSetWindowText( wnd, title );

		BOOL enable = song_renderer != 0 && cfg_control_override;

		initialize_channels();

		HWND w;
		for ( unsigned i = 0; i < DUMB_IT_N_CHANNELS; ++i )
		{
			w = GetDlgItem( wnd, IDC_VOICE1 + i );
			uSendMessage( w, BM_SETCHECK, ! ( ( mute_mask >> i ) & 1 ) , 0 );
			EnableWindow( w, enable );
			ShowWindow( w, ( ( t_uint64(1) << i ) & channels_allowed ) ? SW_SHOWNA : SW_HIDE );
		}
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
		// {3603B091-2661-4AB4-875D-4FDA4565BE64}
		static const GUID guid = 
		{ 0x3603b091, 0x2661, 0x4ab4, { 0x87, 0x5d, 0x4f, 0xda, 0x45, 0x65, 0xbe, 0x64 } };
		return guid;
	}
	
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "DUMB control";
	}

	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "Activates the DUMB advanced controls window.";
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
