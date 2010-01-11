#define MY_VERSION "1.4"

/*
	change log

2004-12-03 12:36 UTC - kode54
- Updated to UI Extension API v5.0.0
- Removed scrollwheel hook
- Version is now 1.4

2004-10-22 15:10 UTC - kode54
- Fixed lame bug in WM_INITDIALOG cfg_which_mixer range checking
- Version is now 1.32

2004-10-21 19:07 UTC - kode54
- Added error message logging to all mixer function calls
- Version is now 1.31

2004-10-19 11:00 UTC - kode54
- Added message hook mess to top level parent window to steal WM_MOUSEWHEEL messages for the volume control tool
- Version is now 1.3

2004-10-16 04:06 UTC - kode54
- Fixed a bug in the trackbar WM_MOUSEMOVE override which would cause it to parse redundant messages
- Version is now 1.21

2004-10-15 09:04 UTC - kode54
- Internal volume control is adjusted on a logarithmic scale
- Mute zeroes temp storage on restore, so it won't restore odd volumes if user manually lowers volume to zero
- Version is now 1.2

2004-10-14 23:10 UTC - kode54
- Added menu_item service for hotkeys, to control the external mixer
- Version is now 1.1

2004-10-14 20:42 UTC - kode54
- Implemented pointer tracking subclassing of trackbar control, mercilessly ripped from foo_ui_columns' seekbar
- Implemented volume info tooltip on trackbar control, ditto
- Version is now 1.0

2004-10-14 10:03 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

// for pexchange, which is fastcall; InterlockedExchange is stdcall
#include <ptypes.h>
USING_PTYPES

// for uFormatMessage
#include "../ui_extension/utf8api.h"

/* volume.cpp */
void g_update_controls(int volume);

/* mixer.cpp */
int g_db_to_vol(int db);
void g_change_mixer(unsigned mixer, unsigned control);
int g_get_volume();
void g_set_volume(int volume);
BOOL uFormatMediaMessage(DWORD dw_error, string_base & out);

cfg_int cfg_which_mixer("mixer_id", 0);
cfg_int cfg_mixer_control("mixer_control", 0);

cfg_int cfg_mixer_mute_restore("mixer_mute_level", 0);

#include "resource.h"

class mixer_monitor : public play_callback
{
	int volume;

public:
	mixer_monitor() : volume(31337) {}

	virtual void on_playback_starting() {}
	virtual void on_playback_new_track(metadb_handle * track) {}
	virtual void on_playback_stop(play_control::stop_reason reason) {}
	virtual void on_playback_seek(double time) {}
	virtual void on_playback_pause(int state) {}
	virtual void on_playback_edited(metadb_handle * track) {}
	virtual void on_playback_dynamic_info(const file_info * info,bool b_track_change) {}

	virtual unsigned get_callback_mask() {return MASK_on_volume_change;};

	virtual void on_volume_change(int new_val)
	{
		if (cfg_which_mixer == 0)
		{
			if (pexchange(&volume, new_val) != new_val) g_update_controls(g_db_to_vol(new_val));
		}
		else
			volume = 31337; // always pull in the next modification if user switches back to internal volume
	}
};

class mixer_config : public config
{
	static void CALLBACK PopulateControls(HWND wnd, DWORD device)
	{
		if (device) uSendDlgItemMessage(wnd, IDC_CONTROL, CB_RESETCONTENT, 0, 0);

		ShowWindow(uGetDlgItem(wnd, IDC_CONTROL_TEXT), !!device);
		ShowWindow(uGetDlgItem(wnd, IDC_CONTROL), !!device);

		ShowWindow(uGetDlgItem(wnd, IDC_INTERNAL_TEXT), !device);

		MMRESULT ret;

		if (device)
		{
			HWND w = uGetDlgItem(wnd, IDC_CONTROL);

			UINT id;
			if ((ret = mixerGetID((HMIXEROBJ)(device-1), &id, MIXER_OBJECTF_MIXER)) != MMSYSERR_NOERROR)
			{
				string8 error;
				uFormatMediaMessage(ret, error);
				console::warning(uStringPrintf("mixerGetID failed - %s", pconst(error)));
				return;
			}

			if (IsUnicode())
			{
				MIXERLINEW mxl;
				memset(&mxl, 0, sizeof(mxl));
				mxl.cbStruct = sizeof(mxl);
				mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

				if ((ret = mixerGetLineInfoW((HMIXEROBJ)id, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_MIXER)) != MMSYSERR_NOERROR)
				{
					string8 error;
					uFormatMediaMessage(ret, error);
					console::warning(uStringPrintf("Failed to query mixer line info for master volume - %s", pconst(error)));
					return;
				}

				SendMessageW(w, CB_ADDSTRING, 0, (LPARAM)&mxl.szName);

				UINT con;

				for (con = 0; con < mxl.cConnections; con++)
				{
					MIXERLINEW mxl1;
					memset(&mxl1, 0, sizeof(mxl1));
					mxl1.cbStruct = sizeof(mxl1);
					mxl1.dwSource = con;
					if ((ret = mixerGetLineInfoW((HMIXEROBJ)id, &mxl1, MIXER_GETLINEINFOF_SOURCE | MIXER_OBJECTF_MIXER)) != MMSYSERR_NOERROR)
					{
						string8 error;
						uFormatMediaMessage(ret, error);
						console::warning(uStringPrintf("Failed to query mixer line info for source control - %s", pconst(error)));
						return;
					}

					SendMessageW(w, CB_ADDSTRING, 0, (LPARAM)&mxl1.szName);
				}

				if ((DWORD)(int)cfg_mixer_control > mxl.cConnections)
				{
					cfg_mixer_control = 0;
					g_change_mixer(cfg_which_mixer, 0);
				}
			}
			else
			{
				MIXERLINEA mxl;
				memset(&mxl, 0, sizeof(mxl));
				mxl.cbStruct = sizeof(mxl);
				mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

				if (mixerGetLineInfoA((HMIXEROBJ)id, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_MIXER) != MMSYSERR_NOERROR)
				{
					string8 error;
					uFormatMediaMessage(ret, error);
					console::warning(uStringPrintf("Failed to query mixer line info for master volume - %s", pconst(error)));
					return;
				}

				SendMessageA(w, CB_ADDSTRING, 0, (LPARAM)&mxl.szName);

				UINT con;

				for (con = 0; con < mxl.cConnections; con++)
				{
					MIXERLINEA mxl1;
					memset(&mxl1, 0, sizeof(mxl1));
					mxl1.cbStruct = sizeof(mxl1);
					mxl1.dwSource = con;
					if (mixerGetLineInfoA((HMIXEROBJ)id, &mxl1, MIXER_GETLINEINFOF_SOURCE | MIXER_OBJECTF_MIXER) != MMSYSERR_NOERROR)
					{
						string8 error;
						uFormatMediaMessage(ret, error);
						console::warning(uStringPrintf("Failed to query mixer line info for source control - %s", pconst(error)));
						return;
					}

					SendMessageA(w, CB_ADDSTRING, 0, (LPARAM)&mxl1.szName);
				}

				if ((DWORD)(int)cfg_mixer_control > mxl.cConnections)
				{
					cfg_mixer_control = 0;
					g_change_mixer(cfg_which_mixer, 0);
				}
			}
			
			uSendMessage(w, CB_SETCURSEL, cfg_mixer_control, 0);
		}
	}

	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				HWND w;

				w = GetDlgItem(wnd, IDC_MIXER);
				uSendMessageText(w, CB_ADDSTRING, 0, "Foobar2000 internal");

				DWORD n_devs = mixerGetNumDevs();

				if (n_devs)
				{
					MMRESULT ret;

					if (IsUnicode())
					{
						MIXERCAPSW mxcaps;
						for (DWORD i = 0; i < n_devs; i++)
						{
							if ((ret = mixerGetDevCapsW(i, &mxcaps, sizeof(mxcaps))) == MMSYSERR_NOERROR)
							{
								SendMessageW(w, CB_ADDSTRING, 0, (LPARAM)&mxcaps.szPname);
							}
							else
							{
								string8 error;
								uFormatMediaMessage(ret, error);
								uSendMessageText(w, CB_ADDSTRING, 0, "");
								console::warning(uStringPrintf("Unable to query mixer info for ID %u - %s", i, pconst(error)));
							}
						}
					}
					else
					{
						MIXERCAPSA mxcaps;
						for (DWORD i = 0; i < n_devs; i++)
						{
							if ((ret = mixerGetDevCapsA(i, &mxcaps, sizeof(mxcaps))) == MMSYSERR_NOERROR)
							{
								SendMessageA(w, CB_ADDSTRING, 0, (LPARAM)&mxcaps.szPname);
							}
							else
							{
								string8 error;
								uFormatMediaMessage(ret, error);
								uSendMessageText(w, CB_ADDSTRING, 0, "");
								console::warning(uStringPrintf("Unable to query mixer info for ID %u - %s", i, pconst(error)));
							}
						}
					}
				}

				if ((DWORD)(int)cfg_which_mixer > n_devs)
				{
					cfg_which_mixer = 0;
					g_change_mixer(0, 0);
				}

				uSendMessage(w, CB_SETCURSEL, cfg_which_mixer, 0);

				PopulateControls(wnd, cfg_which_mixer);
			}
			break;

		case WM_COMMAND:
			switch (wp)
			{
			case (CBN_SELCHANGE<<16) | IDC_MIXER:
				cfg_which_mixer = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
				cfg_mixer_control = 0;
				g_change_mixer(cfg_which_mixer, 0);
				PopulateControls(wnd, cfg_which_mixer);
				break;
			case (CBN_SELCHANGE<<16) | IDC_CONTROL:
				cfg_mixer_control = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
				g_change_mixer(cfg_which_mixer, cfg_mixer_control);
				break;
			}
			break;
		}
		return 0;
	}
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, DialogProc);
	}
	virtual const char * get_name() {return "Volume control toolbar";}
	virtual const char * get_parent_name() {return "Components";}
};

class mixer_actions : public menu_item_v2
{
	type get_type()
	{
		return TYPE_MAIN;
	}

	unsigned get_num_items()
	{
		return 3;
	}

	void enum_item(unsigned n, string_base & out)
	{
		static const char * items[] = {
			"Components/Volume toolbar up",
			"Components/Volume toolbar down",
			"Components/Volume toolbar mute"
		};
		assert (n < tabsize(items));
		out = items[n];
	}

	enabled_state get_enabled_state(unsigned idx)
	{
		return DEFAULT_OFF;
	}

	bool get_display_data(unsigned n, const ptr_list_base<metadb_handle> & data, string_base & out, unsigned & displayflags, const GUID & caller)
	{
		static const char * names[] = {
			"Volume up",
			"Volume down",
			"Volume mute"
		};

		assert (n < tabsize(names));

		out = names[n];
		displayflags = 0;

		return true;
	}

	bool enum_item_guid(unsigned n, GUID & out)
	{
		static const GUID guids[] = 
		{
			{ 0x9aafa8b, 0x1d0, 0x40fc, { 0xa5, 0x96, 0xcb, 0xf0, 0xed, 0xa4, 0x60, 0x3f } },
			{ 0xeaa316fe, 0x841, 0x40da, { 0xbd, 0x4, 0xf4, 0x97, 0x54, 0xa2, 0x48, 0x1a } },
			{ 0x41bf5822, 0x455e, 0x4f97, { 0x9d, 0x82, 0xe5, 0xa6, 0x36, 0xb0, 0xf5, 0x19 } }
		};

		assert(n < tabsize(guids));
		
		out = guids[n];

		return true;
	}

	void perform_command(unsigned n, const ptr_list_base<metadb_handle> & data, const GUID & caller)
	{
		int volume = g_get_volume();
		switch (n)
		{
		case 0:
			volume += 15;
			if (volume > 1000) volume = 1000;
			break;

		case 1:
			volume -= 15;
			if (volume < 0) volume = 0;
			break;

		case 2:
			if (volume)
			{
				cfg_mixer_mute_restore = volume;
				volume = 0;
			}
			else
			{
				volume = cfg_mixer_mute_restore;
				cfg_mixer_mute_restore = 0;
			}
			break;

		default:
			return;
		}

		g_set_volume(volume);
	}
};


static play_callback_factory<mixer_monitor> foo1;
static menu_item_factory<mixer_actions> foo2;
static config_factory<mixer_config> foo3;

DECLARE_COMPONENT_VERSION("Volume control toolbar", MY_VERSION, "Provides a UI Extension service for controlling the\nplayer internal or system mixer volume level.")
