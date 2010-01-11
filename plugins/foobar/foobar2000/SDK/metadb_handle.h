#ifndef _FOOBAR2000_METADB_HANDLE_H_
#define _FOOBAR2000_METADB_HANDLE_H_

#include "file_info.h"

//namespace foobar2000_io {class file; }
class titleformat_hook;
class titleformat_text_filter;



//! metadb_handle object represents interface to reference-counted file_info cache entry for specified location.\n
//! To obtain a metadb_handle to specific location, use metadb::handle_create(). To obtain a list of metadb_handle objects corresponding to specific path (directory, playlist, multitrack file, etc), use relevant playlist_loader static helper methods.
//! metadb_handle is also the most efficient way of passing playable object locations around because it provides fast access to both location and infos, and is reference counted so duplicating it is as fast as possible.

class NOVTABLE metadb_handle : public service_base
{
public:
	//! Retrieves location represented by this metadb_handle object.
	virtual const playable_location & get_location() const = 0;//never fails, returned pointer valid till the object is released


	virtual bool format_title(titleformat_hook * p_hook,string_base & out,const service_ptr_t<class titleformat_object> & p_script,titleformat_text_filter * p_filter) = 0;

	//! Locks metadb to prevent other threads from modifying it while you're working with some of its contents. Some functions (metadb_handle::get_info_locked(), metadb_handle::get_info_async_locked()) can be called only from inside metadb lock section.
	//! Same as metadb::database_lock().
	virtual void metadb_lock() = 0;
	//! Unlocks metadb after metadb_lock(). Some functions (metadb_handle::get_info_locked(), metadb_handle::get_info_async_locked()) can be called only from inside metadb lock section.
	//! Same as metadb::database_unlock().
	virtual void metadb_unlock() = 0;

	//! Returns last seen file stats, filestats_invalid if unknown.
	virtual t_filestats get_filestats() const = 0;

	virtual bool is_info_loaded() const = 0;
	virtual bool get_info(file_info & p_info) const = 0;
	virtual bool get_info_locked(const file_info * & p_info) const = 0;
	virtual bool is_info_loaded_async() const = 0;	
	virtual bool get_info_async(file_info & p_info) const = 0;	
	virtual bool get_info_async_locked(const file_info * & p_info) const = 0;

	virtual void format_title_from_external_info(const file_info & p_info,titleformat_hook * p_hook,string_base & out,const service_ptr_t<class titleformat_object> & p_script,titleformat_text_filter * p_filter)=0;
	
	static bool g_should_reload(const t_filestats & p_old_stats,const t_filestats & p_new_stats,bool p_fresh);
	bool should_reload(const t_filestats & p_new_stats,bool p_fresh) const;
	

	int format_title(titleformat_hook * p_hook,string_base & out,const char * spec,titleformat_text_filter * p_filter);
	int format_title_legacy(string_base & out,const char * spec,const char * extra_items);


	inline const char * get_path() const//never fails
	{
		return get_location().get_path();
	}
	inline t_uint32 get_subsong_index() const
	{
		return get_location().get_subsong_index();
	}

	
	double get_length();//helper
	
	t_filetimestamp get_filetimestamp();
	t_filesize get_filesize();

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

protected:
	metadb_handle() {}
	~metadb_handle() {}
};

typedef service_ptr_t<metadb_handle> metadb_handle_ptr;

namespace metadb_handle_list_helper {
	void sort_by_format_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,const char * spec,titleformat_hook * p_hook);
	void sort_by_format_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order,const char * spec,titleformat_hook * p_hook);
	void sort_by_format_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook);
	void sort_by_format_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook);

	void sort_by_relative_path_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count);
	void sort_by_relative_path_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order);
	
	void remove_duplicates(list_base_t<metadb_handle_ptr> & p_list);
	void sort_by_pointer_remove_duplicates(list_base_t<metadb_handle_ptr> & p_list);
	void sort_by_path_quick(list_base_t<metadb_handle_ptr> & p_list);

	void sort_by_pointer(list_base_t<metadb_handle_ptr> & p_list);
	unsigned bsearch_by_pointer(const list_base_const_t<metadb_handle_ptr> & p_list,const metadb_handle_ptr & val);

	double calc_total_duration(const list_base_const_t<metadb_handle_ptr> & p_list);

	void sort_by_path(list_base_t<metadb_handle_ptr> & p_list);
};

template<class S = array_fast_t<metadb_handle_ptr > >
class metadb_handle_list_t : public service_list_t<metadb_handle,S>
{
public:
	inline void sort_by_format(const char * spec,titleformat_hook * p_hook) {return sort_by_format_partial(0,get_count(),spec,p_hook);}
	inline void sort_by_format_partial(unsigned base,unsigned count,const char * spec,titleformat_hook * p_hook) {metadb_handle_list_helper::sort_by_format_partial(*this,base,count,spec,p_hook);}
	inline void sort_by_format_get_order(unsigned* order,const char * spec,titleformat_hook * p_hook) const {sort_by_format_get_order_partial(0,get_count(),order,spec,p_hook);}
	inline void sort_by_format_get_order_partial(unsigned base,unsigned count,unsigned* order,const char * spec,titleformat_hook * p_hook) const {metadb_handle_list_helper::sort_by_format_get_order_partial(*this,base,count,order,spec,p_hook);}

	inline void sort_by_format(const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook) {return sort_by_format_partial(0,get_count(),p_script,p_hook);}
	inline void sort_by_format_partial(unsigned base,unsigned count,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook) {metadb_handle_list_helper::sort_by_format_partial(*this,base,count,p_script,p_hook);}
	inline void sort_by_format_get_order(unsigned* order,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook) const {sort_by_format_get_order_partial(0,get_count(),order,p_script,p_hook);}
	inline void sort_by_format_get_order_partial(unsigned base,unsigned count,unsigned* order,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook) const {metadb_handle_list_helper::sort_by_format_get_order_partial(*this,base,count,order,p_script,p_hook);}
	
	inline void sort_by_relative_path() {sort_by_relative_path_partial(0,get_count());}
	inline void sort_by_relative_path_partial(unsigned base,unsigned count) {metadb_handle_list_helper::sort_by_relative_path_partial(*this,base,count);}
	inline void sort_by_relative_path_get_order(unsigned* order) const {sort_by_relative_path_get_order_partial(0,get_count(),order);}
	inline void sort_by_relative_path_get_order_partial(unsigned base,unsigned count,unsigned* order) const {metadb_handle_list_helper::sort_by_relative_path_get_order_partial(*this,base,count,order);}
	
	inline void remove_duplicates() {metadb_handle_list_helper::remove_duplicates(*this);}
	inline void sort_by_pointer_remove_duplicates() {metadb_handle_list_helper::sort_by_pointer_remove_duplicates(*this);}
	inline void sort_by_path_quick() {metadb_handle_list_helper::sort_by_path_quick(*this);}

	inline void sort_by_pointer() {metadb_handle_list_helper::sort_by_pointer(*this);}
	inline unsigned bsearch_by_pointer(const metadb_handle_ptr & val) const {return metadb_handle_list_helper::bsearch_by_pointer(*this,val);}

	inline double calc_total_duration() const {return metadb_handle_list_helper::calc_total_duration(*this);}

	inline void sort_by_path() {metadb_handle_list_helper::sort_by_path(*this);}

	inline static void g_swap(metadb_handle_list_t<S> & item1, metadb_handle_list_t<S> & item2)
	{
		pfc::swap_t(*(service_list_t<metadb_handle,S>*)&item1,*(service_list_t<metadb_handle,S>*)&item2);
	}
};

typedef metadb_handle_list_t<> metadb_handle_list;
typedef metadb_handle_list_t<array_fast_aggressive_t<metadb_handle_ptr> > metadb_handle_list_fast;

namespace metadb_handle_list_helper {
	void sorted_by_pointer_extract_difference(metadb_handle_list const & p_list_1,metadb_handle_list const & p_list_2,metadb_handle_list & p_list_1_specific,metadb_handle_list & p_list_2_specific);
};

namespace pfc {

	template<typename S>
	inline void swap_t(metadb_handle_list_t<S> & item1, metadb_handle_list_t<S> & item2)
	{
		metadb_handle_list_t<S>::g_swap(item1,item2);
	}

};


class metadb_handle_lock
{
	metadb_handle_ptr m_ptr;
public:
	inline metadb_handle_lock(const metadb_handle_ptr & param)
	{
		m_ptr = param;
		m_ptr->metadb_lock();
	}	
	inline ~metadb_handle_lock() {m_ptr->metadb_unlock();}
};

inline string_formatter & operator<<(string_formatter & p_fmt,const metadb_handle_ptr & p_location) {return p_fmt << p_location->get_location();}


#endif //_FOOBAR2000_METADB_HANDLE_H_