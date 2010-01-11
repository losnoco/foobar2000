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
//  PlaylistDlg.cpp
//

#include <DFC.h>
#include <foobar2000.h>
#include "../helpers/window_placement_helper.h"

#include "NSF_File.h"
#include "PlaylistDlg.h"

#include "resource.h"

// {67EF39A5-3684-45cd-B26F-7B07AEC388E7}
static const GUID guid_cfg_placement = 
{ 0x67ef39a5, 0x3684, 0x45cd, { 0xb2, 0x6f, 0x7b, 0x7, 0xae, 0xc3, 0x88, 0xe7 } };

static cfg_window_placement cfg_placement(guid_cfg_placement);

class GetTrackLabel : public string8
{
public:
	GetTrackLabel(CNSFFile* file,BYTE track)
	{
		do
		{
			if(track >= file->nTrackCount)		break;
			if(!file->szTrackLabels)			break;
			if(!file->szTrackLabels[track])		break;
			if(!file->szTrackLabels[track][0])	break;
			set_string(file->szTrackLabels[track]);
			return;
		}
		while(0);
		set_string("Track ");
		add_string(format_int(track + 1));
	}
};

void CPlaylistDlg::OnOK()
{
	StoreValues();
	cfg_placement.on_window_destruction(hWnd);
	CDDialog::OnOK();
}

void CPlaylistDlg::OnCancel()
{
	cfg_placement.on_window_destruction(hWnd);
	CDDialog::OnCancel();
}

void CPlaylistDlg::OnInitDialog()
{
	CDDialog::OnInitDialog();

	cfg_placement.on_window_creation(hWnd);

	uSetWindowText(hWnd, pszTitle);

	m_tracklist.AttachToControl(hWnd,IDC_TRACKLIST);
	m_playlist.AttachToControl(hWnd,IDC_PLAYLIST);

	AFX_MAPMESSAGE(IDC_PLAYLIST,LBN_DBLCLK,&CPlaylistDlg::OnRemove);
	AFX_MAPMESSAGE(IDC_TRACKLIST,LBN_DBLCLK,&CPlaylistDlg::OnAdd);
	AFX_MAPMESSAGE(IDC_ADD,BN_CLICKED,&CPlaylistDlg::OnAdd);
	AFX_MAPMESSAGE(IDC_REMOVE,BN_CLICKED,&CPlaylistDlg::OnRemove);
	AFX_MAPMESSAGE(IDC_REMOVEALL,BN_CLICKED,&CPlaylistDlg::OnRemoveAll);
	AFX_MAPMESSAGE(IDC_ADDALL,BN_CLICKED,&CPlaylistDlg::OnAddAll);
	AFX_MAPMESSAGE(IDC_MOVEUP,BN_CLICKED,&CPlaylistDlg::OnUp);
	AFX_MAPMESSAGE(IDC_MOVEDOWN,BN_CLICKED,&CPlaylistDlg::OnDown);

	nPlaylist.RemoveAll();
	for(UINT i = 0; i < pFile->nPlaylistSize; i++)
		nPlaylist.Add(pFile->pPlaylist[i]);

	RefreshList();
}

void CPlaylistDlg::RefreshList()
{
	m_tracklist.ResetContent();
	m_playlist.ResetContent();

	UINT i;
	for(i = 0; i < pFile->nTrackCount; i++)
		m_tracklist.InsertString(i,GetTrackLabel(pFile,i));

	for(i = 0; i < pFile->nPlaylistSize; i++)
		m_playlist.InsertString(i,GetTrackLabel(pFile,pFile->pPlaylist[i]));
}

void CPlaylistDlg::StoreValues()
{
	UINT count = nPlaylist.GetCount();
	if(count)
	{
		if(count > pFile->nPlaylistSize)
		{
			if(pFile->pPlaylist)
				delete[] pFile->pPlaylist;
			pFile->pPlaylist = new BYTE[count];
		}
		pFile->nPlaylistSize = count;
		memcpy(pFile->pPlaylist,nPlaylist.GetData(),count);
	}
	else
	{
		if(pFile->pPlaylist)
			delete[] pFile->pPlaylist;
		pFile->pPlaylist = NULL;
		pFile->nPlaylistSize = 0;
	}
}

void CPlaylistDlg::OnAdd()
{
	UINT sel = m_tracklist.GetCurSel();
	if(sel == -1)	return;

	m_playlist.InsertString(nPlaylist.GetCount(),GetTrackLabel(pFile,sel));
	nPlaylist.Add(sel);
}

void CPlaylistDlg::OnRemove()
{
	UINT sel = m_playlist.GetCurSel();
	if(sel == -1)	return;

	nPlaylist.RemoveAt(sel);
	m_playlist.DeleteString(sel);
	if(sel >= m_playlist.GetCurSel())
		sel = m_playlist.GetCurSel();

	m_playlist.SetCurSel(sel);
}

void CPlaylistDlg::OnAddAll()
{
	for(UINT i = 0; i < pFile->nTrackCount; i++)
	{
		m_playlist.InsertString(nPlaylist.GetCount(),GetTrackLabel(pFile,i));
		nPlaylist.Add(i);
	}
}

void CPlaylistDlg::OnRemoveAll()
{
	nPlaylist.RemoveAll();
	m_playlist.ResetContent();
}

void CPlaylistDlg::OnUp()
{
	UINT sel = m_playlist.GetCurSel();
	if(sel <= 0)	return;

	BYTE temp = nPlaylist[sel];
	nPlaylist[sel] = nPlaylist[sel - 1];
	nPlaylist[sel - 1] = temp;

	m_playlist.DeleteString(sel);
	m_playlist.InsertString(sel - 1,GetTrackLabel(pFile,temp));
	m_playlist.SetCurSel(sel - 1);
}

void CPlaylistDlg::OnDown()
{
	UINT sel = m_playlist.GetCurSel();
	if(sel < 0)		return;
	if((sel + 1) >= m_playlist.GetCount()) return;

	BYTE temp = nPlaylist[sel];
	nPlaylist[sel] = nPlaylist[sel + 1];
	nPlaylist[sel + 1] = temp;

	m_playlist.DeleteString(sel);
	m_playlist.InsertString(sel + 1,GetTrackLabel(pFile,temp));
	m_playlist.SetCurSel(sel + 1);
}