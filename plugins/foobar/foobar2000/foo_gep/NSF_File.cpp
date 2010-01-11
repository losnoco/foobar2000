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
//  NSF_File.cpp
//
//

// 2004-01-23 11:15 UTC - kode54
// - Added null pointer check to track list chunk size calculation

#include <foobar2000.h>

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <stdio.h>
//#include "NSF_Core.h"

const float NSF_PAL_NMIRATE =		50.006981727949469884953755921498f;
const float NSF_NTSC_NMIRATE =		60.098813897440495178612402970624f;

#include "NSF_File.h"

#define SAFE_DELETE(p) { if(p){ delete[] p; p = NULL; } }
#define SAFE_NEW(p,t,s) p = new t[s]; if(!p) return io_result_error_out_of_memory; ZeroMemory(p,sizeof(t) * s)


t_io_result CNSFFile::LoadFile(const service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort)
{
	Destroy();

	UINT type = 0;
	t_io_result status = p_file->read_object(&type, 4, p_abort);
	if (io_result_failed(status)) return status;

	if(type == HEADERTYPE_NESM)		status = LoadFile_NESM(p_file, needdata, p_abort);
	if(type == HEADERTYPE_NSFE)		status = LoadFile_NSFE(p_file, needdata, p_abort);

	return status;
}

void	CNSFFile::Destroy()
{
	SAFE_DELETE(pDataBuffer);
	SAFE_DELETE(pPlaylist);
	SAFE_DELETE(pTrackTime);
	SAFE_DELETE(pTrackFade);
	if(szTrackLabels)
	{
		for(UINT i = 0; i < nTrackCount; i++)
			SAFE_DELETE(szTrackLabels[i]);
		SAFE_DELETE(szTrackLabels);
	}
	SAFE_DELETE(szGameTitle);
	SAFE_DELETE(szArtist);
	SAFE_DELETE(szCopyright);
	SAFE_DELETE(szRipper);

	ZeroMemory(this,sizeof(CNSFFile));
}

t_io_result CNSFFile::LoadFile_NESM(const service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort)
{
	int len;

	try
	{
		{
			t_filesize len64 = p_file->get_size_e(p_abort);
			if (len64 >= ((1 << 30) - 0x80)) return io_result_error_data;
			len = ((int)len64) - 0x80;
		}

		p_file->seek_e(0, p_abort);

		if(len < 1) return io_result_error_data;

		//read the info
		NESM_HEADER					hdr;
		p_file->read_object_e(&hdr, 0x80, p_abort);

		//confirm the header
		if(hdr.nHeader != HEADERTYPE_NESM)		return io_result_error_data;
		if(hdr.nHeaderExtra != 0x1A)			return io_result_error_data;
		if(hdr.nVersion > 2)					return io_result_error_data;

		//NESM is generally easier to work with (but limited!)
		//  just move the data over from NESM_HEADER over to our member data

		bIsExtended =				0;
		nIsPal =					((hdr.nNTSC_PAL & 0x03) == 0x01);
		nPAL_PlaySpeed =			hdr.nSpeedPAL;			//blarg
		nNTSC_PlaySpeed =			hdr.nSpeedNTSC;			//blarg
		nLoadAddress =				hdr.nLoadAddress;
		nInitAddress =				hdr.nInitAddress;
		nPlayAddress =				hdr.nPlayAddress;
		nChipExtensions =			hdr.nExtraChip;


		nTrackCount =				hdr.nTrackCount;
		nInitialTrack =				hdr.nInitialTrack - 1;	//stupid 1-based number =P

		memcpy(nBankswitch,hdr.nBankSwitch,8);

		SAFE_NEW(szGameTitle,char,33);
		SAFE_NEW(szArtist   ,char,33);
		SAFE_NEW(szCopyright,char,33);

		memcpy(szGameTitle,hdr.szGameTitle,32);
		memcpy(szArtist   ,hdr.szArtist   ,32);
		memcpy(szCopyright,hdr.szCopyright,32);

		//read the NSF data
		if(needdata)
		{
			SAFE_NEW(pDataBuffer,BYTE,len);
			p_file->read_object_e(pDataBuffer, len, p_abort);
			nDataBufferSize = len;
		}
	}
	catch(exception_io const & e) {return e.get_code();}

	//if we got this far... it was a successful read
	return io_result_success;
}

t_io_result CNSFFile::LoadFile_NSFE(const service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort)
{
	try
	{
		//restart the file
		p_file->seek_e(0, p_abort);

		//the vars we'll be using
		UINT nChunkType;
		UINT nChunkSize;
		UINT nChunkUsed;
		UINT nDataPos = 0;
		BYTE	bInfoFound = 0;
		BYTE	bEndFound = 0;
		BYTE	bBankFound = 0;

		NSFE_INFOCHUNK	info;
		ZeroMemory(&info,sizeof(NSFE_INFOCHUNK));
		info.nTrackCount = 1;		//default values

		//confirm the header!
		p_file->read_object_e(&nChunkType, 4, p_abort);
		if(nChunkType != HEADERTYPE_NSFE)			return io_result_error_data;

		//begin reading chunks
		while(!bEndFound && !p_abort.is_aborting())
		{
			if(p_file->get_position_e(p_abort) >= p_file->get_size_e(p_abort)) return io_result_error_data;
			p_file->read_object_e(&nChunkSize, 4, p_abort);
			p_file->read_object_e(&nChunkType, 4, p_abort);

			switch(nChunkType)
			{
			case CHUNKTYPE_INFO:
				if(bInfoFound)						return io_result_error_data;	//only one info chunk permitted
				if(nChunkSize < 8)					return io_result_error_data;	//minimum size

				bInfoFound = 1;
				nChunkUsed = min((int)sizeof(NSFE_INFOCHUNK),nChunkSize);

				p_file->read_object_e(&info, nChunkUsed, p_abort);
				p_file->seek2_e(nChunkSize - nChunkUsed, SEEK_CUR, p_abort);

				bIsExtended =			1;
				nIsPal =				info.nIsPal;
				nLoadAddress =			info.nLoadAddress;
				nInitAddress =			info.nInitAddress;
				nPlayAddress =			info.nPlayAddress;
				nChipExtensions =		info.nExt;
				nTrackCount =			info.nTrackCount;
				nInitialTrack =			info.nStartingTrack;

				nPAL_PlaySpeed =		(WORD)(1000000 / NSF_PAL_NMIRATE);		//blarg
				nNTSC_PlaySpeed =		(WORD)(1000000 / NSF_NTSC_NMIRATE);		//blarg
				break;

			case CHUNKTYPE_DATA:
				if(!bInfoFound)						return io_result_error_data;
				if(nDataPos)						return io_result_error_data;
				if(nChunkSize < 1)					return io_result_error_data;

				nDataBufferSize = nChunkSize;
				nDataPos = (UINT) p_file->get_position_e(p_abort);

				p_file->seek2_e(nChunkSize, SEEK_CUR, p_abort);
				break;

			case CHUNKTYPE_NEND:
				bEndFound = 1;
				break;

			case CHUNKTYPE_TIME:
				if(!bInfoFound)						return io_result_error_data;
				if(pTrackTime)						return io_result_error_data;

				SAFE_NEW(pTrackTime,int,nTrackCount);
				nChunkUsed = min(nChunkSize / 4,nTrackCount);

				p_file->read_object_e(pTrackTime, nChunkUsed * 4, p_abort);
				p_file->seek2_e(nChunkSize - (nChunkUsed * 4), SEEK_CUR, p_abort);

				for(; nChunkUsed < nTrackCount; nChunkUsed++)
					pTrackTime[nChunkUsed] = -1;	//negative signals to use default time

				break;

			case CHUNKTYPE_FADE:
				if(!bInfoFound)						return io_result_error_data;
				if(pTrackFade)						return io_result_error_data;

				SAFE_NEW(pTrackFade,int,nTrackCount);
				nChunkUsed = min(nChunkSize / 4,nTrackCount);

				p_file->read_object_e(pTrackFade, nChunkUsed * 4, p_abort);
				p_file->seek2_e(nChunkSize - (nChunkUsed * 4), SEEK_CUR, p_abort);

				for(; nChunkUsed < nTrackCount; nChunkUsed++)
					pTrackFade[nChunkUsed] = -1;	//negative signals to use default time

				break;

			case CHUNKTYPE_BANK:
				if(bBankFound)						return io_result_error_data;

				bBankFound = 1;
				nChunkUsed = min(8,nChunkSize);

				p_file->read_object_e(nBankswitch, nChunkUsed, p_abort);
				p_file->seek2_e(nChunkSize - nChunkUsed, SEEK_CUR, p_abort);
				break;

			case CHUNKTYPE_PLST:
				if(pPlaylist)						return io_result_error_data;

				nPlaylistSize = nChunkSize;
				if(nPlaylistSize < 1)				break;  //no playlist?

				SAFE_NEW(pPlaylist,BYTE,nPlaylistSize);
				p_file->read_object_e(pPlaylist, nChunkSize, p_abort);
				break;

			case CHUNKTYPE_AUTH:		{
				if(szGameTitle)						return io_result_error_data;

				char*		buffer;
				char*		ptr;
				SAFE_NEW(buffer,char,nChunkSize + 4);

				p_file->read_object_e(buffer, nChunkSize, p_abort);
				ptr = buffer;

				char**		ar[4] = {&szGameTitle,&szArtist,&szCopyright,&szRipper};
				int			i;
				for(i = 0; i < 4; i++)
				{
					nChunkUsed = strlen(ptr) + 1;
					*ar[i] = new char[nChunkUsed];
					if(!*ar[i]) { SAFE_DELETE(buffer); return io_result_error_out_of_memory; }
					memcpy(*ar[i],ptr,nChunkUsed);
					ptr += nChunkUsed;
				}
				SAFE_DELETE(buffer);
										}break;

			case CHUNKTYPE_TLBL:		{
				if(!bInfoFound)						return io_result_error_data;
				if(szTrackLabels)					return io_result_error_data;

				SAFE_NEW(szTrackLabels,char*,nTrackCount);

				char*		buffer;
				char*		ptr;
				SAFE_NEW(buffer,char,nChunkSize + nTrackCount);

				p_file->read_object_e(buffer, nChunkSize, p_abort);
				ptr = buffer;

				UINT		i;
				for(i = 0; i < nTrackCount; i++)
				{
					nChunkUsed = strlen(ptr) + 1;
					szTrackLabels[i] = new char[nChunkUsed];
					if(!szTrackLabels[i]) { SAFE_DELETE(buffer); return io_result_error_out_of_memory; }
					memcpy(szTrackLabels[i],ptr,nChunkUsed);
					ptr += nChunkUsed;
				}
				SAFE_DELETE(buffer);
										}break;

			default:		//unknown chunk
				nChunkType &= 0x000000FF;  //check the first byte
				if((nChunkType >= 'A') && (nChunkType <= 'Z'))	//chunk is vital... don't continue
					return io_result_error_data;
				//otherwise, just skip it
				p_file->seek2_e(nChunkSize, SEEK_CUR, p_abort);

				break;
			}		//end switch
		}			//end while

		//if we exited the while loop without a 'return', we must have hit an NEND chunk
		//  if this is the case, the file was layed out as it was expected.
		//  now.. make sure we found both an info chunk, AND a data chunk... since these are
		//  minimum requirements for a valid NSFE file

		if(!bInfoFound)			return io_result_error_data;
		if(!nDataPos)			return io_result_error_data;

		//if both those chunks existed, this file is valid.  Load the data if it's needed

		if(needdata)
		{
			p_file->seek_e(nDataPos, p_abort);
			SAFE_NEW(pDataBuffer,BYTE,nDataBufferSize);
			p_file->read_object_e(pDataBuffer, nDataBufferSize, p_abort);
		}
		else
			nDataBufferSize = 0;
	}
	catch(exception_io const & e) {return e.get_code();}

	//return success!
	return io_result_success;
}


//////////////////////////////////////////////////////////////////////////
//  File saving

t_io_result CNSFFile::SaveFile(service_ptr_t<file> & p_file, abort_callback & p_abort)
{
	if(!pDataBuffer)		//if we didn't grab the data, we can't save it
		return io_result_error_data;

	t_io_result status;
	if(bIsExtended)		status = SaveFile_NSFE(p_file, p_abort);
	else				status = SaveFile_NESM(p_file, p_abort);

	return status;
}

t_io_result CNSFFile::SaveFile_NESM(service_ptr_t<file> & p_file, abort_callback & p_abort)
{
	NESM_HEADER			hdr;
	ZeroMemory(&hdr,0x80);

	hdr.nHeader =				HEADERTYPE_NESM;
	hdr.nHeaderExtra =			0x1A;
	hdr.nVersion =				1;
	hdr.nTrackCount =			nTrackCount;
	hdr.nInitialTrack =			nInitialTrack + 1;
	hdr.nLoadAddress =			nLoadAddress;
	hdr.nInitAddress =			nInitAddress;
	hdr.nPlayAddress =			nPlayAddress;

	if(szGameTitle)				memcpy(hdr.szGameTitle,szGameTitle,min(strlen(szGameTitle),31));
	if(szArtist)				memcpy(hdr.szArtist   ,szArtist   ,min(strlen(szArtist)   ,31));
	if(szCopyright)				memcpy(hdr.szCopyright,szCopyright,min(strlen(szCopyright),31));

	hdr.nSpeedNTSC =			nNTSC_PlaySpeed;
	memcpy(hdr.nBankSwitch,nBankswitch,8);
	hdr.nSpeedPAL =				nPAL_PlaySpeed;
	hdr.nNTSC_PAL =				nIsPal;
	hdr.nExtraChip =			nChipExtensions;

	try
	{
		//the header is all set... slap it in
		p_file->write_object_e(&hdr, 0x80, p_abort);

		//slap in the NSF info
		p_file->write_object_e(pDataBuffer, nDataBufferSize, p_abort);
	}
	catch(exception_io const & e) {return e.get_code();}

	//we're done.. all the other info that isn't recorded is dropped for regular NSFs
	return io_result_success;
}

t_io_result CNSFFile::SaveFile_NSFE(service_ptr_t<file> & p_file, abort_callback & p_abort)
{
	//////////////////////////////////////////////////////////////////////////
	// I must admit... NESM files are a bit easier to work with than NSFEs =P

	UINT			nChunkType;
	int				nChunkSize;
	NSFE_INFOCHUNK	info;

	try
	{
		//write the header
		nChunkType = HEADERTYPE_NSFE;
		p_file->write_object_e(&nChunkType, 4, p_abort);


		//write the info chunk
		nChunkType =			CHUNKTYPE_INFO;
		nChunkSize =			sizeof(NSFE_INFOCHUNK);
		info.nExt =				nChipExtensions;
		info.nInitAddress =		nInitAddress;
		info.nIsPal =			nIsPal;
		info.nLoadAddress =		nLoadAddress;
		info.nPlayAddress =		nPlayAddress;
		info.nStartingTrack =	nInitialTrack;
		info.nTrackCount =		nTrackCount;

		p_file->write_object_e(&nChunkSize, 4, p_abort);
		p_file->write_object_e(&nChunkType, 4, p_abort);
		p_file->write_object_e(&info, nChunkSize, p_abort);

		//if we need bankswitching... throw it in
		for(nChunkSize = 0; nChunkSize < 8; nChunkSize++)
		{
			if(nBankswitch[nChunkSize])
			{
				nChunkType = CHUNKTYPE_BANK;
				nChunkSize = 8;
				p_file->write_object_e(&nChunkSize, 4, p_abort);
				p_file->write_object_e(&nChunkType, 4, p_abort);
				p_file->write_object_e(nBankswitch, nChunkSize, p_abort);
				break;
			}
		}

		//if there's a time chunk, slap it in
		if(pTrackTime)
		{
			nChunkType =		CHUNKTYPE_TIME;
			nChunkSize =		4 * nTrackCount;
			p_file->write_object_e(&nChunkSize, 4, p_abort);
			p_file->write_object_e(&nChunkType, 4, p_abort);
			p_file->write_object_e(pTrackTime, nChunkSize, p_abort);
		}

		//slap in a fade chunk if needed
		if(pTrackFade)
		{
			nChunkType =		CHUNKTYPE_FADE;
			nChunkSize =		4 * nTrackCount;
			p_file->write_object_e(&nChunkSize, 4, p_abort);
			p_file->write_object_e(&nChunkType, 4, p_abort);
			p_file->write_object_e(pTrackFade, nChunkSize, p_abort);
		}

		//auth!
		if(szGameTitle || szCopyright || szArtist || szRipper)
		{
			nChunkType =		CHUNKTYPE_AUTH;
			nChunkSize =		4;
			if(szGameTitle)		nChunkSize += strlen(szGameTitle);
			if(szArtist)		nChunkSize += strlen(szArtist);
			if(szCopyright)		nChunkSize += strlen(szCopyright);
			if(szRipper)		nChunkSize += strlen(szRipper);
			p_file->write_object_e(&nChunkSize, 4, p_abort);
			p_file->write_object_e(&nChunkType, 4, p_abort);

			if(szGameTitle)		p_file->write_object_e(szGameTitle, strlen(szGameTitle) + 1, p_abort);
			else				p_file->write_object_e("", 1, p_abort);
			if(szArtist)		p_file->write_object_e(szArtist, strlen(szArtist) + 1, p_abort);
			else				p_file->write_object_e("", 1, p_abort);
			if(szCopyright)		p_file->write_object_e(szCopyright, strlen(szCopyright) + 1, p_abort);
			else				p_file->write_object_e("", 1, p_abort);
			if(szRipper)		p_file->write_object_e(szRipper, strlen(szRipper) + 1, p_abort);
			else				p_file->write_object_e("", 1, p_abort);
		}

		//plst
		if(pPlaylist)
		{
			nChunkType =		CHUNKTYPE_PLST;
			nChunkSize =		nPlaylistSize;
			p_file->write_object_e(&nChunkSize, 4, p_abort);
			p_file->write_object_e(&nChunkType, 4, p_abort);
			p_file->write_object_e(pPlaylist, nChunkSize, p_abort);
		}

		//tlbl
		if(szTrackLabels)
		{
			nChunkType =		CHUNKTYPE_TLBL;
			nChunkSize =		nTrackCount;

			for(UINT i = 0; i < nTrackCount; i++)
				if (szTrackLabels[i])
					nChunkSize += strlen(szTrackLabels[i]);

			p_file->write_object_e(&nChunkSize, 4, p_abort);
			p_file->write_object_e(&nChunkType, 4, p_abort);

			for(UINT i = 0; i < nTrackCount; i++)
			{
				if(szTrackLabels[i])
					p_file->write_object_e(szTrackLabels[i], strlen(szTrackLabels[i]) + 1, p_abort);
				else
					p_file->write_object_e("", 1, p_abort);
			}
		}

		//data
		nChunkType =			CHUNKTYPE_DATA;
		nChunkSize =			nDataBufferSize;
		p_file->write_object_e(&nChunkSize, 4, p_abort);
		p_file->write_object_e(&nChunkType, 4, p_abort);
		p_file->write_object_e(pDataBuffer, nChunkSize, p_abort);

		//END
		nChunkType =			CHUNKTYPE_NEND;
		nChunkSize =			0;
		p_file->write_object_e(&nChunkSize, 4, p_abort);
		p_file->write_object_e(&nChunkType, 4, p_abort);
	}
	catch(exception_io const & e) {return e.get_code();}

	//w00t
	return io_result_success;
}