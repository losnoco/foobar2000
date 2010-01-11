#include "../../SDK/foobar2000.h"
#include "../../helpers/helpers.h"
#include <math.h>
#include <commctrl.h>
#include "../resource.h"
#include "supereq.h"
#include "paramlist.h"

enum {N_BANDS=18};


// {82AEF845-DCC3-4da5-9D80-E9A972B2140D}
static const GUID g_eq_guid = 
{ 0x82aef845, 0xdcc3, 0x4da5, { 0x9d, 0x80, 0xe9, 0xa9, 0x72, 0xb2, 0x14, 0xd } };

struct t_eq_config
{
	char bands[N_BANDS];
};

static const t_eq_config g_eq_config_default = {20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20};

static HWND g_panel;
static bool g_core_settings_update = false;

static void setup_bands(const t_eq_config & src,double dst[N_BANDS])
{
	int n;
	for(n=0;n<N_BANDS;n++)
	{
		dst[n] = (double)pow(10.0,(src.bands[n]-20)/-20.0);
	}
}

static bool g_preset_to_config(t_eq_config & p_config,const dsp_preset & p_preset)
{
	if (p_preset.get_data_size() == sizeof(p_config))
	{
		p_config = * reinterpret_cast<const t_eq_config*>(p_preset.get_data());
		return true;
	}
	else return false;
}

static void g_config_to_preset(dsp_preset & p_preset,const t_eq_config & p_config)
{
	p_preset.set_owner(g_eq_guid);
	p_preset.set_data(&p_config,sizeof(p_config));
}

class dsp_supereq : public dsp_i_base
{
private:
	ptr_list_t<supereq_base> eqs;
	unsigned m_last_channels,m_last_channel_config,m_last_sample_rate;
	paramlist paramroot;
	t_eq_config my_eq;
	t_eq_config m_config;

	mem_block_t<audio_sample> interlace_buf;

	void eq_write_chunk(const audio_chunk * chunk)
	{
		UINT n;
		if (!chunk->is_empty())
		{
			UINT nch = chunk->get_channels(), samples = chunk->get_sample_count();

			const audio_sample * data = chunk->get_data();

			interlace_buf.check_size(samples);//deinterlace
			
			
			for(n=0;n<nch;n++)
			{
				UINT s;
				for(s=0;s<samples;s++)
					interlace_buf[s]=data[s*nch+n];
				
				eqs[n]->write_samples(interlace_buf,samples);
			}
		}
	}

	void eq_read_chunk(audio_chunk * chunk)
	{
		unsigned total_samples = 0;
		unsigned n;

		for(n=0;n<m_last_channels;n++)
		{
			int samples_out = 0;
			audio_sample *data_out = eqs[n]->get_output(&samples_out);
			if (total_samples<(unsigned)samples_out) total_samples=samples_out;
			interlace_buf.check_size(total_samples*m_last_channels);
			unsigned s;
			for(s=0;s<(unsigned)samples_out;s++)
				interlace_buf[s*m_last_channels+n]=data_out[s];
		}

		if (total_samples>0)
		{
			if (chunk==0) chunk = insert_chunk(total_samples * m_last_channels);
			if (chunk) chunk->set_data(interlace_buf,total_samples,m_last_channels,m_last_sample_rate,m_last_channel_config);
		}
		else if (chunk)
		{
			chunk->reset();
		}
	}
	
public:
	dsp_supereq() : m_config(g_eq_config_default)
	{
	}

	~dsp_supereq()
	{
		eqs.delete_all();
	}

	static GUID g_get_guid()
	{
		return g_eq_guid;
	}

	static void g_get_name(string_base & p_out) { p_out = "Equalizer";}

	
	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (eqs.get_count()>0 && (m_last_channels!=chunk->get_channels() || m_last_channel_config != chunk->get_channel_config() || m_last_sample_rate!=chunk->get_srate()))
		{
			unsigned n;
			for(n=0;n<m_last_channels;n++)
				eqs[n]->write_samples(0,0);

			eq_read_chunk(0);

			eqs.delete_all();
		}

		m_last_sample_rate = chunk->get_srate();
		m_last_channels= chunk->get_channels();
		m_last_channel_config = chunk->get_channel_config();
		
		if (eqs.get_count()==0)
		{
			unsigned n;
			for(n=0;n<m_last_channels;n++)
				eqs.add_item(new supereq<float>);
			double bands[N_BANDS];
			my_eq = m_config;
			setup_bands(my_eq,bands);
			for(n=0;n<m_last_channels;n++)
				eqs[n]->equ_makeTable(bands,&paramroot,(double)m_last_sample_rate);
		}
		else
		{
			if (memcmp(&m_config,&my_eq,sizeof(t_eq_config)))
			{
				my_eq = m_config;
				double bands[N_BANDS];
				setup_bands(my_eq,bands);
				UINT n;
				for(n=0;n<m_last_channels;n++)
					eqs[n]->equ_makeTable(bands,&paramroot,(double)m_last_sample_rate);
			}
		}

		if (eqs.get_count()>0)
		{
			eq_write_chunk(chunk);
			eq_read_chunk(chunk);
			return !chunk->is_empty();
		}
		else return true;
	}

	virtual void on_endofplayback()
	{
		if ((unsigned)eqs.get_count()>0 && m_last_channels==(unsigned)eqs.get_count())
		{
			unsigned n;
			for(n=0;n<m_last_channels;n++)
				eqs[n]->write_samples(0,0);

			eq_read_chunk(0);
		}
	}

	virtual double get_latency()
	{
		return eqs.get_count()>0 ? (double)eqs[0]->samples_buffered() / (double)m_last_sample_rate : 0;
	}

	virtual void flush()
	{
		eqs.delete_all();
	}

	bool need_track_change_mark() {return false;}

	static bool g_have_config_popup() {return true;}

	static bool g_show_config_popup(dsp_preset & p_data,HWND p_parent);

	bool set_data(const dsp_preset & p_preset)
	{
		return g_preset_to_config(m_config,p_preset);
	}
	static bool g_get_default_preset(dsp_preset & p_preset)
	{
		g_config_to_preset(p_preset,g_eq_config_default);
		return true;
	}
};

static dsp_factory_t<dsp_supereq> foo;
//static service_factory_single_t<config,config_eq> foo2;

static const int slider_ids[N_BANDS]={IDC_SLIDER1,IDC_SLIDER2,IDC_SLIDER3,IDC_SLIDER4,IDC_SLIDER5,IDC_SLIDER6,IDC_SLIDER7,IDC_SLIDER8,IDC_SLIDER9,IDC_SLIDER10,IDC_SLIDER11,IDC_SLIDER12,IDC_SLIDER13,IDC_SLIDER14,IDC_SLIDER15,IDC_SLIDER16,IDC_SLIDER17,IDC_SLIDER18};
static const int display_ids[N_BANDS]={IDC_EQ1,IDC_EQ2,IDC_EQ3,IDC_EQ4,IDC_EQ5,IDC_EQ6,IDC_EQ7,IDC_EQ8,IDC_EQ9,IDC_EQ10,IDC_EQ11,IDC_EQ12,IDC_EQ13,IDC_EQ14,IDC_EQ15,IDC_EQ16,IDC_EQ17,IDC_EQ18};

class dialog_eq : private dialog_helper::dialog
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

	dialog_eq(const dsp_preset & p_initdata)
	{
		g_preset_to_config(m_data,p_initdata);
	}
	
	void reinitialize();
	t_eq_config m_data;
	void on_change();
	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);
};

static void eq_format(char *str, int val)
{
	int dbval;

	dbval = -(val-20);

	if (dbval < 0)
	{
		if (dbval <= -10)
		{
			sprintf(str,"-%u",abs(dbval));
		}
		else
		{
			sprintf(str," -%u",abs(dbval));
		}
	}
	else if (dbval >= 0)
	{
		if (dbval >= 10)
		{
			sprintf(str," %u", abs(dbval));
		}
		else
		{
			sprintf(str,"  %u", abs(dbval));
		}
	}								
}


void dialog_eq::reinitialize()
{
	HWND wnd = get_wnd();
	char temp[32];
	unsigned n;
	for(n=0;n<N_BANDS;n++)
	{
		HWND s = GetDlgItem(wnd,slider_ids[n]);
		SendMessage(s,TBM_SETRANGE,0,MAKELONG(0,40));
		SendMessage(s,TBM_SETPOS,1,m_data.bands[n]);
		SendMessage(s,TBM_SETPAGESIZE, 0, 1);		
		eq_format(temp, m_data.bands[n]);
		uSetDlgItemText(wnd,display_ids[n],temp);
	}
}

void dialog_eq::g_set_data(HWND p_wnd,const dsp_preset & p_data)
{
	uSendMessage(p_wnd,MSG_SET_DATA,0,(LPARAM)&p_data);	
}
void dialog_eq::on_change()
{
	HWND wnd = get_wnd();
	uPostMessage(GetParent(wnd),WM_COMMAND,(BN_CLICKED<<16)|(0xFFFF & uGetWindowLong(wnd,GWL_ID)),(LPARAM)wnd);
}

HWND dialog_eq::g_create(const dsp_preset & p_initdata,HWND p_parent,unsigned p_id)
{
	HWND wnd = (new dialog_eq(p_initdata))->run_modeless(IDD_CONFIG_EQ_EMBEDDED,p_parent);
	if (wnd)
	{
		uSetWindowLong(wnd,GWL_ID,p_id);
		uSetWindowLong(wnd,GWL_EXSTYLE,uGetWindowLong(wnd,GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
	}
	return wnd;
}

void dialog_eq::g_get_data(HWND p_wnd,dsp_preset & p_data)
{
	uSendMessage(p_wnd,MSG_GET_DATA,0,(LPARAM)&p_data);
}

BOOL dialog_eq::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	switch(msg)
	{
	case WM_DESTROY:
		delete this;
		return TRUE;
	case MSG_GET_DATA:
		g_config_to_preset(*reinterpret_cast<dsp_preset*>(lp),m_data);
		return TRUE;
	case MSG_SET_DATA:
		g_preset_to_config(m_data,*reinterpret_cast<const dsp_preset*>(lp));
		reinitialize();
		return TRUE;
	case WM_INITDIALOG:
		reinitialize();
		return TRUE;
	case WM_VSCROLL:
		{
			char temp[32];
			int id = GetWindowLong((HWND)lp,GWL_ID);
			int n;
			for(n=0;n<N_BANDS;n++)
			{
				if (slider_ids[n]==id) break;
			}
			if (n<N_BANDS)
			{
				m_data.bands[n]=SendMessage((HWND)lp,TBM_GETPOS,0,0);
				eq_format(temp, m_data.bands[n]);
				uSetDlgItemText(wnd,display_ids[n],temp);
				on_change();
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_ADJUST_MAX0:
			{
				char temp[32];
				int n,max = 40;
				for(n=0;n<N_BANDS;n++)
					if (m_data.bands[n]<max) max = m_data.bands[n];
				int delta = 20 - max;
				for(n=0;n<N_BANDS;n++)
				{
					int v = m_data.bands[n] + delta;
					if (v<0) v=0;
					else if (v>40) v=40;
					m_data.bands[n] = v;
					SendDlgItemMessage(wnd,slider_ids[n],TBM_SETPOS,1,v);
					eq_format(temp, v);
					SetDlgItemTextA(wnd,display_ids[n],temp);
				}
				on_change();
			}
			return TRUE;
		case IDC_BUTTONZERO:
			{
				char temp[32];
				unsigned n;
				for(n=0;n<N_BANDS;n++)
				{
					HWND s = GetDlgItem(wnd,slider_ids[n]);
					SendMessage(s,TBM_SETPOS,1,20);		
					m_data.bands[n]=20;
					eq_format(temp, m_data.bands[n]);
					uSetDlgItemText(wnd,display_ids[n],temp);
				};
				on_change();
			}
			return TRUE;
		case IDC_BUTTONSTOREPRESET:
			{
				OPENFILENAME openf;
				TCHAR buffer[MAX_PATH];	
				TCHAR fpath[MAX_PATH], *splitter;
				FILE *settingsfile;

				GetModuleFileName(0, fpath, tabsize(fpath));
				for (splitter = fpath+_tcslen(fpath); *splitter != '\\'; splitter--)
					*splitter = '\0';
				_tcscat(fpath, TEXT("Equalizer Presets\\"));

	 			memset(&openf, 0, sizeof(OPENFILENAME));
				memset(&buffer, 0, sizeof(buffer));
				openf.lStructSize = sizeof(OPENFILENAME);
				openf.hwndOwner = wnd;
				openf.hInstance = core_api::get_my_instance();     				
				openf.lpstrFilter =	TEXT("Equalizer presets (*.feq)\0*.feq\0");
				openf.lpstrCustomFilter = NULL;
				openf.lpstrFile = buffer;
				openf.nMaxFile = tabsize(buffer);
				openf.lpstrInitialDir = fpath;
				openf.lpstrDefExt = TEXT("feq");
				//openf.Flags = OFN_FILEMUSTEXIST;
				if (GetSaveFileName(&openf)) {
					settingsfile = _tfopen(buffer, TEXT("w"));

					if (settingsfile != NULL) {
						int n;
						for(n=0;n<N_BANDS;n++)
						{
							fprintf(settingsfile, "%d\n", -(m_data.bands[n]-20));							
						}
						fclose(settingsfile);
					}
				}
			}
			return TRUE;
		case IDC_BUTTONLOADPRESET:
			{
				OPENFILENAME openf;
				TCHAR buffer[MAX_PATH];	
				TCHAR fpath[MAX_PATH], *splitter;
				FILE *settingsfile;

				GetModuleFileName(0, fpath, tabsize(fpath));
				for (splitter = fpath+_tcslen(fpath); *splitter != '\\'; splitter--)
					*splitter = '\0';
				_tcscat(fpath, TEXT("Equalizer Presets\\"));


	 		    memset(&openf, 0, sizeof(OPENFILENAME));
				memset(&buffer, 0, sizeof(buffer));
				openf.lStructSize = sizeof(OPENFILENAME);
				openf.hwndOwner = wnd;
				openf.hInstance = core_api::get_my_instance();     				
                openf.lpstrFilter =	TEXT("Equalizer presets (*.feq)\0*.feq\0");
				openf.lpstrCustomFilter = NULL;
				openf.lpstrFile = buffer;
				openf.nMaxFile = tabsize(buffer);
				openf.lpstrInitialDir = fpath;
				openf.lpstrDefExt = TEXT("feq");
				openf.Flags = OFN_FILEMUSTEXIST;
				if (GetOpenFileName(&openf)) {
					settingsfile = _tfopen(buffer, TEXT("r"));

					if (settingsfile != NULL) {
						int n;
						for(n=0;n<N_BANDS;n++)
						{
							int readval;
							if (fscanf(settingsfile, "%d", &readval) == 1)
							{
								char temp[32];
								HWND s = GetDlgItem(wnd,slider_ids[n]);
								m_data.bands[n] = ((-readval)+20);									
								SendMessage(s,TBM_SETPOS,1,m_data.bands[n]);
								eq_format(temp, m_data.bands[n]);
								SetDlgItemTextA(wnd,display_ids[n],temp);
							}
						}
						fclose(settingsfile);
						on_change();
					}
				}
			}
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}


class dialog_eq_modal : private dialog_helper::dialog
{
public:
	static bool g_run(dsp_preset & p_data,HWND p_parent);
private:
	dialog_eq_modal(dsp_preset & p_data) : m_data(p_data) {}
	
	dsp_preset & m_data;

	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);
};

bool dialog_eq_modal::g_run(dsp_preset & p_data,HWND p_parent)
{
	return !!dialog_eq_modal(p_data).run_modal(IDD_CONFIG_EQ_POPUP,p_parent);
}

BOOL dialog_eq_modal::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND child;
			child = dialog_eq::g_create(m_data,wnd,IDC_EQ_EMBEDDED);
			if (child)
				SetWindowPos(child,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE);

		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			dialog_eq::g_get_data(GetDlgItem(wnd,IDC_EQ_EMBEDDED),m_data);
			end_dialog(1);
			return TRUE;
		case IDCANCEL:
			end_dialog(0);
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}

bool dsp_supereq::g_show_config_popup(dsp_preset & p_data,HWND p_parent)
{
	return dialog_eq_modal::g_run(p_data,p_parent);	
}


class dialog_eq_panel : private dialog_helper::dialog
{
public:
	static HWND g_create(HWND p_parent);
private:
	bool is_enabled();
	void on_change();
	void reinitialize();
	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);
};

void dialog_eq_panel::reinitialize()
{
	HWND wnd = get_wnd();

	static_api_ptr_t<dsp_config_manager> api;
	dsp_chain_config_impl cfg;
	api->get_core_settings(cfg);
	dsp_preset_impl data;
	dsp_supereq::g_get_default_preset(data);

	unsigned n,m = cfg.get_count();
	bool enabled = false;
	for(n=0;n<m;n++)
	{
		const dsp_preset & itemref = cfg.get_item(n);
		if (itemref.get_owner() == g_eq_guid)
		{
			enabled = true;
			data.copy(itemref);
			break;
		}
	}

	uSendDlgItemMessage(wnd,IDC_EQ_ENABLED,BM_SETCHECK,enabled?BST_CHECKED:BST_UNCHECKED,0);

	HWND child = GetDlgItem(wnd,IDC_EQ_EMBEDDED);
	if (child == 0)
	{
		child = dialog_eq::g_create(data,wnd,IDC_EQ_EMBEDDED);
		if (child)
			SetWindowPos(child,0,0,0,0,0,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE);

	}
	else
	{
		dialog_eq::g_set_data(child,data);
	}
}


bool dialog_eq_panel::is_enabled()
{
	return uSendDlgItemMessage(get_wnd(),IDC_EQ_ENABLED,BM_GETCHECK,0,0) == BST_CHECKED;
}

BOOL dialog_eq_panel::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	switch(msg)
	{
	case WM_APP:
		reinitialize();
		return TRUE;
	case WM_INITDIALOG:
		{
			g_panel = wnd;
			modeless_dialog_manager::add(wnd);

			reinitialize();
		}
		return TRUE;
	case WM_DESTROY:
		if (g_panel == wnd) g_panel = 0;
		modeless_dialog_manager::remove(wnd);
		delete this;
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_EQ_ENABLED:
			on_change();
			return TRUE;
		case IDC_EQ_EMBEDDED:
			if (is_enabled()) on_change();
			return TRUE;
		case IDCANCEL:
			DestroyWindow(wnd);
			return TRUE;
		default:
			return FALSE;
		}
	case WM_CLOSE:
		DestroyWindow(wnd);
		return TRUE;
	default:
		return FALSE;
	}
}

void dialog_eq_panel::on_change()
{
	HWND wnd = get_wnd();
	bool enabled = is_enabled();
	if (enabled)
	{
		static_api_ptr_t<dsp_config_manager> api;
		dsp_chain_config_impl cfg;
		api->get_core_settings(cfg);

		dsp_preset_impl data;
		dialog_eq::g_get_data(GetDlgItem(wnd,IDC_EQ_EMBEDDED),data);
		bool found = false;
		unsigned n,m = cfg.get_count();
		for(n=0;n<m;n++)
		{
			if (cfg.get_item(n).get_owner() == g_eq_guid)
			{
				cfg.replace_item(data,n);
				found = true;
				break;
			}
		}
		if (!found)
		{
			cfg.insert_item(data,0);
		}

		{
			vartoggle_t<bool> meh(g_core_settings_update,true);
			api->set_core_settings(cfg);
		}
	}
	else
	{
		static_api_ptr_t<dsp_config_manager> api;
		dsp_chain_config_impl cfg;
		api->get_core_settings(cfg);

		unsigned n,m = cfg.get_count();
		bit_array_bittable mask(m);
		bool changed = false;
		for(n=0;n<m;n++)
		{
			bool axe = (cfg.get_item(n).get_owner() == g_eq_guid) ? true : false;
			if (axe) changed = true;
			mask.set(n,axe);
		}
		if (changed)
		{
			cfg.remove_mask(mask);

			{
				vartoggle_t<bool> meh(g_core_settings_update,true);
				api->set_core_settings(cfg);
			}
		}
	}
}

HWND dialog_eq_panel::g_create(HWND p_parent)
{
	if (g_panel)
	{
		ShowWindow(g_panel,SW_SHOW);
		return g_panel;
	}

	HWND wnd = (new dialog_eq_panel())->run_modeless(IDD_CONFIG_EQ_PANEL,p_parent);
	if (wnd) ShowWindow(wnd,SW_SHOW);
	return wnd;
}


class menu_item_equalizer : public menu_item_legacy_main_single
{
public:

	GUID get_guid()
	{
		// {C7BBF941-5877-4e11-A9BD-BC19E6A67027}
		static const GUID guid = 
		{ 0xc7bbf941, 0x5877, 0x4e11, { 0xa9, 0xbd, 0xbc, 0x19, 0xe6, 0xa6, 0x70, 0x27 } };
		return guid;
	}

	void get_name(string_base & out) {out = "Equalizer";}
	void get_default_path(string_base & out) {out = "Components";}
	bool get_description(string_base & out) {out = "Activates equalizer panel.";return true;}
	
	void run()
	{
		dialog_eq_panel::g_create(core_api::get_main_window());
	}
};

static menu_item_factory_t<menu_item_equalizer> g_menu_item_equalizer_factory;

class dsp_config_callback_impl : public dsp_config_callback
{
public:
	void on_core_settings_change(const dsp_chain_config & p_newdata)
	{
		if (g_panel && !g_core_settings_update) uSendMessage(g_panel,WM_APP,0,0);
	}
};

static service_factory_single_t<dsp_config_callback,dsp_config_callback_impl> g_dsp_config_callback_impl_factory;