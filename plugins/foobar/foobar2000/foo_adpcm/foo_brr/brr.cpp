#define MY_VERSION "0.7"

/*
	changelog

2004-10-08 14:45 UTC - kode54
- Rearranged service declarations
- Version is now 0.7

2004-10-08 14:35 UTC - kode54
- Renamed cfg_infinite to cfg_loop
- Removed cfg_loop from in-place checks

2004-10-08 13:47 UTC - kode54
- Implemented seeking

2004-02-01 06:28 UTC - kode54
- Wow, added bitrate info
- Version is now 0.63

2003-11-25 15:53 UTC - kode54
- Minor changes and quick bugfixes in input class
- Version is now 0.62

2003-08-15 20:03 - kode54
- Updated to 0.7 beta 39 API
- Version is now 0.61

2003-06-26 07:24 - kode54
- Updated to 0.7 API
- Version is now 0.6

2003-05-17 19:53 - kode54
- Added XA ADPCM disk writer, 
- Reordered one-time setup code in BRR process_samples
- Version is now 0.5

2003-04-09 16:13 - kode54
- Added bitspersample
- File name is no longer hard coded
- Version is now 0.3

*/

#include <foobar2000.h>

#include <math.h>

/*
#define DBG(a) { \
	OutputDebugString( # a ); \
	a; \
	OutputDebugString("success"); \
}
*/

#ifdef FOO_ADPCM_EXPORTS

#include "../resource.h"
extern cfg_int cfg_loop;

#else

#include "resource.h"
static cfg_int cfg_loop("brr_infinite",0);

#endif

unsigned
readval ( service_ptr_t<file> & r, abort_callback & p_abort )
{
	unsigned val;
	t_uint8 i;
	r->read_object_t( i, p_abort );
	val = i & 0x7f;
	while ( i & 0x80 )
    {
		r->read_object_t( i, p_abort );
		val <<= 7;
		val |= i & 0x7f;
    }
	return val;
}

const int f[5][2] = {   {    0,  0  },
						{   60,  0  },
						{  115, -52 },
						{   98, -55 },
						{  122, -60 } };

typedef struct
{
	int prev0;
	int prev1;
	int prev2;
	int prev3;
} chanprev;

#define CLAMP_TO_SHORT(x) if ((x) != ((short)(x))) (x) = 0x7fff - ((x) >> 20)

static int brr_dec_psx_m(unsigned char *start, signed short *out, int blocks, chanprev *prev)
{
	int s_1, s_2, predict_nr, shift_factor, flags, nSample, s, d, fa;
	int now = 0, end = 0;

	s_1=prev->prev0;
	s_2=prev->prev1;

	for (int pos = 0; pos < blocks && !end; pos++)
	{
		predict_nr=(int)*start;start++;
		shift_factor=predict_nr&0xf;
		predict_nr >>= 4;
		flags=(int)*start;start++;
		end = flags & 1;
		
		// -------------------------------------- //
		for (nSample=0;nSample<14;start++,nSample++)
		{
			d=(int)*start;
			s=((d&0xf)<<12);
			if(s&0x8000) s|=0xffff0000;
			
			fa=(s >> shift_factor);
			fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
			s_2=s_1;s_1=fa;
			s=((d & 0xf0) << 8);

			/*if (fa < -32768) fa = -32768;
			else if (fa > 32767) fa = 32767;*/
			CLAMP_TO_SHORT(fa);
			
			out[now++]=fa;
			
			if(s&0x8000) s|=0xffff0000;
			fa=(s>>shift_factor);              
			fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
			s_2=s_1;s_1=fa;
			
			CLAMP_TO_SHORT(fa);
			
			out[now++]=fa;
		}
	}
	prev->prev0 = s_1;
	prev->prev1 = s_2;
	return now | (end ? 0x80000000 : 0);
}

static int brr_dec_psx_s(unsigned char *start, signed short *out, int blocks, chanprev *prev)
{
	int s_1, s_2, s_3, s_4, predict_nr, predict_nrx, shift_factor, shift_factorx, flags, nSample, s, d, fa;
	int now = 0, end = 0;
	
	s_1=prev->prev0;
	s_2=prev->prev1;
	s_3=prev->prev2;
	s_4=prev->prev3;
	
	for (int pos = 0; pos < blocks && !end; pos++)
	{
		predict_nr=(int)*start;start++;
		shift_factor=predict_nr&0xf;
		predict_nr >>= 4;
		flags=(int)*start;start++;
		end = flags & 1;
		
		predict_nrx=(int)*start;start++;
		shift_factorx=predict_nrx&0xf;
		predict_nrx >>= 4;
		flags=(int)*start;start++;
		
		// -------------------------------------- //
		for (nSample=0;nSample<28;start++,nSample++)
		{
			d=(int)*start;
			s=((d&0xf)<<12);
			if(s&0x8000) s|=0xffff0000;
			
			fa=(s >> shift_factor);
			fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
			s_2=s_1;s_1=fa;
			s=((d & 0xf0) << 8);
			
			CLAMP_TO_SHORT(fa);
			
			out[now++]=fa;
			
			if(s&0x8000) s|=0xffff0000;
			fa=(s>>shift_factorx);
			fa=fa + ((s_3 * f[predict_nrx][0])>>6) + ((s_4 * f[predict_nrx][1])>>6);
			s_4=s_3;s_3=fa;
			
			CLAMP_TO_SHORT(fa);
			
			out[now++]=fa;
		}
	}
	prev->prev0 = s_1;
	prev->prev1 = s_2;
	prev->prev2 = s_3;
	prev->prev3 = s_4;
	return now | (end ? 0x80000000 : 0);
}

static int brr_dec_snes_m(unsigned char *start, signed short *out, int blocks, chanprev *prev)
{
	int prev0, prev1, pos, range, filter, end, loop, counter, temp, input, outx;
	int now = 0;
	prev0 = prev->prev0;
	prev1 = prev->prev1;
	for (pos=0, end=0; pos < blocks && !end; pos++)
	{
		range = (int)*start;start++;	// Let's just put the header here for now
		end = range & 1;	// END bit is bit 0
		loop = range & 2;	// LOOP bit is bit 1
		filter = (range >> 2) & 3;	// FILTER is bits 2 and 3
		range >>= 4;		// RANGE is the upper 4 bits
		
		for (counter = 0; counter < 8; counter++)	// Get the next 8 bytes
		{
			temp = *start++;	// Wherever you want to get this from
			input = (signed char) temp;
			input >>= 4;
			if (range<=0xC)
				outx = (input << range) >> 1;	// Apply the RANGE value
			else
				outx = (outx >> 14) & ~0x7ff;
			
			switch (filter)
			{
			case 0:
				break;
			case 1:
				outx += prev0 >> 1;
				outx += (-prev0) >> 5;
				break;
			case 2:
				outx += prev0;
				outx += (-(prev0 + (prev0 >> 1))) >> 5;
				outx -= prev1 >> 1;
				outx += prev1 >> 5;
				break;
			case 3:
				outx += prev0;
				outx += (-(prev0 + (prev0 << 2) + (prev0 << 3))) >> 7;
				outx -= prev1 >> 1;
				outx += (prev1 + (prev1 >> 1)) >> 4;
				break;
			}
			
			if (outx < -0x8000)
				outx = -0x8000;
			else if (outx > 0x7fff)
				outx = 0x7fff;
			
			prev1 = prev0;
			prev0 = out[now] = (signed short)(outx << 1);
			
			now++;		// Advance our output pointer
			
			// Now do the same thing for the other nybble
			
			input = (signed char) (temp << 4);	// Get the second nybble of the byte
			input >>= 4;
			if (range<=0xC)
				outx = (input << range) >> 1;
			else
				outx &= ~0x7ff;
			
			switch (filter)
			{
			case 0:
				break;
			case 1:
				outx += prev0 >> 1;
				outx += (-prev0) >> 5;
				break;
			case 2:
				outx += prev0;
				outx += (-(prev0 + (prev0 >> 1))) >> 5;
				outx -= prev1 >> 1;
				outx += prev1 >> 5;
				break;
			case 3:
				outx += prev0;
				outx += (-(prev0 + (prev0 << 2) + (prev0 << 3))) >> 7;
				outx -= prev1 >> 1;
				outx += (prev1 + (prev1 >> 1)) >> 4;
				break;
			}
			
			if (outx < -0x8000)
				outx = -0x8000;
			else if (outx > 0x7fff)
				outx = 0x7fff;
			
			prev1 = prev0;
			prev0 = out[now] = outx << 1;
			
			now++;		// Advance our output pointer
		}
	}
	prev->prev0 = prev0;
	prev->prev1 = prev1;
	return now | (end ? 0x80000000 : 0);
}

static int brr_dec_snes_s(unsigned char *start, signed short *out, int blocks, chanprev *prev)
{
	int prev0, prev1, prev2, prev3, pos, range, range2, filter, filter2, end, loop, counter, temp, input, outx;
	int now = 0;
	prev0 = prev->prev0;
	prev1 = prev->prev1;
	prev2 = prev->prev2;
	prev3 = prev->prev3;
	for (pos=0, end=0; pos < blocks && !end; pos++)
	{
		range = (int)*start;start++;	// Let's just put the header here for now
		range2 = (int)*start;start++;
		end = range & 1;	// END bit is bit 0
		loop = range & 2;	// LOOP bit is bit 1
		filter = (range >> 2) & 3;	// FILTER is bits 2 and 3
		filter2 = (range2 >> 2) & 3;
		range >>= 4;		// RANGE is the upper 4 bits
		range2 >>= 4;
		
		for (counter = 0; counter < 16; counter++)	// Get the next 8 bytes
		{
			temp = *start++;	// Wherever you want to get this from
			input = (signed char) temp;
			input >>= 4;
			if (range<=0xC)
				outx = (input << range) >> 1;	// Apply the RANGE value
			else
				outx = (outx >> 14) & ~0x7ff;
			
			switch (filter)
			{
			case 0:
				break;
			case 1:
				outx += prev0 >> 1;
				outx += (-prev0) >> 5;
				break;
			case 2:
				outx += prev0;
				outx += (-(prev0 + (prev0 >> 1))) >> 5;
				outx -= prev1 >> 1;
				outx += prev1 >> 5;
				break;
			case 3:
				outx += prev0;
				outx += (-(prev0 + (prev0 << 2) + (prev0 << 3))) >> 7;
				outx -= prev1 >> 1;
				outx += (prev1 + (prev1 >> 1)) >> 4;
				break;
			}
			
			if (outx < -0x8000)
				outx = -0x8000;
			else if (outx > 0x7fff)
				outx = 0x7fff;
			
			prev1 = prev0;
			prev0 = out[now] = (signed short)(outx << 1);
			
			now++;		// Advance our output pointer
			
			// Now do the same thing for the other nybble
			
			input = (signed char) (temp << 4);	// Get the second nybble of the byte
			input >>= 4;
			if (range2<=0xC)
				outx = (input << range2) >> 1;
			else
				outx = (outx >> 14) & ~0x7ff;
			
			switch (filter2)
			{
			case 0:
				break;
			case 1:
				outx += prev2 >> 1;
				outx += (-prev2) >> 5;
				break;
			case 2:
				outx += prev2;
				outx += (-(prev2 + (prev2 >> 1))) >> 5;
				outx -= prev3 >> 1;
				outx += prev3 >> 5;
				break;
			case 3:
				outx += prev2;
				outx += (-(prev2 + (prev2 << 2) + (prev2 << 3))) >> 7;
				outx -= prev3 >> 1;
				outx += (prev3 + (prev3 >> 1)) >> 4;
				break;
			}
			
			if (outx < -0x8000)
				outx = -0x8000;
			else if (outx > 0x7fff)
				outx = 0x7fff;
			
			prev3 = prev2;
			prev2 = out[now] = (signed short)(outx << 1);
			
			now++;		// Advance our output pointer
		}
	}
	prev->prev0 = prev0;
	prev->prev1 = prev1;
	prev->prev2 = prev2;
	prev->prev3 = prev3;
	return now | (end ? 0x80000000 : 0);
}

static const t_uint8 g_signature[] = { 'B', 'R', 'R', 26 };

class input_brr
{
	pfc::array_t<t_uint8>      brr_buffer;
	pfc::array_t<t_int16>      sample_buffer;

	service_ptr_t<file>        m_file;

	unsigned start, stereo, rate, size, loopstart, psx, eof, dontloop_file, dontloop_instance;

	chanprev prev;

	unsigned pos, swallow;

public:
	input_brr() : start( 0 ) {}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;
	}

private:
	void open_internal( abort_callback & p_abort )
	{
		{
			t_uint8 hdr[4];
			m_file->read_object( hdr, 4, p_abort );
			if ( memcmp( hdr, g_signature, 4 ) ) throw exception_io_data();
			unsigned flags = readval( m_file, p_abort );
			stereo = flags & 1;
			psx = flags & 16;
			dontloop_file = ( flags & 32 );
			if (flags & 2)
				rate = readval( m_file, p_abort );
			else
				rate = 22050;
			if (flags & 4)
				size = readval( m_file, p_abort );
			else
				size = -1;
			if ( flags & 8 )
				loopstart = readval( m_file, p_abort );
			else
				loopstart = ~0;

			start = int( m_file->get_position(p_abort) );

			if (size < 0)
			{
				if ( ! m_file->can_seek() ) throw exception_io_data();
				t_filesize len64 = m_file->get_size_ex( p_abort );
				if ( len64 > (1 << 30) ) throw exception_io_data();
				size = int( len64 - t_filesize( start ) );
			}

			t_filesize size64 = m_file->get_size_ex( p_abort );
			if ( size64 > 0 && size > size64 ) throw exception_io_data();
		}
	}

public:
	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		bool saved_offset = false;
		t_filesize offset;

		if ( ! start ) open_internal( p_abort );
		else if ( sample_buffer.get_size() )
		{
			offset = m_file->get_position( p_abort );
			saved_offset = true;
		}

		try
		{
			tag_processor::read_trailing( m_file, p_info, p_abort );
		}
		catch ( const exception_io_data & ) {}

		if ( saved_offset ) m_file->seek( offset, p_abort );

		p_info.set_length( double ( psx ? ( size * 28 / ( stereo ? 32 : 16 ) ) : ( size * 16 / ( stereo ? 18 : 9 ) ) ) / double( rate ) );

		p_info.info_set_int( "samplerate", rate );
		p_info.info_set_int( "bitspersample", 4 );
		p_info.info_set_int( "decoded_bitspersample", 16 );
		p_info.info_set_int( "channels", 1 + stereo );
		p_info.info_set_int( "bitrate", rate * ( stereo + 1 ) * 8 * ( psx ? 16 : 9 ) / ( psx ? 28 : 16 ) / 1000 );
		if (loopstart!=~0) p_info.info_set_int( "brr_loop_start", loopstart );
		p_info.info_set( "codec", psx ? "PSX ADPCM" : "SNES ADPCM" );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! start ) open_internal( p_abort );
		else m_file->seek( start, p_abort );

		eof = 0;
		pos = 0;
		swallow = 0;
		prev.prev0 = prev.prev1 = 0;
		if (stereo) prev.prev2 = prev.prev3 = 0;

		dontloop_instance = dontloop_file || !cfg_loop || ( p_flags & input_flag_no_looping );

		sample_buffer.set_size( 576 << stereo );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( eof ) return false;

		signed short * output = sample_buffer.get_ptr();

		unsigned now = 0, todo;

		unsigned end = 0;
		
		unsigned unit, perblock, blocksize;

		if (psx)
		{
			perblock = 28;
			blocksize = 16;
		}
		else
		{
			perblock = 16;
			blocksize = 9;
		}
		unit = blocksize * ( 576 / perblock ) << stereo;

		brr_buffer.grow_size( unit );

		unsigned char * ptr = brr_buffer.get_ptr();

more:
		p_abort.check();

		todo = m_file->read( ptr, unit, p_abort );
		
		if ( todo ) todo = todo * perblock / blocksize;
		else end = 1;

		while ( end == 0 && now < todo )
		{
			p_abort.check();

			if (psx)
			{
				if (stereo) end = brr_dec_psx_s( ptr, output, todo / 56, &prev );
				else end = brr_dec_psx_m( ptr, output, todo / 28, &prev );
			}
			else
			{
				if (stereo) end = brr_dec_snes_s( ptr, output, todo / 32, &prev );
				else end = brr_dec_snes_m( ptr, output, todo / 16, &prev );
			}
			if (swallow)
			{
				unsigned swallowed = ( end & 0x7fffffff ) >> stereo;
				if ( swallowed < swallow ) swallow -= swallowed;
				else
				{
					now = ( swallowed - swallow ) << stereo;
					if ( now )
					{
						output += swallow << stereo;
						swallow = 0;
						break;
					}
					swallow = 0;
				}
				todo = m_file->read( ptr, unit, p_abort );
				if (todo) todo = todo * perblock / blocksize;
				else end = 0x80000000;
			}
			else
			{
				now += end & 0x7fffffff;
			}
			pos += end & 0x7fffffff;
			end = end & 0x80000000 ? 1 : 0;
		}

		if (end == 1)
		{
			if (!dontloop_instance)
			{
				if (loopstart != ~0)
				{
					int offset = start + (loopstart ? (loopstart * blocksize << stereo) : 0);
					m_file->seek(offset, p_abort);
				}
				else
				{
					prev.prev0 = prev.prev1 = 0;
					if (stereo)
						prev.prev2 = prev.prev3 = 0;
					m_file->seek(start, p_abort);
				}
			}
			else
				eof = 1;
		}

		if (swallow && !eof && !now) goto more;

		if (now)
		{
			p_chunk.set_data_fixedpoint( output, now << 1, rate, 1 + stereo, 16, audio_chunk::g_guess_channel_config( 1 + stereo ) );
			return true;
		}
		
		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		eof = 0;
		swallow = int( audio_math::time_to_samples( p_seconds, rate ) );
		if ( swallow > pos )
		{
			swallow -= pos;
			return;
		}
		pos = 0;
		memset( & prev, 0, sizeof( prev ) );
		m_file->seek( start, p_abort );
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

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		bool saved_offset = false;
		t_filesize offset;

		if ( sample_buffer.get_size() )
		{
			offset = m_file->get_position( p_abort );
			saved_offset = true;
		}

		t_uint8 hdr[4];
		m_file->seek( 0, p_abort );
		m_file->read_object( hdr, 4, p_abort );
		if ( memcmp( hdr, g_signature, 4 ) ) throw exception_io_data();

		tag_processor::write_apev2( m_file, p_info, p_abort );

		if ( saved_offset ) m_file->seek( offset, p_abort );
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return ! strcmp( p_content_type, "audio/x-brr" ); // I don't expect anyone will be streaming this high bitrate crap, though
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return !stricmp( p_extension, "brr" );
	}
};

#ifndef FOO_ADPCM_EXPORTS

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_INFINITE, BM_SETCHECK, cfg_loop, 0);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_INFINITE:
			cfg_loop = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		}
		break;
	}
	return 0;
}

class config_brr : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "BRR ADPCM decoder";}
	virtual const char * get_parent_name() {return "Input";}
};

#endif

#if 0
const signed short iCoef[4][2] = {
  {0, 0},
  {240, 0},
  {488, -240},
  {460, -208}
};

static double
AdpcmMashS (int curc,		/* current channel */
			int ch,		/* total channels */
			signed short *v,		/* values to use as starting 2 */
			signed short *vl,		/* if set, factor final two samples against these and add to error */
			const signed short iCoef[2],	/* lin predictor coeffs */
			signed short *ibuff,	/* ibuff[] is interleaved input samples */
			int iopow,		/* input/output step, REQUIRE 16 <= *st <= 0x7fff */
			unsigned char *obuff	/* output buffer[blockAlign], or NULL for no output  */
			)
{
	signed short *ip, *itop;
	unsigned char *op;
	int ox = curc;		/*  */
	int d, v0, v1, step;
	double d2;			/* long long is okay also, speed abt the same */

	ip = ibuff + curc;		/* point ip to 1st input sample for this channel */
	itop = ibuff + (ch << 4) + curc;
	v0 = v[0];
	v1 = v[1];
	d2 = 0;

	step = 1 << iopow;

	op = obuff;			/* output pointer (or NULL) */
	for (; ip < itop; ip += ch)
    {
		int vlin, d, dp, c;

		/* make linear prediction for next sample */
		/*      vlin = (v0 * iCoef[0] + v1 * iCoef[1]) >> 8; */
		vlin = (v0 * iCoef[0]) >> 9;
		vlin += (v1 * iCoef[1]) >> 9;
		vlin <<= 1;
		d = *ip - vlin;		/* difference between linear prediction and current sample */
		dp = d + (step << 3) + (step >> 1);
		c = 0;
		if (dp > 0)
		{
			c = dp / step;
			if (c > 15)
				c = 15;
		}
		c -= 8;
		dp = c * step;		/* quantized estimate of samp - vlin */
		c &= 0x0f;		/* mask to 4 bits */

		v1 = v0;			/* shift history */
		v0 = vlin + dp;
		if (v0 < -0x10000) v0 = -0x10000;
		else if (v0 > 0xffff) v0 = 0xffff;
		v0 = (signed short) (v0 & ~1);

		d = *ip - v0;
		d2 += d * d;		/* update square-error */

		if (op)
		{			/* if we want output, put it in proper place */
			/* FIXME: does c<<0 work properly? */
			op[ox >> 1] |= (ox & 1) ? c : (c << 4);
			ox += ch;
		}
    }
	if (vl)
    {
		d = v0 - vl[0];
		d2 += d * d;
		d = v1 - vl[1];
		d2 += d * d;
		d2 /= 18.;
    }
	else
		d2 /= 16.;			/* be sure it's non-negative */
	if (op)
    {
		/* when generating real output, we want to return these */
		v[0] = v0;
		v[1] = v1;
    }
	return sqrt (d2);
}

void
AdpcmBlockMashI (int ch,	/* number of channels */
				 signed short *ip,	/* ip[] is input samples */
				 unsigned char *obuff,	/* output buffer */
				 signed short *v,	/* input/output last samples */
				 signed short *vl)
{
	int s, smin;
	int k, kmin;
	int c;
	double dmin;

	for (s = ch; s < ch * 9; s++)
		obuff[s] = 0;

	for (c = 0; c < ch; c++)
    {
		dmin = 0.;
		kmin = 0;
		smin = 0;
		for (s = 0; s < 13; s++)
		{
			for (k = 0; k < 4; k++)
			{
				double d;
				d =
					AdpcmMashS (c, ch, &v[c << 1], vl ? &vl[c << 1] : 0, iCoef[k],
					ip, s, NULL);

				if ((!s && !k) || d < dmin)
				{
					kmin = k;
					dmin = d;
					smin = s;
				}
			}
		}
		obuff[c] = (smin << 4) | (kmin << 2);

		AdpcmMashS (c, ch, &v[c << 1], 0, iCoef[kmin], ip, smin, obuff + ch);
    }
}

static double
AdpcmMashPS (int curc,		/* current channel */
			int ch,		/* total channels */
			signed short *v,		/* values to use as starting 2 */
			signed short *vl,		/* if set, factor final two samples against these and add to error */
			const signed int iCoef[2],	/* lin predictor coeffs */
			signed short *ibuff,	/* ibuff[] is interleaved input samples */
			int iopow,		/* input/output step, REQUIRE 16 <= *st <= 0x7fff */
			unsigned char *obuff	/* output buffer[blockAlign], or NULL for no output  */
			)
{
	signed short *ip, *itop;
	unsigned char *op;
	int ox = curc;		/*  */
	int d, v0, v1, step;
	double d2;			/* long long is okay also, speed abt the same */

	ip = ibuff + curc;		/* point ip to 1st input sample for this channel */
	itop = ibuff + (ch * 28) + curc;
	v0 = v[0];
	v1 = v[1];
	d2 = 0;

	step = 1 << (12 - iopow);

	op = obuff;			/* output pointer (or NULL) */
	for (; ip < itop; ip += ch)
    {
		int vlin, d, dp, c;

		/* make linear prediction for next sample */
		vlin = (v0 * iCoef[0] + v1 * iCoef[1]) >> 6;
		d = *ip - vlin;		/* difference between linear prediction and current sample */
		dp = d + (step << 3) + (step >> 1);
		c = 0;
		if (dp > 0)
		{
			c = dp / step;
			if (c > 15)
				c = 15;
		}
		c -= 8;
		dp = c * step;		/* quantized estimate of samp - vlin */
		c &= 0x0f;		/* mask to 4 bits */

		v1 = v0;			/* shift history */
		v0 = vlin + dp;
		if (v0 < -0x8000) v0 = -0x8000;
		else if (v0 > 0x7fff) v0 = 0x7fff;

		d = *ip - v0;
		d2 += d * d;		/* update square-error */

		if (op)
		{			/* if we want output, put it in proper place */
			/* FIXME: does c<<0 work properly? */
			op[ox >> 1] |= (ox & 1) ? (c << 4) : c;
			ox += ch;
		}
    }
	if (vl)
    {
		d = v0 - vl[0];
		d2 += d * d;
		d = v1 - vl[1];
		d2 += d * d;
		d2 /= 30.;
    }
	else
		d2 /= 28.;			/* be sure it's non-negative */
	if (op)
    {
		/* when generating real output, we want to return these */
		v[0] = v0;
		v[1] = v1;
    }
	return sqrt (d2);
}

void
AdpcmBlockMashPI (int ch,	/* number of channels */
				 signed short *ip,	/* ip[] is input samples */
				 unsigned char *obuff,	/* output buffer */
				 signed short *v,	/* input/output last samples */
				 signed short *vl)
{
	int s, smin;
	int k, kmin;
	int c;
	double dmin;

	for (s = 2; s < 16 * ch; s++)
		obuff[s] = 0;

	for (c = 0; c < ch; c++)
    {
		dmin = 0.;
		kmin = 0;
		smin = 0;
		for (s = 0; s < 13; s++)
		{
			for (k = 0; k < 5; k++)
			{
				double d;
				d =
					AdpcmMashPS (c, ch, &v[c << 1], vl ? &vl[c << 1] : 0, f[k],
					ip, s, NULL);

				if ((!s && !k) || d < dmin)
				{
					kmin = k;
					dmin = d;
					smin = s;
				}
			}
		}
		obuff[c*2] = (kmin << 4) | smin;
		obuff[(c*2)+1] = 0;

		AdpcmMashPS (c, ch, &v[c << 1], 0, f[kmin], ip, smin, obuff + ch*2);
    }
}

static double
AdpcmMashXA (int unit,		/* current channel */
			int ch,		/* total channels */
			signed short *v,		/* values to use as starting 2 */
			signed short *vl,		/* if set, factor final two samples against these and add to error */
			const signed int iCoef[2],	/* lin predictor coeffs */
			signed short *ibuff,	/* ibuff[] is interleaved input samples */
			int iopow,		/* input/output step, REQUIRE 16 <= *st <= 0x7fff */
			unsigned char *obuff	/* output buffer[blockAlign], or NULL for no output  */
			)
{
	signed short *ip, *itop;
	unsigned char *op;
	int ox = unit;		/*  */
	int d, v0, v1, step;
	double d2;			/* long long is okay also, speed abt the same */

	int curc = (unit & (ch-1)) + ((unit >> (ch-1)) * 28 * ch);

	ip = ibuff + curc;		/* point ip to 1st input sample for this channel */
	itop = ibuff + (ch * 28) + curc;
	v0 = v[0];
	v1 = v[1];
	d2 = 0;

	step = 1 << (12 - iopow);

	op = obuff;			/* output pointer (or NULL) */
	for (; ip < itop; ip += ch)
    {
		int vlin, d, dp, c;

		/* make linear prediction for next sample */
		vlin = (v0 * iCoef[0] + v1 * iCoef[1]) >> 6;
		d = *ip - vlin;		/* difference between linear prediction and current sample */
		dp = d + (step << 3) + (step >> 1);
		c = 0;
		if (dp > 0)
		{
			c = dp / step;
			if (c > 15)
				c = 15;
		}
		c -= 8;
		dp = c * step;		/* quantized estimate of samp - vlin */
		c &= 0x0f;		/* mask to 4 bits */

		v1 = v0;			/* shift history */
		v0 = vlin + dp;
		if (v0 < -0x8000) v0 = -0x8000;
		else if (v0 > 0x7fff) v0 = 0x7fff;

		d = *ip - v0;
		d2 += d * d;		/* update square-error */

		if (op)
		{			/* if we want output, put it in proper place */
			/* FIXME: does c<<0 work properly? */
			op[ox >> 1] |= (ox & 1) ? (c << 4) : c;
			ox += 8;
		}
    }
	if (vl)
    {
		d = v0 - vl[0];
		d2 += d * d;
		d = v1 - vl[1];
		d2 += d * d;
		d2 /= 30.;
    }
	else
		d2 /= 28.;			/* be sure it's non-negative */
	if (op)
    {
		/* when generating real output, we want to return these */
		v[0] = v0;
		v[1] = v1;
    }
	return sqrt (d2);
}

void
AdpcmBlockMashXA4I (int ch,	/* number of channels */
				 signed short *ip,	/* ip[] is input samples */
				 unsigned char *obuff,	/* output buffer */
				 signed short *v,	/* input/output last samples */
				 signed short *vl)
{
	int s, smin;
	int k, kmin;
	int unit, c;
	double dmin;

	memset(obuff + 16, 0, 14 * 8);

	for (unit = 0; unit < 8; unit++)
    {
		c = unit & (ch-1);
		dmin = 0.;
		kmin = 0;
		smin = 0;
		for (s = 0; s < 13; s++)
		{
			for (k = 0; k < 4; k++)
			{
				double d;
				d =
					AdpcmMashXA (unit, ch, &v[c << 1], vl ? &vl[c << 1] : 0, f[k],
					ip, s, NULL);

				if ((!s && !k) || d < dmin)
				{
					kmin = k;
					dmin = d;
					smin = s;
				}
			}
		}
		obuff[4 + unit] = smin | (kmin << 4);

		AdpcmMashXA (unit, ch, &v[c << 1], 0, f[kmin], ip, smin, obuff + 16);
    }
	*(int*)obuff = ((int*)obuff)[1];
	((int*)obuff)[3] = ((int*)obuff)[2];
}

void
writeval (service_ptr_t<file> & r, unsigned val, abort_callback & p_abort)
{
	unsigned i = val, j = 0;
	unsigned char out[5];
	while (i)
    {
		i >>= 7;
		j++;
    }
	while (j--)
    {
		out[i++] = j ? (val >> (j * 7)) | 0x80 : val & 0x7f;
    }
	r->write_object_e(&out, i, p_abort);
}

class NOVTABLE converter_brr : public converter_presetless
{
public:
	virtual void get_name(string_base & out)=0;
	virtual void get_extension(string_base & out)
	{
		out = "brr";
	}

	virtual bool on_user_init(HWND p_parent)
	{
		(void)(unsigned)-1; // FUCKO
		return false;
	}

	virtual t_io_result open(const char * out_path,unsigned expected_bps,bool should_dither,abort_callback & p_abort)
	{
		fn = out_path;
		dither = should_dither;
		return io_result_success;
	}

	virtual t_io_result on_source_track(const metadb_handle_ptr & ptr,abort_callback & p_abort)
	{
		if (!have_info)
		{
			ptr->get_info(info);
			have_info = true;
		}
		return io_result_success;
	}

	virtual t_io_result process_samples(const audio_chunk & src,abort_callback & p_abort)
	{
		int samples = 0;
		t_io_result status;

		if (m_file.is_empty())
		{
			switch (src.get_channels())
			{
			case 1:
				stereo = 0;
				break;
			case 2:
				stereo = 1;
				prev[2] = prev[3] = 0;
				break;
			default:
				return io_result_error_data;
				break;
			}

			status = filesystem::g_open_temp(m_file, p_abort);
			if (io_result_failed(status)) return status;
			if (!audio_postprocessor::g_create(m_cvt)) return io_result_error_generic;

			prev[0] = prev[1] = 0;
			rate = src.get_srate();
			remainder = 0;
		}
		else
		{
			if ((src.get_srate() != rate) ||
				(src.get_channels() != (1+stereo)))
			{
				return io_result_error_data;
			}
			if (remainder)
			{
				if ( ! sample_buffer.check_size( ( remainder + src.get_sample_count() ) << stereo ) )
					return io_result_error_out_of_memory;

				memcpy( sample_buffer.get_ptr(), remainder_buffer.get_ptr(), remainder * sizeof( short ) << stereo );
				samples = remainder;
				remainder = 0;
			}
		}
		mem_block_container_impl out_chunk;
		m_cvt->run( src, out_chunk, 16, 16, dither, 1.0);

		if ( ! sample_buffer.check_size( src.get_sample_count() << stereo ) )
			return io_result_error_out_of_memory;

		signed short *in = sample_buffer.get_ptr();
		memcpy( in + ( samples << stereo ), out_chunk.get_ptr(), out_chunk.get_size() );
		samples += src.get_sample_count();

		if ( psx )
		{
			int total = samples / 28;
			int todo = total;
			remainder = samples % 28;

			if ( ! brr_buffer.check_size( todo * 16 << stereo ) )
				return io_result_error_out_of_memory;

			unsigned char *out = brr_buffer.get_ptr();

			while (todo)
			{
				AdpcmBlockMashPI(stereo+1, in, out, (signed short*)&prev, 0);
				in += 28 << stereo;
				out += 16 << stereo;
				todo--;
			}

			status = m_file->write_object(brr_buffer.get_ptr(), total * 16 << stereo, p_abort);
			if (io_result_failed(status)) return status;
		}
		else
		{
			int total = samples / 16;
			int todo = total;
			remainder = samples % 16;

			if ( ! brr_buffer.check_size( todo * 9 << stereo ) )
				return io_result_error_out_of_memory;

			unsigned char *out = brr_buffer.get_ptr();

			while (todo)
			{
				AdpcmBlockMashI(stereo+1, in, out, (signed short*)&prev, 0);
				in += 16 << stereo;
				out += 9 << stereo;
				todo--;
			}

			status = m_file->write_object(brr_buffer.get_ptr(), total * 9 << stereo, p_abort);
			if (io_result_failed(status)) return status;
		}
		
		if (remainder)
		{
			if ( ! remainder_buffer.check_size( remainder << stereo ) )
				return io_result_error_out_of_memory;

			memcpy( remainder_buffer.get_ptr(), in, remainder * sizeof( short ) << stereo );
		}

		return io_result_success;
	}

	virtual t_io_result flush(abort_callback & p_abort)
	{
		if (m_file.is_empty()) return io_result_success;

		unsigned char *out;
		t_io_result status;

		if ( psx )
		{
			if ( ! brr_buffer.check_size( 16 << stereo ) )
				return io_result_error_out_of_memory;

			out = brr_buffer.get_ptr();
			if ( remainder )
			{
				if ( ! sample_buffer.check_size( 28 << stereo ) )
					return io_result_error_out_of_memory;

				signed short *in = sample_buffer.get_ptr();
				memset( in + ( remainder << stereo ), 0, ( 28 - remainder ) * sizeof( short ) << stereo );
				memcpy( in, remainder_buffer.get_ptr(), remainder * sizeof( short ) << stereo );
				AdpcmBlockMashPI( stereo + 1, in, out, ( signed short * ) & prev, 0 );
				status = m_file->write_object( out, 16 << stereo, p_abort );
				if ( io_result_failed( status ) ) return status;
			}
		}
		else
		{
			if ( ! brr_buffer.check_size( 9 << stereo ) )
				return io_result_error_out_of_memory;

			out = brr_buffer.get_ptr();
			if ( remainder )
			{
				if ( ! sample_buffer.check_size( 16 << stereo ) )
					return io_result_error_out_of_memory;

				signed short *in = sample_buffer.get_ptr();
				memset( in + ( remainder << stereo ), 0, ( 16 - remainder ) * sizeof( short ) << stereo );
				memcpy( in, remainder_buffer.get_ptr(), remainder * sizeof( short ) << stereo );
				AdpcmBlockMashI( stereo + 1, in, out, ( signed short * ) &prev, 0 );
				status = m_file->write_object( out, 9 << stereo, p_abort );
				if ( io_result_failed( status ) ) return status;
			}
		}

		service_ptr_t<file> final;
		status = filesystem::g_open( final, fn.get_ptr(), filesystem::open_mode_write_new, p_abort );
		if ( io_result_failed( status ) ) return status;

		//try
		{
			final->write_object_e( g_signature, 4, p_abort );
			writeval(final, (psx ? 16 : 0) | 4 | 2 | (stereo ? 1 : 0), p_abort);
			writeval(final, rate, p_abort);
			int size = (int)m_file->get_size_e(p_abort);
			writeval(final, size, p_abort);
			m_file->seek_e(0, p_abort);
			if (psx)
			{
				file::g_transfer_object_e(m_file.get_ptr(), final.get_ptr(), size - (16 << stereo), p_abort);
				if (!remainder) m_file->read_object_e(out, 16<<stereo, p_abort);
				out[1] |= 1;
				final->write_object_e(out, 16<<stereo, p_abort);
			}
			else
			{
				file::g_transfer_object_e(m_file.get_ptr(), final.get_ptr(), size - (9 << stereo), p_abort);
				if (!remainder) m_file->read_object_e(out, 9<<stereo, p_abort);
				out[0] |= 1;
				final->write_object_e(out, 9<<stereo, p_abort);
			}
		}
		//catch(exception_io const & e) {return e.get_code();}

		m_file.release();

		if ( have_info )
		{
			have_info = false;
			return tag_processor::write_apev2(final, info, p_abort);
		}

		return io_result_success;
	}

	virtual t_io_result set_info( const file_info & info, abort_callback & p_abort )
	{
		have_info = true;
		this->info.copy( info );
		return io_result_success;
	}

	virtual bool multitrack_query_support()
	{
		return false;
	}

	virtual t_io_result multitrack_split( abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	virtual bool is_permanent()
	{
		return false;
	}

	converter_brr()
	{
		have_info = false;
	}

	int psx;
private:
	service_ptr_t<file> m_file;
	service_ptr_t<audio_postprocessor> m_cvt;
	string_simple fn;
	bool have_info;
	file_info_impl info;
	mem_block_t<signed short> sample_buffer;
	mem_block_t<signed short> remainder_buffer;
	mem_block_t<unsigned char> brr_buffer;

	bool dither;
	unsigned rate, stereo, remainder;
	signed short prev[4];
};

class converter_brr_snes : public converter_brr
{
public:
	virtual void get_name(string_base & out) { out.set_string("BRR (SNES ADPCM)"); }
	converter_brr_snes()
	{
		psx = 0;
	}
	~converter_brr_snes() {}

	virtual GUID get_guid()
	{
		// {DEF7CFBA-A56D-4f87-A48E-E2B3E9E10AD2}
		static const GUID guid = 
		{ 0xdef7cfba, 0xa56d, 0x4f87, { 0xa4, 0x8e, 0xe2, 0xb3, 0xe9, 0xe1, 0xa, 0xd2 } };
		return guid;
	}

};

class converter_brr_psx : public converter_brr
{
public:
	virtual void get_name(string_base & out) { out.set_string("BRR (PSX ADPCM)"); }
	converter_brr_psx()
	{
		psx = 1;
	}
	~converter_brr_psx() {}

	virtual GUID get_guid()
	{
		// {B3B8A53A-723F-4892-9BDA-458F5A1F434B}
		static const GUID guid = 
		{ 0xb3b8a53a, 0x723f, 0x4892, { 0x9b, 0xda, 0x45, 0x8f, 0x5a, 0x1f, 0x43, 0x4b } };
		return guid;
	}

};

class converter_xa : public converter_presetless
{
public:
	virtual void get_name(string_base & out)
	{
		out = "XA ADPCM";
	}

	virtual void get_extension(string_base & out)
	{
		out = "xa";
	}

	virtual bool on_user_init(HWND p_parent)
	{
		(void)(unsigned)-1; // FUCKO
		return false;
	}

	virtual t_io_result open(const char * out_path,unsigned expected_bps,bool should_dither,abort_callback & p_abort)
	{
		fn = out_path;
		dither = should_dither;

		return io_result_success;
	}

	virtual t_io_result on_source_track(const metadb_handle_ptr & ptr,abort_callback & p_abort)
	{
		if (!have_info)
		{
			ptr->get_info(info);
			have_info = true;
		}
		return io_result_success;
	}

	virtual t_io_result process_samples(const audio_chunk & src,abort_callback & p_abort)
	{
		int words = 0;
		t_io_result status;

		if (m_file.is_empty())
		{
			if (src.get_srate() != 37800 && src.get_srate() != 18900) return io_result_error_data;
			switch (src.get_channels())
			{
			case 1:
				stereo = 0;
				break;
			case 2:
				stereo = 1;
				prev[2] = prev[3] = 0;
				break;
			default:
				return io_result_error_data;
				break;
			}

			status = filesystem::g_open_temp( m_file, p_abort );
			if ( io_result_failed( status ) ) return status;
			if (! audio_postprocessor::g_create( m_cvt ) ) return io_result_error_generic;

			prev[0] = prev[1] = 0;
			rate = src.get_srate();
			remainder = 0;
		}
		else
		{
			if ((src.get_srate() != rate) ||
				(src.get_channels() != (1+stereo)))
			{
				return io_result_error_data;
			}
			if (remainder)
			{
				if ( ! sample_buffer.check_size( remainder + ( src.get_sample_count() << stereo ) ) )
					return io_result_error_out_of_memory;

				memcpy( sample_buffer.get_ptr(), remainder_buffer.get_ptr(), remainder * sizeof( short ) );
				words = remainder;
				remainder = 0;
			}
		}
		mem_block_container_impl out_chunk;
		m_cvt->run( src, out_chunk, 16, 16, dither, 1.0);

		if ( ! sample_buffer.check_size( src.get_sample_count() << stereo ) )
			return io_result_error_out_of_memory;

		signed short *in = sample_buffer.get_ptr();
		memcpy( in + words, out_chunk.get_ptr(), out_chunk.get_size() );
		words += src.get_sample_count() << stereo;

		int total = words / 224;
		int todo = total;
		remainder = words % 224;

		if ( ! xa_buffer.check_size(todo * 128) )
			return io_result_error_out_of_memory;

		unsigned char *out = xa_buffer.get_ptr();

		while (todo)
		{
			AdpcmBlockMashXA4I(stereo+1, in, out, (signed short*)&prev, 0);
			in += 224;
			out += 128;
			todo--;
		}

		status = m_file->write_object(xa_buffer.get_ptr(), total * 128, p_abort);
		if (io_result_failed(status)) return status;

		if (remainder)
		{
			if ( ! remainder_buffer.check_size( remainder ) )
				return io_result_error_out_of_memory;

			memcpy( remainder_buffer.get_ptr(), in, remainder * sizeof( short ) );
		}

		return io_result_success;
	}

	virtual t_io_result flush(abort_callback & p_abort)
	{
		if (m_file.is_empty()) return io_result_success;

		if ( ! xa_buffer.check_size( 128 ) )
			return io_result_error_out_of_memory;

		t_io_result status;
		t_filesize sz;
		unsigned char *out = xa_buffer.get_ptr();

		if (remainder)
		{
			if ( ! sample_buffer.check_size( 224 ) )
				return io_result_error_out_of_memory;

			signed short * in = sample_buffer.get_ptr();
			memset( in + remainder, 0, ( 224 - remainder ) * sizeof( short ) );
			memcpy( in, remainder_buffer.get_ptr(), remainder * sizeof( short ) );
			AdpcmBlockMashXA4I( stereo + 1, in, out, ( signed short * ) & prev, 0 );
			status = m_file->write_object( out, 128, p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		service_ptr_t<file> final;
		status = filesystem::g_open(final, fn.get_ptr(), filesystem::open_mode_write_new, p_abort);
		if (io_result_failed(status)) return status;

		//try
		{
			m_file->seek_e(0, p_abort);

			// Prefix, offset 0, length 24:
			memcpy(out, "\0\377\377\377\377\377\377\377\377\377\377", 12);
			out[12] = 0; // minutes
			out[13] = 0; // seconds
			out[14] = 0; // sectors
			out[15] = 2; // mode
			out[20] = out[16] = 0; // file number
			out[21] = out[17] = 0; // channel
			out[22] = out[18] = 0x24; // submode: form=1, audio=1
			out[23] = out[19] = stereo | ((rate == 18900) ? 4 : 0); // coding info

			// Suffix, offset 2328, length 24:
			memset(out + 24, 0, 24); // 20 unused bytes plus unused EDC

			do
			{
				final->write_object_e(out, 24, p_abort);
				file::g_transfer_e(m_file.get_ptr(), final.get_ptr(), 2304, sz, p_abort);
				if (!sz)
				{
					out[22] = out[18] |= 128;
					final->seek2_e(-2352, SEEK_CUR, p_abort);
					final->write_object_e(out, 24, p_abort);
					break;
				}
				else if (sz < 2304)
				{
					out[22] = out[18] |= 128;
					final->seek2_e( - t_int64(sz + 24), SEEK_CUR, p_abort);
					final->write_object_e(out, 24, p_abort);
					memset(out, 12, 16);
					memset(out+16, 0, 112);
					final->seek2_e(sz, SEEK_CUR, p_abort);
					while (sz < 2304)
					{
						final->write_object_e(out, 128, p_abort);
						sz += 128;
					}
					sz = 0;
				}
				final->write_object_e(out + 24, 24, p_abort);
			}
			while (sz == 2304);
		}
		//catch(exception_io const & e) {return e.get_code();}

		m_file.release();
		have_info = false;

		return tag_processor::write_apev2(final, info, p_abort);
	}

	virtual t_io_result set_info( const file_info & info, abort_callback & p_abort )
	{
		have_info = true;
		this->info.copy( info );
		return io_result_success;
	}

	virtual bool multitrack_query_support()
	{
		return false;
	}

	virtual t_io_result multitrack_split( abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	virtual bool is_permanent()
	{
		return false;
	}

	converter_xa()
	{
		have_info = false;
	}

	virtual GUID get_guid()
	{
		// {57B4ACE7-3A68-41eb-A56D-A5DD52686277}
		static const GUID guid = 
		{ 0x57b4ace7, 0x3a68, 0x41eb, { 0xa5, 0x6d, 0xa5, 0xdd, 0x52, 0x68, 0x62, 0x77 } };
		return guid;
	}

private:
	service_ptr_t<file> m_file;
	service_ptr_t<audio_postprocessor> m_cvt;
	string_simple fn;
	bool have_info;
	file_info_impl info;
	mem_block_t<signed short> sample_buffer;
	mem_block_t<signed short> remainder_buffer;
	mem_block_t<unsigned char> xa_buffer;

	bool dither;
	UINT rate, stereo, remainder;
	signed short prev[4];
};
#endif

DECLARE_FILE_TYPE("BRR files", "*.BRR");

static input_singletrack_factory_t<input_brr> g_input_brr_factory;
#if 0
static converter_factory<converter_brr_snes> g_converter_brr_snes_factory;
static converter_factory<converter_brr_psx> g_converter_brr_psx_factory;
static converter_factory<converter_xa> g_converter_xa_factory;
#endif

#ifndef FOO_ADPCM_EXPORTS
static config_factory<config_brr> g_config_brr_factory;
#endif

//DECLARE_COMPONENT_VERSION("BRR decoder and converter", MY_VERSION, "Plays and encodes .BRR files, which are a custom\ncontainer for PSX or SNES ADPCM data.\nNow encodes .XA files. (18900/37800Hz mono/stereo)");
DECLARE_COMPONENT_VERSION("BRR decoder", MY_VERSION, "Plays .BRR files, which are a custom\ncontainer for PSX or SNES ADPCM data.");
