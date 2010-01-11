
// Rar_Extractor 0.3.0. See unrar_license.txt.

#ifndef _RAR_ARCHIVE_
#define _RAR_ARCHIVE_

class RawRead {
private:
	Rar_Array<byte> Data;
	File_Reader* rar_reader;
	int DataSize;
	int ReadPos;
public:
	RawRead( File_Reader*, Rar_Error_Handler* );
	void Reset();
	void Read(int Size);
	void get8( byte& out );
	void get16( ushort& out );
	void get32( uint& out );
	void Get(byte *Field,int Size);
	uint GetCRC( bool partial );
	int Size() {return DataSize;}
};

class Archive {
public:

	Archive( File_Reader*, Rar_Error_Handler* );
	int IsArchive();
	const char* IsArchive2();
	const char* ReadHeader();
	void SeekToNext();
	int GetHeaderType() {return(CurHeaderType);};

	BaseBlock ShortBlock;
	MainHeader NewMhd;
	FileHeader NewLhd;
	FileHeader SubHead;
	
	Int64 CurBlockPos;
	Int64 NextBlockPos;

	bool OldFormat;
	bool Solid;
	enum { SFXSize = 0 }; // we don't support self-extracting archive data at the beginning
	ushort HeaderCRC;

	void Seek( Int64 pos ) { rar_reader->seek( pos ); }
	Int64 Tell() { return rar_reader->tell(); }
	long Read( void* p, long s ) { return rar_reader->read_avail( p, s ); }
	
	Rar_Error_Handler* error_handler;
	
private:
	File_Reader* rar_reader;
	RawRead Raw;
	
	MarkHeader MarkHead;
	OldMainHeader OldMhd;

	int LastReadBlock;
	int CurHeaderType;

	bool IsSignature(byte *D);
	const char* ReadOldHeader();
};

#endif

