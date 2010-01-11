#include <foobar2000.h>

#include <fex/Zip7_Extractor.h>

#include "timefn.h"

class foobar_File_Reader : public File_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

public:
	foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort ) { }
	virtual ~foobar_File_Reader() { }
	
	// Read at most 'n' bytes. Return number of bytes read, zero or negative
	// if error.
	virtual long read_avail( void* p, long n )
	{
		try
		{
			return m_file->read( p, n, m_abort );
		}
		catch (...)
		{
			return -1;
		}
	}
	
	virtual uint64_t remain() const
	{
		t_filesize remain = 0;
		try
		{
			remain = m_file->get_size( m_abort ) - m_file->get_position( m_abort );
		}
		catch (...) {}
		return remain;
	}

	virtual uint64_t size() const
	{
		t_filesize size = 0;
		try
		{
			size = m_file->get_size_ex( m_abort );
		}
		catch (...) {}
		return size;
	}
	
	virtual uint64_t tell() const
	{
		t_filesize position = 0;
		try
		{
			position = m_file->get_position( m_abort );
		}
		catch (...) {}
		return position;
	}
	
	virtual error_t seek( long offset )
	{
		try
		{
			m_file->seek( offset, m_abort );
			return 0;
		}
		catch (...)
		{
			return "Seek error";
		}
	}
};

class foobar_Data_Writer : public Data_Writer
{
	service_ptr_t<file> & m_file;
	abort_callback      & m_abort;
public:
	foobar_Data_Writer( service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort ) { }
	virtual ~foobar_Data_Writer() { }
	
	virtual error_t write( const void* p, long n )
	{
		try
		{
			m_file->write( p, n, m_abort );
			return 0;
		}
		catch (...)
		{
			return "Write error";
		}
	}
};

void handle_error( const char * str )
{
	if ( str ) throw exception_io_data( str );
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
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_data();
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
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_data();
		filesystem::g_open_tempmem( p_out, p_abort );
		foobar_Data_Writer out( p_out, p_abort );
		handle_error( ex.extract( out ) );
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
			make_unpack_path( m_path, path, ex.name() );
			m_stats.m_size = ex.size();
			m_stats.m_timestamp = dostime_to_timestamp( ex.dos_date() );
			if ( p_want_readers )
			{
				filesystem::g_open_tempmem( m_out_file, p_out );
				foobar_Data_Writer out( m_out_file, p_out );
				handle_error( ex.extract( out ) );
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
			if ( ! skip_ext( ex.name() ) )
			{
				filesystem::g_open_tempmem( p_out, p_abort );
				foobar_Data_Writer out( p_out, p_abort );
				ex.extract( out );
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

DECLARE_COMPONENT_VERSION( "7-Zip unpacker", "1.0", (const char*)NULL );