#define MYVERSION "1.6"

/*
	changelog

2011-11-30 01:14 UTC - kode54
- Removed interpolation modes and replaced with band-limited synthesis
- Version is now 1.6

2011-01-09 16:14 UTC - kode54
- Fixed beat to sample conversion so sample positions are calculated correctly for the full
  song lengths of all feasible .ORG files, even up to 192KHz
- Version is now 1.5

2011-01-08 20:38 UTC - kode54
- Added sample rate configuration
- Version is now 1.4

2010-12-30 15:42 UTC - kode54
- Cleaned up advanced preferences a bit
- Added linear and cubic interpolation methods
- Version is now 1.3

2010-12-28 21:50 UTC - kode54
- Added an option to enable or disable looping
- Version is now 1.2

2010-12-28 19:48 UTC - kode54
- Fixed a crash on input destruction when no decoder was allocated
- Fixed a potential crash on invalid file or file read error
- Version is now 1.1

2010-12-28 18:44 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

#include "liborganya/organya.h"
#include "liborganya/decoder.h"

// {0F3F30D3-19E2-49BC-AE54-90A8AACDBC5F}
static const GUID guid_cfg_parent_organya = 
{ 0xf3f30d3, 0x19e2, 0x49bc, { 0xae, 0x54, 0x90, 0xa8, 0xaa, 0xcd, 0xbc, 0x5f } };

// {E67B2244-D778-4B67-B8E0-63F851985DA5}
static const GUID guid_cfg_loop = 
{ 0xe67b2244, 0xd778, 0x4b67, { 0xb8, 0xe0, 0x63, 0xf8, 0x51, 0x98, 0x5d, 0xa5 } };

// {0EBB3585-F5DF-4730-B456-18ED6A4B1137}
static const GUID guid_cfg_sample_rate = 
{ 0xebb3585, 0xf5df, 0x4730, { 0xb4, 0x56, 0x18, 0xed, 0x6a, 0x4b, 0x11, 0x37 } };

advconfig_branch_factory cfg_organya_parent("Organya decoder", guid_cfg_parent_organya, advconfig_branch::guid_branch_playback, 0);

advconfig_checkbox_factory cfg_loop("Loop indefinitely", guid_cfg_loop, guid_cfg_parent_organya, 0, false);

advconfig_integer_factory cfg_sample_rate("Sample rate", guid_cfg_sample_rate, guid_cfg_parent_organya, 0, 44100, 8000, 192000 );

class input_org
{
	org_decoder_t * m_tune;

	t_filestats m_stats;

	unsigned length, sample_rate;

	size_t samples_decoded;

	bool first_frame;

	pfc::array_t< t_int16 > sample_buffer;

public:
	input_org()
	{
		m_tune = 0;
	}

	~input_org()
	{
		if ( m_tune ) org_decoder_destroy( m_tune );
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

		sample_rate = cfg_sample_rate;
		m_tune->state.sample_rate = sample_rate;

		length = org_decoder_get_total_samples( m_tune );
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set( "encoding", "synthesized" );
		p_info.info_set_int( "channels", 2 );
		p_info.set_length( ( double ) length / ( double ) sample_rate );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		org_decoder_seek_sample( m_tune, 0 );

		sample_buffer.set_size( 2048 );

		bool dont_loop = !cfg_loop || !! ( p_flags & input_flag_no_looping );

		m_tune->state.loop_count = dont_loop ? 1 : 0;

		first_frame = true;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort)
	{
		t_int16 * ptr = sample_buffer.get_ptr();

		samples_decoded = org_decode_samples( m_tune, ptr, 1024 );

		if ( samples_decoded ) p_chunk.set_data_fixedpoint( ptr, samples_decoded * 2 * 2, sample_rate, 2, 16, audio_chunk::channel_config_stereo );
		
		return !!samples_decoded;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		first_frame = true;
		unsigned sample = unsigned ( audio_math::time_to_samples( p_seconds, sample_rate ) );
		org_decoder_seek_sample( m_tune, sample );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
	{
		if ( first_frame )
		{
			first_frame = false;
			p_out.info_set_int( "samplerate", sample_rate );
			p_timestamp_delta = - ( ( double ) samples_decoded / ( double ) sample_rate );
			return true;
		}
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
