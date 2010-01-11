#define MY_VERSION "1.5"

/*****************************************************************************
 *
 * A DSP that interfaces to all of the main features of the SoundTouch library
 *
 * Notes:
 * * The only major problem with this DSP is that SoundTouch only operates on
 * 16-bit integer samples, so I had to instanciate the downsampler service.
 * I don't make dithering a configurable option, though perhaps I might at
 * some point.
 *
 * * The configuration dialog process isn't too pretty, but I think the dialog
 * layout is quite nice. If you feel like redoing any of it, be my guest...
 * Just be so kind as to send the changes my way. :)
 *
 * * For best results, copy/move the source to a directory inside your FB2k
 * SDK source tree. For example, if you unpacked the full SDK to c:\fb2k_sdk,
 * create a directory called foo_dsp_soundtouch inside c:\fb2k_sdk\foobar2000.
 * This is also so you don't have to mess with the relative include paths
 * below.
 *
 * * From there, you can add the SDK and PFC projects to the same workspace as
 * the main plug-in project, and set them as dependencies.
 *
 * * Also for convenience, I recommend adding the SoundTouch library project
 * and setting dependency, rather than adding its library directly to the link
 * settings.
 *
 * * Deal with the SoundTouch headers however you want. Perhaps a folder
 * within your include path, or just change the include you see below. You
 * will, however, have to edit STTypes.h. It redefines BOOL, which is already
 * declared in windef.h, included by windows.h, included by a number of the
 * FB2k SDK headers.
 *
 * * Header comment layout and the majority of the legalese shamelessly ripped
 * from the SoundTouch headers.
 *
 * Change log:
 *
 * 2004-10-18 01:00 UTC - kode54
 * - Fixed a bug in SoundTouch, FIRFilter.cpp, FIRFilter::evaluate, which should be
 *   checking that numSamples is greater than length. At least the SSE filter blows
 *   up if they are equal.
 * - Front-end DSP class now buffers 2048 samples at a time for each putSamples, to
 *   allow for better filtering, I hope.
 * - Version is now 1.5
 *
 * 2004-02-29 12:11 UTC - kode54
 * - Updated to SoundTouch v1.2.1, using float samples
 * - Added exception handling checks around SoundTouch code, just in case
 * - I have made one minor change to STTypes.h, and I don't care to really report it
 *   to the author at this time, as it's really minor. Namely, BOOL should be int.
 * - License changed to GNU Lesser General Public License
 * - Version is now 1.4
 *
 * 2003-10-23 06:14 UTC - kode54
 * - Updated configuration to reflect that quickseek also affects pitch
 * - Version is now 1.3.1
 *
 * 2003-07-28 01:06 UTC - Case
 * - Fixed crash on seek
 * - Version is now 1.3
 *
 * 2003-06-26 07:09 - kode54
 * - Updated code to 0.7 API
 * - Version is now 1.2
 *
 * 2003-05-07 16:47 - kode54
 * - Reduced sliders' vertical dimension by one pixel due to odd graphical
 *   glitch when using Windows Classic theme, reported by RIVANVX
 *
 * 2003-04-05 16:45 - kode54
 * - Added manual update to configuration, real-time mode is now optional
 *
 * 2003-04-04 - kode54
 * - Initial release
 * - Removed cfg_float crap, float/int conversions and rounding caused the
 *   values to drift toward zero every time the config panel was redrawn.
 *
 * Author        : Copyright (c) Chris Moeller 2003
 * Author e-mail : kode54@earthlink.net
 * File created  : 04-Apr-2003
 * Last modified : 05-Apr-2003
 *
 * License :
 *
 *  This file is part of a DSP plug-in for the Foobar2000 audio player, and is
 *  designed to interface the SoundTouch library.
 *
 *  SoundTouch DSP is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  SoundTouch DSP is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with SoundTouch; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************/

#define BOOL FUCKO
#include <SoundTouch.h>
#undef BOOL

#include <foobar2000.h>

#include <stdexcept>
#include <commctrl.h>

#include "resource.h"

using namespace soundtouch;

static const GUID g_soundtouch_guid = 
{ 0x91db1ca, 0x84d2, 0x4bed, { 0xbf, 0x68, 0xe3, 0x97, 0xdf, 0x36, 0x3b, 0xe2 } };

struct t_soundtouch_config
{
	bool do_tempo, do_pitch, do_rate;
	bool quickseek, aa;
	int  aa_length;
	int  tempo, pitch, rate;
	bool realtime;
};

static const t_soundtouch_config g_soundtouch_config_default = {false, false, false, false, true, 32, 0, 0, 0, false};

static HWND g_panel;
static bool g_core_settings_update = false;

static bool g_preset_to_config(t_soundtouch_config & p_config,const dsp_preset & p_preset)
{
	if (p_preset.get_data_size() == sizeof(p_config))
	{
		p_config = * reinterpret_cast<const t_soundtouch_config*>(p_preset.get_data());
		return true;
	}
	else return false;
}

static void g_config_to_preset(dsp_preset & p_preset,const t_soundtouch_config & p_config)
{
	p_preset.set_owner(g_soundtouch_guid);
	p_preset.set_data(&p_config,sizeof(p_config));
}

#define BUFFER_SIZE 2048

class dsp_soundtouch : public dsp_i_base
{
    SoundTouch * pST;
	UINT last_nch, last_srate;
	t_soundtouch_config my_soundtouch;
	t_soundtouch_config m_config;

	mem_block_t<soundtouch::SAMPLETYPE> samplebuf;
	unsigned buffered;

	void insert_chunks()
	{
		uint samples = pST->numSamples();
        if (!samples) return;

		soundtouch::SAMPLETYPE * src = samplebuf.check_size(BUFFER_SIZE * nch);

		do
		{
			samples = pST->receiveSamples(src, BUFFER_SIZE);
			if (samples > 0)
			{
				audio_chunk * chunk = insert_chunk(samples * nch);
				chunk->set_data_32(src, samples, last_nch, last_srate);
			}
		}
		while (samples != 0);
	}

public:
	static GUID g_get_guid()
	{
		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "SoundTouch";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		try
		{
			if (chunk->get_channels() > 2)
			{
				flush();
				return 1;
			}

		// Yes, this is a messy set of conditions.
			if (pST && (last_nch != chunk->get_channels() || last_srate != chunk->get_srate()))
			{
				pST->flush();
				insert_chunks();

				delete pST;
				pST=0;
			}

			last_nch   = chunk->get_channels();
			last_srate = chunk->get_srate();

			if (!pST)
			{
				pST = new SoundTouch;
				if (!pST) return 0;
				pST->setSampleRate(srate);
				pST->setChannels(nch);
				my_soundtouch = m_config;
				if (my_soundtouch.do_tempo)
					pST->setTempoChange((float)(my_soundtouch.tempo) * 0.01f);
				if (my_soundtouch.do_pitch)
					pST->setPitchSemiTones((float)(my_soundtouch.pitch) * 0.01f);
				if (my_soundtouch.do_rate)
					pST->setRateChange((float)(my_soundtouch.rate) * 0.01f);
				pST->setSetting(SETTING_USE_QUICKSEEK, my_soundtouch.quickseek);
				pST->setSetting(SETTING_USE_AA_FILTER, my_soundtouch.aa);
				if (my_soundtouch.aa)
					pST->setSetting(SETTING_AA_FILTER_LENGTH, my_soundtouch.aa_length);
			}
			else
			{
				if (memcmp(&m_config, &my_soundtouch, sizeof(t_soundtouch_config)))
				{
					my_soundtouch = m_config;
					if (my_soundtouch.do_tempo)
						pST->setTempoChange((float)(my_soundtouch.tempo) * 0.01f);
					if (my_soundtouch.do_pitch)
						pST->setPitchSemiTones((float)(my_soundtouch.pitch) * 0.01f);
					if (my_soundtouch.do_rate)
						pST->setRateChange((float)(my_soundtouch.rate) * 0.01f);
					pST->setSetting(SETTING_USE_QUICKSEEK, my_soundtouch.quickseek);
					pST->setSetting(SETTING_USE_AA_FILTER, my_soundtouch.aa);
					if (my_soundtouch.aa)
						pST->setSetting(SETTING_AA_FILTER_LENGTH, my_soundtouch.aa_length);
				}
			}

			unsigned n = chunk->get_sample_count();
			const audio_sample * src = chunk->get_data();
			soundtouch::SAMPLETYPE * dst;
			
			samplebuf.check_size(BUFFER_SIZE * nch);

			while (n)
			{
				unsigned todo = BUFFER_SIZE - buffered;
				if (todo > n) todo = n;

				dst = samplebuf.get_ptr() + buffered * nch;

				for (unsigned i = 0, j = todo * nch; i < j; i++)
				{
					*dst++ = (soundtouch::SAMPLETYPE) (*src++);
				}

				n -= todo;
				buffered += todo;

				if (buffered == BUFFER_SIZE)
				{
					pST->putSamples(samplebuf.get_ptr(), buffered);
					buffered = 0;
					insert_chunks();
				}
			}
		}
		catch (std::runtime_error & e)
		{
			console::warning(e.what());
		}

		return 0;
	}

	virtual void on_endofplayback()
	{
		try
		{
			if (pST)
			{
				insert_chunks();
				if (buffered)
				{
					pST->putSamples(samplebuf.get_ptr(), buffered);
					buffered = 0;
				}
				pST->flush();
				insert_chunks();
				delete pST;
				pST = 0;
			}
		}
		catch (std::runtime_error & e)
		{
			console::warning(e.what());
		}
	}

	virtual void flush()
	{
		try
		{
			if (pST)
			{
				pST->clear();
				/* pST->flush();
				insert_chunks();
				delete pST;
				pST = 0; */
			}
		}
		catch (std::runtime_error & e)
		{
			console::warning(e.what());
		}
		buffered = 0;
	}

	double get_latency()
	{
		try
		{
			return (pST && srate) ? ((double)(pST->numSamples() + buffered) / (double)srate) : 0;
		}
		catch (std::runtime_error & e)
		{
			console::warning(e.what());
		}
		return 0;
	}

	virtual bool need_track_change_mark() {return false;}
	
	dsp_soundtouch() : m_config(g_soundtouch_config_default)
	{
		pST=0;
		srate=0;
		nch=0;
		buffered=0;
		memset(my_soundtouch, 0, sizeof(t_soundtouch_config));
	}

	~dsp_soundtouch()
	{
		if (pST) delete pST;
	}
};

class dialog_soundtouch : private dialog_helper::dialog
{
public:
	static HWND g_create(const dsp_preset & p_initdata,HWND p_parent,unsigned p_id);
	static void g_get_data(HWND p_wnd,dsp_preset & p_data);
	static void g_set_data(HWND p_wnd,const dsp_preset & p_data);
private:

	enum
	{
		MSG_GET_DATA = WM_APP,
		MSG_SET_DATA
	};

	dialog_soundtouch(const dsp_preset & p_initdata)
	{
		g_preset_to_config(m_data,p_initdata);
	}

	void reinitialize();
	t_soundtouch_config m_data;
	void on_change();
	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);

	void enable_aa(bool status);

	void set_dotempo(bool status);
	void set_dopitch(bool status);
	void set_dorate(bool status);
	void set_aa(bool status);

	void upd_tempo(bool apply);
	void upd_pitch(bool apply);
	void upd_rate(bool apply);
	void upd_aalength(bool apply);
};

void dialog_soundtouch::set_dotempo(bool status)
{
	HWND wnd = get_wnd();
	EnableWindow(GetDlgItem(wnd, IDC_QUICKSEEK), status || m_data.do_pitch);
	EnableWindow(GetDlgItem(wnd, IDC_TEMPOSLIDER), status);
	EnableWindow(GetDlgItem(wnd, IDC_TEMPO), status);
	EnableWindow(GetDlgItem(wnd, IDC_TEMPOST), status);
	uSendDlgItemMessage(wnd, IDC_DOTEMPO, BM_SETCHECK, status, 0);
	m_data.do_tempo = status;
}

void dialog_soundtouch::enable_aa(bool status)
{
	HWND wnd = get_wnd();
	EnableWindow(GetDlgItem(wnd, IDC_TAPS), status);
	EnableWindow(GetDlgItem(wnd, IDC_TAPSST), status);
	EnableWindow(GetDlgItem(wnd, IDC_AASLIDER), status);
}

void dialog_soundtouch::set_dopitch(bool status)
{
	HWND wnd = get_wnd();
	EnableWindow(GetDlgItem(wnd, IDC_AA), status || m_data.do_rate);
	EnableWindow(GetDlgItem(wnd, IDC_QUICKSEEK), status || m_data.do_tempo);
	enable_aa((status || m_data.do_rate) && m_data.aa);
	EnableWindow(GetDlgItem(wnd, IDC_PITCHSLIDER), status);
	EnableWindow(GetDlgItem(wnd, IDC_PITCH), status);
	EnableWindow(GetDlgItem(wnd, IDC_PITCHST), status);
	uSendDlgItemMessage(wnd, IDC_DOPITCH, BM_SETCHECK, status, 0);
	m_data.do_pitch = status;
}

void dialog_soundtouch::set_dorate(bool status)
{
	HWND wnd = get_wnd();
	EnableWindow(GetDlgItem(wnd, IDC_AA), status || m_data.do_pitch);
	enable_aa((status || m_data.do_pitch) && m_data.aa);
	EnableWindow(GetDlgItem(wnd, IDC_RATESLIDER), status);
	EnableWindow(GetDlgItem(wnd, IDC_RATE), status);
	EnableWindow(GetDlgItem(wnd, IDC_RATEST), status);
	uSendDlgItemMessage(wnd, IDC_DORATE, BM_SETCHECK, status, 0);
	m_data.do_rate = status;
}

void dialog_soundtouch::set_aa(HWND wnd, int status)
{
	enable_aa((m_data.do_pitch || m_data.do_rate) && status);
	uSendDlgItemMessage(wnd, IDC_AA, BM_SETCHECK, status, 0);
	m_data.aa = status;
}

void dialog_soundtouch::upd_tempo(bool apply)
{
	HWND wnd = get_wnd();
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_TEMPOSLIDER, TBM_GETPOS, 0, 0)-5000;
	temp.add_byte(val < 0 ? '-' : '+');
	temp.add_int(abs(val / 100));
	temp.add_byte('.');
	if (abs(val % 100) < 10) temp.add_byte('0');
	temp.add_int(abs(val % 100));
	uSetDlgItemText(wnd, IDC_TEMPO, temp);
	if (m_data.realtime || apply)
		m_data.tempo = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), m_data.tempo == val ? 0 : 1);
}

void dialog_soundtouch::upd_pitch(bool apply)
{
	HWND wnd = get_wnd();
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_PITCHSLIDER, TBM_GETPOS, 0, 0)-1200;
	temp.add_byte(val < 0 ? '-' : '+');
	temp.add_int(abs(val / 100));
	temp.add_byte('.');
	if (abs(val % 100) < 10) temp.add_byte('0');
	temp.add_int(abs(val % 100));
	uSetDlgItemText(wnd, IDC_PITCH, temp);
	if (m_data.realtime || apply)
		m_data.pitch = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), m_data.pitch == val ? 0 : 1);
}

void dialog_soundtouch::upd_rate(HWND wnd, int apply)
{
	HWND wnd = get_wnd();
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_RATESLIDER, TBM_GETPOS, 0, 0)-5000;
	temp.add_byte(val < 0 ? '-' : '+');
	temp.add_int(abs(val / 100));
	temp.add_byte('.');
	if (abs(val % 100) < 10) temp.add_byte('0');
	temp.add_int(abs(val % 100));
	uSetDlgItemText(wnd, IDC_RATE, temp);
	if (m_data.realtime || apply)
		m_data.rate = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), m_data.rate == val ? 0 : 1);
}

void dialog_soundtouch::upd_aalength(bool apply)
{
	HWND wnd = get_wnd();
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_AASLIDER, TBM_GETPOS, 0, 0);
	temp.add_int(val);
	uSetDlgItemText(wnd, IDC_TAPS, temp);
	if (m_data.realtime || apply)
		m_data.aa_length = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), m_data.aa_length == val ? 0 : 1);
}

void dialog_soundtouch::reinitialize()
{
	HWND wnd = get_wnd();
	uSendDlgItemMessage(wnd, IDC_QUICKSEEK, BM_SETCHECK, m_data.quickseek, 0);
	uSendDlgItemMessage(wnd, IDC_REALTIME, BM_SETCHECK, m_data.realtime, 0);
	set_dotempo(wnd, m_data.do_tempo);
	set_dopitch(wnd, m_data.do_pitch);
	set_dorate(wnd, m_data.do_rate);
	set_aa(wnd, m_data.aa);
	uSendDlgItemMessage(wnd,IDC_TEMPOSLIDER,TBM_SETRANGE,0,MAKELONG(0,15000));
	uSendDlgItemMessage(wnd,IDC_TEMPOSLIDER,TBM_SETPOS,1,m_data.tempo+5000);
	upd_tempo(false);
	uSendDlgItemMessage(wnd,IDC_PITCHSLIDER,TBM_SETRANGE,0,MAKELONG(0,2400));
	uSendDlgItemMessage(wnd,IDC_PITCHSLIDER,TBM_SETPOS,1,m_data.pitch+1200);
	upd_pitch(false);
	uSendDlgItemMessage(wnd,IDC_RATESLIDER,TBM_SETRANGE,0,MAKELONG(0,15000));
	uSendDlgItemMessage(wnd,IDC_RATESLIDER,TBM_SETPOS,1,m_data.rate+5000);
	upd_rate(false);
	uSendDlgItemMessage(wnd,IDC_AASLIDER,TBM_SETRANGE,0,MAKELONG(8,128));
	uSendDlgItemMessage(wnd,IDC_AASLIDER,TBM_SETPOS,1,m_data.aa_length);
	upd_aalength(false);
}

BOOL dialog_soundtouch::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	switch(msg)
	{
	case WM_INITDIALOG:
		reinitialize();
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_QUICKSEEK:
			cfg_quickseek = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_DOTEMPO:
			set_dotempo(wnd, uSendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDC_DOPITCH:
			set_dopitch(wnd, uSendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDC_DORATE:
			set_dorate(wnd, uSendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDC_AA:
			set_aa(wnd, uSendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDC_REALTIME:
			cfg_realtime = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_APPLY:
			upd_tempo(wnd, 1);
			upd_pitch(wnd, 1);
			upd_rate(wnd, 1);
			upd_aalength(wnd, 1);
			EnableWindow((HWND)lp, 0);
			break;
		}
	case WM_HSCROLL:
		switch(uGetWindowLong((HWND)lp,GWL_ID))
		{
		case IDC_TEMPOSLIDER:
			upd_tempo(wnd, 0);
			break;
		case IDC_PITCHSLIDER:
			upd_pitch(wnd, 0);
			break;
		case IDC_RATESLIDER:
			upd_rate(wnd, 0);
			break;
		case IDC_AASLIDER:
			upd_aalength(wnd, 0);
			break;
		}
		break;
	}
	return 0;
}

class config_soundtouch : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG_ST,parent,ConfigProc);
	}
	virtual const char * get_name() {return "SoundTouch";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

class version_soundtouch : public componentversion
{
public:
	virtual void get_file_name(string_base & out) { out.set_string(core_api::get_my_file_name()); }
	virtual void get_component_name(string_base & out) { out.set_string("SoundTouch DSP"); }
	virtual void get_component_version(string_base & out) { out.set_string(MY_VERSION); }
	virtual void get_about_message(string_base & out)
	{
		out.set_string("Processes audio using SoundTouch v");
		out.add_string(SoundTouch::getVersionString());
		out.add_string(".\n\nThis plug-in is licensed under the GNU Lesser\nGeneral Public License. See COPYING.TXT.");
	}
};

static service_factory_t<dsp,dsp_soundtouch> foo;
static service_factory_single_t<config,config_soundtouch> foo2;
static service_factory_single_t<componentversion,version_soundtouch> foo3;
