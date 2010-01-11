// change log

// 2003-08-30 09:03 - kode54
// - Menu item will only appear if DSP is in use
// - Version is now 1.1

#include "../SDK/foobar2000.h"

#include <commctrl.h>

#include "resource.h"

#define RANGE 1000

static cfg_int cfg_level("mute_level", 200);

static bool muted = false;

static volatile long running = 0;

class dsp_mute : public dsp_i_base
{
private:
	bool incremented;
public:
	virtual GUID get_guid()
	{
		// {3CD2177F-A188-45ff-AF7B-DF95C1D6B79B}
		static const GUID guid = 
		{ 0x3cd2177f, 0xa188, 0x45ff, { 0xaf, 0x7b, 0xdf, 0x95, 0xc1, 0xd6, 0xb7, 0x9b } };
		return guid;
	}

	virtual const char * get_name() { return "Mute";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (!incremented)
		{
			incremented = true;
			InterlockedIncrement(&running);
		}
		if (muted) dsp_util::scale_chunk(chunk, (double)cfg_level * .001);
		return true;
	}

	dsp_mute()
	{
		incremented = false;
	}

	~dsp_mute()
	{
		if (incremented) InterlockedDecrement(&running);
	}

	virtual void flush() {}
	virtual double get_latency() { return 0; }
	virtual bool need_track_change_mark() { return false; }
};

static void update_display(HWND wnd)
{
	string8 temp;
	int val = uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_GETPOS,0,0);
	cfg_level = val;
	temp.add_int(val/10);
	temp.add_char('.');
	val %= 10;
	temp.add_int(val);
	temp.add_char('%');
	uSetDlgItemText(wnd,IDC_DISPLAY,temp);
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETRANGE,0,MAKELONG(0,RANGE));
		uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETPOS,1,cfg_level);
		update_display(wnd);
		return 1;
	case WM_HSCROLL:
		update_display(wnd);
		break;
	}
	return 0;
}

class config_mute : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	virtual const char * get_name() {return "Mute";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

class menu_mute : public menu_item_legacy
{
	virtual type get_type()
	{
		return TYPE_MAIN;
	}

	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void enum_item(unsigned n, string_base & out)
	{
		out = "Components/Toggle mute";
	}

	virtual GUID get_item_guid(unsigned n)
	{
		// {2B1FDDE3-2F3C-470b-B3D6-22EF62A1D341}
		static const GUID guid = 
		{ 0x2b1fdde3, 0x2f3c, 0x470b, { 0xb3, 0xd6, 0x22, 0xef, 0x62, 0xa1, 0xd3, 0x41 } };
		return guid;
	}

	virtual bool get_display_data(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned & displayflags,const GUID & caller)
	{
		if (!running) return false;
		if (muted) out = "Unmute";
		else out = "Mute";
		return true;
	}

	virtual void perform_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller)
	{
		muted = muted ? false : true;
	}
};

static service_factory_t<dsp,dsp_mute> foo;
static service_factory_single_t<config,config_mute> foo2;
static menu_item_factory_t<menu_mute> foo3;

DECLARE_COMPONENT_VERSION("Mute switch", "1.1", "Attenuator DSP, activated by a menu item which\ncan be assigned to a keyboard shortcut.");
