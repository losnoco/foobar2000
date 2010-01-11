#include "base.h"
#include "reader.h"

#include <Hes_Emu.h>

class input_hes : public input_gep
{
	Hes_Emu::header_t m_header;
public:
	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "hes" ) || ! stricmp( p_extension, "pce" );
	}

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status = input_gep::open( p_filehint, p_path, p_reason, p_abort );
		if ( io_result_failed( status ) ) return status;

		foobar_File_Reader rdr( m_file, p_abort );

		try
		{
			ERRCHK( rdr.read( & m_header, sizeof( m_header ) ) );

			if ( 0 != memcmp( m_header.tag, "HESM", 4 ) )
			{
				console::print("Not a HES file");
				throw io_result_error_data;
			}

			if ( m_header.vers != 0 )
			{
				console::print("Unsupported HES format");
				throw io_result_error_data;
			}
		}
		catch ( t_io_result code )
		{
			return code;
		}

		return io_result_success;
	}

	unsigned get_subsong_count()
	{
		return m_header.track_count - m_header.first_track;
	}

	t_uint32 get_subsong( unsigned p_subsong )
	{
		return m_header.first_track + p_subsong;
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set("codec", "HES");

		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);

		p_info.set_length(double(tag_song_ms + tag_fade_ms) * .001);

		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Hes_Emu * emu = ( Hes_Emu * ) this->emu;
		if ( ! emu )
		{
			emu = new Hes_Emu;
			if ( !emu )
			{
				console::print("Out of memory");
				return io_result_error_out_of_memory;
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

		return input_gep::decode_initialize( p_subsong, p_flags, p_abort );
	}
};

DECLARE_FILE_TYPE( "HES files", "*.HES" );

static input_factory_t        <input_hes>   g_input_factory_hes;
