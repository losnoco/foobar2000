// File_Extractor 0.4.3
// This source code is a heavily modified version based on the unrar package.
// It may not be used to develop a RAR (WinRAR) compatible archiver.
// See unrar/license.txt for copyright and licensing.

#include "blargg_common.h"

#if FEX_ENABLE_RAR

#include "Rar_Extractor.h"

#include "unrar/rar.hpp"

#include "blargg_source.h"

Rar_Extractor::Rar_Extractor() : File_Extractor( fex_rar_type )
{
	impl = 0;
}

extern "C" {
	static File_Extractor* new_rar() { return BLARGG_NEW Rar_Extractor; }
}

fex_type_t_ const fex_rar_type [1] = {{ "RAR", &new_rar }};

void Rar_Extractor::close_()
{
	delete impl;
	impl = 0;
}

Rar_Extractor::~Rar_Extractor() { close(); }

blargg_err_t Rar_Extractor::open_()
{
	RETURN_ERR( file().seek( 0 ) );

	CHECK_ALLOC( impl = BLARGG_NEW CmdExtract( &file() ) );

	CHECK_ALLOC( !setjmp( impl->Arc.jmp_env ) );

	impl->Unp.Init( 0 );

	if ( !impl->Arc.IsArchive() )
		return fex_wrong_file_type;

	RETURN_ERR( impl->Arc.IsArchive2() );

	index       = -1;
	can_extract = true;

	return next_item();
}

blargg_err_t Rar_Extractor::rewind_()
{
	assert( impl );
	close_();
	return open_();
}

blargg_err_t Rar_Extractor::next_item()
{
	CHECK_ALLOC( !setjmp( impl->Arc.jmp_env ) );

	index++;
	for (;;)
	{
		RETURN_ERR( impl->Arc.SeekToNext() );
		blargg_err_t err = impl->Arc.ReadHeader();
		if ( err )
		{
			if ( *err )
				return err;

			dprintf( "RAR: Didn't end with ENDARC_HEAD\n" ); // rar -en causes this
			set_done();
			break;
		}

		HEADER_TYPE type = (HEADER_TYPE) impl->Arc.GetHeaderType();
		if ( type == ENDARC_HEAD )
		{
			// no files beyond this point
			set_done();
			break;
		}

		// skip non-files
		if ( type != FILE_HEAD )
		{
			if ( type != NEWSUB_HEAD && type != PROTECT_HEAD && type != SIGN_HEAD && type != SUB_HEAD )
				dprintf( "RAR: Skipping unknown block type: %X\n", (int) type );
			continue;
		}

		// skip labels
		if ( impl->Arc.NewLhd.HostOS <= HOST_WIN32 && (impl->Arc.NewLhd.FileAttr & 8) )
		{
			dprintf( "RAR: Skipping label\n" );
			continue;
		}

		// skip links
		if ( (impl->Arc.NewLhd.FileAttr & 0xF000) == 0xA000 )
		{
			dprintf( "RAR: Skipping link\n" );
			continue;
		}

		// skip directories
		if ( (impl->Arc.NewLhd.Flags & LHD_WINDOWMASK) == LHD_DIRECTORY )
		{
			dprintf( "RAR: Skipping directory\n" );
			continue;
		}

		set_info( impl->Arc.NewLhd.UnpSize, impl->Arc.NewLhd.FileName,
				impl->Arc.NewLhd.mtime.time );
		break;
	}

	extracted = false;
	return 0;
}

blargg_err_t Rar_Extractor::next_()
{
	CHECK_ALLOC( !setjmp( impl->Arc.jmp_env ) );

	if ( !extracted && impl->Arc.Solid )
	{
		if ( is_scan_only() )
		{
			can_extract = false;
		}
		else
		{
			// must (usually) extract every file in a solid archive
			Null_Writer out;
			RETURN_ERR( impl->ExtractCurrentFile( out, true ) );
		}
	}

	return next_item();
}

blargg_err_t Rar_Extractor::extract( Data_Writer& out )
{
	if ( !impl || extracted )
		return eof_error;

	CHECK_ALLOC( !setjmp( impl->Arc.jmp_env ) );

	if ( !can_extract )
	{
		// We skipped some files in solid archive, so we need to
		// go back and extract every one before current
		int saved_index = index;

		RETURN_ERR( rewind_() );
		scan_only( false );
		while ( index < saved_index )
		{
			if ( done() )
				return "Corrupt RAR archive";
			RETURN_ERR( next_() );
		}
		assert( can_extract );
	}
	extracted = true;

	impl->Arc.write_error = 0;
	RETURN_ERR( impl->ExtractCurrentFile( out ) );
	return impl->Arc.write_error;
}

// Rar_Extractor_Impl

const char* ComprDataIO::Seek( Int64 n, int )
{
	if ( n != Tell_ )
	{
		RETURN_ERR( reader->seek( n ) );
		Tell_ = n;
	}
	return 0;
}

long ComprDataIO::Read( void* p, long n )
{
	long result = reader->read_avail( p, n );
	if ( result < 0 )
		result = 0;
	Tell_ += result;
	return result;
}

void ComprDataIO::UnpWrite( byte* out, uint count )
{
	if ( !write_error )
		write_error = writer->write( out, count );

	if ( !SkipUnpCRC )
	{
		if ( OldFormat )
			UnpFileCRC = OldCRC( (ushort) UnpFileCRC, out, count );
		else
			UnpFileCRC = CRC( UnpFileCRC, out, count );
	}
}

int ComprDataIO::UnpRead( byte* out, uint count )
{
	if ( count <= 0 )
		return 0;

	if ( count > (uint) UnpPackedSize )
		count = UnpPackedSize;

	long result = Read( out, count );
	UnpPackedSize -= result;
	return result;
}

CmdExtract::CmdExtract( File_Reader* in ) :
	Unp( &Arc ),
	Buffer( &Arc )
{
	Arc.reader = in;
	Arc.Tell_  = 0;
	FileCount = 0;
	InitCRC();
}

void Rar_Error_Handler::MemoryError()
{
	longjmp( jmp_env, 1 );
}
#endif
