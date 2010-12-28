#define MYVERSION "1.0"

/*
	changelog

2010-12-28 18:44 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

#include "liborganya/organya.h"
#include "liborganya/decoder.h"

class input_org
{
	org_decoder_t * m_tune;

	t_filestats m_stats;

	unsigned length;

	pfc::array_t< t_int16 > sample_buffer;

public:
	input_org()
	{
		m_tune = 0;
	}

	~input_org()
	{
		org_decoder_destroy( m_tune );
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( m_file.is_empty() ) filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );

		m_stats = m_file->get_stats( p_abort );

		pfc::string8 my_path = core_api::get_my_full_path();
		my_path.truncate( my_path.scan_filename() );
		my_path += "samples";

		m_tune = org_decoder_create( m_file, my_path, 1, p_abort );
		if ( !m_tune ) throw exception_io_data();

		length = org_decoder_get_total_samples( m_tune );
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set( "encoding", "synthesized" );
		p_info.info_set_int( "samplerate", 44100 );
		p_info.info_set_int( "channels", 2 );
		p_info.set_length( ( double ) length / 44100. );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		org_decoder_seek_sample( m_tune, 0 );

		sample_buffer.set_size( 2048 );

		bool dont_loop = !! ( p_flags & input_flag_no_looping );

		m_tune->state.loop_count = dont_loop ? 1 : 0;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort)
	{
		t_int16 * ptr = sample_buffer.get_ptr();

		size_t samples_decoded = org_decode_samples( m_tune, ptr, 1024 );

		if ( samples_decoded ) p_chunk.set_data_fixedpoint( ptr, samples_decoded * 2 * 2, 44100, 2, 16, audio_chunk::channel_config_stereo );
		
		return !!samples_decoded;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		unsigned sample = unsigned ( audio_math::time_to_samples( p_seconds, 44100 ) );
		org_decoder_seek_sample( m_tune, sample );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
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

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_data();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return !stricmp(p_extension, "ORG");
	}
};

static input_singletrack_factory_t<input_org>               g_input_hvl_factory;

DECLARE_FILE_TYPE("Organya files", "*.ORG");

DECLARE_COMPONENT_VERSION( "Organya decoder", MYVERSION, "Using liborganya revision a60e04ccf4a2.\n\nhttps://bitbucket.org/vspader/liborganya");

VALIDATE_COMPONENT_FILENAME( "foo_input_org.dll" );
