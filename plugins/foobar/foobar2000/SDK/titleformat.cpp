#include "foobar2000.h"

void titleformat::remove_color_marks(const char * src,string_base & out)//helper
{
	out.reset();
	while(*src)
	{
		if (*src==3)
		{
			src++;
			while(*src && *src!=3) src++;
			if (*src==3) src++;
		}
		else out.add_byte(*src++);
	}
}

static bool test_for_bad_char(const char * source,unsigned source_char_len,const char * reserved)
{
	return strstr_ex(reserved,(unsigned)(-1),source,source_char_len) != (unsigned)(-1);
}

void titleformat::remove_forbidden_chars(titleformat_text_out * p_out,const char * p_source,unsigned p_source_len,const char * p_reserved_chars)
{
	if (p_reserved_chars == 0 || *p_reserved_chars == 0)
	{
		p_out->write(p_source,p_source_len);
	}
	else
	{
		p_source_len = strlen_max(p_source,p_source_len);
		unsigned index = 0;
		unsigned good_byte_count = 0;
		while(index < p_source_len)
		{
			unsigned delta = utf8_char_len(p_source + index,p_source_len - index);
			if (delta == 0) break;
			if (test_for_bad_char(p_source+index,delta,p_reserved_chars))
			{
				if (good_byte_count > 0) {p_out->write(p_source+index-good_byte_count,good_byte_count);good_byte_count=0;}
				p_out->write("_",1);
			}
			else
			{
				good_byte_count += delta;
			}
			index += delta;
		}
		if (good_byte_count > 0) {p_out->write(p_source+index-good_byte_count,good_byte_count);good_byte_count=0;}
	}
}

void titleformat::remove_forbidden_chars_string_append(string_base & p_out,const char * p_source,unsigned p_source_len,const char * p_reserved_chars)
{
	remove_forbidden_chars(&titleformat_text_out_impl_string(p_out),p_source,p_source_len,p_reserved_chars);
}

void titleformat::remove_forbidden_chars_string(string_base & p_out,const char * p_source,unsigned p_source_len,const char * p_reserved_chars)
{
	p_out.reset();
	remove_forbidden_chars_string_append(p_out,p_source,p_source_len,p_reserved_chars);
}

void titleformat_hook_impl_file_info::process_codec(titleformat_text_out * p_out)
{
	string8 temp;
	const char * val = m_info->info_get("codec");
	if (val)
	{
		output_text(p_out,val,infinite);
	}
	else
	{
		val = m_info->info_get("referenced_file");
		if (val) uAddStringUpper(temp,string_extension(val));
		else uAddStringUpper(temp,string_extension(m_location.get_path()));
		output_text(p_out,temp,infinite);
	}
}

bool titleformat_hook_impl_file_info::process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag)
{
	p_found_flag = false;

	//todo make this bsearch someday
	if (!stricmp_utf8_ex(p_name,p_name_length,"filename",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		output_text(p_out,string_filename(temp),infinite);
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"path",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		output_text(p_out,temp.is_empty() ? "n/a" : temp.get_ptr(),infinite);
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"directory",infinite))
	{
		int count = 1;
		if (count > 0)
		{
			string8_fastalloc temp;
			filesystem::g_get_display_path(m_location.get_path(),temp);
			
			for(;count;count--)
			{
				int ptr = temp.scan_filename();
				if (ptr==0) {temp.reset();break;}
				ptr--;
				temp.truncate(ptr);
			}
			
			if (temp.is_empty())
			{
				p_found_flag = false;
			}
			else
			{
				p_out->write(temp + temp.scan_filename(),infinite);
				p_found_flag = true;
			}
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"subsong",infinite))
	{
		output_int(p_out,m_location.get_subsong());
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"channels",infinite))
	{
		unsigned val = (unsigned)m_info->info_get_int("channels");
		switch(val)
		{
		case 0: output_text(p_out,"N/A",infinite); break;
		case 1: output_text(p_out,"mono",infinite); p_found_flag = true; break;
		case 2: output_text(p_out,"stereo",infinite); p_found_flag = true; break;
		default: output_int(p_out,val); output_text(p_out,"ch",infinite); p_found_flag = true; break;
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"bitrate",infinite))
	{
		const char * value = m_info->info_get("bitrate_dynamic");
		if (value == 0 || *value == 0) value = m_info->info_get("bitrate");
		if (value == 0 || *value == 0) return false;
		output_text(p_out,value,infinite);
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"samplerate",infinite))
	{
		const char * value = m_info->info_get("samplerate");
		if (value == 0 || *value == 0) return false;
		output_text(p_out,value,infinite);
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"album",infinite))
	{
		if (process_meta(p_out,"album",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		}
		else if (process_meta(p_out,"venue",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"artist",infinite))
	{
		if (process_meta(p_out,"artist",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		} else if (process_meta(p_out,"album artist",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		} else if (process_meta(p_out,"composer",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		} else if (process_meta(p_out,"performer",infinite,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"album artist",infinite))
	{
		if (process_album_artist(p_out))
		{
			p_found_flag = true;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"track artist",infinite))
	{
		unsigned index_artist, index_album_artist;
		
		index_artist = m_info->meta_find("artist");
		if (index_artist == infinite) return false;
		index_album_artist = m_info->meta_find("album artist");
		if (index_album_artist == infinite) return false;
		if (m_info->are_meta_fields_identical(index_artist,index_album_artist)) return false;

		process_meta(p_out,"artist",infinite,", ",2,", ",2);

		p_found_flag = true;
		return true;

		
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"title",infinite))
	{
		if (process_meta(p_out,p_name,p_name_length,", ",2,", ",2))
		{
			p_found_flag = true;
			return true;
		}
		else
		{
			string8 temp;
			filesystem::g_get_display_path(m_location.get_path(),temp);
			string_filename fn(temp);
			if (fn.is_empty()) output_text(p_out,temp,infinite);
			else output_text(p_out,fn,infinite);
			p_found_flag = true;
			return true;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"codec",infinite))
	{
		process_codec(p_out);
		p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"track",infinite) || !stricmp_utf8_ex(p_name,p_name_length,"tracknumber",infinite))
	{
		const unsigned pad = 2;
		const char * val = m_info->meta_get_ex("tracknumber",infinite,0);
		if (val == 0) m_info->meta_get_ex("track",infinite,0);
		if (val != 0)
		{
			p_found_flag = true;
			unsigned val_len = strlen(val);
			if (val_len < pad)
			{
				unsigned n = pad - val_len;
				do {
					output_text(p_out,"0",1);
					n--;
				} while(n > 0);
			}
			output_text(p_out,val,infinite);
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"disc",infinite) || !stricmp_utf8_ex(p_name,p_name_length,"discnumber",infinite))
	{
		const unsigned pad = 1;
		const char * val = m_info->meta_get_ex("discnumber",infinite,0);
		if (val == 0) val = m_info->meta_get_ex("disc",infinite,0);
		if (val != 0)
		{
			p_found_flag = true;
			unsigned val_len = strlen(val);
			if (val_len < pad)
			{
				unsigned n = pad - val_len;
				do {
					output_text(p_out,"0",1);
					n--;
				} while(n > 0);
			}
			output_text(p_out,val,infinite);
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"length",infinite))
	{
		double len = m_info->get_length();
		if (len>0)
		{
			output_text(p_out,format_time((t_int64)len),infinite);
			p_found_flag = true;
			return true;
		}
		else return false;
	}
	else if (p_name_length > 2 && p_name[0] == '_' && p_name[1] == '_')
	{//info
		if (!stricmp_utf8_ex(p_name,p_name_length,"__replaygain_album_gain",infinite))
		{
			char rgtemp[replaygain_info::text_buffer_size];
			m_info->get_replaygain().format_album_gain(rgtemp);
			if (rgtemp[0] == 0) return false;
			output_text(p_out,rgtemp,infinite);
			p_found_flag = true;
			return true;
		}
		if (!stricmp_utf8_ex(p_name,p_name_length,"__replaygain_album_peak",infinite))
		{
			char rgtemp[replaygain_info::text_buffer_size];
			m_info->get_replaygain().format_album_peak(rgtemp);
			if (rgtemp[0] == 0) return false;
			output_text(p_out,rgtemp,infinite);
			p_found_flag = true;
			return true;
		}
		if (!stricmp_utf8_ex(p_name,p_name_length,"__replaygain_track_gain",infinite))
		{
			char rgtemp[replaygain_info::text_buffer_size];
			m_info->get_replaygain().format_track_gain(rgtemp);
			if (rgtemp[0] == 0) return false;
			output_text(p_out,rgtemp,infinite);
			p_found_flag = true;
			return true;
		}
		if (!stricmp_utf8_ex(p_name,p_name_length,"__replaygain_track_peak",infinite))
		{
			char rgtemp[replaygain_info::text_buffer_size];
			m_info->get_replaygain().format_track_peak(rgtemp);
			if (rgtemp[0] == 0) return false;
			output_text(p_out,rgtemp,infinite);
			p_found_flag = true;
			return true;
		}
		const char * value = m_info->info_get_ex(p_name+2,p_name_length-2);
		if (value == 0 || *value == 0) return false;
		output_text(p_out,value,infinite);
		p_found_flag = true;
		return true;
	}
	else if (p_name_length > 1 && p_name[0] == '_')
	{//special field
		bool found = process_extra(p_out,p_name+1,p_name_length-1);
		p_found_flag = found;
		return found;
	}
	else
	{//meta
		bool status = process_meta(p_out,p_name,p_name_length,", ",2,", ",2);
		p_found_flag = status;
		return status;
	}
}

bool titleformat_hook_impl_file_info::process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
	p_found_flag = false;
	if (!stricmp_utf8_ex(p_name,p_name_length,"meta",infinite))
	{
		switch(p_params->get_param_count())
		{
		case 1:
			{
				const char * name;
				unsigned name_length;
				p_params->get_param(0,name,name_length);
				bool status = process_meta(p_out,name,name_length,", ",2,", ",2);
				p_found_flag = status;
				return true;
			}
		case 2:
			{
				const char * name;
				unsigned name_length;
				p_params->get_param(0,name,name_length);
				unsigned index_val = p_params->get_param_uint(1);
				const char * value = m_info->meta_get_ex(name,name_length,index_val);
				if (value != 0)
				{
					p_found_flag = true;
					output_text(p_out,value,infinite);
				}
				return true;
			}
		default:
			return false;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"meta_sep",infinite))
	{
		switch(p_params->get_param_count())
		{
		case 2:
			{
				const char * name, * sep1;
				unsigned name_length, sep1_length;
				p_params->get_param(0,name,name_length);
				p_params->get_param(1,sep1,sep1_length);
				bool status = process_meta(p_out,name,name_length,sep1,sep1_length,sep1,sep1_length);
				p_found_flag = status;
				return true;
			}
		case 3:
			{
				const char * name, * sep1, * sep2;
				unsigned name_length, sep1_length, sep2_length;
				p_params->get_param(0,name,name_length);
				p_params->get_param(1,sep1,sep1_length);
				p_params->get_param(2,sep2,sep2_length);
				bool status = process_meta(p_out,name,name_length,sep1,sep1_length,sep2,sep2_length);
				p_found_flag = status;
				return true;
			}
		default:
			return false;
		}
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"meta_test",infinite))
	{
		unsigned n, count = p_params->get_param_count();
		if (count == 0) return false;
		bool found_all = true;
		for(n=0;n<count;n++)
		{
			const char * name;
			unsigned name_length;
			p_params->get_param(n,name,name_length);
			if (!m_info->meta_exists_ex(name,name_length))
			{
				found_all = false;
				break;
			}
		}
		if (found_all)
		{
			p_found_flag = true;
			output_int(p_out,1);
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"meta_num",infinite))
	{
		if (p_params->get_param_count() != 1) return false;
		const char * name;
		unsigned name_length;
		p_params->get_param(0,name,name_length);
		unsigned count = m_info->meta_get_count_by_name_ex(name,name_length);
		output_int(p_out,count);
		if (count > 0) p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"info",infinite))
	{
		if (p_params->get_param_count() != 1) return false;
		const char * name;
		unsigned name_length;
		p_params->get_param(0,name,name_length);
		const char * value = m_info->info_get_ex(name,name_length);
		if (value != 0)
		{
			p_found_flag = true;
			output_text(p_out,value,infinite);
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"extra",infinite))
	{
		if (p_params->get_param_count() != 1) return false;
		const char * name;
		unsigned name_length;
		p_params->get_param(0,name,name_length);
		if (process_extra(p_out,name,name_length)) p_found_flag = true;
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"codec",infinite))
	{
		if (p_params->get_param_count() != 0) return false;
		process_codec(p_out);
		p_found_flag = true;
		return true;		
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"channels",infinite))
	{
		if (p_params->get_param_count() != 0) return false;
		unsigned val = (unsigned)m_info->info_get_int("channels");
		switch(val)
		{
		case 0: output_text(p_out,"N/A",infinite); break;
		case 1: output_text(p_out,"mono",infinite); p_found_flag = true; break;
		case 2: output_text(p_out,"stereo",infinite); p_found_flag = true; break;
		default: output_int(p_out,val); output_text(p_out,"ch",infinite); p_found_flag = true; break;
		}
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"tracknumber",infinite))
	{
		unsigned pad = 2;
		unsigned param_count = p_params->get_param_count();
		if (param_count > 1) return false;
		if (param_count == 1) pad = (unsigned)p_params->get_param_uint(0);
		const char * val = m_info->meta_get_ex("tracknumber",infinite,0);
		if (val != 0)
		{
			p_found_flag = true;
			unsigned val_len = strlen(val);
			if (val_len < pad)
			{
				unsigned n = pad - val_len;
				do {
					output_text(p_out,"0",1);
					n--;
				} while(n > 0);
			}
			output_text(p_out,val,infinite);
		}
		return true;
	}
	else return false;
}

bool titleformat_hook_impl_file_info::process_album_artist(titleformat_text_out * p_out)
{
	if (process_meta(p_out,"album artist",infinite,", ",2,", ",2))
		return true;
	else if (process_meta(p_out,"artist",infinite,", ",2,", ",2))
		return true;
	else if (process_meta(p_out,"composer",infinite,", ",2,", ",2))
		return true;
	else if (process_meta(p_out,"performer",infinite,", ",2,", ",2))
		return true;
	else
		return false;
}

bool titleformat_hook_impl_file_info::process_meta(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,const char * p_sep1,unsigned p_sep1_length,const char * p_sep2,unsigned p_sep2_length)
{
	unsigned index = m_info->meta_find_ex(p_name,p_name_length);
	if (index == infinite) return false;

	unsigned n, m = m_info->meta_enum_value_count(index);
	for(n=0;n<m;n++)
	{
		if (n>0)
		{
			if (n+1 == m) output_text(p_out,p_sep2,p_sep2_length);
			else output_text(p_out,p_sep1,p_sep1_length);
		}			
		output_text(p_out,m_info->meta_enum_value(index,n),infinite);
	}
	return true;
}

bool titleformat_hook_impl_file_info::process_extra(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length)
{
	if (!stricmp_utf8_ex(p_name,p_name_length,"FILENAME",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		string_filename fn(temp);
		if (fn.is_empty()) output_text(p_out,temp,infinite);
		else output_text(p_out,fn,infinite);
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"FILENAME_EXT",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		string_filename_ext fn(temp);
		if (fn.is_empty()) output_text(p_out,temp,infinite);
		else output_text(p_out,fn,infinite);
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"DIRECTORYNAME",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		int offs = temp.scan_filename();
		if (offs>0)
		{
			temp.truncate(offs-1);
			offs = temp.scan_filename();
		}
		if (offs>0)
		{
			output_text(p_out,temp + offs,infinite);
		}
		else output_text(p_out,".",infinite);

		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"PATH",infinite))
	{
		string8 temp;
		filesystem::g_get_display_path(m_location.get_path(),temp);
		output_text(p_out,temp.is_empty() ? "n/a" : temp.get_ptr(),infinite);
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"PATH_RAW",infinite))
	{
		output_text(p_out,m_location.get_path(),infinite);
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"SUBSONG",infinite))
	{
		output_int(p_out,m_location.get_subsong());
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"FOOBAR2000_VERSION",infinite))
	{
		output_text(p_out,core_version_info::g_get_version_string(),infinite);
		return true;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"LENGTH",infinite))
	{
		double len = m_info->get_length();
		if (len>0)
		{
			output_text(p_out,format_time((t_int64)len),infinite);
			return true;
		}
		else return false;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"LENGTH_EX",infinite))
	{
		double len = m_info->get_length();
		if (len>0)
		{
			output_text(p_out,format_time_ex(len),infinite);
			return true;
		}
		else return false;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"LENGTH_SECONDS",infinite))
	{
		double len = m_info->get_length();
		if (len>0)
		{
			output_int(p_out,(t_uint64)len);
			return true;
		}
		else return false;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"LENGTH_SECONDS_FP",infinite))
	{
		double len = m_info->get_length();
		if (len>0)
		{
			string8 temp;
			temp.add_float(len,7);
			output_text(p_out,temp,infinite);
			return true;
		}
		else return false;
	}
	else if (!stricmp_utf8_ex(p_name,p_name_length,"LENGTH_SAMPLES",infinite))
	{
		t_int64 val = m_info->info_get_length_samples();
		if (val>0)
		{
			output_int(p_out,val);
			return true;
		}
		else return false;
	}
	else return false;
}

void titleformat_hook_impl_file_info::output_text(titleformat_text_out * p_out,const char * p_text,unsigned p_text_length)
{
	p_out->write(p_text,p_text_length);
}

void titleformat_hook_impl_file_info::output_int(titleformat_text_out * p_out,t_uint64 p_val)
{
	char temp[32];
	_i64toa(p_val,temp,10);
	output_text(p_out,temp,infinite);
}

bool titleformat_hook_impl_legacy_extrainfos::process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag)
{
	p_found_flag = false;
	if (m_extralist && p_name_length > 1 && p_name[0] == '_'  && p_name[1] != '_')
	{
		const char * name = p_name + 1;
		unsigned name_length = p_name_length - 1;
		const char * ptr = m_extralist;
		while(*ptr)
		{
			const char * ptr2 = strchr(ptr,'=');
			if (ptr2)
			{
				if (!stricmp_utf8_ex(name,name_length,ptr,ptr2-ptr))
				{
					p_out->write(ptr2+1,infinite);
					p_found_flag = true;
					return true;
				}
			}
			ptr += strlen(ptr) + 1;
		}
		return false;
	}
	else return false;
}

bool titleformat_hook_impl_legacy_extrainfos::process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
	return false;
}

void titleformat_object::run_filtered(titleformat_hook * p_source,string_base & p_out,titleformat_text_filter * p_filter)
{
	if (p_filter)
		run(&titleformat_hook_impl_text_filter(p_source,p_filter),p_out);
	else
		run(p_source,p_out);
}

void titleformat_object::run_hook_filtered(const playable_location & p_location,const file_info * p_source,titleformat_hook * p_hook,string_base & p_out,titleformat_text_filter * p_filter)
{
	if (p_hook)
	{
		run_filtered(
			&titleformat_hook_impl_splitter(
			&titleformat_hook_impl_file_info(p_location,p_source),
			p_hook),
			p_out,p_filter);
	}
	else
	{
		run_filtered(
			&titleformat_hook_impl_file_info(p_location,p_source),
			p_out,p_filter);
	}
}

void titleformat_object::run_simple(const playable_location & p_location,const file_info * p_source,string_base & p_out)
{
	run(&titleformat_hook_impl_file_info(p_location,p_source),p_out);
}

void titleformat_object::run_legacy(const playable_location & p_location,const file_info * p_source,string_base & p_out,const char * p_extra)
{
	run(
		&titleformat_hook_impl_splitter(
		&titleformat_hook_impl_file_info(p_location,p_source),
		&titleformat_hook_impl_legacy_extrainfos(p_extra)
		),
		p_out);
}

unsigned titleformat_hook_function_params::get_param_uint(unsigned index)
{
	const char * str;
	unsigned str_len;
	get_param(index,str,str_len);
	return atoui_ex(str,str_len);
}


void titleformat_text_out_impl_filter_chars::write(const char * p_data,unsigned p_data_length)
{
	titleformat::remove_forbidden_chars(m_chain,p_data,p_data_length,m_restricted_chars);
}

bool titleformat_hook_impl_splitter::process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag)
{
	p_found_flag = false;
	if (m_hook1 && m_hook1->process_field(p_out,p_name,p_name_length,p_found_flag)) return true;
	p_found_flag = false;
	if (m_hook2 && m_hook2->process_field(p_out,p_name,p_name_length,p_found_flag)) return true;
	p_found_flag = false;
	return false;
}

bool titleformat_hook_impl_splitter::process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
	p_found_flag = false;
	if (m_hook1 && m_hook1->process_function(p_out,p_name,p_name_length,p_params,p_found_flag)) return true;
	p_found_flag = false;
	if (m_hook2 && m_hook2->process_function(p_out,p_name,p_name_length,p_params,p_found_flag)) return true;
	p_found_flag = false;
	return false;
}

void titleformat_text_out::write_int_padded(t_int64 val,t_int64 maxval)
{
	const unsigned bufsize = 64;
	char temp[bufsize+1];
	unsigned len = 0;
	while(maxval) {maxval/=10;len++;}
	if (len == 0) len = 1;
	unsigned n;
	for(n=0;n<bufsize;n++) temp[n] = '0';
	temp[n] = 0;
	_i64toa(val,temp+bufsize/2,10);
	write(temp + strlen(temp) - len,infinite);
}

void titleformat_text_out::write_int(t_int64 val)
{
	char temp[32];
	_i64toa(val,temp,10);
	write(temp,32);
}

void titleformat_text_out_impl_filter::write(const char * p_data,unsigned p_data_length)
{
	m_filter->write(m_out,p_data,p_data_length);
}

void titleformat_text_filter_impl_reserved_chars::write(titleformat_text_out * p_out,const char * p_data,unsigned p_data_length)
{
	titleformat::remove_forbidden_chars(p_out,p_data,p_data_length,m_reserved_chars);
}


bool titleformat_hook_impl_text_filter::process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag)
{
	m_filter->on_new_field();
	return m_chain->process_field(
		&titleformat_text_out_impl_filter(m_filter,p_out),
		p_name,p_name_length,p_found_flag);
}

bool titleformat_hook_impl_text_filter::process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
	m_filter->on_new_field();
	return m_chain->process_function(
		&titleformat_text_out_impl_filter(m_filter,p_out),
		p_name,p_name_length,p_params,p_found_flag);
}


void titleformat_text_filter_impl_reserved_chars::on_new_field()
{
}

void titleformat::run(titleformat_hook * p_source,string_base & p_out,const char * p_spec)
{
	service_ptr_t<titleformat_object> ptr;
	if (!compile(ptr,p_spec)) p_out = "[COMPILATION ERROR]";
	else ptr->run(p_source,p_out);
}

void titleformat::compile_safe(service_ptr_t<titleformat_object> & p_out,const char * p_spec)
{
	if (!compile(p_out,p_spec))
	{
		compile(p_out,"%_filename%");
	}
}