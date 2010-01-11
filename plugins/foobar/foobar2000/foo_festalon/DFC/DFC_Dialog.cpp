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
//  DFC_Dialog.cpp
//
//

#include "DFC.h"



//////////////////////////////////////////////////////////////////////////
//
//  WndProc

int CDDialog::WndProc(UINT msg,WPARAM wParam,LPARAM lParam)
{
	if(msg == WM_COMMAND)
	{
		if(LOWORD(wParam) == IDOK)			OnOK();
		if(LOWORD(wParam) == IDCANCEL)		OnCancel();
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////
//
//  Construction / Destruction

CDDialog::CDDialog() : CDWnd()
{
	hWnd = NULL;
	bIsModal = 0;
}

CDDialog::~CDDialog()
{
	CDWnd::~CDWnd();
}


//////////////////////////////////////////////////////////////////////////
//
//  OnOK / OnCancel

void CDDialog::OnOK()
{
	if(bIsModal)
		EndDialog(hWnd,IDOK);
	else
		ShowWindow(0);
}

void CDDialog::OnCancel()
{
	if(bIsModal)
		EndDialog(hWnd,IDCANCEL);
	else
		ShowWindow(0);
}

void CDDialog::Destroy()
{
	if(!bIsModal)
		DestroyWindow(hWnd);
}

//////////////////////////////////////////////////////////////////////////
//
//  Creation Initialization

int		CDDialog::DoModal(HINSTANCE instance,HWND parentwnd,UINT dlgtemplate)
{
	if(hWnd != NULL)	return -1;		//dialog cannot already be in use

	bIsModal = 1;
	hInstance = instance;
	
	return (int)DialogBox(instance,MAKEINTRESOURCE(dlgtemplate),parentwnd,
		GenericWindowProc,(LPARAM)this);
}

HWND	CDDialog::DoModeless(HINSTANCE instance,HWND parentwnd,UINT dlgtemplate)
{
	if(hWnd != NULL)	return NULL;

	bIsModal = 0;
	hInstance = instance;

	return CreateDialog(instance,MAKEINTRESOURCE(dlgtemplate),parentwnd,
		GenericWindowProc,(LPARAM)this);
}


//////////////////////////////////////////////////////////////////////////
//  DIALOG CONTROLS

CDDialogControl::CDDialogControl() : CDWnd()
{
	nID = 0;
}

CDDialogControl::~CDDialogControl()
{
	CDWnd::~CDWnd();
}

void CDDialogControl::AttachToControl(HWND hParent,UINT id)
{
	hParentWnd = hParent;
	nID = id;
	hWnd = GetDlgItem(hParent,id);
}

//////////////////////////////////////////////////////////////////////////
// Individual controls


//////////////////////////////////////////////////////////////////////////
//  CDListBox

void CDListBox::GetText(UINT index, pfc::string_base & out)
{
	uListBox_GetText(hWnd,index,out);
}

//////////////////////////////////////////////////////////////////////////
//  CDTab

UINT CDTab::InsertItem(UINT index,LPCSTR str)
{
	uTCITEM item;
	ZeroMemory(&item,sizeof(uTCITEM));
	item.mask = TCIF_TEXT;
	item.pszText = (LPSTR)str;
	item.iImage = -1;

	return uTabCtrl_InsertItem(hWnd,index,&item);
}