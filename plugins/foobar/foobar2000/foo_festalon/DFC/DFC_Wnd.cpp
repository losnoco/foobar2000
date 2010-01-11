/*
 *
 *	Copyright (C) 2003  Disch

 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.

 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.

 *	You should have received a copy of the GNU Lesser General Public
 *	License along with this library; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

//////////////////////////////////////////////////////////////////////////
//
//  DFC_Wnd.cpp
//
//


#include "DFC.h"


//bool				bCommonControlsInitialized = 0;

CDWnd::CDWnd()
{
	/*if(!bCommonControlsInitialized)
	{
		InitCommonControls();
		bCommonControlsInitialized = 1;
	}*/

	hWnd =			NULL;
	hInstance =		NULL;
	hParentWnd =	NULL;
	m_MessageMap =	NULL;
	m_NotifyMap =	NULL;
}

CDWnd::~CDWnd()
{
	DDlgMsgItem*		next;
	while(m_MessageMap)
	{
		next = m_MessageMap->next;
		delete m_MessageMap;
		m_MessageMap = next;
	}
	while(m_NotifyMap)
	{
		next = m_NotifyMap->next;
		delete m_NotifyMap;
		m_NotifyMap = next;
	}
}

//////////////////////////////////////////////////////////////////////////
// The list!
class CDWnd_List
{
public:
	CDWnd_List() { wnd = NULL; next = NULL; }

	CDWnd*			wnd;
	CDWnd_List*		next;
};

CDWnd_List*				pGlobalWindowList;


INT_PTR CALLBACK CDWnd::GenericWindowProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	register CDWnd*			object;
	register CDWnd_List*	ptr;
	register CDWnd_List*	prev;
	DDlgMsgItem*			msgmap;

	if(msg == WM_CREATE || msg == WM_INITDIALOG)
	{
		//////////////////////////////////
		//add this window to the list
		ptr = new CDWnd_List();
		if(ptr == NULL)								return -1;

		if(msg == WM_CREATE)
			ptr->wnd = object = (CDWnd*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		else
			ptr->wnd = object = (CDWnd*)lParam;

		ptr->next = pGlobalWindowList;
		pGlobalWindowList = ptr;

		if(!object->hWnd)
			object->hWnd = wnd;		//make sure they have the handle

		if(msg == WM_INITDIALOG)
			((CDDialog*)object)->OnInitDialog();
	}
	else
	{
		//find the object
		object = NULL;
		prev = NULL;
		ptr = pGlobalWindowList;
		while(ptr != NULL)
		{
			if(ptr->wnd->hWnd == wnd)
			{
				object = ptr->wnd;
				break;
			}
			prev = ptr;
			ptr = ptr->next;
		}

		if(object == NULL)			return -1;		//no window found!

		if(msg == WM_COMMAND)		//command messages may be mapped
		{
			msgmap = object->m_MessageMap;
			while(msgmap)
			{
				if((msgmap->dwCtrlID == LOWORD(wParam)) && (msgmap->dwMsgID == HIWORD(wParam)))
				{
					(object->*(msgmap->pProc))();
					return 0;
				}
				msgmap = msgmap->next;
			}
		}
		else if(msg == WM_NOTIFY)
		{
			msgmap = object->m_NotifyMap;
			while(msgmap)
			{
				if((msgmap->dwCtrlID == ((LPNMHDR)lParam)->idFrom) && (msgmap->dwMsgID == ((LPNMHDR)lParam)->code))
				{
					(object->*(msgmap->pProc))();
					return 0;
				}
				msgmap = msgmap->next;
			}
		}
		else if(msg == WM_DESTROY)
		{
			//remove the object from the global list
			if(prev)
				prev->next = ptr->next;
			if(pGlobalWindowList == ptr)
				pGlobalWindowList = ptr->next;
			delete ptr;
			ptr = NULL;
		}
	}

	return object->WndProc(msg,wParam,lParam);
}


//////////////////////////////////////////////////////////////////////////
// support functions

void CDWnd::ScreenToClient(RECT* rc)
{
	LPPOINT pt = (LPPOINT)rc;
	::ScreenToClient(hWnd,pt);	pt++;
	::ScreenToClient(hWnd,pt);
}

void CDWnd::MoveWindow(RECT* rc,bool redraw /* = 1 */)
{
	::MoveWindow(hWnd,rc->left,rc->top,rc->right - rc->left,rc->bottom - rc->top,redraw);
}