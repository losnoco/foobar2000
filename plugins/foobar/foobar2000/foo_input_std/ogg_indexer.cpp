#include "stdafx.h"
#include "ogg_helper.h"

class track_indexer_ogg : public track_indexer
{
private:
	static bool is_our_path(const char * fn)
	{
		string_extension_8 ext(fn);
		return !stricmp_utf8(ext,"ogg") || !stricmp_utf8(ext,"spx");
	}

	t_io_result get_tracks(const char * p_path,const service_ptr_t<file> & p_reader,track_indexer_callback & p_callback)
	{
		TRACK_CALL_TEXT("track_indexer_ogg::get_tracks");
		if (!is_our_path(p_path)) return io_result_error_data;
		
		service_ptr_t<file> r = p_reader;

		t_io_result status;

		if (r.is_empty())
		{
			status = filesystem::g_open_precache(r,p_path,p_callback);
			if (io_result_failed(status)) return status;
		}

		if (!r->can_seek()) return io_result_error_data;

		status = r->seek(0,p_callback);
		if (io_result_failed(status)) return status;
		

		unsigned links;
		status = ogg_helper::query_link_count(p_path,r,links,p_callback);
		if (io_result_failed(status)) return status;

		if (links == 0) return io_result_error_data;

		t_filestats stats;

		status = r->get_stats(stats,p_callback);
		if (io_result_failed(status)) return status;

		unsigned n;
		for(n=0;n<links;n++)
		{
			metadb_handle_ptr item;
			if (!p_callback.handle_create(item,make_playable_location(p_path,n))) return io_result_error_generic;
			p_callback.on_entry(item,stats);
		}

		return io_result_success;
	}
};

static service_factory_single_t<track_indexer,track_indexer_ogg> foo1;


DECLARE_FILE_TYPE("Ogg files","*.OGG");