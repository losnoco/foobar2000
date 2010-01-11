#include "../ui_extension/ui_extension.h"
#include "../ui_extension/utf8api.h"

// for pincrement/pdecrement, which are fastcall; InterlockedIncrement etc are stdcall
#include <ptypes.h>
USING_PTYPES

static const char class_name[] = "F14E14BB-32D3-4d44-9D0F-CDEAC87F371E";

class volume_control_v : public ui_extension
{
	HWND wnd_focuslost, wnd_host, wnd_control;

	ui_extension_host * host;

public:
	volume_control_v();
	~volume_control_v();

	const GUID & get_extension_guid() const;
	void get_name(string_base & out) const;
	void get_category(string_base & out) const;
	bool get_short_name(string_base & out) const;
	unsigned get_type() const;

	bool is_available(ui_extension_host * p_host) const;
	HWND init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, cfg_var::read_config_helper * config);
	void destroy_window();
	HWND get_wnd() const;

	static LRESULT WINAPI host_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	LRESULT WINAPI on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
};

class register_window_class_v
{
	int ref_count;
	ATOM class_atom;

public:
	register_window_class_v() : ref_count(0), class_atom(0) {}
	~register_window_class_v()
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
		wc.lpfnWndProc   = (WNDPROC)volume_control_v::host_proc;
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

static register_window_class_v rwc;

volume_control_v::volume_control_v()
: wnd_host(0), wnd_control(0), host(0) {}

volume_control_v::~volume_control_v()
{
	if (wnd_host) destroy_window();
}

const GUID & volume_control_v::get_extension_guid() const
{
	// {CBD4748E-6920-4217-BE90-193EAB0CCC9D}
	static const GUID guid = 
	{ 0xcbd4748e, 0x6920, 0x4217, { 0xbe, 0x90, 0x19, 0x3e, 0xab, 0xc, 0xcc, 0x9d } };
	return guid;
}

void volume_control_v::get_name(string_base & out) const
{
	out = "Volume control (vertical)";
}

void volume_control_v::get_category(string_base & out) const
{
	out = "Panels";
}

bool volume_control_v::get_short_name(string_base & out) const
{
	out = "Volume";
	return true;
}

unsigned volume_control_v::get_type() const
{
	return ui_extension_flag::TYPE_PANEL;
}

bool volume_control_v::is_available(ui_extension_host * p_host) const
{
	return true;
}

HWND volume_control_v::init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, cfg_var::read_config_helper * config)
{
	if (wnd_host)
	{
		SetParent(wnd_host, wnd_parent);
		host->relinquish_ownership(wnd_host);
		host = p_host;
	}
	else
	{
		host = p_host;

		ATOM wnd_class = rwc.Register();
		if (!wnd_class)
		{
			string8 error;
			uFormatMessage(GetLastError(), error);
			console::error(uStringPrintf("Failed to register window class - %s", pconst(error)));
			return 0;
		}

		wnd_host = uCreateWindowEx(0, (const char *)wnd_class, "Volume", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0, wnd_parent, 0, core_api::get_my_instance(), this);

		if (!wnd_host)
		{
			string8 error;
			uFormatMessage(GetLastError(), error);
			console::error(uStringPrintf("Failed to create window - %s", pconst(error)));
			rwc.Unregister();
		}
	}

	return wnd_host;
}

void volume_control_v::destroy_window()
{
	DestroyWindow(HWND(pexchange((int*)&wnd_host, 0)));
	rwc.Unregister();
}

HWND volume_control_v::get_wnd() const
{
	return wnd_host;
}

LRESULT WINAPI volume_control_v::host_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	volume_control_v * p_this;

	if(msg == WM_NCCREATE)
	{
		p_this = (volume_control_v *)((CREATESTRUCT *)(lp))->lpCreateParams;
		uSetWindowLong(wnd, GWL_USERDATA, (LPARAM)p_this);
	}
	else
		p_this = reinterpret_cast<volume_control_v*>(uGetWindowLong(wnd,GWL_USERDATA));

	return p_this ? p_this->on_message(wnd, msg, wp, lp) : uDefWindowProc(wnd, msg, wp, lp);
}

extern critical_section g_panel_sync;
extern mem_block_list<HWND> g_panels;

int g_get_volume();
void g_set_volume(int vol);

LRESULT WINAPI volume_control_v::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		wnd_host = wnd;

		wnd_control = uCreateWindowEx(0, "msctls_trackbar32", 0, TBS_BOTH | TBS_NOTICKS | TBS_VERT | WS_CHILD | WS_VISIBLE, 0, 0, 20, 100, wnd, 0, core_api::get_my_instance(), 0);

		if (wnd_control)
		{
			uSendMessage(wnd_control, TBM_SETRANGE, 0, MAKELONG(0, 1000));
			uSendMessage(wnd_control, TBM_SETPOS, 1, 1000 - g_get_volume());
			insync(g_panel_sync);
			g_panels.add_item(wnd_host);
		}
		break;

	case WM_SIZE:
		if (wnd_control) SetWindowPos(wnd_control, 0, 0, 0, LOWORD(lp), HIWORD(lp), SWP_NOZORDER);
		break;

	case WM_SETFOCUS:
		wnd_focuslost = (HWND)wp;
		break;

	case WM_KEYDOWN:
		if (wp == VK_ESCAPE)
		{
			if (GetCapture() == wnd_control) ReleaseCapture();
			SetFocus(IsWindow(wnd_focuslost) ? wnd_focuslost : core_api::get_main_window());
			return 0;
		}
		break;

	case WM_VSCROLL:
		if ((HWND)lp == wnd_control)
		{
			if (LOWORD(wp) == SB_THUMBTRACK ||
				LOWORD(wp) == SB_THUMBPOSITION) g_set_volume(1000 - HIWORD(wp));
			return 0;
		}
		break;

	case WM_DESTROY:
		if (wnd_control)
		{
			{
				insync(g_panel_sync);
				g_panels.remove_item(wnd_host);
			}
			DestroyWindow(wnd_control);
			wnd_control = 0;
		}
		break;

	case TBM_SETPOS:
		if (wnd_control)
		{
			return uSendMessage(wnd_control, msg, wp, 1000 - lp);
		}
	}

	return uDefWindowProc(wnd, msg, wp, lp);
}

static ui_extension_factory<volume_control_v> foo1;
