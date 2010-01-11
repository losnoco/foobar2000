#define MY_VERSION "1.4"

/*
	change log

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

#ifdef FOO_ADPCM_EXPORTS

#include "../resource.h"
extern cfg_int cfg_loop;

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

static void adx_decode(audio_sample *out,const unsigned char *in,PREV *prev)
{
	int scale = ((in[0]<<8)|(in[1]));
	int i;
	int s0,s1,s2,d;

	//    printf("%x ",scale);

	in+=2;
	s1 = prev->s1;
	s2 = prev->s2;
	for(i=0;i<16;i++) {
		d = (signed char)in[i];
		// d>>=4; if (d&8) d-=16;
		d >>= 4;
		s0 = (BASEVOL*d*scale + SCALE1*s1 - SCALE2*s2)>>14;
		//CLIP(s0);
		*out++=audio_sample(s0) * (1. / 32768.);
		s2 = s1;
		s1 = s0;

		d = (signed char)in[i];
		//d&=15; if (d&8) d-=16;
		d = (signed char)(d<<4) >> 4;
		s0 = (BASEVOL*d*scale + SCALE1*s1 - SCALE2*s2)>>14;
		//CLIP(s0);
		*out++=audio_sample(s0) * (1. / 32768.);
		s2 = s1;
		s1 = s0;
	}
	prev->s1 = s1;
	prev->s2 = s2;

}

static void adx_decode_stereo(audio_sample *out,const unsigned char *in,PREV *prev)
{
	audio_sample tmp[32*2];
	int i;

	adx_decode(tmp   ,in   ,prev);
	adx_decode(tmp+32,in+18,prev+1);
	for(i=0;i<32;i++) {
		out[i*2]   = tmp[i];
		out[i*2+1] = tmp[i+32];
	}
}

static unsigned read_long(const unsigned char *p)
{
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
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

class input_adx
{
	service_ptr_t<file>        m_file;
	ADXContext               * context;

	mem_block_t<unsigned char> data_buffer;

	unsigned srate, nch, size;

	unsigned offset, pos, swallow;

	/*
	unsigned pos, filled, swallow;
	int loop_start;

	bool eof;
	*/

	/* for loops: */
	unsigned                   loop_start;
	unsigned                   loop_start_offset;
	unsigned                   loop_end;
	unsigned                   loop_swallow;
	ADXContext               * loop_context;

public:
	input_adx() : context(0), loop_context(0), srate(0) {}

	~input_adx()
	{
		if (loop_context) delete loop_context;
		if (context) delete context;
	}

	t_io_result open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}
		else m_file = p_filehint;

		return io_result_success;
	}

private:
	t_io_result open_internal( abort_callback & p_abort )
	{
		t_filesize len = m_file->get_size_e( p_abort );

		if ( len < 4 ) return io_result_error_data;

		if ( ! data_buffer.check_size( 4 ) )
			return io_result_error_out_of_memory;

		unsigned char * ptr = data_buffer.get_ptr();

		//try
		{
			static const unsigned char signature[] = { '(', 'c', ')', 'C', 'R', 'I' };
			m_file->read_object_e(ptr, 0x4, p_abort);
			if ( ptr[0] != 0x80 ) return io_result_error_data;
			offset = ( read_long( ptr ) & ~0x80000000 ) + 4;
			if ( t_filesize( offset ) >= len ) return io_result_error_data;
			if ( offset < 12 + 4 + 6 ) return io_result_error_data; // need at least srate/size/signature
			if ( ! data_buffer.check_size( offset ) )
				return io_result_error_out_of_memory;
			ptr = data_buffer.get_ptr();
			m_file->read_object_e( ptr + 4, offset - 4, p_abort );
			if ( memcmp( ptr + offset - 6, signature, 6 ) ) return io_result_error_data;
			nch = ptr[7];
			if ( nch < 1 || nch > 2 ) return io_result_error_data;
		}
		//catch(exception_io const & e) {return e.get_code();}

		srate = read_long( ptr + 8 );
		size = read_long( ptr + 12 );

		unsigned version = 0;
		
		if ( offset >= 16 + 4 + 6 ) version = read_long( ptr + 16 );

		loop_start = ~0;

		if ( version == 0x01F40300 )
		{
			if ( ( offset >= 0x28 + 4 + 6 ) && ( read_long( ptr + 0x18 ) == 1 ) )
			{
				loop_start = read_long( ptr + 0x1C );
				loop_start_offset = read_long( ptr + 0x20 );
				loop_end = read_long( ptr + 0x24 );

				if ( loop_start >= loop_end || loop_end > size ) loop_start = ~0;
			}
		}
		else if ( version == 0x01F40400 )
		{
			if ( ( offset >= 0x34 + 4 + 6 ) && ( read_long( ptr + 0x24 ) == 1 ) )
			{
				loop_start = read_long( ptr + 0x28 );
				loop_start_offset = read_long( ptr + 0x2C );
				loop_end = read_long( ptr + 0x30 );

				if ( loop_start >= loop_end || loop_end > size ) loop_start = ~0;
			}
		}

		if ( ! srate || ! size ) return io_result_error_data;

		return io_result_success;
	}

public:
	t_io_result get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! srate )
		{
			t_io_result status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", nch );
		p_info.info_set_int( "bitspersample", 4 );
		p_info.info_set_int( "decoded_bitspersample", 16 );
		p_info.info_set( "codec", "ADX" );
		p_info.info_set_int( "bitrate", (srate * nch * 18 * 8 + 16000) / (1000 * 32) );

		if (loop_start != ~0)
		{
			p_info.info_set_int( "adx_loop_start", loop_start );
			p_info.info_set_int( "adx_loop_end", loop_end );
		}

		p_info.set_length( double( size ) / double( srate ) );

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		return m_file->get_stats( p_stats, p_abort );
	}

	t_io_result decode_initialize( unsigned p_flags,abort_callback & p_abort )
	{
		if ( ! srate )
		{
			t_io_result status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		if ( ! cfg_loop || ( p_flags & input_flag_no_looping ) ) loop_start = ~0;

		context = new ADXContext;
		if ( ! context ) return io_result_error_out_of_memory;
		memset( context, 0, sizeof( * context ) );

		pos = 0;
		swallow = 0;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		if (pos >= size) return io_result_eof;

		if ( ! data_buffer.check_size( 18 * 32 * nch ) ||
			! p_chunk.check_data_size(32 * 32 * nch) )
			return io_result_error_out_of_memory;

		unsigned char * in = data_buffer.get_ptr();
		audio_sample * out = p_chunk.get_data();

		unsigned done, read, n;

		done = 0;

		//try
		{
more:
			while (!p_abort.is_aborting() && done < 32 * 32)
			{
				//dprintf(_T("reading\n"));
				read = m_file->read_e(in, 18 * 32 * nch, p_abort);
				if (read < 18 * nch) return io_result_error_data;

				//dprintf(_T("decoding %u bytes\n"),read);
				for (n = 0; !p_abort.is_aborting() && (pos < size) && (done < 32 * 32) && ((read - n) >= (18 * nch)); n += 18 * nch, pos += 32)
				{
					//dprintf(_T("block: pos = %u, offset = %I64u\n"), pos, m_file->get_position_e(p_abort) - read + t_filesize( n ) );
					/*t_filesize current_offset = m_file->get_position_e(p_abort) - read + n;
					if ( current_offset != ( offset + pos * nch * 18 / 32 ) )
					{
						dprintf(_T("expected offset: %I64u\ncurrent offset: %I64u\n"), t_filesize( offset + pos * nch * 18 / 32 ), current_offset );
						assert(0);
					}*/
					if (loop_start != ~0)
					{
						if (loop_start - pos < 32)
						{
							//dprintf(_T("loop start found\n"));
							if (!loop_context) loop_context = new ADXContext;
							*loop_context = *context;
							loop_swallow = loop_start - pos;
						}

						if (pos >= loop_end) break;
					}

					if (nch == 1) adx_decode(out + done, in + n, context->prev);
					else adx_decode_stereo(out + done * 2, in + n, context->prev);
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
				if (loop_start != ~0)
				{
					if (pos >= loop_end) break;
				}
				if (pos >= size) break;
			}

			if (p_abort.is_aborting()) return io_result_aborted;

			if (loop_start != ~0)
			{
				if (pos >= loop_end)
				{
					swallow += pos - loop_end;
					pos = loop_start - loop_swallow;
					*context = *loop_context;
					m_file->seek_e(loop_start_offset, p_abort);
					if (swallow >= done)
					{
						swallow -= done;
						goto more;
					}
					if (swallow)
					{
						out += swallow * nch;
						done -= swallow;
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

			if (!done) return io_result_eof;

			if (pos > size) done -= pos - size;

			if (done)
			{
				p_chunk.set_sample_count(done);
				p_chunk.set_srate(srate);
				p_chunk.set_channels(nch);
				return io_result_success;
			}
		}
		//catch(exception_io const & e) {return e.get_code();}

		return io_result_error_generic;
	}

	t_io_result decode_seek( double p_seconds,abort_callback & p_abort )
	{
//		memset(context, 0, sizeof(*context));
		swallow = int( p_seconds * double( srate ) + .5 );
		if ( swallow > pos )
		{
			swallow -= pos;
			return io_result_success;
		}
		pos = 0;
		return m_file->seek( offset, p_abort );
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
		return ! stricmp( p_extension, "adx" );
	}
};

DECLARE_FILE_TYPE("Dreamcast ADX files", "*.ADX");

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

static input_singletrack_factory_t<input_adx> g_input_adx_factory;

#ifndef FOO_ADPCM_EXPORTS
static config_factory<config_adx> g_config_adx_factory;
#endif

DECLARE_COMPONENT_VERSION("ADX decoder", "1.0", "Decodes Dreamcast ADX files.");
