#include "foobar2000.h"

static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback & callback,playlist_loader_callback::t_entry_type type,const t_filestats & p_stats);

namespace {
	class archive_callback_impl : public archive_callback
	{
	public:
		archive_callback_impl(playlist_loader_callback & p_callback) : m_callback(p_callback) {}
		bool on_entry(archive * owner,const char * p_path,const t_filestats & p_stats,const service_ptr_t<file> & p_reader)
		{
			process_path_internal(p_path,p_reader,m_callback,playlist_loader_callback::entry_directory_enumerated,p_stats);
			return !m_callback.is_aborting();
		}
		bool is_aborting() {return m_callback.is_aborting();}
	private:
		playlist_loader_callback & m_callback;
	};
}

t_io_result playlist_loader::g_load_playlist(const char * p_filename,playlist_loader_callback & callback)
{
	TRACK_CALL_TEXT("playlist_loader::g_load_playlist");
	string8 filename;
	
	filename = file_path_canonical(p_filename);

	service_enum_t<playlist_loader> e;
	service_ptr_t<playlist_loader> l;

	string_extension_8 extension(filename);

	service_ptr_t<file> r;
	t_io_result status;

	

	{
		bool /*have_content_type, */have_extension;
//		string8 content_type;
//		have_content_type = r->get_content_type(content_type);
		have_extension = extension.length()>0;

		if (e.first(l)) do {
			if (
//			have_content_type && l->is_our_content_type(content_type) || 
				(have_extension && !stricmp_utf8(l->get_extension(),extension)))
			{
				if (r.is_empty())
				{
					status = filesystem::g_open_read(r,filename,callback);
					if (io_result_failed(status)) return status;
				}

				{
					TRACK_CODE("playlist_loader::open",status = l->open(filename,r,callback));
					if (io_result_succeeded(status)) return io_result_success;
					else if (status != io_result_error_data) return status;
				}
				if (r.is_valid())
				{
					status = r->reopen(callback);
					if (io_result_failed(status)) return status;
				}
			}
		} while(e.next(l));
	}

	return io_result_error_data;
}

static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback & p_callback,playlist_loader_callback::t_entry_type p_type,const t_filestats & p_stats)
{
	//p_path must be canonical

	if (p_callback.is_aborting()) return;

	p_callback.on_progress(p_path);

	
	{
		if (p_reader.is_empty())
		{
			directory_callback_i directory_results(true);
			if (io_result_succeeded(filesystem::g_list_directory(p_path,directory_results,p_callback)))
			{
				unsigned n;
				for(n=0;n<directory_results.get_count();n++)
				{
					process_path_internal(directory_results.get_item(n),0,p_callback,playlist_loader_callback::entry_directory_enumerated,directory_results.get_item_stats(n));
					if (p_callback.is_aborting()) break;
				}
				return;
			}
		}

		bool found = false;


		{
			archive_callback_impl archive_results(p_callback);
			service_enum_t<filesystem> e;
			service_ptr_t<filesystem> f;
			if (e.first(f)) do {
				service_ptr_t<archive> arch;
				if (f->service_query_t(arch))
				{
					t_io_result status;

					if (p_reader.is_valid() && p_reader->can_seek())
					{
						status = p_reader->seek(0,p_callback);
						if (io_result_failed(status)) return;
					}

					TRACK_CODE("archive::archive_list",status = arch->archive_list(p_path,p_reader,archive_results,true));

					if (io_result_succeeded(status))
					{
						found = true;
						break;
					}

					if (p_callback.is_aborting()) break;
				}
			} while(e.next(f));
		}

		if (found || p_callback.is_aborting()) return;

	}

	

	bool resolved = false;
	{
		service_ptr_t<link_resolver> ptr;
		if (link_resolver::g_find(ptr,p_path))
		{
			string8 temp;
			if (io_result_succeeded(ptr->resolve(p_reader,p_path,temp,p_callback)))
			{
				track_indexer::g_get_tracks_wrap(temp,0,filestats_invalid,playlist_loader_callback::entry_from_playlist,p_callback);
				resolved = true;
			}
			else
			{
				if (p_reader.is_valid())
					p_reader->reopen(p_callback);
			}
		}
	}


	if (!resolved)
	{
		track_indexer::g_get_tracks_wrap(p_path,p_reader,p_stats,p_type,p_callback);
	}
}

void playlist_loader::g_process_path(const char * p_filename,playlist_loader_callback & callback,playlist_loader_callback::t_entry_type type)
{
	TRACK_CALL_TEXT("playlist_loader::g_process_path");

	file_path_canonical filename(p_filename);

	process_path_internal(filename,0,callback,type,filestats_invalid);
}

t_io_result playlist_loader::g_save_playlist(const char * p_filename,const list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("playlist_loader::g_save_playlist");
	t_io_result status;
	string8 filename;
	filesystem::g_get_canonical_path(p_filename,filename);
	service_ptr_t<file> r;
	status = filesystem::g_open(r,filename,filesystem::open_mode_write_new,p_abort);
	if (io_result_failed(status)) return status;

	string_extension ext(filename);
	
	service_enum_t<playlist_loader> e;
	service_ptr_t<playlist_loader> l;
	if (e.first(l)) do {
		if (l->can_write() && !stricmp_utf8(ext,l->get_extension()))
		{
			TRACK_CODE("playlist_loader::write",status = l->write(filename,r,data,p_abort));
			if (io_result_succeeded(status)) return io_result_success;
			if (status != io_result_error_data) return status;
		}
	} while(e.next(l));
	
	r.release();
	filesystem::g_remove(filename,p_abort);

	return io_result_error_data;
}