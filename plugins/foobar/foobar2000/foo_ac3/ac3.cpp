#define MY_VERSION "0.8"

/*
	changelog

2006-10-08 11:26 UTC - kode54
- Moved common sample handling code to functions.
- Configured liba52 to use DJBFFT with ppro optimizations.
- Added packet decoder.
- Version is now 0.8

2005-12-04 20:07 UTC - kode54
- Fixed synchronization from get_info
- Version is now 0.7

2004-03-07 15:47 UTC - kode54
- Added VBR scanning threshold because scanning takes too damn long on xxbog hueg files
- Restricted seektable and accurate seeking/scanning to VBR files, because its ficking
  slow unless the files are small, or your drive is faaaaast
- Version is now 0.6

2004-03-07 07:02 UTC - kode54
- Added seek table manager from foo_input_std\mp3.cpp, now supports accurate seeking even with VBR files
- Also skips over whole frames more efficiently
- Load-time also scans over the entire file for accurate length even for VBR files

2004-01-14 16:50 UTC - kode54
- Fixed playback to skip over blocks with unwanted bsid, but length and seeking will still be broken
- Version is now 0.5.2

2003-12-23 12:30 UTC - kode54
- Hotfix for new stereo and quad mappings
- Version is now 0.5.1

2003-12-22 18:02 UTC - kode54
- Added smaller speaker count mappings for mono, stereo, and quad without LFE
- Version is now 0.5

2003-10-29 15:51 UTC - kode54
- Added APEv2 tag writing
- Version is now 0.4

2003-07-16 18:29 - kode54
- Added sample accuracy to seeking
- Version is now 0.3

2003-06-26 07:34 - kode54
- Updated to 0.7 API
- Version is now 0.2

*/

#include <foobar2000.h>

#include <inttypes.h>
#include <a52.h>
#include <mm_accel.h>

#include "resource.h"

// {49A08985-F4FF-4610-85DC-E084731AD7E8}
static const GUID guid_cfg_dynrng = 
{ 0x49a08985, 0xf4ff, 0x4610, { 0x85, 0xdc, 0xe0, 0x84, 0x73, 0x1a, 0xd7, 0xe8 } };

static cfg_int cfg_dynrng(guid_cfg_dynrng, 0);

/*#define DBG(a) \
	OutputDebugString( # a); \
	a; \
	OutputDebugString("success");*/

struct APETagFooterStruct {
    char   ID       [8];
    char   Version  [4];
    char   Length   [4];
    char   TagCount [4];
    char   Flags    [4];
    char   Reserved [8];
};

namespace pfc
{
	template<> class traits_t<::APETagFooterStruct> : public traits_rawobject {};
}

unsigned get_channels( int flags )
{
	static const uint8_t ch[] = { 2, 1, 2, 3, 3, 4, 4, 5, 0, 0, 2, 0, 0, 0, 0, 0 };
	return ch[ flags & 15 ] + ( ( flags & 16 ) >> 4 );
}

unsigned get_speaker_config( int flags )
{
	static const unsigned config[] =
	{
		audio_chunk::channel_config_stereo,
		audio_chunk::channel_config_mono,
		audio_chunk::channel_config_stereo,
		audio_chunk::channel_config_stereo | audio_chunk::channel_front_center,
		audio_chunk::channel_config_stereo | audio_chunk::channel_back_center,
		audio_chunk::channel_config_stereo | audio_chunk::channel_front_center | audio_chunk::channel_back_center,
		audio_chunk::channel_config_stereo | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
		audio_chunk::channel_config_stereo | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
		0, 0,
		audio_chunk::channel_config_stereo,
		0, 0, 0, 0, 0
	};
	pfc::static_assert_t< audio_chunk::channel_lfe == 8 >();
	return config[ flags & 15 ] + ( ( flags & 16 ) >> 1 );
}

void prepare_chunk( const sample_t * p_src, audio_sample * p_dst, unsigned sample_count, int flags )
{
	int i;
	const sample_t * src = p_src;
	audio_sample * dst = p_dst;
	if ( flags & A52_LFE )
	{
		switch ( flags & A52_CHANNEL_MASK )
		{
		case A52_MONO:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = *src++;
			}
			break;

		case A52_STEREO:
		case A52_CHANNEL:
		case A52_DOLBY:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[512];
				*dst++ = *src++;
			}
			break;

		case A52_3F:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[768];
				*dst++ = src[512];
				*dst++ = *src++;
			}
			break;

		case A52_2F1R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[512];
				*dst++ = *src;
				*dst++ = src[768];
				src++;
			}
			break;

		case A52_3F1R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[768];
				*dst++ = src[512];
				*dst++ = *src;
				*dst++ = src[1024];
				src++;
			}
			break;

		case A52_2F2R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[512];
				*dst++ = *src;
				*dst++ = src[768];
				*dst++ = src[1024];
				src++;
			}
			break;

		case A52_3F2R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = src[256];
				*dst++ = src[768];
				*dst++ = src[512];
				*dst++ = *src;
				*dst++ = src[1024];
				*dst++ = src[1280];
				src++;
			}
			break;
		}
	}
	else
	{
		switch (flags & A52_CHANNEL_MASK)
		{
		case A52_MONO:
			memcpy(dst, src, sample_count * sizeof(audio_sample));
			break;

		case A52_STEREO:
		case A52_CHANNEL:
		case A52_DOLBY:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[256];
				src++;
			}
			break;

		case A52_2F2R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[256];
				*dst++ = src[512];
				*dst++ = src[768];
				src++;
			}
			break;

		case A52_3F:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[512];
				*dst++ = src[256];
				src++;
			}
			break;

		case A52_2F1R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[256];
				*dst++ = src[512];
				src++;
			}
			break;

		case A52_3F1R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[512];
				*dst++ = src[256];
				*dst++ = src[768];
				src++;
			}
			break;

		case A52_3F2R:
			for (i = 0; i < sample_count; ++i)
			{
				*dst++ = *src;
				*dst++ = src[512];
				*dst++ = src[256];
				*dst++ = src[768];
				*dst++ = src[1024];
				src++;
			}
			break;
		}
	}
}

class input_ac3
{
private:
	int srate, flags, bitrate, remain;
	bool is_seekable, noseeking;
	sample_t bias, level;

	bool do_dynrng, ignore_broken_files;

	service_ptr_t<file> m_file;

	t_uint64 first_frame;
	t_uint64 total_frames;
	t_uint64 frames_done;
	t_uint64 skip_frames;
	t_uint32 skip_samples;

	a52_state_t * state;
	pfc::array_t<t_uint8> buffer;

	bool sync_e( const service_ptr_t<file> & r, t_uint8 * buffer, abort_callback & p_abort )
	{
		t_uint32 i = 0;
		t_uint8 * ptr = buffer;
		int left = int( r->read( ptr + 7, 3840 - 7, p_abort ) + 7 );
retry:
		while ( left > 1 )
		{
			if (*ptr == 0xb && ptr[1] == 0x77) break;
			ptr++;
			left--;
		}
		if (left > 1)
		{
			r->seek_ex(-left, file::seek_from_current, p_abort);
			r->read_object(ptr = buffer, 7, p_abort);
			if (ptr[5] >= 0x60 || (ptr[4] & 63) >= 38)
			{
				i = 8192 - left;
				ptr += 7;
				left -= 7;
				r->seek_ex(left, file::seek_from_current, p_abort);
				goto retry;
			}

			return true;
		}
		else
		{
			if (i)
			{
				uint32_t toread = i;
				if (toread > 3840) toread = 3840;
				ptr -= 3840;
				left = r->read(ptr, toread, p_abort);
				i -= toread;
				goto retry;
			}
			//console::info("Resync failed.");
			return false;
		}
	}

public:
	input_ac3()
	{
		state = 0;
	}
	~input_ac3()
	{
		if (state) a52_free(state);
	}
	
	void open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		static const char nch[] = {2,1,2,3,3,4,4,5,1,1,2};

		bool saved_offset = false;
		t_filesize offset;

		if ( state )
		{
			offset = m_file->get_position( p_abort );
			saved_offset = true;
		}

		pfc::array_t<t_uint8> buffer;
		buffer.set_size(3840);

		uint8_t * ptr = buffer.get_ptr();

		//try
		{
			m_file->read_object(ptr, 7, p_abort);

			int flags, srate, bitrate;
			DWORD bs = a52_syncinfo(ptr, &flags, &srate, &bitrate);

			if ( ! bs )
			{
				if ( ! sync_e( m_file, ptr, p_abort ) ) throw exception_io_data();
				bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
			}

			t_filesize pos = m_file->get_position(p_abort);
			t_filesize end = m_file->get_size(p_abort);
			try
			{
				tag_processor::read_trailing( m_file, p_info, p_abort );

				struct APETagFooterStruct   T;
				m_file->seek( end - ( sizeof T ), p_abort );
				m_file->read_object_t( T, p_abort );
				end -= t_filesize( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) & T.Length ) ) );
				if ( T.Flags[3] & 0x80 ) end -= sizeof(T);
			}
			catch ( const exception_io_data & ) {}

			p_info.info_set_int( "bitrate", bitrate / 1000 );
			p_info.set_length( double( end ) / double( bitrate >> 3 ) );

			if ( saved_offset ) m_file->seek( offset, p_abort );

			p_info.info_set( "codec", "ATSC A/52" );
			p_info.info_set_int( "channels", nch[ flags & A52_CHANNEL_MASK ] + !!( flags & A52_LFE ) );
			p_info.info_set_int( "samplerate", srate );
		}
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		ignore_broken_files = ! ( p_flags & input_flag_testing_integrity );

		if (state) a52_free(state);

		state = a52_init( MM_ACCEL_DJBFFT );
		if ( ! state ) throw std::bad_alloc();

		buffer.set_size( 3840 );

		uint8_t * ptr = buffer.get_ptr();

		t_filesize end;

		//try
		{
			m_file->seek( 0, p_abort );
			m_file->read_object(ptr, 7, p_abort);

			DWORD bs = a52_syncinfo(ptr, &flags, &srate, &bitrate);

			if ( ! bs )
			{
				if ( ! sync_e( m_file, ptr, p_abort ) ) throw exception_io_data();
				bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
			}

			first_frame = m_file->get_position( p_abort ) - 7;

			struct APETagFooterStruct   T;

			end = m_file->get_size(p_abort);

			m_file->seek( end - sizeof( T ), p_abort );
			m_file->read_object_t( T, p_abort );
			m_file->seek( first_frame, p_abort );

			static t_uint8 signature[] = { 'A', 'P', 'E', 'T', 'A', 'G', 'E', 'X' };
			if ( !memcmp ( T.ID, signature, sizeof ( T.ID ) ) )
			{
				unsigned Ver = pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &T.Version ) );
				if ( (Ver == 1000) || (Ver == 2000) )
				{
					end -= t_filesize( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) &T.Length ) ) );
					if ( T.Flags[ 3 ] & 0x80 ) end -= sizeof( T );
				}
			}
		}

		do_dynrng = !! cfg_dynrng;

		total_frames = end * t_uint64( srate ) / t_uint64( bitrate / 8 ) / 1536;

		bias = 0.;
		level = 1.;

		remain = 0;
		frames_done = 0;
		skip_frames = 0;
		skip_samples = 0;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		try
		{
		while (!remain)
		{
			p_abort.check();

			if ( total_frames && frames_done >= total_frames )
			{
				//console::info(string_printf("Hit end of file. (current: %d, total: %d)", (int)frames_done, (int)total_frames));
				return false;
			}

			uint8_t * ptr = buffer.get_ptr();

			//try
			{
				m_file->read_object( ptr, 7, p_abort );
				DWORD bs = a52_syncinfo( ptr, &flags, &srate, &bitrate );
				if (!bs)
				{
					if ( ! sync_e( m_file, ptr, p_abort ) ) throw exception_io_data();
					bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
				}

				++frames_done;

				if ( skip_frames )
				{
					if ( m_file->can_seek() )
					{
						m_file->seek_ex( bs - 7, file::seek_from_current, p_abort );
					}
					else
					{
						m_file->read_object( ptr + 7, bs - 7, p_abort );
					}

					--skip_frames;

					continue;
				}

				m_file->read_object( ptr + 7, bs - 7, p_abort );
			}

			if ( a52_frame( state, ptr, & flags, & level, bias) ) throw exception_io_data();
			if ( ! do_dynrng ) a52_dynrng(state, NULL, NULL);
			remain = 6;

			while ( skip_samples >= 256 && remain )
			{
				p_abort.check();
				--remain;
				if ( a52_block( state ) ) throw exception_io_data();
				skip_samples -= 256;
			}
		}

		--remain;
		if ( a52_block( state ) ) throw exception_io_data();

		p_chunk.set_data_size( ( 256 - skip_samples ) * get_channels( flags ) );
		p_chunk.set_channels( get_channels( flags ), get_speaker_config( flags ) );
		p_chunk.set_srate( srate );
		p_chunk.set_sample_count( 256 - skip_samples );
		prepare_chunk( a52_samples( state ) + skip_samples, p_chunk.get_data(), 256 - skip_samples, flags );

		skip_samples = 0;
		return true;
		}
		catch ( const exception_io_data & )
		{
			if ( ignore_broken_files ) return false;
			throw;
		}
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		if ( ! m_file->can_seek() ) throw exception_io_data();
		if ( p_seconds < 0 ) p_seconds = 0;

		t_uint64 dest_sample = audio_math::time_to_samples( p_seconds, srate );

		t_uint64 dest_skip = dest_sample % 1536;
		t_uint64 dest_offset = ( dest_sample - dest_skip ) * ( bitrate >> 3 ) / srate;
		
		m_file->seek( dest_offset, p_abort );

		skip_samples = t_uint32( dest_skip );

		remain = 0;
	}

	bool decode_can_seek()
	{
		return m_file.is_valid() ? m_file->can_seek() : false;
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

		if ( state )
		{
			offset = m_file->get_position( p_abort );
			saved_offset = true;
		}

		tag_processor::write_apev2( m_file, p_info, p_abort );

		if ( saved_offset ) m_file->seek( offset, p_abort );
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return !stricmp( p_extension, "ac3" );
	}
};

class packet_decoder_ac3 : public packet_decoder_streamparse
{
	a52_state_t * m_state;
	bool m_dynrng, m_decode;

public:
    packet_decoder_ac3()
    {
		m_state = NULL;
		m_dynrng = !!cfg_dynrng;
    }

    ~packet_decoder_ac3()
    {
		cleanup();
    }

	void cleanup()
	{
		if ( m_state )
		{
			a52_free( m_state );
			m_state = NULL;
		}
	}

	static bool g_is_our_setup( const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size )
	{
		if ( p_owner == owner_matroska )
		{
			if ( p_param2size == sizeof( matroska_setup ) )
			{
				const matroska_setup * setup = ( const matroska_setup * ) p_param2;
				if ( !strncmp( setup->codec_id, "A_AC3", 5 ) )
				{
					return true;
				}
			}
		}

		return false;
	}

	void open( const GUID & p_owner, bool p_decode, t_size p_param1, const void * p_param2, t_size p_param2size, abort_callback & p_abort )
	{
		assert( g_is_our_setup( p_owner, p_param1, p_param2, p_param2size ) );

		cleanup();

		m_state = a52_init( MM_ACCEL_DJBFFT );
		if ( ! m_state ) throw std::bad_alloc();

		m_decode = p_decode;
	}

	virtual t_size set_stream_property( const GUID & p_type, t_size p_param1, const void * p_param2, t_size p_param2size )
	{
		return 0;
	}

	virtual void get_info( file_info & p_info )
	{
		p_info.info_set( "codec", "ATSC A/52" );
	}

	virtual unsigned get_max_frame_dependency() { return 0; }
	virtual double get_max_frame_dependency_time() { return 0; }

	virtual void reset_after_seek() {}

	virtual void decode( const void * p_buffer, t_size p_bytes, audio_chunk & p_chunk, abort_callback & p_abort )
	{
		t_size temp;
		decode_ex( p_buffer, p_bytes, temp, p_chunk, p_abort );
	}

	virtual bool analyze_first_frame_supported() { return false; }
	virtual void analyze_first_frame( const void * p_buffer, t_size p_bytes, abort_callback & p_abort ) {}

	virtual void decode_ex( const void * p_buffer, t_size p_bytes, t_size & p_bytes_processed, audio_chunk & p_chunk, abort_callback & p_abort )
	{
		int frame_size = 0;
		int i = -1;
		int flags, sample_rate, bit_rate;
		while ( frame_size == 0 && i + 7 < p_bytes )
		{
			++i;
			frame_size = a52_syncinfo( ( uint8_t * ) p_buffer + i, &flags, &sample_rate, &bit_rate );
		}
		if ( !frame_size || frame_size + i < p_bytes ) throw exception_io_data();

		p_bytes_processed = frame_size + i;

		sample_t level = 1.0, bias = 0;

		if ( a52_frame( m_state, ( uint8_t * ) p_buffer + i, &flags, &level, bias ) ) throw exception_io_data();

		if ( ! m_decode )
		{
			for ( i = 0; i < 6; ++i )
			{
				if ( a52_block( m_state ) ) throw exception_io_data();
			}
		}
		else
		{
			int channels = get_channels( flags );
			p_chunk.set_data_size( 256 * 6 * channels );
			p_chunk.set_channels( channels, get_speaker_config( flags ) );
			p_chunk.set_srate( sample_rate );
			p_chunk.set_sample_count( 256 * 6 );

			const sample_t * in = a52_samples( m_state );
			audio_sample * out = p_chunk.get_data();

			for ( i = 0; i < 6; ++i )
			{
				if ( a52_block( m_state ) ) throw exception_io_data();
				prepare_chunk( in, out, 256, flags );
				out += 256 * channels;
			}
		}
	}

	virtual void analyze_first_frame_ex( const void * p_buffer, t_size p_bytes, t_size & p_bytes_processed, abort_callback & p_abort ) {}
};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_DYNRNG, BM_SETCHECK, cfg_dynrng, 0);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_DYNRNG:
			cfg_dynrng = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		}
		break;
	}
	return 0;
}

class preferences_page_ac3 : public preferences_page
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	GUID get_guid()
	{
		// {69C304F0-B305-40d8-9CF6-191E9210A74C}
		static const GUID guid = 
		{ 0x69c304f0, 0xb305, 0x40d8, { 0x9c, 0xf6, 0x19, 0x1e, 0x92, 0x10, 0xa7, 0x4c } };
		return guid;
	}
	virtual const char * get_name() {return "AC3 decoder";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_dynrng = 0;
	}
};

DECLARE_FILE_TYPE("AC3 files", "*.AC3");

static input_singletrack_factory_t< input_ac3 >           g_input_factory_ac3;
static packet_decoder_factory_t< packet_decoder_ac3 >     g_packet_decoder_factory_ac3;
static preferences_page_factory_t< preferences_page_ac3 > g_preferences_page_factory_ac3;

DECLARE_COMPONENT_VERSION("AC3 decoder", MY_VERSION, "Based on liba52 v0.7.4");
