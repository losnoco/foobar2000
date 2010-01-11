#include "rar.hpp"

static const char rar_corrupt [] = "Corrupt RAR file";

// arcread.cpp
const char* Archive::ReadHeader()
{
	CurBlockPos=Tell();

#ifndef SFX_MODULE
	if (OldFormat)
	{
		ReadOldHeader();

		if ( Raw.Size() == 0 )
			return ""; // right at end of file

		if ( Raw.PaddedSize() > 0 ) // added check
			return rar_corrupt; // missing data

		return NULL;
	}
#endif

	Raw.Reset();

	// (removed decryption)

	Raw.Read(SIZEOF_SHORTBLOCKHEAD);
	if (Raw.Size()==0)
		return ""; // right at end of file

	Raw.Get(ShortBlock.HeadCRC);
	byte HeadType;
	Raw.Get(HeadType);
	ShortBlock.HeadType=(HEADER_TYPE)HeadType;
	Raw.Get(ShortBlock.Flags);
	Raw.Get(ShortBlock.HeadSize);
	if (ShortBlock.HeadSize<SIZEOF_SHORTBLOCKHEAD)
		return rar_corrupt; // invalid HeadSize

	// catches unknown block types and reading something that isn't even a header
	check( MARK_HEAD <= ShortBlock.HeadType && ShortBlock.HeadType <= ENDARC_HEAD );

	if (ShortBlock.HeadType==COMM_HEAD)
		Raw.Read(SIZEOF_COMMHEAD-SIZEOF_SHORTBLOCKHEAD);
	else
		if (ShortBlock.HeadType==MAIN_HEAD && (ShortBlock.Flags & MHD_COMMENT)!=0)
			Raw.Read(SIZEOF_NEWMHD-SIZEOF_SHORTBLOCKHEAD);
		else
			Raw.Read(ShortBlock.HeadSize-SIZEOF_SHORTBLOCKHEAD);

	if ( Raw.PaddedSize() > 0 ) // added check
		return rar_corrupt; // missing data

	NextBlockPos=CurBlockPos+ShortBlock.HeadSize;

	switch(ShortBlock.HeadType)
	{
	case MAIN_HEAD:
		*(BaseBlock *)&NewMhd=ShortBlock;
		Raw.Get(NewMhd.HighPosAV);
		Raw.Get(NewMhd.PosAV);
		check( Raw.ReadPos == Raw.DataSize ); // we should have read all fields
		break;
	case FILE_HEAD:
	case NEWSUB_HEAD:
		{
			FileHeader *hd=ShortBlock.HeadType==FILE_HEAD ? &NewLhd:&SubHead;
			*(BaseBlock *)hd=ShortBlock;
			Raw.Get(hd->PackSize);
			Raw.Get(hd->UnpSize);
			Raw.Get(hd->HostOS);
			Raw.Get(hd->FileCRC);
			Raw.Get(hd->FileTime);
			Raw.Get(hd->UnpVer);
			Raw.Get(hd->Method);
			Raw.Get(hd->NameSize);
			Raw.Get(hd->FileAttr);
			if (hd->Flags & LHD_LARGE)
			{
				Raw.Get(hd->HighPackSize);
				Raw.Get(hd->HighUnpSize);
			}
			else
			{
				hd->HighPackSize=hd->HighUnpSize=0;
			}
			check( hd->UnpSize != 0xFFFFFFFF);
			hd->FullPackSize=hd->PackSize;
			hd->FullUnpSize=hd->UnpSize;

			if ( hd->HighPackSize || hd->HighUnpSize )
				return "Huge RAR files not supported";

			char (&FileName) [NM*4] = hd->FileName; // eliminated local buffer
			int NameSize=Min(hd->NameSize,sizeof(FileName)-1);
			Raw.Get((byte *)FileName,NameSize);
			FileName[NameSize]=0;

			if (hd->HeadType==NEWSUB_HEAD)
			{
				// have to adjust this, even through we're ignoring this block
				NextBlockPos+=hd->FullPackSize;
				break;
			}
			else
				if (hd->HeadType==FILE_HEAD)
				{
					if (hd->Flags & LHD_UNICODE)
					{
						EncodeFileName NameCoder;
						int Length=strlen(FileName);
						if (Length==hd->NameSize)
						{
							/*UtfToWide(FileName,hd->FileNameW,sizeof(hd->FileNameW)/sizeof(hd->FileNameW[0])-1);
							WideToChar(hd->FileNameW,hd->FileName,sizeof(hd->FileName)/sizeof(hd->FileName[0])-1);
							ExtToInt(hd->FileName,hd->FileName);*/
						}
						else
						{
							wchar_t FileNameW[NM*4];
							Length++;
							NameCoder.Decode(FileName,(byte *)FileName+Length,
								hd->NameSize-Length,FileNameW,
								sizeof(FileNameW)/sizeof(FileNameW[0]));
							WideToUtf( FileNameW, hd->FileName, sizeof(hd->FileName)/sizeof(hd->FileName[0]) );
						}
					}
					else
					{
						// Local codepage to UTF-8 conversion
						wchar_t FileNameW[NM*4];
						CharToWide( FileName, FileNameW, sizeof(FileNameW)/sizeof(FileNameW[0]) );
						WideToUtf( FileNameW, hd->FileName, sizeof(hd->FileName)/sizeof(hd->FileName[0]) );
					}

					ConvertUnknownHeader();
				}
			if (hd->Flags & LHD_SALT)
				Raw.Get(hd->Salt,SALT_SIZE);
			hd->mtime.SetDos(hd->FileTime);
			if (hd->Flags & LHD_EXTTIME)
			{
				ushort Flags;
				Raw.Get(Flags);
				// Ignore additional time information
				for (int I=0;I<4;I++)
				{
					uint rmode=Flags>>(3-I)*4;
					if ((rmode & 8)==0)
						continue;
					if (I!=0)
					{
						uint DosTime;
						Raw.Get(DosTime);
					}

					// skip time info
		            int count=rmode&3;
		            for (int J=0;J<count;J++)
					{
						byte CurByte;
						Raw.Get(CurByte);
					}
				}
			}
			NextBlockPos+=hd->FullPackSize;
			bool CRCProcessedOnly=(hd->Flags & LHD_COMMENT)!=0;
			HeaderCRC=~Raw.GetCRC(CRCProcessedOnly)&0xffff;
			if (hd->HeadCRC!=HeaderCRC)
				return rar_corrupt;
			check( CRCProcessedOnly == false ); // I need to test on archives where this doesn't hold
			check( Raw.ReadPos == Raw.DataSize ); // we should have read all fields
		}
		break;
#ifndef SFX_MODULE
	// Handle these block types just so we can adjust NextBlockPos properly
	case PROTECT_HEAD:
		Raw.Get(ProtectHead.DataSize);
		NextBlockPos+=ProtectHead.DataSize;
		break;
	case SUB_HEAD:
		Raw.Get(SubBlockHead.DataSize);
		NextBlockPos+=SubBlockHead.DataSize;
		break;
#endif
	default:
		if (ShortBlock.Flags & LONG_BLOCK)
		{
			uint DataSize;
			Raw.Get(DataSize);
			NextBlockPos+=DataSize;
		}
		break;
	}
	HeaderCRC=~Raw.GetCRC(false)&0xffff;
	CurHeaderType=ShortBlock.HeadType;
	// (removed decryption)

	if (NextBlockPos<CurBlockPos+Raw.Size())
		return rar_corrupt; // next block isn't past end of current block's header

	return NULL;
}

#ifndef SFX_MODULE
int Archive::ReadOldHeader()
{
	Raw.Reset();
	if (CurBlockPos<=SFXSize)
	{
		Raw.Read(SIZEOF_OLDMHD);
		Raw.Get(OldMhd.Mark,4);
		Raw.Get(OldMhd.HeadSize);
		Raw.Get(OldMhd.Flags);
		NextBlockPos=CurBlockPos+OldMhd.HeadSize;
		CurHeaderType=MAIN_HEAD;
	}
	else
	{
		OldFileHeader OldLhd;
		Raw.Read(SIZEOF_OLDLHD);
		NewLhd.HeadType=FILE_HEAD;
		Raw.Get(NewLhd.PackSize);
		Raw.Get(NewLhd.UnpSize);
		Raw.Get(OldLhd.FileCRC);
		Raw.Get(NewLhd.HeadSize);
		Raw.Get(NewLhd.FileTime);
		Raw.Get(OldLhd.FileAttr);
		Raw.Get(OldLhd.Flags);
		Raw.Get(OldLhd.UnpVer);
		Raw.Get(OldLhd.NameSize);
		Raw.Get(OldLhd.Method);

		NewLhd.Flags=OldLhd.Flags|LONG_BLOCK;
		NewLhd.UnpVer=(OldLhd.UnpVer==2) ? 13 : 10;
		NewLhd.Method=OldLhd.Method+0x30;
		NewLhd.NameSize=OldLhd.NameSize;
		NewLhd.FileAttr=OldLhd.FileAttr;
		NewLhd.FileCRC=OldLhd.FileCRC;
		NewLhd.FullPackSize=NewLhd.PackSize;
		NewLhd.FullUnpSize=NewLhd.UnpSize;

		NewLhd.mtime.SetDos(NewLhd.FileTime);
		// (removed other times)

		Raw.Read(OldLhd.NameSize);
		Raw.Get((byte *)NewLhd.FileName,OldLhd.NameSize);
		NewLhd.FileName[OldLhd.NameSize]=0;
		// (removed name case conversion)

		if (Raw.Size()!=0)
			NextBlockPos=CurBlockPos+NewLhd.HeadSize+NewLhd.PackSize;
		CurHeaderType=FILE_HEAD;
	}
	return(NextBlockPos>CurBlockPos ? Raw.Size():0);
}
#endif

// (removed name case and attribute conversion)

void Archive::ConvertUnknownHeader()
{
	if (NewLhd.UnpVer<20 && (NewLhd.FileAttr & 0x10))
		NewLhd.Flags|=LHD_DIRECTORY;
	if (NewLhd.HostOS>=HOST_MAX)
	{
		if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
			NewLhd.FileAttr=0x10;
		else
			NewLhd.FileAttr=0x20;
	}
	for (char *s=NewLhd.FileName;*s!=0;s=charnext(s))
	{
		if (*s=='/' || *s=='\\')
			*s=CPATHDIVIDER;
	}
	// (removed Apple and Unicode handling)
}
