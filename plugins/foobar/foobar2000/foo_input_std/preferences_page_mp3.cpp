#include "stdafx.h"
#include "resource.h"
#include "preferences_mp3.h"

// {8164E823-4DA7-4301-98E3-1E1D62F38AEE}
static const GUID guid_cfg_mp3_notagaction = 
{ 0x8164e823, 0x4da7, 0x4301, { 0x98, 0xe3, 0x1e, 0x1d, 0x62, 0xf3, 0x8a, 0xee } };

// {4478ED2B-BFBF-4d51-8222-61295FE7A383}
static const GUID guid_cfg_mp3_id3v1action = 
{ 0x4478ed2b, 0xbfbf, 0x4d51, { 0x82, 0x22, 0x61, 0x29, 0x5f, 0xe7, 0xa3, 0x83 } };

// {9698923E-D598-4ed1-A040-ECA40ED00156}
static const GUID guid_cfg_mp3_forcetag = 
{ 0x9698923e, 0xd598, 0x4ed1, { 0xa0, 0x40, 0xec, 0xa4, 0xe, 0xd0, 0x1, 0x56 } };

// {2030AFDE-B983-44e4-8C71-2310140C0585}
static const GUID guid_cfg_mp3_forcetagtype = 
{ 0x2030afde, 0xb983, 0x44e4, { 0x8c, 0x71, 0x23, 0x10, 0x14, 0xc, 0x5, 0x85 } };


cfg_int cfg_mp3_notagaction(guid_cfg_mp3_notagaction,mp3_tagtype_id3v1),
	cfg_mp3_id3v1action(guid_cfg_mp3_id3v1action,mp3_tagtype_id3v2_id3v1),
	cfg_mp3_forcetag(guid_cfg_mp3_forcetag,0),
	cfg_mp3_forcetagtype(guid_cfg_mp3_forcetagtype,mp3_tagtype_apev2);



static const char * notagaction_messages[] =
{
	"Add ID3v1 (default)",
	"Add APEv2",
	"Add APEv2 + ID3v1",
	"Add ID3v2",
	"Add ID3v2 + ID3v1",
};

static const char * id3v1action_messages[] =
{
	"Truncate fields that don't fit and keep ID3v1",
	"Change to APEv2 tag",
	"Change to APEv2 + ID3v1 (default)",
	"Change to ID3v2",
	"Change to ID3v2 + ID3v1",
};

static const char * forcetag_messages[] =
{
	"Add/update ID3v1, remove ID3v2 and APEv2 if present",
	"Add/update APEv2, remove ID3v2 and ID3v1 if present",
	"Add/update APEv2 + ID3v1, remove ID3v2 if present",
	"Add/update ID3v2, remove APEv2 and ID3v1 if present",
	"Add/update ID3v2 + ID3v1, remove APEv2 if present",
};

static void update_forcetag(HWND p_wnd)
{
	BOOL state = !!cfg_mp3_forcetag;
	EnableWindow(GetDlgItem(p_wnd,IDC_STATIC_NOTAGACTION),!state);
	EnableWindow(GetDlgItem(p_wnd,IDC_NOTAGACTION),!state);
	EnableWindow(GetDlgItem(p_wnd,IDC_STATIC_ID3v1ACTION),!state);
	EnableWindow(GetDlgItem(p_wnd,IDC_ID3v1ACTION),!state);
	EnableWindow(GetDlgItem(p_wnd,IDC_FORCETAGTYPE),state);
}

static BOOL CALLBACK PreferencesTagProc(HWND p_wnd,UINT p_msg,WPARAM p_wp,LPARAM p_lp)
{
	switch(p_msg)
	{
	case WM_INITDIALOG:
		{
			HWND combo;
			unsigned n;
			bool have_id3v2 = tag_processor::is_id3v2_processor_available();
			unsigned numoptions = have_id3v2 ? 5 : 3;
			
			//if (have_id3v2) uSetDlgItemText(p_wnd,IDC_ID3v2_NOTICE,"ID3v2 support is installed.");
			if (!have_id3v2) uSetDlgItemText(p_wnd,IDC_ID3v2_NOTICE,"ID3v2 support is not installed.");

			uSendDlgItemMessage(p_wnd,IDC_FORCETAG,BM_SETCHECK,cfg_mp3_forcetag,0);

			update_forcetag(p_wnd);

			combo = uGetDlgItem(p_wnd,IDC_NOTAGACTION);			

			for(n=0;n<numoptions;n++)
				uSendMessageText(combo,CB_ADDSTRING,0,notagaction_messages[n]);

			uSendMessage(combo,CB_SETCURSEL,cfg_mp3_notagaction,0);

			combo = uGetDlgItem(p_wnd,IDC_ID3v1ACTION);

			for(n=0;n<numoptions;n++)
				uSendMessageText(combo,CB_ADDSTRING,0,id3v1action_messages[n]);

			uSendMessage(combo,CB_SETCURSEL,cfg_mp3_id3v1action,0);
			
			combo = uGetDlgItem(p_wnd,IDC_FORCETAGTYPE);

			for(n=0;n<numoptions;n++)
				uSendMessageText(combo,CB_ADDSTRING,0,forcetag_messages[n]);

			uSendMessage(combo,CB_SETCURSEL,cfg_mp3_forcetagtype,0);
		}
		return TRUE;
	case WM_COMMAND:
		switch(p_wp)
		{
		case IDC_NOTAGACTION | (CBN_SELCHANGE << 16):
			cfg_mp3_notagaction = uSendMessage((HWND)p_lp,CB_GETCURSEL,0,0);
			return TRUE;
		case IDC_ID3v1ACTION | (CBN_SELCHANGE << 16):
			cfg_mp3_id3v1action	= uSendMessage((HWND)p_lp,CB_GETCURSEL,0,0);
			return TRUE;
		case IDC_FORCETAG:
			cfg_mp3_forcetag = uSendMessage((HWND)p_lp,BM_GETCHECK,0,0);
			update_forcetag(p_wnd);
			return TRUE;
		case IDC_FORCETAGTYPE | (CBN_SELCHANGE << 16):
			cfg_mp3_forcetagtype = uSendMessage((HWND)p_lp,CB_GETCURSEL,0,0);
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}

class preferences_page_mp3tag : public preferences_page
{
public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_PREFERENCES_MP3TAG,parent,PreferencesTagProc);
	}

	const char * get_name() {return "MP3";}

	GUID get_guid()
	{
		// {11569BFA-AD7B-4a42-AAB0-572B2A206542}
		static const GUID guid = 
		{ 0x11569bfa, 0xad7b, 0x4a42, { 0xaa, 0xb0, 0x57, 0x2b, 0x2a, 0x20, 0x65, 0x42 } };
		return guid;
	}

	GUID get_parent_guid() {return guid_tag_writing;}

	bool reset_query() {return true;}

	void reset()
	{
		cfg_mp3_notagaction = mp3_tagtype_id3v1;
		cfg_mp3_id3v1action = mp3_tagtype_apev2_id3v1;
		cfg_mp3_forcetag = 0;
		cfg_mp3_forcetagtype = mp3_tagtype_apev2;

	}
};

static preferences_page_factory_t<preferences_page_mp3tag> g_preferences_page_mp3tag_factory;