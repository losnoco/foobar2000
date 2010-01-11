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
//  DFC_Array.h
//

class CDByteArray
{
public:
	CDByteArray();
	~CDByteArray();

	UINT			Add(BYTE add);
	UINT			GetCount() { return nCount; }
	const BYTE*		GetData() { return pData; }
	void			RemoveAll() { nCount = 0; }
	void			RemoveAt(UINT index);

	BYTE& operator [] (UINT index) {return pData[index]; }

protected:
	void			IncreaseBufferSize(UINT required);

	BYTE*			pData;
	UINT			nBufferSize;
	UINT			nCount;
	UINT			nGrowBy;
};