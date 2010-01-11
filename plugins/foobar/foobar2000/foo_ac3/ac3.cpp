// changelog

// 2004-03-07 15:47 UTC - kode54
// - Added VBR scanning threshold because scanning takes too damn long on xxbog hueg files
// - Restricted seektable and accurate seeking/scanning to VBR files, because its ficking
//   slow unless the files are small, or your drive is faaaaast
// - Version is now 0.6

// 2004-03-07 07:02 UTC - kode54
// - Added seek table manager from foo_input_std\mp3.cpp, now supports accurate seeking even with VBR files
// - Also skips over whole frames more efficiently
// - Load-time also scans over the entire file for accurate length even for VBR files

// 2004-01-14 16:50 UTC - kode54
// - Fixed playback to skip over blocks with unwanted bsid, but length and seeking will still be broken
// - Version is now 0.5.2

// 2003-12-23 12:30 UTC - kode54
// - Hotfix for new stereo and quad mappings
// - Version is now 0.5.1

// 2003-12-22 18:02 UTC - kode54
// - Added smaller speaker count mappings for mono, stereo, and quad without LFE
// - Version is now 0.5

// 2003-10-29 15:51 UTC - kode54
// - Added APEv2 tag writing
// - Version is now 0.4

// 2003-07-16 18:29 - kode54
// - Added sample accuracy to seeking
// - Version is now 0.3

// 2003-06-26 07:34 - kode54
// - Updated to 0.7 API
// - Version is now 0.2

#include <foobar2000.h>

#include <inttypes.h>
#include <a52.h>

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

static unsigned long Read_LE_Uint32_unsigned ( const unsigned char* p )
{
    return ((unsigned long)p[0] <<  0) |
           ((unsigned long)p[1] <<  8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

static unsigned long Read_LE_Uint32 ( const char* p ) {return Read_LE_Uint32_unsigned((const unsigned char*)p);}

class input_ac3
{
private:
	int srate, flags, bitrate, remain;
	bool is_seekable, noseeking;
	sample_t bias, level;

	bool do_dynrng;

	service_ptr_t<file> m_file;

	t_uint64 first_frame;
	t_uint64 total_frames;
	t_uint64 frames_done;
	t_uint64 skip_frames;
	t_uint32 skip_samples;

	a52_state_t * state;
	mem_block_t<t_uint8> buffer;

	bool sync_e( const service_ptr_t<file> & r, abort_callback & p_abort )
	{
		t_uint32 meh = 0;
		t_uint8 * ptr = buffer.get_ptr();
		int left = int( r->read_e( ptr + 7, 3840 - 7, p_abort ) + 7 );
retry:
		while ( left > 1 )
		{
			if (*ptr == 0xb && ptr[1] == 0x77) break;
			ptr++;
			left--;
		}
		if (left > 1)
		{
			r->seek2_e(-left, SEEK_CUR, p_abort);
			r->read_object_e(ptr = buffer.get_ptr(), 7, p_abort);
			if (ptr[5] >= 0x60 || (ptr[4] & 63) >= 38)
			{
				meh = 8192 - left;
				ptr += 7;
				left -= 7;
				r->seek2_e(left, SEEK_CUR, p_abort);
				goto retry;
			}

			return true;
		}
		else
		{
			if (meh)
			{
				uint32_t toread = meh;
				if (toread > 3840) toread = 3840;
				ptr -= 3840;
				left = r->read_e(ptr, toread, p_abort);
				meh -= toread;
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
	
	t_io_result open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		t_io_result status;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( m_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}
		else m_file = p_filehint;

		return io_result_success;
	}

	t_io_result get_info( file_info & p_info, abort_callback & p_abort )
	{
		static const char nch[] = {2,1,2,3,3,4,4,5,1,1,2};

		bool saved_offset = false;
		t_filesize offset;

		if ( state )
		{
			t_io_result status = m_file->get_position( offset, p_abort );
			if ( io_result_failed( status ) ) return status;
			saved_offset = true;
		}

		mem_block_t<t_uint8> buffer;
		if ( ! buffer.set_size(3840) )
			return io_result_error_out_of_memory;

		uint8_t * ptr = buffer.get_ptr();

		try
		{
			m_file->read_object_e(ptr, 7, p_abort);

			int flags, srate, bitrate;
			DWORD bs = a52_syncinfo(ptr, &flags, &srate, &bitrate);

			if ( ! bs )
			{
				if ( ! sync_e( m_file, p_abort ) ) return io_result_error_data;
				bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
			}

			t_filesize pos = m_file->get_position_e(p_abort);
			t_filesize end = m_file->get_size_e(p_abort);
			t_io_result status;
			status = tag_processor::read_trailing( m_file, p_info, p_abort );
			if ( status != io_result_error_not_found && status != io_result_error_data && io_result_failed( status ) ) return status;

			if ( status != io_result_error_not_found )
			{
				// tag is present, so let's just assume it's legal, huh.
				struct APETagFooterStruct   T;
				m_file->seek_e( end - ( sizeof T ), p_abort );
				m_file->read_object_e( &T, sizeof T, p_abort );
				end -= t_filesize( Read_LE_Uint32( T.Length ) );
				if ( T.Flags[3] & 0x80 ) end -= sizeof(T);
			}

			p_info.info_set_int( "bitrate", bitrate / 1000 );
			p_info.set_length( double( end ) / double( bitrate >> 3 ) );

			if ( saved_offset )
			{
				m_file->seek_e( offset, p_abort );
			}

			p_info.info_set( "codec", "ATSC A/52" );
			p_info.info_set_int( "channels", nch[ flags & A52_CHANNEL_MASK ] + !!( flags & A52_LFE ) );
			p_info.info_set_int( "samplerate", srate );
		}
		catch ( t_io_result code )
		{
			return code;
		}

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		return m_file->get_stats( p_stats, p_abort );
	}

	t_io_result decode_initialize( unsigned p_flags,abort_callback & p_abort )
	{
		state = a52_init(0);
		if (!state)
		{
			//console::info("Could not allocate new a52dec state.");
			return io_result_error_out_of_memory;
		}

		if ( ! buffer.set_size( 3840 ) )
			return io_result_error_out_of_memory;

		uint8_t * ptr = buffer.get_ptr();

		t_filesize end;

		try
		{
			m_file->seek_e( 0, p_abort );
			m_file->read_object_e(ptr, 7, p_abort);

			DWORD bs = a52_syncinfo(ptr, &flags, &srate, &bitrate);

			if ( ! bs )
			{
				if ( ! sync_e( m_file, p_abort ) ) return io_result_error_data;
				bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
			}

			first_frame = m_file->get_position_e( p_abort ) - 7;

			struct APETagFooterStruct   T;

			end = m_file->get_size_e(p_abort);

			m_file->seek_e( end - sizeof( T ), p_abort );
			m_file->read_object_e( & T, sizeof( T ), p_abort );
			m_file->seek_e( first_frame, p_abort );

			static t_uint8 signature[] = { 'A', 'P', 'E', 'T', 'A', 'G', 'E', 'X' };
			if ( !memcmp ( T.ID, signature, sizeof ( T.ID ) ) )
			{
				unsigned long Ver = Read_LE_Uint32 ( T.Version );
				if ( (Ver == 1000) || (Ver == 2000) )
				{
					end -= t_filesize( Read_LE_Uint32( T.Length ) );
					if ( T.Flags[ 3 ] & 0x80 ) end -= sizeof( T );
				}
			}
		}
		catch ( t_io_result code )
		{
			return code;
		}

		do_dynrng = !! cfg_dynrng;

		total_frames = end * t_uint64( srate ) / t_uint64( bitrate / 8 ) / 1536;

		bias = 0.;
		level = 1.;

		remain = 0;
		frames_done = 0;
		skip_frames = 0;
		skip_samples = 0;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		while (!remain && !p_abort.is_aborting())
		{
			if ( total_frames && frames_done >= total_frames )
			{
				//console::info(string_printf("Hit end of file. (current: %d, total: %d)", (int)frames_done, (int)total_frames));
				return io_result_eof;
			}

			uint8_t * ptr = buffer.get_ptr();

			try
			{
				m_file->read_object_e( ptr, 7, p_abort );
				DWORD bs = a52_syncinfo( ptr, &flags, &srate, &bitrate );
				if (!bs)
				{
					if ( ! sync_e( m_file, p_abort ) ) return io_result_error_data;
					bs = a52_syncinfo( ptr, & flags, & srate, & bitrate );
				}

				++frames_done;

				if ( skip_frames )
				{
					if ( m_file->can_seek() )
					{
						m_file->seek2_e( bs - 7, SEEK_CUR, p_abort );
					}
					else
					{
						m_file->read_object_e( ptr + 7, bs - 7, p_abort );
					}

					--skip_frames;

					continue;
				}

				m_file->read_object_e( ptr + 7, bs - 7, p_abort );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			if ( a52_frame( state, ptr, & flags, & level, bias) ) return io_result_error_data;
			if ( ! do_dynrng ) a52_dynrng(state, NULL, NULL);
			remain = 6;

			while ( skip_samples >= 256 && remain && !p_abort.is_aborting() )
			{
				--remain;
				a52_block( state );
				skip_samples -= 256;
			}
		}

		--remain;
		if ( a52_block( state ) ) return io_result_error_data;
		int meh;
		{
			meh = 0;
			switch (flags & A52_CHANNEL_MASK)
			{
			case A52_MONO:
				meh = 1;
				break;
			case A52_STEREO:
			case A52_CHANNEL:
			case A52_DOLBY:
				meh = 2;
				break;
			case A52_3F:
			case A52_2F1R:
				meh = 3;
				break;
			case A52_3F1R:
			case A52_2F2R:
				meh = 4;
				break;
			case A52_3F2R:
				meh = 5;
				break;
			}

			if (meh && flags & A52_LFE) meh++;
			if (!meh)
			{
				console::print( "Unsupported channel configuration" );
				return io_result_error_data;
			}
		}
		if ( ! p_chunk.check_data_size( ( 256 - skip_samples ) * meh ) )
			return io_result_error_out_of_memory;
		audio_sample * foo = p_chunk.get_data();
		sample_t * bar = a52_samples( state ) + skip_samples;
		if ( flags & A52_LFE )
		{
			switch ( flags & A52_CHANNEL_MASK )
			{
			case A52_MONO:
				p_chunk.set_channels(2, audio_chunk::channel_config_mono | audio_chunk::channel_lfe);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = *bar++;
				}
				break;

			case A52_STEREO:
			case A52_CHANNEL:
			case A52_DOLBY:
				p_chunk.set_channels(3, audio_chunk::channel_config_stereo | audio_chunk::channel_lfe);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[512];
					*foo++ = *bar++;
				}
				break;

			case A52_3F:
				p_chunk.set_channels(4, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[768];
					*foo++ = bar[512];
					*foo++ = *bar++;
				}
				break;

			case A52_2F1R:
				p_chunk.set_channels(4, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_lfe | audio_chunk::channel_back_center);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[512];
					*foo++ = *bar;
					*foo++ = bar[768];
					bar++;
				}
				break;

			case A52_3F1R:
				p_chunk.set_channels(5, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_back_center);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[768];
					*foo++ = bar[512];
					*foo++ = *bar;
					*foo++ = bar[1024];
					bar++;
				}
				break;

			case A52_2F2R:
				p_chunk.set_channels(5, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_lfe | audio_chunk::channel_back_left | audio_chunk::channel_back_right);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[512];
					*foo++ = *bar;
					*foo++ = bar[768];
					*foo++ = bar[1024];
					bar++;
				}
				break;

			case A52_3F2R:
				p_chunk.set_channels(6, audio_chunk::channel_config_5point1);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = bar[256];
					*foo++ = bar[768];
					*foo++ = bar[512];
					*foo++ = *bar;
					*foo++ = bar[1024];
					*foo++ = bar[1280];
					bar++;
				}
				break;
			}
		}
		else
		{
			switch (flags & A52_CHANNEL_MASK)
			{
			case A52_MONO:
				p_chunk.set_channels(1, audio_chunk::channel_config_mono);
				memcpy(bar, foo, (256 - skip_samples) * sizeof(audio_sample));
				break;

			case A52_STEREO:
			case A52_CHANNEL:
			case A52_DOLBY:
				p_chunk.set_channels(2, audio_chunk::channel_config_stereo);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[256];
					bar++;
				}
				break;

			case A52_2F2R:
				p_chunk.set_channels(4, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[256];
					*foo++ = bar[512];
					*foo++ = bar[768];
					bar++;
				}
				break;

			case A52_3F:
				p_chunk.set_channels(3, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[512];
					*foo++ = bar[256];
					bar++;
				}
				break;

			case A52_2F1R:
				p_chunk.set_channels(3, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_center);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[256];
					*foo++ = bar[512];
					bar++;
				}
				break;

			case A52_3F1R:
				p_chunk.set_channels(4, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_center);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[512];
					*foo++ = bar[256];
					*foo++ = bar[768];
					bar++;
				}
				break;

			case A52_3F2R:
				p_chunk.set_channels(5, audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right);
				for (meh = skip_samples; meh < 256; meh++)
				{
					*foo++ = *bar;
					*foo++ = bar[512];
					*foo++ = bar[256];
					*foo++ = bar[768];
					*foo++ = bar[1024];
					bar++;
				}
				break;
			}
		}
		p_chunk.set_srate(srate);
		p_chunk.set_sample_count(256 - skip_samples);
		skip_samples = 0;
		return io_result_success;
	}

	t_io_result decode_seek( double p_seconds,abort_callback & p_abort )
	{
		if ( ! m_file->can_seek() ) return io_result_error_data;
		if ( p_seconds < 0 ) p_seconds = 0;

		t_uint64 dest_sample = t_uint64( p_seconds * double( srate ) );

		t_uint64 dest_skip = dest_sample % 1536;
		t_uint64 dest_offset = ( dest_sample - dest_skip ) * ( bitrate >> 3 ) / srate;
		
		t_io_result status = m_file->seek( dest_offset, p_abort );
		if ( io_result_failed( status ) ) return status;

		skip_samples = t_uint32( dest_skip );

		//console::info(string_printf("skip %d samples", skip_samples));

		remain = 0;
		return io_result_success;
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
	}

	t_io_result retag( const file_info & p_info,abort_callback & p_abort )
	{
		t_io_result status;

		bool saved_offset = false;
		t_filesize offset;

		if ( state )
		{
			status = m_file->get_position( offset, p_abort );
			if ( io_result_failed( status ) ) return status;
			saved_offset = true;
		}

		status = tag_processor::write_apev2( m_file, p_info, p_abort );
		if ( io_result_failed( status ) ) return status;

		if ( saved_offset )
		{
			return m_file->seek( offset, p_abort );
		}
		return io_result_success;
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

static input_singletrack_factory_t<input_ac3>            g_input_factory_ac3;
static preferences_page_factory_t <preferences_page_ac3> g_preferences_page_factory_ac3;

DECLARE_COMPONENT_VERSION("AC3 decoder", "0.6", "Based on liba52 v0.7.4");
