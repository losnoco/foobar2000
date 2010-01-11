#include <foobar2000.h>

#include <gme/Music_Emu.h>
#include <gme/Multi_Buffer.h>

#define ERRCHK(f) \
	{ \
		blargg_err_t err = (f); \
		if (err) \
		{ \
			console::info(err); \
			throw io_result_error_data; \
		} \
	}

#define HEADER_STRING(i,n,f) if ((f)[0]) (i).meta_add((n), pfc::stringcvt::string_utf8_from_ansi((f), sizeof((f))))

class input_gep
{
protected:

	Music_Emu                * emu;

	unsigned                   sample_rate;

	int                        subsong;
	int                        voice_mask;

	int                        played;

	int                        tag_song_ms;
	int                        tag_fade_ms;
	int                        song_len;
	int                        fade_len;
	bool                       no_infinite;

	mem_block_t<blip_sample_t> sample_buffer;

	service_ptr_t<file>        m_file;
	string_simple              m_path;
	t_filestats                m_stats;

	//H                          m_header;

public:
	input_gep();
	~input_gep();

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort );

	unsigned get_subsong_count();
	t_uint32 get_subsong( unsigned p_index );

	//t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort );

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort );

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort );
	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort );
	t_io_result decode_seek( double p_seconds, abort_callback & p_abort );
	bool decode_can_seek();
	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta );
	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta );
	void decode_on_idle( abort_callback & p_abort );

	t_io_result retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort );
	t_io_result retag_commit( abort_callback & p_abort );

	static bool g_is_our_content_type( const char * p_content_type );
};