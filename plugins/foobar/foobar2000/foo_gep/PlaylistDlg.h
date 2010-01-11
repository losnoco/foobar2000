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
//  PlaylistDlg.h
//
//

class CPlaylistDlg : public CDDialog
{
public:
				CPlaylistDlg() : CDDialog() { }
				~CPlaylistDlg() { }

	void		OnOK();
	void		OnCancel();
	void		OnInitDialog();

	void		OnRemove();
	void		OnRemoveAll();
	void		OnAdd();
	void		OnAddAll();
	void		OnUp();
	void		OnDown();

	void		StoreValues();

	void		RefreshList();

	CNSFFile*	pFile;
	LPCSTR      pszTitle;

	CDByteArray	nPlaylist;

	CDListBox	m_tracklist;
	CDListBox	m_playlist;
};