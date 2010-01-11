#define MY_VERSION "1.0"

/*
	changelog

2009-08-14 00:17 UTC - kode54
- Disabled pointless unpacker service

2009-08-13 21:27 UTC - kode54
- tar_parser now caches file size and timestamp

2009-08-12 01:33 UTC - kode54
- Version is now 1.0

*/

#include <foobar2000.h>

#include <string.h>

static const char base_64_digits[64] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static char base_64_map[256];

static struct init_base64_map
{
	init_base64_map()
	{
		memset( base_64_map, 64, sizeof( base_64_map ) );
		for ( unsigned i = 0; i < 64; i++ )
		{
			base_64_map [ base_64_digits[ i ] ] = i;
		}
	}
} ibm;

t_filetimestamp UnixTimeToFileTime( time_t sec )
{
	const t_filetimestamp SECS_BETWEEN_EPOCHS = 11644473600;
	const t_filetimestamp SECS_TO_100NS = 10000000;

	t_filetimestamp Ret;

	Ret = ( ( t_filetimestamp ) sec + SECS_BETWEEN_EPOCHS ) * SECS_TO_100NS;

	return Ret;
}

class tar_parser
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

	pfc::string8_fastalloc      filename;
	t_filestats                 stats;

	struct header_t
	{
		char filename [100]; // 0
		char mode [8];       // 100
		char oid [8];        // 108
		char gid [8];        // 116
		char size [12];      // 124
		char mtime [12];     // 136
		char chksum [8];     // 148
		char type;           // 156
		char linkname [100]; // 157

		// UStar
		char magic [6];      // 257
		char version [2];    // 263
		char oname [32];     // 265
		char gname [32];     // 297
		char devmajor [8];   // 329
		char devminor [8];   // 337
		char prefix [155];   // 345

		// Pad to 512 bytes
		char padding [12];   // 500
	};

	header_t header;

	t_filesize next_header;

	t_filesize parse_number( const char * in, unsigned size )
	{
		const char * ptr = in;
		const char * end = in + size;

		// Handle null overflow from previous field, written by crappy version of tar
		ptr += !*ptr;

		for (;;)
		{
			if ( ptr == end ) return ~0;
			if ( *ptr != ' ' ) break;
			ptr++;
		}

		t_filesize value = 0;

		if ( ( unsigned ) *ptr - '0' <= 7 )
		{
			// Octal string
			const char * ptr1 = ptr;

			for (;;)
			{
				value += *ptr++ - '0';
				if ( ptr == end || ( ( unsigned ) *ptr - '0' > 7 ) ) break;
				value <<= 3;
			}
		}
		else if ( *ptr == '+' || *ptr == '-' )
		{
			// Base64 encoded from tar test versions 1.13.6 (1999-08-11) through 1.13.11 (1999-08-23)
			bool negative = *ptr++ == '-';
			int digit;
			while ( ptr != end && ( digit = base_64_map[ (unsigned char) *ptr ] ) < 64 )
			{
				value = ( value << 6 ) + digit;
				ptr++;
			}
			if ( negative ) value = -value;
		}
		else if ( *ptr == '\200' || *ptr == '\377' )
		{
			// Base256 encoded
			int signbit = *ptr & ( 1 << 6 );
			value = ( *ptr++ & ( ( 1 << 6 ) - 1 ) ) - signbit;
			for (;;)
			{
				value = ( value << 8 ) + ( unsigned char ) *ptr++;
				if ( ptr == end ) break;
			}
		}

		return value;
	}

	void verify_checksum()
	{
		unsigned char * ptr = ( unsigned char * ) &header;

		unsigned i;

		int signed_checksum;
		int unsigned_checksum;

		signed_checksum = unsigned_checksum = 0x20 * 8;

		for ( i = 0; i < offsetof( header_t, chksum ); i++ )
		{
			unsigned_checksum += ptr [i];
			signed_checksum += ( signed char ) ptr [i];
		}
		for ( i = offsetof( header_t, type ); i < 512; i++ )
		{
			unsigned_checksum += ptr [i];
			signed_checksum += ( signed char ) ptr [i];
		}

		t_filesize checksum = parse_number( header.chksum, sizeof( header.chksum ) );
		if ( unsigned_checksum != checksum && signed_checksum != checksum ) throw exception_io_data();
	}

public:
	tar_parser( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort )
	{
		reset();
	}

	void reset()
	{
		next_header = 0;
	}

	bool next()
	{
		unsigned blank_count = 0;
		//header_t blank = {0};

		bool long_filename = false;

		while ( blank_count < 2 )
		{
			m_file->seek( next_header, m_abort );
			m_file->read_object( &header, sizeof( header ), m_abort );
			next_header += sizeof( header );
			if ( header.filename [0] != 0 /*memcmp( &header, &blank, sizeof( header ) )*/ )
			{
				verify_checksum();

				if ( !long_filename ) filename.set_string( header.filename, sizeof( header.filename ) );
				stats.m_size = parse_number( header.size, sizeof( header.size ) );
				stats.m_timestamp = UnixTimeToFileTime( parse_number( header.mtime, sizeof( header.mtime ) ) );

				t_filesize size = get_size();
				size = ( size + 511 ) & ~511;
				next_header += size;

				if ( header.type == '0' || header.type == 0 ) break;

				if ( header.type == 'L' )
				{
					long_filename = true;
					m_file->read_string_ex( filename, get_size(), m_abort );
				}

				continue;
			}
			++blank_count;
		}

		if ( blank_count == 2 ) return false;

		return true;
	}

	inline const char * get_filename() const
	{
		return filename;
	}

	inline t_filesize get_size() const
	{
		return stats.m_size;
	}

	inline t_filetimestamp get_timestamp() const
	{
		return stats.m_timestamp;
	}

	inline t_filestats get_stats() const
	{
		return stats;
	}
};

class archive_tar : public archive_impl
{
public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "tar";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		tar_parser ex( m_file, p_abort );
		while ( ex.next() )
		{
			if ( ! strcmp( ex.get_filename(), p_file ) ) return ex.get_stats();
		}
		throw exception_io_not_found();
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
		service_ptr_t< file > m_file;
		filesystem::g_open( m_file, p_archive, filesystem::open_mode_read, p_abort );
		tar_parser ex( m_file, p_abort );
		while ( ex.next() )
		{
			if ( ! strcmp( ex.get_filename(), p_file ) )
			{
				filesystem::g_open_tempmem( p_out, p_abort );
				file::g_transfer_object( m_file, p_out, ex.get_size(), p_abort );
				p_out->reopen( p_abort );
				return;
			}
		}
		throw exception_io_not_found();
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		if ( stricmp_utf8( pfc::string_extension( path ), "tar" ) )
			throw exception_io_data();

		service_ptr_t< file > m_file = p_reader;
		if ( m_file.is_empty() )
			filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

		tar_parser ex( m_file, p_out );

		pfc::string8_fastalloc m_path;
		service_ptr_t<file> m_out_file;
		while ( ex.next() )
		{
			make_unpack_path( m_path, path, ex.get_filename() );
			if ( p_want_readers )
			{
				filesystem::g_open_tempmem( m_out_file, p_out );
				file::g_transfer_object( m_file, m_out_file, ex.get_size(), p_out );
				m_out_file->reopen( p_out );
			}
			if ( ! p_out.on_entry( this, m_path, ex.get_stats(), m_out_file ) ) break;
		}
	}
};

/*class unpacker_tar : public unpacker
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

		tar_parser ex( p_source, p_abort );
		while ( ex.next() )
		{
			if ( ! skip_ext( ex.get_filename() ) )
			{
				filesystem::g_open_tempmem( p_out, p_abort );
				file::g_transfer_object( p_source, p_out, ex.get_size(), p_abort );
				p_out->reopen( p_abort );
				return;
			}
		}
		throw exception_io_data();
	}
};*/

static archive_factory_t < archive_tar >  g_archive_tar_factory;
//static unpacker_factory_t< unpacker_tar > g_unpacker_tar_factory;

DECLARE_COMPONENT_VERSION( "TAR reader", MY_VERSION, (const char*)NULL );