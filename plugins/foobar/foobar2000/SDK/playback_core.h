#ifndef _PLAYBACK_CORE_H_
#define _PLAYBACK_CORE_H_

#include "service.h"
#include "metadb_handle.h"
#include "replaygain.h"
#include "output_manager.h"

struct t_playback_item_userdata
{
	unsigned m_playlist,m_item;
};

struct t_playback_config
{
	bool m_process_total_time_counter;
	bool m_low_priority;
	bool m_process_dynamic_info;
	

	
	t_output_config m_output;

	t_playback_config() {reset();}

	t_playback_config(const t_playback_config & p_config) {*this = p_config;}

	
	void reset()
	{
		m_process_total_time_counter = false;
		m_low_priority = false;
		m_process_dynamic_info = true;
		m_output.reset();
	}
};

class NOVTABLE playback_file_lock : public service_base
{
public:
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

protected:
	inline playback_file_lock() {}
	inline ~playback_file_lock() {}
};

class NOVTABLE playback_core : public service_base
{
public:

	struct track
	{
		metadb_handle_ptr handle;
		t_playback_item_userdata user;
	};


	class NOVTABLE callback
	{
	public:
		enum t_file_error_handler
		{
			file_error_abort,
			file_error_continue,
			file_error_replace
		};
		virtual bool get_next_track(track * out)=0;//return false if end-of-playlist, true if out struct is filled with data
		virtual void on_file_open(const track * src) {}//called before attempting file open; use only for logging etc
		virtual void on_file_open_success(const track * ptr) {}
		virtual t_file_error_handler on_file_error(const track * src,track * replace,t_io_result p_code) = 0;
		virtual void on_track_change(const track * ptr) {};//called when currently played track has changed, some time after get_next_track
		virtual void on_dynamic_info(const file_info * info,bool b_track_change) {}
		virtual void on_exiting(bool error) {};//called when terminating (either end-of-last-track or some kind of error or stop())
		virtual bool on_decode_error(const track * p_src,double p_time,t_io_result p_code) = 0;//return true to continue decoding or false to stop
		
		//DO NOT call any playback_core methods from inside callback methods !
		//all callback methods are called from playback thread, you are responsible for taking proper anti-deadlock measures (eg. dont call SendMessage() directly)
		//safest way to handle end-of-playback: create a window from main thread, PostMessage to it when you get callbacks, execute callback handlers (and safely call playback_core api) from message handlers
	};

	class NOVTABLE vis_callback
	{
	public:
		virtual void on_data(class audio_chunk * chunk,double timestamp)=0;//timestamp == timestamp of first sample in the chunk, according to get_cur_time_fromstart() timing
		virtual void on_flush()=0;
	};

	struct stats
	{
		double decoding_position,playback_position,dsp_latency,output_latency;
	};


	//MUST be called first, immediately after creation (hint: create() does this for you)
	virtual void set_callback(callback * ptr)=0;

	virtual void set_vis_callback(vis_callback * ptr)=0;//optional

	virtual void set_config(const t_playback_config & data)=0;//call before starting playback

	virtual void start(bool paused = false)=0;
	virtual void stop()=0;
	virtual bool is_playing()=0;
	virtual bool is_finished()=0;
	virtual bool is_paused()=0;
	virtual void pause(int paused)=0;

	virtual bool can_seek()=0;
	virtual void seek(double time)=0;
	virtual void seek_delta(double time_delta)=0;

	virtual double get_cur_time()=0;
	virtual double get_cur_time_fromstart()=0;
	virtual bool get_cur_track(track * out)=0;
	virtual bool get_stats(stats * out)=0;

	virtual void set_replaygain_config(const t_replaygain_config & p_config) = 0;
	virtual void set_volume(double p_value) = 0;
	virtual void set_dsp_config(const dsp_chain_config & p_data) = 0;

	virtual bool lock_file(service_ptr_t<playback_file_lock> & p_lock,const char * p_path,abort_callback & p_abort) = 0;


	bool get_cur_track_handle(metadb_handle_ptr & p_out);
	double get_total_time();
	
	static bool g_create(service_ptr_t<playback_core> & p_out,callback * cb);
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
protected:
	inline playback_core() {}
	inline ~playback_core() {}
};

class NOVTABLE play_sound : public service_base
{
public:
	virtual void play(const metadb_handle_ptr & file)=0;//settings are optional; if non-zero, they will be passed to playback_core being used
	virtual void stop_all()=0;
	
	static bool g_get(service_ptr_t<play_sound> & p_out) {return service_enum_create_t(p_out,0);}

	static void g_play(const metadb_handle_ptr & file)
	{
		service_ptr_t<play_sound> ptr;
		if (g_get(ptr)) ptr->play(file);
	}

	static void g_stop_all()
	{
		service_ptr_t<play_sound> ptr;
		if (g_get(ptr)) ptr->stop_all();
	}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
protected:
	inline play_sound() {}
	inline ~play_sound() {}
};

#endif