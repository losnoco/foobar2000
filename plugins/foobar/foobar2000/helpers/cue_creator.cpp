#include "stdafx.h"


namespace {
	class format_cuetime
	{
	public:
		format_cuetime(double p_time)
		{
			t_uint64 ticks = dsp_util::duration_samples_from_time(p_time,75);
			t_uint64 seconds = ticks / 75; ticks %= 75;
			t_uint64 minutes = seconds / 60; seconds %= 60;
			m_buffer << format_uint(minutes,2) << ":" << format_uint(seconds,2) << ":" << format_uint(ticks,2);
		}

		inline operator const char*() const {return m_buffer;}
	private:
		string_formatter m_buffer;
	};

	class format_meta
	{
	public:
		format_meta(const file_info & p_source,const char * p_name)
		{
			p_source.meta_format(p_name,m_buffer);
			m_buffer.replace_byte('\"','\'');
		}
		inline operator const char*() const {return m_buffer;}
	private:
		string8_fastalloc m_buffer;
	};
}

static bool is_meta_same_everywhere(const cue_creator::t_entry_list & p_list,const char * p_meta)
{
	string8_fastalloc reference,temp;

	cue_creator::t_entry_list::const_iterator iter;
	iter = p_list.first();
	if (!iter.is_valid()) return false;
	if (!iter->m_infos.meta_format(p_meta,reference)) return false;
	for(;iter.is_valid();++iter)
	{
		if (!iter->m_infos.meta_format(p_meta,temp)) return false;
		if (strcmp(temp,reference)!=0) return false;
	}
	return true;
}

static const char g_eol[] = "\r\n";


namespace cue_creator
{
	void create(string_formatter & p_out,const t_entry_list & p_data)
	{
		if (p_data.get_count() == 0) return;
		bool album_artist_global =	is_meta_same_everywhere(p_data,"album artist"),
			artist_global =			is_meta_same_everywhere(p_data,"artist"),
			album_global =			is_meta_same_everywhere(p_data,"album"),
			genre_global =			is_meta_same_everywhere(p_data,"genre"),
			date_global =			is_meta_same_everywhere(p_data,"date"),
			discid_global =			is_meta_same_everywhere(p_data,"discid"),
			comment_global =		is_meta_same_everywhere(p_data,"comment"),
			catalog_global =		is_meta_same_everywhere(p_data,"catalog"),
			isrc_global =			is_meta_same_everywhere(p_data,"isrc");

		if (genre_global)
		{
			p_out << "REM GENRE " << format_meta(p_data.first()->m_infos,"genre") << g_eol;
		}
		if (date_global)
		{
			p_out << "REM DATE " << format_meta(p_data.first()->m_infos,"date") << g_eol;
		}
		if (discid_global)
		{
			p_out << "REM DISCID " << format_meta(p_data.first()->m_infos,"discid") << g_eol;
		}
		if (comment_global)
		{
			p_out << "REM COMMENT " << format_meta(p_data.first()->m_infos,"comment") << g_eol;
		}

		if (catalog_global)
		{
			p_out << "CATALOG " << format_meta(p_data.first()->m_infos,"catalog") << g_eol;
		}

		if (isrc_global)
		{
			p_out << "ISRC " << format_meta(p_data.first()->m_infos,"isrc") << g_eol;
		}

		if (album_artist_global)
		{
			p_out << "PERFORMER \"" << format_meta(p_data.first()->m_infos,"album artist") << "\"" << g_eol;
			artist_global = false;
		}
		else if (artist_global)
		{
			p_out << "PERFORMER \"" << format_meta(p_data.first()->m_infos,"artist") << "\"" << g_eol;
		}
		if (album_global)
		{
			p_out << "TITLE \"" << format_meta(p_data.first()->m_infos,"album") << "\"" << g_eol;
		}

		{
			replaygain_info::t_text_buffer rgbuffer;
			replaygain_info rg = p_data.first()->m_infos.get_replaygain();
			if (rg.format_album_gain(rgbuffer))
				p_out << "REM REPLAYGAIN_ALBUM_GAIN " << rgbuffer << g_eol;
			if (rg.format_album_peak(rgbuffer))
				p_out << "REM REPLAYGAIN_ALBUM_PEAK " << rgbuffer << g_eol;			
		}

		string8 last_file;

		for(t_entry_list::const_iterator iter = p_data.first();iter.is_valid();++iter)
		{
			if (strcmp(last_file,iter->m_file) != 0)
			{
				p_out << "FILE \"" << iter->m_file << "\" WAVE" << g_eol;
				last_file = iter->m_file;
			}

			p_out << "  TRACK " << format_int(iter->m_track_number,2) << " AUDIO" << g_eol;

			if (iter->m_infos.meta_find("title") != infinite)
				p_out << "    TITLE \"" << format_meta(iter->m_infos,"title") << "\"" << g_eol;
			
			if (!artist_global && iter->m_infos.meta_find("artist") != infinite)
				p_out << "    PERFORMER \"" << format_meta(iter->m_infos,"artist") << "\"" << g_eol;


			{
				replaygain_info::t_text_buffer rgbuffer;
				replaygain_info rg = iter->m_infos.get_replaygain();
				if (rg.format_track_gain(rgbuffer))
					p_out << "    REM REPLAYGAIN_TRACK_GAIN " << rgbuffer << g_eol;
				if (rg.format_track_peak(rgbuffer))
					p_out << "    REM REPLAYGAIN_TRACK_PEAK " << rgbuffer << g_eol;			
			}
			
			{
				t_index_list::const_iterator indexiter;
				for(indexiter = iter->m_index_list.first();indexiter.is_valid();++indexiter)
				{
					p_out << "    INDEX " << format_uint(indexiter->m_index,2) << " " <<format_cuetime(indexiter->m_offset) << g_eol;
				}
			}

			// p_out << "    INDEX 01 " << format_cuetime(iter->m_offset) << g_eol;
		}
	}


	void t_entry::set_simple_index(double p_time)
	{
		m_index_list.remove_all();
		t_index_list::iterator iter = m_index_list.insert_last();
		iter->m_index = 1;
		iter->m_offset = p_time;
	}

}

