#include "../SDK/foobar2000.h"
#include "resource.h"
#include <math.h>
#include <commctrl.h>

#define SQRTHALF          0.70710678118654752440084436210485

class dsp_51_downmix : public dsp_i_base
{
	mem_block_t<audio_sample> temp;
public:
	static GUID g_get_guid()
	{
		// {866233BB-C466-43e6-AFC4-8B7BE563D912}
		static const GUID guid = 
		{ 0x866233bb, 0xc466, 0x43e6, { 0xaf, 0xc4, 0x8b, 0x7b, 0xe5, 0x63, 0xd9, 0x12 } };
		
		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Convert 5.1 to stereo";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 6 && chunk->get_channel_config() == audio_chunk::channel_config_5point1)
		{ 
			UINT n,samples = chunk->get_sample_count();
			audio_sample * p_temp = temp.check_size(samples*2),
				* data = chunk->get_data();			
			for(n=0;n<samples;n++)
			{
				p_temp[n<<1]	 = data[0] + data[2] * SQRTHALF + data[4] * SQRTHALF + data[3];
				p_temp[(n<<1)+1] = data[1] + data[2] * SQRTHALF + data[5] * SQRTHALF + data[3];
				data += 6;
			}
			chunk->set_data(p_temp,samples,2,chunk->get_srate(),audio_chunk::channel_config_stereo);
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};


class dsp_mono : public dsp_i_base
{
public:
	static GUID g_get_guid()
	{
		// {EFC441DD-C7B4-4f34-974A-D3058F3A1BDE}
		static const GUID guid = 
		{ 0xefc441dd, 0xc7b4, 0x4f34, { 0x97, 0x4a, 0xd3, 0x5, 0x8f, 0x3a, 0x1b, 0xde } };
		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Downmix channels to mono";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		UINT nch = chunk->get_channels();
		if (nch > 1)
		{ 
			audio_sample * source = chunk->get_data();
			audio_sample * out = source;
			UINT n,c;
			UINT samples = chunk->get_sample_count();
			audio_sample div = (audio_sample)(1.0 / (float)nch);
			for(n=0;n<samples;n++)
			{
				audio_sample res = 0;
				for(c=0;c<nch;c++)
					res += *(source++);

				out[n] = res * div;
			}		

			chunk->set_channels(1,audio_chunk::channel_config_mono);
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_mono2stereo : public dsp_i_base
{
	mem_block_t<audio_sample> temp;
public:
	static GUID g_get_guid()
	{
		// {F661D118-5627-40ad-AB31-CCCF71A4819D}
		static const GUID guid = 
		{ 0xf661d118, 0x5627, 0x40ad, { 0xab, 0x31, 0xcc, 0xcf, 0x71, 0xa4, 0x81, 0x9d } };
		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Convert mono to stereo";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 1)
		{ 
			UINT n,samples = chunk->get_sample_count();
			audio_sample * p_temp = temp.check_size(samples*2),
				* src = chunk->get_data();			
			for(n=0;n<samples;n++)
			{
				p_temp[(n<<1)+1]=p_temp[n<<1]=src[n];
			}
			chunk->set_data(p_temp,samples,2,chunk->get_srate());
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_stereo2x : public dsp_i_base
{
	mem_block_t<audio_sample> temp;
public:
	static GUID g_get_guid()
	{
		// {7AAB4CE8-414A-427e-8228-78CD4A47388F}
		static const GUID guid = 
		{ 0x7aab4ce8, 0x414a, 0x427e, { 0x82, 0x28, 0x78, 0xcd, 0x4a, 0x47, 0x38, 0x8f } };

		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Convert stereo to 4 channels";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 2 && chunk->get_channel_config() == audio_chunk::channel_config_stereo)
		{ 
			UINT samples = chunk->get_sample_count();
			audio_sample * p_temp = temp.check_size(samples*4),
				* data = chunk->get_data();
			UINT n;
			for(n=0;n<samples;n++)
			{
				p_temp[(n<<2)]   = p_temp[(n<<2)+2] = data[(n<<1)];
				p_temp[(n<<2)+1] = p_temp[(n<<2)+3] = data[(n<<1)+1];
			}

			chunk->set_data(p_temp,samples,4,chunk->get_srate());
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_stereo_rear : public dsp_i_base
{
	mem_block_t<audio_sample> temp;
public:
	static GUID g_get_guid()
	{
		// {E24420A0-5DC6-4ee1-8AE2-02452E8D8DFB}
		static const GUID guid = 
		{ 0xe24420a0, 0x5dc6, 0x4ee1, { 0x8a, 0xe2, 0x2, 0x45, 0x2e, 0x8d, 0x8d, 0xfb } };

		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Move stereo to rear channels";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 2 && chunk->get_channel_config() == audio_chunk::channel_config_stereo)
		{
			chunk->set_channels(2,audio_chunk::channel_back_left | audio_chunk::channel_back_right);
			//chunk->set_channels(2,audio_chunk::channel_front_center | audio_chunk::channel_back_center);
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_reverse_stereo : public dsp_i_base
{
public:
	static GUID g_get_guid()
	{
		// {45BAE3C0-10B1-4008-BAAC-C5E6CEB06BA2}
		static const GUID guid = 
		{ 0x45bae3c0, 0x10b1, 0x4008, { 0xba, 0xac, 0xc5, 0xe6, 0xce, 0xb0, 0x6b, 0xa2 } };
		return guid;
	}

	static void g_get_name(string_base & p_out) { p_out = "Reverse stereo channels";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels()==2)
		{
			audio_sample * data = chunk->get_data();
			unsigned samples = chunk->get_sample_count();
			unsigned n;
			for(n=0;n<samples<<1;n+=2)
			{
				audio_sample t = data[n];
				data[n]=data[n+1];
				data[n+1]=t;
			}
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_surround : public dsp_i_base
{
public:
	static GUID g_get_guid()
	{
		// {E9405083-C764-464d-9700-72B0956DB712}
		static const GUID guid = 
		{ 0xe9405083, 0xc764, 0x464d, { 0x97, 0x0, 0x72, 0xb0, 0x95, 0x6d, 0xb7, 0x12 } };
		return guid;
	}

	static void g_get_name(string_base & p_out) { p_out = "Simple surround";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels()==2)
		{
			unsigned n, samples = chunk->get_sample_count();
			audio_sample * data = chunk->get_data();
			for(n=0;n<samples;n++)
				data[n<<1] = -data[n<<1];
		}
		return true;
	}
	
	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

#if 0

static cfg_int cfg_preamp("dsp_preamp",0);//in 0.1 dB

class dsp_preamp : public dsp_i_base
{
public:
	static GUID g_get_guid()
	{
		// {4E19660F-C8BB-4892-B2F6-A54C61714430}
		static const GUID guid = 
		{ 0x4e19660f, 0xc8bb, 0x4892, { 0xb2, 0xf6, 0xa5, 0x4c, 0x61, 0x71, 0x44, 0x30 } };
		return guid;
	}

	static void g_get_name(string_base & p_out) { p_out = "Preamp";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		dsp_util::scale_chunk(chunk,(audio_sample)pow(10.0,((float)(int)cfg_preamp) / 200.0));
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

enum {RANGE = 200};

static void update_display(HWND wnd)
{
	char temp[32];
	int val = SendDlgItemMessage(wnd,IDC_SLIDER,TBM_GETPOS,0,0);
	val -= RANGE;
	cfg_preamp = val;
	wsprintfA(temp,"%s%u.%u dB",val<0 ? "-" : "",abs(val/10),abs(val%10));
	SetDlgItemTextA(wnd,IDC_DISPLAY,temp);
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETRANGE,0,MAKELONG(0,2*RANGE));
		SendDlgItemMessage(wnd,IDC_SLIDER,TBM_SETPOS,1,cfg_preamp+RANGE);
		update_display(wnd);
		return TRUE;
	case WM_HSCROLL:
		update_display(wnd);
		return TRUE;
	default:
		return FALSE;
	}
}

class config_preamp : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG_PREAMP,parent,ConfigProc);
	}
	virtual const char * get_name() {return "Preamp";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

static dsp_factory_nopreset_t<dsp_preamp> foo3;
static service_factory_single_t<config,config_preamp> foo4;
#endif

static dsp_factory_nopreset_t<dsp_mono> foo1;
static dsp_factory_nopreset_t<dsp_mono2stereo> foo1a;
static dsp_factory_nopreset_t<dsp_reverse_stereo> foo2;

static dsp_factory_nopreset_t<dsp_surround> foo5;
static dsp_factory_nopreset_t<dsp_stereo2x> foo6;
static dsp_factory_nopreset_t<dsp_stereo_rear> foo7;
static dsp_factory_nopreset_t<dsp_51_downmix> foo8;

DECLARE_COMPONENT_VERSION("Standard DSP array","1.0","SSRC and SuperEQ code originally written by Naoki Shibata\nhttp://shibatch.sourceforge.net/\n\nAdvanced limiter by MTRH\n5.1 Downmix by Garf\n");