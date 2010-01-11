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
		bool FB2KAPI is_aborting() {return m_callback.is_aborting();}
	private:
		playlist_loader_callback & m_callback;
	};
}

void playlist_loader::g_load_playlist(const char * p_path,playlist_loader_callback & callback)
{
	TRACK_CALL_TEXT("playlist_loader::g_load_playlist");
	string8 filename;
	
	filename = file_path_canonical(p_path);

	service_enum_t<playlist_loader> e;
	service_ptr_t<playlist_loader> l;

	string_extension extension(filename);

	service_ptr_t<file> r;

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
				if (r.is_empty()) {
					filesystem::g_open_read(r,filename,callback);
				}

				try {
					TRACK_CODE("playlist_loader::open",l->open(filename,r,callback));
					return;//success
				} catch(exception_io_data const &) {
					//fall thru
				}

				if (r.is_valid()) r->reopen(callback);
			}
		} while(e.next(l));
	}

	throw exception_io_data();
}

static void track_indexer__g_get_tracks_e(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback & p_callback,bool & p_got_input)
{
	if (p_reader.is_empty() && filesystem::g_is_remote_safe(p_path))
	{
		metadb_handle_ptr handle;
		p_callback.handle_create(handle,make_playable_location(p_path,0));
		p_got_input = true;
		p_callback.on_entry(handle,p_type,p_stats,true);
	} else {
		service_ptr_t<input_info_reader> instance;
		input_entry::g_open_for_info_read(instance,p_reader,p_path,p_callback);

		t_filestats stats = instance->get_file_stats(p_callback);

		t_uint32 subsong,subsong_count = instance->get_subsong_count();
		for(subsong=0;subsong<subsong_count;subsong++)
		{
			p_callback.check_e();
			metadb_handle_ptr handle;
			t_uint32 index = instance->get_subsong(subsong);
			p_callback.handle_create(handle,make_playable_location(p_path,index));

			p_got_input = true;
			if (p_callback.want_info(handle,p_type,stats,true))
			{
				file_info_impl info;
				instance->get_info(handle->get_subsong_index(),info,p_callback);
				p_callback.on_entry_info(handle,p_type,stats,info,true);
			}
			else
			{
				p_callback.on_entry(handle,p_type,stats,true);
			}
		}
	}
}


static void track_indexer__g_get_tracks_wrap(const char * p_path,const service_ptr_t<file> & p_reader,const t_filestats & p_stats,playlist_loader_callback::t_entry_type p_type,playlist_loader_callback & p_callback)
{
	bool got_input = false;
	try {
		track_indexer__g_get_tracks_e(p_path,p_reader,p_stats,p_type,p_callback,got_input); 
	} catch(std::exception const & e) {
		console::formatter() << "could not enumerate tracks (" << e << ") on:\n" << file_path_display(p_path);
		if (!got_input && !p_callback.is_aborting()) {
			if (p_type == playlist_loader_callback::entry_user_requested)
			{
				metadb_handle_ptr handle;
				p_callback.handle_create(handle,make_playable_location(p_path,0));
				p_callback.on_entry(handle,p_type,p_stats,true);
			}
		}
	}
}


static void process_path_internal(const char * p_path,const service_ptr_t<file> & p_reader,playlist_loader_callback & p_callback,playlist_loader_callback::t_entry_type p_type,const t_filestats & p_stats)
{
	//p_path must be canonical

	p_callback.check_e();

	p_callback.on_progress(p_path);

	
	{
		if (p_reader.is_empty()) {
			directory_callback_impl directory_results(true);
			try {
				filesystem::g_list_directory(p_path,directory_results,p_callback);
				for(t_size n=0;n<directory_results.get_count();n++) {
					process_path_internal(directory_results.get_item(n),0,p_callback,playlist_loader_callback::entry_directory_enumerated,directory_results.get_item_stats(n));
				}
				return;
			} catch(exception_aborted const &) {throw;}
			catch(std::exception const & ) {
				//do nothing, fall thru
				//fixme - catch only filesystem exceptions?
			}
		}

		bool found = false;


		{
			archive_callback_impl archive_results(p_callback);
			service_enum_t<filesystem> e;
			service_ptr_t<filesystem> f;
			while(e.next(f)) {
				p_callback.check_e();
				service_ptr_t<archive> arch;
				if (f->service_query_t(arch)) {
					if (p_reader.is_valid()) p_reader->reopen(p_callback);

					try {
						TRACK_CODE("archive::archive_list",arch->archive_list(p_path,p_reader,archive_results,true));
						return;
					} catch(exception_aborted const &) {throw;} 
					catch(std::exception const &) {}
				}
			} 
		}
	}

	

	bool resolved = false;
	{
		service_ptr_t<link_resolver> ptr;
		if (link_resolver::g_find(ptr,p_path))
		{
			if (p_reader.is_valid()) p_reader->reopen(p_callback);

			string8 temp;
			try {
				TRACK_CODE("link_resolver::resolve",ptr->resolve(p_reader,p_path,temp,p_callback));

				track_indexer__g_get_tracks_wrap(temp,0,filestats_invalid,playlist_loader_callback::entry_from_playlist,p_callback);
				resolved = true;
			} catch(exception_aborted const &) {throw;}
			catch(std::exception const &) {}
		}
	}


	if (!resolved)
	{
		track_indexer__g_get_tracks_wrap(p_path,p_reader,p_stats,p_type,p_callback);
	}
}

void playlist_loader::g_process_path(const char * p_filename,playlist_loader_callback & callback,playlist_loader_callback::t_entry_type type)
{
	TRACK_CALL_TEXT("playlist_loader::g_process_path");

	file_path_canonical filename(p_filename);

	process_path_internal(filename,0,callback,type,filestats_invalid);
}

void playlist_loader::g_save_playlist(const char * p_filename,const list_base_const_t<metadb_handle_ptr> & data,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("playlist_loader::g_save_playlist");
	string8 filename;
	filesystem::g_get_canonical_path(p_filename,filename);
	try {
		service_ptr_t<file> r;
		filesystem::g_open(r,filename,filesystem::open_mode_write_new,p_abort);

		string_extension ext(filename);
		
		service_enum_t<playlist_loader> e;
		service_ptr_t<playlist_loader> l;
		if (e.first(l)) do {
			if (l->can_write() && !stricmp_utf8(ext,l->get_extension())) {
				try {
					TRACK_CODE("playlist_loader::write",l->write(filename,r,data,p_abort));
					return;
				} catch(exception_io_data const &) {}
			}
		} while(e.next(l));
		throw exception_io_data();
	} catch(std::exception const &) {
		try {
			filesystem::g_remove(filename,p_abort);
		} catch(std::exception const &) {}
		throw;
	}
}