#include "../../SDK/foobar2000.h"
#include "../../helpers/helpers.h"
#include "ssrc.h"
#include "../resource.h"

// {8EFCFE86-EA25-4933-A621-529FCE49046B}
static const GUID g_ssrc_guid = 
{ 0x8efcfe86, 0xea25, 0x4933, { 0xa6, 0x21, 0x52, 0x9f, 0xce, 0x49, 0x4, 0x6b } };


struct t_resampler_config
{
	unsigned m_dest_srate;
	float m_quality_scale;
};

static const t_resampler_config g_default_config = {48000,0};

static bool g_preset_to_config(t_resampler_config & p_config,const dsp_preset & p_preset)
{
	if (p_preset.get_data_size() == sizeof(p_config))
	{
		t_resampler_config temp = * reinterpret_cast<const t_resampler_config*>(p_preset.get_data());
		byte_order::order_le_to_native(&temp.m_dest_srate,sizeof(temp.m_dest_srate));
		byte_order::order_le_to_native(&temp.m_quality_scale,sizeof(temp.m_quality_scale));
		p_config = temp;
		return true;
	}
	else return false;
}

static void g_config_to_preset(dsp_preset & p_preset,const t_resampler_config & p_config)
{
	p_preset.set_owner(g_ssrc_guid);
	t_resampler_config temp = p_config;
	byte_order::order_native_to_le(&temp.m_dest_srate,sizeof(temp.m_dest_srate));
	byte_order::order_native_to_le(&temp.m_quality_scale,sizeof(temp.m_quality_scale));
	p_preset.set_data(&temp,sizeof(temp));
}


class dsp_ssrc : public dsp_i_base
{
private:
	ssrc_resampler * res;
	t_resampler_config m_config;
	unsigned m_channels,m_channel_config,m_sample_rate,m_dest_sample_rate;
	bool fast;
	bool whined;

	bool setup_chunk(audio_chunk* chunk)
	{
		unsigned size = res->GetDataDoneSize();
		const audio_sample * ptr = res->GetDataDoneBuffer();
		if (size>0)
		{
			if (chunk==0) chunk = insert_chunk(size);
			chunk->set_data(ptr,size/m_channels,m_channels,m_dest_sample_rate,m_channel_config);
			res->ResetDataDone();
			return true;
		}
		else return false;
	}


public:

	

	static bool g_is_conversion_supported(unsigned src_srate,unsigned dst_srate)
	{
		return !!ssrc_resampler::CanResample(src_srate,dst_srate);
	}

	static GUID g_get_guid()
	{
		return g_ssrc_guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Resampler (SSRC)";}

	bool on_chunk(audio_chunk * chunk)
	{
		UINT new_dest_srate = m_config.m_dest_srate;
		bool new_fast = m_config.m_quality_scale <= 0.5;

		if (res && (chunk->get_channels()!=m_channels || chunk->get_channel_config() != m_channel_config || chunk->get_srate()!=m_sample_rate || new_dest_srate!=m_dest_sample_rate || fast!=new_fast))
		{
			res->Finish();
			setup_chunk(0);
			delete res;
			res=0;
		}

		m_channels = chunk->get_channels();
		m_channel_config = chunk->get_channel_config();
		m_sample_rate = chunk->get_srate();
		m_dest_sample_rate = new_dest_srate;

		fast = new_fast;

		if (!res)
		{
			if (m_dest_sample_rate==m_sample_rate) return true;//same specs, dont resample

			res = SSRC_create(m_sample_rate,m_dest_sample_rate,m_channels,fast);
			if (!res)
			{
				if (!whined)
				{
					console::error(uStringPrintf("unable to resample from %uHz to %uHz",m_sample_rate,m_dest_sample_rate));
					whined = true;
				}
				return true;//unsupported src/dest samplerate combo, could make alt cheap linear resampler for those
			}
		}

		res->Write(chunk->get_data(),chunk->get_data_length());

		return setup_chunk(chunk);
	}


	void on_endofplayback()
	{
		if (res)
		{
			res->Finish();
			setup_chunk(0);
			delete res;
			res=0;
		}
	}

	void flush()
	{
		if (res) {delete res;res=0;}
	}
	
	double get_latency()
	{
		return res ? (double)res->GetLatency() : 0;
	}

	bool need_track_change_mark() {return false;}


	dsp_ssrc() : m_config(g_default_config)
	{
		res=0;
		m_sample_rate = 0;
		m_channels = 0;
		m_channel_config = 0;
		fast=0;
		whined = false;
	}

	~dsp_ssrc()
	{
		if (res) delete res;
	}

	static bool g_have_config_popup() {return true;}

	static bool g_show_config_popup(dsp_preset & p_data,HWND p_parent);

	bool set_data(const dsp_preset & p_preset)
	{
		return g_preset_to_config(m_config,p_preset);
	}
	static bool g_get_default_preset(dsp_preset & p_preset)
	{
		g_config_to_preset(p_preset,g_default_config);
		return true;
	}

	static bool g_create_preset(dsp_preset & p_out,unsigned p_target_srate,float p_quality_scale)
	{
		t_resampler_config temp = {p_target_srate,p_quality_scale};
		g_config_to_preset(p_out,temp);
		return true;
	}

	static float g_get_priority() {return 1;}

};

static resampler_factory_t<dsp_ssrc> g_dsp_ssrc_factory;

static const unsigned g_srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};


class dialog_ssrc_modal : private dialog_helper::dialog
{
public:
	static bool g_run(dsp_preset & p_data,HWND p_parent);
private:
	dialog_ssrc_modal(dsp_preset & p_data) : m_data(p_data) {}
	
	dsp_preset & m_data;

	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);
};

BOOL dialog_ssrc_modal::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			t_resampler_config temp;
			g_preset_to_config(temp,m_data);

			HWND list = GetDlgItem(wnd,IDC_SAMPLERATE);
			int n;
			for(n=0;n<tabsize(g_srate_tab);n++)
			{
				char temp[16];
				sprintf(temp,"%u",g_srate_tab[n]);
				uSendMessageText(list,CB_ADDSTRING,0,temp);
			}
			uSetDlgItemInt(wnd,IDC_SAMPLERATE,temp.m_dest_srate,0);
			uSendDlgItemMessage(wnd,IDC_SLOW,BM_SETCHECK,temp.m_quality_scale > 0.5,0);
		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				t_resampler_config temp;
				int t = uGetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<4000) t=4000;
				else if (t>384000) t=384000;
				temp.m_dest_srate = (unsigned)t;
				temp.m_quality_scale = (float)(uSendDlgItemMessage(wnd,IDC_SLOW,BM_GETCHECK,0,0) ? 1.0 : 0.0);
				g_config_to_preset(m_data,temp);
				end_dialog(1);
			}
			return TRUE;
		case IDCANCEL:
			end_dialog(0);
			return TRUE;
		}
		return FALSE;
	default:
		return FALSE;
	}
}

bool dialog_ssrc_modal::g_run(dsp_preset & p_data,HWND p_parent)
{
	return !!dialog_ssrc_modal(p_data).run_modal(IDD_CONFIG_SSRC,p_parent);
}

bool dsp_ssrc::g_show_config_popup(dsp_preset & p_data,HWND p_parent)
{
	return dialog_ssrc_modal::g_run(p_data,p_parent);
}