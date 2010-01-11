#define MY_VERSION "0.5"

/*
	change log

2005-02-08 09:00 UTC - kode54
- Updated a few things again
- Version is now 0.5

2003-06-26 07:19 - kode54
- Updated to 0.7 API
- Version is now 0.4

*/

#include "../SDK/foobar2000.h"

#include <commctrl.h>

#include "resource.h"

#define RANGE 1000

static cfg_int cfg_length("pause_length", 5000);

class dsp_pause : public dsp
{
private:
	UINT srate, nch;
public:
	dsp_pause() : srate(0), nch(0) {}

	virtual GUID get_guid()
	{
		// {80A6ECC4-2FBC-4e8e-AE7F-296E0DF555F1}
		static const GUID guid = 
		{ 0x80a6ecc4, 0x2fbc, 0x4e8e, { 0xae, 0x7f, 0x29, 0x6e, 0xd, 0xf5, 0x55, 0xf1 } };
		return guid;
	}

	static inline void g_get_name(string_base & p_out) { p_out = "Pause between tracks";}

	virtual void run(dsp_chunk_list * list, const metadb_handle_ptr & cur_file, int flags)
	{
		if (list->get_count())
		{
			audio_chunk * chunk = list->get_item(list->get_count()-1);
			srate = chunk->get_srate();
			nch = chunk->get_channels();
		}
		if (!(flags & END_OF_TRACK)) return;
		if (!srate || !nch) return;
		int size = srate * cfg_length / 1000;
		audio_chunk * chunk = list->add_item(size);
		chunk->pad_with_silence(size);
	}
	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return 1;}
};

static void update_display(HWND wnd)
{
	string8 temp;
	int val = uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_GETPOS,0,0);
	cfg_length = val * 10;
	temp.add_int(val/100);
	temp.add_char('.');
	val %= 100;
	if (val < 10) temp.add_char('0');
	temp.add_int(val);
	uSetDlgItemText(wnd,IDC_DISPLAY,temp);
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETRANGE,0,MAKELONG(0,RANGE));
		uSendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETPOS,1,cfg_length / 10);
		update_display(wnd);
		return 1;
	case WM_HSCROLL:
		update_display(wnd);
		break;
	}
	return 0;
}

class config_pause : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	virtual const char * get_name() {return "Pause between tracks";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

static dsp_factory_t<dsp_pause>     g_dsp_pause_factory;
static config_factory<config_pause> g_config_pause_factory;

DECLARE_COMPONENT_VERSION("Pause between tracks", MY_VERSION, "Inserts a configurable pause (digital silence) after each file.");
