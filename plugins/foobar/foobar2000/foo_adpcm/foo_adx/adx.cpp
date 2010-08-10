#define MY_VERSION "1.8"

/*
	change log

2010-08-10 20:15 UTC - kode54
- Added more keys

2009-04-07 03:56 UTC - kode54
- Added another key

2009-04-07 00:54 UTC - kode54
- Added new ADX keys

2009-04-05 05:59 UTC - kode54
- Read cut-off frequency from file header
- Implemented encrypted file support from vgmstream
- Version is now 1.8

2008-02-04 03:45 UTC - kode54
- Filter coefficients calculated correctly from sample rate and cut-off frequency

2006-09-19 12:42 UTC - kode54
- Fixed loop end sample truncation
- Oops, now version info exposes correct version number
- Changed description
- Fixed long standing bug in seeking that was masked by fade on seek
- Version is now 1.6

2005-12-18 07:49 UTC - kode54
- Added support for files with 32 bytes of padding before the header (Sonic Mega Collection)
- Version is now 1.5

2005-11-17 10:38 UTC - kode54
- Added version field (?) detection for loop control, and handled potential newer (?) version (from in_adx)
- Version is now 1.4

2005-10-21 19:31 UTC - kode54
- Fixed seeking by swallowing whole sets of decoded samples instead of single blocks of 32
- Version is now 1.3

2005-01-09 03:37 UTC - kode54
- Fixed (?) looping where end points were not on a sample block boundary
- Version is now 1.2

2004-12-28 00:25 UTC - kode54
- Removed clipping and altered to decode straight to floating point (spoony SH2 soundtrack clips like mad without this)
- Version is now 1.1

2004-10-08 12:14 UTC - kode54
- Initial release
- Version is now 1.0

*/

#include <foobar2000.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef FOO_ADPCM_EXPORTS

#include "../resource.h"
extern advconfig_checkbox_factory cfg_loop;

#else

#include "resource.h"
static cfg_int cfg_loop("adx_loop", 0);

#endif

typedef struct {
	int s1,s2;
} PREV;

typedef struct {
	PREV prev[2];
} ADXContext;

#define    BASEVOL   0x4000
#define    SCALE1    0x7298
#define    SCALE2    0x3350

//#define    CLIP(s)    if (s>32767) s=32767; else if (s<-32768) s=-32768

static void adx_decode(t_int32 *out,const unsigned char *in,PREV *prev, const short * coeff, const t_uint16 xor)
{
	int scale = (((in[0]<<8)|(in[1]))^xor)+1;
	int i;
	int s0,s1,s2,d;

	int coeff1 = coeff [0];
	int coeff2 = coeff [1];

	//    printf("%x ",scale);

	in+=2;
	s1 = prev->s1;
	s2 = prev->s2;
	for(i=0;i<16;i++) {
		d = (signed char)in[i];
		// d>>=4; if (d&8) d-=16;
		d >>= 4;
		s0 = d*scale + ( ( coeff1 * s1 + coeff2 * s2 ) >> 12 );
		//CLIP(s0);
		*out++=s0;
		s2 = s1;
		s1 = s0;

		d = (signed char)in[i];
		//d&=15; if (d&8) d-=16;
		d = (signed char)(d<<4) >> 4;
		s0 = d*scale + ( ( coeff1 * s1 + coeff2 * s2 ) >> 12 );
		//CLIP(s0);
		*out++=s0;
		s2 = s1;
		s1 = s0;
	}
	prev->s1 = s1;
	prev->s2 = s2;

}

static void adx_decode_stereo(t_int32 *out,const unsigned char *in,PREV *prev, const short * coeff, const t_uint16 xor1, const t_uint16 xor2)
{
	t_int32 tmp[32*2];
	int i;

	adx_decode(tmp   ,in   ,prev,   coeff, xor1);
	adx_decode(tmp+32,in+18,prev+1, coeff, xor2);
	for(i=0;i<32;i++) {
		out[i*2]   = tmp[i];
		out[i*2+1] = tmp[i+32];
	}
}

#ifndef NDEBUG
#include <stdarg.h>
void dprintf( const TCHAR * fmt, ... )
{
	TCHAR temp[256];
	va_list list;
	va_start(list,fmt);
	_vsntprintf(temp,256,fmt,list);
	OutputDebugString(temp);
	va_end(list);
}
#endif

static inline t_uint32 read_long( const t_uint8 * ptr )
{
	return pfc::byteswap_if_le_t( * ( ( t_uint32 * ) ptr ) );
}

static int find_key(service_ptr_t<file> & p_file, t_uint16 & xor_start, t_uint16 & xor_mult, t_uint16 & xor_add, abort_callback & p_abort);

class input_adx
{
	service_ptr_t<file>        m_file;
	ADXContext               * context;

	pfc::array_t<t_uint8>      data_buffer;

	unsigned srate, nch, size;

	unsigned head_skip, offset;
	t_uint64 pos, swallow;

	/*
	unsigned pos, filled, swallow;
	int loop_start;

	bool eof;
	*/

	/* for loops: */
	bool                       loop;
	unsigned                   loop_start;
	unsigned                   loop_start_offset;
	unsigned                   loop_end;
	unsigned                   loop_swallow;
	ADXContext               * loop_context;

	short                      coeff [2];

	bool                       encoded;
	t_uint16                   xor_start;
	t_uint16                   xor_mult;
	t_uint16                   xor_add;
	t_uint16                   xor;
	t_uint16                   xor_loop_start;

	inline void next_key()
	{
		xor = ( xor * xor_mult + xor_add ) & 0x7FFF;
	}

public:
	input_adx() : context(0), loop_context(0), srate(0) {}

	~input_adx()
	{
		if (loop_context) delete loop_context;
		if (context) delete context;
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;
	}

private:
	void open_internal( abort_callback & p_abort )
	{
		t_filesize len = m_file->get_size( p_abort );

		if ( len < 4 ) throw exception_io_data();

		data_buffer.grow_size( 4 );

		head_skip = 0;

		t_uint8 * ptr = data_buffer.get_ptr();

		{
			static const t_uint8 signature[] = { '(', 'c', ')', 'C', 'R', 'I' };
			m_file->read_object(ptr, 4, p_abort);
			if ( ptr[0] != 0x80 )
			{
				m_file->seek( 0x20, p_abort );
				m_file->read_object( ptr, 4, p_abort );
				if ( ptr[0] != 0x80 ) throw exception_io_data();
				head_skip = 0x20;
			}
			offset = ( read_long( ptr ) & ~0x80000000 ) + 4;
			if ( t_filesize( offset ) >= len ) throw exception_io_data();
			if ( offset < 12 + 4 + 6 ) throw exception_io_data(); // need at least srate/size/signature
			data_buffer.grow_size( offset );
			ptr = data_buffer.get_ptr();
			m_file->read_object( ptr + 4, offset - 4, p_abort );
			if ( memcmp( ptr + offset - 6, signature, 6 ) ) throw exception_io_data();
			if ( ptr [4] != 3 || ptr [5] != 18 || ptr [6] != 4 ) throw exception_io_data();
			nch = ptr[7];
			if ( nch < 1 || nch > 2 ) throw exception_io_data();
		}

		srate = read_long( ptr + 8 );
		size = read_long( ptr + 12 );

		unsigned version = 0;
		
		if ( offset >= 16 + 4 + 6 ) version = ptr [18] * 0x100 + ptr [19];

		loop_start = ~0;

		// Encryption
		encoded = false;
		if ( version == 0x0408 )
		{
			if ( find_key( m_file, xor_start, xor_mult, xor_add, p_abort ) )
			{
				encoded = true;
				version = 0x0400;
			}
		}

		if ( version == 0x0300 )
		{
			if ( ( offset >= 0x28 + 4 + 6 ) && ( read_long( ptr + 0x18 ) == 1 ) )
			{
				loop_start = read_long( ptr + 0x1C );
				loop_start_offset = read_long( ptr + 0x20 );
				loop_end = read_long( ptr + 0x24 );

				if ( loop_start >= loop_end || loop_end > size ) loop_start = ~0;
			}
		}
		else if ( version == 0x0400 )
		{
			if ( ( offset >= 0x34 + 4 + 6 ) && ( read_long( ptr + 0x24 ) == 1 ) )
			{
				loop_start = read_long( ptr + 0x28 );
				loop_start_offset = read_long( ptr + 0x2C );
				loop_end = read_long( ptr + 0x30 );

				if ( loop_start >= loop_end || loop_end > size ) loop_start = ~0;
			}
		}

		if ( ! srate || ! size ) throw exception_io_data();

		double x,y,a,b,c;

		x = ptr [16] * 0x100 + ptr [17];
		y = srate;

		a = M_SQRT2 - cos( 2.0 * M_PI * x / y );
		b = M_SQRT2 - 1.0;
		c = ( a - sqrt( ( a + b ) * ( a - b ) ) ) / b;

		coeff [0] = floor( c * 8192.0 );
		coeff [1] = floor( c * c * -4096.0 );
	}

public:
	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! srate ) open_internal( p_abort );

		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", nch );
		p_info.info_set_int( "bitspersample", 4 );
		p_info.info_set_int( "decoded_bitspersample", 16 );
		p_info.info_set( "codec", "ADX" );
		p_info.info_set( "encoding", "lossy" );
		p_info.info_set_int( "bitrate", (srate * nch * 18 * 8 + 16000) / (1000 * 32) );

		if (loop_start != ~0)
		{
			p_info.info_set_int( "adx_loop_start", loop_start );
			p_info.info_set_int( "adx_loop_end", loop_end );
		}

		p_info.set_length( double( size ) / double( srate ) );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! srate ) open_internal( p_abort );

		loop = cfg_loop && ! ( p_flags & input_flag_no_looping );

		if ( ! context ) context = new ADXContext;
		memset( context, 0, sizeof( * context ) );

		pos = 0;
		swallow = 0;

		m_file->seek( head_skip + offset, p_abort );

		if ( encoded )
		{
			xor = xor_start;
		}
		else
		{
			xor = 0;
		}
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (pos >= size) return false;

		pfc::static_assert_t< sizeof( audio_sample ) == sizeof( t_int32 ) >();

		data_buffer.grow_size( 18 * 32 * nch );
		p_chunk.set_data_size( 32 * 32 * nch );

		unsigned char * in = data_buffer.get_ptr();
		t_int32 * out = ( t_int32 * ) p_chunk.get_data();

		unsigned done, read, n;

		done = 0;

		{
more:
			while ( done < 32 * 32 )
			{
				p_abort.check();
				//dprintf(_T("reading\n"));
				read = m_file->read( in, 18 * 32 * nch, p_abort );
				if ( read < 18 * nch ) throw exception_io_data();

				//dprintf(_T("decoding %u bytes\n"),read);
				for ( n = 0; (pos < size) && (done < 32 * 32) && ((read - n) >= (18 * nch)); n += 18 * nch, pos += 32 )
				{
					//dprintf(_T("block: pos = %u, offset = %I64u\n"), pos, m_file->get_position_e(p_abort) - read + t_filesize( n ) );
					/*t_filesize current_offset = m_file->get_position_e(p_abort) - read + n;
					if ( current_offset != ( offset + pos * nch * 18 / 32 ) )
					{
						dprintf(_T("expected offset: %I64u\ncurrent offset: %I64u\n"), t_filesize( offset + pos * nch * 18 / 32 ), current_offset );
						assert(0);
					}*/
					if (loop && loop_start != ~0)
					{
						if (loop_start - pos < 32)
						{
							//dprintf(_T("loop start found\n"));
							if ( ! loop_context ) loop_context = new ADXContext;
							*loop_context = *context;
							loop_swallow = loop_start - pos;
							xor_loop_start = xor;
						}

						if (pos >= loop_end) break;
					}

					if (nch == 1)
					{
						adx_decode( out + done, in + n, context->prev, coeff, xor );
						if ( encoded ) next_key();
					}
					else
					{
						if ( encoded )
						{
							t_uint16 xor1 = xor;
							next_key();
							t_uint16 xor2 = xor;
							next_key();
							adx_decode_stereo( out + done * 2, in + n, context->prev, coeff, xor1, xor2 );
						}
						else
						{
							adx_decode_stereo( out + done * 2, in + n, context->prev, coeff, 0, 0 );
						}
					}
					done += 32; //, dprintf(_T("output\n"));
				}
				if (swallow)
				{
					if ( swallow >= done )
					{
						swallow -= done;
						done = 0;
					}
				}
				if (loop && loop_start != ~0)
				{
					if (pos >= loop_end) break;
				}
				if (pos >= size) break;
			}

			if (loop && loop_start != ~0)
			{
				if (pos >= loop_end)
				{
					unsigned swallow_end = pos - loop_end;
					pos = loop_start - loop_swallow;
					*context = *loop_context;
					xor = xor_loop_start;
					m_file->seek( head_skip + loop_start_offset, p_abort );
					if (swallow + swallow_end >= done)
					{
						swallow -= done - swallow_end - loop_swallow;
						goto more;
					}
					if (swallow || swallow_end)
					{
						out += swallow * nch;
						done -= swallow + swallow_end;
					}
					swallow = loop_swallow;
				}
				else
				{
					if (swallow)
					{
						out += swallow * nch;
						done -= swallow;
						swallow = 0;
					}
				}
			}
			else
			{
				if (swallow)
				{
					out += swallow * nch;
					done -= swallow;
					swallow = 0;
				}
			}

			if (!done) return false;

			if ( pos > size ) done -= pos - size;

			if (done)
			{
				p_chunk.set_sample_count(done);
				p_chunk.set_srate(srate);
				p_chunk.set_channels(nch);

				audio_math::convert_from_int32( out, done * nch, p_chunk.get_data(), 1 << 16 );

				return true;
			}
		}

		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		swallow = audio_math::time_to_samples( p_seconds, srate );
		if ( swallow > pos )
		{
			swallow -= pos;
			return;
		}
		pos = 0;
		m_file->seek( head_skip + offset, p_abort );
		memset(context, 0, sizeof(*context));
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
		return ! stricmp( p_extension, "adx" );
	}
};

DECLARE_FILE_TYPE("CRI ADX files", "*.ADX");

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

class config_adx : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "ADX decoder";}
	virtual const char * get_parent_name() {return "Input";}
};

#endif

/* guessadx stuff */

static struct {
	t_uint16 start,mult,add;
} keys[] = {
	/* Clover Studio (GOD HAND, Okami) */
	/* I'm pretty sure this is right, based on a decrypted version of some GOD HAND tracks. */
	/* Also it is the 2nd result from guessadx */
	{0x49e1,0x4a57,0x553d},

	/* Grasshopper Manufacture 0 (Blood+) */
	/* this is estimated */
	{0x5f5d,0x58bd,0x55ed},

	/* Grasshopper Manufacture 1 (Killer7) */
	/* this is estimated */
	{0x50fb,0x5803,0x5701},

	/* Grasshopper Manufacture 2 (Samurai Champloo) */
	/* confirmed unique with guessadx */
	{0x4f3f,0x472f,0x562f},

	/* Moss Ltd (Raiden III) */
	/* this is estimated */
	{0x66f5,0x58bd,0x4459},

	/* Sonic Team 0 (Phantasy Star Universe) */
	/* this is estimated */
	{0x5deb,0x5f27,0x673f},

    /* G.rev 0 (Senko no Ronde) */
	/* this is estimated */
	{0x46d3,0x5ced,0x474d},

	/* Sonic Team 1 (NiGHTS: Journey of Dreams) */
	/* this seems to be dead on, but still estimated */
	{0x440b,0x6539,0x5723},

	/* from guessadx (unique?), unknown source */
	{0x586d,0x5d65,0x63eb},

	/* Navel (Shuffle! On the Stage) */
	/* 2nd key from guessadx */
	{0x4969,0x5deb,0x467f},

	/* Success (Aoishiro) */
	/* 1st key from guessadx */
	{0x4d65,0x5eb7,0x5dfd},

	/* Sonic Team 2 (Sonic and the Black Knight) */
	/* confirmed unique with guessadx */
	{0x55b7,0x6191,0x5a77},

	/* (Enterbrain) Amagami */
	/* one of 32 from guessadx */
	{0x5a17,0x509f,0x5bfd},

    /* Yamasa (Yamasa Digi Portable: Matsuri no Tatsujin) */
    /* confirmed unique with guessadx */
    {0x4c01,0x549d,0x676f},

    /* Kadokawa Shoten (Fragments Blue) */
    /* confirmed unique with guessadx */
    {0x5803,0x4555,0x47bf},

    /* Namco (Soulcalibur IV) */
    /* confirmed unique with guessadx */
    {0x59ed,0x4679,0x46c9},

    /* G.rev 1 (Senko no Ronde DUO) */
    /* from guessadx */
    {0x6157,0x6809,0x4045},

    /* ASCII Media Works 0 (Nogizaka Haruka no Himitsu: Cosplay Hajimemashita) */
    /* 2nd from guessadx, other was {0x45ad,0x5f27,0x10fd} */
    {0x45af,0x5f27,0x52b1},

    /* D3 Publisher 0 (Little Anchor) */
    /* confirmed unique with guessadx */
    {0x5f65,0x5b3d,0x5f65},

    /* Marvelous 0 (Hanayoi Romanesque: Ai to Kanashimi) */
    /* 2nd from guessadx, other was {0x5562,0x5047,0x1433} */
    {0x5563,0x5047,0x43ed},

	/* Capcom (Mobile Suit Gundam: Gundam vs. Gundam NEXT PLUS) */
    /* confirmed unique with guessadx */
    {0x4f7b,0x4fdb,0x5cbf},

	/* Developer: Bridge NetShop
	 * Publisher: Kadokawa Shoten (Shoukan Shoujo: Elemental Girl Calling) */
    /* confirmed unique with guessadx */
    {0x4f7b,0x5071,0x4c61},

	/* Developer: Net Corporation
	 * Publisher: Tecmo (Rakushou! Pachi-Slot Sengen 6: Rio 2 Cruising Vanadis) */
    /* confirmed unique with guessadx */
    {0x53e9,0x586d,0x4eaf},
};

static const int key_count = sizeof(keys)/sizeof(keys[0]);

/* return 0 if not found, 1 if found and set parameters */
static int find_key(service_ptr_t<file> & p_file, t_uint16 & xor_start, t_uint16 & xor_mult, t_uint16 & xor_add, abort_callback & p_abort)
{
	pfc::array_t<t_uint16> scales;
	pfc::array_t<t_uint16> prescales;
	int bruteframe=0,bruteframecount=-1;
	int startoff, endoff;
	int rc = 0;
	unsigned char nch;

	p_file->seek( 0, p_abort );
	p_file->read_bendian_t( startoff, p_abort );
	startoff = ( startoff & 0x7fffffff ) + 4;
	p_file->seek( 7, p_abort );
	p_file->read_object_t( nch, p_abort );
	p_file->seek( 12, p_abort );
	p_file->read_bendian_t( endoff, p_abort );
	endoff = (endoff + 31) / 32 * 18 * nch + startoff;

	/* how many scales? */
	{
		int framecount=(endoff-startoff)/18;
		if (framecount<bruteframecount || bruteframecount<0)
			bruteframecount=framecount;
	}

	/* find longest run of nonzero frames */
	{
		int longest=-1,longest_length=-1;
		int i;
		int length=0;
		p_file->seek( startoff, p_abort );
		for (i=0;i<bruteframecount;i++) {
			static const unsigned char zeroes[18]={0};
			unsigned char buf[18];
			p_file->read_object_t( buf, p_abort );
			if (memcmp(zeroes,buf,18)) length++;
			else length=0;
			if (length > longest_length) {
				longest_length=length;
				longest=i-length+1;
				if (longest_length >= 0x8000) break;
			}
		}
		if (longest==-1) {
			return rc;
		}
		bruteframecount = longest_length;
		bruteframe = longest;
	}

	{
		/* try to guess key */
#define MAX_FRAMES (INT_MAX/0x8000)
		int scales_to_do;
		int key_id;

		/* allocate storage for scales */
		scales_to_do = (bruteframecount > MAX_FRAMES ? MAX_FRAMES : bruteframecount);
		scales.set_size(scales_to_do);
		/* prescales are those scales before the first frame we test
		* against, we use these to compute the actual start */
		if (bruteframe > 0) {
			int i;
			/* allocate memory for the prescales */
			prescales.set_size(bruteframe);
			/* read the prescales */
			for (i=0; i<bruteframe; i++) {
				p_file->seek( startoff + i * 18, p_abort );
				p_file->read_bendian_t( prescales [i], p_abort );
			}
		}

		/* read in the scales */
		{
			int i;
			for (i=0; i < scales_to_do; i++) {
				p_file->seek( startoff + ( bruteframe + i ) * 18, p_abort );
				p_file->read_bendian_t( scales [i], p_abort );
			}
		}

		/* guess each of the keys */
		for (key_id=0;key_id<key_count;key_id++) {
			/* test pre-scales */
			t_uint16 xor = keys[key_id].start;
			t_uint16 mult = keys[key_id].mult;
			t_uint16 add = keys[key_id].add;
			int i;

			for (i=0;i<bruteframe &&
				((prescales[i]&0x6000)==(xor&0x6000) ||
				prescales[i]==0);
			i++) {
				xor = xor * mult + add;
			}

			if (i == bruteframe)
			{
				/* test */
				for (i=0;i<scales_to_do &&
					(scales[i]&0x6000)==(xor&0x6000);i++) {
						xor = xor * mult + add;
				}
				if (i == scales_to_do)
				{
					xor_start = keys[key_id].start;
					xor_mult = keys[key_id].mult;
					xor_add = keys[key_id].add;

					rc = 1;
					return rc;
				}
			}
		}
	}

	return rc;
}

static input_singletrack_factory_t<input_adx> g_input_adx_factory;

#ifndef FOO_ADPCM_EXPORTS
static config_factory<config_adx> g_config_adx_factory;
#endif

//DECLARE_COMPONENT_VERSION("ADX decoder", MY_VERSION, "Decodes CRI ADX files.");
