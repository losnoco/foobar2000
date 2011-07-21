#define MY_VERSION "1.4"

/*
	changelog

2011-07-21 03:05 UTC - kode54
- Fixed file modification timestamp reporting
- Version is now 1.4

2010-03-19 16:59 UTC - kode54
- Updated zlib to version 1.2.4
- Version is now 1.3

2010-01-14 01:47 UTC - kode54
- Fixed componentversion about message declaration
- Version is now 1.2

2009-10-31 12:39 UTC - kode54
- Updated to File_Extractor v1.0.0
- Version is now 1.1

2009-08-13 21:27 UTC - kode54
- Implemented simple gzip stream length checking

*/

#include <foobar2000.h>

#include <fex/Gzip_Reader.h>

#include "file_interface.h"

#include "file_buffer.h"

static void handle_error( const char * str )
{
	if ( str ) throw exception_io_data( str );
}

static t_filesize streamSize( const service_ptr_t< file > & src_fd, abort_callback & p_abort )
{
	foobar_File_Reader in( src_fd, p_abort );
	Gzip_Reader ingz;
	handle_error( ingz.open( &in ) );

	if ( ! ingz.deflated() ) throw exception_io_data( "Not GZIP data" );

	return ingz.remain();
}

static void uncompressStream( const service_ptr_t< file > & src_fd, service_ptr_t< file > & dst_fd, abort_callback & p_abort )
{
	foobar_File_Reader in( src_fd, p_abort );
	Gzip_Reader ingz;
	handle_error( ingz.open( &in ) );

	if ( ! ingz.deflated() ) throw exception_io_data( "Not GZIP data" );

	while ( ingz.remain() )
	{
		unsigned char buffer[1024];

		BOOST::uint64_t to_read = ingz.remain();
		if ( to_read > 1024 ) to_read = 1024;
		handle_error( ingz.read( buffer, (long)to_read ) );
		dst_fd->write( buffer, (t_size)to_read, p_abort );
	}
}

class archive_gz : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "gz";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file, m_temp;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );

		t_filestats ret = m_file->get_stats( p_abort );
		ret.m_size = streamSize( m_file, p_abort );

		return ret;
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		p_out = new service_impl_t<file_buffer>( m_file->get_timestamp( p_abort ) );
		uncompressStream( m_file, p_out, p_abort );
		p_out->reopen( p_abort );
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "gz" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

		pfc::string8 m_path;
		service_ptr_t<file> m_out_file;
		t_filestats m_stats;

		make_unpack_path( m_path, path, pfc::string_filename( path ) );

		m_stats.m_timestamp = m_file->get_timestamp( p_out );

		if ( p_want_readers )
		{
			m_out_file = new service_impl_t<file_buffer>( m_stats.m_timestamp );
			uncompressStream( m_file, m_out_file, p_out );
			m_out_file->reopen( p_out );

			m_stats.m_size = m_out_file->get_size_ex( p_out );
		}
		else
		{
			m_stats.m_size = streamSize( m_file, p_out );
		}

		p_out.on_entry( this, m_path, m_stats, m_out_file );
	}
};

class unpacker_gz : public unpacker
{
public:
	virtual void open( service_ptr_t< file > & p_out, const service_ptr_t< file > & p_source, abort_callback & p_abort )
	{
		if ( p_source.is_empty() ) throw exception_io_data();

		p_out = new service_impl_t<file_buffer>( p_source->get_timestamp( p_abort ) );

		uncompressStream( p_source, p_out, p_abort );

		p_out->reopen( p_abort );
	}
};

static archive_factory_t< archive_gz >   g_archive_gz_factory;
static unpacker_factory_t< unpacker_gz > g_unpacker_gz_factory;

//DECLARE_COMPONENT_VERSION( "GZIP reader", MY_VERSION, "" );