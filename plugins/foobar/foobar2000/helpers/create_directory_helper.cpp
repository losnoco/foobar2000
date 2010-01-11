#include "stdafx.h"
#include "create_directory_helper.h"

namespace create_directory_helper
{
	void create_path(const char * p_path,abort_callback & p_abort)
	{
		mem_block_t<char> temp(strlen(p_path)+1);
		strcpy(temp,p_path);
		char * ptr = strstr(temp,":\\");
		if (ptr)
		{
			ptr+=2;
			while(*ptr)
			{
				if (*ptr == '\\')
				{
					*ptr = 0;

					filesystem::g_create_directory(temp,p_abort);
					*ptr = '\\';
				}
				ptr++;
			}
		}
	}

	static bool is_bad_dirchar(char c)
	{
		return c==' ' || c=='.';
	}

	void make_path(const char * parent,const char * filename,const char * extension,bool allow_new_dirs,string8 & out,bool really_create_dirs,abort_callback & p_abort)
	{
		out.reset();
		if (parent && *parent)
		{
			out = parent;
			out.fix_dir_separator('\\');
		}
		bool last_char_is_dir_sep = true;
		while(*filename)
		{
#ifdef WIN32
			if (allow_new_dirs && is_bad_dirchar(*filename))
			{
				const char * ptr = filename+1;
				while(is_bad_dirchar(*ptr)) ptr++;
				if (*ptr!='\\' && *ptr!='/') out.add_string(filename,ptr-filename);
				filename = ptr;
				if (*filename==0) break;
			}
#endif
			if (is_path_bad_char(*filename))
			{
				if (allow_new_dirs && (*filename=='\\' || *filename=='/'))
				{
					if (!last_char_is_dir_sep)
					{
						if (really_create_dirs) filesystem::g_create_directory(out,p_abort);
						out.add_char('\\');
						last_char_is_dir_sep = true;
					}
				}
				else
					out.add_char('_');
			}
			else
			{
				out.add_byte(*filename);
				last_char_is_dir_sep = false;
			}
			filename++;
		}
		if (out.length()>0 && out[out.length()-1]=='\\')
		{
			out.add_string("noname");
		}
		if (extension && *extension)
		{
			out.add_char('.');
			out.add_string(extension);
		}
	}
}

namespace {

	class titleformat_text_filter_impl_createdir : public titleformat_text_filter
	{
	public:
		void on_new_field() {}
		void write(titleformat_text_out * p_out,const char * p_data,unsigned p_data_length)
		{//not "UTF-8 aware" but coded not to clash with UTF-8, since only filtered chars are lower ASCII
			unsigned index = 0;
			unsigned good_bytes = 0;
			while(index < p_data_length && p_data[index] != 0)
			{
				unsigned char c = (unsigned char)p_data[index];
				if (c < ' ' || c == '\\' || c=='/' || c=='|' || c==':')
				{
					if (good_bytes > 0) {p_out->write(p_data+index-good_bytes,good_bytes);good_bytes=0;}
					p_out->write("_",1);
				}
				else good_bytes++;
				index++;
			}
			if (good_bytes > 0) {p_out->write(p_data+index-good_bytes,good_bytes);good_bytes=0;}
		}
	};
}

void create_directory_helper::format_filename(const metadb_handle_ptr & handle,titleformat_hook * p_hook,const char * spec,string8 & out)
{
	string8 temp;
	handle->format_title(p_hook,temp,spec,&titleformat_text_filter_impl_createdir());
	temp.replace_char('/','\\');
	temp.fix_filename_chars('_','\\');
	out = temp;
}
