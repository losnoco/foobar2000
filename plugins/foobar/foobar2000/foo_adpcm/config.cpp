#define MY_VERSION "1.3"

/*
	change log

2009-08-01 05:29 UTC - kode54
- Minor fixes to the DSP decoder
- Version is now 1.3

2005-05-22 11:14 UTC - kode54
- Fixed stupid bug in ACM init function
- Version is now 1.2

2004-10-08 14:57 UTC - kode54
- Wow, my first combined plug-in
- Hopefully, this file's component_version service stays at the top of the service list, for foover
- Version is now 1.0

*/

#include <foobar2000.h>

#include "resource.h"

// {CCB20BBE-8C08-4337-B4EE-77778AF499E3}
static const GUID guid_cfg_loop = 
{ 0xccb20bbe, 0x8c08, 0x4337, { 0xb4, 0xee, 0x77, 0x77, 0x8a, 0xf4, 0x99, 0xe3 } };

cfg_int cfg_loop(guid_cfg_loop, 0);

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_LOOP, BM_SETCHECK, cfg_loop, 0);
			/*uSendDlgItemMessage(wnd, IDC_SCANLOOPS, BM_SETCHECK, cfg_scanloops, 0);*/
		}
		return 1;
	case WM_COMMAND:
		switch (wp)
		{
		case IDC_LOOP:
			cfg_loop = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;
		/*case IDC_SCANLOOPS:
			cfg_scanloops = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;*/
		}
		break;
	}
	return 0;
}

class preferences_page_adpcm : public preferences_page
{
private:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}
	GUID get_guid()
	{
		// {F459F00D-3AF8-4157-AD4A-D4F1187E1EA9}
		static const GUID guid = 
		{ 0xf459f00d, 0x3af8, 0x4157, { 0xad, 0x4a, 0xd4, 0xf1, 0x18, 0x7e, 0x1e, 0xa9 } };
		return guid;
	}
	virtual const char * get_name() { return "kode's ADPCM decoders"; }
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_loop = 0;
		/*cfg_scanloops = 0;*/
	}
};

static preferences_page_factory_t<preferences_page_adpcm> foo;

DECLARE_COMPONENT_VERSION("kode's ADPCM decoders", MY_VERSION, "Contains ADX, BRR, RAC, and XA decoders, and\nthe BRR and XA converters.");
