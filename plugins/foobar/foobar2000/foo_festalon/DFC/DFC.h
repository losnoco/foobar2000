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
//  DFC.h
//
//		Contains classes for common windows features
//
//

#include <windows.h>
#include <stdlib.h>
#include <pfc/pfc.h>
#include <shared.h>

#include <CommCtrl.h>
#include "DFC_Array.h"

//////////////////////////////////////////////////////////////////////////
//
//  CDWnd
//
//		for windows!
//

class CDWnd;
typedef void(CDWnd::*MessageMapCallback)();

#define AFX_MAPMESSAGE(CtrlID,MsgID,Func) MapMessage(CtrlID,MsgID,(MessageMapCallback)Func)
#define AFX_MAPNOTIFY(CtrlID,MsgID,Func) MapNotify(CtrlID,MsgID,(MessageMapCallback)Func)

class CDWnd
{
public:
	CDWnd();
	~CDWnd();

	virtual void				SetWindowText(const char * text) { uSetWindowText(hWnd,text); }
	virtual void				GetWindowText(string_base & out) { uGetWindowText(hWnd, out); }

	virtual HWND				GetHandle() { return hWnd; }
	virtual HWND				GetParentHandle() { return hParentWnd; }

	virtual void				GetWindowRect(RECT* rc) { ::GetWindowRect(hWnd,rc); }
	virtual void				MoveWindow(RECT* rc,bool redraw = 1);

	virtual void				EnableWindow(int enable) { ::EnableWindow(hWnd,enable); }
	virtual void				ShowWindow(int show) { ::ShowWindow(hWnd,show); }

	virtual void				ScreenToClient(RECT* rc);

protected:
	static INT_PTR CALLBACK		GenericWindowProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam);
	virtual int					WndProc(UINT msg,WPARAM wParam,LPARAM lParam) { return (int)uDefWindowProc(hWnd,msg,wParam,lParam); };


protected:
	// Message mapping
	class DDlgMsgItem
	{
	public:
		DDlgMsgItem()		{ next = NULL; }

		DWORD				dwCtrlID;
		DWORD				dwMsgID;
		MessageMapCallback	pProc;
		DDlgMsgItem*		next;
	};

	DDlgMsgItem*			m_MessageMap;
	DDlgMsgItem*			m_NotifyMap;

	void					MapMessage(UINT uControl,UINT uMsg,MessageMapCallback pProc)
	{
		DDlgMsgItem*		pItem = new DDlgMsgItem();
		pItem->dwCtrlID =	uControl;
		pItem->dwMsgID =	uMsg;
		pItem->pProc =		pProc;
		pItem->next =		m_MessageMap;

		m_MessageMap =		pItem;
	}

	void					MapNotify(UINT uControl,UINT uMsg,MessageMapCallback pProc)
	{
		DDlgMsgItem*		pItem = new DDlgMsgItem();
		pItem->dwCtrlID =	uControl;
		pItem->dwMsgID =	uMsg;
		pItem->pProc =		pProc;
		pItem->next =		m_NotifyMap;

		m_NotifyMap =		pItem;
	}

protected:
	HINSTANCE				hInstance;
	HWND					hWnd;
	HWND					hParentWnd;
};


//////////////////////////////////////////////////////////////////////////
//
//  CDDialogControl
//
//		Dialog controls
//

class CDDialogControl : public CDWnd
{
public:
	CDDialogControl();
	~CDDialogControl();

	//////////////////////////////////////////////////////////////////////////
	// Creation
	virtual void		AttachToControl(HWND hParent,UINT id);

protected:
	UINT			nID;
};


//////////////////////////////////////////////////////////////////////////
//
//  CDStatic
//
//	  for static controls on dialogs
//

class CDStatic : public CDDialogControl
{
public:
	CDStatic() : CDDialogControl()		{ }
	~CDStatic()							{CDDialogControl::~CDDialogControl(); }
};

//////////////////////////////////////////////////////////////////////////
//
//  CDEdit
//
//	  for edit boxes (maybe there'll be more later?)
//

class CDEdit : public CDDialogControl
{
public:
	CDEdit() : CDDialogControl()		{ }
	~CDEdit()							{CDDialogControl::~CDDialogControl(); }
};

//////////////////////////////////////////////////////////////////////////
//
//  CDButton
//
//	  for buttons
//

class CDButton : public CDDialogControl
{
public:
	CDButton() : CDDialogControl()		{ }
	~CDButton()							{CDDialogControl::~CDDialogControl(); }

	//////////////////////////////////////////////////////////////////////////
	//interactive functions
	void			SetCheck(int set)	{::CheckDlgButton(hParentWnd,nID,set);}
	UINT			GetCheck()			{return ::IsDlgButtonChecked(hParentWnd,nID);}
};

//////////////////////////////////////////////////////////////////////////
//
//  CDListBox

class CDListBox : public CDDialogControl
{
public:
	CDListBox() : CDDialogControl()			{ }
	~CDListBox()							{CDDialogControl::~CDDialogControl(); }

	//////////////////////////////////////////////////////////////////////////
	//interactive functions
	UINT			GetCurSel()								{return (UINT)uSendMessage(hWnd,LB_GETCURSEL,0,0);}
	void			SetCurSel(UINT sel)						{uSendMessage(hWnd,LB_SETCURSEL,sel,0);}
	UINT			GetCount()								{return (UINT)uSendMessage(hWnd,LB_GETCOUNT,0,0);}

	//adding/removing strings
	void			ResetContent()							{uSendMessage(hWnd,LB_RESETCONTENT,0,0);}
	void			DeleteString(UINT index)				{uSendMessage(hWnd,LB_DELETESTRING,index,0);}
	UINT			AddString(LPCSTR str)					{return (UINT)uSendMessageText(hWnd,LB_ADDSTRING,0,str);}
	UINT			InsertString(UINT index,LPCSTR str)		{return (UINT)uSendMessageText(hWnd,LB_INSERTSTRING,index,str);}

	//text retrieval
	//  -is unsafe in non-Unicode environments, and is not used in my project -kode54
	void			GetText(UINT index, string_base & out);
};

//////////////////////////////////////////////////////////////////////////
//
//  CDComboBox

class CDComboBox : public CDDialogControl
{
public:
	CDComboBox() : CDDialogControl()		{ }
	~CDComboBox()							{ CDDialogControl::~CDDialogControl(); }

	//////////////////////////////////////////////////////////////////////////
	//interactive functions
	UINT			GetCurSel()								{return (UINT)uSendMessage(hWnd,CB_GETCURSEL,0,0);}
	void			SetCurSel(UINT sel)						{uSendMessage(hWnd,CB_SETCURSEL,sel,0);}
	UINT			GetCount()								{return (UINT)uSendMessage(hWnd,CB_GETCOUNT,0,0);}

	//adding/removing strings
	void			ResetContent()							{uSendMessage(hWnd,CB_RESETCONTENT,0,0);}
	void			DeleteString(UINT index)				{uSendMessage(hWnd,CB_DELETESTRING,index,0);}
	UINT			AddString(LPCSTR str)					{return (UINT)uSendMessageText(hWnd,CB_ADDSTRING,0,str);}
	UINT			InsertString(UINT index,LPCSTR str)		{return (UINT)uSendMessageText(hWnd,CB_INSERTSTRING,index,str);}
};

//////////////////////////////////////////////////////////////////////////
//
//  CDSlider

class CDSlider : public CDDialogControl
{
public:
	CDSlider() : CDDialogControl()		{ }
	~CDSlider()							{CDDialogControl::~CDDialogControl();}

	void			SetRange(UINT Min,UINT Max,bool redraw = 1)	{uSendMessage(hWnd,TBM_SETRANGE,redraw,MAKELONG(Min,Max));}
	void			SetPos(UINT pos,bool redraw = 1)				{uSendMessage(hWnd,TBM_SETPOS,redraw,pos);}
	UINT			GetPos()									{return (UINT)uSendMessage(hWnd,TBM_GETPOS,0,0);}
};

//////////////////////////////////////////////////////////////////////////
//
//  CDTab

class CDTab : public CDDialogControl
{
public:
	CDTab() : CDDialogControl()			{ }
	~CDTab()							{CDDialogControl::~CDDialogControl(); }

	//////////////////////////////////////////////////////////////////////////
	//interactive functions
	UINT			GetCurSel()							{return (UINT)uSendMessage(hWnd,TCM_GETCURSEL,0,0); }
	void			SetCurSel(UINT sel)					{uSendMessage(hWnd,TCM_SETCURSEL,sel,0); }
	UINT			GetCount();

	//adding/removing items
	void			DeleteAllItems()					{uSendMessage(hWnd,TCM_DELETEALLITEMS,0,0);}
	void			DeleteItem(UINT index)				{uSendMessage(hWnd,TCM_DELETEITEM,index,0);}
	UINT			InsertItem(UINT index,LPCSTR str);
};

//////////////////////////////////////////////////////////////////////////
//
//  CDDialog
//
//    for Modal/Modeless dialogs
//

class CDDialog : public CDWnd
{
public:
	//construction / destruction
	CDDialog();
	~CDDialog();


	//creation / initialization
	int							DoModal(HINSTANCE instance,HWND parentwnd,UINT dlgtemplate);
	HWND						DoModeless(HINSTANCE instance,HWND parentwnd,UINT dlgtemplate);

	virtual void				OnOK();
	virtual	void				OnCancel();
	virtual void				OnInitDialog() { }
	virtual void				Destroy();

protected:
	// window proc
	virtual int					WndProc(UINT msg,WPARAM wParam,LPARAM lParam);


protected:
	// data members
	BYTE			bIsModal;
};