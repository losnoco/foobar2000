#define MY_VERSION "1.2"

/*
	change log

2005-12-04 13:37 UTC - kode54
- Updated to in_cube mess
- Version is now 1.2

2005-11-22 11:20 UTC - kode54
- Fixed seeking/swallow functionality (maybe it's about time I wrote that generic swallow/skip input framework for all these decoders)
- Version is now 1.1

2005-11-19 01:48 UTC - kode54
- Initial release
- Version is now 1.0

*/

#include <foobar2000.h>

#include "cube.h"

static const char * exts[] =
{
	"DSP",
	"GCM",
	"HPS",
	"IDSP",
	"SPT",
	"SPD",
	"MSS",
	"ADP"
};

class input_dsp
{
	headertype            type;
	CUBEFILE              dsp;

	pfc::array_t<t_int16> sample_buffer;

	int pos, swallow;

	bool looped;

	void remove_samples( unsigned n )
	{
		dsp.ch[0].filled -= n;
		dsp.ch[0].readloc = ( dsp.ch[0].readloc + n ) % ( 0x8000/8*14 );
		if ( dsp.NCH == 2 )
		{
			dsp.ch[1].filled -= n;
			dsp.ch[1].readloc = ( dsp.ch[1].readloc + n ) % ( 0x8000/8*14 );
		}
	}

public:
	input_dsp() { type = type_std; pos = 0; dsp.file_length = 0; looped = false; }
	~input_dsp() {}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( p_filehint, p_path, filesystem::open_mode_read, p_abort );
		}

		dsp.ch[0].infile = p_filehint;

		const char * ptr = p_path + pfc::scan_filename( p_path );
		pfc::string_extension ext( ptr );
		const char * dot = strrchr( ptr, '.' );
		if ( dot ) ptr = dot - 1;
		else if ( *ptr ) ptr += strlen( ptr ) - 1;
		bool mp1 = *ptr == 'L' || *ptr == 'l' || *ptr == 'R' || *ptr == 'r';
		bool ww = ptr[-1] == '_' && ( *ptr == '0' || *ptr == '1' );
		bool spt = *ptr && ptr[1] == '.' && ( ptr[2] == 'S' || ptr[2] == 's' ) && ( ptr[3] == 'P' || ptr[3] == 'p' ) && ( ptr[4] == 'D' || ptr[4] == 'd' || ptr[4] == 'T' || ptr[4] == 't' );
		if ( mp1 || ww || spt )
		{
			pfc::string8 temp( p_path );
			if ( mp1 ) temp.set_char( ptr - p_path, *ptr ^ ( 'L' ^ 'R' ) );
			else if ( ww ) temp.set_char( ptr - p_path, *ptr ^ ( '0' ^ '1' ) );
			else temp.set_char( ptr - p_path + 4, ptr[4] ^ ( 'D' ^ 'T' ) );
			try
			{
				filesystem::g_open( p_filehint, temp, filesystem::open_mode_read, p_abort );

				dsp.ch[1].infile = p_filehint;
				bool swap = false;
				if ( mp1 ) swap = ( *ptr | 0x20 ) == 'r';
				else if ( ww ) swap = *ptr == '1';
				else swap = ( ptr[4] | 0x20 ) == 'd';
				if ( swap ) pfc::swap_t( dsp.ch[0].infile, dsp.ch[1].infile );
			}
			catch ( const exception_io_not_found & )
			{
				if ( spt ) throw;
				dsp.ch[1].infile = dsp.ch[0].infile;
			}
		}
		else dsp.ch[1].infile = dsp.ch[0].infile;

		if ( ! stricmp_utf8( ext, "MSS" ) ) type = type_mss;
		else if ( ! stricmp_utf8( ext, "GCM" ) ) type = type_gcm;
		else if ( ! stricmp_utf8( ext, "ADP" ) ) type = type_adp;
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! dsp.file_length )
		{
			if ( type == type_adp )
			{
				if ( ! InitADPFILE( & dsp, p_abort ) ) type = type_std;
			}
			if ( type != type_adp ) InitDSPFILE( & dsp, p_abort, type );

			if ( dsp.ch[0].header.loop_flag ) looped = true;
		}

		p_info.info_set_int( "samplerate", dsp.ch[0].header.sample_rate );
		p_info.info_set_int( "channels", dsp.NCH );
		if ( dsp.ch[0].type == type_adp )
		{
			p_info.info_set( "codec", "ADP" );
		}
		else
		{
			p_info.info_set( "codec", "DSP" );
			{
				static const char * header_type[] = {
					"Standard",
					"Star Fox Assault Cstr",
					"Metroid Prime 2 RS03",
					"Paper Mario 2 STM",
					"HALPST",
					"Metroid Prime 2 Demo",
					"IDSP",
					"SPT+SPD",
					"MSS",
					"GCM",
					"Monopoly Party"
				};
				p_info.info_set( "dsp_header_type", header_type[ dsp.ch[0].type ] );
			}
			if ( looped )
			{
				p_info.info_set_int( "dsp_loop_start", dsp.ch[0].header.sa );
				p_info.info_set_int( "dsp_loop_end", dsp.ch[0].header.ea );
			}
		}

		p_info.set_length( double( dsp.nrsamples ) / double( dsp.ch[0].header.sample_rate ) );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return dsp.ch[0].infile->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! dsp.file_length || pos )
		{
			if ( type == type_adp )
			{
				if ( ! InitADPFILE( & dsp, p_abort ) ) type = type_std;
			}
			if ( type != type_adp ) InitDSPFILE( & dsp, p_abort, type );

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
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		bool status = true;
		int todo, done = 0;

		if ( dsp.ch[0].header.loop_flag ) todo = 1024;
		else
		{
			todo = dsp.nrsamples - pos;
			if ( todo > 1024 ) todo = 1024;
			if ( ! todo ) return false;
		}

		sample_buffer.grow_size( todo * dsp.NCH );

		while ( done < todo && status )
		{
			if ( dsp.ch[0].filled < todo )
			{
				status = fillbuffers( & dsp, p_abort );
			}

			done = dsp.ch[0].filled;
			if ( done > todo ) done = todo;

			if ( ! done && ! status ) return false;

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

		dsp.ch[0].filled -= done;
		if ( dsp.NCH == 2 ) dsp.ch[1].filled -= done;

		t_int16 * out = sample_buffer.get_ptr();

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
			p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), ( ( unsigned char * ) out ) - ( ( unsigned char * ) sample_buffer.get_ptr() ), dsp.ch[0].header.sample_rate, dsp.NCH, 16, audio_chunk::g_guess_channel_config( dsp.NCH ) );
			return true;
		}

		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		swallow = int( audio_math::time_to_samples( p_seconds, dsp.ch[0].header.sample_rate ) );
		if ( swallow >= pos )
		{
			swallow -= pos;
			return;
		}

		unsigned swallow = this->swallow;
		decode_initialize( dsp.ch[0].header.loop_flag ? 0 : input_flag_no_looping, p_abort );
		this->swallow = swallow;
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
		dsp.ch[0].infile->on_idle( p_abort );
		if ( dsp.ch[0].infile != dsp.ch[1].infile )
			dsp.ch[1].infile->on_idle( p_abort );
	}

	void retag( const file_info & p_info,abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		int n;
		for(n=0;n<tabsize(exts);n++)
		{
			if (!stricmp(p_extension,exts[n])) return true;
		}
		return false;
	}
};

DECLARE_FILE_TYPE("GCN DSP audio files", "*.DSP;*.GCM;*.HPS;*.IDSP;*.SPT;*.SPD;*.MSS");

static input_singletrack_factory_t<input_dsp> g_input_acm_factory;

DECLARE_COMPONENT_VERSION("GCN DSP decoder", MY_VERSION, "Decodes DSP, GCM, and HPS files ripped from various GameCube discs.");
