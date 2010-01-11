#ifndef _RAR_ARCHIVE_
#define _RAR_ARCHIVE_

typedef ComprDataIO File;
#include "rawread.hpp"

class Archive:public File
{
private:
	bool IsSignature(byte *D);
	void ConvertUnknownHeader();
	int ReadOldHeader();

	RawRead Raw;

	MarkHeader MarkHead;
	OldMainHeader OldMhd;

	int CurHeaderType;

public:
	Archive();
	int IsArchive();
	const char* IsArchive2();
	const char* ReadHeader();
    const char* SeekToNext();
	int GetHeaderType() {return(CurHeaderType);};

	BaseBlock ShortBlock;
	MainHeader NewMhd;
	FileHeader NewLhd;
    SubBlockHeader SubBlockHead;
	FileHeader SubHead;
    ProtectHeader ProtectHead;

	Int64 CurBlockPos;
	Int64 NextBlockPos;

	bool Solid;
    bool Volume;
    bool Encrypted;
	enum { SFXSize = 0 }; // self-extracting not supported
	ushort HeaderCRC;
};

#endif
