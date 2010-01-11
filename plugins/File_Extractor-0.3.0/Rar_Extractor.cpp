
// Rar_Extractor 0.3.0. http://www.slack.net/~ant/

#include "Rar_Extractor.h"

#include "timefn.h"

#include "unrar/rar.hpp"
#include <assert.h>

// See unrar_license.txt

char end_of_rar [] = "Unexpected end of RAR archive";

Rar_Extractor::Rar_Extractor() : impl( NULL ) { }

File_Extractor* extractor_open_rar( const char* path, const char** err )
{
	return extractor_open_( new Rar_Extractor, path, err );
}

void Rar_Extractor::close_()
{
	if ( impl )
	{
		delete impl;
		impl = NULL;
	}
}

Rar_Extractor::~Rar_Extractor() { close(); }

Rar_Extractor::error_t Rar_Extractor::open_()
{
	error_t err = reader->seek( 0 );
	if ( err )
		return err;
	
	impl = new Rar_Extractor_Impl( reader );
	if ( !impl )
		return "Out of memory";
	
	if ( setjmp( impl->jmp_env ) )
		return "Out of memory";
	
	impl->Unp.Init( NULL );
	
	if ( !impl->arc.IsArchive() )
	{
		close_();
		return not_archive_err;
	}
	
	err = impl->arc.IsArchive2();
	if ( err )
	{
		close_();
		return err;
	}
	
	done_ = false;
	scan_only_ = 0;
	return next_item();
}

Rar_Extractor::error_t Rar_Extractor::rewind()
{
	assert( impl );
	close_();
	return open_();
}

Rar_Extractor::error_t Rar_Extractor::next_item()
{
	for ( ;; impl->arc.SeekToNext() )
	{
		error_t err = impl->arc.ReadHeader();
		if ( err == end_of_rar )
		{
			done_ = true;
			break;
		}
		if ( err )
			return err;
		
		HEADER_TYPE type = (HEADER_TYPE) impl->arc.GetHeaderType();
		if ( type == ENDARC_HEAD )
		{
			// no files beyond this point
			done_ = true;
			break;
		}
		
		// skip non-files
		if ( type != FILE_HEAD )
		{
			#ifdef dprintf
				if ( type != NEWSUB_HEAD )
					dprintf( "Skipping unknown RAR block type: %X\n", (int) type );
			#endif
			continue;
		}
		
		// skip label
		if ( impl->arc.NewLhd.HostOS <= HOST_WIN32 && (impl->arc.NewLhd.FileAttr & 8) )
			continue;
		
		// skip links
		if ( (impl->arc.NewLhd.FileAttr & 0xF000) == 0xA000 )
			continue;
		
		// skip directories
		if ( (impl->arc.NewLhd.Flags & LHD_WINDOWMASK) == LHD_DIRECTORY )
			continue;
		
		name_ = impl->arc.NewLhd.FileName;
		size_ = impl->arc.NewLhd.UnpSize;
		timestamp_ = dostime_to_timestamp(impl->arc.NewLhd.FileTime);
		break;
	}
	
	extracted = false;
	return 0;
}

Rar_Extractor::error_t Rar_Extractor::next()
{
	if ( setjmp( impl->jmp_env ) )
		return "Out of memory";
	
	if ( !extracted && !scan_only_ && impl->arc.Solid )
	{
		// must extract every file in a solid archive
		Null_Writer out;
		error_t err = impl->extract( out, false );
		if ( err )
			return err;
	}
	
	impl->arc.SeekToNext();
	return next_item();
}
	
Rar_Extractor::error_t Rar_Extractor::extract( Data_Writer& out )
{
	if ( setjmp( impl->jmp_env ) )
		return "Out of memory";
	
	assert( !extracted ); // can't extract twice
	assert( !scan_only_ || impl->arc.Solid ); // can't extract solid when scanning only
	extracted = true;
	
	impl->write_error = 0;
	error_t err = impl->extract( out );
	if ( !err )
		err = impl->write_error;
	return err;
}
	
// Rar_Extractor_Impl

void rar_out_of_memory( Rar_Error_Handler* eh )
{
	longjmp( eh->jmp_env, 1 );
}

Rar_Extractor_Impl::Rar_Extractor_Impl( File_Reader* in ) :
	arc( in, this ),
	Unp( this, this ),
	Buffer( this )
{
	first_file = true;
	InitCRC();
}

int Rar_Extractor_Impl::UnpRead( byte* out, uint count )
{
	if ( count <= 0 )
		return 0;
	
	if ( count > UnpPackedSize )
		count = UnpPackedSize;
	
	int result = arc.Read( out, count );
	if ( result >= 0 )
		UnpPackedSize -= result;
	return result;
}

void Rar_Extractor_Impl::UnpWrite( byte const* out, uint count )
{
	if ( !write_error )
		write_error = writer->write( out, count );
	if ( MaintainCRC )
	{
		if ( arc.OldFormat )
			UnpFileCRC = OldCRC( (ushort) UnpFileCRC, out, count );
		else
			UnpFileCRC = CRC( UnpFileCRC, out, count );
	}
}

void Rar_Extractor_Impl::UnstoreFile( Int64 DestUnpSize )
{
	Buffer.Alloc( 0x10000 );
	while ( true )
	{
		unsigned int result = UnpRead( &Buffer[0], Buffer.Size() );
		if ( result == 0 || (int) result == -1 )
			break;
		
		result = (result < DestUnpSize ? result : int64to32( DestUnpSize ));
		UnpWrite( &Buffer[0], result );
		if ( DestUnpSize >= 0 )
			DestUnpSize -= result;
	}
	Buffer.Reset();
}

const char* Rar_Extractor_Impl::extract( Data_Writer& rar_writer, bool check_crc )
{
	assert( arc.GetHeaderType() == FILE_HEAD );
	
	if ( arc.NewLhd.Flags & (LHD_SPLIT_AFTER | LHD_SPLIT_BEFORE) )
		return "Segmented RAR not supported";
	
	if ( arc.NewLhd.Flags & LHD_PASSWORD )
		return "Encrypted RAR archive not supported";
	
	if ( arc.NewLhd.UnpVer < 13 || arc.NewLhd.UnpVer > UNP_VER )
		return "RAR file uses an unsupported format (too old or too recent)";
	
	arc.Seek( arc.NextBlockPos - arc.NewLhd.FullPackSize );
	
	UnpFileCRC = arc.OldFormat ? 0 : 0xFFFFFFFF;
	PackedCRC = 0xFFFFFFFF;
	UnpPackedSize = arc.NewLhd.FullPackSize;
	MaintainCRC = check_crc;
	this->writer = &rar_writer;
	
	if ( arc.NewLhd.Method == 0x30 )
	{
		UnstoreFile( arc.NewLhd.FullUnpSize );
	}
	else
	{
		Unp.SetDestSize( arc.NewLhd.FullUnpSize );
		if ( arc.NewLhd.UnpVer <= 15 )
			Unp.DoUnpack( 15, arc.Solid && !first_file );
		else
			Unp.DoUnpack( arc.NewLhd.UnpVer, arc.NewLhd.Flags & LHD_SOLID);
	}
	
	first_file = false;
	
	arc.SeekToNext();
	
	if ( check_crc )
	{
		unsigned long correct = arc.NewLhd.FileCRC;
		if ( !arc.OldFormat )
			correct = ~correct;
		
		if ( (UnpFileCRC ^ correct) & 0xFFFFFFFF )
			return "CRC error";
	}
	
	return NULL;
}

