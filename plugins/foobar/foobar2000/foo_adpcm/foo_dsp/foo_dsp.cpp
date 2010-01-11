#define MY_VERSION "1.4"

/*
	change log

2009-08-01 05:27 UTC - kode54
- Increased DSP read buffer size
- Added buffer size checking for RWSD and RSTM header reading
- Version is now 1.4

2006-12-07 09:12 UTC - kode54
- Fixed seeking, passes input validator.
- Version is now 1.3

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
	"DSP", "GCM", "HPS", "IDSP", "SPT", "SPD", "MSS", "MPDSP", "ISH", "YMF", "WAM", "WAC", "WAD", "WAA", "WVS", "THP",
	"RSD", "RSP",
	"RWSD", "BRSTM", "GCA", "ZWDSP", "KRAW", "ASR",
	"ADP"
};

const char * const SPM22names[] = {
"btl_boss_middle1_44k_lp.brstm",
"btl_boss_stg1_22k_lp.brstm",
"btl_boss_stg2_22k_lp.brstm",
"btl_boss_stg3_44k_lp.brstm",
"btl_boss_stg4_lp.brstm",
"btl_boss_stg5_e3_44k_lp.brstm",
"btl_boss_stg6_44k_lp.brstm",
"btl_boss_stg7_44k_lp.brstm",
"btl_demen1_44k_lp.brstm",
"btl_demenL_44k_lp.brstm",
"btl_dodontas2_44k_lp.brstm",
"btl_koopa1_44k_lp.brstm",
"btl_manera1_44k_lp.brstm",
"btl_mr_zigen1_44k_lp.brstm",
"b_happy_flower_44k_lp.brstm",
"evt_angel_44k_lp.brstm",
"evt_area_start1_44k_lp..brstm",
"evt_cooking1_44k_lp.brstm",
"evt_danger1_44k_lp.brstm",
"evt_demen_appear1_44k_lp.brstm",
"evt_Dliigi_appear1_44k_lp.brstm",
"evt_dodontas_appear1_44k_lp.brstm",
"evt_dotabata1_44k_e4_lp.brstm",
"evt_exective_appear1_44k_lp.brstm",
"evt_exective_soudan1_44k_lp.brstm",
"evt_fairlin_magin1_44k_lp.brstm",
"evt_gesso_appear1_44k_lp.brstm",
"evt_hamerustone1_44k_lp.brstm",
"evt_hana_appear1_44k_lp.brstm",
"evt_happy_flower_44k.brstm",
"evt_hiroshi1_44k_lp.brstm",
"evt_kaisou_stg1_44k_lp.brstm",
"evt_kaisou_stg2_44k_lp.brstm",
"evt_kaisou_stg3_44k_lp.brstm",
"evt_kaisou_stg4_44k_lp.brstm",
"evt_konton1_44k_lp.brstm",
"evt_koopa_catsle_44k_lp.brstm",
"evt_manela_appear2_44k_lp.brstm",
"evt_manuke1_44k_lp.brstm",
"evt_mario_house1_44k_lp.brstm",
"evt_op_book_s_lp.brstm",
"evt_op_peach1_e1_lp.brstm",
"evt_peach_hen1_44k_lp.brstm",
"evt_plo4_open1_44k_lp.brstm",
"evt_quiz1_44k_lp.brstm",
"evt_quiz_s_e1_lp.brstm",
"evt_sini_appear_44k_lp.brstm",
"evt_star1_44k_lp.brstm",
"evt_stg1_crystale1_44k_lp.brstm",
"evt_stg3_adv1_22k_lp_e1.brstm",
"evt_stg6_syoumetsu1_e1.brstm",
"evt_stg7_rpg1_e2_44k_lp.brstm",
"evt_stg7_rpg_e3_44k_lp.brstm",
"evt_stg7_rpg_ff1_e2_44k.brstm",
"evt_stg8_end_44k_lp.brstm",
"evt_stg8_escape1_lp.brstm",
"evt_stg8_pure_born1_lp.brstm",
"evt_stg8_pure_go1_44k_lp.brstm",
"evt_stg8_world_break1_44k_lp.brstm",
"evt_uranai_44k_lp.brstm",
"evt_wedding1_44k_lp.brstm",
"evt_zigen_appear1_44k_lp.brstm",
"evt_zunbaba_appear1_44k_lp.brstm",
"ff_areastart_44k_lp.brstm",
"ff_card_get1_lp.brstm",
"ff_companion1_44k_lp.brstm",
"ff_correct1_32k.brstm",
"ff_pureheart1_44k_lp.brstm",
"ff_pureheartget1_s_lp.brstm",
"ff_pureheart_get_s2_lp.brstm",
"ff_umai1_e1_lp.brstm",
"ff_yado1_44k_lp.brstm",
"ff_zigenwaza_get1_44k_lp.brstm",
"map_100f_44k_lp.brstm",
"map_dungeon1_22k_lp.brstm",
"map_room1_22k_lp.brstm",
"map_stg1_1_44k_e_lp.brstm",
"map_stg1_sabaku_lp.brstm",
"map_stg1_start1_44k_lp.brstm",
"map_stg2_1_e3_44k_lp.brstm",
"map_stg2_start1_44k_lp.brstm",
"map_stg2_yakata_e6_lp.brstm",
"map_stg2_yakata_out1_lp.brstm",
"map_stg3_1_e6_44k_lp.brstm",
"map_stg3_castle1_22k_lp.brstm",
"map_stg3_room1_44k_lp.brstm",
"map_stg3_start1_44k_lp.brstm",
"map_stg3_water1_22k_lp.brstm",
"map_stg4_4d1_44k_lp.brstm",
"map_stg4_44k_lp.brstm",
"map_stg4_start1_44k_lp.brstm",
"map_stg5_1_e5_lp.brstm",
"map_stg5_2_e14_lp.brstm",
"map_stg5_start1_44k_lp.brstm",
"map_stg5_truck2_44k_lp.brstm",
"map_stg6_2_44k_lp.brstm",
"map_stg6_3_44k_lp.brstm",
"map_stg6_44k_lp.brstm",
"map_stg6_start1_44k_lp.brstm",
"map_stg7_3_44k_lp.brstm",
"map_stg7_44k_lp.brstm",
"map_stg7_sanzu1_44k_lp.brstm",
"map_stg7_start1_44k_lp.brstm",
"map_stg8_1_44k_lp.brstm",
"map_stg8_start1_44k_lp.brstm",
"map_town_omote1_44k_lp.brstm",
"map_town_ura1_44k_lp.brstm",
"minigame_gura_44k_lp.brstm",
"minigame_hayauti1_44k_e2_lp.brstm",
"minigame_hayauti1_e7_lp.brstm",
"minigame_hayauti1_e8_lp.brstm",
"minigame_koura1_44k_lp.brstm",
"minigame_panel_lp.brstm",
"sys_gameover1_44k_lp.brstm",
"sys_stage_clear_E3_lp.brstm",
"sys_title1_44k_lp.brstm",
"sys_yarare1_44k_lp.brstm",
};

bool checkSPM22(const char * infile) {
	const char * basename = infile + pfc::scan_filename( infile );

	for (unsigned i=0, j = tabsize( SPM22names ); i < j; i++ )
	{
		if ( ! strcmp( basename, SPM22names [i] ) ) return true;
	}
	return false;
}

class input_dsp
{
	headertype            type;
	CUBEFILE              dsp;

	pfc::array_t<t_int16> sample_buffer;

	t_uint64 pos, swallow;

	bool looped;

	unsigned remove_samples( unsigned n )
	{
		unsigned remainder = n % 28;
		n -= remainder;
		dsp.ch[0].filled -= n;
		dsp.ch[0].readloc = ( dsp.ch[0].readloc + n ) % ( 0x8000/8*14 );
		if ( dsp.NCH == 2 )
		{
			dsp.ch[1].filled -= n;
			dsp.ch[1].readloc = ( dsp.ch[1].readloc + n ) % ( 0x8000/8*14 );
		}
		return remainder;
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
		else if ( ! stricmp_utf8( ext, "YMF" ) ) type = type_ymf;
		else if ( ! stricmp_utf8( ext, "WVS" ) ) type = type_wvs;
		else if ( ! stricmp_utf8( ext, "BRSTM" ) && checkSPM22( p_path ) ) type = type_spmrstm_wii;
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
		p_info.info_set( "encoding", "lossy" );
		if ( dsp.ch[0].type == type_adp )
		{
			p_info.info_set( "codec", "ADP" );
		}
		else
		{
			p_info.info_set( "codec", "DSP" );
			/*{
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
			}*/
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
		unsigned swallow_remainder = 0;

		if ( dsp.ch[0].header.loop_flag ) todo = 1008;
		else
		{
			if ( pos >= dsp.nrsamples ) return false;
			todo = dsp.nrsamples - ( pos - swallow );
			if ( todo > 1008 ) todo = 1008;
			if ( ! todo ) return false;
		}

		sample_buffer.grow_size( todo * dsp.NCH );

		while ( ( status && done < todo ) || !done )
		{
			if ( dsp.ch[0].filled < todo )
			{
				status = fillbuffers( & dsp, p_abort );
			}

			done = dsp.ch[0].filled;
			if ( done > todo ) done = todo;

			if ( ! done && ! status ) return false;

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
					swallow_remainder = remove_samples( swallow );
					swallow = 0;
				}
			}
		}

		pos += done - swallow_remainder;

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
			out -= swallow_remainder * dsp.NCH;
			if ( out <= sample_buffer.get_ptr() ) return false;
			p_chunk.set_data_fixedpoint( sample_buffer.get_ptr() + swallow_remainder * dsp.NCH,
				( ( unsigned char * ) out ) - ( ( unsigned char * ) sample_buffer.get_ptr() ),
				dsp.ch[0].header.sample_rate, dsp.NCH, 16, audio_chunk::g_guess_channel_config( dsp.NCH ) );
			return true;
		}

		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		swallow = audio_math::time_to_samples( p_seconds, dsp.ch[0].header.sample_rate );
		if ( swallow >= pos )
		{
			swallow -= pos;
			pos += swallow;
			return;
		}

		{
			t_uint64 swallow = this->swallow;
			decode_initialize( dsp.ch[0].header.loop_flag ? 0 : input_flag_no_looping, p_abort );
			this->swallow = pos = swallow;
		}
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

class dsp_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 4;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		static const char * names[] = { "DSP Files", "RSD Files", "Wii Files", "ADP Files" };
		if (idx > 3) return false;
		out = names[idx];
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		static const unsigned int offsets[] = { 0, 16, 18, 24 };
		static const unsigned int counts[] = { 16, 2, 6, 1 };
		if (idx > 3) return false;
		out.reset();
		for (int l = 0, n = offsets [idx], o = counts [idx]; l < o; l++, n++)
		{
			if (l) out.add_byte(';');
			out << "*." << exts[n];
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static input_singletrack_factory_t<input_dsp>      g_input_acm_factory;
static service_factory_single_t   <dsp_file_types> g_input_file_type_dsp_factory;

DECLARE_COMPONENT_VERSION("GCN DSP decoder", MY_VERSION, "Decodes DSP, GCM, and HPS files ripped from various GameCube and Wii discs.");
