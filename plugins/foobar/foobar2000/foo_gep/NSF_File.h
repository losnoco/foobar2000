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
//  NSF_File.h
//
//

#define					HEADERTYPE_NESM			'MSEN'
#define					HEADERTYPE_NSFE			'EFSN'

#define					CHUNKTYPE_INFO			'OFNI'
#define					CHUNKTYPE_DATA			'ATAD'
#define					CHUNKTYPE_NEND			'DNEN'
#define					CHUNKTYPE_PLST			'tslp'
#define					CHUNKTYPE_TIME			'emit'
#define					CHUNKTYPE_FADE			'edaf'
#define					CHUNKTYPE_TLBL			'lblt'
#define					CHUNKTYPE_AUTH			'htua'
#define					CHUNKTYPE_BANK			'KNAB'


struct NESM_HEADER
{
	t_uint32		nHeader;
	t_uint8			nHeaderExtra;
	t_uint8			nVersion;
	t_uint8			nTrackCount;
	t_uint8			nInitialTrack;
	t_uint16		nLoadAddress;
	t_uint16		nInitAddress;
	t_uint16		nPlayAddress;
	t_int8			szGameTitle[32];
	t_int8			szArtist[32];
	t_int8			szCopyright[32];
	t_uint16		nSpeedNTSC;
	t_uint8			nBankSwitch[8];
	t_uint16		nSpeedPAL;
	t_uint8			nNTSC_PAL;
	t_uint8			nExtraChip;
	t_uint8			nExpansion[4];
};

struct NSFE_INFOCHUNK
{
	t_uint16		nLoadAddress;
	t_uint16		nInitAddress;
	t_uint16		nPlayAddress;
	t_uint8			nIsPal;
	t_uint8			nExt;
	t_uint8			nTrackCount;
	t_uint8			nStartingTrack;
};



class CNSFFile
{
public:
	CNSFFile() { ZeroMemory(this,sizeof(CNSFFile)); }
	~CNSFFile() { Destroy(); }
	void LoadFile( service_ptr_t< file > & p_file, bool needdata, abort_callback & p_abort );//Loads a file from a specified path.  If needdata is false,
														//  the NSF code is not loaded, only the other information
														//  (like track times, game title, Author, etc)
														//If you're loading an NSF with intention to play it, needdata
														//  must be true
	void SaveFile( service_ptr_t< file > & p_file, abort_callback & p_abort );				//Saves the NSF to a file... including any changes you made (like to track times, etc)
	void			Destroy();							//Cleans up memory

protected:
	void LoadFile_NESM(service_ptr_t<file> & p_file,bool needdata, abort_callback & p_abort);	//these functions are used internally and should not be called
	void LoadFile_NSFE(service_ptr_t<file> & p_file,bool needdata, abort_callback & p_abort);

	void SaveFile_NESM(service_ptr_t<file> & p_file, abort_callback & p_abort);
	void SaveFile_NSFE(service_ptr_t<file> & p_file, abort_callback & p_abort);

public:

	///////////////////////////////////
	//data members

	//basic NSF info
	bool				bIsExtended;		//0 = NSF, 1 = NSFE
	BYTE				nIsPal;				//0 = NTSC, 1 = PAL, 2,3 = mixed NTSC/PAL (interpretted as NTSC)
	int					nLoadAddress;		//The address to which the NSF code is loaded to
	int					nInitAddress;		//The address of the Init routine (called at track change)
	int					nPlayAddress;		//The address of the Play routine (called several times a second)
	BYTE				nChipExtensions;	//Bitwise representation of the external chips used by this NSF.  Read NSFSpec.txt for details.
	
	//old NESM speed stuff (blarg)
	int					nNTSC_PlaySpeed;
	int					nPAL_PlaySpeed;

	//track info
	UINT				nTrackCount;		//The number of tracks in the NSF (1 = 1 track, 5 = 5 tracks, etc)
	UINT				nInitialTrack;		//The initial track (ZERO BASED:  0 = 1st track, 4 = 5th track, etc)

	//nsf data
	BYTE*				pDataBuffer;		//the buffer containing NSF code.  If needdata was false when loading the NSF, this is NULL
	UINT				nDataBufferSize;	//the size of the above buffer.  0 if needdata was false

	//playlist
	BYTE*				pPlaylist;			//the buffer containing the playlist (NULL if none exists).  Each entry is the zero based index of the song to play
	UINT				nPlaylistSize;		//the size of the above buffer (and the number of tracks in the playlist)

	//track time / fade
	t_int32*				pTrackTime;			//the buffer containing the track times.  NULL if no track times specified.  Otherwise this buffer MUST BE (nTrackCount * sizeof(int)) in size
	t_int32*				pTrackFade;			//the buffer containing the track fade times.  NULL if none are specified.  Same conditions as pTrackTime

	//track labels
	char**				szTrackLabels;		//the buffer containing track labels.  NULL if there are no labels.  There must be nTrackCount char pointers (or none if NULL).
											//Each pointer must point to it's own buffer containing a string (the length of this buffer doesn't matter, just so long as the string is NULL terminated)
											// the string's buffer may be NULL if a string isn't needed
											//szTrackLabels as well as all of the buffers it points to are destroyed upon
											// a call to Destroy (or the destructor).

	//string info
	char*				szGameTitle;		//pointer to a NULL-terminated string containing the name of the game.  Or NULL if no name specified
	char*				szArtist;			//pointer to a NULL-terminated string containing the author of the NSF.  Or NULL if none specified
	char*				szCopyright;		//pointer to a NULL-terminated string containing the copyright info.  Or NULL if none specified
	char*				szRipper;			//pointer to a NULL-terminated string containing the 'hacker' who ripped the NSF.  Or NULL if none specified

	//bankswitching info
	BYTE				nBankswitch[8];		//The initial bankswitching registers needed for some NSFs.  If the NSF does not use bankswitching, these values will all be zero
};