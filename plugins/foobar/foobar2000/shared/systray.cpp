#include "shared.h"
#include <shellapi.h>

using namespace pfc;

#define NIIF_NOSOUND    0x00000010

void SHARED_EXPORT uFixAmpersandChars(const char * src,string_base & out)
{
	out.reset();
	while(*src)
	{
		if (*src=='&')
		{
			out.add_string("&&&");
			src++;
			while(*src=='&')
			{
				out.add_string("&&");
				src++;
			}
		}
		else out.add_byte(*(src++));
	}
}

void SHARED_EXPORT uFixAmpersandChars_v2(const char * src,string_base & out)
{
	out.reset();
	while(*src)
	{
		if (*src=='&')
		{
			out.add_string("&&");
			src++;
		}
		else out.add_byte(*(src++));
	}
}

static BOOL run_action(DWORD action,NOTIFYICONDATA * data)
{
	if (Shell_NotifyIcon(action,data)) return TRUE;
	if (action==NIM_MODIFY)
	{
		if (Shell_NotifyIcon(NIM_ADD,data)) return TRUE;
	}
	return FALSE;
}

extern "C"
{

	BOOL SHARED_EXPORT uShellNotifyIcon(DWORD action,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip)
	{
		NOTIFYICONDATA nid;
		memset(&nid,0,sizeof(nid));
		nid.cbSize = sizeof(nid);
		nid.hWnd = wnd;
		nid.uID = id;
		nid.uFlags = 0;
		if (callbackmsg)
		{
			nid.uFlags |= NIF_MESSAGE;
			nid.uCallbackMessage = callbackmsg;
		}
		if (icon)
		{
			nid.uFlags |= NIF_ICON;
			nid.hIcon = icon;
		}			
		if (tip)
		{
			nid.uFlags |= NIF_TIP;
			_tcsncpy(nid.szTip,pfc::stringcvt::string_os_from_utf8(tip),tabsize(nid.szTip)-1);
		}

		return run_action(action,&nid);
	}

	BOOL SHARED_EXPORT uShellNotifyIconEx(DWORD action,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip,const char * balloon_title,const char * balloon_msg)
	{

		NOTIFYICONDATA nid;
		memset(&nid,0,sizeof(nid));
		nid.cbSize = sizeof(nid);
		nid.hWnd = wnd;
		nid.uID = id;
		if (callbackmsg)
		{
			nid.uFlags |= NIF_MESSAGE;
			nid.uCallbackMessage = callbackmsg;
		}
		if (icon)
		{
			nid.uFlags |= NIF_ICON;
			nid.hIcon = icon;
		}			
		if (tip)
		{
			nid.uFlags |= NIF_TIP;
			_tcsncpy(nid.szTip,pfc::stringcvt::string_os_from_utf8(tip),tabsize(nid.szTip)-1);
		}

		nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
		//if (balloon_title || balloon_msg)
		{
			nid.uFlags |= NIF_INFO;
			if (balloon_title) _tcsncpy(nid.szInfoTitle,pfc::stringcvt::string_os_from_utf8(balloon_title),tabsize(nid.szInfoTitle)-1);
			if (balloon_msg) _tcsncpy(nid.szInfo,pfc::stringcvt::string_os_from_utf8(balloon_msg),tabsize(nid.szInfo)-1);	
		}
		return run_action(action,reinterpret_cast<NOTIFYICONDATA*>(&nid));


	}

}//extern "C"