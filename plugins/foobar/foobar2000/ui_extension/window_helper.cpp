#include "window_helper.h"

bool container_window::class_release()
{
	get_refcount()--;
	if (!get_refcount() && get_class_registered())
	{
		get_class_registered() = !uUnregisterClass(get_class_name(), core_api::get_my_instance());
	}
	return get_class_registered() == false;
}

container_window::container_window() : wnd_host(0){};

HWND container_window::create(HWND wnd_parent, unsigned styles, unsigned ex_styles)
{
	
	ensure_class_registered();
	wnd_host = uCreateWindowEx(ex_styles|WS_EX_CONTROLPARENT, get_class_name(), get_class_name(),
		styles|WS_CHILD|WS_CLIPCHILDREN, 0, 0, 0, 0,
		wnd_parent, 0, core_api::get_my_instance(), this);
	
	return wnd_host;
}

bool container_window::ensure_class_registered()
{
	get_refcount()++;
	if (!get_class_registered())
	{
		uWNDCLASS  wc;
		memset(&wc,0,sizeof(uWNDCLASS));
		//		wc.style          = CS_DBLCLKS; //causes issue where double clicking resets cursor icon
		wc.lpfnWndProc    = (WNDPROC)window_proc;
		wc.hInstance      = core_api::get_my_instance();
		wc.hCursor        = uLoadCursor(NULL, uMAKEINTRESOURCE(IDC_ARROW));;
		wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
		wc.lpszClassName  = get_class_name();
		
		get_class_registered() = (uRegisterClass(&wc) != 0);
	}
	return get_class_registered();
}

void container_window::destroy() //if destroying someother way, you should make sure you call class_release()  properly
{
	DestroyWindow(wnd_host);
	class_release();
}

LRESULT WINAPI container_window::window_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	container_window * p_this;
	
	if(msg == WM_NCCREATE)
	{
		
		p_this = reinterpret_cast<container_window *>(((CREATESTRUCT *)(lp))->lpCreateParams); //retrieve pointer to class
		uSetWindowLong(wnd, GWL_USERDATA, (LPARAM)p_this);//store it for future use
		if (p_this) p_this->wnd_host = wnd;
		
	}
	else
		p_this = reinterpret_cast<container_window*>(uGetWindowLong(wnd,GWL_USERDATA));//if isnt wm_create, retrieve pointer to class
	
	if (p_this && p_this->get_want_transparent_background())
	{
		if (msg == WM_ERASEBKGND)
		{
			HDC dc = (HDC)wp;
			
			if (dc)
			{
				HWND wnd_parent = GetParent(wnd);
				POINT pt = {0, 0};
				MapWindowPoints(wnd, wnd_parent, &pt, 1);
				OffsetWindowOrgEx(dc, pt.x, pt.y, 0);
				uSendMessage(wnd_parent, WM_ERASEBKGND,wp, 0);
				SetWindowOrgEx(dc, pt.x, pt.y, 0);
			}
			return TRUE;
		}
		else if (msg==WM_WINDOWPOSCHANGING && p_this)
		{
			HWND meh_lazy = GetWindow(wnd, GW_CHILD);
			RedrawWindow(meh_lazy, 0, 0, RDW_ERASE);
		}
	}
	
	return p_this ? p_this->on_message(wnd, msg, wp, lp) : uDefWindowProc(wnd, msg, wp, lp);
}

HWND container_window::get_wnd()const{return wnd_host;}

