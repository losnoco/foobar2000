#define MY_VERSION "0.9.10"

/*
	changelog

2018-01-26 02:57 UTC - kode54
- Hopefully fix the SSE2 compiler issue with some workaround switch
- Version is now 0.9.10

2018-01-13 03:58 UTC - kode54
- Updated to v1.4 SDK
- Version is now 0.9.9

2017-02-04 01:41 UTC - kode54
- Add link to about string
- Version is now 0.9.8

2012-08-30 00:56 UTC - kode54
- Added channel info reporting
- Version is now 0.9.7

2012-03-03 12:51 UTC - kode54
- Packet decoder now correctly disables dynamic range compression
- Version is now 0.9.6

2010-04-13 14:51 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 0.9.5

2010-01-11 04:40 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 0.9.4

2009-04-09 01:43 UTC - kode54
- Fixed AC3 parser and caller to handle EOF when there's extra data after the last packet.
- Version is now 0.9.3

2009-03-02 21:02 UTC - kode54
- Fixed AC3 parser to ignore end of file when refilling the buffer, caller handles EOF already.
- Version is now 0.9.2

2008-05-29 01:21 UTC - kode54
- Fixed AC3 parser to handle end of file.
- Version is now 0.9.1

2007-02-07 03:07 UTC - kode54
- Implemented a new buffering packet processor which includes CRC checking to hopefully avoid any
  further garbage data which resembles AC3 packet headers.
- Now uses tag processor on open to detect position of trailing (APEv2) tags, and corrals the decoder
  within the range detected from (first detected packet) <-> (beginning of tag or end of file).
- Version is now 0.9

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

#include "../ATLHelpers/ATLHelpers.h"

#include <inttypes.h>
#include <a52.h>
#include <mm_accel.h>

#include "ac3_parser.h"

#include "resource.h"

enum
{
	default_cfg_dynrng = 0
};

// {49A08985-F4FF-4610-85DC-E084731AD7E8}
static const GUID guid_cfg_dynrng = 
{ 0x49a08985, 0xf4ff, 0x4610, { 0x85, 0xdc, 0xe0, 0x84, 0x73, 0x1a, 0xd7, 0xe8 } };

static cfg_int cfg_dynrng(guid_cfg_dynrng, default_cfg_dynrng);

/*#define DBG(a) \
	OutputDebugString( # a); \
	a; \
	OutputDebugString("success");*/

static class init_a52
{
public:
	init_a52()
	{
		a52_imdct_init();
	}

	~init_a52() {}
} g_initializer;

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

void get_mode_description( int p_flags, pfc::string8 & p_out )
{
	static const char *ac3_mode_list[11] = {
		{ "Dual Mono" },
		{ "Mono" },
		{ "Stereo" },
		{ "3 front channels" },
		{ "2 front, 1 rear surround channel" },
		{ "3 front, 1 rear surround channel" },
		{ "2 front, 2 rear surround channels" },
		{ "3 front, 2 rear surround channels" },
		{ "Channel 1" },
		{ "Channel 2" },
		{ "Dolby Stereo" }
	};
	unsigned channel_mode = p_flags & A52_CHANNEL_MASK;
	if ( channel_mode < _countof(ac3_mode_list) ) p_out = ac3_mode_list[ channel_mode ];
	else { p_out = "Unknown mode ("; p_out += pfc::format_int( channel_mode ); p_out += ")"; }
	if ( p_flags & A52_LFE ) p_out += " + LFE";
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

class input_ac3 : public input_stubs
{
private:
	int srate, flags, bitrate; //, remain;
	bool is_seekable, noseeking;
	//sample_t bias, level;

	bool do_dynrng, ignore_broken_files;

	service_ptr_t<file> m_file_base;
	service_ptr_t<file> m_file;

	file_info_impl m_info;

	t_filesize m_size;
	t_filesize first_frame;

	t_uint64 total_frames;
	t_uint64 frames_done;
	//t_uint64 skip_frames;
	t_uint32 skip_samples;

	a52_state_t * state;
	parser_ac3 parser;

public:
	input_ac3()
	{
		state = 0;
	}

	~input_ac3()
	{
		if (state) a52_free(state);
	}
	
	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( p_filehint, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}

		m_size = p_filehint->get_size_ex( p_abort );

		try
		{
			tag_processor::read_trailing_ex( p_filehint, m_info, m_size, p_abort );
		}
		catch ( const exception_io_data & ) { }

		p_filehint->seek( 0, p_abort );

		unsigned block_size = parser.sync( p_filehint, flags, srate, bitrate, p_abort );

		service_ptr_t<reader_limited> p_file = new service_impl_t<reader_limited> ();
		p_file->init( p_filehint, parser.packet_offset( p_filehint->get_position( p_abort ) ), m_size, p_abort );

		m_file_base = p_filehint;
		m_file = p_file;

		total_frames = m_file->get_size_ex( p_abort ) / block_size;

		m_info.info_set_int( "bitrate", bitrate / 1000 );
		m_info.set_length( double( total_frames * 1536 ) / double( srate ) );

		m_info.info_set( "codec", "ATSC A/52" );
		m_info.info_set_int( "channels", get_channels( flags ) );
		m_info.info_set_int( "samplerate", srate );
		m_info.info_set( "encoding", "lossy" );

		pfc::string8 channel_mode;
		get_mode_description( flags, channel_mode );
		m_info.info_set( "channel_mode", channel_mode );
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		ignore_broken_files = ! ( p_flags & input_flag_testing_integrity );

		if (state) a52_free(state);

		state = a52_init( /*0 /*MM_ACCEL_DJBFFT*/ );
		if ( ! state ) throw std::bad_alloc();

		m_file->seek( 0, p_abort );

		parser.reset();

		do_dynrng = !! cfg_dynrng;

		//remain = 0;
		frames_done = 0;
		//skip_frames = 0;
		skip_samples = 0;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		try
		{
		sample_t level = 1.0, bias = 0;
		unsigned remain = 0;
		while (!remain)
		{
			p_abort.check();

			if ( total_frames && frames_done >= total_frames )
			{
				//console::info(string_printf("Hit end of file. (current: %d, total: %d)", (int)frames_done, (int)total_frames));
				return false;
			}

			parser.packet_flush();
			parser.sync( m_file, flags, srate, bitrate, p_abort );

			++frames_done;

			/*while ( skip_frames-- )
			{
				parser.packet_flush();
				parser.sync( m_file, flags, srate, bitrate, p_abort );
			}*/

			if ( a52_frame( state, parser.packet_get(), & flags, & level, bias ) ) throw exception_io_data();
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

		unsigned channels = get_channels( flags );
		unsigned samples = remain * 256 - skip_samples;
		p_chunk.set_data_size( samples * channels );
		p_chunk.set_channels( channels, get_speaker_config( flags ) );
		p_chunk.set_srate( srate );
		p_chunk.set_sample_count( samples );

		const sample_t * in = a52_samples( state );
		audio_sample * out = p_chunk.get_data();

		if ( a52_block( state ) ) throw exception_io_data();
		prepare_chunk( in + skip_samples, out, 256 - skip_samples, flags );
		out += ( 256 - skip_samples ) * channels;

		skip_samples = 0;

		for ( unsigned i = 1; i < remain; ++i )
		{
			if ( a52_block( state ) ) throw exception_io_data();
			prepare_chunk( in, out, 256, flags );
			out += 256 * channels;
		}

		return true;
		}
		catch ( const exception_io_data_truncation & )
		{
			return false;
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

		t_uint64 dest_sample = audio_math::time_to_samples( p_seconds, srate );

		frames_done = dest_sample / 1536;
		if ( frames_done < total_frames )
		{
			t_uint64 dest_skip = dest_sample % 1536;
			t_uint64 dest_offset = frames_done * 1536 * ( bitrate >> 3 ) / srate;

			m_file->seek( dest_offset, p_abort );
			parser.reset();

			skip_samples = t_uint32( dest_skip );

			//remain = 0;
		}
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
		tag_processor::write_apev2( m_file_base, p_info, p_abort );
		m_info.copy( p_info );
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return !stricmp( p_extension, "ac3" );
	}

	static GUID g_get_guid()
	{
		static const GUID guid = { 0xd11f990, 0xf41b, 0x4e45,{ 0x83, 0x89, 0x2f, 0x22, 0xa, 0x1e, 0x55, 0x33 } };
		return guid;
	}

	static const char * g_get_name()
	{
		return "AC3 decoder";
	}

	static GUID g_get_preferences_guid()
	{
		static const GUID guid = { 0x69c304f0, 0xb305, 0x40d8,{ 0x9c, 0xf6, 0x19, 0x1e, 0x92, 0x10, 0xa7, 0x4c } };
		return guid;
	}
};

class packet_decoder_ac3 : public packet_decoder_streamparse
{
	a52_state_t * m_state;
	bool m_dynrng, m_decode;
	int srate, nch, flags, bitrate;

public:
    packet_decoder_ac3()
    {
		m_state = NULL;
		m_dynrng = !!cfg_dynrng;
		cleanup();
    }

    ~packet_decoder_ac3()
    {
		cleanup();
    }

	void cleanup()
	{
		srate = 0;
		nch = 0;
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

		m_state = a52_init( /*MM_ACCEL_DJBFFT*/ );
		if ( ! m_state ) throw std::bad_alloc();

        const matroska_setup *setup = (const matroska_setup *)p_param2;
		srate = setup->sample_rate;
		nch = setup->channels;

		m_decode = p_decode;
	}

	virtual t_size set_stream_property( const GUID & p_type, t_size p_param1, const void * p_param2, t_size p_param2size )
	{
		return 0;
	}

	virtual void get_info( file_info & p_info )
	{
        p_info.info_set_int( "samplerate", srate );
        p_info.info_set_int( "channels", nch );
		p_info.info_set( "codec", "ATSC A/52" );
		p_info.info_set( "encoding", "lossy" );

		if ( bitrate )
		{
			pfc::string8 channel_mode;
			get_mode_description( flags, channel_mode );
			p_info.info_set( "channel_mode", channel_mode );
		}
	}

	virtual unsigned get_max_frame_dependency() { return 0; }
	virtual double get_max_frame_dependency_time() { return 0; }

	virtual void reset_after_seek() {}

	virtual void decode( const void * p_buffer, t_size p_bytes, audio_chunk & p_chunk, abort_callback & p_abort )
	{
		t_size temp;
		decode_ex( p_buffer, p_bytes, temp, p_chunk, p_abort );
	}

	virtual bool analyze_first_frame_supported() { return true; }
	virtual void analyze_first_frame( const void * p_buffer, t_size p_bytes, abort_callback & p_abort )
	{
		if ( p_bytes < 7 ) throw exception_io_data();
		int frame_size = a52_syncinfo( ( uint8_t * ) p_buffer, &flags, &srate, &bitrate );
		if ( !frame_size || frame_size != p_bytes ) throw exception_io_data();
	}

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
		if ( ! m_dynrng ) a52_dynrng( m_state, NULL, NULL );

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

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_DYNRNG, BN_CLICKED, OnButtonClick)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnButtonClick(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SendDlgItemMessage( IDC_DYNRNG, BM_SETCHECK, cfg_dynrng, FALSE );
	return FALSE;
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SendDlgItemMessage( IDC_DYNRNG, BM_SETCHECK, default_cfg_dynrng, FALSE );
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_dynrng = SendDlgItemMessage( IDC_DYNRNG, BM_GETCHECK, FALSE, FALSE );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return SendDlgItemMessage( IDC_DYNRNG, BM_GETCHECK, FALSE, FALSE ) != cfg_dynrng;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return input_ac3::g_get_name();}
	GUID get_guid() {return input_ac3::g_get_preferences_guid();}
	GUID get_parent_guid() {return guid_input;}
};

DECLARE_FILE_TYPE("AC3 files", "*.AC3");

static input_singletrack_factory_t< input_ac3 >              g_input_factory_ac3;
static packet_decoder_factory_t< packet_decoder_ac3 >        g_packet_decoder_factory_ac3;
static preferences_page_factory_t< preferences_page_myimpl > g_preferences_page_factory_ac3;

DECLARE_COMPONENT_VERSION("AC3 decoder", MY_VERSION, "Based on liba52 v0.7.4\n\nhttps://www.patreon.com/kode54");

VALIDATE_COMPONENT_FILENAME("foo_ac3.dll");
