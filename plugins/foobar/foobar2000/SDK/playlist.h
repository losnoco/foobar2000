#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

class NOVTABLE playlist_lock : public service_base {
public:
	enum 
	{
		filter_add		= 1 << 0,
		filter_remove	= 1 << 1,
		filter_reorder  = 1 << 2,
		filter_replace  = 1 << 3,
		filter_rename	= 1 << 4,
		filter_remove_playlist = 1 << 5,
		filter_default_action = 1 << 6,
	};

	virtual bool query_items_add(t_size start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;
	virtual bool query_items_reorder(const t_size * order,t_size count)=0;
	virtual bool query_items_remove(const bit_array & mask,bool p_force)=0;//if p_force is set, files have been physically removed and your return value is ignored
	virtual bool query_item_replace(t_size idx,const metadb_handle_ptr & p_old,const metadb_handle_ptr & p_new)=0;
	virtual bool query_playlist_rename(const char * p_new_name,t_size p_new_name_len) = 0;
	virtual bool query_playlist_remove() = 0;
	virtual bool execute_default_action(t_size p_item) = 0;
	virtual void on_playlist_index_change(t_size p_new_index) = 0;
	virtual void on_playlist_remove() = 0;
	virtual void get_lock_name(pfc::string_base & p_out) = 0;
	virtual void show_ui() = 0;
	virtual t_uint32 get_filter_mask() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_lock() {}
	~playlist_lock() {}
};

struct t_playback_queue_item {
	metadb_handle_ptr m_handle;
	t_size m_playlist,m_item;

	bool operator==(const t_playback_queue_item & p_item) const;
	bool operator!=(const t_playback_queue_item & p_item) const;
};

//important: playlist engine is SINGLE-THREADED. call any APIs not from main thread and things will either blow up or refuse to work. all callbacks can be assumed to come from main thread.

class NOVTABLE playlist_manager : public service_base
{
public:

	class NOVTABLE enum_items_callback
	{
	public:
		virtual bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected) = 0;//return false to stop
	};

	virtual t_size get_playlist_count() = 0;
	virtual t_size get_active_playlist() = 0;//infinite if there are no playlists, otherwise valid playlist index
	virtual void set_active_playlist(t_size p_index) = 0;
	virtual t_size get_playing_playlist() = 0;
	virtual void set_playing_playlist(t_size p_index) = 0;
	virtual bool remove_playlists(const bit_array & mask) = 0;
	virtual t_size create_playlist(const char * p_name,t_size p_name_len,t_size p_index) = 0;//p_index may be infinite to append the new playlist at the end of list; returns actual index of new playlist or infinite on failure (rare)
	virtual bool reorder(const t_size * p_order,t_size p_count) = 0;
	
	
	//retrieving status
	virtual t_size playlist_get_item_count(t_size p_playlist) = 0;
	virtual void playlist_enum_items(t_size p_playlist,enum_items_callback * p_callback,const bit_array & p_mask) = 0;
	virtual t_size playlist_get_focus_item(t_size p_playlist) = 0;//focus may be infinite if no item is focused
	virtual bool playlist_get_name(t_size p_playlist,pfc::string_base & p_out) = 0;
	
	//modifying playlist
	virtual bool playlist_reorder_items(t_size p_playlist,const t_size * order,t_size count) = 0;
	virtual void playlist_set_selection(t_size p_playlist,const bit_array & affected,const bit_array & status) = 0;
	virtual bool playlist_remove_items(t_size p_playlist,const bit_array & mask)=0;
	virtual bool playlist_replace_item(t_size p_playlist,t_size p_item,const metadb_handle_ptr & p_new_item) = 0;
	virtual void playlist_set_focus_item(t_size p_playlist,t_size p_item) = 0;
	virtual t_size playlist_insert_items(t_size p_playlist,t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection) = 0;
	virtual void playlist_ensure_visible(t_size p_playlist,t_size p_item) = 0;
	virtual bool playlist_rename(t_size p_index,const char * p_name,t_size p_name_len) = 0;

	virtual void playlist_undo_backup(t_size p_index) = 0;
	virtual bool playlist_undo_restore(t_size p_index) = 0;
	virtual bool playlist_redo_restore(t_size p_index) = 0;
	virtual bool playlist_is_undo_available(t_size p_playlist) = 0;
	virtual bool playlist_is_redo_available(t_size p_playlist) = 0;

	virtual void playlist_item_format_title(t_size p_playlist,t_size p_item,titleformat_hook * p_hook,pfc::string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter,play_control::t_display_level p_playback_info_level)=0;

	virtual bool get_playing_item_location(t_size * p_playlist,t_size * p_index) = 0;
	
//	virtual t_size playlist_get_playback_cursor(t_size p_playlist) = 0;
//	virtual void playlist_set_playback_cursor(t_size p_playlist,t_size p_cursor) = 0;

	virtual bool playlist_sort_by_format(t_size p_playlist,const char * spec,bool p_sel_only) = 0;

	//! p_items must be sorted by metadb::path_compare; use file_operation_callback static methods instead of calling this directly
	virtual void on_files_deleted_sorted(const pfc::list_base_const_t<const char *> & p_items) = 0;
	//! p_from must be sorted by metadb::path_compare; use file_operation_callback static methods instead of calling this directly
	virtual void on_files_moved_sorted(const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to) = 0;

	virtual bool playlist_lock_install(t_size p_playlist,const service_ptr_t<playlist_lock> & p_lock) = 0;//returns false when invalid playlist or already locked
	virtual bool playlist_lock_uninstall(t_size p_playlist,const service_ptr_t<playlist_lock> & p_lock) = 0;
	virtual bool playlist_lock_is_present(t_size p_playlist) = 0;
	virtual bool playlist_lock_query_name(t_size p_playlist,pfc::string_base & p_out) = 0;
	virtual bool playlist_lock_show_ui(t_size p_playlist) = 0;
	virtual t_uint32 playlist_lock_get_filter_mask(t_size p_playlist) = 0;


	virtual t_size playback_order_get_count() = 0;
	virtual const char * playback_order_get_name(t_size p_index) = 0;
	virtual GUID playback_order_get_guid(t_size p_index) = 0;
	virtual t_size playback_order_get_active() = 0;
	virtual void playback_order_set_active(t_size p_index) = 0;
	
	virtual void queue_remove_mask(bit_array const & p_mask) = 0;
	virtual void queue_add_item_playlist(t_size p_playlist,t_size p_item) = 0;
	virtual void queue_add_item(metadb_handle_ptr p_item) = 0;
	virtual t_size queue_get_count() = 0;
	virtual void queue_get_contents(pfc::list_base_t<t_playback_queue_item> & p_out) = 0;
	//! Returns index (0-based) on success, infinite on failure.
	virtual t_size queue_find_index(t_playback_queue_item const & p_item) = 0;

	virtual void register_callback(class playlist_callback * p_callback,unsigned p_flags) = 0;
	virtual void register_callback(class playlist_callback_single * p_callback,unsigned p_flags) = 0;
	virtual void unregister_callback(class playlist_callback * p_callback) = 0;
	virtual void unregister_callback(class playlist_callback_single * p_callback) = 0;
	virtual void modify_callback(class playlist_callback * p_callback,unsigned p_flags) = 0;
	virtual void modify_callback(class playlist_callback_single * p_callback,unsigned p_flags) = 0;

	virtual bool playlist_execute_default_action(t_size p_playlist,t_size p_item) = 0;

	
	//helpers
	void queue_flush();
	bool queue_is_active();

	bool highlight_playing_item();
	bool remove_playlist(t_size idx);
	bool remove_playlist_switch(t_size idx);//attempts to switch to another playlist after removing

	bool playlist_is_item_selected(t_size p_playlist,t_size p_item);
	bool playlist_get_item_handle(metadb_handle_ptr & p_out,t_size p_playlist,t_size p_item);

	bool playlist_move_selection(t_size p_playlist,int p_delta);
	void playlist_get_selection_mask(t_size p_playlist,bit_array_var & out);
	void playlist_get_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask);
	void playlist_get_all_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out);
	void playlist_get_selected_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out);
	
	void playlist_clear(t_size p_playlist);
	bool playlist_add_items(t_size playlist,const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);
	void playlist_clear_selection(t_size p_playlist);
	void playlist_remove_selection(t_size p_playlist,bool p_crop = false);
	
	
	//retrieving status
	t_size activeplaylist_get_item_count();
	void activeplaylist_enum_items(enum_items_callback * p_callback,const bit_array & p_mask);
	t_size activeplaylist_get_focus_item();//focus may be infinite if no item is focused
	bool activeplaylist_get_name(pfc::string_base & p_out);

	//modifying playlist
	bool activeplaylist_reorder_items(const t_size * order,t_size count);
	void activeplaylist_set_selection(const bit_array & affected,const bit_array & status);
	bool activeplaylist_remove_items(const bit_array & mask);
	bool activeplaylist_replace_item(t_size p_item,const metadb_handle_ptr & p_new_item);
	void activeplaylist_set_focus_item(t_size p_item);
	t_size activeplaylist_insert_items(t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);
	void activeplaylist_ensure_visible(t_size p_item);
	bool activeplaylist_rename(const char * p_name,t_size p_name_len);

	void activeplaylist_undo_backup();
	bool activeplaylist_undo_restore();
	bool activeplaylist_redo_restore();

	bool activeplaylist_is_item_selected(t_size p_item);
	bool activeplaylist_get_item_handle(metadb_handle_ptr & item,t_size p_item);
	void activeplaylist_move_selection(int p_delta);
	void activeplaylist_get_selection_mask(bit_array_var & out);
	void activeplaylist_get_items(pfc::list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask);
	void activeplaylist_get_all_items(pfc::list_base_t<metadb_handle_ptr> & out);
	void activeplaylist_get_selected_items(pfc::list_base_t<metadb_handle_ptr> & out);
	void activeplaylist_clear();

	bool activeplaylist_add_items(const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);

	bool playlist_insert_items_filter(t_size p_playlist,t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);
	bool activeplaylist_insert_items_filter(t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);

	bool playlist_insert_locations(t_size p_playlist,t_size p_base,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);
	bool activeplaylist_insert_locations(t_size p_base,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);

	bool playlist_add_items_filter(t_size p_playlist,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);
	bool activeplaylist_add_items_filter(const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);

	bool playlist_add_locations(t_size p_playlist,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);
	bool activeplaylist_add_locations(const pfc::list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);

	void reset_playing_playlist();

	void activeplaylist_clear_selection();
	void activeplaylist_remove_selection(bool p_crop = false);

	void activeplaylist_item_format_title(t_size p_item,titleformat_hook * p_hook,pfc::string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter,play_control::t_display_level p_playback_info_level);

	void playlist_set_selection_single(t_size p_playlist,t_size p_item,bool p_state);
	void activeplaylist_set_selection_single(t_size p_item,bool p_state);

	t_size playlist_get_selection_count(t_size p_playlist,t_size p_max);
	t_size activeplaylist_get_selection_count(t_size p_max);

	bool playlist_get_focus_item_handle(metadb_handle_ptr & p_item,t_size p_playlist);
	bool activeplaylist_get_focus_item_handle(metadb_handle_ptr & item);

	t_size find_playlist(const char * p_name,t_size p_name_length);
	t_size find_or_create_playlist(const char * p_name,t_size p_name_length);

	bool activeplaylist_sort_by_format(const char * spec,bool p_sel_only);

	t_uint32 activeplaylist_lock_get_filter_mask();
	bool activeplaylist_is_undo_available();
	bool activeplaylist_is_redo_available();

	bool activeplaylist_execute_default_action(t_size p_item);

	void remove_items_from_all_playlists(const pfc::list_base_const_t<metadb_handle_ptr> & p_data);

	void active_playlist_fix();

	bool get_all_items(pfc::list_base_t<metadb_handle_ptr> & out);

	static bool g_get(service_ptr_t<playlist_manager> & p_out);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_manager() {}
	~playlist_manager() {}
};

class NOVTABLE playlist_callback
{
public:
	virtual void FB2KAPI on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;//inside any of these methods, you can call playlist APIs to get exact info about what happened (but only methods that read playlist state, not those that modify it)
	virtual void FB2KAPI on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count)=0;//changes selection too; doesnt actually change set of items that are selected or item having focus, just changes their order
	virtual void FB2KAPI on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)=0;//called before actually removing them
	virtual void FB2KAPI on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)=0;
	virtual void FB2KAPI on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state) = 0;
	virtual void FB2KAPI on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to)=0;//focus may be -1 when no item has focus; reminder: focus may also change on other callbacks
	
	virtual void FB2KAPI on_items_modified(t_size p_playlist,const bit_array & p_mask)=0;
	virtual void FB2KAPI on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level)=0;

	struct t_on_items_replaced_entry
	{
		t_size m_index;
		metadb_handle_ptr m_old,m_new;
	};

	virtual void FB2KAPI on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data)=0;

	virtual void FB2KAPI on_item_ensure_visible(t_size p_playlist,t_size p_idx)=0;

	virtual void FB2KAPI on_playlist_activate(t_size p_old,t_size p_new) = 0;
	virtual void FB2KAPI on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len) = 0;
	virtual void FB2KAPI on_playlists_reorder(const t_size * p_order,t_size p_count) = 0;
	virtual void FB2KAPI on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) = 0;
	virtual void FB2KAPI on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) = 0;
	virtual void FB2KAPI on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len) = 0;

	virtual void FB2KAPI on_default_format_changed() = 0;
	virtual void FB2KAPI on_playback_order_changed(t_size p_new_index) = 0;
	virtual void FB2KAPI on_playlist_locked(t_size p_playlist,bool p_locked) = 0;

	enum {
		flag_on_items_added					= 1 << 0,
		flag_on_items_reordered				= 1 << 1,
		flag_on_items_removing				= 1 << 2,
		flag_on_items_removed				= 1 << 3,
		flag_on_items_selection_change		= 1 << 4,
		flag_on_item_focus_change			= 1 << 5,
		flag_on_items_modified				= 1 << 6,
		flag_on_items_modified_fromplayback	= 1 << 7,
		flag_on_items_replaced				= 1 << 8,
		flag_on_item_ensure_visible			= 1 << 9,
		flag_on_playlist_activate			= 1 << 10,
		flag_on_playlist_created			= 1 << 11,
		flag_on_playlists_reorder			= 1 << 12,
		flag_on_playlists_removing			= 1 << 13,
		flag_on_playlists_removed			= 1 << 14,
		flag_on_playlist_renamed			= 1 << 15,
		flag_on_default_format_changed		= 1 << 16,
		flag_on_playback_order_changed		= 1 << 17,
		flag_on_playlist_locked				= 1 << 18,

		flag_all							= ~0,
	};
protected:
	playlist_callback() {}
	~playlist_callback() {}
};

class NOVTABLE playlist_callback_static : public service_base, public playlist_callback 
{
public:
	virtual unsigned get_flags() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_callback_static() {}
	~playlist_callback_static() {}
};

class NOVTABLE playlist_callback_single
{
public:
	virtual void FB2KAPI on_items_added(t_size start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;//inside any of these methods, you can call playlist APIs to get exact info about what happened (but only methods that read playlist state, not those that modify it)
	virtual void FB2KAPI on_items_reordered(const t_size * order,t_size count)=0;//changes selection too; doesnt actually change set of items that are selected or item having focus, just changes their order
	virtual void FB2KAPI on_items_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count)=0;//called before actually removing them
	virtual void FB2KAPI on_items_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count)=0;
	virtual void FB2KAPI on_items_selection_change(const bit_array & p_affected,const bit_array & p_state) = 0;
	virtual void FB2KAPI on_item_focus_change(t_size p_from,t_size p_to)=0;//focus may be -1 when no item has focus; reminder: focus may also change on other callbacks
	virtual void FB2KAPI on_items_modified(const bit_array & p_mask)=0;
	virtual void FB2KAPI on_items_modified_fromplayback(const bit_array & p_mask,play_control::t_display_level p_level)=0;
	virtual void FB2KAPI on_items_replaced(const bit_array & p_mask,const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data)=0;
	virtual void FB2KAPI on_item_ensure_visible(t_size p_idx)=0;

	virtual void FB2KAPI on_playlist_switch() = 0;
	virtual void FB2KAPI on_playlist_renamed(const char * p_new_name,t_size p_new_name_len) = 0;
	virtual void FB2KAPI on_playlist_locked(bool p_locked) = 0;

	virtual void FB2KAPI on_default_format_changed() = 0;
	virtual void FB2KAPI on_playback_order_changed(t_size p_new_index) = 0;

	enum {
		flag_on_items_added					= 1 << 0,
		flag_on_items_reordered				= 1 << 1,
		flag_on_items_removing				= 1 << 2,
		flag_on_items_removed				= 1 << 3,
		flag_on_items_selection_change		= 1 << 4,
		flag_on_item_focus_change			= 1 << 5,
		flag_on_items_modified				= 1 << 6,
		flag_on_items_modified_fromplayback = 1 << 7,
		flag_on_items_replaced				= 1 << 8,
		flag_on_item_ensure_visible			= 1 << 9,
		flag_on_playlist_switch				= 1 << 10,
		flag_on_playlist_renamed			= 1 << 11,
		flag_on_playlist_locked				= 1 << 12,
		flag_on_default_format_changed		= 1 << 13,
		flag_on_playback_order_changed		= 1 << 14,
		flag_all							= ~0,
	};
protected:
	playlist_callback_single() {}
	~playlist_callback_single() {}
};

class playlist_callback_single_static : public service_base, public playlist_callback_single
{
public:
	virtual unsigned get_flags() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_callback_single_static() {}
	~playlist_callback_single_static() {}
};

class NOVTABLE dropped_files_data {
public:
	virtual void set_paths(pfc::string_list_const const & p_paths) = 0;
	virtual void set_handles(const pfc::list_base_const_t<metadb_handle_ptr> & p_handles) = 0;
protected:
	dropped_files_data() {}
	~dropped_files_data() {}
};


class NOVTABLE playlist_incoming_item_filter : public service_base
{
public:
	virtual bool filter_items(const pfc::list_base_const_t<metadb_handle_ptr> & in,pfc::list_base_t<metadb_handle_ptr> & out) = 0;//sort / remove duplicates
	virtual bool process_locations(const pfc::list_base_const_t<const char*> & urls,pfc::list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_restrict_mask_overide, const char * p_exclude_mask_override,HWND p_parentwnd) = 0;
	virtual bool process_dropped_files(interface IDataObject * pDataObject,pfc::list_base_t<metadb_handle_ptr> & out,bool filter,HWND p_parentwnd) = 0;
	virtual bool process_dropped_files_check(interface IDataObject * pDataObject) = 0;
	virtual bool process_dropped_files_check_if_native(interface IDataObject * pDataObject) = 0;
	virtual interface IDataObject * create_dataobject(const pfc::list_base_const_t<metadb_handle_ptr> & data) = 0;
	virtual bool process_dropped_files_check_ex(interface IDataObject * pDataObject, DWORD * p_effect) = 0;
	virtual bool process_dropped_files_delayed(dropped_files_data & p_out,interface IDataObject * pDataObject) = 0;

	//helper
	bool process_location(const char * url,pfc::list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_mask,const char * p_exclude,HWND p_parentwnd);

	static bool g_get(service_ptr_t<playlist_incoming_item_filter> & p_out) {return service_enum_t<playlist_incoming_item_filter>().first(p_out);}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_incoming_item_filter() {}
	~playlist_incoming_item_filter() {}
};

class dropped_files_data_impl : public dropped_files_data {
public:
	dropped_files_data_impl() : m_is_paths(false) {}
	void set_paths(pfc::string_list_const const & p_paths) {
		m_is_paths = true;
		m_paths = p_paths;
	}
	void set_handles(const pfc::list_base_const_t<metadb_handle_ptr> & p_handles) {
		m_is_paths = false;
		m_handles = p_handles;
	}
	bool to_handles(pfc::list_base_t<metadb_handle_ptr> & p_out,bool p_filter,HWND p_parentwnd) {
		if (m_is_paths) {
			return static_api_ptr_t<playlist_incoming_item_filter>()->process_locations(m_paths,p_out,p_filter,0,0,p_parentwnd);
		} else {
			if (static_api_ptr_t<metadb_io>()->load_info_multi(m_handles,metadb_io::load_info_default,p_parentwnd,true) == metadb_io::load_info_aborted) return false;
			p_out = m_handles;
			return true;
		}
	}
private:
	pfc::string_list_impl m_paths;
	metadb_handle_list m_handles;
	bool m_is_paths;
};


class NOVTABLE playback_queue_callback : public service_base
{
public:
	enum t_change_origin {
		changed_user_added,
		changed_user_removed,
		changed_playback_advance,
	};
	virtual void on_changed(t_change_origin p_origin) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playback_queue_callback() {}
	~playback_queue_callback() {}
};

#endif