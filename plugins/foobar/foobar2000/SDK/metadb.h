#ifndef _METADB_H_
#define _METADB_H_

#include "metadb_handle.h"

//! API for tag read/write operations. Legal to call from main thread only, except for hint_multi_async() / hint_async().
class NOVTABLE metadb_io : public service_base
{
public:
	enum t_load_info_type
	{
		load_info_default,
		load_info_force,
		load_info_check_if_changed
	};

	//! Returns whether some tag I/O operation is currently running. Another one can't be started.
	virtual bool is_busy() = 0;
	//! Returns whether - in result of user settings - all update operations will fail.
	virtual bool is_updating_disabled() = 0;
	//! Returns whether - in result of user settings - all update operations will silently succeed but without actually modifying files.
	virtual bool is_file_updating_blocked() = 0;
	//! If another tag I/O operation is running, this call will give focus to its progress window.
	virtual void highlight_running_process() = 0;
	//! Loads tags from multiple items.
	virtual bool load_info_multi(const list_base_const_t<metadb_handle_ptr> & p_list,t_load_info_type p_type,HWND p_parent_window,bool p_show_errors,t_io_result * p_retcodes) = 0;
	//! Updates tags on multiple items.
	virtual bool update_info_multi(const list_base_const_t<metadb_handle_ptr> & p_list,const list_base_const_t<file_info*> & p_new_info,HWND p_parent_window,bool p_show_errors,t_io_result * p_retcodes) = 0;
	//! Rewrites tags on multiple items.
	virtual bool rewrite_info_multi(const list_base_const_t<metadb_handle_ptr> & p_list,HWND p_parent_window,bool p_show_errors,t_io_result * p_retcodes) = 0;
	//! Removes tags from multiple items.
	virtual bool remove_info_multi(const list_base_const_t<metadb_handle_ptr> & p_list,HWND p_parent_window,bool p_show_errors,t_io_result * p_retcodes) = 0;
	//! Returns true when tag updates on busy files will be queued for later, false if fail (default behavior).
	virtual bool pending_is_enabled() = 0;
	//! Returns count of pending updates (after files were busy during first update attempt).
	virtual unsigned pending_get_count() = 0;
	//! Attempts to execute any pending tag update operations.
	//! @param p_force If set to true, will display dialog box asking user to approve dropping updates.
	//! @param p_parent Parent window for progress dialog and dialog asking user to approve dropping updates if p_force is set to true.
	//! @param p_show_errors If set to true, will report any I/O errors to user (using popup_message API).
	//! @returns number of updates still pending. If returns non-zero after p_force was set to true, user chose "cancel" in the dialog.
	virtual unsigned pending_flush(bool p_force,HWND p_parent,bool p_show_errors) = 0;
	//! Resets pending tag update operations. WARNING: this will cause data loss.
	virtual void pending_reset() = 0;
	//! If specified item is one of queued updates, retrieves info that is about to be written to file. Returns true on success, false if specified item is not in queue.
	virtual bool pending_query(metadb_handle_ptr p_item,file_info & p_out) = 0; 

	virtual void hint_multi(const list_base_const_t<metadb_handle_ptr> & p_list,const list_base_const_t<const file_info*> & p_infos,const list_base_const_t<t_filestats> & p_stats,const bit_array & p_fresh_mask) = 0;

	virtual void hint_multi_async(const list_base_const_t<metadb_handle_ptr> & p_list,const list_base_const_t<const file_info*> & p_infos,const list_base_const_t<t_filestats> & p_stats,const bit_array & p_fresh_mask) = 0;

	virtual void hint_reader(service_ptr_t<class input_info_reader> p_reader,const char * p_path,abort_callback & p_abort) = 0;

	virtual t_io_result path_to_handles_simple(const char * p_path,list_base_t<metadb_handle_ptr> & p_out) = 0;

	void hint_async(metadb_handle_ptr p_item,const file_info & p_info,const t_filestats & p_stats,bool p_fresh);

	bool pending_query_ex(metadb_handle_ptr p_item,file_info & p_out);


	t_io_result load_info(metadb_handle_ptr p_item,t_load_info_type p_type,HWND p_parent_window,bool p_show_errors);
	t_io_result update_info(metadb_handle_ptr p_item,file_info & p_info,HWND p_parent_window,bool p_show_errors);
	
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class NOVTABLE metadb_io_callback : public service_base
{
public:
	virtual void on_changed_sorted(const list_base_const_t<metadb_handle_ptr> & p_items_sorted)=0;//items are always sorted by pointer value

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

//only one implementation in main exe, do not derive from this
class NOVTABLE metadb : public service_base
{
public:
	//! Locks metadb to prevent other threads from modifying it while you're working with some of its contents. Some functions (metadb_handle::get_info_locked(), metadb_handle::get_info_async_locked()) can be called only from inside metadb lock section.
	virtual void database_lock()=0;
	//! Unlocks metadb after database_lock(). Some functions (metadb_handle::get_info_locked(), metadb_handle::get_info_async_locked()) can be called only from inside metadb lock section.
	virtual void database_unlock()=0;
	
	//! Returns metadb_handle object referencing specified location (attempts to find existing one, creates new one if doesn't exist).
	//! @param p_out Receives metadb_handle pointer.
	//! @param p_location Location to create metadb_handle for.
	//! @returns true on success, false on failure (rare).
	virtual bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location)=0;


	bool handle_create_replace_path_canonical(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path);//should never fail
	bool handle_replace_path_canonical(metadb_handle_ptr & p_out,const char * p_new_path);
	bool handle_create_replace_path(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path);//should never fail


	enum {case_sensitive = true};

	inline static int path_compare_ex(const char * p1,unsigned len1,const char * p2,unsigned len2) {return case_sensitive ? strcmp_ex(p1,len1,p2,len2) : stricmp_utf8_ex(p1,len1,p2,len2);}
	inline static int path_compare(const char * p1,const char * p2) {return case_sensitive ? strcmp(p1,p2) : stricmp_utf8(p1,p2);}
	inline static int path_compare_metadb_handle(const metadb_handle_ptr & p1,const metadb_handle_ptr & p2) {return path_compare(p1->get_path(),p2->get_path());}

	static bool g_get(service_ptr_t<metadb> & p_out);//should never fail

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};


class in_metadb_sync
{
public:
	in_metadb_sync()
	{
		service_ptr_t<metadb> api;
		if (metadb::g_get(api)) api->database_lock();
	}
	~in_metadb_sync()
	{
		service_ptr_t<metadb> api;
		if (metadb::g_get(api)) api->database_unlock();
	}
};

class file_info_update_helper
{
public:
	file_info_update_helper(const list_base_const_t<metadb_handle_ptr> & p_data);
	file_info_update_helper(metadb_handle_ptr p_item);

	bool read_infos(HWND p_parent,bool p_show_errors);

	enum t_write_result
	{
		write_ok,
		write_aborted,
		write_busy,
		write_error
	};
	t_write_result write_infos(HWND p_parent,bool p_show_errors);

	unsigned get_item_count() const;
	bool is_item_valid(unsigned p_index) const;//returns false where info reading failed
	
	file_info & get_item(unsigned p_index);
	metadb_handle_ptr get_item_handle(unsigned p_index) const;

	void invalidate_item(unsigned p_index);

private:
	metadb_handle_list m_data;
	array_t<file_info_impl> m_infos;
	array_t<bool> m_mask;
};


#endif