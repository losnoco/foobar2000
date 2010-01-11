#ifndef _FOOBAR2000_SDK_TRACK_INDEXER_H_
#define _FOOBAR2000_SDK_TRACK_INDEXER_H_

//deprecated, left here only to wrap old API calls from code that hasn't been updated to new conventions

class NOVTABLE track_indexer_callback : public abort_callback
{
public:
	virtual void on_entry(const metadb_handle_ptr & ptr,const t_filestats & p_stats)=0;//stats equal to filestats_invalid when unknown
	virtual bool want_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats) = 0;
	virtual void on_entry_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats,const file_info & p_info) = 0;
	
	virtual bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) = 0;
};

class track_indexer_callback_impl_simple : public track_indexer_callback
{
public:
	track_indexer_callback_impl_simple(list_base_t<metadb_handle_ptr> & p_list,abort_callback & p_abort) : m_list(p_list), m_abort(p_abort) {}
	void on_entry(const metadb_handle_ptr & ptr,const t_filestats & p_stats) {m_list.add_item(ptr);}
	bool want_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats) {return false;}
	void on_entry_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats,const file_info & p_info)	{m_list.add_item(ptr);}
	bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) {return m_api->handle_create(p_out,p_location);}
	bool is_aborting() {return m_abort.is_aborting();}
private:
	list_base_t<metadb_handle_ptr> & m_list;
	abort_callback & m_abort;
	static_api_ptr_t<metadb> m_api;
};


namespace track_indexer
{
	t_io_result g_get_tracks(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,track_indexer_callback & p_callback);
	t_io_result g_get_tracks_wrap(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type,playlist_loader_callback & p_callback);
	t_io_result g_get_tracks_simple(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,list_base_t<metadb_handle_ptr> & p_out,abort_callback & p_abort);
};

#endif