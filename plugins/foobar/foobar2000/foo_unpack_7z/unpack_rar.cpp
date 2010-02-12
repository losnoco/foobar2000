#include <foobar2000.h>

#include <fex/Rar_Extractor.h>

#include "file_interface.h"
#include "timefn.h"

static void handle_error( const char * str )
{
	if ( str )
		throw exception_io_data( str );
}

class archive_rar : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "rar";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		foobar_File_Reader in( m_file, p_abort );
		Rar_Extractor ex;
		handle_error( ex.open( &in ) );
		ex.scan_only();
		while ( ! ex.done() )
		{
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
		Rar_Extractor ex;
		handle_error( ex.open( &in ) );
		while ( ! ex.done() )
		{
			if ( ! strcmp( ex.name(), p_file ) ) break;
			handle_error( ex.next() );
		}
		if ( ex.done() ) throw exception_io_not_found();
		filesystem::g_open_tempmem( p_out, p_abort );
		foobar_Data_Writer out( p_out, p_abort );
		handle_error( ex.extract( out ) );
		p_out->reopen( p_abort );
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "rar" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

		foobar_File_Reader in( m_file, p_out );
		Rar_Extractor ex;
		handle_error( ex.open( &in ) );
		if ( ! p_want_readers ) ex.scan_only();
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

class unpacker_rar : public unpacker
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
		Rar_Extractor ex;
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

static archive_factory_t < archive_rar >  g_archive_rar_factory;
static unpacker_factory_t< unpacker_rar > g_unpacker_rar_factory;

DECLARE_COMPONENT_VERSION("RAR reader", "1.1", "" );