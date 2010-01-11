#include "foobar2000.h"

t_io_result tag_processor_trailing::write_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return write(p_file,p_info,flag_id3v1,p_abort);
}

t_io_result tag_processor_trailing::write_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return write(p_file,p_info,flag_apev2,p_abort);
}

t_io_result tag_processor_trailing::write_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return write(p_file,p_info,flag_id3v1|flag_apev2,p_abort);
}


static t_io_result write_id3v2_internal(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	service_ptr_t<tag_processor_id3v2> ptr;
	if (tag_processor_id3v2::g_get(ptr))
		return ptr->write(p_file,p_info,p_abort);
	else
		return io_result_error_not_found;
}

bool tag_processor::is_id3v2_processor_available()
{
	return service_enum_get_count_t(tag_processor_id3v2) > 0;
}

enum {
	g_flag_id3v1 = 1<<0,
	g_flag_id3v2 = 1<<1,
	g_flag_apev2 = 1<<2
};

static void tagtype_list_append(string_base & p_out,const char * p_name)
{
	if (!p_out.is_empty()) p_out += "|";
	p_out += p_name;
}

static t_io_result g_write_tags(unsigned p_flags,const service_ptr_t<file> & p_file,const file_info * p_info,abort_callback & p_abort)
{
	t_io_result status;

	assert( p_flags == 0 || p_info != 0);

#if 0
	if (p_flags & g_flag_id3v1)
	{
		if (g_is_tag_empty_id3v1(*p_info)) p_flags &= ~g_flag_id3v1;
	}
	if (p_flags & g_flag_id3v2)
	{
		if (g_is_tag_empty_id3v2(*p_info)) p_flags &= ~g_flag_id3v2;
	}
	if (p_flags & g_flag_apev2)
	{
		if (g_is_tag_empty_apev2(*p_info)) p_flags &= ~g_flag_apev2;
	}
#endif

/*	if (p_info)
	{
		if (p_flags == 0) p_info->info_remove("tagtype");
		else
		{
			string8 tagtype;
			if (p_flags & g_flag_id3v2) tagtype_list_append(tagtype,"id3v2");
			if (p_flags & g_flag_apev2) tagtype_list_append(tagtype,"apev2");
			if (p_flags & g_flag_id3v1) tagtype_list_append(tagtype,"id3v1");
			p_info->info_set("tagtype",tagtype);
		}
	}*/
	

	if (p_flags & (g_flag_id3v1 | g_flag_apev2))
	{
		switch(p_flags & (g_flag_id3v1 | g_flag_apev2))
		{
		case g_flag_id3v1 | g_flag_apev2:
			status = static_api_ptr_t<tag_processor_trailing>()->write_apev2_id3v1(p_file,*p_info,p_abort);
			break;
		case g_flag_id3v1:
			status = static_api_ptr_t<tag_processor_trailing>()->write_id3v1(p_file,*p_info,p_abort);
			break;
		case g_flag_apev2:
			status = static_api_ptr_t<tag_processor_trailing>()->write_apev2(p_file,*p_info,p_abort);
			break;
		default:
			status = io_result_error_data;
			break;
		}
		if (io_result_failed(status)) return status;
	}
	else
	{
		status = static_api_ptr_t<tag_processor_trailing>()->remove(p_file,p_abort);
		if (io_result_failed(status)) return status;
	}

	if (p_flags & g_flag_id3v2)
	{
		status = write_id3v2_internal(p_file,*p_info,p_abort);
		if (io_result_failed(status)) return status;
	}
	else
	{
		t_uint64 dummy;
		status = tag_processor_id3v2::g_remove(p_file,dummy,p_abort);
		if (io_result_failed(status)) return status;
	}

	return io_result_success;	
}


t_io_result tag_processor::write_multi(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort,bool p_write_id3v1,bool p_write_id3v2,bool p_write_apev2)
{
	unsigned flags = 0;
	if (p_write_id3v1) flags |= g_flag_id3v1;
	if (p_write_id3v2) flags |= g_flag_id3v2;
	if (p_write_apev2) flags |= g_flag_apev2;
	return g_write_tags(flags,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_id3v1,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_apev2,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_apev2|g_flag_id3v1,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_id3v2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_id3v2,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_id3v2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_id3v2|g_flag_id3v1,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_id3v2_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_id3v2|g_flag_apev2,p_file,&p_info,p_abort);
}

t_io_result tag_processor::write_id3v2_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	return g_write_tags(g_flag_id3v2|g_flag_apev2|g_flag_id3v1,p_file,&p_info,p_abort);
}

t_io_result tag_processor::remove_trailing(const service_ptr_t<file> & p_file,abort_callback & p_abort)
{
	return static_api_ptr_t<tag_processor_trailing>()->remove(p_file,p_abort);
}

t_io_result tag_processor::remove_id3v2(const service_ptr_t<file> & p_file,abort_callback & p_abort)
{
	t_uint64 dummy;
	return tag_processor_id3v2::g_remove(p_file,dummy,p_abort);
}

t_io_result tag_processor::remove_id3v2_trailing(const service_ptr_t<file> & p_file,abort_callback & p_abort)
{
	t_io_result status;
	status = remove_id3v2(p_file,p_abort);
	if (io_result_failed(status)) return status;
	return remove_trailing(p_file,p_abort);
}

t_io_result tag_processor::read_trailing(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort)
{
	t_io_result status = static_api_ptr_t<tag_processor_trailing>()->read(p_file,p_info,p_abort);
	if (status == io_result_error_data) status = io_result_error_not_found;
	return status;
}

t_io_result tag_processor::read_id3v2(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort)
{
	service_ptr_t<tag_processor_id3v2> ptr;
	if (tag_processor_id3v2::g_get(ptr))
	{
		t_io_result status = ptr->read(p_file,p_info,p_abort);
		if (status == io_result_error_data) status = io_result_error_not_found;
		return status;
	}
	else
		return io_result_error_not_found;
}

t_io_result tag_processor::read_id3v2_trailing(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort)
{
	t_io_result status;
	file_info_i temp_infos[2];
	bool have_id3v2 = true, have_trailing = true;
	status = read_id3v2(p_file,temp_infos[0],p_abort);
	if (status == io_result_error_not_found) have_id3v2 = false;
	else if (io_result_failed(status)) return status;
	status = read_trailing(p_file,temp_infos[1],p_abort);
	if (status == io_result_error_not_found) have_trailing = false;
	else if (io_result_failed(status)) return status;
	
	if (!have_id3v2 && !have_trailing) return io_result_error_not_found;
	else 
	{
		ptr_list_t<const file_info> blargh;
		if (have_id3v2) blargh.add_item(&temp_infos[0]);
		if (have_trailing) blargh.add_item(&temp_infos[1]);
		p_info.merge(blargh);
		return io_result_success;
	}
}

t_io_result tag_processor::skip_d3v2(const service_ptr_t<file> & p_file,t_uint64 & p_size_skipped,abort_callback & p_abort)
{
	return tag_processor_id3v2::g_skip(p_file,p_size_skipped,p_abort);
}

bool tag_processor::is_id3v1_sufficient(const file_info & p_info)
{
	return static_api_ptr_t<tag_processor_trailing>()->is_id3v1_sufficient(p_info);
}

void tag_processor::truncate_to_id3v1(file_info & p_info)
{
	static_api_ptr_t<tag_processor_trailing>()->truncate_to_id3v1(p_info);
}