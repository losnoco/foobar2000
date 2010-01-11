#ifndef _UI_EXTENSION_UTF8API_H_
#define _UI_EXTENSION_UTF8API_H_

/****************************
Some utf8 Win32 API wrappers not in utf8api, and other helpers
****************************/

#include "../../pfc/pfc.h"

#include <windows.h>
#include <WindowsX.h>
#include <commctrl.h>

typedef HDITEMA uHDITEM;
typedef TOOLINFOA uTOOLINFO;
typedef REBARBANDINFOA uREBARBANDINFO;

int uHeader_InsertItem(HWND wnd, int n, uHDITEM * hdi, bool insert = true); //set insert to false to set the item instead
BOOL uToolTip_AddTool(HWND wnd, uTOOLINFO * ti, bool update = false);
BOOL uRebar_InsertItem(HWND wnd, int n, uREBARBANDINFO * rbbi, bool insert = true); //set insert to false to set the item instead

BOOL uTabCtrl_InsertItemText(HWND wnd, int idx, const char * text, bool insert = true); //fixes '&' characters also, set insert to false to set the item instead

inline void GetRelativeRect(HWND wnd, HWND wnd_parent, RECT * rc)//get rect of wnd in wnd_parent coordinates
{
	GetWindowRect(wnd, rc);
	MapWindowPoints(HWND_DESKTOP, wnd_parent, (LPPOINT)rc,2);
}

BOOL uComboBox_GetText(HWND combo,UINT index,string_base & out);
BOOL uComboBox_SelectString(HWND combo,const char * src);
BOOL uStatus_SetText(HWND wnd,int part,const char * text);

HFONT uCreateIconFont(); 
HFONT uCreateMenuFont();

inline void GetMessagePos(LPPOINT pt)
{
	DWORD mp = GetMessagePos();
	pt->x =  GET_X_LPARAM(mp);
	pt->y =  GET_Y_LPARAM(mp);
}


BOOL uFormatMessage(DWORD dw_error, string_base & out, HANDLE hModule = NULL);

inline BOOL uGetLastErrorMessage(string_base & out)
{
	return uFormatMessage(GetLastError(), out);
}

DWORD uGetClassLong(HWND wnd, int index);
DWORD uSetClassLong(HWND wnd, int index, long new_long);

#if(WINVER >= 0x0500)
BOOL uGetMonitorInfo(HMONITOR monitor, LPMONITORINFO lpmi);
HWND uRecursiveChildWindowFromPoint(HWND parent, POINT pt_parent);//pt_parent is in parent window coordinates!
#endif

#endif