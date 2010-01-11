#ifndef _PLAY_CALLBACK_H_
#define _PLAY_CALLBACK_H_

#include "service.h"

#include "metadb_handle.h"
//callbacks for getting notified about playback status
//multithread safety: all play_callback api calls come from main thread.


class NOVTABLE play_callback
{
public:
	//! Playback process is being initialized. on_playback_new_track() should be called soon after this when first file is successfully opened for decoding.
	virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused)=0;
	//! Playback advanced to new track.
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track) = 0;
	//! Playback stopped.
	virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason)=0;
	//! User has seeked to specific time.
	virtual void FB2KAPI on_playback_seek(double p_time)=0;
	//! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state)=0;
	//! Called when currently played file gets edited.
	virtual void FB2KAPI on_playback_edited(metadb_handle_ptr p_track) = 0;
	//! Dynamic info (VBR bitrate etc) change.
	virtual void FB2KAPI on_playback_dynamic_info(const file_info & p_info) = 0;
	//! Per-track dynamic info (stream track titles etc) change. Happens less often than on_playback_dynamic_info().
	virtual void FB2KAPI on_playback_dynamic_info_track(const file_info & p_info) = 0;
	//! Called every second, for time display
	virtual void FB2KAPI on_playback_time(double p_time) = 0;
	//! User changed volume settings. Possibly called when not playing.
	//! @param p_new_val new volume level in dB; 0 for full volume.
	virtual void FB2KAPI on_volume_change(float p_new_val) = 0;

	enum {
		flag_on_playback_starting			= 1 << 0,
		flag_on_playback_new_track			= 1 << 1, 
		flag_on_playback_stop				= 1 << 2,
		flag_on_playback_seek				= 1 << 3,
		flag_on_playback_pause				= 1 << 4,
		flag_on_playback_edited				= 1 << 5,
		flag_on_playback_dynamic_info		= 1 << 6,
		flag_on_playback_dynamic_info_track	= 1 << 7,
		flag_on_playback_time				= 1 << 8,
		flag_on_volume_change				= 1 << 9,

		flag_on_playback_all = flag_on_playback_starting | flag_on_playback_new_track | 
			flag_on_playback_stop | flag_on_playback_seek | 
			flag_on_playback_pause | flag_on_playback_edited |
			flag_on_playback_dynamic_info | flag_on_playback_dynamic_info_track | flag_on_playback_time,
	};
protected:
	play_callback() {}
	~play_callback() {}
};

class NOVTABLE play_callback_manager : public service_base
{
public:
	//! Registers a play_callback object.
	//! @param p_callback Interface to register.
	//! @param p_flags Indicates which notifications are requested.
	virtual void FB2KAPI register_callback(play_callback * p_callback,unsigned p_flags,bool p_forward_status_on_register) = 0;
	//! Unregisters a play_callback object.
	//! @p_callback Previously registered interface to unregister.
	virtual void FB2KAPI unregister_callback(play_callback * p_callback) = 0;


	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	play_callback_manager() {}
	~play_callback_manager() {}
};

class play_callback_static : public service_base, public play_callback {
public:
	virtual unsigned get_flags() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	play_callback_static() {}
	~play_callback_static() {}
};

template<class T>
class play_callback_static_factory_t : public service_factory_single_t<play_callback_static,T> {};

#endif