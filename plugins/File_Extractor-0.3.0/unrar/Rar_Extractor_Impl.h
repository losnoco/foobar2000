
// Rar_Extractor 0.3.0

#ifndef RAR_EXTRACTOR_IMPL_H
#define RAR_EXTRACTOR_IMPL_H

#include "rar.hpp"

extern char end_of_rar [];

class Rar_Extractor_Impl : public Rar_Allocator {
public:
	int UnpRead( byte*, uint );
	void UnpWrite( byte const*, uint );
	jmp_buf jmp_env;
	
private:
	Archive arc;
	bool first_file;
	Unpack Unp;
 	Rar_Array<byte> Buffer;
	const char* write_error; // once write error occurs, no more writes are made
	Data_Writer* writer;
	Int64 UnpPackedSize;
	uint UnpFileCRC,PackedCRC;
	bool MaintainCRC;
	
	const char* extract( Data_Writer&, bool check_crc = true );
	
	Rar_Extractor_Impl( File_Reader* in );
	void UnstoreFile( Int64 );
	
	friend class Rar_Extractor;
};

#endif

