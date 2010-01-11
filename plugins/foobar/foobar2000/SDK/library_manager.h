#ifndef _foobar2000_sdk_library_manager_h_
#define _foobar2000_sdk_library_manager_h_

class NOVTABLE library_manager : public service_base
{
public:
	class NOVTABLE enum_callback
	{
	public:
		virtual bool on_item(const metadb_handle_ptr & p_item) = 0;
	};

	virtual bool is_item_in_library(const metadb_handle_ptr & p_item) = 0;
	virtual bool is_item_addable(const metadb_handle_ptr & p_item) = 0;
	virtual bool is_path_addable(const char * p_path) = 0;
	virtual bool get_relative_path(const metadb_handle_ptr & p_item,pfc::string_base & out) = 0;
	virtual void enum_items(enum_callback * p_callback) = 0;
	virtual void add_items(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	virtual void remove_items(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	virtual void add_items_async(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! p_data must be sorted by metadb::path_compare; use file_operation_callback static methods instead of calling this directly
	virtual void on_files_deleted_sorted(const pfc::list_base_const_t<const char *> & p_data) = 0;

	virtual void get_all_items(pfc::list_base_t<metadb_handle_ptr> & p_out) = 0;

	virtual bool is_library_enabled() = 0;
	virtual void show_preferences() = 0;

	virtual void rescan() = 0;
	
	virtual void check_dead_entries(const pfc::list_base_t<metadb_handle_ptr> & p_list) = 0;

	static bool g_get(service_ptr_t<library_manager> & p_out) {return service_enum_t<library_manager>().first(p_out);}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	library_manager() {}
	~library_manager() {}
};

class NOVTABLE library_callback : public service_base
{
public:
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	library_callback() {}
	~library_callback() {}
};

class NOVTABLE library_viewer : public service_base
{
public:
	virtual GUID get_preferences_page() = 0;
	virtual bool have_activate() = 0;
	virtual void activate() = 0;
	virtual GUID get_guid() = 0;//for internal identification, different than preferences page
	virtual const char * get_name() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	library_viewer() {}
	~library_viewer() {}
};

#endif _foobar2000_sdk_library_manager_h_