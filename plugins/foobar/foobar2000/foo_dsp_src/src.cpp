#include "../SDK/foobar2000.h"
#include "samplerate.h"
#include "resource.h"

#include <assert.h>

static cfg_int cfg_srate("resampler_srate",48000),cfg_slow("resampler_slow",0);


class dsp_src : public dsp_i_base_t<resampler>
{
	SRC_STATE * src_state;
	SRC_DATA * src_data;
	mem_block_t<float> inbuffer;
	mem_block_t<audio_sample> outbuffer;
	UINT nch,srate,dest_srate;
	__int64 total_in, total_out;
	bool fast;
	bool whined;

	unsigned srate_override;
	int slow_override;
	bool have_slow_override;

	bool setup_chunk(audio_chunk* chunk)
	{
		if (src_data->output_frames_gen>0)
		{
			total_out += src_data->output_frames_gen;
			float * in = src_data->data_out;
			audio_sample * out = (audio_sample *) in;
			if (chunk==0) chunk = insert_chunk(src_data->output_frames_gen);
			for (int i = src_data->output_frames_gen * nch - 1; i >= 0; i--) out[i] = in[i];
			chunk->set_data(out,src_data->output_frames_gen,nch,dest_srate);
			src_data->output_frames_gen = 0;
			return true;
		}
		else return false;
	}

	virtual void set_dest_sample_rate(unsigned p_srate)
	{
		srate_override = p_srate;
	}

	virtual void set_config(unsigned flags)
	{
		slow_override = (flags & resampler::FLAG_SLOW) ? 1 : 0;
		have_slow_override = true;
	}

	virtual bool is_conversion_supported(unsigned src_srate,unsigned dst_srate)
	{
		return true;
	}

public:

	virtual GUID get_guid()
	{
		// {58CFB630-3023-4e47-8D98-4A1590C08CA9}
		static const GUID guid = 
		{ 0x58cfb630, 0x3023, 0x4e47, { 0x8d, 0x98, 0x4a, 0x15, 0x90, 0xc0, 0x8c, 0xa9 } };
		return guid;
	}

	virtual const char * get_name() {return "Resampler (SRC)";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		UINT new_dest_srate = srate_override>0 ? srate_override : cfg_srate;
		bool new_fast = have_slow_override ? !slow_override : !cfg_slow;

		if (src_state && (chunk->get_channels()!=nch || chunk->get_srate()!=srate || new_dest_srate!=dest_srate || fast!=new_fast))
		{
			src_data->end_of_input = 1;
			src_process(src_state, src_data);
			setup_chunk(0);
			src_state = src_delete(src_state);
			delete src_data;
			src_data=0;
		}

		nch=chunk->get_channels();
		srate=chunk->get_srate();
		dest_srate = new_dest_srate;
		fast = new_fast;

		if (!src_state)
		{
			if (dest_srate==srate) return true;//same specs, dont resample

			int error;

			src_state = src_new(fast ? 1 : 0, nch, &error);
			double ratio = (double) dest_srate / (double) srate;
			if (!src_state || !src_is_valid_ratio(ratio))
			{
				if (src_state) src_state = src_delete(src_state);
				if (!whined)
				{
					console::error(uStringPrintf("unable to resample from %uHz to %uHz",srate,dest_srate));
					whined = true;
				}
				return true;//FUCKO: unsupported src/dest samplerate combo, could make alt cheap linear resampler for those
			}
			src_data = new SRC_DATA;
			memset(src_data, 0, sizeof(*src_data));
			src_data->src_ratio = ratio;
			total_in = 0;
			total_out = 0;
		}

		dsp_util::kill_denormal(chunk->get_data(),chunk->get_data_length());

		audio_sample * in = chunk->get_data();
		float * out = inbuffer.check_size(chunk->get_sample_count() * nch);

		for (int i = chunk->get_sample_count() * nch - 1; i >= 0; i--) out[i] = (float) in[i];

		src_data->data_in = out;
		src_data->data_out = (float*) outbuffer.check_size(chunk->get_sample_count() * nch);
		src_data->input_frames = chunk->get_sample_count();
		src_data->output_frames = src_data->input_frames;

		src_process(src_state, src_data);

		while (src_data->output_frames_gen)
		{
			setup_chunk(0);
			src_data->data_in += src_data->input_frames_used * nch;
			src_data->input_frames -= src_data->input_frames_used;
			total_in += src_data->input_frames_used;
			src_data->input_frames_used = 0;
			src_process(src_state, src_data);
		}

		return 0;
	}


	virtual void on_endofplayback()
	{
		if (src_state)
		{
			src_data->end_of_input = 1;
			src_process(src_state, src_data);
			setup_chunk(0);
			flush();
		}
	}

	virtual void flush()
	{
		if (src_state)
		{
			src_state = src_delete(src_state);
			delete src_data;
			src_data = 0;
		}
	}
	
	virtual double get_latency()
	{//todo
		return src_data ? (double)total_in / (double)srate - (double)total_out / (double)dest_srate : 0;
	}

	dsp_src()
	{
		have_slow_override = false;
		srate_override = 0;
		slow_override = 0;
		src_state=0;
		src_data=0;
		srate=0;
		nch=0;
		fast=0;
		total_in = 0;
		total_out = 0;
		whined = false;
	}

	~dsp_src()
	{
		if (src_state) src_delete(src_state);
		if (src_data) delete src_data;
	}
};

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(wnd,IDC_SAMPLERATE);
			int n;
			for(n=0;n<tabsize(srate_tab);n++)
			{
				char temp[16];
				wsprintfA(temp,"%u",srate_tab[n]);
				SendMessageA(list,CB_ADDSTRING,0,(long)temp);
			}			
			SetDlgItemInt(wnd,IDC_SAMPLERATE,cfg_srate,0);
			SendDlgItemMessage(wnd,IDC_SLOW,BM_SETCHECK,cfg_slow,0);
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_SLOW:
			cfg_slow = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
			cfg_srate = srate_tab[SendMessage((HWND)lp,CB_GETCURSEL,0,0)];
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>192000) t=192000;
				cfg_srate = t;
			}
			break;
		}
		break;
	}
	return 0;
}

class config_src : public config
{
	virtual HWND create(HWND parent)
	{
		return CreateDialog(core_api::get_my_instance(),MAKEINTRESOURCE(IDD_CONFIG),parent,ConfigProc);
	}
	virtual const char * get_name() {return "Resampler (SRC)";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

class version_src : public componentversion
{
public:
	virtual void get_file_name(string_base & out) { out.set_string(service_factory_base::get_my_file_name()); }
	virtual void get_component_name(string_base & out) { out.set_string("SRC Resampler"); }
	virtual void get_component_version(string_base & out) { out.set_string("1.0"); }
	virtual void get_about_message(string_base & out)
	{
		out.set_string("Using: ");
		out.add_string(src_get_version());
		out.add_string("\n\nhttp://www.mega-nerd.com/SRC/\n\nThis plug-in is licensed under the GNU General\nPublic License. See COPYING.TXT.");
	}
};

static service_factory_t<dsp,dsp_src> foo;
static service_factory_single_t<config,config_src> foo2;
static service_factory_single_t<componentversion,version_src> foo3;
