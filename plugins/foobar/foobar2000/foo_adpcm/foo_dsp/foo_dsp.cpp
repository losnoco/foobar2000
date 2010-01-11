#define MY_VERSION "1.0"

/*
	change log

2005-11-19 01:48 UTC - kode54
- Initial release
- Version is now 1.0

*/

#include <foobar2000.h>

#include "dsp.h"

class input_dsp
{
	DSPFILE            dsp;

	mem_block_t<short> sample_buffer;

	int pos, swallow;

	bool looped;

	void remove_samples( unsigned n )
	{
		dsp.ch[0].readloc = ( dsp.ch[0].readloc + n ) % ( 0x8000/8*14 );
		if ( dsp.NCH == 2 )
		{
			dsp.ch[1].readloc = ( dsp.ch[1].readloc + n ) % ( 0x8000/8*14 );
		}
	}

public:
	input_dsp() { pos = 0; dsp.file_length = 0; looped = false; }
	~input_dsp() {}

	t_io_result open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( p_filehint, p_path, filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		dsp.ch[0].infile = p_filehint;

		const char * ptr = p_path + string8::g_scan_filename( p_path );
		const char * dot = strrchr( ptr, '.' );
		if ( dot ) ptr = dot - 1;
		else if ( *ptr ) ptr += strlen( ptr ) - 1;
		if ( *ptr == 'L' || *ptr == 'l' || *ptr == 'R' || *ptr == 'r' )
		{
			string8 temp( p_path );
			temp.set_char( ptr - p_path, *ptr ^ ( 'L' ^ 'R' ) );
			status = filesystem::g_open( p_filehint, temp, filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) && status != io_result_error_not_found ) return status;
			if ( status != io_result_error_not_found )
			{
				dsp.ch[1].infile = p_filehint;
				if ( *ptr == 'R' || *ptr == 'r' ) pfc::swap_t( dsp.ch[0].infile, dsp.ch[1].infile );
			}
			else dsp.ch[1].infile = dsp.ch[0].infile;
		}
		else dsp.ch[1].infile = dsp.ch[0].infile;

		return io_result_success;
	}

	t_io_result get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! dsp.file_length )
		{
			try
			{
				InitDSPFILE( & dsp, p_abort );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			if ( dsp.ch[0].header.loop_flag ) looped = true;
		}

		p_info.info_set_int( "samplerate", dsp.ch[0].header.sample_rate );
		p_info.info_set_int( "channels", dsp.NCH );
		p_info.info_set( "codec", "DSP" );
		{
			static const char * header_type[] = { "Standard", "Star Fox: Assault", "Metroid Prime 2", "Paper Mario 2", "HALPST", "Metroid Prime 2 demo" };
			p_info.info_set( "dsp_header_type", header_type[ dsp.ch[0].type ] );
		}
		if ( looped )
		{
			p_info.info_set_int( "dsp_loop_start", dsp.ch[0].header.sa );
			p_info.info_set_int( "dsp_loop_end", dsp.ch[0].header.ea );
		}

		p_info.set_length( double( dsp.nrsamples ) / double( dsp.ch[0].header.sample_rate ) );

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		return dsp.ch[0].infile->get_stats( p_stats, p_abort );
	}

	t_io_result decode_initialize( unsigned p_flags,abort_callback & p_abort )
	{
		if ( ! dsp.file_length || pos )
		{
			try
			{
				InitDSPFILE( & dsp, p_abort );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			if ( dsp.ch[0].header.loop_flag ) looped = true;
		}

		if ( p_flags & input_flag_no_looping )
		{
			dsp.ch[0].header.loop_flag = 0;
			dsp.ch[1].header.loop_flag = 0;
		}
		else if ( looped )
		{
			dsp.ch[0].header.loop_flag = 1;
			dsp.ch[1].header.loop_flag = 1;
		}

		pos = 0;
		swallow = 0;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		bool eof = false;
		int todo, done = 0;

		if ( dsp.ch[0].header.loop_flag ) todo = 1024;
		else
		{
			todo = dsp.nrsamples - pos;
			if ( todo > 1024 ) todo = 1024;
			if ( ! todo ) return io_result_eof;
		}

		if ( ! sample_buffer.check_size( todo * dsp.NCH ) )
			return io_result_error_out_of_memory;

		while ( done < todo && !eof )
		{
			if ( dsp.ch[0].filled < todo )
			{
				try
				{
					fillbuffers( & dsp, p_abort );
				}
				catch ( t_io_result code )
				{
					if ( code != io_result_eof ) return code;
					eof = true;
				}
			}

			done = dsp.ch[0].filled;
			if ( done > todo ) done = todo;

			if ( ! done && eof ) return io_result_eof;

			dsp.ch[0].filled -= done;
			if ( dsp.NCH == 2 ) dsp.ch[1].filled -= done;

			pos += done;

			if ( swallow )
			{
				if ( swallow > done )
				{
					swallow -= done;
					remove_samples( done );
					done = 0;
				}
				else
				{
					done -= swallow;
					remove_samples( swallow );
					swallow = 0;
				}
			}
		}

		short * out = sample_buffer.get_ptr();

		if ( dsp.NCH == 2 )
		{
			for ( unsigned i = 0; i < done; ++i )
			{
				*out++ = dsp.ch[0].chanbuf[ dsp.ch[0].readloc++ ];
				*out++ = dsp.ch[1].chanbuf[ dsp.ch[1].readloc++ ];
				if ( dsp.ch[0].readloc >= 0x8000/8*14 ) dsp.ch[0].readloc = 0;
				if ( dsp.ch[1].readloc >= 0x8000/8*14 ) dsp.ch[1].readloc = 0;
			}
		}
		else
		{
			for ( unsigned i = 0; i < done; ++i )
			{
				*out++ = dsp.ch[0].chanbuf[ dsp.ch[0].readloc++ ];
				if ( dsp.ch[0].readloc >= 0x8000/8*14 ) dsp.ch[0].readloc = 0;
			}
		}

		if (done)
		{
			p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), done * 2 * dsp.NCH, dsp.ch[0].header.sample_rate, dsp.NCH, 16, audio_chunk::g_guess_channel_config( dsp.NCH ) );
			return io_result_success;
		}

		return io_result_eof;
	}

	t_io_result decode_seek( double p_seconds,abort_callback & p_abort )
	{
		swallow = int( p_seconds * double( dsp.ch[0].header.sample_rate ) + .5 );
		if ( swallow > pos )
		{
			swallow -= pos;
			return io_result_success;
		}

		unsigned swallow = this->swallow;
		t_io_result status = decode_initialize( dsp.ch[0].header.loop_flag ? 0 : input_flag_no_looping, p_abort );
		this->swallow = swallow;
		return status;
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	t_io_result retag( const file_info & p_info,abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "dsp" ) || ! stricmp( p_extension, "gcm" ) || ! stricmp( p_extension, "hps" );
	}
};

DECLARE_FILE_TYPE("GCN DSP audio files", "*.DSP;*.GCM;*.HPS");

static input_singletrack_factory_t<input_dsp> g_input_acm_factory;

DECLARE_COMPONENT_VERSION("GCN DSP decoder", MY_VERSION, "Decodes DSP, GCM, and HPS files ripped from various GameCube discs.");
