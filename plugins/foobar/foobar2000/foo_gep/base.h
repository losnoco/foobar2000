#include <foobar2000.h>

#include <gme/Music_Emu.h>
#include <gme/Multi_Buffer.h>
#include <gme/Effects_Buffer.h>

#define ERRCHK(f) \
	{ \
		blargg_err_t err = (f); \
		if (err) \
		{ \
			console::info(err); \
			throw exception_io_data(err); \
		} \
	}

#define HEADER_STRING(i,n,f) if ((f)[0]) (i).meta_add((n), pfc::stringcvt::string_utf8_from_ansi((f), sizeof((f))))

class input_gep
{
protected:

	gme_t                 * emu;

	Simple_Effects_Buffer     * buffer;

	unsigned                    sample_rate;

	int                         subsong;
	//int                         voice_mask;

	int                         played;

	int                         tag_song_ms;
	int                         tag_fade_ms;
	/*int                         song_len;
	int                         fade_len;*/
	bool                        no_infinite;

	bool                        stop_on_errors;

	bool                        first_block;

	bool                        monitoring;

	bool                        effects_enable;
	int                         effects_bass;
	int                         effects_treble;
	int                         effects_echo_depth;

	pfc::array_t<blip_sample_t> sample_buffer;

	service_ptr_t<file>         m_file;
	pfc::string_simple          m_path;
	t_filestats                 m_stats;

	void handle_warning();

	void meta_add( file_info & p_info, const char * name, const char * value, t_size max );

	void monitor_start();
	void monitor_update();
	void monitor_stop();

	void setup_effects( bool echo = true );
	void setup_effects_2();

public:
	input_gep();
	~input_gep();

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort );

	unsigned get_subsong_count();
	t_uint32 get_subsong( unsigned p_index );

	//void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort );

	t_filestats get_file_stats( abort_callback & p_abort );

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort );
	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort );
	void decode_seek( double p_seconds, abort_callback & p_abort );
	bool decode_can_seek();
	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta );
	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta );
	void decode_on_idle( abort_callback & p_abort );

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort );
	void retag_commit( abort_callback & p_abort );

	static bool g_is_our_content_type( const char * p_content_type );
};