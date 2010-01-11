#define MYVERSION "1.3"

/*
	change log

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

#include "config.h"
#include "../helpers/dropdown_helper.h"

#include "logo.h"

#include "resource.h"

static const GUID guid_cfg_sample_rate = { 0xaeeb3c42, 0x2089, 0x4533, { 0xbe, 0x2e, 0x41, 0x11, 0xe3, 0x77, 0x52, 0x2d } };
static const GUID guid_cfg_indefinite = { 0x333d265d, 0x4099, 0x4ee9, { 0xa0, 0xdf, 0xb6, 0xa4, 0xa3, 0xb7, 0x26, 0xe } };
static const GUID guid_cfg_default_length = { 0x725a4bb8, 0xd924, 0x44f7, { 0x82, 0xf2, 0x65, 0xc4, 0x93, 0x2f, 0x5d, 0x93 } };
static const GUID guid_cfg_default_fade = { 0x39da3fe0, 0x8f89, 0x4f35, { 0xbc, 0x16, 0xec, 0xc6, 0x0, 0x7a, 0xe6, 0xc9 } };
static const GUID guid_cfg_nsf_disable = { 0x1c561fd6, 0xd992, 0x4fea, { 0xac, 0x4c, 0xd4, 0x27, 0xb0, 0x57, 0x9, 0x7d } };
static const GUID guid_cfg_write = { 0x477ac718, 0xaf, 0x4873, { 0xa0, 0xce, 0x8e, 0x47, 0xf6, 0xec, 0xe5, 0x37 } };
static const GUID guid_cfg_write_nsfe = { 0x3d33ee75, 0x5abc, 0x4e41, { 0x91, 0x66, 0x4d, 0x5a, 0xd9, 0x9d, 0xe, 0xb5 } };
static const GUID guid_cfg_nsfe_ignore_playlists = { 0xc219de94, 0xcbd1, 0x45d4, { 0xa3, 0x21, 0xd, 0xee, 0xc4, 0x99, 0x82, 0x86 } };
static const GUID guid_cfg_spc_anti_surround = { 0x5d2b2962, 0x6c57, 0x4303, { 0xb9, 0xde, 0xd6, 0x97, 0x9c, 0x0, 0x45, 0x7a } };
static const GUID guid_cfg_history_rate = { 0xce4842e1, 0x5707, 0x4e43, { 0xaa, 0x56, 0x48, 0xc8, 0x1d, 0xce, 0x5c, 0xac } };
static const GUID guid_cfg_vgm_gd3_prefers_japanese = { 0x54ad3715, 0x5491, 0x45a8, { 0x9b, 0x11, 0xc3, 0x9d, 0x65, 0x2b, 0x15, 0x2f } };


cfg_int cfg_sample_rate(guid_cfg_sample_rate, 44100);

cfg_int cfg_indefinite(guid_cfg_indefinite, 0);
cfg_int cfg_default_length(guid_cfg_default_length, 170000);
cfg_int cfg_default_fade(guid_cfg_default_fade, 10000);

cfg_int cfg_nsf_disable(guid_cfg_nsf_disable, 0);

cfg_int cfg_write(guid_cfg_write, 0);
cfg_int cfg_write_nsfe(guid_cfg_write_nsfe, 0);
cfg_int cfg_nsfe_ignore_playlists(guid_cfg_nsfe_ignore_playlists, 0);

cfg_int cfg_spc_anti_surround(guid_cfg_spc_anti_surround, 0);

cfg_int cfg_vgm_gd3_prefers_japanese(guid_cfg_vgm_gd3_prefers_japanese, 0);

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
	string8 foo = input;
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

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			char temp[16];
			uSendDlgItemMessage(wnd, IDC_INDEFINITE, BM_SETCHECK, cfg_indefinite, 0);
			uSendDlgItemMessage(wnd, IDC_NSFDISABLE, BM_SETCHECK, cfg_nsf_disable, 0);
			uSendDlgItemMessage(wnd, IDC_WRITE, BM_SETCHECK, cfg_write, 0);
			uSendDlgItemMessage(wnd, IDC_WNSFE, BM_SETCHECK, cfg_write_nsfe, 0);
			uSendDlgItemMessage(wnd, IDC_NSFEPL, BM_SETCHECK, cfg_nsfe_ignore_playlists, 0);
			uSendDlgItemMessage(wnd, IDC_ANTISURROUND, BM_SETCHECK, cfg_spc_anti_surround, 0);
			uSendDlgItemMessage(wnd, IDC_GD3JAPANESE, BM_SETCHECK, cfg_vgm_gd3_prefers_japanese, 0);
			print_time_crap(cfg_default_length, (char *)&temp);
			uSetDlgItemText(wnd, IDC_DLENGTH, (char *)&temp);
			print_time_crap(cfg_default_fade, (char *)&temp);
			uSetDlgItemText(wnd, IDC_DFADE, (char *)&temp);

			HWND w;
			int n;
			for(n=tabsize(srate_tab);n--;)
			{
				if (srate_tab[n] != cfg_sample_rate)
				{
					itoa(srate_tab[n], temp, 10);
					cfg_history_rate.add_item(temp);
				}
			}
			itoa(cfg_sample_rate, temp, 10);
			cfg_history_rate.add_item(temp);
			cfg_history_rate.setup_dropdown(w = GetDlgItem(wnd,IDC_SAMPLERATE));
			uSendMessage(w, CB_SETCURSEL, 0, 0);

			CreateLogo( wnd, (HMENU)IDC_LOGO, 239, 72 );
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_INDEFINITE:
			cfg_indefinite = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_NSFDISABLE:
			cfg_nsf_disable = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_WRITE:
			cfg_write = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_WNSFE:
			cfg_write_nsfe = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_NSFEPL:
			cfg_nsfe_ignore_playlists = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_ANTISURROUND:
			cfg_spc_anti_surround = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_GD3JAPANESE:
			cfg_vgm_gd3_prefers_japanese = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (EN_CHANGE<<16)|IDC_LENGTH:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_default_length = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_LENGTH:
			{
				char temp[16];
				print_time_crap(cfg_default_length, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		case (EN_CHANGE<<16)|IDC_FADE:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_default_fade = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_FADE:
			{
				char temp[16];
				print_time_crap(cfg_default_fade, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>96000) t=96000;
				cfg_sample_rate = t;
			}
			break;
		}
		break;
	case WM_DESTROY:
		char temp[16];
		itoa(cfg_sample_rate, temp, 10);
		cfg_history_rate.add_item(temp);

		DestroyLogo( GetDlgItem( wnd, IDC_LOGO ) );

		break;
	}
	return 0;
}

class preferences_page_gep : public preferences_page
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}
	GUID get_guid()
	{
		// {00C3BD9B-CA1D-477d-B381-434EE9FB993B}
		static const GUID guid = 
		{ 0xc3bd9b, 0xca1d, 0x477d, { 0xb3, 0x81, 0x43, 0x4e, 0xe9, 0xfb, 0x99, 0x3b } };
		return guid;
	}
	virtual const char * get_name() {return "Game Emu Player";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_sample_rate = 44100;

		cfg_indefinite = 0;
		cfg_default_length = 170000;
		cfg_default_fade = 10000;

		cfg_nsf_disable = 0;

		cfg_write = 0;
		cfg_write_nsfe = 0;
		cfg_nsfe_ignore_playlists = 0;

		cfg_spc_anti_surround = 0;
	}
};

static preferences_page_factory_t<preferences_page_gep> foo1;
DECLARE_COMPONENT_VERSION("Game Emu Player", MYVERSION, "Based on Game_Music_Emu v0.2.2 by Shay Green\n\nhttp://www.slack.net/~ant/")
