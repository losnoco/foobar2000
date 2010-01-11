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
//  DFC_Array.cpp
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "DFC_Array.h"

CDByteArray::CDByteArray()
{
	nGrowBy = 16;
	nBufferSize = nGrowBy;
	pData = new BYTE[nBufferSize];

	nCount = 0;
}

CDByteArray::~CDByteArray()
{
	delete[] pData;
	nCount = 0;
	nBufferSize = 0;
	nGrowBy = 16;
	pData = NULL;
}

UINT CDByteArray::Add(BYTE add)
{
	if(nCount >= nBufferSize)
		IncreaseBufferSize(nCount + 1);

	pData[nCount] = add;
	nCount++;
	return (nCount - 1);
}

void CDByteArray::RemoveAt(UINT index)
{
	if(!nCount) return;

	for(index++; index < nCount; index++)
		pData[index - 1] = pData[index];
	 nCount--;
}

void CDByteArray::IncreaseBufferSize(UINT required)
{
	while(nBufferSize < required)
		nBufferSize += nGrowBy;

	BYTE* temp = new BYTE[nBufferSize];

	memcpy(temp,pData,nCount);

	delete[] pData;
	pData = temp;
}