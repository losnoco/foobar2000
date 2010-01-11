#define _WIN32_WINNT 0x0500

#include <foobar2000.h>
#include <commctrl.h>
#include <windowsx.h>
#include <limits.h>

#include "../ui_extension/ui_extension.h"
#include "../ui_extension/utf8api.h"

//#include "hook.h"

// for pincrement/pdecrement, which are fastcall; InterlockedIncrement etc are stdcall
#include <ptypes.h>
USING_PTYPES

/* config.cpp */
extern cfg_int cfg_which_mixer;

/* mixer.cpp */
int g_vol_to_db(int vol);

static const char class_name[] = "C4553E95-BFE3-4016-A475-ACF3ED951813";

#define TRACKING_BS

class volume_control : public ui_extension
{
#ifdef TRACKING_BS
	UINT last_message, last_wp, last_lp;
#endif
	HWND wnd_focuslost, wnd_host, wnd_control;
#ifdef TRACKING_BS
	HWND wnd_tooltip;
#endif
	WNDPROC trackbarproc;
	bool dragging;

	ui_extension_host * host;

	//window_hook * whook;

public:
	volume_control();
	~volume_control();

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

	static LRESULT WINAPI main_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	LRESULT WINAPI hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

#ifdef TRACKING_BS
	void CloseTooltip();
	void OpenTooltip(const char * text, unsigned x, unsigned y);
	void UpdateTooltip(const char * text);
#endif
};

class register_window_class
{
	int ref_count;
	ATOM class_atom;

public:
	register_window_class() : ref_count(0), class_atom(0) {}
	~register_window_class()
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
		wc.lpfnWndProc   = (WNDPROC)volume_control::host_proc;
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

static register_window_class rwc;

volume_control::volume_control()
:
#ifdef TRACKING_BS
last_message(0), last_wp(0), last_lp(0),
#endif
wnd_host(0), wnd_control(0), trackbarproc(0), dragging(false), host(0)/*, whook(0)*/ {}

volume_control::~volume_control()
{
	//if (whook) delete whook;
	if (wnd_host) destroy_window();
}

const GUID & volume_control::get_extension_guid() const
{
	// {CEB9D7DF-1F61-443f-AC72-65CD7063082A}
	static const GUID guid = 
	{ 0xceb9d7df, 0x1f61, 0x443f, { 0xac, 0x72, 0x65, 0xcd, 0x70, 0x63, 0x8, 0x2a } };
	return guid;
}

void volume_control::get_name(string_base & out) const
{
	out = "Volume control";
}

void volume_control::get_category(string_base & out) const
{
	out = "Toolbars";
}

bool volume_control::get_short_name(string_base & out) const
{
	out = "Volume";
	return true;
}

unsigned volume_control::get_type() const
{
	return ui_extension_flag::TYPE_TOOLBAR;
}

bool volume_control::is_available(ui_extension_host * p_host) const
{
	return true;
}

HWND volume_control::init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, cfg_var::read_config_helper * config)
{
	if (wnd_host)
	{
		//if (whook) delete whook;
		SetParent(wnd_host, wnd_parent);
		host->relinquish_ownership(wnd_host);
		host = p_host;
		/*whook = new window_hook;
		whook->Hook(wnd_control);*/
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
		/*else
		{
			whook = new window_hook;
			whook->Hook(wnd_control);
		}*/
	}

	return wnd_host;
}

void volume_control::destroy_window()
{
	/*window_hook * hook = tpexchange<window_hook>(&whook, 0);
	if (hook) delete hook;*/
	DestroyWindow(HWND(pexchange((int*)&wnd_host, 0)));
	rwc.Unregister();
}

HWND volume_control::get_wnd() const
{
	return wnd_host;
}

LRESULT WINAPI volume_control::host_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	volume_control * p_this;
	
	if(msg == WM_NCCREATE)
	{
		p_this = (volume_control *)((CREATESTRUCT *)(lp))->lpCreateParams;
		uSetWindowLong(wnd, GWL_USERDATA, (LPARAM)p_this);
	}
	else
		p_this = reinterpret_cast<volume_control*>(uGetWindowLong(wnd,GWL_USERDATA));
	
	return p_this ? p_this->on_message(wnd, msg, wp, lp) : uDefWindowProc(wnd, msg, wp, lp);
}

critical_section g_panel_sync;
mem_block_list<HWND> g_panels;

int g_get_volume();
void g_set_volume(int vol);

LRESULT WINAPI volume_control::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		wnd_host = wnd;

		wnd_control = uCreateWindowEx(0, "msctls_trackbar32", 0, TBS_BOTH | TBS_NOTICKS | WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, wnd, 0, core_api::get_my_instance(), 0);

		if (wnd_control)
		{
			uSendMessage(wnd_control, TBM_SETRANGE, 0, MAKELONG(0, 1000));
			uSendMessage(wnd_control, TBM_SETPOS, 1, g_get_volume());
			insync(g_panel_sync);
			g_panels.add_item(wnd_control);

			uSetWindowLong(wnd_control,GWL_USERDATA,(LPARAM)(this));
			trackbarproc = (WNDPROC)uSetWindowLong(wnd_control,GWL_WNDPROC,(LPARAM)(main_hook));
		}
		break;

	case WM_SIZE:
		if (wnd_control) SetWindowPos(wnd_control, 0, 0, 0, LOWORD(lp), HIWORD(lp), SWP_NOZORDER);
		break;

	case WM_SETFOCUS:
		wnd_focuslost = (HWND)wp;
		break;

	case WM_GETMINMAXINFO:
		if (wnd_control) reinterpret_cast<MINMAXINFO *>(lp)->ptMinTrackSize.y = 20;
		break;

	case WM_KEYDOWN:
		if (wp == VK_ESCAPE)
		{
#ifdef TRACKING_BS
			CloseTooltip();
#endif
			if (GetCapture() == wnd_control) ReleaseCapture();
			SetFocus(IsWindow(wnd_focuslost) ? wnd_focuslost : core_api::get_main_window());
			return 0;
		}
		break;

	case WM_HSCROLL:
		if ((HWND)lp == wnd_control)
		{
			if (LOWORD(wp) == SB_THUMBTRACK ||
				LOWORD(wp) == SB_THUMBPOSITION) g_set_volume(HIWORD(wp));
			return 0;
		}
		break;

	case WM_DESTROY:
		if (wnd_control)
		{
			{
				insync(g_panel_sync);
				g_panels.remove_item(wnd_control);
			}
			DestroyWindow(wnd_control);
			wnd_control = 0;
		}
		break;
	}

	return uDefWindowProc(wnd, msg, wp, lp);
}

LRESULT WINAPI volume_control::main_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	volume_control * p_this;
	LRESULT rv;
	
	p_this = reinterpret_cast<volume_control*>(uGetWindowLong(wnd,GWL_USERDATA));
	
	rv = p_this ? p_this->hook(wnd,msg,wp,lp) : uDefWindowProc(wnd, msg, wp, lp);

	return rv;
}

LRESULT WINAPI volume_control::hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg)
	{

#ifdef TRACKING_BS

	case WM_MOUSEMOVE:
		if (dragging)
		{
			if (last_message == WM_MOUSEMOVE)
			{
				if (((UINT)wp == last_wp) && ((UINT)lp == last_lp)) break;
			}
			last_wp = (UINT)wp;
			last_lp = (UINT)lp;
			last_message = WM_MOUSEMOVE;

			if (IsWindowEnabled(wnd))
			{
				int temp, temp_x = GET_X_LPARAM(lp);
				POINT pt;
				RECT rc_client, rc_channel, rc_thumb;
				GetClientRect(wnd, &rc_client);
				uSendMessage(wnd, TBM_GETCHANNELRECT, 0, (LPARAM)&rc_channel);
				uSendMessage(wnd, TBM_GETTHUMBRECT, 0, (LPARAM)&rc_thumb);
				rc_channel.top = rc_thumb.bottom;
				rc_channel.bottom = rc_thumb.top;
				temp = (rc_thumb.right - rc_thumb.left) / 2;
				rc_channel.left += temp;
				rc_channel.right -= temp;
				pt.x = GET_X_LPARAM(lp);
				pt.y = GET_Y_LPARAM(lp);
				ClientToScreen(wnd, &pt);
				if (temp_x > rc_channel.right) temp_x = rc_channel.right;
				else if (temp_x < rc_channel.left) temp_x = rc_channel.left;
						
				uSendMessage(wnd_tooltip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y + 21));
				temp = MulDiv(temp_x - rc_channel.left, uSendMessage(wnd, TBM_GETRANGEMAX, 0, 0), rc_channel.right - rc_channel.left);
				uSendMessage(wnd, TBM_SETPOS, 1, temp);

				//uSendMessage(wnd_host, WM_HSCROLL, MAKELONG(SB_THUMBTRACK, temp), (LPARAM)wnd);
				g_set_volume(temp);

				if (cfg_which_mixer) UpdateTooltip(uStringPrintf("%d.%d%%", temp / 10, temp % 10));
				else
				{
					temp = g_vol_to_db(temp);
					UpdateTooltip(uStringPrintf("%d.%02d dB", temp / 100, -(temp % 100)));
				}
			}
			return 0;
		}
		break;
#endif

	case WM_LBUTTONDOWN:
#ifdef TRACKING_BS
		last_message = WM_LBUTTONDOWN;
#endif
		if (IsWindowEnabled(wnd))
		{
			POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
			RECT rc_client;
			GetClientRect(wnd, &rc_client);
			if (PtInRect(&rc_client, pt))
			{
#ifdef TRACKING_BS
				RECT rc_channel, rc_thumb;
				uSendMessage(wnd, TBM_GETCHANNELRECT, 0, (LPARAM)&rc_channel);
				uSendMessage(wnd, TBM_GETTHUMBRECT, 0, (LPARAM)&rc_thumb);
#endif
				dragging = true;
#ifdef TRACKING_BS
				SetCapture(wnd);
				SetFocus(wnd_host);

				int temp = MulDiv(pt.x - rc_channel.left, uSendMessage(wnd, TBM_GETRANGEMAX, 0, 0) + 1, rc_channel.right - rc_channel.left - 1);
				uSendMessage(wnd, TBM_SETPOS, 1, temp);

				ClientToScreen(wnd, &pt);

				if (cfg_which_mixer) OpenTooltip(uStringPrintf("%d.%d%%", temp / 10, temp % 10), pt.x, pt.y);
				else
				{
					temp = g_vol_to_db(temp);
					OpenTooltip(uStringPrintf("%d.%02d dB", temp / 100, -(temp % 100)), pt.x, pt.y);
				}
#endif
			}
		}
#ifdef TRACKING_BS
		return 0;
#endif
		break;

	case WM_LBUTTONUP:
		if (dragging)
		{
#ifndef TRACKING_BS
			uCallWindowProc(trackbarproc, wnd, msg, wp, lp);
#endif
			uSendMessage(wnd_host, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, uSendMessage(wnd, TBM_GETPOS, 0, 0)), (LPARAM)wnd);
#ifdef TRACKING_BS
			CloseTooltip();
#endif

			if (GetCapture() == wnd) ReleaseCapture();
			dragging = false;
#ifdef TRACKING_BS
			last_message = WM_LBUTTONUP;
#endif
			SetFocus(IsWindow(wnd_focuslost) ? wnd_focuslost : core_api::get_main_window());
		}
		return 0;
		break;

	case WM_RBUTTONUP:
		if (dragging)
		{
#ifdef TRACKING_BS
			CloseTooltip();
#endif
			if (GetCapture() == wnd) ReleaseCapture();
			SetFocus(IsWindow(wnd_focuslost) ? wnd_focuslost : core_api::get_main_window());
			dragging = false;
		}
		break;

	case WM_MBUTTONDOWN:
		return 0;
		break;

	case WM_MOUSEWHEEL:
		if (!(GET_KEYSTATE_WPARAM(wp) & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		{
			POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
			RECT rc_client;
			GetClientRect(wnd, &rc_client);
			ScreenToClient(wnd, &pt);
			if (PtInRect(&rc_client, pt))
			{
				int delta = GET_WHEEL_DELTA_WPARAM(wp);
				UINT lines = 3;
				SystemParametersInfo(SPI_GETWHEELSCROLLLINES, NULL, &lines, 0);
				if (lines == WHEEL_PAGESCROLL || lines > INT_MAX) lines = 20;
				delta = MulDiv(delta, lines * 10, WHEEL_DELTA);
				int pos = uSendMessage(wnd, TBM_GETPOS, 0, 0);
				int max = uSendMessage(wnd, TBM_GETRANGEMAX, 0, 0);
				int newpos = pos + delta;
				if (newpos < 0) newpos = 0;
				else if (newpos > max) newpos = max;
				if (newpos != pos)
				{
					uSendMessage(wnd, TBM_SETPOS, 1, newpos);
					uSendMessage(wnd_host, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, newpos), (LPARAM)wnd);
				}
				return 0;
			}
		}
		break;
	}
	return uCallWindowProc(trackbarproc,wnd,msg,wp,lp);
}

#ifdef TRACKING_BS
class param_utf16_from_utf8 : public string_utf16_from_utf8
{
	bool is_null;
	WORD low_word;
public:
	param_utf16_from_utf8(const char * p) : 
		is_null(p==0), 
		low_word( HIWORD((DWORD)p)==0 ? LOWORD((DWORD)p) : 0),
		string_utf16_from_utf8( p && HIWORD((DWORD)p)!=0 ?p:"") 
		{}
	inline operator const WCHAR *()
	{
		return get_ptr();
	}
	const WCHAR * get_ptr()
	{
		return low_word ? (const WCHAR*)(DWORD)low_word : is_null ? 0 : string_utf16_from_utf8::get_ptr();
	}
	
};

class param_ansi_from_utf8 : public string_ansi_from_utf8
{
	bool is_null;
	WORD low_word;
public:
	param_ansi_from_utf8(const char * p) : 
		is_null(p==0), 
		low_word( HIWORD((DWORD)p)==0 ? LOWORD((DWORD)p) : 0),
		string_ansi_from_utf8( p && HIWORD((DWORD)p)!=0 ?p:"") 
		{}
	inline operator const char *()
	{
		return get_ptr();
	}
	const char * get_ptr()
	{
		return low_word ? (const char*)(DWORD)low_word : is_null ? 0 : string_ansi_from_utf8::get_ptr();
	}
	
};

void volume_control::CloseTooltip()
{
	if (wnd_tooltip)
	{
		DestroyWindow(wnd_tooltip);
		wnd_tooltip = 0;
	}
}

void volume_control::OpenTooltip(const char * text, unsigned x, unsigned y)
{
	CloseTooltip();
	wnd_tooltip = uCreateWindowEx(WS_EX_TOPMOST, "tooltips_class32", 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, wnd_host, 0, core_api::get_my_instance(), 0);
	SetWindowPos(wnd_tooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	uTOOLINFO ti;
	memset(&ti, 0, sizeof(ti));
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_TRANSPARENT | TTF_ABSOLUTE | TTF_TRACK;
	ti.hwnd = wnd_host;
	ti.hinst = core_api::get_my_instance();
	ti.lpszText = (LPSTR)text;
	uToolTip_AddTool(wnd_tooltip, &ti, false);

	uSendMessage(wnd_tooltip, TTM_TRACKPOSITION, 0, MAKELONG(x, y + 21));
	uSendMessage(wnd_tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
}

void volume_control::UpdateTooltip(const char * text)
{
	if (wnd_tooltip)
	{
		uTOOLINFO ti;
		memset(&ti, 0, sizeof(ti));
		ti.cbSize = sizeof(ti);
		ti.uFlags = TTF_TRANSPARENT | TTF_ABSOLUTE | TTF_TRACK | TTF_SUBCLASS;
		ti.hwnd = wnd_host;
		ti.hinst = core_api::get_my_instance();
		ti.lpszText = (LPSTR)text;

		uToolTip_AddTool(wnd_tooltip, &ti, true);
	}
}
#endif

void g_update_controls(int volume)
{
	insync(g_panel_sync);
	for (unsigned i = 0, j = g_panels.get_count(); i < j; i++)
	{
		uSendMessage(g_panels[i], TBM_SETPOS, 1, volume);
	}
}

LRESULT g_scroll_controls(WPARAM wp, LPARAM lp)
{
	insync(g_panel_sync);
	for (unsigned i = 0, j = g_panels.get_count(); i < j; i++)
	{
		if (!uSendMessage(g_panels[i], WM_MOUSEWHEEL, wp, lp)) return 0;
	}
	return 1;
}

static ui_extension_factory<volume_control> foo1;
