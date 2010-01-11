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
#define SAFE_NEW(p,t,s) p = new t[s]; ZeroMemory(p,sizeof(t) * s)


void CNSFFile::LoadFile( service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort )
{
	Destroy();

	t_uint32 type;
	p_file->read_lendian_t( type, p_abort );

	if ( type == HEADERTYPE_NESM ) LoadFile_NESM(p_file, needdata, p_abort);
	else if ( type == HEADERTYPE_NSFE ) LoadFile_NSFE(p_file, needdata, p_abort);
	else throw exception_io_data();
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

void CNSFFile::LoadFile_NESM( service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort )
{
	int len;

	{
		{
			t_filesize len64 = p_file->get_size_ex( p_abort );
			if ( len64 >= ((1 << 30) - 0x80)) throw exception_io_data();
			len = ((int)len64) - 0x80;
		}

		p_file->seek( 0, p_abort );

		if( len < 1 ) throw exception_io_data();

		//read the info
		NESM_HEADER					hdr;
		p_file->read_object( &hdr, 0x80, p_abort );

		//confirm the header
		if( byte_order::dword_le_to_native( hdr.nHeader ) != HEADERTYPE_NESM ) throw exception_io_data();
		if( hdr.nHeaderExtra != 0x1A )                                         throw exception_io_data();
		if( hdr.nVersion > 2 )                                                 throw exception_io_data();

		//NESM is generally easier to work with (but limited!)
		//  just move the data over from NESM_HEADER over to our member data

		bIsExtended =				false;
		nIsPal =					((hdr.nNTSC_PAL & 0x03) == 0x01);
		nPAL_PlaySpeed =			byte_order::word_le_to_native( hdr.nSpeedPAL );			//blarg
		nNTSC_PlaySpeed =			byte_order::word_le_to_native( hdr.nSpeedNTSC );			//blarg
		nLoadAddress =				byte_order::word_le_to_native( hdr.nLoadAddress );
		nInitAddress =				byte_order::word_le_to_native( hdr.nInitAddress );
		nPlayAddress =				byte_order::word_le_to_native( hdr.nPlayAddress );
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
			p_file->read_object(pDataBuffer, len, p_abort);
			nDataBufferSize = len;
		}
	}

	//if we got this far... it was a successful read
}

void CNSFFile::LoadFile_NSFE( service_ptr_t<file> & p_file, bool needdata, abort_callback & p_abort )
{
	{
		//restart the file
		p_file->seek(0, p_abort);

		//the vars we'll be using
		t_uint32 nChunkType;
		t_uint32 nChunkSize;
		t_uint32 nChunkUsed;
		t_filesize nDataPos = 0;
		bool	bInfoFound = false;
		bool	bEndFound = false;
		bool	bBankFound = false;

		NSFE_INFOCHUNK	info;
		ZeroMemory(&info,sizeof(NSFE_INFOCHUNK));
		info.nTrackCount = 1;		//default values

		//confirm the header!
		p_file->read_lendian_t( nChunkType, p_abort);
		if(nChunkType != HEADERTYPE_NSFE)			throw exception_io_data();

		//begin reading chunks
		while(!bEndFound)
		{
			p_abort.check();

			if( p_file->is_eof( p_abort ) ) throw exception_io_data();

			p_file->read_lendian_t( nChunkSize, p_abort );
			p_file->read_lendian_t( nChunkType, p_abort );

			switch( nChunkType )
			{
			case CHUNKTYPE_INFO:
				if(bInfoFound)						throw exception_io_data();	//only one info chunk permitted
				if(nChunkSize < 8)					throw exception_io_data();	//minimum size

				bInfoFound = true;
				nChunkUsed = min((int)sizeof(NSFE_INFOCHUNK),nChunkSize);

				p_file->read_object( &info, nChunkUsed, p_abort );
				p_file->skip( nChunkSize - nChunkUsed, p_abort );

				bIsExtended =			true;
				nIsPal =				info.nIsPal;
				nLoadAddress =			byte_order::word_le_to_native( info.nLoadAddress );
				nInitAddress =			byte_order::word_le_to_native( info.nInitAddress );
				nPlayAddress =			byte_order::word_le_to_native( info.nPlayAddress );
				nChipExtensions =		info.nExt;
				nTrackCount =			info.nTrackCount;
				nInitialTrack =			info.nStartingTrack;

				nPAL_PlaySpeed =		(WORD)(1000000 / NSF_PAL_NMIRATE);		//blarg
				nNTSC_PlaySpeed =		(WORD)(1000000 / NSF_NTSC_NMIRATE);		//blarg
				break;

			case CHUNKTYPE_DATA:
				if(!bInfoFound)						throw exception_io_data();
				if(nDataPos)						throw exception_io_data();
				if(nChunkSize < 1)					throw exception_io_data();

				nDataBufferSize = nChunkSize;
				nDataPos = p_file->get_position( p_abort );

				p_file->skip( nChunkSize, p_abort );
				break;

			case CHUNKTYPE_NEND:
				bEndFound = true;
				break;

			case CHUNKTYPE_TIME:
				if(!bInfoFound)						throw exception_io_data();
				if(pTrackTime)						throw exception_io_data();

				SAFE_NEW( pTrackTime, int, nTrackCount );
				nChunkUsed = min( nChunkSize / 4, nTrackCount );

				for ( unsigned i = 0; i < nChunkUsed; ++i )
					p_file->read_lendian_t( pTrackTime[ i ], p_abort );
				p_file->skip( nChunkSize - ( nChunkUsed * 4 ), p_abort );

				for(; nChunkUsed < nTrackCount; nChunkUsed++)
					pTrackTime[nChunkUsed] = -1;	//negative signals to use default time

				break;

			case CHUNKTYPE_FADE:
				if(!bInfoFound)						throw exception_io_data();
				if(pTrackFade)						throw exception_io_data();

				SAFE_NEW( pTrackFade, int, nTrackCount );
				nChunkUsed = min( nChunkSize / 4, nTrackCount );

				for ( unsigned i = 0; i < nChunkUsed; ++i )
					p_file->read_lendian_t( pTrackFade[ i ], p_abort );
				p_file->skip( nChunkSize - ( nChunkUsed * 4 ), p_abort );

				for(; nChunkUsed < nTrackCount; nChunkUsed++)
					pTrackFade[nChunkUsed] = -1;	//negative signals to use default time

				break;

			case CHUNKTYPE_BANK:
				if(bBankFound)						throw exception_io_data();

				bBankFound = 1;
				nChunkUsed = min(8,nChunkSize);

				p_file->read_object(nBankswitch, nChunkUsed, p_abort);
				p_file->skip(nChunkSize - nChunkUsed, p_abort);
				break;

			case CHUNKTYPE_PLST:
				if(pPlaylist)						throw exception_io_data();

				nPlaylistSize = nChunkSize;
				if(nPlaylistSize < 1)				break;  //no playlist?

				SAFE_NEW(pPlaylist,BYTE,nPlaylistSize);
				p_file->read_object(pPlaylist, nChunkSize, p_abort);
				break;

			case CHUNKTYPE_AUTH:		{
				if(szGameTitle)						throw exception_io_data();

				char*		buffer = 0;

				try
				{
					char*		ptr;
					SAFE_NEW(buffer,char,nChunkSize + 4);

					p_file->read_object(buffer, nChunkSize, p_abort);
					ptr = buffer;

					char**		ar[4] = {&szGameTitle,&szArtist,&szCopyright,&szRipper};
					int			i;
					for(i = 0; i < 4; i++)
					{
						nChunkUsed = strlen(ptr) + 1;
						*ar[i] = new char[nChunkUsed];
						memcpy(*ar[i],ptr,nChunkUsed);
						ptr += nChunkUsed;
					}
				}
				catch (...)
				{
					SAFE_DELETE(buffer);
					throw;
				}
				SAFE_DELETE(buffer);
										}break;

			case CHUNKTYPE_TLBL:		{
				if(!bInfoFound)						throw exception_io_data();
				if(szTrackLabels)					throw exception_io_data();

				SAFE_NEW(szTrackLabels,char*,nTrackCount);

				char*		buffer = 0;

				try
				{
					char*		ptr;
					SAFE_NEW(buffer,char,nChunkSize + nTrackCount);

					p_file->read_object(buffer, nChunkSize, p_abort);
					ptr = buffer;

					UINT		i;
					for(i = 0; i < nTrackCount; i++)
					{
						nChunkUsed = strlen(ptr) + 1;
						szTrackLabels[i] = new char[nChunkUsed];
						memcpy(szTrackLabels[i],ptr,nChunkUsed);
						ptr += nChunkUsed;
					}
				}
				catch (...)
				{
					SAFE_DELETE(buffer);
					throw;
				}
				SAFE_DELETE(buffer);
										}break;

			default:		//unknown chunk
				nChunkType &= 0x000000FF;  //check the first byte
				if((nChunkType >= 'A') && (nChunkType <= 'Z'))	//chunk is vital... don't continue
					throw exception_io_data();
				//otherwise, just skip it
				p_file->skip(nChunkSize, p_abort);

				break;
			}		//end switch
		}			//end while

		//if we exited the while loop without a 'return', we must have hit an NEND chunk
		//  if this is the case, the file was layed out as it was expected.
		//  now.. make sure we found both an info chunk, AND a data chunk... since these are
		//  minimum requirements for a valid NSFE file

		if(!bInfoFound)			throw exception_io_data();
		if(!nDataPos)			throw exception_io_data();

		//if both those chunks existed, this file is valid.  Load the data if it's needed

		if(needdata)
		{
			p_file->seek( nDataPos, p_abort );
			SAFE_NEW( pDataBuffer, BYTE, nDataBufferSize );
			p_file->read_object( pDataBuffer, nDataBufferSize, p_abort );
		}
		else
			nDataBufferSize = 0;
	}

	//return success!
}


//////////////////////////////////////////////////////////////////////////
//  File saving

void CNSFFile::SaveFile( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	if( ! pDataBuffer )		//if we didn't grab the data, we can't save it
		throw exception_io_data();

	if(bIsExtended)		SaveFile_NSFE(p_file, p_abort);
	else				SaveFile_NESM(p_file, p_abort);
}

void CNSFFile::SaveFile_NESM( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	NESM_HEADER			hdr;
	ZeroMemory(&hdr,0x80);

	hdr.nHeader =				HEADERTYPE_NESM;
	hdr.nHeaderExtra =			0x1A;
	hdr.nVersion =				1;
	hdr.nTrackCount =			nTrackCount;
	hdr.nInitialTrack =			nInitialTrack + 1;
	hdr.nLoadAddress =			byte_order::word_native_to_le( nLoadAddress );
	hdr.nInitAddress =			byte_order::word_native_to_le( nInitAddress );
	hdr.nPlayAddress =			byte_order::word_native_to_le( nPlayAddress );

	if(szGameTitle)				memcpy(hdr.szGameTitle,szGameTitle,min(strlen(szGameTitle),31));
	if(szArtist)				memcpy(hdr.szArtist   ,szArtist   ,min(strlen(szArtist)   ,31));
	if(szCopyright)				memcpy(hdr.szCopyright,szCopyright,min(strlen(szCopyright),31));

	hdr.nSpeedNTSC =			byte_order::word_native_to_le( nNTSC_PlaySpeed );
	memcpy(hdr.nBankSwitch,nBankswitch,8);
	hdr.nSpeedPAL =				byte_order::word_native_to_le( nPAL_PlaySpeed );
	hdr.nNTSC_PAL =				nIsPal;
	hdr.nExtraChip =			nChipExtensions;

	//the header is all set... slap it in
	p_file->write_object(&hdr, 0x80, p_abort);

	//slap in the NSF info
	p_file->write_object(pDataBuffer, nDataBufferSize, p_abort);

	//we're done.. all the other info that isn't recorded is dropped for regular NSFs
}

void CNSFFile::SaveFile_NSFE( service_ptr_t<file> & p_file, abort_callback & p_abort )
{
	//////////////////////////////////////////////////////////////////////////
	// I must admit... NESM files are a bit easier to work with than NSFEs =P

	t_uint32        nChunkType;
	t_uint32        nChunkSize;
	NSFE_INFOCHUNK  info;

	{
		//write the header
		nChunkType = HEADERTYPE_NSFE;
		p_file->write_lendian_t( nChunkType, p_abort);


		//write the info chunk
		nChunkType =			CHUNKTYPE_INFO;
		nChunkSize =			sizeof(NSFE_INFOCHUNK);
		info.nExt =				nChipExtensions;
		info.nInitAddress =		byte_order::word_native_to_le( nInitAddress );
		info.nIsPal =			nIsPal;
		info.nLoadAddress =		byte_order::word_native_to_le( nLoadAddress );
		info.nPlayAddress =		byte_order::word_native_to_le( nPlayAddress );
		info.nStartingTrack =	nInitialTrack;
		info.nTrackCount =		nTrackCount;

		p_file->write_lendian_t( nChunkSize, p_abort );
		p_file->write_lendian_t( nChunkType, p_abort);
		p_file->write_object( &info, nChunkSize, p_abort);

		//if we need bankswitching... throw it in
		for(nChunkSize = 0; nChunkSize < 8; nChunkSize++)
		{
			if(nBankswitch[nChunkSize])
			{
				nChunkType = CHUNKTYPE_BANK;
				nChunkSize = 8;
				p_file->write_lendian_t( nChunkSize, p_abort );
				p_file->write_lendian_t( nChunkType, p_abort );
				p_file->write_object( nBankswitch, nChunkSize, p_abort );
				break;
			}
		}

		//if there's a time chunk, slap it in
		if( pTrackTime )
		{
			nChunkType =		CHUNKTYPE_TIME;
			nChunkSize =		4 * nTrackCount;
			p_file->write_lendian_t( nChunkSize, p_abort );
			p_file->write_lendian_t( nChunkType, p_abort );

			for ( unsigned i = 0; i < nTrackCount; ++i )
				p_file->write_lendian_t( pTrackTime[ i ], p_abort );
		}

		//slap in a fade chunk if needed
		if( pTrackFade )
		{
			nChunkType =		CHUNKTYPE_FADE;
			nChunkSize =		4 * nTrackCount;
			p_file->write_lendian_t( nChunkSize, p_abort );
			p_file->write_lendian_t( nChunkType, p_abort );

			for ( unsigned i = 0; i < nTrackCount; ++i )
				p_file->write_lendian_t( pTrackFade[ i ], p_abort );
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
			p_file->write_lendian_t( nChunkSize, p_abort );
			p_file->write_lendian_t( nChunkType, p_abort );

			if(szGameTitle)		p_file->write_object(szGameTitle, strlen(szGameTitle) + 1, p_abort);
			else				p_file->write_object("", 1, p_abort);
			if(szArtist)		p_file->write_object(szArtist, strlen(szArtist) + 1, p_abort);
			else				p_file->write_object("", 1, p_abort);
			if(szCopyright)		p_file->write_object(szCopyright, strlen(szCopyright) + 1, p_abort);
			else				p_file->write_object("", 1, p_abort);
			if(szRipper)		p_file->write_object(szRipper, strlen(szRipper) + 1, p_abort);
			else				p_file->write_object("", 1, p_abort);
		}

		//plst
		if(pPlaylist)
		{
			nChunkType =		CHUNKTYPE_PLST;
			nChunkSize =		nPlaylistSize;
			p_file->write_lendian_t( nChunkSize, p_abort );
			p_file->write_lendian_t( nChunkType, p_abort );
			p_file->write_object( pPlaylist, nChunkSize, p_abort );
		}

		//tlbl
		if( szTrackLabels )
		{
			nChunkType =		CHUNKTYPE_TLBL;
			nChunkSize =		nTrackCount;

			for(UINT i = 0; i < nTrackCount; i++)
				if (szTrackLabels[i])
					nChunkSize += strlen(szTrackLabels[i]);

			p_file->write_lendian_t( nChunkSize, p_abort );
			p_file->write_lendian_t( nChunkType, p_abort );

			for( UINT i = 0; i < nTrackCount; i++ )
			{
				if( szTrackLabels[i] )
					p_file->write_object( szTrackLabels[i], strlen(szTrackLabels[i]) + 1, p_abort );
				else
					p_file->write_object( "", 1, p_abort );
			}
		}

		//data
		nChunkType =			CHUNKTYPE_DATA;
		nChunkSize =			nDataBufferSize;
		p_file->write_lendian_t( nChunkSize, p_abort );
		p_file->write_lendian_t( nChunkType, p_abort );
		p_file->write_object( pDataBuffer, nChunkSize, p_abort );

		//END
		nChunkType =			CHUNKTYPE_NEND;
		nChunkSize =			0;
		p_file->write_lendian_t( nChunkSize, p_abort );
		p_file->write_lendian_t( nChunkType, p_abort );
	}

	//w00t
}