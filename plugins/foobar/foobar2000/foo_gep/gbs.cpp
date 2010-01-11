#include "base.h"
#include "reader.h"

#include <gme/Gbs_Emu.h>

class input_gbs : public input_gep
{
	Gbs_Emu::header_t m_header;

public:
	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "gbs" );
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

			if ( 0 != memcmp( m_header.tag, "GBS", 3 ) )
			{
				console::print("Not a GBS file");
				throw io_result_error_data;
			}

			if ( m_header.vers != 1 )
			{
				console::print("Unsupported GBS format");
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
		return m_header.track_count;
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		HEADER_STRING(p_info, "album", m_header.game);
		HEADER_STRING(p_info, "artist", m_header.author);
		HEADER_STRING(p_info, "copyright", m_header.copyright);

		p_info.info_set("codec", "GBS");

		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);

		p_info.set_length(double(tag_song_ms + tag_fade_ms) * .001);

		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Gbs_Emu * emu = ( Gbs_Emu * ) this->emu;
		if ( ! emu )
		{
			emu = new Gbs_Emu;
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

				ERRCHK( emu->set_sample_rate( sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
			}
			catch ( t_io_result code )
			{
				if ( emu )
				{
					delete emu;
					this->emu = emu = NULL;
				}
				return code;
			}

			m_file.release();
		}

		return input_gep::decode_initialize( p_subsong, p_flags, p_abort );
	}
};

DECLARE_FILE_TYPE( "GBS files", "*.GBS" );

static input_factory_t        <input_gbs>   g_input_factory_gbs;
