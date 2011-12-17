#define MYVERSION "1.116"

/*
	change log

2011-12-17 00:42 UTC - kode54
- Correctly implemented reading of the DATE field from M3U playlists
- Version is now 1.116

2011-07-26 03:14 UTC - kode54
- Fixed VGM format handling bugs introduced by recent changes
- Version is now 1.115

2011-07-25 20:24 UTC - kode54
- Added more header comment fields to the M3U parser
- M3U parser now correctly clears all fields on reset
- Version is now 1.114

2011-04-16 08:49 UTC - kode54
- Changed SPC-700 Status visualization from service_impl_t to window_service_impl_t,
  fixing the crash which occurred when cutting the element in layout edit mode
- Version is now 1.113

2011-03-28 09:34 UTC - kode54
- Increased maximum initial silence to 15 seconds for all formats except for GBS,
  which remains at 21 seconds, and VGM, which gets one second because all files
  should already be trimmed anyway.
- Version is now 1.112

2011-02-25 06:33 UTC - kode54
- Added support for more M3U comment fields
- Version is now 1.111

2011-02-02 04:36 UTC - kode54
- Fixed the GEP Control dialog so background decoding won't interfere with it
- Version is now 1.110

2011-01-24 02:00 UTC - kode54
- Implemented VGM7Z support
- Version is now 1.109

2011-01-17 05:44 UTC - kode54
- Added a SPC echo buffer clearing hack for when it's first enabled post-load
- Version is now 1.108

2010-11-20 20:59 UTC - kode54
- Changed zlib dependency to use standard zlib1.dll
- Version is now 1.107

2010-11-10 08:34 UTC - kode54
- Added GEP Control to modeless dialog manager properly, fixes tab navigation
- Version is now 1.106

2010-11-10 00:23 UTC - kode54
- Fixed files playing forever after seeking backwards when they weren't supposed to
- Version is now 1.105

2010-11-03 00:24 UTC - kode54
- Made minor change to YM2162 emulator that's not likely to have any audible effect
- Version is now 1.104

2010-10-18 23:37 UTC - kode54
- Implemented SPC visualizer reference counter so SPC input only emits data when a visualizer is opened
- Version is now 1.103

2010-04-22 07:03 UTC - kode54
- Implemented native gzip reader for VGM input in case an appropriate unpacker service is not installed
- Version is now 1.102

2010-04-13 14:53 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 1.101

2010-04-01 03:25 UTC - kode54
- Made SPC700 visualization volume display nicer like Super Jukebox
- Version is now 1.100

2010-03-26 04:36 UTC - kode54
- Implemented Enhanced SCC support
- Version is now 1.99

2010-03-20 23:55 UTC - kode54
- Improved SPC700 UI Element behavior by adding bump/activate support
- Fixed UI Element context menu handler to correctly position the menu when triggered by keyboard
- Version is now 1.98

2010-03-19 16:30 UTC - kode54
- Fixed flickering on window move with SPC700 UI Element
- Version is now 1.97

2010-03-18 19:51 UTC - kode54
- Implemented SPC700 visualization as a UI Element
- Version is now 1.96

2010-03-17 13:48 UTC - kode54
- Implemented SPC700 visualization based on Super Jukebox
- Version is now 1.95

2010-02-10 07:22 UTC - kode54
- Commented out DAC panning offset changing since it caused popping sounds sometimes
- Version is now 1.94

2010-02-09 10:06 UTC - kode54
- Implemented DAC panning support for GYM and VGM
- Version is now 1.93

2010-02-09 03:16 UTC - kode54
- Fixed YM2612 LFO and internal timer handling
- Version is now 1.92

2010-02-07 20:14 UTC - kode54
- Implemented HES ADPCM support
- Version is now 1.91

2010-02-05 07:58 UTC - kode54
- Updated RSN archive callback to replace the file extension with the original extension instead of
  an all-lowercase rsn
- Version is now 1.9

2010-01-28 09:43 UTC - kode54
- Updated YM2612 emulator again, this time to latest from Genesis Plus
- Version is now 1.89

2010-01-21 06:29 UTC - kode54
- Updated YM2612 emulator to latest from MAME
- Version is now 1.88

2010-01-14 00:44 UTC - kode54
- Fixed a crash in the HES emulator
- Version is now 1.87

2010-01-13 00:40 UTC - kode54
- Updated NSF and SPC context menu handlers
- Version is now 1.86

2010-01-11 10:47 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.85

2010-01-05 04:29 UTC - kode54
- Fixed a bug with playing Coleco SGC files after Sega FM files
- Version is now 1.84

2010-01-05 03:59 UTC - kode54
- Added configuration to enable or disable the SGC format support
- Version is now 1.83

2010-01-05 03:11 UTC - kode54
- Added file type mask and association for SGC files
- Version is now 1.82

2010-01-05 01:14 UTC - kode54
- Small update to KSS emulator
- Version is now 1.81

2010-01-04 02:49 UTC - kode54
- Updated Game_Music_Emu to support SGC
- Bundled Coleco BIOS for Coleco SGC files
- Version is now 1.8

2009-12-17 06:47 UTC - kode54
- Fixed NSF->NSFE conversion to only occur when configured
- Removed NSF<->NSFE renaming for now
- Changed context menu to allow NSFE writing only for NSFE files even when NSFE writing is disabled
- Version is now 1.78

2009-11-28 08:30 UTC - kode54
- Fixed M3U fade time reporting
- Version is now 1.77

2009-11-22 08:10 UTC - kode54
- Fixed indefinite playback again
- Version is now 1.76

2009-11-19 10:16 UTC - kode54
- Implemented SPC sinc interpolation
- Version is now 1.75

2009-10-13 01:38 UTC - kode54
- Fixed VGM Gd3 tag reading
- Version is now 1.71

2009-10-10 17:31 UTC - kode54
- Updated to blargg's latest unreleased rewrite of Game_Music_Emu
- Fixed SPC cubic interpolation and anti-surround settings
- Version is now 1.7

2009-08-05 04:13 UTC - kode54
- Implemented VGM YM2413 support based on MAME's YM2413 emulator
- Fixed NSFE playlist editor for when user cancels all editor dialogs
- Implemented missing NSFE playlist loading
- Version is now 1.68

2009-08-04 08:47 UTC - kode54
- Updated NSFE and SPC context menu tagging functions to the new metadb_io_v2 API
- Fixed a bug where newly tagged SPC length and fade would not register on the reread
  file length after a tag edit operation.
- Version is now 1.67

2009-08-04 06:43 UTC - kode54
- Enabled equalizer-only effects control over GYM and VGM inputs.
- Version is now 1.666

2009-07-22 03:24 UTC - kode54
- Implemented RSN archive wrapper for the existing RAR archive reader.
- Version is now 1.66

2009-07-21 06:28 UTC - kode54
- Implemented surround removal into the accurate SPC DSP core. Whoops.
- Implemented cubic interpolation into the accurate SPC DSP core.
- Version is now 1.65

2009-04-21 21:35 UTC - kode54
- Fixed a bug in Gb_Emu that would lead to crashes reading from 0xFF40
- Version is now 1.64

2009-04-19 04:29 UTC - kode54
- Implemented SPC ID666 and xid6 tag writing.
- Version is now 1.63

2009-04-17 20:44 UTC - kode54
- Rolled back changes from 1.61 as GYM crashed and VGM didn't have any noticeable effect
- Version is now 1.62

2009-03-31 00:38 UTC - kode54
- Implemented effects for the blip parts of GYM and VGM
- Version is now 1.61

2009-02-01 07:20 UTC - kode54
- Implemented effects control for applicable formats.
- Version is now 1.6

2006-12-18 17:07 UTC - kode54
- Logo is now self-cleaning and unregisters on shutdown.
- Version is now 1.5

2006-12-18 14:12 UTC - kode54
- Implemented tempo and voice control and pop-up dialog control

2006-12-17 21:19 UTC - kode54
- Updated Game_Music_Emu to v0.5.2

2006-11-29 13:47 UTC - kode54
- Added configuration for SPC cubic interpolation.
- Version is now 1.4.1

2006-09-23 09:20 UTC - kode54
- Fixed DFC dialog code so it uses DialogBoxParam and CreateDialogParam. Somehow,
  older compiler or Platform SDK allowed passing dialog parameter with DialogBox
  or CreateDialog macros.
- Fixed NSFE playlist editor function to check for IDOK return value, as DFC dialog
  code would otherwise return IDCANCEL, which is also non-zero.
- Fixed NSFE context menu code so it doesn't lock metadb around the update_info()
  calls, to prevent deadlock.
- Version is now 1.4

2006-06-09 --:-- UTC - kode54
- Added cubic interpolation to SPC input and accidentally committed toggling test
  mode and forgot to remove it for the above release.

2005-06-06 14:40 UTC - kode54
- Updated Game_Music_Emu to v0.2.4mod
- Version is now 1.3

2005-06-02 00:35 UTC - kode54
- Updated Game_Music_Emu to v0.2.4b
- Version is now 1.2

2005-01-01 01:19 UTC - kode54
- Added NSF support disable option
- Version is now 1.1

2004-12-04 06:35 UTC - kode54
- Added useless logo to config dialog

2004-12-03 18:48 UTC - kode54
- Whoops, forgot initial voice muting in spc.cpp

2004-12-03 17:05 UTC - kode54
- Initial release
- Version is now 1.0

*/

#define _WIN32_WINNT 0x0501

#include "config.h"
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "logo.h"

#include "resource.h"

static const GUID guid_cfg_sample_rate = { 0xaeeb3c42, 0x2089, 0x4533, { 0xbe, 0x2e, 0x41, 0x11, 0xe3, 0x77, 0x52, 0x2d } };
static const GUID guid_cfg_indefinite = { 0x333d265d, 0x4099, 0x4ee9, { 0xa0, 0xdf, 0xb6, 0xa4, 0xa3, 0xb7, 0x26, 0xe } };
static const GUID guid_cfg_default_length = { 0x725a4bb8, 0xd924, 0x44f7, { 0x82, 0xf2, 0x65, 0xc4, 0x93, 0x2f, 0x5d, 0x93 } };
static const GUID guid_cfg_default_fade = { 0x39da3fe0, 0x8f89, 0x4f35, { 0xbc, 0x16, 0xec, 0xc6, 0x0, 0x7a, 0xe6, 0xc9 } };
static const GUID guid_cfg_write = { 0x477ac718, 0xaf, 0x4873, { 0xa0, 0xce, 0x8e, 0x47, 0xf6, 0xec, 0xe5, 0x37 } };
static const GUID guid_cfg_write_nsfe = { 0x3d33ee75, 0x5abc, 0x4e41, { 0x91, 0x66, 0x4d, 0x5a, 0xd9, 0x9d, 0xe, 0xb5 } };
static const GUID guid_cfg_nsfe_ignore_playlists = { 0xc219de94, 0xcbd1, 0x45d4, { 0xa3, 0x21, 0xd, 0xee, 0xc4, 0x99, 0x82, 0x86 } };
static const GUID guid_cfg_spc_anti_surround = { 0x5d2b2962, 0x6c57, 0x4303, { 0xb9, 0xde, 0xd6, 0x97, 0x9c, 0x0, 0x45, 0x7a } };
static const GUID guid_cfg_spc_interpolation = { 0xf3f5df07, 0x7b49, 0x462a, { 0x8a, 0xd5, 0x9c, 0xd5, 0x79, 0x66, 0x31, 0x97 } };
static const GUID guid_cfg_history_rate = { 0xce4842e1, 0x5707, 0x4e43, { 0xaa, 0x56, 0x48, 0xc8, 0x1d, 0xce, 0x5c, 0xac } };
static const GUID guid_cfg_vgm_gd3_prefers_japanese = { 0x54ad3715, 0x5491, 0x45a8, { 0x9b, 0x11, 0xc3, 0x9d, 0x65, 0x2b, 0x15, 0x2f } };
static const GUID guid_cfg_format_enable = { 0xaeda04b5, 0x7b72, 0x4784, { 0xab, 0xda, 0xdf, 0xc8, 0x2f, 0xae, 0x20, 0x9 } };
static const GUID guid_cfg_vgm_loop_count = { 0xc6690d9, 0x6c36, 0x470e, { 0x93, 0x6f, 0x52, 0x89, 0x4a, 0xe4, 0xd7, 0xe0 } };


static const GUID guid_cfg_control_override = { 0x550a107e, 0x8b34, 0x41e5, { 0xae, 0xd6, 0x2, 0x1b, 0xf8, 0x3e, 0x14, 0xe4 } };
static const GUID guid_cfg_control_tempo = { 0xfbddc77c, 0x2a6, 0x41c9, { 0xbf, 0xfa, 0x54, 0x60, 0xbe, 0x2a, 0xa5, 0x23 } };

static const GUID guid_cfg_effects_enable = { 0x7a12d84d, 0x92ab, 0x4dae, { 0x89, 0x7, 0xfc, 0x47, 0x11, 0x1e, 0x66, 0x74 } };
static const GUID guid_cfg_effects_bass = { 0x6bad04a5, 0xb579, 0x400e, { 0x8c, 0xbd, 0x59, 0xb1, 0x22, 0x63, 0x2a, 0x37 } };
static const GUID guid_cfg_effects_treble = { 0x908aec30, 0x66b8, 0x4ab1, { 0x90, 0xa5, 0x77, 0xb9, 0xea, 0x98, 0xe, 0xd8 } };
static const GUID guid_cfg_effects_echo_depth = { 0x4c04e4ce, 0xeab9, 0x4046, { 0xb8, 0x33, 0xf1, 0x68, 0x44, 0xa3, 0x50, 0x19 } };

enum
{
	default_cfg_sample_rate = 44100,
	default_cfg_indefinite = 0,
	default_cfg_default_length = 170000,
	default_cfg_default_fade = 10000,
	default_cfg_write = 0,
	default_cfg_write_nsfe = 0,
	default_cfg_nsfe_ignore_playlists = 0,
	default_cfg_spc_anti_surround = 0,
	default_cfg_spc_interpolation = 0,
	default_cfg_vgm_loop_count = 1,
	default_cfg_vgm_gd3_prefers_japanese = 0,
	default_cfg_format_enable = ~0,
	default_cfg_control_override = 0,
	default_cfg_control_tempo = 10000,
	default_cfg_effects_enable = 0,
	default_cfg_effects_bass = 128,
	default_cfg_effects_treble = 128,
	default_cfg_effects_echo_depth = 31
};

cfg_int cfg_sample_rate(guid_cfg_sample_rate, 44100);

cfg_int cfg_indefinite(guid_cfg_indefinite, 0);
cfg_int cfg_default_length(guid_cfg_default_length, 170000);
cfg_int cfg_default_fade(guid_cfg_default_fade, 10000);

cfg_int cfg_write(guid_cfg_write, 0);
cfg_int cfg_write_nsfe(guid_cfg_write_nsfe, 0);
cfg_int cfg_nsfe_ignore_playlists(guid_cfg_nsfe_ignore_playlists, 0);

cfg_int cfg_spc_anti_surround(guid_cfg_spc_anti_surround, 0);
cfg_int cfg_spc_interpolation(guid_cfg_spc_interpolation, 0);

cfg_int cfg_vgm_loop_count(guid_cfg_vgm_loop_count, 1);
cfg_int cfg_vgm_gd3_prefers_japanese(guid_cfg_vgm_gd3_prefers_japanese, 0);

cfg_int cfg_format_enable(guid_cfg_format_enable, ~0);

cfg_int cfg_control_override(guid_cfg_control_override, 0);
cfg_int cfg_control_tempo(guid_cfg_control_tempo, 10000);

cfg_int cfg_effects_enable(guid_cfg_effects_enable, 0);
cfg_int cfg_effects_bass(guid_cfg_effects_bass, 128);
cfg_int cfg_effects_treble(guid_cfg_effects_treble, 128);
cfg_int cfg_effects_echo_depth(guid_cfg_effects_echo_depth, 31);

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000};

unsigned long parse_time_crap(const char *input)
{
	if (!input) return BORK_TIME;
	int len = strlen(input);
	if (!len) return BORK_TIME;
	int value = 0;
	{
		int i;
		for (i = len - 1; i >= 0; i--)
		{
			if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
			{
				return BORK_TIME;
			}
		}
	}
	pfc::string8 foo = input;
	char *bar = (char *)foo.get_ptr();
	char *strs = bar + foo.length() - 1;
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	if (*strs == '.' || *strs == ',')
	{
		// fraction of a second
		strs++;
		if (strlen(strs) > 3) strs[3] = 0;
		value = atoi(strs);
		switch (strlen(strs))
		{
		case 1:
			value *= 100;
			break;
		case 2:
			value *= 10;
			break;
		}
		strs--;
		*strs = 0;
		strs--;
	}
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	// seconds
	if (*strs < '0' || *strs > '9') strs++;
	value += atoi(strs) * 1000;
	if (strs > bar)
	{
		strs--;
		*strs = 0;
		strs--;
		while (strs > bar && (*strs >= '0' && *strs <= '9'))
		{
			strs--;
		}
		if (*strs < '0' || *strs > '9') strs++;
		value += atoi(strs) * 60000;
		if (strs > bar)
		{
			strs--;
			*strs = 0;
			strs--;
			while (strs > bar && (*strs >= '0' && *strs <= '9'))
			{
				strs--;
			}
			value += atoi(strs) * 3600000;
		}
	}
	return value;
}

void print_time_crap(int ms, char *out)
{
	char frac[8];
	int i,h,m,s;
	if (ms % 1000)
	{
		sprintf(frac, ".%3.3d", ms % 1000);
		for (i = 3; i > 0; i--)
			if (frac[i] == '0') frac[i] = 0;
		if (!frac[1]) frac[0] = 0;
	}
	else
		frac[0] = 0;
	h = ms / (60*60*1000);
	m = (ms % (60*60*1000)) / (60*1000);
	s = (ms % (60*1000)) / 1000;
	if (h) sprintf(out, "%d:%2.2d:%2.2d%s",h,m,s,frac);
	else if (m) sprintf(out, "%d:%2.2d%s",m,s,frac);
	else sprintf(out, "%d%s",s,frac);
}

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_INDEFINITE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_WRITE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_WNSFE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_NSFEPL, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_ANTISURROUND, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_GD3JAPANESE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_EFFECTS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_NSF, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_SPC, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_VGM, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_AY, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_GBS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_GYM, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_HES, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_KSS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_SAP, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FORMAT_SGC, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_DLENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DFADE, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_VGMLOOPCOUNT, CBN_SELCHANGE, OnSelectionChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
		DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, cfg_history_rate)
		COMMAND_HANDLER_EX(IDC_INTERPOLATION, CBN_SELCHANGE, OnSelectionChange)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnSelectionChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	void OnHScroll(UINT, UINT, CScrollBar);
	bool HasChanged();
	void OnChanged();

	void enable_vgm_loop_count(BOOL);

	const preferences_page_callback::ptr m_callback;
};

void CMyPreferences::enable_vgm_loop_count(BOOL status)
{
	GetDlgItem( IDC_VGMLOOPCOUNT_TEXT ).EnableWindow( status );
	GetDlgItem( IDC_VGMLOOPCOUNT ).EnableWindow( status );
}

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	char temp[16];
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, cfg_indefinite );
	SendDlgItemMessage( IDC_WRITE, BM_SETCHECK, cfg_write );
	SendDlgItemMessage( IDC_WNSFE, BM_SETCHECK, cfg_write_nsfe );
	SendDlgItemMessage( IDC_NSFEPL, BM_SETCHECK, cfg_nsfe_ignore_playlists );
	SendDlgItemMessage( IDC_ANTISURROUND, BM_SETCHECK, cfg_spc_anti_surround );
	SendDlgItemMessage( IDC_GD3JAPANESE, BM_SETCHECK, cfg_vgm_gd3_prefers_japanese );
	SendDlgItemMessage( IDC_EFFECTS, BM_SETCHECK, cfg_effects_enable );
	print_time_crap( cfg_default_length, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	print_time_crap( cfg_default_fade, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );

	CWindow w;

	w = GetDlgItem( IDC_SLIDER_BASS );
	::SendMessage( w, TBM_SETRANGE, 0, MAKELONG( 0, 255 ) );
	::SendMessage( w, TBM_SETPOS, 1, cfg_effects_bass );

	w = GetDlgItem( IDC_SLIDER_TREBLE );
	::SendMessage( w, TBM_SETRANGE, 0, MAKELONG( 0, 255 ) );
	::SendMessage( w, TBM_SETPOS, 1, cfg_effects_treble );

	w = GetDlgItem( IDC_SLIDER_ECHO_DEPTH );
	::SendMessage( w, TBM_SETRANGE, 0, MAKELONG( 0, 255 ) );
	::SendMessage( w, TBM_SETPOS, 1, cfg_effects_echo_depth );

	int n,o;
	for(n=IDC_FORMAT_NSF,o=0;n<=IDC_FORMAT_SGC;n++,o++)
	{
		SendDlgItemMessage( n, BM_SETCHECK, cfg_format_enable & ( 1 << o ) );
	}
	for(n=tabsize(srate_tab);n--;)
	{
		if (srate_tab[n] != cfg_sample_rate)
		{
			itoa(srate_tab[n], temp, 10);
			cfg_history_rate.add_item(temp);
		}
	}
	itoa( cfg_sample_rate, temp, 10 );
	cfg_history_rate.add_item(temp);
	w = GetDlgItem( IDC_SAMPLERATE );
	cfg_history_rate.setup_dropdown( w );
	::SendMessage( w, CB_SETCURSEL, 0, 0 );

	w = GetDlgItem( IDC_VGMLOOPCOUNT );
	uSendMessageText( w, CB_ADDSTRING, 0, "none" );
	for (n = 1; n <= 10; n++)
	{
		itoa( n, temp, 10 );
		uSendMessageText( w, CB_ADDSTRING, 0, temp );
	}
	::SendMessage( w, CB_SETCURSEL, cfg_vgm_loop_count, 0 );

	enable_vgm_loop_count( !cfg_indefinite );

	w = GetDlgItem( IDC_INTERPOLATION );
	uSendMessageText( w, CB_ADDSTRING, 0, "Gaussian" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Cubic" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Sinc" );
	::SendMessage( w, CB_SETCURSEL, cfg_spc_interpolation, 0 );

	union
	{
		RECT r;
		POINT p [2];
	};

	w = GetDlgItem( IDC_GROUPBOX );
	w.GetClientRect( &r );
	w.MapWindowPoints( m_hWnd, &p [1], 1 );

	CreateLogo( m_hWnd, p [1].x + 2, p [1].y - 181 );

	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnSelectionChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	enable_vgm_loop_count( ! SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK ) );
	OnChanged();
}

void CMyPreferences::OnHScroll(UINT, UINT, CScrollBar) {
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	int n, o;
	char temp[16];
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, default_cfg_indefinite );
	SendDlgItemMessage( IDC_WRITE, BM_SETCHECK, default_cfg_write );
	SendDlgItemMessage( IDC_WNSFE, BM_SETCHECK, default_cfg_write_nsfe );
	SendDlgItemMessage( IDC_NSFEPL, BM_SETCHECK, default_cfg_nsfe_ignore_playlists );
	SendDlgItemMessage( IDC_ANTISURROUND, BM_SETCHECK, default_cfg_spc_anti_surround );
	SendDlgItemMessage( IDC_GD3JAPANESE, BM_SETCHECK, default_cfg_vgm_gd3_prefers_japanese );
	SendDlgItemMessage( IDC_EFFECTS, BM_SETCHECK, default_cfg_effects_enable );
	for(n=IDC_FORMAT_NSF,o=0;n<=IDC_FORMAT_SGC;n++,o++)
	{
		SendDlgItemMessage( n, BM_SETCHECK, !! ( default_cfg_format_enable & ( 1 << o ) ) );
	}
	print_time_crap( default_cfg_default_length, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	print_time_crap( default_cfg_default_fade, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	SendDlgItemMessage( IDC_SLIDER_BASS, TBM_SETPOS, 1, default_cfg_effects_bass );
	SendDlgItemMessage( IDC_SLIDER_TREBLE, TBM_SETPOS, 1, default_cfg_effects_treble );
	SendDlgItemMessage( IDC_SLIDER_ECHO_DEPTH, TBM_SETPOS, 1, default_cfg_effects_echo_depth );
	SetDlgItemInt( IDC_SAMPLERATE, default_cfg_sample_rate, FALSE );
	SendDlgItemMessage( IDC_VGMLOOPCOUNT, CB_SETCURSEL, default_cfg_vgm_loop_count );
	enable_vgm_loop_count( !default_cfg_indefinite );
	SendDlgItemMessage( IDC_INTERPOLATION, CB_SETCURSEL, default_cfg_spc_interpolation );

	OnChanged();
}

void CMyPreferences::apply() {
	char temp[16];
	int t = GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE );
	if ( t < 6000 ) t = 6000;
	else if ( t > 96000 ) t = 96000;
	SetDlgItemInt( IDC_SAMPLERATE, t, FALSE );
	itoa( t, temp, 10 );
	cfg_history_rate.add_item(temp);
	cfg_sample_rate = t;
	cfg_vgm_loop_count = SendDlgItemMessage( IDC_VGMLOOPCOUNT, CB_GETCURSEL );
	cfg_spc_interpolation = SendDlgItemMessage( IDC_INTERPOLATION, CB_GETCURSEL );
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
	if ( t != BORK_TIME ) cfg_default_length = t;
	else
	{
		print_time_crap( cfg_default_length, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	}
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
	if ( t != BORK_TIME ) cfg_default_fade = t;
	else
	{
		print_time_crap( cfg_default_fade, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	}
	cfg_indefinite = SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK );
	cfg_write = SendDlgItemMessage( IDC_WRITE, BM_GETCHECK );
	cfg_write_nsfe = SendDlgItemMessage( IDC_WNSFE, BM_GETCHECK );
	cfg_nsfe_ignore_playlists = SendDlgItemMessage( IDC_NSFEPL, BM_GETCHECK );
	cfg_spc_anti_surround = SendDlgItemMessage( IDC_ANTISURROUND, BM_GETCHECK );
	cfg_vgm_gd3_prefers_japanese = SendDlgItemMessage( IDC_GD3JAPANESE, BM_GETCHECK );
	cfg_effects_enable = SendDlgItemMessage( IDC_EFFECTS, BM_GETCHECK );
	cfg_format_enable = ~0;
	for (unsigned wp = IDC_FORMAT_NSF; wp <= IDC_FORMAT_SGC; wp++)
	{
		unsigned bit = 1 << ( wp - IDC_FORMAT_NSF );
		unsigned mask = ~0 ^ bit;
		cfg_format_enable = ( cfg_format_enable & mask ) | ( bit * SendDlgItemMessage( wp, BM_GETCHECK ) );
	}
	cfg_effects_bass = SendDlgItemMessage( IDC_SLIDER_BASS, TBM_GETPOS );
	cfg_effects_treble = SendDlgItemMessage( IDC_SLIDER_TREBLE, TBM_GETPOS );
	cfg_effects_echo_depth = SendDlgItemMessage( IDC_SLIDER_ECHO_DEPTH, TBM_GETPOS );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if ( !changed && GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE ) != cfg_sample_rate ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_INTERPOLATION, CB_GETCURSEL ) != cfg_spc_interpolation ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_VGMLOOPCOUNT, CB_GETCURSEL ) != cfg_vgm_loop_count ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK ) != cfg_indefinite ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_WRITE, BM_GETCHECK ) != cfg_write ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_WNSFE, BM_GETCHECK ) != cfg_write_nsfe ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_NSFEPL, BM_GETCHECK ) != cfg_nsfe_ignore_playlists ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_ANTISURROUND, BM_GETCHECK ) != cfg_spc_anti_surround ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_GD3JAPANESE, BM_GETCHECK ) != cfg_vgm_gd3_prefers_japanese ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_EFFECTS, BM_GETCHECK ) != cfg_effects_enable ) changed = true;
	if ( !changed )
	{
		unsigned format_enable = ~0;
		for (unsigned wp = IDC_FORMAT_NSF; wp <= IDC_FORMAT_SGC; wp++)
		{
			unsigned bit = 1 << ( wp - IDC_FORMAT_NSF );
			unsigned mask = ~0 ^ bit;
			format_enable = ( format_enable & mask ) | ( bit * SendDlgItemMessage( wp, BM_GETCHECK ) );
		}
		if ( format_enable != cfg_format_enable ) changed = true;
	}
	if ( !changed && SendDlgItemMessage( IDC_SLIDER_BASS, TBM_GETPOS ) != cfg_effects_bass ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_SLIDER_TREBLE, TBM_GETPOS ) != cfg_effects_treble ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_SLIDER_ECHO_DEPTH, TBM_GETPOS ) != cfg_effects_echo_depth ) changed = true;
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
		if ( t != BORK_TIME && t != cfg_default_length ) changed = true;
	}
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
		if ( t != BORK_TIME && t != cfg_default_fade ) changed = true;
	}
	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "Game Emu Player";}
	GUID get_guid() {
		// {00C3BD9B-CA1D-477d-B381-434EE9FB993B}
		static const GUID guid = { 0xc3bd9b, 0xca1d, 0x477d, { 0xb3, 0x81, 0x43, 0x4e, 0xe9, 0xfb, 0x99, 0x3b } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

static preferences_page_factory_t<preferences_page_myimpl> foo1;
DECLARE_COMPONENT_VERSION("Game Emu Player", MYVERSION, "Based on Game_Music_Emu vX.X.X by Shay Green\n\nhttp://www.slack.net/~ant/")
VALIDATE_COMPONENT_FILENAME("foo_gep.dll");
