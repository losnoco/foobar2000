#define _WIN32_WINNT 0x0500
#include <foobar2000.h>

#include "hook.h"

/* volume.cpp */
LRESULT g_scroll_controls(WPARAM wp, LPARAM lp);

critical_section g_hook_sync;
ptr_list_t<window_hook_info> g_hook_info;


#undef GetAncestor
#define GetAncestor _GetAncestor

extern HWND (CALLBACK * GetAncestor)(HWND, UINT);

static HWND WINAPI
Emulate_GetAncestor(HWND hwnd, UINT gaFlags)
{
	switch (gaFlags)
	{
	case GA_PARENT:
		{
			HWND hwOwner = GetWindow(hwnd, GW_OWNER);
			HWND hwParent = GetParent(hwnd);
			if (hwParent != hwOwner) return hwParent;
			return 0;
		}
		break;

	case GA_ROOT:
		{
			HWND hwDesktop = GetDesktopWindow();
			HWND hwParent = Emulate_GetAncestor(hwnd, GA_PARENT);
			if (!hwParent || hwParent == hwDesktop) return 0;
			for (;;)
			{
				HWND hwTemp = hwParent;
				hwParent = Emulate_GetAncestor(hwParent, GA_PARENT);
				if (!hwParent || hwParent == hwDesktop) return hwTemp;
			}
		}
		break;

	case GA_ROOTOWNER:
		{
			HWND hwDesktop = GetDesktopWindow();
			HWND hwParent = GetParent(hwnd);
			if (!hwParent || hwParent == hwDesktop) return 0;
			for (;;)
			{
				HWND hwTemp = hwParent;
				hwParent = GetParent(hwParent);
				if (!hwParent || hwParent == hwDesktop) return hwTemp;
			}
		}
		break;
	}
	
	return 0;
}

static HWND WINAPI
Probe_GetAncestor(HWND hwnd, UINT gaFlags)
{
    HINSTANCE hinst;
    FARPROC fp;
    HWND (CALLBACK *RealGetAncestor)(HWND, UINT);

	hinst = uGetModuleHandle("USER32");
	fp = GetProcAddress(hinst, "GetAncestor");

	if (fp)
	{
		*(FARPROC *)&RealGetAncestor = fp;
		GetAncestor = RealGetAncestor;
	}
	else
	{
		GetAncestor = Emulate_GetAncestor;
	}

	return GetAncestor(hwnd, gaFlags);
}

HWND (CALLBACK * GetAncestor)(HWND, UINT) = Probe_GetAncestor;


static window_hook_info g_get_hook_info(DWORD dwThreadId)
{
	insync(g_hook_sync);
	for (unsigned i = 0, j = g_hook_info.get_count(); i < j; i++)
	{
		window_hook_info * whi = g_hook_info[i];
		if (whi->dwThreadId == dwThreadId)
		{
			return *whi;
		}
	}
	window_hook_info whi = {0, 0, 0};
	return whi;
}

LRESULT CALLBACK window_hook::GetMsgProc(int code, WPARAM wp, LPARAM lp)
{
	window_hook_info whi = g_get_hook_info(GetCurrentThreadId());
	if (code == HC_ACTION && lp)
	{
		MSG * msg = (MSG *)lp;
		if (msg->message == WM_MOUSEWHEEL)
		{
			if (msg->hwnd == whi.hWnd || IsChild(whi.hWnd, msg->hwnd))
			{
				if (!g_scroll_controls(msg->wParam, msg->lParam))
				{
					msg->message = WM_NULL;
				}
			}
		}
	}
	return CallNextHookEx(whi.hGetMsg, code, wp, lp);
}

void window_hook::Hook(HWND hWnd)
{
	insync(g_hook_sync);

	whi.hWnd = GetAncestor(hWnd, GA_ROOT);
	whi.dwThreadId = GetWindowThreadProcessId(whi.hWnd, NULL);

	window_hook_info lwhi = g_get_hook_info(whi.dwThreadId);
	if (lwhi.dwThreadId == whi.dwThreadId) return;

	g_hook_info.add_item(&whi);
	added = true;

	if (IsUnicode())
	{
		whi.hGetMsg = SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, NULL, whi.dwThreadId);
	}
	else
	{
		whi.hGetMsg = SetWindowsHookExA(WH_GETMESSAGE, GetMsgProc, NULL, whi.dwThreadId);
	}
}

window_hook::window_hook()
{
	added = false;
	whi.hGetMsg = NULL;
}

window_hook::~window_hook()
{
	if (whi.hGetMsg) UnhookWindowsHookEx(whi.hGetMsg);
	if (added)
	{
		insync(g_hook_sync);
		g_hook_info.remove_item(&whi);
	}
}
