/*

   Copyright (C) 2003-2004, Chris Moeller,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#define VERSION "1.68"

/*
#define GDIPLUS
	^ this is defined by the gdiplus project, which I created so I don't have two build targets instead
*/

/*
	change log

2010-11-23 06:36 UTC - kode54
- Menu commands, and thus keyboard shortcuts, are back. Although I don't know why the dynamic
  commands don't appear in the UI's main menu.
- Version is now 1.68

2010-10-27 23:34 UTC - kode54
- Fixed hide until idle constantly popping the overlay up
- Version is now 1.67

2010-10-26 22:44 UTC - kode54
- Fixed hide until idle to stop idle timer when the overlay is manually or trigger hidden
- Version is now 1.66

2010-10-26 02:22 UTC - kode54
- Fixed hide until idle to correctly hide visible overlays when the system is no longer idle
- Version is now 1.65

2010-10-25 04:48 UTC - kode54
- Implemented idle timeout display control
- Version is now 1.64

2010-10-23 19:39 UTC - kode54
- Changed preferences dialog to not crash when started with no presets loaded
- Removed next track formatting from default title formatting script
- Version is now 1.63

2010-09-27 20:26 UTC - kode54
- Changed overlay to position itself relative to the entire virtual desktop instead
  of just the primary desktop
- Version is now 1.62

2010-04-13 14:58 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 1.61

2010-01-11 19:34 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.6

2005-11-28 12:40 UTC - kode54
- Fixed stupid typo in menu_osd_control::item_execute_simple guid from guid mess
- Same in menu_item_osd::set_data!
- Version is now 1.52

2005-05-12 06:34 UTC - kode54
- Fixed volume/test color retrieval for $rgb()
- Fixed position/align left/right reading backwards in config_interface/save()
- Version is now 1.51

2005-05-12 05:34 UTC - kode54
- Updated to a25 / beta 1 SDK
- Fixed context->n_preset when adding a new preset
- Changed formatting color boxes to use $rgb()

2005-02-13 04:37 UTC - kode54
- Completed configuration dialog
- Reorganized a few bits into separate files
- Version is now 1.50

2005-02-10 08:04 UTC - kode54
- Completed internal side of multiple dynamic overlay system, except for the glue for the interactive functions to come

2004-04-25 00:18 UTC - kode54
- Changed OSD window destructor a bit, should fix crashes when OSD is fading or dissolving
- Version is now 1.29.2

2004-04-13 11:16 UTC - kode54
- Removed unnecessary registration with modeless dialog manager
- Version is now 1.29.1

2004-02-23 21:01 UTC - kode54
- Changed menu class to menu_item_v2 and added GUIDs for all
- Version is now 1.29

2003-11-01 05:02 UTC - kode54
- Fixed stupid bug with GDI+ version
- Version is now 1.28

2003-10-23 07:37 UTC - kode54
- Added menu action to hide the overlay
- Version is now 1.27

2003-10-17 11:52 UTC - kode54
- Changed the dialog init to use no paired dlgitemmessage mess
- Changed the horizontal sliders a bit

2003-10-11 06:20 UTC - kode54
- Added get_callback_mask, as if it will do anything special
- Updated interval to use on_playback_time
- Version is now 1.26

2003-09-30 22:25 UTC - kode54
- Added a pop-up test button to the configuration dialog
- Version is now 1.25

2003-09-29 07:08 UTC - kode54
- Added permanent display option
- Version is now 1.24

2003-09-26 11:54 UTC - kode54
- Changed how normal GDI volume bar is drawn, should be faster now

2003-09-25 04:07 UTC - kode54
- %_next% formatting is now configurable
- Version is now 1.23a

2003-09-25 00:02 UTC - kode54
- Removed some unnecessary service_release() calls
- Added %_next% to display the next song, based on current formatting, sans color codes
- Version is now 1.23

2003-09-23 14:01 UTC - kode54
- Volume bar now has configurable minimum dB level
- Version is now 1.22

2003-09-23 13:38 UTC - kode54
- Volume bar size and step count are now configurable
- Version is now 1.21b

2003-09-23 11:24 UTC - kode54
- Fixed retarded bug with new menu_item enum list
- Version is now 1.21a

2003-09-23 09:36 UTC - kode54
- Added volume control change event
- Added volume control display menu item
- Added volume bar renderers and display function to OSD class
- Version is now 1.21

2003-09-22 04:29 UTC - kode54
- Removed some debugging code

2003-09-22 03:37 UTC - kode54
- Added on-seek event, may draw some seekbar shit in the future
- Changed interval behavior, ie. no updates when paused or displaying playlist name
- Fixed window placement correction
- Version is now 1.20

2003-09-21 19:57 UTC - kode54
- Fixed lame typo with disabling anti-aliasing in vanilla GDI build
- Version is now 1.19a

2003-09-21 18:42 UTC - kode54
- Added interval update
- Version is now 1.19

2003-09-21 18:16 UTC - kode54
- Added alternate RGBA code based on standard GDI
- Comparison shows that GDI+ has smoother anti-aliasing, but blurrier inner text
- Plus sign appended to version number for GDI+ builds

2003-09-21 12:03 UTC - kode54
- Oops, was calling ReleaseDC on created DC's when I should have used DeleteDC - Pays to RTFMSDN

2003-09-21 10:32 UTC - kode54
- Whew! Full alpha text rendering c/o GDI+
- Color key transparency eliminated
- Background transparency is now variable
- Version is now 1.18

2003-09-20 23:19 UTC - kode54
- Overhauled to use UpdateLayeredWindow
- Version is now 1.17

2003-09-19 11:31 UTC - kode54
- Added automatic event for dynamic info on on track change
- Version is now 1.16

2003-09-17 07:18 UTC - kode54
- Abbreviated On-Screen Display to OSD in display items
- Added show-current-playlist menu item

2003-09-17 06:40 UTC - kode54
- Added on-playlist-switch event
- Added separate pop-up dialog for event toggles
- Version is now 1.15

2003-09-14 17:05 UTC - kode54
- Widened fade duration editbox, increased limit to 5000ms
- Version is now 1.14

2003-09-14 02:57 UTC - kode54
- UpdateWindow() is only called on hidden/fading out posts
- Version is now 1.13

2003-09-14 01:25 UTC - kode54
- Now Post() will not be called on blank strings, ie. hotkey when nothing is playing

2003-09-13 23:53 UTC - kode54
- Fading state won't be affected by successive Post() calls while the window is still visible
- Tweaked Reset() and NewFont()
- Version is now 1.12

2003-09-13 06:15 UTC - kode54
- Fixed issue with background color updating when color key is disabled
- Version is now 1.11

2003-09-13 05:38 UTC - kode54
- Changed color key attribute reset
- Special surprise for those non-color-key users

2003-09-13 04:11 UTC - kode54
- Fixed retardedness in fading maths; I should code only when I've had proper sleep

2003-09-12 12:10 UTC - kode54
- Added fade in/out
- Version is now 1.10

2003-09-11 01:08 UTC - kode54
- Key/background color is now configurable
- Version is now 1.09

2003-09-11 00:31 UTC - kode54
- Color key transparency is now optional
- Renamed alpha checkbox label to Translucency

2003-09-04 10:37 UTC - kode54
- Added optional un/pause notification
- Version is now 1.08

2003-09-04 09:50 UTC - kode54
- Made cfg_pop toggle menu show a check mark instead of Enable/Disable

2003-09-04 06:50 UTC - kode54
- Added hotkey to toggle cfg_pop
- Version is now 1.07

2003-09-03 22:04 UTC - kode54
- Added hotkey support, and made auto pop-up optional, separate from enabling the OSD
- Position relative to text bounding box depends on alignment
- Text alignment is now separate from window position alignment
- Version is now 1.06

2003-09-03 21:37 UTC - kode54
- cfg_x/cfg_y are now percentages of the desktop size, relative to the center of the text window
- Text window is sized to the bounding box necessary to render the text
- Left/center/right text alignment
- Version is now 1.05

2003-09-03 13:04 UTC - kode54
- Window height is now calculated from the text height
- Version is now 1.04

2003-09-03 12:06 UTC - kode54
- Added multi-line rendering shit
- Version is now 1.03

2003-09-03 08:38 UTC - kode54
- Oops, fixed NewFont() call when g_osd doesn't exist
- Added osdsync to on_quit(), just in case
- Version is now 1.02

2003-09-03 07:57 UTC - kode54
- Added enable toggle, disables config if it fails to initialize COsdWnd
- Version is now 1.01

2003-09-03 03:43 UTC - kode54
- Initial release
- Version is now 1.0

*/

#define _WIN32_WINNT 0x500

#ifdef GDIPLUS
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>

#ifdef GDIPLUS
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")
#endif

#include <foobar2000.h>
#include "../helpers/window_placement_helper.h"

#include "config.h"
#include "overlay.h"
#include "main.h"
#include "config_interface.h"

class initquit_osd : public initquit
{
#ifdef GDIPLUS
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
#endif

	void on_init()
	{
#ifdef GDIPLUS
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif

		if (!!cfg_enable)
		{
			g_osd.init();
		}
	}

	void on_quit()
	{
		g_osd.quit();

#ifdef GDIPLUS
		GdiplusShutdown(gdiplusToken);
#endif
	}
};

class play_callback_osd : public play_callback
{
public:
	void FB2KAPI on_playback_starting(play_control::t_track_command,bool) {}
	void FB2KAPI on_playback_edited(metadb_handle_ptr) {}

	void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
	{
		g_osd.on_playback_new_track(p_track);
	}

	void FB2KAPI on_playback_stop(play_control::t_stop_reason)
	{
		g_osd.on_playback_stop();
	}

	void FB2KAPI on_playback_seek(double)
	{
		g_osd.on_playback_seek();
	}

	void FB2KAPI on_playback_pause(bool p_state)
	{
		g_osd.on_playback_pause(p_state);
	}

	void FB2KAPI on_playback_dynamic_info(const file_info &)
	{
		g_osd.on_playback_dynamic_info(false);
	}

	void FB2KAPI on_playback_dynamic_info_track(const file_info &)
	{
		g_osd.on_playback_dynamic_info(true);
	}

	void FB2KAPI on_playback_time(double)
	{
		g_osd.on_playback_time();
	}

	void FB2KAPI on_volume_change(float p_new_val)
	{
		g_osd.on_volume_change(p_new_val * 100.f);
	}
};

class playlist_callback_osd : public playlist_callback
{
public:
	void FB2KAPI on_items_added(unsigned,unsigned,const pfc::list_base_const_t<metadb_handle_ptr> &,const bit_array &) {}
	void FB2KAPI on_items_reordered(unsigned,const unsigned *,unsigned) {}
	void FB2KAPI on_items_removing(unsigned,const bit_array &,unsigned,unsigned) {}
	void FB2KAPI on_items_removed(unsigned,const bit_array &,unsigned,unsigned) {}
	void FB2KAPI on_items_selection_change(unsigned,const bit_array &,const bit_array &) {}
	void FB2KAPI on_item_focus_change(unsigned,unsigned,unsigned) {}

	void FB2KAPI on_items_modified(unsigned,const bit_array &) {}
	void FB2KAPI on_items_modified_fromplayback(unsigned,const bit_array &,play_control::t_display_level) {}

	void FB2KAPI on_items_replaced(unsigned,const bit_array &,const pfc::list_base_const_t<t_on_items_replaced_entry> &) {}

	void FB2KAPI on_item_ensure_visible(unsigned,unsigned) {}

	void FB2KAPI on_playlist_created(unsigned,const char *,unsigned) {}
	void FB2KAPI on_playlists_reorder(const unsigned *,unsigned) {}
	void FB2KAPI on_playlists_removing(const bit_array &,unsigned,unsigned) {}
	void FB2KAPI on_playlists_removed(const bit_array &,unsigned,unsigned) {}
	void FB2KAPI on_playlist_renamed(unsigned,const char *,unsigned) {}

	void FB2KAPI on_default_format_changed() {}
	void FB2KAPI on_playback_order_changed(unsigned) {}
	void FB2KAPI on_playlist_locked(unsigned,bool) {}

	void FB2KAPI on_playlist_activate(unsigned,unsigned)
	{
		g_osd.on_playlist_switch();
	}
};

play_callback_osd     g_play_callback_osd;
playlist_callback_osd g_playlist_callback_osd;

void g_callback_register()
{
	unsigned play_flags, playlist_flags;
	g_osd.get_callback_flags( play_flags, playlist_flags );
	static_api_ptr_t<play_callback_manager>()->register_callback( & g_play_callback_osd, play_flags, false );
	static_api_ptr_t<playlist_manager>()->register_callback( & g_playlist_callback_osd, playlist_flags );
}

void g_callback_modify()
{
	unsigned play_flags, playlist_flags;
	g_osd.get_callback_flags( play_flags, playlist_flags );
	static_api_ptr_t<play_callback_manager> p_pcm;
	p_pcm->unregister_callback( & g_play_callback_osd );
	p_pcm->register_callback( & g_play_callback_osd, play_flags, false );
	static_api_ptr_t<playlist_manager>()->modify_callback( & g_playlist_callback_osd, playlist_flags );
}

void g_callback_unregister()
{
	static_api_ptr_t<play_callback_manager>()->unregister_callback( & g_play_callback_osd );
	static_api_ptr_t<playlist_manager>()->unregister_callback( & g_playlist_callback_osd );
}

static initquit_factory_t<initquit_osd> g_initquit_osd_factory;

#define NAME "On-Screen Display"

#ifdef GDIPLUS
#define NSTRING NAME " GDI+"
#define VSTRING VERSION
#else
#define NSTRING NAME
#define VSTRING VERSION
#endif

DECLARE_COMPONENT_VERSION(NSTRING,VSTRING,"Displays a brief pop-up notice whenever a song starts to play.");

VALIDATE_COMPONENT_FILENAME("foo_osd.dll");
