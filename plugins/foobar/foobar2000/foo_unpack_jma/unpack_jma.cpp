#define MY_VERSION "1.5"

/*
	changelog

2011-07-21 02:45 UTC - kode54
- Fixed file modification timestamp reporting
- Version is now 1.5

2010-11-14 02:48 UTC - kode54
- Fixed invalid pointer reference in *_FileSet class used by archive_list function
- Version is now 1.4

2010-01-14 01:45 UTC - kode54
- Fixed componentversion about message declaration
- Version is now 1.3

2010-01-11 19:40 UTC - kode54
- Added filename validator
- Version is now 1.2

2009-04-21 21:39 UTC - kode54
- Attempts to query missing files now correctly throws exception_io_not_found
- Version is now 1.1

*/

#include <foobar2000.h>

#include "jma/jma.h"

class archive_jma : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "jma";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		JMA::jma_open JMAFile( m_file, p_abort );
		return JMAFile.get_stats_in_archive( p_file, p_abort );
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		JMA::jma_open JMAFile( m_file, p_abort );
		JMAFile.open_archive( p_out, p_file, p_abort );
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "jma" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

		JMA::jma_open JMAFile( m_file, p_out );
		JMAFile.archive_list( this, path, p_out, p_want_readers );
	}
};

class unpacker_jma : public unpacker, private archive_callback
{
	abort_callback * m_abort;

	service_ptr_t< file > m_out;

	virtual bool is_aborting() const
	{
		return m_abort->is_aborting();
	}

	virtual abort_callback_event get_abort_event() const
	{
		return m_abort->get_abort_event();
	}

	virtual bool on_entry( archive * owner, const char * url, const t_filestats & p_stats, const service_ptr_t<file> & p_reader )
	{
		static const char * exts[] = { "txt", "nfo", "info", "diz" };
		pfc::string_extension ext( url );
		for ( unsigned n = 0; n < tabsize( exts ); ++n )
		{
			if ( ! stricmp_utf8( ext, exts[ n ] ) ) return true;
		}

		m_out = p_reader;
		return false;
	}

public:
	virtual void open( service_ptr_t< file > & p_out, const service_ptr_t< file > & p_source, abort_callback & p_abort )
	{
		if ( p_source.is_empty() ) throw exception_io_data();

		JMA::jma_open JMAFile( p_source, p_abort );

		m_abort = &p_abort;

		JMAFile.archive_list( 0, "", *this, true );

		if ( m_out.is_empty() ) throw exception_io_data();

		p_out = m_out;
	}
};

static archive_factory_t < archive_jma >  g_archive_jma_factory;
static unpacker_factory_t< unpacker_jma > g_unpacker_jma_factory;

DECLARE_COMPONENT_VERSION( "JMA reader", MY_VERSION, "" );

VALIDATE_COMPONENT_FILENAME("foo_unpack_jma.dll");
