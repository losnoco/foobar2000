#ifndef _PLAYLIST_LOADER_H_
#define _PLAYLIST_LOADER_H_

class NOVTABLE playlist_loader_callback : public abort_callback	//receives new playlist entries from playlists/filesystem/whatever
{
public:
	enum t_entry_type
	{
		entry_user_requested,
		entry_directory_enumerated,
		entry_from_playlist,
	};
	virtual void on_progress(const char * path) = 0;
	
	virtual void on_entry(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh)=0;//stats equal to filestats_invalid when unknown; p_fresh applies to stats
	virtual bool want_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh) = 0;
	virtual void on_entry_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,const file_info & p_info,bool p_fresh) = 0;

	virtual bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) = 0;
};

class playlist_loader_callback_i : public playlist_loader_callback
{
	metadb_handle_list m_data;
	abort_callback & m_abort;
	static_api_ptr_t<metadb> m_api;
public:

	~playlist_loader_callback_i() {}
	explicit playlist_loader_callback_i(abort_callback & p_abort) : m_abort(p_abort) {}

	bool FB2KAPI is_aborting() {return m_abort.is_aborting();}

	const metadb_handle_ptr & get_item(unsigned idx) const {return m_data[idx];}
	const metadb_handle_ptr & operator[](unsigned idx) const {return m_data[idx];}
	unsigned get_count() const {return m_data.get_count();}
	
	const metadb_handle_list & get_data() const {return m_data;}

	void on_progress(const char * path) {}

	void on_entry(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh) {m_data.add_item(ptr);}
	bool want_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh) {return false;}
	void on_entry_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,const file_info & p_info,bool p_fresh) {m_data.add_item(ptr);}

	bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) {return m_api->handle_create(p_out,p_location);}

};

class NOVTABLE playlist_loader : public service_base
{
public:
	virtual t_io_result open(const char * filename, const service_ptr_t<file> & r,playlist_loader_callback & callback)=0;	//send new entries to callback
	virtual t_io_result write(const char * filename, const service_ptr_t<file> & r,const list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort)=0;
	virtual const char * get_extension()=0;
	virtual bool can_write()=0;
	virtual bool is_our_content_type(const char*) = 0;
	virtual bool is_associatable() = 0;

	static t_io_result g_load_playlist(const char * filename,playlist_loader_callback & callback);
	//attempts to load file as a playlist, return 0 on failure

	static t_io_result g_save_playlist(const char * filename,const list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort);
	//saves playlist into file

	static void g_process_path(const char * filename,playlist_loader_callback & callback,playlist_loader_callback::t_entry_type type = playlist_loader_callback::entry_user_requested);
	//adds contents of filename (can be directory) to playlist, doesnt load playlists
	
	//this helper also loads playlists
	//return true if loaded as playlist, false if loaded as files
	static bool g_process_path_ex(const char * filename,playlist_loader_callback & callback,playlist_loader_callback::t_entry_type type = playlist_loader_callback::entry_user_requested)
	{
		if (io_result_failed(g_load_playlist(filename,callback)))
		{
			g_process_path(filename,callback,type);
			return false;
		}
		else return true;
	}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	playlist_loader() {}
	~playlist_loader() {}
};


#endif //_PLAYLIST_LOADER_H_