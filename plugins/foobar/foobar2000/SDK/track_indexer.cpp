#include "foobar2000.h"


t_io_result track_indexer::g_get_tracks(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,track_indexer_callback & p_callback)
{
	if (p_reader.is_empty() && filesystem::g_is_remote(p_path))
	{
		metadb_handle_ptr handle;
		if (p_callback.handle_create(handle,make_playable_location(p_path,0)))
			p_callback.on_entry(handle,p_stats);
		return io_result_success;
	}

	t_io_result status;
	service_ptr_t<input_info_reader> instance;
	status = input_entry::g_open_for_info_read(instance,p_reader,p_path,p_callback);
	if (io_result_failed(status)) return status;

	t_filestats stats = p_stats;
	if (stats == filestats_invalid)
	{
		status = instance->get_file_stats(stats,p_callback);
		if (io_result_failed(status)) return status;
	}

	unsigned subsong,subsong_count = instance->get_subsong_count();
	for(subsong=0;subsong<subsong_count;subsong++)
	{
		if (p_callback.is_aborting()) return io_result_aborted;
		metadb_handle_ptr handle;
		if (p_callback.handle_create(handle,make_playable_location(p_path,instance->get_subsong(subsong))))
		{
			if (p_callback.want_info(handle,stats))
			{
				file_info_impl info;
				status = instance->get_info(handle->get_subsong_index(),info,p_callback);
				if (io_result_failed(status)) return status;
				p_callback.on_entry_info(handle,stats,info);
			}
			else
			{
				p_callback.on_entry(handle,stats);
			}
		}
	}
	return io_result_success;
}





namespace {
	class track_indexer_callback_impl_wrap : public track_indexer_callback
	{
	public:
		track_indexer_callback_impl_wrap(playlist_loader_callback & p_callback,playlist_loader_callback::t_entry_type p_type) : m_callback(p_callback), m_type(p_type), m_got_input(false) {}

		void on_entry(const metadb_handle_ptr & ptr,const t_filestats & p_stats) {m_got_input = true;m_callback.on_entry(ptr,m_type,p_stats,true);}
		bool want_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats) {return m_callback.want_info(ptr,m_type,p_stats,true);}
		void on_entry_info(const metadb_handle_ptr & ptr,const t_filestats & p_stats,const file_info & p_info) {m_got_input = true;m_callback.on_entry_info(ptr,m_type,p_stats,p_info,true);}
		bool handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) {return m_callback.handle_create(p_out,p_location);}
		bool is_aborting() {return m_callback.is_aborting();}
		bool got_input() const {return m_got_input;}
	private:
		bool m_got_input;
		playlist_loader_callback & m_callback;
		playlist_loader_callback::t_entry_type m_type;
	};

};

t_io_result track_indexer::g_get_tracks_wrap(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback & p_callback)
{
	track_indexer_callback_impl_wrap wrapper(p_callback,p_type);
	t_io_result status = g_get_tracks(p_path,p_reader,p_stats,wrapper);
	if (io_result_failed(status) && !wrapper.got_input())
	{
		if (p_type == playlist_loader_callback::entry_user_requested)
		{
			metadb_handle_ptr handle;
			if (p_callback.handle_create(handle,make_playable_location(p_path,0)))
			{
				p_callback.on_entry(handle,p_type,p_stats,true);
			}
		}
	}
	return status;
}

t_io_result track_indexer::g_get_tracks_simple(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,list_base_t<metadb_handle_ptr> & p_out,abort_callback & p_abort)
{
	return g_get_tracks(p_path,p_reader,p_stats,track_indexer_callback_impl_simple(p_out,p_abort));
}
