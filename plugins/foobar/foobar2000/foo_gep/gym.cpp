#include "base.h"
#include "reader.h"

#include <Gym_Emu.h>

class input_gym : public input_gep
{
	Gym_Emu::header_t m_header;

public:
	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "gym" );
	}

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		t_io_result status = input_gep::open( p_filehint, p_path, p_reason, p_abort );
		if ( io_result_failed( status ) ) return status;

		foobar_File_Reader rdr( m_file, p_abort );

		try
		{
			ERRCHK( rdr.read( & m_header, sizeof( m_header ) ) );
		}
		catch ( t_io_result code )
		{
			return code;
		}

		return io_result_success;
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		if ( memcmp( m_header.tag, "GYMX", 4 ) == 0 )
		{
			HEADER_STRING(p_info, "title", m_header.song);
			HEADER_STRING(p_info, "album", m_header.game);
			HEADER_STRING(p_info, "copyright", m_header.copyright);
			HEADER_STRING(p_info, "emulator", m_header.emulator);
			HEADER_STRING(p_info, "dumper", m_header.dumper);
			HEADER_STRING(p_info, "comment", m_header.comment);
		}

		p_info.info_set("codec", "GYM");

		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);

		Gym_Emu * emu = new Gym_Emu;
		if ( !emu )
		{
			console::info("Out of memory");
			throw io_result_error_out_of_memory;
		}

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

		double loop_start;

		p_info.set_length( emu->track_length( &loop_start ) );

		if (loop_start >= 0) p_info.info_set_float("gym_loop_start", loop_start, 4);

		delete emu;

		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Gym_Emu * emu = new Gym_Emu;
		if ( !emu )
		{
			console::info("Out of memory");
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

		double loop_start;
		double length = emu->track_length( &loop_start );

		if (loop_start >= 0 && no_infinite)
		{
			tag_song_ms = int( length * 1000. + .5 );
			tag_fade_ms = 0;
		}

		return input_gep::decode_initialize( 0, p_flags, p_abort );
	}
};

DECLARE_FILE_TYPE("GYM files", "*.GYM");

static input_factory_t<input_gym> g_input_gym_factory;
