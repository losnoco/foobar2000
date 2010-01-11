#include "rar.hpp"

#include <stdio.h>

#define Unp    (&Unp)
#define DataIO Arc

const char* CmdExtract::ExtractCurrentFile( Data_Writer& rar_writer, bool SkipSolid )
{
	check( Arc.GetHeaderType() == FILE_HEAD );

	if ( Arc.NewLhd.Flags & (LHD_SPLIT_AFTER | LHD_SPLIT_BEFORE) )
		return "Segmented RAR file not supported";

	if ( Arc.NewLhd.Flags & LHD_PASSWORD )
		return "Encrypted RAR file not supported";

	check(      Arc.NextBlockPos-Arc.NewLhd.FullPackSize == Arc.Tell() );
	Arc.Seek(Arc.NextBlockPos-Arc.NewLhd.FullPackSize,SEEK_SET);

	// (removed lots of command-line handling)

#ifdef SFX_MODULE
	if ((Arc.NewLhd.UnpVer!=UNP_VER && Arc.NewLhd.UnpVer!=29) &&
			Arc.NewLhd.Method!=0x30)
#else
	if (Arc.NewLhd.UnpVer<13 || Arc.NewLhd.UnpVer>UNP_VER)
#endif
	{
		if (Arc.NewLhd.UnpVer>UNP_VER)
			return "RAR file uses new unsupported compression";
		return "RAR file uses old unsupported compression";
	}

	// (removed lots of command-line/encryption/volume handling)

	DataIO.UnpFileCRC=Arc.OldFormat ? 0 : 0xffffffff;
	// (removed decryption)
	DataIO.SetPackedSizeToRead(Arc.NewLhd.FullPackSize);
	// (removed command-line handling)
	DataIO.SetSkipUnpCRC(SkipSolid);
	DataIO.writer = &rar_writer;
	FileCount++;

	if (Arc.NewLhd.Method==0x30)
		UnstoreFile(Arc.NewLhd.FullUnpSize);
	else
	{
		Unp->SetDestSize(Arc.NewLhd.FullUnpSize);
#ifndef SFX_MODULE
		if (Arc.NewLhd.UnpVer<=15)
			Unp->DoUnpack(15,FileCount>1 && Arc.Solid);
		else
#endif
			Unp->DoUnpack(Arc.NewLhd.UnpVer,Arc.NewLhd.Flags & LHD_SOLID);
	}

	if (!SkipSolid)
	{
        if (Arc.OldFormat && UINT32(DataIO.UnpFileCRC)==UINT32(Arc.NewLhd.FileCRC) ||
            !Arc.OldFormat && UINT32(DataIO.UnpFileCRC)==UINT32(Arc.NewLhd.FileCRC^0xffffffff))
        {
        	// CRC is correct
        }
        else
        {
			return "Corrupt RAR file";
        }
	}

	// (removed broken file handling)
	// (removed command-line handling)

	return 0;
}


void CmdExtract::UnstoreFile(Int64 DestUnpSize)
{
	Buffer.Alloc(0x10000);
	while (1)
	{
		unsigned int Code=DataIO.UnpRead(&Buffer[0],Buffer.Size());
		if (Code==0 || (int)Code==-1)
			break;
		Code=Code<DestUnpSize ? Code:int64to32(DestUnpSize);
		DataIO.UnpWrite(&Buffer[0],Code);
		if (DestUnpSize>=0)
			 DestUnpSize-=Code;
	}
	Buffer.Reset();
}

