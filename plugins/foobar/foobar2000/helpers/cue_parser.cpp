#include "stdafx.h"

namespace {
	class exception_cue : public std::exception {
	public:
		exception_cue(const char * p_msg) : exception(p_msg,0) {}
		exception_cue(const exception_cue & p_exception) {*this = p_exception;}
	};
};

static bool is_numeric(char c) {return c>='0' && c<='9';}


static bool is_spacing(char c)
{
	return c == ' ' || c == '\t';
}

static bool is_linebreak(char c)
{
	return c == 10 || c == 13;
}

namespace {
	
	class NOVTABLE cue_parser_callback
	{
	public:
		virtual void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length) = 0;
		virtual void on_track(unsigned p_index,const char * p_type,unsigned p_type_length) = 0;
		virtual void on_pregap(unsigned p_value) = 0;
		virtual void on_index(unsigned p_index,unsigned p_value) = 0;
		virtual void on_title(const char * p_title,unsigned p_title_length) = 0;
		virtual void on_performer(const char * p_performer,unsigned p_performer_length) = 0;
		virtual void on_songwriter(const char * p_songwriter,unsigned p_songwriter_length) = 0;
		virtual void on_isrc(const char * p_isrc,unsigned p_isrc_length) = 0;
		virtual void on_catalog(const char * p_catalog,unsigned p_catalog_length) = 0;
		virtual void on_comment(const char * p_comment,unsigned p_comment_length) = 0;
	};

	class NOVTABLE cue_parser_callback_meta : public cue_parser_callback
	{
	public:
		virtual void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length) = 0;
		virtual void on_track(unsigned p_index,const char * p_type,unsigned p_type_length) = 0;
		virtual void on_pregap(unsigned p_value) = 0;
		virtual void on_index(unsigned p_index,unsigned p_value) = 0;
		virtual void on_meta(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) = 0;
	protected:
		static bool is_known_meta(const char * p_name,unsigned p_length)
		{
			static const char * metas[] = {"genre","date","discid","comment","replaygain_track_gain","replaygain_track_peak","replaygain_album_gain","replaygain_album_peak"};
			for(unsigned n=0;n<tabsize(metas);n++)
			{
				if (!stricmp_utf8_ex(p_name,p_length,metas[n],infinite)) return true;
			}
			return false;
		}

		void on_comment(const char * p_comment,unsigned p_comment_length)
		{
			unsigned ptr = 0;
			while(ptr < p_comment_length && !is_spacing(p_comment[ptr])) ptr++;
			if (is_known_meta(p_comment, ptr))
			{
				unsigned name_length = ptr;
				while(ptr < p_comment_length && is_spacing(p_comment[ptr])) ptr++;
				if (ptr < p_comment_length)
				{
					if (p_comment[ptr] == '\"')
					{
						ptr++;
						unsigned value_base = ptr;
						while(ptr < p_comment_length && p_comment[ptr] != '\"') ptr++;
						if (ptr == p_comment_length) throw exception_cue("invalid REM syntax");
						if (ptr > value_base) on_meta(p_comment,name_length,p_comment + value_base,ptr - value_base);
					}
					else
					{
						unsigned value_base = ptr;
						while(ptr < p_comment_length /*&& !is_spacing(p_comment[ptr])*/) ptr++;
						if (ptr > value_base) on_meta(p_comment,name_length,p_comment + value_base,ptr - value_base);
					}
				}
			}
		}
		void on_title(const char * p_title,unsigned p_title_length)
		{
			on_meta("title",5,p_title,p_title_length);
		}
		void on_songwriter(const char * p_songwriter,unsigned p_songwriter_length) {
			on_meta("songwriter",9,p_songwriter,p_songwriter_length);
		}
		void on_performer(const char * p_performer,unsigned p_performer_length)
		{
			on_meta("artist",6,p_performer,p_performer_length);
		}

		void on_isrc(const char * p_isrc,unsigned p_isrc_length)
		{
			on_meta("isrc",4,p_isrc,p_isrc_length);
		}
		void on_catalog(const char * p_catalog,unsigned p_catalog_length)
		{
			on_meta("catalog",7,p_catalog,p_catalog_length);
		}
	};


	class cue_parser_callback_retrievelist : public cue_parser_callback
	{
	public:
		cue_parser_callback_retrievelist(pfc::chain_list_t<cue_parser::cue_entry> & p_out) : m_out(p_out), m_track(0), m_pregap(0), m_index0_set(false), m_index1_set(false)
		{
		}
		
		void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length)
		{
			if (stricmp_utf8_ex(p_type,p_type_length,"WAVE",infinite) && stricmp_utf8_ex(p_type,p_type_length,"MP3",infinite) && stricmp_utf8_ex(p_type,p_type_length,"AIFF",infinite)) throw exception_cue("only files of WAVE, MP3 and AIFF type supported");
			m_file.set_string(p_file,p_file_length);
		}
		
		void on_track(unsigned p_index,const char * p_type,unsigned p_type_length)
		{
			if (stricmp_utf8_ex(p_type,p_type_length,"audio",infinite)) throw exception_cue("only tracks of type AUDIO supporteD");
			//if (p_index != m_track + 1) throw exception_cue("cuesheet tracks out of order");
			if (m_track != 0) finalize_track();
			if (m_file.is_empty()) throw exception_cue("declaring a track with no file set");
			m_trackfile = m_file;
			m_track = p_index;
		}

		void on_pregap(unsigned p_value) {m_pregap = (double) p_value / 75.0;}

		void on_index(unsigned p_index,unsigned p_value)
		{
			if (p_index < t_cuesheet_index_list::count)
			{
				switch(p_index)
				{
				case 0: m_index0_set = true; break;
				case 1: m_index1_set = true; break;
				}
				m_index_list.m_positions[p_index] = (double) p_value / 75.0;
			}
		}

		void on_title(const char * p_title,unsigned p_title_length) {}
		void on_performer(const char * p_performer,unsigned p_performer_length) {}
		void on_songwriter(const char * p_songwriter,unsigned p_songwriter_length) {}
		void on_isrc(const char * p_isrc,unsigned p_isrc_length) {}
		void on_catalog(const char * p_catalog,unsigned p_catalog_length) {}
		void on_comment(const char * p_comment,unsigned p_comment_length) {}

		void finalize()
		{
			if (m_track != 0)
			{
				finalize_track();
				m_track = 0;
			}
		}

	private:
		void finalize_track()
		{
			if (!m_index1_set) throw exception_cue("INDEX 01 not set");
			if (!m_index0_set) m_index_list.m_positions[0] = m_index_list.m_positions[1] - m_pregap;

			pfc::chain_list_t<cue_parser::cue_entry>::iterator iter;
			iter = m_out.insert_last();
			if (m_trackfile.is_empty()) throw exception_cue("track has no file assigned");
			iter->m_file = m_trackfile;
			iter->m_track_number = m_track;
			iter->m_indexes = m_index_list;
			
			m_index_list.reset();
			m_index0_set = false;
			m_index1_set = false;
			m_pregap = 0;
		}

		bool m_index0_set,m_index1_set;
		t_cuesheet_index_list m_index_list;
		double m_pregap;
		unsigned m_track;
		string8 m_file,m_trackfile;
		pfc::chain_list_t<cue_parser::cue_entry> & m_out;
	};

	class cue_parser_callback_retrieveinfo : public cue_parser_callback_meta
	{
	public:
		cue_parser_callback_retrieveinfo(file_info & p_out,unsigned p_wanted_track) : m_out(p_out), m_wanted_track(p_wanted_track), m_track(0), m_is_va(false), m_index0_set(false), m_index1_set(false), m_pregap(0) {}

		void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length) {}

		void on_track(unsigned p_index,const char * p_type,unsigned p_type_length)
		{
			if (p_index == 0) throw exception_cue("invalid TRACK index");
			if (p_index == m_wanted_track)
			{
				if (stricmp_utf8_ex(p_type,p_type_length,"audio",infinite)) throw exception_cue("only tracks of type AUDIO supporteD");
			}
			m_track = p_index;
			
		}

		void on_pregap(unsigned p_value) {if (m_track == m_wanted_track) m_pregap = (double) p_value / 75.0;}

		void on_index(unsigned p_index,unsigned p_value)
		{
			if (m_track == m_wanted_track && p_index < t_cuesheet_index_list::count)
			{
				switch(p_index)
				{
				case 0:	m_index0_set = true; break;
				case 1: m_index1_set = true; break;
				}
				m_indexes.m_positions[p_index] = (double) p_value / 75.0;
			}
		}

		
		void on_meta(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length)
		{
			t_meta_list::iterator iter;
			if (m_track == 0) //globals
			{
				//convert global title to album
				if (!stricmp_utf8_ex(p_name,p_name_length,"title",infinite))
				{
					p_name = "album";
					p_name_length = 5;
				}
				else if (!stricmp_utf8_ex(p_name,p_name_length,"artist",infinite))
				{
					m_album_artist.set_string(p_value,p_value_length);
				}

				iter = m_globals.insert_last();
			}
			else
			{
				if (!m_is_va)
				{
					if (!stricmp_utf8_ex(p_name,p_name_length,"artist",infinite))
					{
						if (!m_album_artist.is_empty())
						{
							if (stricmp_utf8_ex(p_value,p_value_length,m_album_artist,m_album_artist.length())) m_is_va = true;
						}
					}
				}

				if (m_track == m_wanted_track) //locals
				{
					iter = m_locals.insert_last();
				}
			}
			if (iter.is_valid())
			{
				iter->m_name.set_string(p_name,p_name_length);
				iter->m_value.set_string(p_value,p_value_length);
			}
		}

		void finalize()
		{
			if (!m_index1_set) throw exception_cue("INDEX 01 not set");
			if (!m_index0_set) m_indexes.m_positions[0] = m_indexes.m_positions[1] - m_pregap;
			m_indexes.to_infos(m_out);

			replaygain_info rg;
			rg.reset();
			t_meta_list::const_iterator iter;

			if (m_is_va)
			{
				//clean up VA mess

				t_meta_list::const_iterator iter_global,iter_local;

				iter_global = find_first_field(m_globals,"artist");
				iter_local = find_first_field(m_locals,"artist");
				if (iter_global.is_valid())
				{
					m_out.meta_set("album artist",iter_global->m_value);
					if (iter_local.is_valid()) m_out.meta_set("artist",iter_local->m_value);
					else m_out.meta_set("artist",iter_global->m_value);
				}
				else
				{
					if (iter_local.is_valid()) m_out.meta_set("artist",iter_local->m_value);
				}
				

				wipe_field(m_globals,"artist");
				wipe_field(m_locals,"artist");
				
			}

			for(iter=m_globals.first();iter.is_valid();iter++)
			{
				if (!rg.set_from_meta(iter->m_name,iter->m_value))
					m_out.meta_set(iter->m_name,iter->m_value);
			}
			for(iter=m_locals.first();iter.is_valid();iter++)
			{
				if (!rg.set_from_meta(iter->m_name,iter->m_value))
					m_out.meta_set(iter->m_name,iter->m_value);
			}
			m_out.meta_set("tracknumber",string_formatter() << m_wanted_track);
			m_out.set_replaygain(rg);

		}
	private:
		struct t_meta_entry {
			string8 m_name,m_value;
		};
		typedef pfc::chain_list_t<t_meta_entry> t_meta_list;

		static t_meta_list::const_iterator find_first_field(t_meta_list const & p_list,const char * p_field)
		{
			t_meta_list::const_iterator iter;
			for(iter=p_list.first();iter.is_valid();++iter)
			{
				if (!stricmp_utf8(p_field,iter->m_name)) return iter;
			}
			return t_meta_list::const_iterator();//null iterator
		}

		static void wipe_field(t_meta_list & p_list,const char * p_field)
		{
			t_meta_list::iterator iter;
			for(iter=p_list.first();iter.is_valid();)
			{
				if (!stricmp_utf8(p_field,iter->m_name))
				{
					t_meta_list::iterator temp = iter;
					++temp;
					p_list.remove_single(iter);
					iter = temp;
				}
				else
				{
					++iter;
				}
			}
		}
		
		t_meta_list m_globals,m_locals;
		file_info & m_out;
		unsigned m_wanted_track, m_track;
		string8 m_album_artist;
		bool m_is_va;
		t_cuesheet_index_list m_indexes;
		bool m_index0_set,m_index1_set;
		double m_pregap;
	};

};


static void g_parse_cue_line(const char * p_line,unsigned p_line_length,cue_parser_callback & p_callback)
{
	unsigned ptr = 0;
	while(ptr < p_line_length && !is_spacing(p_line[ptr])) ptr++;
	if (!stricmp_utf8_ex(p_line,ptr,"file",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		unsigned file_base,file_length, type_base,type_length;
		
		if (p_line[ptr] == '\"')
		{
			ptr++;
			file_base = ptr;
			while(ptr < p_line_length && p_line[ptr] != '\"') ptr++;
			if (ptr == p_line_length) throw exception_cue("invalid FILE syntax");
			file_length = ptr - file_base;
			ptr++;
			while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		}
		else
		{
			file_base = ptr;
			while(ptr < p_line_length && !is_spacing(p_line[ptr])) ptr++;
			file_length = ptr - file_base;
			while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		}

		type_base = ptr;
		while(ptr < p_line_length && !is_spacing(p_line[ptr])) ptr++;
		type_length = ptr - type_base;
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;

		if (ptr != p_line_length || file_length == 0 || type_length == 0) throw exception_cue("invalid FILE syntax");

		p_callback.on_file(p_line + file_base, file_length, p_line + type_base, type_length);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"track",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;

		unsigned track_base = ptr, track_length;
		while(ptr < p_line_length && !is_spacing(p_line[ptr]))
		{
			if (!is_numeric(p_line[ptr])) throw exception_cue("invalid TRACK syntax");
			ptr++;
		}
		track_length = ptr - track_base;
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		
		unsigned type_base = ptr, type_length;
		while(ptr < p_line_length && !is_spacing(p_line[ptr])) ptr++;
		type_length = ptr - type_base;

		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		if (ptr != p_line_length || type_length == 0) throw exception_cue("invalid TRACK syntax");
		unsigned track = atoui_ex(p_line+track_base,track_length);
		if (track < 1 || track > 99) throw exception_cue("invalid track number");

		p_callback.on_track(track,p_line + type_base, type_length);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"index",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;

		unsigned index_base,index_length, time_base,time_length;
		index_base = ptr;
		while(ptr < p_line_length && !is_spacing(p_line[ptr]))
		{
			if (!is_numeric(p_line[ptr])) throw exception_cue("invalid INDEX syntax");
			ptr++;
		}
		index_length = ptr - index_base;
		
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		time_base = ptr;
		while(ptr < p_line_length && !is_spacing(p_line[ptr]))
		{
			if (!is_numeric(p_line[ptr]) && p_line[ptr] != ':')
				throw exception_cue("invalid INDEX syntax");
			ptr++;
		}
		time_length = ptr - time_base;

		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		
		if (ptr != p_line_length || index_length == 0 || time_length == 0)
			throw exception_cue("invalid INDEX syntax");

		unsigned index = atoui_ex(p_line+index_base,index_length);
		if (index > 99) throw exception_cue("invalid INDEX syntax");
		unsigned time = cuesheet_parse_index_time_ticks_e(p_line + time_base,time_length);
		
		p_callback.on_index(index,time);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"pregap",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;

		unsigned time_base, time_length;
		time_base = ptr;
		while(ptr < p_line_length && !is_spacing(p_line[ptr]))
		{
			if (!is_numeric(p_line[ptr]) && p_line[ptr] != ':')
				throw exception_cue("invalid PREGAP syntax");
			ptr++;
		}
		time_length = ptr - time_base;

		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		
		if (ptr != p_line_length || time_length == 0)
			throw exception_cue("invalid PREGAP syntax");

		unsigned time = cuesheet_parse_index_time_ticks_e(p_line + time_base,time_length);
		
		p_callback.on_pregap(time);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"title",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		if (ptr == p_line_length) throw exception_cue("invalid TITLE syntax");
		if (p_line[ptr] == '\"')
		{
			ptr++;
			unsigned base = ptr;
			while(ptr < p_line_length && p_line[ptr] != '\"') ptr++;
			if (ptr == p_line_length) throw exception_cue("invalid TITLE syntax");
			unsigned length = ptr-base;
			ptr++;
			while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
			if (ptr != p_line_length) throw exception_cue("invalid TITLE syntax");
			p_callback.on_title(p_line+base,length);
		}
		else
		{
			p_callback.on_title(p_line+ptr,p_line_length-ptr);
		}
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"performer",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		if (ptr == p_line_length) throw exception_cue("invalid PERFORMER syntax");
		if (p_line[ptr] == '\"')
		{
			ptr++;
			unsigned base = ptr;
			while(ptr < p_line_length && p_line[ptr] != '\"') ptr++;
			if (ptr == p_line_length) throw exception_cue("invalid PERFORMER syntax");
			unsigned length = ptr-base;
			ptr++;
			while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
			if (ptr != p_line_length) throw exception_cue("invalid PERFORMER syntax");
			p_callback.on_performer(p_line+base,length);
		}
		else
		{
			p_callback.on_performer(p_line+ptr,p_line_length-ptr);
		}
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"songwriter",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		if (ptr == p_line_length) throw exception_cue("invalid SONGWRITER syntax");
		if (p_line[ptr] == '\"')
		{
			ptr++;
			unsigned base = ptr;
			while(ptr < p_line_length && p_line[ptr] != '\"') ptr++;
			if (ptr == p_line_length) throw exception_cue("invalid SONGWRITER syntax");
			unsigned length = ptr-base;
			ptr++;
			while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
			if (ptr != p_line_length) throw exception_cue("invalid SONGWRITER syntax");
			p_callback.on_songwriter(p_line+base,length);
		}
		else
		{
			p_callback.on_songwriter(p_line+ptr,p_line_length-ptr);
		}
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"isrc",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		unsigned length = p_line_length - ptr;
		if (length == 0) throw exception_cue("invalid ISRC syntax");
		p_callback.on_isrc(p_line+ptr,length);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"catalog",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		unsigned length = p_line_length - ptr;
		if (length == 0) throw exception_cue("invalid CATALOG syntax");
		p_callback.on_catalog(p_line+ptr,length);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"flags",infinite))
	{
		//todo?
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"rem",infinite))
	{
		while(ptr < p_line_length && is_spacing(p_line[ptr])) ptr++;
		if (ptr < p_line_length)
			p_callback.on_comment(p_line + ptr, p_line_length - ptr);
	}
	else if (!stricmp_utf8_ex(p_line,ptr,"postgap",infinite)) {
		throw exception_cue("POSTGAP is not supported");
	} else if (!stricmp_utf8_ex(p_line,ptr,"cdtextfile",infinite)) {
		//do nothing
	}
	else throw exception_cue("unknown cuesheet item");
}

static void g_parse_cue(const char * p_cuesheet,cue_parser_callback & p_callback)
{
	const char * parseptr = p_cuesheet;
	while(*parseptr)
	{
		while(is_spacing(*parseptr)) *parseptr++;
		if (*parseptr)
		{
			unsigned length = 0;
			while(parseptr[length] && !is_linebreak(parseptr[length])) length++;
			if (length > 0)
			{
				g_parse_cue_line(parseptr,length,p_callback);
			}
			parseptr += length;
			while(is_linebreak(*parseptr)) parseptr++;
		}
	}
}

bool cue_parser::parse(const char *p_cuesheet,pfc::chain_list_t<cue_entry> & p_out)
{
	try {
		cue_parser_callback_retrievelist callback(p_out);
		g_parse_cue(p_cuesheet,callback);
		callback.finalize();
		return true;
	} catch(exception_cue const & e)
	{
		console::formatter() << "cuesheet parsing error : " << e.what();
		return false;
	}
}
bool cue_parser::parse_info(const char * p_cuesheet,file_info & p_info,unsigned p_index)
{
	try {
		cue_parser_callback_retrieveinfo callback(p_info,p_index);
		g_parse_cue(p_cuesheet,callback);
		callback.finalize();
		return true;
	} catch(exception_cue const & e)
	{
		console::formatter() << "cuesheet parsing error : " << e.what();
		return false;
	}
}

namespace {

	class cue_parser_callback_retrievecount : public cue_parser_callback
	{
	public:
		cue_parser_callback_retrievecount() : m_count(0) {}
		unsigned get_count() const {return m_count;}
		void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length) {}
		void on_track(unsigned p_index,const char * p_type,unsigned p_type_length) {m_count++;}
		void on_pregap(unsigned p_value) {}
		void on_index(unsigned p_index,unsigned p_value) {}
		void on_title(const char * p_title,unsigned p_title_length) {}
		void on_performer(const char * p_performer,unsigned p_performer_length) {}
		void on_isrc(const char * p_isrc,unsigned p_isrc_length) {}
		void on_catalog(const char * p_catalog,unsigned p_catalog_length) {}
		void on_comment(const char * p_comment,unsigned p_comment_length) {}
	private:
		unsigned m_count;
	};

	class cue_parser_callback_retrievecreatorentries : public cue_parser_callback
	{
	public:
		cue_parser_callback_retrievecreatorentries(cue_creator::t_entry_list & p_out) : m_out(p_out), m_track(0), m_pregap(0), m_index0_set(false), m_index1_set(false) {}

		void on_file(const char * p_file,unsigned p_file_length,const char * p_type,unsigned p_type_length)
		{
			if (stricmp_utf8_ex(p_type,p_type_length,"WAVE",infinite) && stricmp_utf8_ex(p_type,p_type_length,"MP3",infinite) && stricmp_utf8_ex(p_type,p_type_length,"AIFF",infinite)) throw exception_cue("only files of WAVE, MP3 and AIFF type supported");
			m_file.set_string(p_file,p_file_length);
		}
		
		void on_track(unsigned p_index,const char * p_type,unsigned p_type_length)
		{
			if (stricmp_utf8_ex(p_type,p_type_length,"audio",infinite)) throw exception_cue("only tracks of type AUDIO supporteD");
			//if (p_index != m_track + 1) throw exception_cue("cuesheet tracks out of order");
			if (m_track != 0) finalize_track();
			if (m_file.is_empty()) throw exception_cue("declaring a track with no file set");
			m_trackfile = m_file;
			m_track = p_index;
		}
		
		void on_pregap(unsigned p_value)
		{
			m_pregap = (double) p_value / 75.0;
		}

		void on_index(unsigned p_index,unsigned p_value)
		{
			if (p_index < t_cuesheet_index_list::count)
			{
				switch(p_index)
				{
				case 0:	m_index0_set = true; break;
				case 1: m_index1_set = true; break;
				}
				m_indexes.m_positions[p_index] = (double) p_value / 75.0;
			}
		}
		void on_title(const char * p_title,unsigned p_title_length) {}
		void on_performer(const char * p_performer,unsigned p_performer_length) {}
		void on_songwriter(const char * p_performer,unsigned p_performer_length) {}
		void on_isrc(const char * p_isrc,unsigned p_isrc_length) {}
		void on_catalog(const char * p_catalog,unsigned p_catalog_length) {}
		void on_comment(const char * p_comment,unsigned p_comment_length) {}		
		void finalize()
		{
			if (m_track != 0)
			{
				finalize_track(); 
				m_track = 0;
			}
		}
	private:
		void finalize_track()
		{
			if (m_track < 1 || m_track > 99) throw exception_cue("track number out of range");
			if (!m_index1_set) throw exception_cue("INDEX 01 not set");
			if (!m_index0_set) m_indexes.m_positions[0] = m_indexes.m_positions[1] - m_pregap;

			cue_creator::t_entry_list::iterator iter;
			iter = m_out.insert_last();
			iter->m_track_number = m_track;
			iter->m_file = m_trackfile;
			iter->m_index_list = m_indexes;			
			m_pregap = 0;
			m_indexes.reset();
			m_index0_set = m_index1_set = false;
		}

		bool m_index0_set,m_index1_set;
		double m_pregap;
		unsigned m_track;
		cue_creator::t_entry_list & m_out;
		string8 m_file,m_trackfile;
		t_cuesheet_index_list m_indexes;
	};
}

bool cue_parser::parse_full(const char * p_cuesheet,cue_creator::t_entry_list & p_out)
{
	try {

		{
			cue_parser_callback_retrievecreatorentries callback(p_out);
			g_parse_cue(p_cuesheet,callback);
			callback.finalize();
		}

		{
			cue_creator::t_entry_list::iterator iter;
			for(iter=p_out.first();iter.is_valid();++iter)
			{
				cue_parser_callback_retrieveinfo callback(iter->m_infos,iter->m_track_number);
				g_parse_cue(p_cuesheet,callback);
				callback.finalize();
			}
		}

		return true;
	} catch(exception_cue const & e)
	{
		console::formatter() << "cuesheet parsing error : " << e.what();
		p_out.remove_all();
		return false;
	}
}

namespace cue_parser
{


	static const char * extract_meta_test_field(const char * p_field,int p_index)
	{
		if (stricmp_utf8_partial(p_field,"CUE_TRACK") != 0) return 0;
		p_field += skip_utf8_chars(p_field,9);
		if ((int)p_field[0] - '0' != p_index / 10) return 0;
		if ((int)p_field[1] - '0' != p_index % 10) return 0;
		if (p_field[2] != '_') return 0;
		p_field += 3;
		if (!stricmp_utf8(p_field,"disc")) return "discnumber";
		else return p_field;
	}

	static bool extract_meta_is_reserved(const char * p_field)
	{
		return stricmp_utf8(p_field,"cuesheet") == 0;
	}

	static bool extract_meta_is_global(const char * p_field)
	{
		if (extract_meta_is_reserved(p_field)) return false;
		else return stricmp_utf8_partial(p_field,"CUE_TRACK") != 0;
	}

	static void extract_meta(const file_info & p_baseinfo,file_info & p_info,int p_index)
	{
		unsigned n, m = p_baseinfo.meta_get_count();

		for(n=0;n<m;n++)
		{
			if (extract_meta_is_global(p_baseinfo.meta_enum_name(n)))
				p_info.copy_meta_single(p_baseinfo,n);
		}
		if (p_index >= 1 && p_index <= 100)
		{
			for(n=0;n<m;n++)
			{
				const char * field = extract_meta_test_field(p_baseinfo.meta_enum_name(n),p_index);
				if (field) p_info.copy_meta_single_rename(p_baseinfo,n,field);
			}
		}
	}

	bool extract_info(const file_info & p_baseinfo,file_info & p_info, unsigned p_subsong_index)
	{
		TRACK_CALL_TEXT("cue_parser::extract_info");

		p_info.copy_info(p_baseinfo);
		p_info.set_replaygain(p_baseinfo.get_replaygain());

		unsigned cue_track = p_subsong_index;
		const char * cue = p_baseinfo.meta_get("cuesheet",0);
		if (!cue || (cue && !*cue)) return false;

		pfc::chain_list_t<cue_entry> cue_data;

		//if (!cue_parser::parse_old(cue,cue_data,&p_info,cue_track)) return false;
		if (!cue_parser::parse(cue,cue_data)) return false;
		if (!cue_parser::parse_info(cue,p_info,cue_track)) return false;

		double end = 0, start = 0;
		
		{
			bool found = false;

			pfc::chain_list_t<cue_entry>::const_iterator iter;

			for(iter = cue_data.first(); iter.is_valid(); ++iter)
			{
				if (iter->m_track_number == cue_track) {
					start = iter->m_indexes.start();
					found = true;

					++iter;
					if (iter.is_valid())
						end = iter->m_indexes.start();
					else
						end = p_baseinfo.get_length();

					if (end < start) return false;

					break;
				}
			}
			if (!found) return false;
		}

		p_info.meta_set("tracknumber", string_formatter() << cue_track);

		extract_meta(p_baseinfo,p_info,cue_track);

		p_info.set_length(end - start);
		p_info.info_set("cue_embedded","yes");

		return true;
	}

	void input_wrapper_cue_base::write_meta_create_field(string_base & p_out,const char * p_name,int p_index)
	{
		p_out.set_string("CUE_TRACK");
		p_out.add_char((p_index / 10) + '0');
		p_out.add_char((p_index % 10) + '0');
		p_out.add_char('_');
		p_out.add_string(p_name);
	}

	void input_wrapper_cue_base::write_meta(file_info & p_baseinfo,const file_info & p_trackinfo,unsigned p_subsong_index)
	{
		TRACK_CALL_TEXT("input_wrapper_cue_base::write_meta");

		string8_fastalloc temp;

		{
			unsigned n, m = p_baseinfo.meta_get_count();
			bit_array_bittable to_remove(m);
			for(n=0;n<m;n++)
			{
				to_remove.set(n, extract_meta_test_field(p_baseinfo.meta_enum_name(n),p_subsong_index) != 0);
			}
			p_baseinfo.meta_remove_mask(to_remove);
		}

		unsigned n, m = p_trackinfo.meta_get_count();

		for(n=0;n<m;n++)
		{
			write_meta_create_field(temp,p_trackinfo.meta_enum_name(n),p_subsong_index);
			p_baseinfo.copy_meta_single_rename(p_trackinfo,n,temp);
		}
	}

	void strip_cue_track_metadata(file_info & p_info)
	{
		unsigned n, m = p_info.meta_get_count();
		bit_array_bittable mask(m);
		for(n=0;n<m;n++)
		{
			mask.set(n,!extract_meta_is_global(p_info.meta_enum_name(n)));
		}
		p_info.meta_remove_mask(mask);
	}
}

