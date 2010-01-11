#include "ui_extension.h"

#if(WINVER >= 0x0500)
#define _GetParent(wnd) \
	GetAncestor(wnd, GA_PARENT)
#else
#define _GetParent(wnd) \
	GetParent(wnd)
#endif

// {F72649F6-92EA-434a-929F-44E39866F6C7}
const GUID ui_extension_host::class_guid = 
{ 0xf72649f6, 0x92ea, 0x434a, { 0x92, 0x9f, 0x44, 0xe3, 0x98, 0x66, 0xf6, 0xc7 } };

// {0BAF8681-348B-44c2-BE3E-AF5125A14EF3}
const GUID menu_ui_extension::class_guid = 
{ 0xbaf8681, 0x348b, 0x44c2, { 0xbe, 0x3e, 0xaf, 0x51, 0x25, 0xa1, 0x4e, 0xf3 } };

// {DF6396C3-7A28-470b-AD84-A15ABC1D0007}
const GUID ui_extension::class_guid =
{ 0xdf6396c3, 0x7a28, 0x470b, { 0xad, 0x84, 0xa1, 0x5a, 0xbc, 0x1d, 0x0, 0x7 } };

HWND ui_extension::g_on_tab(HWND wnd_focus)
{
	HWND rv = 0;
	
	HWND wnd_temp = _GetParent(wnd_focus);
	
	while (wnd_temp && GetWindowLong(wnd_temp, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)
	{
		if (GetWindowLong(wnd_temp, GWL_STYLE) & WS_POPUP) break;
		else wnd_temp = _GetParent(wnd_temp);
	}
	
	if (wnd_temp)
	{
		HWND wnd_next = GetNextDlgTabItem(wnd_temp, wnd_focus, (GetKeyState(VK_SHIFT) & KF_UP) ? TRUE :  FALSE);
		if (wnd_next && wnd_next != wnd_focus) 
		{
			unsigned flags = uSendMessage(wnd_next, WM_GETDLGCODE, 0, 0);
			if (flags & DLGC_HASSETSEL) uSendMessage(wnd_next, EM_SETSEL, 0, -1);
			SetFocus(wnd_next);
			
			rv = wnd_next;
		}
	}
	return rv;
};

void ui_extension_info_list_simple::get_name_by_guid (const GUID & in, string_base & out)
{
	unsigned n, count = ptr_list_t<ui_extension_info_simple>::get_count();
	for (n=0; n<count; n++)
	{
		if ((*this)[n]->guid == in)
		{
			out = (*this)[n]->name;
			return;
		}
	}
}

int ui_extension_info_list_simple::sortproc::compare(const ui_extension_info_simple * &n1,const ui_extension_info_simple * &n2)
{
	int rv = 0;
	rv =  uStringCompare(n1->category,n2->category);
	if (!rv) rv = uStringCompare(n1->name,n2->name);
	return rv;
}
