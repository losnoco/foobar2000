#include "../SDK/input.h"
#include "../SDK/file_info_helper.h"
#include "../SDK/unpack.h"
#include "../SDK/config.h"
#include "../SDK/componentversion.h"

#include "resource.h"
#include <stdio.h>
#include <commctrl.h>

#include <libmodplug/modplug.h>

static const char * exts[]=
{
	// Original set copied from foo_mod, perhaps most frequently used types
	"MOD","MDZ",
	"STM","STZ",
	"S3M","S3Z",
	"IT","ITZ",
	"XM","XMZ",
	"MTM",
	"ULT",
	"669",
	"UMX",
	"FAR",
	"OKT",
	"PTM",
	// Now all of the other working types from libmodplug, lumped together
	"AMF", "AMS", "DBM", "DMF", "DSM", "MDL", "PSM",
};

static const char interp[] = {0, 1, 3, 2};

//#define SAMPLERATE 48000

static cfg_int cfg_samplerate("srate",48000);
static cfg_int cfg_interp("interp",2);

static cfg_int cfg_loop("mod_loop",1);
static cfg_int cfg_nr("mod_nr",0);
static cfg_int cfg_xbass("mod_xbass",0);
static cfg_int cfg_xbasslevel("mod_xbasslevel",54);
static cfg_int cfg_xbassrange("mod_xbassrange",62);
static cfg_int cfg_reverb("mod_reverb",0);
static cfg_int cfg_reverbdepth("mod_reverbdepth",42);
static cfg_int cfg_reverbdelay("mod_reverbdelay",199);
static cfg_int cfg_surround("mod_surround",0);
static cfg_int cfg_surrounddepth("mod_surrounddepth",76);
static cfg_int cfg_surrounddelay("mod_surrounddelay",22);

// static cfg_int cfg_infinite("spc_infinite",0);

class input_mod : public input_pcm
{
private:

	int open_file(reader * r,file_info * info)
	{
		bool is_unpacker;
		reader * unpack = unpacker::unpack_open(r);
		if (unpack)
		{
			r = unpack;
			is_unpacker = true;
		}
		else
		{
			is_unpacker = false;
		}
		__int64 size64 = r->get_length();
		char *ptr;
		if (size64 < 0 || size64 > (1<<30)) return 0;
		int size = (int)size64;

//		OutputDebugString("allocating buffer");
		ptr = buf.set_size(size);

//		OutputDebugString("reading file");
		if (r->read(ptr,size)!=size)
		{
			if (is_unpacker) r->reader_release();
			return 0;
		}
		if (is_unpacker) r->reader_release();

//		OutputDebugString("configuring libmodplug");
		ModPlug_SetSettings(&modSettings);

		if (module) ModPlug_Unload(module);
//		OutputDebugString("loading module");
		module = ModPlug_Load(ptr, size);
		buf.set_size(0);

		if (!module) return 0;
//		OutputDebugString("module loaded");

		info->info_set_int("samplerate", cfg_samplerate);
		info->info_set_int("channels", ModPlug_GetLogicalChannels(module));
		info->info_set("extrainfo","module");

		ptr = (char*) ModPlug_GetName(module);
		if (*ptr) info->meta_add_ansi("TITLE", ptr);

		info->set_length((double)ModPlug_GetLength(module)/1000.0);

		return 1;

	}

public:

	virtual int test_filename(const char * fn,const char * ext)
	{
		int n;
		int found=0;
		for(n=0;n<tabsize(exts);n++)
		{
			if (!stricmp(ext,exts[n])) return 1;
		}
		return 0;
	}

	virtual int open( reader * r, file_info * info,int full_open)	//FUCKO: multiinstance safety ?
	{
//		OutputDebugString("open()");
		if (!open_file(r,info)) return 0;
		if (!full_open)
		{
			ModPlug_Unload(module);
			module = NULL;
		}

/*		if (cfg_dspopt & (DSP_REVERSE|DSP_SURND))
			OSPC_SetFilter(&openspc_filter);
*/
//		pos = 0;

		return 1;


	}
	input_mod()
	{
		int f;
		module = NULL;
		f = MODPLUG_ENABLE_OVERSAMPLING;
//		if (cfg_agc) f |= MODPLUG_ENABLE_AGC;
		if (cfg_nr) f |= MODPLUG_ENABLE_NOISE_REDUCTION;
		if (cfg_xbass) f |= MODPLUG_ENABLE_MEGABASS;
		if (cfg_reverb) f |= MODPLUG_ENABLE_REVERB;
		if (cfg_surround) f |= MODPLUG_ENABLE_SURROUND;
		modSettings.mFlags = f;
		modSettings.mChannels = 2;
		modSettings.mBits = 32;
		if (cfg_samplerate > 48000)
		{
			// fag0ts shall have their fucking 96000Hz
			modSettings.mFrequency = cfg_samplerate >> 1;
			hax_double = 1;
		}
		else
		{
			modSettings.mFrequency = cfg_samplerate;
			hax_double = 0;
		}
		modSettings.mResamplingMode = interp[cfg_interp];
		modSettings.mBassAmount = cfg_xbasslevel;
		modSettings.mBassRange = cfg_xbassrange;
		modSettings.mReverbDepth = cfg_reverbdepth;
		modSettings.mReverbDelay = cfg_reverbdelay;
		modSettings.mSurroundDepth = cfg_surrounddepth;
		modSettings.mSurroundDelay = cfg_surrounddelay;
		modSettings.mLoopCount = cfg_loop ? -1 : 0;
	}

	~input_mod()
	{
		if (module) ModPlug_Unload(module);
	}

	virtual int set_info(reader *r,const file_info * info)
	{
		return 0;
	}

	virtual int get_samples_pcm(void ** buffer,int * size,int * srate,int * bps,int * nch)
	{
		*srate = modSettings.mFrequency;
		*bps = 32;
		*nch = 2;

		UINT written=0;

		int samples = 576 * 2; //(stereo)
		
		written=ModPlug_Read(module, buf.check_size(samples * sizeof(int)), samples * sizeof(int));

		if (hax_double)
		{
			// woot, lets fuq dat shiat up
			short * hax = (short*) buf.get_ptr();

			for (int i = 0; i < written / sizeof(int); i++)
			{
				*hax = hax[1];
				hax += 2;
			}

			*bps = 16;
			*srate <<= 1;
		}

		if (!written) return 0;

		*buffer = buf.get_ptr();
		*size = written;

		return 1;
	}

	virtual int seek(double pos64)
	{
		ModPlug_Seek(module, (int)(pos64*1000.0));
		return 1;
	}

private:

	ModPlugFile *module;
	ModPlug_Settings modSettings;
	mem_block_t<char> buf;
	int hax_double;
};

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND w = GetDlgItem(wnd,IDC_INTERPOLATION);
			SendMessage(w,CB_ADDSTRING,0,(long)"none");
			SendMessage(w,CB_ADDSTRING,0,(long)"linear");
			SendMessage(w,CB_ADDSTRING,0,(long)"cubic");
			SendMessage(w,CB_ADDSTRING,0,(long)"FIR");
			SendMessage(w,CB_SETCURSEL,interp[cfg_interp],0);
			w = GetDlgItem(wnd,IDC_SAMPLERATE);
			int n;
			for(n=0;n<tabsize(srate_tab);n++)
			{
				char temp[16];
				wsprintf(temp,"%u",srate_tab[n]);
				SendMessage(w,CB_ADDSTRING,0,(long)temp);
			}
			SetDlgItemInt(wnd,IDC_SAMPLERATE,cfg_samplerate,0);
			SendDlgItemMessage(wnd, IDC_LOOP, BM_SETCHECK, cfg_loop, 0);
			SendDlgItemMessage(wnd, IDC_NR, BM_SETCHECK, cfg_nr, 0);
			SendDlgItemMessage(wnd, IDC_XBASS, BM_SETCHECK, cfg_xbass, 0);
			w = GetDlgItem(wnd, IDC_XBASSLEVEL);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(0, 100));
			SendMessage(w, TBM_SETPOS, 1, cfg_xbasslevel);
			w = GetDlgItem(wnd, IDC_XBASSRANGE);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(10, 100));
			SendMessage(w, TBM_SETPOS, 1, cfg_xbassrange);
			SendDlgItemMessage(wnd, IDC_REVERB, BM_SETCHECK, cfg_reverb, 0);
			w = GetDlgItem(wnd, IDC_REVERBDEPTH);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(0, 100));
			SendMessage(w, TBM_SETPOS, 1, cfg_reverbdepth);
			w = GetDlgItem(wnd, IDC_REVERBDELAY);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(40, 250));
			SendMessage(w, TBM_SETPOS, 1, cfg_reverbdelay);
			SendDlgItemMessage(wnd, IDC_SURROUND, BM_SETCHECK, cfg_surround, 0);
			w = GetDlgItem(wnd, IDC_SURROUNDDEPTH);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(0, 100));
			SendMessage(w, TBM_SETPOS, 1, cfg_surrounddepth);
			w = GetDlgItem(wnd, IDC_SURROUNDDELAY);
			SendMessage(w, TBM_SETRANGE, 0, MAKELONG(5, 50));
			SendMessage(w, TBM_SETPOS, 1, cfg_surrounddelay);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
			cfg_samplerate = srate_tab[SendMessage((HWND)lp,CB_GETCURSEL,0,0)];
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>96000) t=96000;
				cfg_samplerate = t;
			}
			break;
		case (CBN_SELCHANGE<<16)|IDC_INTERPOLATION:
			cfg_interp = interp[SendMessage((HWND)lp,CB_GETCURSEL,0,0)];
			break;
		case IDC_LOOP:
			cfg_loop = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_NR:
			cfg_nr = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_XBASS:
			cfg_xbass = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_REVERB:
			cfg_reverb = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_SURROUND:
			cfg_surround = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		}
		break;
	case WM_HSCROLL:
		switch(GetWindowLong((HWND)lp,GWL_ID))
		{
		case IDC_XBASSLEVEL:
			cfg_xbasslevel = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		case IDC_XBASSRANGE:
			cfg_xbassrange = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		case IDC_REVERBDEPTH:
			cfg_reverbdepth = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		case IDC_REVERBDELAY:
			cfg_reverbdelay = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		case IDC_SURROUNDDEPTH:
			cfg_surrounddepth = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		case IDC_SURROUNDDELAY:
			cfg_surrounddelay = SendMessage((HWND)lp,TBM_GETPOS,0,0);
			break;
		}
		break;

	}
	return 0;
}

class config_mod : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return CreateDialog(service_factory_base::get_my_instance(),(const char*)IDD_MOD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "Module Decoder";}
	virtual const char * get_parent_name() {return "Playback";}
};

class version_mod : public componentversion
{
public:
	virtual const char * get_file_name() { return "foo_mod.dll"; }
	virtual const char * get_component_name() { return "Module Decoder"; }
	virtual const char * get_component_version() { return "0.1"; }
	virtual const char * get_about_message() { return "Based on libmodplug v2.0\n\nhttp://modplug-xmms.sourceforge.net/"; }
};

static service_factory_t<input,input_mod> foo;
static service_factory_single_t<config,config_mod> foo2;
static service_factory_single_t<componentversion,version_mod> foo3;
