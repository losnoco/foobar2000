#define MY_VERSION "1.7"

/*
	changelog

2011-08-09 01:17 UTC - kode54
- Decompression now pre-allocates the output file buffers rather than expanding them
  gradually
- Version is now 1.7

2011-07-21 02:20 UTC - kode54
- Fixed archive file timestamp reporting
- Version is now 1.6

2010-12-06 08:38 UTC - kode54
- Updated File_Extractor to LZMA SDK v9.20 to support more filters, LZMA2, and PPMd
- Version is now 1.5

2010-01-14 01:45 UTC - kode54
- Fixed componentversion about message declaration
- Version is now 1.4

2010-01-11 19:39 UTC - kode54
- Added filename validator
- Version is now 1.3

2009-10-31 12:39 UTC - kode54
- Updated to File_Extractor v1.0.0
- Version is now 1.2

2009-04-21 21:40 UTC - kode54
- Attempts to query missing files now correctly throws exception_io_not_found
- Version is now 1.1

*/

#include <foobar2000.h>

#include <fex/Zip7_Extractor.h>

#include "file_buffer.h"
#include "file_interface.h"
#include "timefn.h"

static void handle_error( const char * str )
{
	if ( str ) throw exception_io_data( str );
}

static void transfer_file( Data_Reader & in, service_ptr_t<file> & out, abort_callback & p_abort )
{
	char buffer[1024];
	int read;
	for (;;)
	{
		read = 1024;
		in.read_avail( buffer, &read );
		out->write_object( buffer, read, p_abort );
		if ( read < 1024 ) break;
	}
}

class archive_7z : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "7z";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		foobar_File_Reader in( m_file, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_not_found();
		t_filestats ret;
		ret.m_size = ex.size();
		ret.m_timestamp = dostime_to_timestamp( ex.dos_date() );
		return ret;
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		foobar_File_Reader in( m_file, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_not_found();
		p_out = new service_impl_t<file_buffer>( ex.size(), dostime_to_timestamp( ex.dos_date() ) );
		transfer_file( ex.reader(), p_out, p_abort );
		p_out->reopen( p_abort );
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "7z" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

		foobar_File_Reader in( m_file, p_out );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		// if ( ! p_want_readers ) ex.scan_only(); // this is only needed for Rar_Extractor
		pfc::string8_fastalloc m_path;
		service_ptr_t<file> m_out_file;
		t_filestats m_stats;
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			make_unpack_path( m_path, path, ex.name() );
			m_stats.m_size = ex.size();
			m_stats.m_timestamp = dostime_to_timestamp( ex.dos_date() );
			if ( p_want_readers )
			{
				m_out_file = new service_impl_t<file_buffer>( m_stats.m_size, m_stats.m_timestamp );
				transfer_file( ex.reader(), m_out_file, p_out );
				m_out_file->reopen( p_out );
			}
			if ( ! p_out.on_entry( this, m_path, m_stats, m_out_file ) ) break;
			handle_error( ex.next() );
		}
	}
};

class unpacker_7z : public unpacker
{
	inline bool skip_ext( const char * p )
	{
		static const char * exts[] = { "txt", "nfo", "info", "diz" };
		pfc::string_extension ext( p );
		for ( unsigned n = 0; n < tabsize( exts ); ++n )
		{
			if ( ! stricmp_utf8( ext, exts[ n ] ) ) return true;
		}
		return false;
	}

public:
	virtual void open( service_ptr_t< file > & p_out, const service_ptr_t< file > & p_source, abort_callback & p_abort )
	{
		if ( p_source.is_empty() ) throw exception_io_data();

		foobar_File_Reader in( p_source, p_abort );
		Zip7_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			handle_error( ex.stat() );
			if ( ! skip_ext( ex.name() ) )
			{
				p_out = new service_impl_t<file_buffer>( ex.size(), dostime_to_timestamp( ex.dos_date() ) );
				transfer_file( ex.reader(), p_out, p_abort );
				p_out->reopen( p_abort );
				return;
			}
			handle_error( ex.next() );
		}
		throw exception_io_data();
	}
};

static archive_factory_t < archive_7z >  g_archive_7z_factory;
static unpacker_factory_t< unpacker_7z > g_unpacker_7z_factory;

DECLARE_COMPONENT_VERSION( "7-Zip reader", MY_VERSION, "" );

VALIDATE_COMPONENT_FILENAME("foo_unpack_7z.dll");
