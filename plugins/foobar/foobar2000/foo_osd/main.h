#ifndef _OSD_MAIN_H_
#define _OSD_MAIN_H_

#include "config.h"
#include "overlay.h"

class cfg_osd_list : public cfg_var
{
	bool                   initialized;

	ptr_list_t<osd_config> val;

	critical_section       sync;
	ptr_list_t<osd_state>  state;
	ptr_list_t<COsdWnd>    osd;

	virtual t_io_result get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	virtual t_io_result set_data_raw(stream_reader * p_stream,unsigned p_sizehint,abort_callback & p_abort);

public:
	cfg_osd_list(const GUID & p_guid);
	~cfg_osd_list();

	// reset settings to default
	void reset();

	// global overlay creation/cleanup
	bool init();
	void quit();

	// enumeration
	void get(unsigned n, osd_config & p_out);
	void get_names(array_t<string_simple> & p_out);
	//void get_all(array_t<osd_config> & p_out);
	void get_callback_flags(unsigned & p_play_callback_flags, unsigned & p_playlist_callback_flags);

	// modification
	void set(unsigned n, const osd_config & p_in);
	unsigned add(osd_config & p_in);
	unsigned del(unsigned n);
	void rename(unsigned n, const char * name);

	// hmm, I wonder...
	void test(unsigned n);

	// callback interface
	void on_playback_time();
	void on_volume_change(int new_val);
	void on_playback_dynamic_info(bool b_track_change);
	void on_playback_seek();
	void on_playback_pause(int _state);
	void on_playback_new_track(const metadb_handle_ptr & track);
	void on_playback_stop();
	void on_playlist_switch();

	// dynamic menu interface
	void show_track(unsigned n);
	void show_playlist(unsigned n);
	void show_volume(unsigned n);
	void hide(unsigned n);
};

extern cfg_osd_list g_osd;

#endif
