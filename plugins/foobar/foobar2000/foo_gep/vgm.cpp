#include "base.h"
#include "reader.h"
#include "config.h"

#include <gme/Vgm_Emu.h>

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
				pfc::string8 temp;
				do
				{
					if ( ptr_cr && ptr_lf && ptr_cr < ptr_lf ) ptr_lf = ptr_cr;
					if ( ptr_lf )
					{
						temp += pfc::stringcvt::string_utf8_from_wide( value, ptr_lf - value );
						temp += "\r\n";
						value = ptr_lf;
						if ( value[ 0 ] == '\r' && value[ 1 ] == '\n' ) value += 2;
						else ++value;
						ptr_lf = wcschr( value, '\n' );
						ptr_cr = wcschr( value, '\r' );
					}
				}
				while ( * value && ( ptr_lf || ptr_cr ) );

				if ( * value ) temp += pfc::stringcvt::string_utf8_from_wide( value );

				p_info.meta_add( name, temp );
			}
			else
				p_info.meta_add( name, pfc::stringcvt::string_utf8_from_wide( value ) );
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
		if ( ! ( cfg_format_enable & 4 ) ) return false;
		return ! stricmp( p_extension, "vgm" ) || ! stricmp( p_extension, "vgz" );
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		input_gep::open( p_filehint, p_path, p_reason, p_abort );

		try
		{
			service_ptr_t<file> p_unpackfile;
			unpacker::g_open( p_unpackfile, m_file, p_abort );
			m_file = p_unpackfile;
		}
		catch ( const exception_io_data & )
		{
			m_file->seek( 0, p_abort );
		}

		foobar_File_Reader rdr(m_file, p_abort);

		{
			ERRCHK( rdr.read( &m_header, sizeof(m_header) ) );

			if ( 0 != memcmp( m_header.tag, "Vgm ", 4 ) )
			{
				console::info("Not a VGM file");
				throw exception_io_data();
			}
			if ( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &m_header.version ) ) > 0x0150 )
			{
				console::info("Unsupported VGM format");
				throw exception_io_data();
			}
			if ( ! m_header.track_duration )
			{
				console::info("Header contains empty track duration");
			}
		}
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set("codec", "VGM");

		//p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);

		Vgm_Emu * emu = ( Vgm_Emu * ) this->emu;
		if ( ! emu )
		{
			this->emu = emu = new Vgm_Emu();
			emu->disable_oversampling();

			try
			{
				m_file->seek( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->set_sample_rate( sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
				handle_warning();
				emu->start_track( 0 );
				handle_warning();
			}
			catch(...)
			{
				if ( emu )
				{
					delete emu;
					this->emu = emu = NULL;
				}
				throw;
			}

			m_file.release();
		}

		p_info.set_length( double( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &m_header.track_duration ) ) ) / 44100 );

		if ( * ( ( t_uint32 * ) &m_header.loop_offset ) )
		{
			unsigned loop_end = pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &m_header.track_duration ) );
			unsigned loop_dur = pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &m_header.loop_duration ) );
			p_info.info_set_int("vgm_loop_start", loop_end - loop_dur);
		}

		int size = 0;
		const unsigned char * gd3_tag = emu->gd3_data( & size );
		if ( gd3_tag && size )
		{
			process_gd3_tag( gd3_tag, size, p_info );
		}
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Vgm_Emu * emu = ( Vgm_Emu * ) this->emu;
		if ( ! emu )
		{
			this->emu = emu = new Vgm_Emu();
			emu->disable_oversampling();

			try
			{
				m_file->seek( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->set_sample_rate( sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
				handle_warning();
			}
			catch(...)
			{
				if ( emu )
				{
					delete emu;
					this->emu = emu = NULL;
				}
				throw;
			}

			m_file.release();
		}

		if ( p_flags & input_flag_playback ) monitor_start();

		emu->start_track( 0 );
		handle_warning();

		played = 0;
		no_infinite = !cfg_indefinite || ( p_flags & input_flag_no_looping );

		if ( no_infinite )
		{
			int song_len = pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &m_header.track_duration ) );
			if ( song_len )
			{
				//int fade_min = ( 512 * 8 * 1000 / 2 + sample_rate - 1 ) / sample_rate;
				emu->set_fade( song_len * 10 / 441, 0 /*fade_min*/ );
			}
		}

		subsong = 0;

		stop_on_errors = !! ( p_flags & input_flag_testing_integrity );

		first_block = true;
	}
};

DECLARE_FILE_TYPE("VGM files", "*.VGM;*.VGZ");

static input_factory_t<input_vgm> g_input_vgm_factory;
