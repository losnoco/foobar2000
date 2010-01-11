#include <foobar2000.h>

#include "file_cached.h"

#include "unlha/unlha32.h"

extern const char *lha_methods[12];

#ifdef _DEBUG
void Log( LPCSTR fmt, ... )
{
	va_list list;
	va_start(list,fmt);
	uOutputDebugString( pfc::string_printf_va( fmt, list ) );
	va_end(list);
}
#endif

#define UNIX_FILE_TYPEMASK		0170000
#define UNIX_FILE_REGULAR		0100000

const t_filetimestamp SECS_BETWEEN_EPOCHS = 11644473600;
const t_filetimestamp SECS_TO_100NS = 10000000;

t_filetimestamp UnixTimeToFileTime( time_t sec )
{
	t_filetimestamp Ret;

	Ret = ( ( t_filetimestamp ) sec + SECS_BETWEEN_EPOCHS ) * SECS_TO_100NS;

	return Ret;
}

static bool is_hex( char c )
{
	if ( ( c >= '0' && c <= '9' ) ||
		( c >= 'A' && c <= 'F' ) ||
		( c >= 'a' && c <= 'f' ) ) return true;
	return false;
}

static int decode_hex( char c )
{
	if ( c >= '0' && c <= '9' )
		return c - '0';
	else if ( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
	else if ( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	else
		return -1;
}

static void convert_mess_to_utf8( const char * p_src, size_t p_src_len, pfc::string_base & p_dst )
{
	t_size src_len = strnlen( p_src, p_src_len );

	t_size n;
	for ( n = 0; n < src_len; ++n )
	{
		if ( p_src[ n ] == ':' && n + 2 < src_len )
		{
			if ( is_hex( p_src[ n + 1 ] ) && is_hex( p_src[ n + 2 ] ) )
			{
				pfc::string8 moo( p_src, n );
				for ( ; n < src_len; )
				{
					if ( p_src[ n ] == ':' && n + 2 < src_len && is_hex( p_src[ n + 1 ] ) && is_hex( p_src[ n + 2 ] ) )
					{
						int val = decode_hex( p_src[ n + 1 ] ) << 4;
						val += decode_hex( p_src[ n + 2 ] );
						if ( val >= 0x20 )
							moo.add_byte( val );
						else
							moo.add_string( p_src + n, 3 );
						n += 3;
					}
					else
					{
						moo.add_byte( p_src[ n ] );
						++n;
					}
				}
				p_dst = pfc::stringcvt::string_utf8_from_codepage( 932, moo ); // Shift-JIS
				return;
			}
		}
		if ( p_src[ n ] & 0x80 ) break;
	}
	if ( n == src_len )
	{
		p_dst = p_src;
		return;
	}

	UINT codepage;
	int ret = MultiByteToWideChar( codepage = 65001, MB_ERR_INVALID_CHARS, p_src, src_len, 0, 0 ); // UTF-8
	if ( ! ret && GetLastError() == ERROR_NO_UNICODE_TRANSLATION )
		ret = MultiByteToWideChar( codepage = 932, MB_ERR_INVALID_CHARS, p_src, src_len, 0, 0 ); // Shift-JIS
	if ( ! ret && GetLastError() == ERROR_NO_UNICODE_TRANSLATION )
		ret = MultiByteToWideChar( codepage = 936, MB_ERR_INVALID_CHARS, p_src, src_len, 0, 0 ); // GBK
	if ( ! ret && GetLastError() == ERROR_NO_UNICODE_TRANSLATION )
		ret = MultiByteToWideChar( codepage = 950, MB_ERR_INVALID_CHARS, p_src, src_len, 0, 0 ); // Big5
	if ( ! ret )
		codepage = pfc::stringcvt::codepage_system;

	if ( codepage == 65001 )
		p_dst = p_src;
	else
		p_dst = pfc::stringcvt::string_utf8_from_codepage( codepage, p_src );
}

class archive_lha_impl : private CLhaArchive
{
	pfc::array_t< t_uint8 > archive;

	abort_callback * m_abort;

	service_ptr_t< file > m_in, m_out;

	bool table_initialized;

	virtual int lharead( void * p, int sz1, int sz2 )
	{
		int sz = sz1 * sz2;
		if ( max( sz1, sz2 ) > sz ) return 0;
		if ( m_in.is_empty() || !m_abort ) return 0;
		sz = m_in->read( p, sz, *m_abort );
		return sz / sz1;
	}

	virtual void fwrite_crc( const void * p, int n )
	{
		crc = calccrc( crc, p, n );
		if ( m_out.is_valid() && m_abort ) m_out->write( p, n, *m_abort );
	}

protected:
	friend class archive_lha;
	friend class unpacker_lha;

	void walk_to_file( const char * p_path, LzHeader & hdr, abort_callback & p_abort )
	{
		if ( ! table_initialized )
		{
			table_initialized = true;
			make_crctable();
		}

		m_in->reopen( p_abort );

		//LzHeader hdr;

		pfc::string8_fastalloc temp;

		while ( get_header( &hdr ) )
		{
			p_abort.check();

			if ( ( hdr.unix_mode & UNIX_FILE_TYPEMASK ) == UNIX_FILE_REGULAR )
			{
				for ( unsigned i = 0, j = strnlen( hdr.name, sizeof( hdr.name ) ); i < j; ++i )
				{
					if ( hdr.name[ i ] == '\xFF' ) hdr.name[ i ] = '/';
				}
				convert_mess_to_utf8( hdr.name, sizeof( hdr.name ), temp );

				if ( ! strcmp( temp, p_path ) ) return;

				m_in->skip( hdr.packed_size, p_abort );
			}
		}

		throw exception_io_data();
	}

	//! Must be called immediately after successful call to walk_to_file or get_header
	void extract_file( LzHeader const& hdr, service_ptr_t< file > & p_out, abort_callback & p_abort )
	{
		// safety
		if ( ( hdr.unix_mode & UNIX_FILE_TYPEMASK ) != UNIX_FILE_REGULAR ) throw exception_io_data();

		if ( ! m_pDecoderData )
		{
			// huf.c needs 9344 bytes, dhuf.c needs about 12KB
			m_pDecoderData = new BYTE[16384]; // 16K of data - should be enough

			// Init misc tables
			InitDecodeTables();
			InitHufTables();
			//make_crctable();
		}

		int method;

		for ( method = 0; ; ++method )
		{
			if ( lha_methods[ method ] == NULL )
			{
				throw exception_io_data( pfc::string_formatter() << "unsupported compression method: " << pfc::string8( ( const char * ) &hdr.method, 5 ) );
			}

			if ( ! strncmp( hdr.method, lha_methods[ method ], 5 ) ) break;
		}

		crc = 0;

		t_filesize data_offset = m_in->get_position( p_abort );

		try
		{
			m_abort = &p_abort;
			m_out = p_out;

			{
				service_ptr_t< file > p_orig = m_in;

				try
				{
					service_ptr_t< file > p_temp;
					file_cached< 4096 >::g_create( p_temp, m_in, p_abort, hdr.packed_size );
					m_in = p_temp;
					decode_lzhuf( hdr.original_size, hdr.packed_size, method );
					m_in = p_orig;
				}
				catch (...)
				{
					m_in = p_orig;
				}
			}

			if ( hdr.has_crc && crc != hdr.crc )
				throw exception_io_data( "header crc mismatch" );

			t_sfilesize pos = m_in->get_position( p_abort );
			pos = data_offset + hdr.packed_size - pos;
			if ( pos )
				m_in->seek_ex( pos, file::seek_from_current, p_abort );

			m_out.release();
			m_abort = 0;

			p_out->reopen( p_abort );
		}
		catch (...)
		{
			m_out.release();
			m_abort = 0;

			throw;
		}
	}

	t_filestats get_stats_in_archive( LzHeader const& hdr )
	{
		t_filestats ret;

		ret.m_size = hdr.original_size;
		ret.m_timestamp = UnixTimeToFileTime( hdr.unix_last_modified_stamp );

		return ret;
	}

public:
	archive_lha_impl() : table_initialized( false ) {}

protected:
	t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		filesystem::g_open( m_in, p_archive, filesystem::open_mode_read, p_abort );

		try
		{
			m_abort = &p_abort;

			LzHeader hdr;

			walk_to_file( p_file, hdr, p_abort );

			return get_stats_in_archive( hdr );
		}
		catch (...)
		{
			m_abort = 0;
			throw;
		}
	}

	void open_archive( service_ptr_t< file > & p_out, const char * archive, const char * file, abort_callback & p_abort )
	{
		filesystem::g_open( m_in, archive, filesystem::open_mode_read, p_abort );

		try
		{
			LzHeader hdr;

			m_abort = &p_abort;

			walk_to_file( file, hdr, p_abort );

			filesystem::g_open_tempmem( p_out, p_abort );

			extract_file( hdr, p_out, p_abort );
		}
		catch (...)
		{
			m_abort = 0;
			throw;
		}
	}

	void archive_list( foobar2000_io::archive * p_owner, const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( p_reader.is_valid() )
		{
			m_in = p_reader;
			m_in->reopen( p_out );
		}
		else filesystem::g_open( m_in, path, filesystem::open_mode_read, p_out );

		if ( ! table_initialized )
		{
			table_initialized = true;
			make_crctable();
		}

		try
		{
			m_abort = &p_out;

			bool item_found = false;

			LzHeader hdr;

			pfc::string8_fastalloc temp, tempname;

			while ( get_header( &hdr ) )
			{
				p_out.check();

				item_found = true;

				if ( ( hdr.unix_mode & UNIX_FILE_TYPEMASK ) == UNIX_FILE_REGULAR )
				{
					t_filestats stats = get_stats_in_archive( hdr );

					service_ptr_t< file > p_file;

					if ( p_want_readers )
					{
						filesystem::g_open_tempmem( p_file, p_out );

						t_filesize data_position = m_in->get_position( p_out );

						extract_file( hdr, p_file, p_out );

						t_filesize pos = m_in->get_position( p_out );
						pos = data_position + hdr.packed_size - pos;
						if ( pos ) m_in->skip( pos, p_out );
					}
					else m_in->skip( hdr.packed_size, p_out );

					for ( unsigned i = 0, j = strnlen( hdr.name, sizeof( hdr.name ) ); i < j; ++i )
					{
						if ( hdr.name[ i ] == '\xFF' ) hdr.name[ i ] = '/';
					}
					convert_mess_to_utf8( hdr.name, sizeof( hdr.name ), tempname );

					archive_impl::g_make_unpack_path( temp, path, tempname, "lha" );

					if ( ! p_out.on_entry( p_owner, temp, stats, p_file ) ) break;
				}
			}

			if ( ! item_found ) throw exception_io_data();
		}
		catch (...)
		{
			m_abort = 0;
			throw;
		}
	}
};

class archive_lha : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "lha";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		return archive_lha_impl().get_stats_in_archive( p_archive, p_file, p_abort );
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * archive, const char * file, abort_callback & p_abort )
	{
		archive_lha_impl().open_archive( p_out, archive, file, p_abort );
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		{
			pfc::string_extension ext( path );
			if ( stricmp_utf8( ext, "lha" ) && stricmp_utf8( ext, "lzh" ) )
				throw exception_io_data();
		}

		archive_lha_impl().archive_list( this, path, p_reader, p_out, p_want_readers );
	}
};

class unpacker_lha : public unpacker, private archive_callback
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

		archive_lha_impl m_archive;

		m_abort = &p_abort;

		m_archive.archive_list( 0, "", p_source, *this, true );

		if ( m_out.is_empty() ) throw exception_io_data();

		p_out = m_out;
	}
};

static archive_factory_t < archive_lha >  g_archive_lha_factory;
static unpacker_factory_t< unpacker_lha > g_unpacker_lha_factory;

DECLARE_COMPONENT_VERSION( "LHA unpacker", "0.1", (const char*)NULL );
