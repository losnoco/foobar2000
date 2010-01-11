// File_Extractor 0.4.3
// This source code is a heavily modified version based on the unrar package.
// It may not be used to develop a RAR (WinRAR) compatible archiver.
// See unrar/license.txt for copyright and licensing.

#ifndef RAR_COMMON_HPP
#define RAR_COMMON_HPP

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <limits.h>

//// Glue

#define rarmalloc   malloc
#define rarrealloc  realloc
#define rarfree     free

// Renaming to avoid collisions, since they are such generic names
#define Array       Rar_Array
#define uint32      rar_uint32
#define sint32      rar_sint32

// Must NOT be defined (left in source to keep it closer to original)
#undef SFX_MODULE

// During debugging, if expr is false, prints message then continues execution
#ifndef check
	#define check( expr ) ((void) 0)
#endif

struct Rar_Error_Handler
{
	jmp_buf jmp_env;
	void MemoryError();
};

struct Rar_Allocator
{
	// provides allocator that doesn't throw an exception on failure
	static void operator delete ( void* p ) { free( p ); }
	static void* operator new ( size_t s )
// throw spec mandatory in ISO C++ if operator new can return NULL
#if __cplusplus >= 199711 || __GNUC__ >= 3
	throw ()
#endif
	{
		return malloc( s );
	}
};

//// os.hpp
#define FALSE 0
#define TRUE  1
#define NM  1024
#define CPATHDIVIDER '/'

#undef STRICT_ALIGNMENT_REQUIRED
#undef LITTLE_ENDIAN

#if defined (__i386__) || defined (__x86_64__) || defined (_M_IX86) || defined (_M_X64)
	// Optimizations mostly only apply to x86
	#define LITTLE_ENDIAN
	#define ALLOW_NOT_ALIGNED_INT
#endif

#if defined(__sparc) || defined(sparc) || defined(__sparcv9)
/* prohibit not aligned access to data structures in text comression
   algorithm, increases memory requirements */
	#define STRICT_ALIGNMENT_REQUIRED
#endif

//// rartypes.hpp
#if INT_MAX == 0x7FFFFFFF && UINT_MAX == 0xFFFFFFFF
	typedef unsigned int     uint32; //32 bits exactly
	typedef          int     sint32; //signed 32 bits exactly
	#define PRESENT_INT32
#endif

//// rartypes.hpp
typedef unsigned char    byte;   //8 bits
typedef unsigned short   ushort; //preferably 16 bits, but can be more
typedef unsigned int     uint;   //32 bits or more

#define SHORT16(x) (sizeof(ushort)==2 ? (ushort)(x):((x)&0xffff))
#define UINT32(x)  (sizeof(uint  )==4 ? (uint  )(x):((x)&0xffffffff))

//// rardefs.hpp
#define  Min(x,y) (((x)<(y)) ? (x):(y))
#define  Max(x,y) (((x)>(y)) ? (x):(y))

#define  ASIZE(x) (sizeof(x)/sizeof(x[0]))

//// unicode.hpp
#define charnext(s) ((s)+1)

//// int64.hpp
// 64-bit int support disabled, since this library isn't meant for huge 2GB+ archives
typedef long Int64;
#define int64to32(x) ((uint)(x))
#define is64plus(x) (x>=0)

//// crc.hpp
void InitCRC();
uint CRC(uint StartCRC,const void *Addr,uint Size);
ushort OldCRC(ushort StartCRC,const void *Addr,uint Size);

//// rartime.hpp
struct RarTime
{
	unsigned time;
    void SetDos(uint DosTime) { time = DosTime; }
};

//// rdwrfn.hpp
class ComprDataIO
		: public Rar_Error_Handler
{
public:
	class File_Reader* reader;
	class Data_Writer* writer;
	const char* write_error; // once write error occurs, no more writes are made
 	long Tell_;
	bool OldFormat;

private:
	Int64 UnpPackedSize;
    bool SkipUnpCRC;

public:
	int UnpRead(byte *Addr,uint Count);
    void UnpWrite(byte *Addr,uint Count);
	void SetSkipUnpCRC( bool b ) { SkipUnpCRC = b; }
	void SetPackedSizeToRead( Int64 n ) { UnpPackedSize = n; }

	uint UnpFileCRC;

	const char* Seek( long, int ignored = 0 );
	long Tell() { return Tell_; }
	long Read( void*, long );
};

//// rar.hpp
class Unpack;
#include "array.hpp"
#include "headers.hpp"
#include "getbits.hpp"
#include "archive.hpp"
#include "rawread.hpp"
#include "compress.hpp"
#include "rarvm.hpp"
#include "model.hpp"
#include "unpack.hpp"

//// extract.hpp
class CmdExtract
	: public Rar_Allocator
{
private:
	Unpack Unp;
	long FileCount;
 	Array<byte> Buffer;

	const char* ExtractCurrentFile( Data_Writer&, bool SkipSolid = 0 );

	CmdExtract( File_Reader* in );
	void UnstoreFile( Int64 );

public:
	Archive Arc;
	friend class Rar_Extractor;
};

#endif
