#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include "service.h"
#include "metadb_handle.h"
#include "play_control.h"

class NOVTABLE playlist_lock : public service_base
{
public:
	enum 
	{
		filter_add		= 1 << 0,
		filter_remove	= 1 << 1,
		filter_reorder  = 1 << 2,
		filter_replace  = 1 << 3,
		filter_rename	= 1 << 4
	};

	virtual bool query_items_add(unsigned start, const list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;
	virtual bool query_items_reorder(const unsigned * order,unsigned count)=0;
	virtual bool query_items_remove(const bit_array & mask,bool p_force)=0;//if p_force is set, files have been physically removed and your return value is ignored
	virtual bool query_item_replace(unsigned idx,const metadb_handle_ptr & p_old,const metadb_handle_ptr & p_new)=0;
	virtual bool query_playlist_rename(const char * p_new_name,unsigned p_new_name_len) = 0;
	virtual void on_playlist_index_change(unsigned p_new_index) = 0;
	virtual void on_playlist_remove() = 0;
	virtual void get_lock_name(string_base & p_out) = 0;
	virtual void show_ui() = 0;
	virtual t_uint32 get_filter_mask() = 0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};


//important: playlist engine is SINGLE-THREADED. call any APIs not from main thread and things will either blow up or refuse to work. all callbacks can be assumed to come from main thread.

class NOVTABLE playlist_manager : public service_base
{
public:

	class NOVTABLE enum_items_callback
	{
	public:
		virtual bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected) = 0;//return false to stop
	};

	virtual unsigned get_playlist_count() = 0;
	virtual unsigned get_active_playlist() = 0;//infinite if there are no playlists, otherwise valid playlist index
	virtual void set_active_playlist(unsigned p_index) = 0;
	virtual unsigned get_playing_playlist() = 0;
	virtual void set_playing_playlist(unsigned p_index) = 0;
	virtual bool remove_playlists(const bit_array & mask) = 0;
	virtual unsigned create_playlist(const char * p_name,unsigned p_name_len,unsigned p_index) = 0;//p_index may be infinite to append the new playlist at the end of list; returns actual index of new playlist or infinite on failure (rare)
	virtual bool reorder(const unsigned * p_order,unsigned p_count) = 0;
	
	
	//retrieving status
	virtual unsigned playlist_get_item_count(unsigned p_playlist) = 0;
	virtual void playlist_enum_items(unsigned p_playlist,enum_items_callback * p_callback,const bit_array & p_mask) = 0;
	virtual unsigned playlist_get_focus_item(unsigned p_playlist) = 0;//focus may be infinite if no item is focused
	virtual bool playlist_get_name(unsigned p_playlist,string_base & p_out) = 0;
	
	//modifying playlist
	virtual bool playlist_reorder_items(unsigned p_playlist,const unsigned * order,unsigned count) = 0;
	virtual void playlist_set_selection(unsigned p_playlist,const bit_array & affected,const bit_array & status) = 0;
	virtual bool playlist_remove_items(unsigned p_playlist,const bit_array & mask)=0;
	virtual bool playlist_replace_item(unsigned p_playlist,unsigned p_item,const metadb_handle_ptr & p_new_item) = 0;
	virtual void playlist_set_focus_item(unsigned p_playlist,unsigned p_item) = 0;
	virtual unsigned playlist_insert_items(unsigned p_playlist,unsigned p_base,const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection) = 0;
	virtual void playlist_ensure_visible(unsigned p_playlist,unsigned p_item) = 0;
	virtual bool playlist_rename(unsigned p_index,const char * p_name,unsigned p_name_len) = 0;

	virtual void playlist_undo_backup(unsigned p_index) = 0;
	virtual bool playlist_undo_restore(unsigned p_index) = 0;
	virtual bool playlist_redo_restore(unsigned p_index) = 0;
	virtual bool playlist_is_undo_available(unsigned p_playlist) = 0;
	virtual bool playlist_is_redo_available(unsigned p_playlist) = 0;

	virtual void playlist_item_format_title(unsigned p_playlist,unsigned p_item,titleformat_hook * p_hook,string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter)=0;//spec may be null, will use core settings; extra items are optional

	virtual bool get_playing_item_location(unsigned * p_playlist,unsigned * p_index) = 0;
	
	virtual unsigned playlist_get_playback_cursor(unsigned p_playlist) = 0;
	virtual void playlist_set_playback_cursor(unsigned p_playlist,unsigned p_cursor) = 0;

	virtual bool playlist_sort_by_format(unsigned p_playlist,const char * spec,bool p_sel_only) = 0;

	//! p_items must be sorted by metadb::path_compare; use file_operation_callback static methods instead of calling this directly
	virtual void on_files_deleted_sorted(const list_base_const_t<const char *> & p_items) = 0;
	//! p_from must be sorted by metadb::path_compare; use file_operation_callback static methods instead of calling this directly
	virtual void on_files_moved_sorted(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to) = 0;

	virtual bool playlist_lock_install(unsigned p_playlist,const service_ptr_t<playlist_lock> & p_lock) = 0;//returns false when invalid playlist or already locked
	virtual bool playlist_lock_uninstall(unsigned p_playlist,const service_ptr_t<playlist_lock> & p_lock) = 0;
	virtual bool playlist_lock_is_present(unsigned p_playlist) = 0;
	virtual bool playlist_lock_query_name(unsigned p_playlist,string_base & p_out) = 0;
	virtual bool playlist_lock_show_ui(unsigned p_playlist) = 0;
	virtual t_uint32 playlist_lock_get_filter_mask(unsigned p_playlist) = 0;


	virtual unsigned playback_order_get_count() = 0;
	virtual const char * playback_order_get_name(unsigned p_index) = 0;
	virtual GUID playback_order_get_guid(unsigned p_index) = 0;
	virtual unsigned playback_order_get_active() = 0;
	virtual void playback_order_set_active(unsigned p_index) = 0;
	
	virtual void queue_flush() = 0;
	virtual void queue_add_item_playlist(unsigned p_playlist,unsigned p_item) = 0;
	virtual void queue_add_item(metadb_handle_ptr p_item) = 0;
	virtual bool queue_is_active() = 0;

	//helpers
	bool highlight_playing_item();
	bool remove_playlist(unsigned idx);
	bool remove_playlist_switch(unsigned idx);//attempts to switch to another playlist after removing

	bool playlist_is_item_selected(unsigned p_playlist,unsigned p_item);
	bool playlist_get_item_handle(metadb_handle_ptr & p_out,unsigned p_playlist,unsigned p_item);

	bool playlist_move_selection(unsigned p_playlist,int p_delta);
	void playlist_get_selection_mask(unsigned p_playlist,bit_array_var & out);
	void playlist_get_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask);
	void playlist_get_all_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out);
	void playlist_get_selected_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out);
	
	void playlist_clear(unsigned p_playlist);
	bool playlist_add_items(unsigned playlist,const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);
	void playlist_clear_selection(unsigned p_playlist);
	void playlist_remove_selection(unsigned p_playlist,bool p_crop = false);
	
	
	//retrieving status
	unsigned activeplaylist_get_item_count();
	void activeplaylist_enum_items(enum_items_callback * p_callback,const bit_array & p_mask);
	unsigned activeplaylist_get_focus_item();//focus may be infinite if no item is focused
	bool activeplaylist_get_name(string_base & p_out);

	//modifying playlist
	bool activeplaylist_reorder_items(const unsigned * order,unsigned count);
	void activeplaylist_set_selection(const bit_array & affected,const bit_array & status);
	bool activeplaylist_remove_items(const bit_array & mask);
	bool activeplaylist_replace_item(unsigned p_item,const metadb_handle_ptr & p_new_item);
	void activeplaylist_set_focus_item(unsigned p_item);
	unsigned activeplaylist_insert_items(unsigned p_base,const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);
	void activeplaylist_ensure_visible(unsigned p_item);
	bool activeplaylist_rename(const char * p_name,unsigned p_name_len);

	void activeplaylist_undo_backup();
	bool activeplaylist_undo_restore();
	bool activeplaylist_redo_restore();

	bool activeplaylist_is_item_selected(unsigned p_item);
	bool activeplaylist_get_item_handle(metadb_handle_ptr & item,unsigned p_item);
	void activeplaylist_move_selection(int p_delta);
	void activeplaylist_get_selection_mask(bit_array_var & out);
	void activeplaylist_get_items(list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask);
	void activeplaylist_get_all_items(list_base_t<metadb_handle_ptr> & out);
	void activeplaylist_get_selected_items(list_base_t<metadb_handle_ptr> & out);
	void activeplaylist_clear();

	bool activeplaylist_add_items(const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection);

	bool playlist_insert_items_filter(unsigned p_playlist,unsigned p_base,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);
	bool activeplaylist_insert_items_filter(unsigned p_base,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);

	bool playlist_insert_locations(unsigned p_playlist,unsigned p_base,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);
	bool activeplaylist_insert_locations(unsigned p_base,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);

	bool playlist_add_items_filter(unsigned p_playlist,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);
	bool activeplaylist_add_items_filter(const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select);

	bool playlist_add_locations(unsigned p_playlist,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);
	bool activeplaylist_add_locations(const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd);

	void reset_playing_playlist();

	void activeplaylist_clear_selection();
	void activeplaylist_remove_selection(bool p_crop = false);

	void activeplaylist_item_format_title(unsigned p_item,titleformat_hook * p_hook,string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter);

	void playlist_set_selection_single(unsigned p_playlist,unsigned p_item,bool p_state);
	void activeplaylist_set_selection_single(unsigned p_item,bool p_state);

	unsigned playlist_get_selection_count(unsigned p_playlist,unsigned p_max);
	unsigned activeplaylist_get_selection_count(unsigned p_max);

	bool playlist_get_focus_item_handle(metadb_handle_ptr & p_item,unsigned p_playlist);
	bool activeplaylist_get_focus_item_handle(metadb_handle_ptr & item);

	unsigned find_playlist(const char * p_name,unsigned p_name_length);
	unsigned find_or_create_playlist(const char * p_name,unsigned p_name_length);

	bool activeplaylist_sort_by_format(const char * spec,bool p_sel_only);

	t_uint32 activeplaylist_lock_get_filter_mask();
	bool activeplaylist_is_undo_available();
	bool activeplaylist_is_redo_available();

	void remove_items_from_all_playlists(const list_base_const_t<metadb_handle_ptr> & p_data);

	void active_playlist_fix();

	bool get_all_items(list_base_t<metadb_handle_ptr> & out);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}


	static bool g_get(service_ptr_t<playlist_manager> & p_out);
};

class NOVTABLE playlist_callback : public service_base
{
public:
	virtual void on_items_added(unsigned p_playlist,unsigned start, const list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;//inside any of these methods, you can call IPlaylist APIs to get exact info about what happened (but only methods that read playlist state, not those that modify it)
	virtual void on_items_reordered(unsigned p_playlist,const unsigned * order,unsigned count)=0;//changes selection too; doesnt actually change set of items that are selected or item having focus, just changes their order
	virtual void on_items_removing(unsigned p_playlist,const bit_array & mask)=0;//called before actually removing them
	virtual void on_items_removed(unsigned p_playlist,const bit_array & mask)=0;
	virtual void on_items_selection_change(unsigned p_playlist,const bit_array & affected,const bit_array & state) = 0;
	virtual void on_item_focus_change(unsigned p_playlist,unsigned from,unsigned to)=0;//focus may be -1 when no item has focus; reminder: focus may also change on other callbacks
	
	virtual void on_items_modified(unsigned p_playlist,const bit_array & p_mask)=0;

	struct t_on_items_replaced_entry
	{
		unsigned m_index;
		metadb_handle_ptr m_old,m_new;
	};

	virtual void on_items_replaced(unsigned p_playlist,const bit_array & p_mask,const list_base_const_t<t_on_items_replaced_entry> & p_data)=0;

	virtual void on_item_ensure_visible(unsigned p_playlist,unsigned idx)=0;

	virtual void on_playlist_activate(unsigned p_old,unsigned p_new) = 0;
	virtual void on_playlist_created(unsigned p_index,const char * p_name,unsigned p_name_len) = 0;
	virtual void on_playlists_reorder(const unsigned * p_order,unsigned p_count) = 0;
	virtual void on_playlists_removing(const bit_array & p_mask,unsigned p_old_count,unsigned p_new_count) = 0;
	virtual void on_playlists_removed(const bit_array & p_mask,unsigned p_old_count,unsigned p_new_count) = 0;
	virtual void on_playlist_renamed(unsigned p_index,const char * p_new_name,unsigned p_new_name_len) = 0;

	virtual void on_default_format_changed() = 0;
	virtual void on_playback_order_changed(unsigned p_new_index) = 0;
	virtual void on_playlist_locked(unsigned p_playlist,bool p_locked) = 0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};

class NOVTABLE playlist_callback_single : public service_base
{
public:
	virtual void on_items_added(unsigned start, const list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)=0;//inside any of these methods, you can call IPlaylist APIs to get exact info about what happened (but only methods that read playlist state, not those that modify it)
	virtual void on_items_reordered(const unsigned * order,unsigned count)=0;//changes selection too; doesnt actually change set of items that are selected or item having focus, just changes their order
	virtual void on_items_removing(const bit_array & mask)=0;//called before actually removing them
	virtual void on_items_removed(const bit_array & mask)=0;
	virtual void on_items_selection_change(const bit_array & affected,const bit_array & state) = 0;
	virtual void on_item_focus_change(unsigned from,unsigned to)=0;//focus may be -1 when no item has focus; reminder: focus may also change on other callbacks
	virtual void on_items_modified(const bit_array & p_mask)=0;
	virtual void on_items_replaced(const bit_array & p_mask,const list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data)=0;
	virtual void on_item_ensure_visible(unsigned idx)=0;

	virtual void on_playlist_switch() = 0;
	virtual void on_playlist_renamed(const char * p_new_name,unsigned p_new_name_len) = 0;
	virtual void on_playlist_locked(bool p_locked) = 0;

	virtual void on_default_format_changed() = 0;
	virtual void on_playback_order_changed(unsigned p_new_index) = 0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};

class NOVTABLE playlist_incoming_item_filter : public service_base
{
public:
	virtual bool filter_items(const list_base_const_t<metadb_handle_ptr> & in,list_base_t<metadb_handle_ptr> & out) = 0;//sort / remove duplicates
	virtual bool process_locations(const list_base_const_t<const char*> & urls,list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_restrict_mask_overide, const char * p_exclude_mask_override,HWND p_parentwnd)=0;
	virtual bool process_dropped_files(interface IDataObject * pDataObject,list_base_t<metadb_handle_ptr> & out,bool filter,HWND p_parentwnd)=0;
	virtual bool process_dropped_files_check(interface IDataObject * pDataObject)=0;
	virtual bool process_dropped_files_check_if_native(interface IDataObject * pDataObject)=0;
	virtual interface IDataObject * create_dataobject(const list_base_const_t<metadb_handle_ptr> & data)=0;

	//helper
	bool process_location(const char * url,list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_mask,const char * p_exclude,HWND p_parentwnd);

	static bool g_get(service_ptr_t<playlist_incoming_item_filter> & p_out) {return service_enum_t<playlist_incoming_item_filter>().first(p_out);}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};


#endif