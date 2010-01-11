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
public:
	static inline bool g_test_filename(const char * full_path, const char * extension)
	{
		return !stricmp(extension, "vgm");
	}

	static GUID g_get_guid()
	{
		// {1C2E4065-5235-464c-BE84-B9AC6D1DBC92}
		static const GUID guid = 
		{ 0x1c2e4065, 0x5235, 0x464c, { 0xbe, 0x84, 0xb9, 0xac, 0x6d, 0x1d, 0xbc, 0x92 } };
		return guid;
	}

	static const char * g_get_name() {return "GEP VGM decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

private:
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

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info,bool p_can_loop)
	{
		foobar_File_Reader rdr(p_file, p_abort);
		Vgm_Emu::header_t header;

		try
		{
			if (p_want_info || p_decode)
			{
				ERRCHK( rdr.read( &header, sizeof(header) ) );

				if ( 0 != memcmp( header.tag, "Vgm ", 4 ) )
				{
					console::info("Not a VGM file");
					throw io_result_error_data;
				}
				if ( get_le32( header.vers ) > 0x0101 )
				{
					console::info("Unsupported VGM format");
					throw io_result_error_data;
				}

				//if (!no_infinite) no_infinite = !!memcmp( header.tag, "GYMX" );
				//if (!no_infinite) 
				if (!no_infinite) no_infinite = !p_can_loop;
			}

			if (p_want_info)
			{
				p_info.info_set("codec", "VGM");

				p_info.info_set_int("samplerate", sample_rate);
				p_info.info_set_int("channels", 2);
				p_info.info_set_int("bitspersample", 16);

				if (!p_decode)
				{
					Vgm_Emu * emu = new Vgm_Emu;
					if ( !emu )
					{
						console::info("Out of memory");
						throw io_result_error_out_of_memory;
					}

					ERRCHK( emu->init( sample_rate ) );
					ERRCHK( emu->load( header, rdr ) );
					ERRCHK( emu->start_track( 0 ) );

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

					delete emu;
				}
			}

			if (p_decode)
			{
				Vgm_Emu * emu = new Vgm_Emu;
				if ( !emu )
				{
					console::info("Out of memory");
					throw io_result_error_out_of_memory;
				}

				this->emu = emu;

				ERRCHK( emu->init( sample_rate ) );
				ERRCHK( emu->load( header, rdr ) );
				ERRCHK( emu->start_track( 0 ) );

				Vgm_Emu::track_data_t tdata = emu->track_data();

				if (p_want_info)
				{
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
				}

				if ( no_infinite )
				{
					newtag = false;
					song_len = int( ( tdata.loop_end ? tdata.loop_end : tdata.length ) * double( sample_rate ) );
					fade_len = 0;
				}

				voice_mask = 0;
				subsong = 0;
			}
		}
		catch(t_io_result code)
		{
			return code;
		}

		return io_result_success;
	}
};

DECLARE_FILE_TYPE("VGM files", "*.VGM");

static input_factory_t<input_vgm> g_input_vgm_factory;
