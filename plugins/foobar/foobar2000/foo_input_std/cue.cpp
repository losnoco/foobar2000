#include "stdafx.h"
#include "../helpers/helpers.h"

static const char * cue_fields[] =
{
	"TRACKNUMBER","TITLE","ARTIST","ALBUM",
};

static t_io_result cue_parse(const service_ptr_t<file> & p_file,list_base_t<cue_parser::cue_entry*> & out,const playable_location & p_location,file_info * info,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("cue_parse");

	string8 cuedata;
	{
		bool uninteresting;
		t_io_result status;
		status = text_file_loader::read(p_file,p_abort,cuedata,uninteresting);
		if (io_result_failed(status)) return status;
	}

	if (!cue_parser::parse(cuedata,out,info,p_location.get_subsong())) return io_result_error_data;

	return io_result_success;
}


class input_cue : public input
{
private:
	file_info_i m_info;
	playable_location_i m_location;

	ptr_list_t<cue_parser::cue_entry> cue_data;

	int cue_track;
	double time_done,dur;
	double start,end;

	input_helper in;

public:

    t_io_result set_info(const service_ptr_t<file> & p_file, const playable_location & p_location,file_info & p_info, abort_callback & p_abort)
    {
		TRACK_CALL_TEXT("input_cue::set_info");

		t_io_result status;
		string8 old_cue,new_cue;
		bool is_utf8;

		status = text_file_loader::read(p_file,p_abort,old_cue,is_utf8);
		if (io_result_failed(status)) return status;

		if (!cue_parser::modify(old_cue,new_cue,p_location.get_subsong(),p_info))
			return io_result_error_data;

		status = text_file_loader::write(p_file,p_abort,new_cue,is_utf8);
		if (io_result_failed(status)) return status;

		return io_result_success;
    }

	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_cue::get_info");

		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		TRACK_CALL_TEXT("input_cue::open");

		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		TRACK_CALL_TEXT("input_cue::open_internal");

		if (!p_file.is_valid()) return io_result_error_data;
		
		cue_data.delete_all();
		cue_track = 0;
		time_done = 0; dur = 0;
		start = 0; end = 0;

		{
			t_io_result status;
			status = cue_parse(p_file,cue_data,p_location,&p_info, p_abort);
			if (io_result_failed(status)) return status;
		}

		int track = p_location.get_subsong();
		unsigned n;
		start = -1;
		end = -1;
        double index = 0;
		string8 filename_short;

		{
			char temp[16];
			sprintf(temp,"%u",track);
			p_info.meta_set("TRACKNUMBER",temp);
		}

		for(n=0;n<cue_data.get_count();n++)
		{
			if (cue_data[n]->track == track)
			{
				filename_short = cue_data[n]->file;
				start = cue_data[n]->start;
                index = cue_data[n]->index;
				break;
			}
		}

		for(n=0;n<cue_data.get_count();n++)
		{
			if (cue_data[n]->track == track+1)
			{
				if (!stricmp_utf8(cue_data[n]->file,filename_short))
					end = cue_data[n]->start;
			}
		}

		if (start<0) return io_result_error_data;

		string8 filename = p_location.get_path();
		filename.truncate(filename.scan_filename());
		filename += filename_short;
		if (!filesystem::g_exists(filename,p_abort))
		{
			filename = file_path_canonical(filename_short);
			if (!filesystem::g_exists(filename,p_abort))
			{
//				console::error("referenced file doesn't exist");
				return io_result_error_data;
			}
		}

		if (m_location.compare(make_playable_location(filename,0))!=0)
		{
			m_info.reset();
			m_location.copy(make_playable_location(filename,0));
			if (p_decode)
			{
				t_io_result status;
				status = in.open(m_location,m_info,p_abort);
				if (io_result_failed(status))
				{
					if (status == io_result_error_not_found) status = io_result_error_data;
					return status;
				}
			}
			else
			{
				t_io_result status;
				status = input_entry::g_get_info(m_location,m_info,p_abort);
				if (io_result_failed(status))
				{
					if (status == io_result_error_not_found) status = io_result_error_data;
					return status;
				}
			}
		}

		if (end<start) end = m_info.get_length();

		if (end<start) return io_result_error_data;

		if (p_decode)
		{
			t_io_result status;
			status = in.seek(start,p_abort);
			if (io_result_failed(status)) return status;
		}

        dur = end - start;

		{
			unsigned n;
			for(n=0;n<m_info.info_get_count();n++)
			{
				p_info.info_set(m_info.info_enum_name(n),m_info.info_enum_value(n));
			}

			p_info.set_replaygain(replaygain_info::g_merge(p_info.get_replaygain(),m_info.get_replaygain()));

            /*
            const char *ext = (const char *)strrchr(filename, '.');
            if (ext) {
			    p_info->info_set("cue_audiotype",ext+1);
            }
            */
            unsigned srate = 44100;
            /*
            const char *base = (const char *)strrchr(filename, '\\');
            if (!base) base = filename; else base++;
			p_info->info_set("referenced_file",base);
            */
			p_info.info_set("referenced_file",filename_short);

            const char *s = m_info.info_get("samplerate");
            if (s) srate = atoi(s);

			if ( index > 0 && srate > 0 ) p_info.info_set_int("index",(t_int64)(index*srate));

			for(n=0;n<m_info.meta_get_count();n++)
			{
				const char * name = m_info.meta_enum_name(n);
				unsigned z;
				bool ignore = 0;
				for(z=0;z<tabsize(cue_fields);z++)
				{
					if (!stricmp_utf8(name,cue_fields[z]))
					{
						ignore=1;
						break;
					}
				}

				if (!ignore)
					p_info.copy_meta_single(m_info,n);
			}
		}

		p_info.set_length(dur);

		time_done = 0;

		return io_result_success;
	}

	inline static bool g_test_filename(const char * fn,const char * ext) {return !stricmp_utf8(ext,"CUE");}

	input_cue()
	{
		cue_track=0;
	}

	~input_cue()
	{
		cue_data.delete_all();
	}

	t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_cue::run");

		if (time_done >= dur) return io_result_eof;

		t_io_result rv = in.run(chunk,p_abort);
		if (rv == io_result_success)
		{
			double delta = chunk->get_duration();

			if (time_done + delta > dur)
			{
                delta = dur - time_done;
				if (delta<0) delta = 0;
				time_done = dur;
                unsigned samples = (unsigned)(delta * (double)chunk->get_srate() + 0.5);
				if (samples<chunk->get_sample_count()) chunk->set_sample_count(samples);
                if (samples == 0) rv = io_result_eof;
			}
			else time_done += delta;
		}
		else if (rv==io_result_eof && time_done==0) rv = io_result_error_generic;
		return rv;
	}

	t_io_result seek(double seconds,abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_cue::seek");

		time_done=seconds;
		return in.seek(time_done+start,p_abort);
	}

	bool get_dynamic_info(file_info & out, double * timestamp_delta,bool * b_track_change)
	{
		TRACK_CALL_TEXT("input_cue::get_dynamic_info");

		static const char * infos_table[] = {"bitrate_dynamic","samplerate","bitrate"};
		if (in.get_dynamic_info(m_info,timestamp_delta,b_track_change))
		{
			unsigned n,m = m_info.info_get_count();
			for(n=0;n<m;n++)
			{
				const char * name = m_info.info_enum_name(n);
				unsigned i;
				for(i=0;i<tabsize(infos_table);i++)
				{
					if (!stricmp_utf8(name,infos_table[i]))
					{
						out.info_set(name,m_info.info_enum_value(n));
						break;
					}
				}
			}
			
			return true;
		}
		else return false;
	}

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}
	inline static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {F1600556-CBBA-4f70-B2DD-093CDF2DE236}
		static const GUID guid = 
		{ 0xf1600556, 0xcbba, 0x4f70, { 0xb2, 0xdd, 0x9, 0x3c, 0xdf, 0x2d, 0xe2, 0x36 } };
		return guid;
	}

	static const char * g_get_name() {return "Cuesheet parser";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

class indexer_cue : public track_indexer
{
private:
	t_io_result get_tracks(const char * p_path,const service_ptr_t<file> & p_reader,track_indexer_callback & p_callback)
	{
		TRACK_CALL_TEXT("indexer_cue::get_tracks");

		if (stricmp_utf8(string_extension_8(p_path),"CUE")) return io_result_error_data;

		t_io_result status;

		service_ptr_t<file> r = p_reader;

		if (r.is_empty())
		{
			status = filesystem::g_open_precache(r,p_path,p_callback);
			if (io_result_failed(status)) return status;
		}
		
		ptr_list_autodel_t<cue_parser::cue_entry> cue_data;
		{
			status = cue_parse(r,cue_data,make_playable_location(p_path,0),0,p_callback);
			if (io_result_failed(status)) return status;
		}
		t_filestats stats;
		status = r->get_stats(stats,p_callback);
		if (io_result_failed(status)) return status;
		unsigned n;
		for(n=0;n<cue_data.get_count();n++)
		{
			metadb_handle_ptr item;
			if (!p_callback.handle_create(item,make_playable_location(p_path,cue_data[n]->track))) return io_result_error_generic;
			p_callback.on_entry(item,stats);
		}
		return io_result_success;
	}
};

static input_factory_t<input_cue> g_input_cue_factory;
static service_factory_single_t<track_indexer,indexer_cue> foo2;


DECLARE_FILE_TYPE("Cue files","*.CUE");