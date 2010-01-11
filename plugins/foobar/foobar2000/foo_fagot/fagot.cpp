#include <foobar2000.h>

#include "resource.h"

typedef struct st_owned
{
	unsigned frame;

	HBITMAP frames[6];

	HWND hwnd;
} owned_data;


static LRESULT CALLBACK YuoFagot(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HGLOBAL hglob = (HGLOBAL) GetPropA(wnd, "yuo_fagot");
	if (hglob == NULL) return DefWindowProc(wnd, msg, wp, lp);
	owned_data * yuo_fagot = (owned_data *) GlobalLock(hglob);
	switch (msg)
	{
	case WM_TIMER:
		if (wp == 666)
		{
			yuo_fagot->frame = (yuo_fagot->frame + 1) % 6;
			uSendMessage(yuo_fagot->hwnd, STM_SETIMAGE, IMAGE_BITMAP, (long) yuo_fagot->frames[yuo_fagot->frame]);
		}
		break;

	case WM_DESTROY:
		DestroyWindow(yuo_fagot->hwnd);
		for (int i = 0; i < 6; i++)
		{
			DeleteObject(yuo_fagot->frames[i]);
		}
		GlobalUnlock(hglob);
		GlobalFree(hglob);
		RemoveProp(wnd, "yuo_fagot");
		standard_commands::main_exit();
		return 0;
	}

	GlobalUnlock(hglob);

	return uDefWindowProc(wnd, msg, wp, lp);
}

static const char * images[6] = {MAKEINTRESOURCE(IDB_BITMAP1),
								 MAKEINTRESOURCE(IDB_BITMAP2),
								 MAKEINTRESOURCE(IDB_BITMAP3),
								 MAKEINTRESOURCE(IDB_BITMAP4),
								 MAKEINTRESOURCE(IDB_BITMAP5),
								 MAKEINTRESOURCE(IDB_BITMAP6)};

class fagotry : public initquit
{
	void on_init()
	{
		HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE,sizeof(owned_data));
		owned_data * yuo_fagot = (owned_data *) GlobalLock(hglob);

		yuo_fagot->frame = 5;

		HINSTANCE hinst = core_api::get_my_instance();

		for (int i = 0; i < 6; i++)
		{
			yuo_fagot->frames[i] = (HBITMAP) uLoadImage(hinst, images[i], IMAGE_BITMAP, 0, 0, 0);
		}

		HWND w = core_api::get_main_window();
		
		HWND die = GetWindow(w, GW_CHILD);

		while (die)
		{
			DestroyWindow(die);
			die = GetWindow(w, GW_CHILD);
		}

		RECT rc;

		SetWindowPos(w, HWND_TOPMOST, 0, 0, 450, 450, SWP_NOMOVE);
		GetClientRect(w, &rc);
		SetWindowPos(w, HWND_TOPMOST, 0, 0, 450 + (450 - rc.right), 450 + (450 - rc.bottom), SWP_NOMOVE);

		yuo_fagot->hwnd = uCreateWindowEx(0, "Static", 0, SS_BITMAP | WS_VISIBLE | WS_CHILD, 0, 0, 450, 450, w, 0, hinst, 0);

		GlobalUnlock(hglob);

		SetProp(w, "yuo_fagot", hglob);

		SetWindowLong(w, GWL_WNDPROC, (LONG) YuoFagot);
		SetTimer(w, 666, 50, 0);
	}

	void on_quit() {}
};

static initquit_factory<fagotry> yuo_equal_fagot;

DECLARE_COMPONENT_VERSION("YUO = FAGOT", "SHUTS UP!!1", "EAT A BAG OF HELL");
