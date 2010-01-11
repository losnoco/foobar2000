#ifndef _FILE_OPERATION_CALLBACK_H_
#define _FILE_OPERATION_CALLBACK_H_

//! Interface to notify component system about files being deleted or moved. Operates in app's main thread only.

class file_operation_callback : public service_base
{
public:
	//! p_items is a metadb::path_compare sorted list of files that have been deleted.
	virtual void on_files_deleted_sorted(const list_base_const_t<const char *> & p_items) = 0;
	//! p_from is a metadb::path_compare sorted list of files that have been moved, p_to is a list of corresponding target locations.
	virtual void on_files_moved_sorted(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to) = 0;
	//! p_from is a metadb::path_compare sorted list of files that have been copied, p_to is a list of corresponding target locations.
	virtual void on_files_copied_sorted(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to) = 0;

	static void g_on_files_deleted(const list_base_const_t<const char *> & p_items);
	static void g_on_files_moved(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to);
	static void g_on_files_copied(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to);


	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};


#endif //_FILE_OPERATION_CALLBACK_H_