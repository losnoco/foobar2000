#include <foobar2000.h>

// for pincrement/pdecrement, which are fastcall; InterlockedIncrement etc are stdcall
#include <ptypes.h>
USING_PTYPES

// for uFormatMessage
#include "../ui_extension/utf8api.h"

/* volume.cpp */
void g_update_controls(int volume);

/* config.cpp */
extern cfg_int cfg_which_mixer, cfg_mixer_control;

BOOL uFormatMediaMessage(DWORD dw_error, string_base & out)
{
	return uFormatMessage(dw_error, out, uGetModuleHandle("WINMM"));
}

class console_cache
{
	class message_t
	{
	public:
		enum type_t
		{
			M_NONE = 0,
			M_INFO,
			M_WARN,
			M_ERROR
		};

	private:
		char * text;
		type_t type;

	public:
		message_t() : text(0), type(M_NONE) {}
		message_t(const char * m, type_t t) : text(strdup(m)), type(t) {}

		~message_t()
		{
			if (text) free(text);
		}

		inline const char * get_text() const
		{
			return text ? text : "";
		}

		inline type_t get_type() const
		{
			return type;
		}
	};

	critical_section      sync;
	ptr_list_t<message_t> messages;

public:
	console_cache() {}
	~console_cache()
	{
		if (messages.get_count())
		{
			string8 out;
			for (unsigned i = 0, j = messages.get_count(); i < j; i++)
			{
				message_t * message = messages[i];
				if (out.length()) out.add_byte('\n');
				switch (message->get_type())
				{
				case message_t::M_INFO:
					out += "info";
					break;

				case message_t::M_WARN:
					out += "warning";
					break;

				case message_t::M_ERROR:
					out += "error";
					break;
				}
				
				out.add_byte(':');
				out.add_byte(' ');

				out += message->get_text();
			}

			if (out.length()) uMessageBox(NULL, out, "UIE Volume notices", 0);
		}
		messages.delete_all();
	}

	void dump_messages()
	{
		insync(sync);
		for (unsigned i = 0, j = messages.get_count(); i < j; i++)
		{
			message_t * message = messages[i];
			switch (message->get_type())
			{
			case message_t::M_INFO:
				console::info(message->get_text());
				break;

			case message_t::M_WARN:
				console::warning(message->get_text());
				break;

			case message_t::M_ERROR:
				console::error(message->get_text());
				break;
			}
		}
		messages.delete_all();
	}

	void drop_messages()
	{
		insync(sync);
		messages.delete_all();
	}

	void info(const char * m)
	{
		if (core_api::are_services_available())
		{
			dump_messages();
			console::info(m);
		}
		else
		{
			insync(sync);
			messages.add_item(new message_t(m, message_t::M_INFO));
		}
	}

	void warning(const char * m)
	{
		if (core_api::are_services_available())
		{
			dump_messages();
			console::warning(m);
		}
		else
		{
			insync(sync);
			messages.add_item(new message_t(m, message_t::M_WARN));
		}
	}

	void error(const char * m)
	{
		if (core_api::are_services_available())
		{
			dump_messages();
			console::info(m);
		}
		else
		{
			insync(sync);
			messages.add_item(new message_t(m, message_t::M_ERROR));
		}
	}
};

static console_cache consol;

class console_dump : public initquit
{
	void on_init()
	{
		consol.dump_messages();
	}

	void on_quit() {}

	void on_system_shutdown()
	{
		consol.drop_messages();
	}
};

static const char class_name[] = "8DEB9B7A-B049-4599-AE4B-258E9D58D5F3";

class mixer_control
{
	HWND wnd;

	HMIXER m_hMixer;
	DWORD m_dwMinimum, m_dwMaximum;
	DWORD m_dwVolumeControlID;

public:
	mixer_control();
	~mixer_control();

	static LRESULT WINAPI wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	LRESULT WINAPI on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	bool ChangeMixer(unsigned mixer, unsigned control);

	int  GetVolume();
	void SetVolume(int volume);
};

class mixer_register_window_class
{
	int ref_count;
	ATOM class_atom;

public:
	mixer_register_window_class() : ref_count(0), class_atom(0) {}
	~mixer_register_window_class()
	{
		if (ref_count) _unregister();
	}

	ATOM Register()
	{
		if (pincrement(&ref_count) == 1)
		{
			_register();
			if (!class_atom) ref_count = 0;
		}
		return class_atom;
	}

	void Unregister()
	{
		if (pdecrement(&ref_count) == 0) _unregister();
	}

private:
	void _register()
	{
		uWNDCLASS wc;
		memset(&wc, 0, sizeof(wc));
		wc.lpfnWndProc   = (WNDPROC)mixer_control::wnd_proc;
		wc.hInstance     = core_api::get_my_instance();
		wc.hCursor       = uLoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
		wc.lpszClassName = class_name;
		class_atom = uRegisterClass(&wc);
	}

	void _unregister()
	{
		if (class_atom)
		{
			uUnregisterClass((const char *)class_atom, core_api::get_my_instance());
			class_atom = 0;
		}
	}
};

static mixer_register_window_class rwc;

mixer_control::mixer_control() : wnd(0), m_hMixer(0) {}
mixer_control::~mixer_control()
{
	ChangeMixer(0, 0);
}

LRESULT WINAPI mixer_control::wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	mixer_control * p_this;
	
	if(msg == WM_NCCREATE)
	{
		p_this = (mixer_control *)((CREATESTRUCT *)(lp))->lpCreateParams;
		uSetWindowLong(wnd, GWL_USERDATA, (LPARAM)p_this);
	}
	else
		p_this = reinterpret_cast<mixer_control*>(uGetWindowLong(wnd,GWL_USERDATA));
	
	return p_this ? p_this->on_message(wnd, msg, wp, lp) : uDefWindowProc(wnd, msg, wp, lp);
}

LRESULT WINAPI mixer_control::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == MM_MIXM_CONTROL_CHANGE)
	{
		if (reinterpret_cast<HMIXER>(wp) == m_hMixer &&
			static_cast<DWORD>(lp) == m_dwVolumeControlID)
		{
			g_update_controls(GetVolume());
		}
	}

	return uDefWindowProc(wnd, msg, wp, lp);
}

bool mixer_control::ChangeMixer(unsigned mixer, unsigned control)
{
	if (m_hMixer != NULL)
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;
	}

	if (wnd && !mixer)
	{
		DestroyWindow(wnd);
		wnd = 0;
		rwc.Unregister();
	}

	if (mixer)
	{
		DWORD n_devs = mixerGetNumDevs();

		if (mixer > n_devs) return false;

		if (!wnd)
		{
			ATOM wnd_class = rwc.Register();
			if (!wnd_class)
			{
				string8 error;
				uFormatMessage(GetLastError(), error);
				console::error(uStringPrintf("Failed to register mixer callback window class - %s", pconst(error)));
				return false;
			}

			wnd = uCreateWindowEx(0, (const char *)wnd_class, "uninteresting", 0, 0, 0, 0, 0, core_api::get_main_window(), 0, core_api::get_my_instance(), this);
			if (!wnd)
			{
				string8 error;
				uFormatMessage(GetLastError(), error);
				console::error(uStringPrintf("Failed to create mixer callback window - %s", pconst(error)));
				rwc.Unregister();
				return false;
			}
		}

		MMRESULT ret;
		const char * errorfmt = "mixerOpen failed - %s";

		if ((ret = mixerOpen(&m_hMixer, mixer - 1, reinterpret_cast<DWORD>(wnd), NULL, MIXER_OBJECTF_MIXER | CALLBACK_WINDOW)) == MMSYSERR_NOERROR)
		{
			MIXERLINE mxl;
			memset(&mxl, 0, sizeof(mxl));
			mxl.cbStruct = sizeof(mxl);
			mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

			do
			{
				if ((ret = mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(m_hMixer), &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)) == MMSYSERR_NOERROR)
				{
					if (control && control <= mxl.cConnections)
					{
						memset(&mxl, 0, sizeof(mxl));
						mxl.cbStruct = sizeof(mxl);
						mxl.dwSource = control - 1;
						if ((ret = mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(m_hMixer), &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_SOURCE))!= MMSYSERR_NOERROR)
						{
							errorfmt = "mixerGetLineInfo failed to enumerate source control - %s";
							break;
						}
					}

					MIXERCONTROL mxc;
					MIXERLINECONTROLS mxlc;
					mxlc.cbStruct = sizeof(mxlc);
					mxlc.dwLineID = mxl.dwLineID;
					mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
					mxlc.cControls = 1;
					mxlc.cbmxctrl = sizeof(mxc);
					mxlc.pamxctrl = &mxc;
					if ((ret = mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(m_hMixer), &mxlc, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)) == MMSYSERR_NOERROR)
					{
						m_dwMinimum = mxc.Bounds.dwMinimum;
						m_dwMaximum = mxc.Bounds.dwMaximum;
						m_dwVolumeControlID = mxc.dwControlID;
						return true;
					}
					else
					{
						errorfmt = "mixerGetLineControls failed - %s";
					}
				}
				else
				{
					errorfmt = "mixerGetLineInfo failed to enumerate master volume control - %s";
				}
			}
			while (0);

			mixerClose(m_hMixer);
		}

		DestroyWindow(wnd);
		wnd = 0;
		rwc.Unregister();

		string8 error;
		uFormatMediaMessage(ret, error);
		consol.error(uStringPrintf(errorfmt, pconst(error)));

		return false;
	}

	return true;
}

int g_db_to_vol(int db)
{
	db = (int)((double)pow(10., (double)db * (1. / 2000.)) * 1000.);
	if (db == 1) return 0;
	return db;
}

int g_vol_to_db(int vol)
{
	return vol>0 ? (int)(2000*log10((double)vol * .001)) : -6000;
}

int mixer_control::GetVolume()
{
	if (m_hMixer)
	{
		MMRESULT ret;
		MIXERCONTROLDETAILS_UNSIGNED mxcdVolume;
		MIXERCONTROLDETAILS mxcd;
		mxcd.cbStruct = sizeof(mxcd);
		mxcd.dwControlID = m_dwVolumeControlID;
		mxcd.cChannels = 1;
		mxcd.cMultipleItems = 0;
		mxcd.cbDetails = sizeof(mxcdVolume);
		mxcd.paDetails = &mxcdVolume;
		if ((ret = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer), &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)) == MMSYSERR_NOERROR)
		{
			return ((mxcdVolume.dwValue - m_dwMinimum) * 1000) / (m_dwMaximum - m_dwMinimum);
		}
		else
		{
			string8 error;
			uFormatMediaMessage(ret, error);
			consol.warning(uStringPrintf("mixerGetControlDetails failed in GetVolume - %s", pconst(error)));
		}

		return 0;
	}

	/*return (play_control::get()->get_volume() / 10) + 1000;*/
	return g_db_to_vol(play_control::get()->get_volume());
}

void mixer_control::SetVolume(int volume)
{
	if (m_hMixer)
	{
		MMRESULT ret;
		MIXERCONTROLDETAILS_UNSIGNED mxcdVolume = { ((volume * (m_dwMaximum - m_dwMinimum)) / 1000) + m_dwMinimum };
		MIXERCONTROLDETAILS mxcd;
		mxcd.cbStruct = sizeof(mxcd);
		mxcd.dwControlID = m_dwVolumeControlID;
		mxcd.cChannels = 1;
		mxcd.cMultipleItems = 0;
		mxcd.cbDetails = sizeof(mxcdVolume);
		mxcd.paDetails = &mxcdVolume;
		if ((ret = mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer), &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE)) != MMSYSERR_NOERROR)
		{
			string8 error;
			uFormatMediaMessage(ret, error);
			consol.warning(uStringPrintf("mixerSetControlDetails failed in SetVolume - %s", pconst(error)));
		}
		return;
	}

	/*
	play_control::get()->set_volume((volume - 1000) * 10);
	*/
	play_control::get()->set_volume(g_vol_to_db(volume));
}

static critical_section g_mixer_sync;
static bool g_mixer_initialized = false;
static mixer_control g_mixer;

void g_change_mixer(unsigned mixer, unsigned control)
{
	insync(g_mixer_sync);
	if (!g_mixer.ChangeMixer(mixer, control))
	{
		cfg_which_mixer = 0;
	}
	g_update_controls(g_mixer.GetVolume());
}

int g_get_volume()
{
	insync(g_mixer_sync);
	if (!g_mixer_initialized)
	{
		g_mixer_initialized = true;
		g_change_mixer(cfg_which_mixer, cfg_mixer_control);
	}
	return g_mixer.GetVolume();
}

void g_set_volume(int volume)
{
	insync(g_mixer_sync);
	if (!g_mixer_initialized)
	{
		g_mixer_initialized = true;
		g_change_mixer(cfg_which_mixer, cfg_mixer_control);
	}
	g_mixer.SetVolume(volume);
}

static initquit_factory<console_dump> foo1;
