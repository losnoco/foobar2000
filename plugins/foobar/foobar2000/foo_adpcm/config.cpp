#define MY_VERSION "1.4"

/*
	change log

2010-01-11 05:13 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.4

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

#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

enum
{
	default_cfg_loop = 0
};

// {CCB20BBE-8C08-4337-B4EE-77778AF499E3}
static const GUID guid_cfg_loop = 
{ 0xccb20bbe, 0x8c08, 0x4337, { 0xb4, 0xee, 0x77, 0x77, 0x8a, 0xf4, 0x99, 0xe3 } };

cfg_int cfg_loop(guid_cfg_loop, default_cfg_loop);

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
		COMMAND_HANDLER_EX(IDC_LOOP, BN_CLICKED, OnButtonClick)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnButtonClick(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SendDlgItemMessage( IDC_LOOP, BM_SETCHECK, cfg_loop, FALSE );
	return TRUE;
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SendDlgItemMessage( IDC_LOOP, BM_SETCHECK, default_cfg_loop, FALSE );
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_loop = SendDlgItemMessage( IDC_LOOP, BM_GETCHECK, FALSE, FALSE );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return SendDlgItemMessage( IDC_LOOP, BM_GETCHECK, FALSE, FALSE ) != cfg_loop;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "kode's ADPCM decoders";}
	GUID get_guid() {
		// {F459F00D-3AF8-4157-AD4A-D4F1187E1EA9}
		static const GUID guid = { 0xf459f00d, 0x3af8, 0x4157, { 0xad, 0x4a, 0xd4, 0xf1, 0x18, 0x7e, 0x1e, 0xa9 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

static preferences_page_factory_t<preferences_page_myimpl> foo;

DECLARE_COMPONENT_VERSION("kode's ADPCM decoders", MY_VERSION, "Contains ACM, ADX, BRR, EA MUS, Game Cube DSP/ADP, OKI ADPCM, RAC, and XA decoders.");

VALIDATE_COMPONENT_FILENAME("foo_adpcm.dll");
