#ifndef _PLAY_CONTROL_H_
#define _PLAY_CONTROL_H_

#include "service.h"
#include "metadb_handle.h"
//important: you should call api commands declared in this header ONLY from main thread !!
//do not call them from a database_lock section, some commands (eg. those stopping playback) will deadlock if you do !


//note:
//all methods returning metadb_handle return add_ref'd handles, you have to handle_release them
//all methods taking metadb_handle will add_ref them if they store them, so it's your responsibility to later release handles you pass

class NOVTABLE play_control : public service_base
{
public:

	enum stop_reason {
		STOP_REASON_USER = 0,
		STOP_REASON_EOF,
		STOP_REASON_STARTING_ANOTHER,
	};


	enum track_command {
		TRACK_COMMAND_ABORT = -1,
		TRACK_COMMAND_DEFAULT = 0,
		TRACK_COMMAND_PLAY,
		TRACK_COMMAND_NEXT,
		TRACK_COMMAND_PREV,
		TRACK_COMMAND_SETTRACK,
		TRACK_COMMAND_RAND,
		TRACK_COMMAND_RESUME,
	};

	virtual bool get_now_playing(metadb_handle_ptr & p_out)=0;//will return 0 if not playing
	virtual void play_start(track_command cmd = TRACK_COMMAND_PLAY,bool paused = false)=0;
	virtual void play_stop(stop_reason reason = STOP_REASON_USER)=0;
	virtual bool is_playing()=0;
	virtual bool is_paused()=0;
	virtual double get_playback_time()=0;
	virtual void pause()=0;

	virtual void show_preferences(const GUID & p_page)=0;

	virtual bool get_stop_after_current()=0;
	virtual void set_stop_after_current(bool state)=0;
	virtual void set_volume(int)=0;
	virtual int get_volume()=0;//volume is in 0.01 dB
	virtual void volume_up()=0;
	virtual void volume_down()=0;
	virtual void volume_mute_toggle()=0;

	virtual void playback_seek(double param)=0;
	virtual void playback_seek_delta(double param)=0;
	virtual bool playback_can_seek()=0;
	virtual double playback_get_position()=0;

	//reminder: see titleformat.h for descriptions of titleformatting parameters
	virtual void playback_format_title(titleformat_hook * p_hook,string_base & out,const service_ptr_t<class titleformat_object> & p_script,titleformat_text_filter * p_filter)=0;
	
	virtual bool playback_format_title_ex(const metadb_handle_ptr & handle,titleformat_hook * p_hook,string_base & out,const service_ptr_t<class titleformat_object> & p_script,titleformat_text_filter * p_filter,bool b_include_time,bool b_include_dynamicinfo)=0;
	
	double playback_get_length();

	inline void toggle_stop_after_current() {set_stop_after_current(!get_stop_after_current());}

	static bool g_get(service_ptr_t<play_control> & p_out);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};


#endif