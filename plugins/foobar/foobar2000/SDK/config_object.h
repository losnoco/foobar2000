#ifndef _CONFIG_OBJECT_H_
#define _CONFIG_OBJECT_H_

class config_object;

class NOVTABLE config_object_notify_manager : public service_base
{
public:
	virtual void on_changed(const service_ptr_t<config_object> & p_object) = 0;
	static void g_on_changed(const service_ptr_t<config_object> & p_object);


	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class NOVTABLE config_object_read_callback
{
public:
	virtual t_io_result get_data(void * p_buffer,abort_callback & p_abort) = 0;
};

class NOVTABLE config_object_write_callback
{
public:
	virtual t_io_result set_data(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort) = 0;//must be called only once inside config_object::get_data()
};

class NOVTABLE config_object : public service_base
{
public:
	//interface
	virtual GUID get_guid() const = 0;
	virtual t_io_result get_data(config_object_write_callback * p_callback,abort_callback & p_abort) const = 0;
	virtual t_io_result set_data(config_object_read_callback * p_callback,unsigned p_bytes,abort_callback & p_abort,bool p_sendnotify = true) = 0;

	//helpers
	static bool g_find(service_ptr_t<config_object> & p_out,const GUID & p_guid);

	t_io_result set_data_raw(const void * p_data,unsigned p_bytes,bool p_sendnotify = true);
	t_io_result get_data_raw(void * p_out,unsigned p_bytes,unsigned & p_bytes_used);
	t_io_result get_data_raw_length(unsigned & p_bytes_used);

	template<class T> t_io_result get_data_struct_t(T& p_out);
	template<class T> t_io_result set_data_struct_t(const T& p_in);
	template<class T> static t_io_result g_get_data_struct_t(const GUID & p_guid,T & p_out);
	template<class T> static t_io_result g_set_data_struct_t(const GUID & p_guid,const T & p_in);

	t_io_result set_data_string(const char * p_data,unsigned p_length);
	t_io_result get_data_string(string_base & p_out);
	
	t_io_result get_data_bool(bool & p_out);
	t_io_result set_data_bool(bool p_val);
	t_io_result get_data_int32(t_int32 & p_out);
	t_io_result set_data_int32(t_int32 p_val);
	bool get_data_bool_simple(bool p_default);
	t_int32 get_data_int32_simple(t_int32 p_default);

	static t_io_result g_get_data_string(const GUID & p_guid,string_base & p_out);
	static t_io_result g_set_data_string(const GUID & p_guid,const char * p_data,unsigned p_length = infinite);

	static t_io_result g_get_data_bool(const GUID & p_guid,bool & p_out);
	static t_io_result g_set_data_bool(const GUID & p_guid,bool p_val);
	static t_io_result g_get_data_int32(const GUID & p_guid,t_int32 & p_out);
	static t_io_result g_set_data_int32(const GUID & p_guid,t_int32 p_val);
	static bool g_get_data_bool_simple(const GUID & p_guid,bool p_default);
	static t_int32 g_get_data_int32_simple(const GUID & p_guid,t_int32 p_default);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class standard_config_objects
{
public:
	static const GUID bool_remember_window_positions, bool_ui_always_on_top,bool_playlist_stop_after_current;
	static const GUID bool_playback_follows_cursor, bool_cursor_follows_playback;
	static const GUID string_gui_last_directory_media,string_gui_last_directory_playlists;
	static const GUID int32_dynamic_bitrate_display_rate;
};

class config_object_notify : public service_base
{
public:
	virtual unsigned get_watched_object_count() = 0;
	virtual GUID get_watched_object(unsigned p_index) = 0;
	virtual void on_watched_object_changed(const service_ptr_t<config_object> & p_object) = 0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

#endif _CONFIG_OBJECT_H_
