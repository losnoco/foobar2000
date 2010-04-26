#define MY_VERSION "1.0"

/*
	change log

2004-10-08 12:10 UTC - kode54
- Changed seek function to only brute force forward from the current position
- Version is now 1.0

*/

#include <foobar2000.h>

#include "foofile.h"
#include "racloops.h"

#ifdef FOO_ADPCM_EXPORTS

#include "../resource.h"
extern advconfig_checkbox_factory cfg_loop;

#else

#include "resource.h"
static cfg_int cfg_loop("rac_loop", 0);

#endif

#include "../ima_adpcm.h"

class ImaExpandM
{
	typedef struct imastate
	{
		int val, state;
	} imastate;

	foofile * input;

	imastate state;
	imastate statel;

	int position;
	int loop_start;

	int i, ip;

public:
	ImaExpandM(foofile * p_input)
		: input(p_input), position(0), loop_start(-1), i(0), ip(0)
	{
		memset(&state, 0, sizeof(state));
	}

	ImaExpandM(foofile * p_input, int p_loop_start)
		: input(p_input), position(0), loop_start(p_loop_start), i(0), ip(0)
	{
		memset(&state, 0, sizeof(state));
	}

	void reset_state()
	{
		memset(&state, 0, sizeof(state));
		i = 0;
		position = 0;
	}

	inline void reset_state( int loop_start )
	{
		this->loop_start = loop_start;
		reset_state();
	}

	void reload_state()
	{
		state = statel;
		i = 0;
		position = loop_start;
	}

	unsigned decode(short * out, unsigned out_size, abort_callback & p_abort)
	{
		int done = 0;

		while (out_size && !input->get_eof(p_abort)) {

			int step,dp,c,cm;

			if (i&1) {
				cm = ip>>4;
			} else {
				if (position++ == loop_start) statel = state;
				ip = input->get(p_abort);
				cm = ip & 0x0f;
			}

			step = imaStepSizeTable[state.state];
			/* Update the state for the next sample */
			c = cm & 0x07;
			state.state = imaStateAdjustTable[state.state][c];

#               ifdef STRICT_IMA
			dp = 0;
			if (c & 4) dp += step;
			step = step >> 1;
			if (c & 2) dp += step;
			step = step >> 1;
			if (c & 1) dp += step;
			step = step >> 1;
			dp += step;
#               else
			dp = ((c+c+1) * step) >> 3; /* faster than bit-test & add on my cpu */
#               endif
			if (c != cm) {
				state.val -= dp;
				if (state.val<-0x8000) state.val = -0x8000;
			} else {
				state.val += dp;
				if (state.val>0x7fff) state.val = 0x7fff;
			}

			*out++ = state.val;

			out_size--;
			done++;
			i++;
		}
		return done;
	}
};

class ImaExpandS
{
	typedef struct imastate
	{
		int val0, state0;
		int val1, state1;
	} imastate;

	foofile * input;

	imastate state;
	imastate statel;

	int position;
	int loop_start;

	int i, ip0, ip1;

public:
	ImaExpandS(foofile * p_input)
		: input(p_input), position(0), loop_start(-1), i(0), ip0(0), ip1(0)
	{
		memset(&state, 0, sizeof(state));
	}

	ImaExpandS(foofile * p_input, int p_loop_start)
		: input(p_input), position(0), loop_start(p_loop_start), i(0), ip0(0), ip1(0)
	{
		memset(&state, 0, sizeof(state));
	}

	void reset_state()
	{
		memset(&state, 0, sizeof(state));
		i = 0;
		position = 0;
	}

	inline void reset_state( int loop_start )
	{
		this->loop_start = loop_start;
		reset_state();
	}

	void reload_state()
	{
		state = statel;
		i = 0;
		position = loop_start;
	}

	unsigned decode(short * out, unsigned out_size, abort_callback & p_abort)
	{
		int done = 0;

		while (out_size && !input->get_eof(p_abort)) {

			if (!(i & 1))
			{
				int step,dp,c,cm;

				if (i&2) {
					cm = ip0>>4;
				} else {
					if (position++ == loop_start) statel = state;
					ip0 = input->get(p_abort);
					cm = ip0 & 0x0f;
				}

				step = imaStepSizeTable[state.state0];
				/* Update the state for the next sample */
				c = cm & 0x07;
				state.state0 = imaStateAdjustTable[state.state0][c];

#               ifdef STRICT_IMA
				dp = 0;
				if (c & 4) dp += step;
				step = step >> 1;
				if (c & 2) dp += step;
				step = step >> 1;
				if (c & 1) dp += step;
				step = step >> 1;
				dp += step;
#               else
				dp = ((c+c+1) * step) >> 3; /* faster than bit-test & add on my cpu */
#               endif
				if (c != cm) {
					state.val0 -= dp;
					if (state.val0<-0x8000) state.val0 = -0x8000;
				} else {
					state.val0 += dp;
					if (state.val0>0x7fff) state.val0 = 0x7fff;
				}

				*out++ = state.val0;
			}
			else
			{
				int step,dp,c,cm;

				if (i&2) {
					cm = ip1>>4;
				} else {
					position++;
					ip1 = input->get(p_abort);
					cm = ip1 & 0x0f;
				}

				step = imaStepSizeTable[state.state1];
				/* Update the state for the next sample */
				c = cm & 0x07;
				state.state1 = imaStateAdjustTable[state.state1][c];

#               ifdef STRICT_IMA
				dp = 0;
				if (c & 4) dp += step;
				step = step >> 1;
				if (c & 2) dp += step;
				step = step >> 1;
				if (c & 1) dp += step;
				step = step >> 1;
				dp += step;
#               else
				dp = ((c+c+1) * step) >> 3; /* faster than bit-test & add on my cpu */
#               endif
				if (c != cm) {
					state.val1 -= dp;
					if (state.val1<-0x8000) state.val1 = -0x8000;
				} else {
					state.val1 += dp;
					if (state.val1>0x7fff) state.val1 = 0x7fff;
				}
				*out++ = state.val1;
			}
			out_size--;
			done++;
			i++;
		}
		return done;
	}
};

/* without crossfade, these loops pop annoyingly */

const int xfade_shift = 6;
const int xfade_size = (1UL << xfade_shift);

class input_rac
{
	service_ptr_t<file>   m_file;
	foofile             * inf;

	/*ImaExpandM        * expand_m;*/
	ImaExpandS          * expand_s;

	pfc::array_t<t_int16> sample_buffer;

	t_uint64 pos, filled, total, swallow;
	int loop_start;

	pfc::string8 file_name;
	t_filesize   file_length;

	bool loop, eof;

public:
	input_rac() : inf(0), /*expand_m(0),*/ expand_s(0), file_length(0) {}

	~input_rac()
	{
		/*if (expand_m) delete expand_m;*/
		if (expand_s) delete expand_s;
		if (inf) delete inf;
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;

		file_name = pfc::string_filename( p_path );
		file_name.truncate( file_name.find_last( '.' ) );
	}

private:
	void open_internal( abort_callback & p_abort )
	{
		file_length = m_file->get_size_ex( p_abort );

		if ( file_length == 0 || file_length > (1UL << 30) ) throw exception_io_data();

		if ( file_length & 1 ) throw exception_io_data();

		loop_start = find_loop_start( file_name, int( file_length ) );
	}

public:
	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! file_length ) open_internal( p_abort );

		p_info.info_set_int( "samplerate", 22050 );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set_int( "bitspersample", 4 );
		p_info.info_set_int( "decoded_bitspersample", 16 );
		p_info.info_set( "codec", "IMA ADPCM" );
		p_info.info_set( "encoding", "lossy" );
		p_info.info_set_int( "bitrate", 22050 * 8 / 1000 );

		if ( loop_start >= 0 ) p_info.info_set_int( "rac_loop_start", loop_start );

		p_info.set_length( double( file_length ) * ( 1. / 22050. ) );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! file_length ) open_internal( p_abort );

		loop = cfg_loop && ! ( p_flags & input_flag_no_looping );

		if ( ! inf ) inf = new foofile( m_file );
		else inf->seek( 0, p_abort );

		if ( ! expand_s ) expand_s = new ImaExpandS( inf, loop ? loop_start : -1 );
		else expand_s->reset_state( loop ? loop_start : -1 );

		pos = 0;
		filled = 0;
		swallow = 0;
		total = 0;
		eof = false;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		sample_buffer.grow_size( 2048 + xfade_size * 2 );

		short * buffer = sample_buffer.get_ptr();

		unsigned done, todo;

		if ( eof )
		{
			if ( filled > 0 )
			{
eof:
				done = ( pos - filled ) & 0x7FF;
				todo = 2048 - done;
				if ( todo > filled ) todo = filled;
				p_chunk.set_data_fixedpoint( buffer + done, todo * 2, 22050, 2, 16, audio_chunk::channel_config_stereo );
				filled -= todo;
				return true;
			}
			return false;
		}

		{
			while ( filled < 2048 || swallow )
			{
				p_abort.check();

				todo = 2048 - filled;
				if ( ( 2048 - pos ) < todo ) todo = 2048 - pos;
				done = expand_s->decode( buffer + pos, todo, p_abort );
				pos = ( pos + done ) & 0x7FF;
				filled += done;
				total += done;
				if ( done == todo && filled < 2048 )
				{
					todo = 2048 - filled;
					done = expand_s->decode( buffer + pos, todo, p_abort );
					pos = ( pos + done ) & 0x7FF;
					filled += done;
					total += done;
				}
				if ( done < todo )
				{
					if ( loop && loop_start >= 0 )
					{
						if ( filled < xfade_size * 2 ) break;
						inf->seek( loop_start, p_abort );
						expand_s->reload_state();
						done = expand_s->decode( buffer + 2048, xfade_size * 2, p_abort );
						if ( done < xfade_size * 2 ) break;
						pos -= xfade_size * 2;
						for ( unsigned i = 0; i < xfade_size; ++i )
						{
							buffer[ ( pos + i * 2 ) & 0x7FF ] += ( ( buffer[ 2048 + i * 2 ] - buffer[ ( pos + i * 2 ) & 0x7FF ] ) * i ) >> xfade_shift;
							buffer[ ( pos + i * 2 + 1 ) & 0x7FF ] += ( ( buffer[ 2048 + i * 2 + 1 ] - buffer[ ( pos + i * 2 + 1 ) & 0x7FF] ) * i ) >> xfade_shift;
						}
						pos += xfade_size * 2;
					}
					else
					{
						eof = true;
						break;
					}
				}

				if ( swallow )
				{
					done = filled >> 1;
					if ( done > swallow ) done = swallow;
					swallow -= done;
					done <<= 1;
					filled -= done;
					if ( ! filled ) pos = 0;
				}
			}
		}

		if ( ! filled ) return false;

		if ( filled < xfade_size * 2 )
		{
			eof = true;
			goto eof;
		}

		done = ( pos - filled ) & 0x7FF;
		todo = 2048 - done;
		if ( todo > ( filled - xfade_size * 2 ) ) todo = filled - xfade_size * 2;
		p_chunk.set_data_fixedpoint( buffer + done, todo * 2, 22050, 2, 16, audio_chunk::channel_config_stereo );
		filled -= todo;

		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		eof = false;
		swallow = audio_math::time_to_samples( p_seconds, 22050 );
		if ( swallow > total )
		{
			swallow -= total;
			return;
		}
		total = 0;
		expand_s->reset_state();

		inf->seek( 0, p_abort );
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
		m_file->on_idle( p_abort );
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
		return ! stricmp( p_extension, "rac" );
	}
};

DECLARE_FILE_TYPE("RAC files", "*.RAC");

#ifndef FOO_ADPCM_EXPORTS

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_LOOP, BM_SETCHECK, cfg_loop, 0);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_LOOP:
			cfg_loop = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		}
		break;
	}
	return 0;
}

class config_rac : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "RAC decoder";}
	virtual const char * get_parent_name() {return "Input";}
};

#endif

static input_singletrack_factory_t<input_rac> g_input_rac_factory;

#ifndef FOO_ADPCM_EXPORTS
static input_file_type_factory<rac_file_types> g_config_rac_factory;
#endif

//DECLARE_COMPONENT_VERSION("RAC decoder", MY_VERSION, "Decodes all of the RAC files found in STTNG.ZIP on\nthe Star Trek: The Next Generation - A Final Unity CD.");
