#include "base.h"
#include "reader.h"
#include "config.h"

#include <Vgm_Emu.h>

static long get_le32( const uint8_t b [4] ) {
	return b [3] * 0x1000000L + b [2] * 0x10000L + b [1] * 0x100L + b [0];
}

static const char tag_track_name[]    = "title";
static const char tag_track_name_e[]  = "title_e";
static const char tag_track_name_j[]  = "title_j";
static const char tag_game_name[]     = "album";
static const char tag_game_name_e[]   = "album_e";
static const char tag_game_name_j[]   = "album_j";
static const char tag_system_name[]   = "system";
static const char tag_system_name_e[] = "system_e";
static const char tag_system_name_j[] = "system_j";
static const char tag_artist[]        = "artist";
static const char tag_artist_e[]      = "artist_e";
static const char tag_artist_j[]      = "artist_j";
static const char tag_date[]          = "date";
static const char tag_ripper[]        = "ripper";
static const char tag_notes[]         = "comment";

class input_vgm : public input_gep
{
	Vgm_Emu::header_t m_header;

	const wchar_t * get_string( const wchar_t * & ptr, unsigned & wchar_remain )
	{
		while ( * ptr && wchar_remain ) ++ptr, --wchar_remain;
		if ( wchar_remain > 1 )
		{
			--wchar_remain;
			return ++ptr;
		}

		return 0;
	}

	void add_tag( file_info & p_info, const char * name, const wchar_t * value )
	{
		if ( value && * value )
		{
			const wchar_t * ptr_lf = wcschr( value, '\n' );
			const wchar_t * ptr_cr = wcschr( value, '\r' );
			if ( ptr_lf || ptr_cr )
			{
				string8 temp;
				do
				{
					if ( ptr_cr && ptr_lf && ptr_cr < ptr_lf ) ptr_lf = ptr_cr;
					if ( ptr_lf )
					{
						temp.add_string_utf16( value, ptr_lf - value );
						temp += "\r\n";
						value = ptr_lf;
						if ( value[ 0 ] == '\r' && value[ 1 ] == '\n' ) value += 2;
						else ++value;
						ptr_lf = wcschr( value, '\n' );
						ptr_cr = wcschr( value, '\r' );
					}
				}
				while ( * value && ( ptr_lf || ptr_cr ) );

				if ( * value ) temp.add_string_utf16( value );

				p_info.meta_add( name, temp );
			}
			else
				p_info.meta_add( name, string_utf8_from_utf16( value ) );
		}
	}

	void process_gd3_tag( const byte * tag, unsigned size, file_info & p_info )
	{
		static const unsigned char signature[] = { 'G', 'd', '3', ' ' };
		static const unsigned char version[]   = {   0,   1,   0,   0 };

		if ( size >= 12 && ! memcmp( tag, signature, 4 ) && ! memcmp( tag + 4, version, 4 ) )
		{
			unsigned data_size = tag[ 8 ] | ( tag[ 9 ] << 8 ) | ( tag[ 10 ] << 16 ) | ( tag[ 11 ] << 24 );
			if ( data_size && size >= data_size + 12 )
			{
				const wchar_t * ptr = ( const wchar_t * ) ( tag + 12 );
				unsigned wchar_remain = data_size / 2;

				const wchar_t * track_name_e  = ptr;
				const wchar_t * track_name_j  = get_string( ptr, wchar_remain );
				const wchar_t * game_name_e   = get_string( ptr, wchar_remain );
				const wchar_t * game_name_j   = get_string( ptr, wchar_remain );
				const wchar_t * system_name_e = get_string( ptr, wchar_remain );
				const wchar_t * system_name_j = get_string( ptr, wchar_remain );
				const wchar_t * artist_e      = get_string( ptr, wchar_remain );
				const wchar_t * artist_j      = get_string( ptr, wchar_remain );
				const wchar_t * date          = get_string( ptr, wchar_remain );
				const wchar_t * ripper        = get_string( ptr, wchar_remain );
				const wchar_t * notes         = get_string( ptr, wchar_remain );

				const wchar_t * track_name  = 0;
				const wchar_t * game_name   = 0;
				const wchar_t * system_name = 0;
				const wchar_t * artist      = 0;

				if ( cfg_vgm_gd3_prefers_japanese )
				{
					track_name  = ( track_name_j && * track_name_j )   ? track_name_j  : track_name_e;
					game_name   = ( game_name_j && * game_name_j )     ? game_name_j   : game_name_e;
					system_name = ( system_name_j && * system_name_j ) ? system_name_j : system_name_e;
					artist      = ( artist_j && * artist_j )           ? artist_j      : artist_e;
				}
				else
				{
					track_name  = ( track_name_e && * track_name_e )   ? track_name_e  : track_name_j;
					game_name   = ( game_name_e && * game_name_e )     ? game_name_e   : game_name_j;
					system_name = ( system_name_e && * system_name_e ) ? system_name_e : system_name_j;
					artist      = ( artist_e && * artist_e )           ? artist_e      : artist_j;
				}

				add_tag( p_info, tag_track_name, track_name );
				add_tag( p_info, tag_track_name_e, track_name_e );
				add_tag( p_info, tag_track_name_j, track_name_j );
				add_tag( p_info, tag_game_name, game_name );
				add_tag( p_info, tag_game_name_e, game_name_e );
				add_tag( p_info, tag_game_name_j, game_name_j );
				add_tag( p_info, tag_system_name, system_name );
				add_tag( p_info, tag_system_name_e, system_name_e );
				add_tag( p_info, tag_system_name_j, system_name_j );
				add_tag( p_info, tag_artist, artist );
				add_tag( p_info, tag_artist_e, artist_e );
				add_tag( p_info, tag_artist_j, artist_j );
				add_tag( p_info, tag_date, date );
				add_tag( p_info, tag_ripper, ripper );
				add_tag( p_info, tag_notes, notes );
			}
		}
	}

public:
	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "vgm" );
	}

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status = input_gep::open( p_filehint, p_path, p_reason, p_abort );
		if ( io_result_failed( status ) ) return status;


		foobar_File_Reader rdr(m_file, p_abort);

		try
		{
			ERRCHK( rdr.read( &m_header, sizeof(m_header) ) );

			if ( 0 != memcmp( m_header.tag, "Vgm ", 4 ) )
			{
				console::info("Not a VGM file");
				throw io_result_error_data;
			}
			if ( get_le32( m_header.vers ) > 0x0101 )
			{
				console::info("Unsupported VGM format");
				throw io_result_error_data;
			}
		}
		catch ( t_io_result code )
		{
			return code;
		}

		return io_result_success;
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set("codec", "VGM");

		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);

		Vgm_Emu * emu = ( Vgm_Emu * ) this->emu;
		if ( ! emu )
		{
			emu = new Vgm_Emu;
			if ( ! emu )
			{
				console::info("Out of memory");
				throw io_result_error_out_of_memory;
			}
			this->emu = emu;

			try
			{
				m_file->seek_e( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->init( sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
				ERRCHK( emu->start_track( 0 ) );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			m_file.release();
		}

		Vgm_Emu::track_data_t tdata = emu->track_data();

		if ( tdata.loop_end )
		{
			p_info.set_length( tdata.loop_end );
			p_info.info_set_float("vgm_loop_start", tdata.loop_start, 4);
			p_info.info_set_float("vgm_loop_end", tdata.loop_end, 4);
		}
		else p_info.set_length( tdata.length );

		if ( tdata.trailer && tdata.trailer_size )
		{
			process_gd3_tag( tdata.trailer, tdata.trailer_size, p_info );
		}

		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Vgm_Emu * emu = ( Vgm_Emu * ) this->emu;
		if ( ! emu )
		{
			emu = new Vgm_Emu;
			if ( ! emu )
			{
				console::info("Out of memory");
				throw io_result_error_out_of_memory;
			}
			this->emu = emu;

			try
			{
				m_file->seek_e( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->init( sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			m_file.release();
		}

		ERRCHK( emu->start_track( 0 ) );

		Vgm_Emu::track_data_t tdata = emu->track_data();

		played = 0;
		no_infinite = !cfg_indefinite || ( p_flags & input_flag_no_looping );

		if ( no_infinite )
		{
			song_len = int( ( tdata.loop_end ? tdata.loop_end : tdata.length ) * double( sample_rate ) );
			fade_len = 0;
		}

		subsong = 0;

		return io_result_success;
	}
};

DECLARE_FILE_TYPE("VGM files", "*.VGM");

static input_factory_t<input_vgm> g_input_vgm_factory;
