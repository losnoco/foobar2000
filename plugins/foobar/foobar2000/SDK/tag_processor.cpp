#include "foobar2000.h"

void tag_processor_trailing::write_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	write(p_file,p_info,flag_id3v1,p_abort);
}

void tag_processor_trailing::write_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	write(p_file,p_info,flag_apev2,p_abort);
}

void tag_processor_trailing::write_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
{
	write(p_file,p_info,flag_id3v1|flag_apev2,p_abort);
}




enum {
	g_flag_id3v1 = 1<<0,
	g_flag_id3v2 = 1<<1,
	g_flag_apev2 = 1<<2
};

static void tagtype_list_append(pfc::string_base & p_out,const char * p_name)
{
	if (!p_out.is_empty()) p_out += "|";
	p_out += p_name;
}

static void g_write_tags(unsigned p_flags,const service_ptr_t<file> & p_file,const file_info * p_info,abort_callback & p_abort) {
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
	

	if (p_flags & (g_flag_id3v1 | g_flag_apev2)) {
		switch(p_flags & (g_flag_id3v1 | g_flag_apev2)) {
		case g_flag_id3v1 | g_flag_apev2:
			static_api_ptr_t<tag_processor_trailing>()->write_apev2_id3v1(p_file,*p_info,p_abort);
			break;
		case g_flag_id3v1:
			static_api_ptr_t<tag_processor_trailing>()->write_id3v1(p_file,*p_info,p_abort);
			break;
		case g_flag_apev2:
			static_api_ptr_t<tag_processor_trailing>()->write_apev2(p_file,*p_info,p_abort);
			break;
		default:
			throw exception_io_data();
		}
	} else {
		static_api_ptr_t<tag_processor_trailing>()->remove(p_file,p_abort);
	}

	if (p_flags & g_flag_id3v2)
	{
		static_api_ptr_t<tag_processor_id3v2>()->write(p_file,*p_info,p_abort);
	}
	else
	{
		t_uint64 dummy;
		tag_processor_id3v2::g_remove(p_file,dummy,p_abort);
	}
}


void tag_processor::write_multi(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort,bool p_write_id3v1,bool p_write_id3v2,bool p_write_apev2) {
	unsigned flags = 0;
	if (p_write_id3v1) flags |= g_flag_id3v1;
	if (p_write_id3v2) flags |= g_flag_id3v2;
	if (p_write_apev2) flags |= g_flag_apev2;
	g_write_tags(flags,p_file,&p_info,p_abort);
}

void tag_processor::write_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_id3v1,p_file,&p_info,p_abort);
}

void tag_processor::write_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_apev2,p_file,&p_info,p_abort);
}

void tag_processor::write_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_apev2|g_flag_id3v1,p_file,&p_info,p_abort);
}

void tag_processor::write_id3v2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_id3v2,p_file,&p_info,p_abort);
}

void tag_processor::write_id3v2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_id3v2|g_flag_id3v1,p_file,&p_info,p_abort);
}

void tag_processor::write_id3v2_apev2(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_id3v2|g_flag_apev2,p_file,&p_info,p_abort);
}

void tag_processor::write_id3v2_apev2_id3v1(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort) {
	g_write_tags(g_flag_id3v2|g_flag_apev2|g_flag_id3v1,p_file,&p_info,p_abort);
}

void tag_processor::remove_trailing(const service_ptr_t<file> & p_file,abort_callback & p_abort) {
	return static_api_ptr_t<tag_processor_trailing>()->remove(p_file,p_abort);
}

void tag_processor::remove_id3v2(const service_ptr_t<file> & p_file,abort_callback & p_abort) {
	t_uint64 dummy;
	tag_processor_id3v2::g_remove(p_file,dummy,p_abort);
}

void tag_processor::remove_id3v2_trailing(const service_ptr_t<file> & p_file,abort_callback & p_abort) {
	remove_id3v2(p_file,p_abort);
	remove_trailing(p_file,p_abort);
}

void tag_processor::read_trailing(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort) {
	static_api_ptr_t<tag_processor_trailing>()->read(p_file,p_info,p_abort);
}

void tag_processor::read_trailing_ex(const service_ptr_t<file> & p_file,file_info & p_info,t_uint64 & p_tagoffset,abort_callback & p_abort) {
	static_api_ptr_t<tag_processor_trailing>()->read_ex(p_file,p_info,p_tagoffset,p_abort);
}

void tag_processor::read_id3v2(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort) {
	static_api_ptr_t<tag_processor_id3v2>()->read(p_file,p_info,p_abort);
}

void tag_processor::read_id3v2_trailing(const service_ptr_t<file> & p_file,file_info & p_info,abort_callback & p_abort)
{
	file_info_i temp_infos[2];
	bool have_id3v2 = true, have_trailing = true;
	try {
		read_id3v2(p_file,temp_infos[0],p_abort);
	} catch(exception_tag_not_found const &) {
		have_id3v2 = false;
	}
	try {
		read_trailing(p_file,temp_infos[1],p_abort);
	} catch(exception_tag_not_found const &) {
		have_trailing = false;
	}

	if (!have_id3v2 && !have_trailing) throw exception_tag_not_found();
	else  {
		ptr_list_t<const file_info> blargh;
		if (have_id3v2) blargh.add_item(&temp_infos[0]);
		if (have_trailing) blargh.add_item(&temp_infos[1]);
		p_info.merge(blargh);
	}
}

void tag_processor::skip_d3v2(const service_ptr_t<file> & p_file,t_uint64 & p_size_skipped,abort_callback & p_abort) {
	tag_processor_id3v2::g_skip(p_file,p_size_skipped,p_abort);
}

bool tag_processor::is_id3v1_sufficient(const file_info & p_info)
{
	return static_api_ptr_t<tag_processor_trailing>()->is_id3v1_sufficient(p_info);
}

void tag_processor::truncate_to_id3v1(file_info & p_info)
{
	static_api_ptr_t<tag_processor_trailing>()->truncate_to_id3v1(p_info);
}