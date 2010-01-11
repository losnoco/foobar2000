#include "foobar2000.h"

void popup_message::g_show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length)
{
	static_api_ptr_t<popup_message>()->show_ex(p_msg,p_msg_length,p_title,p_title_length);
}

void popup_message::g_show_file_error(const char * p_message,t_io_result p_status,const char * p_path,const char * p_title)
{
	string8_fastalloc message;
	message = p_message;
	if (io_result_failed(p_status))
	{
		message += " (";
		message += io_result_get_message(p_status);
		message += ")";
	}
	message += ":\n";
	message += file_path_display(p_path);
	g_show(message,p_title);
}


void popup_message::g_show_file_error_multi(const char * p_message,const list_base_const_t<const char *> & p_list,const char * p_title)
{
	string8_fastalloc message;
	message = p_message;
	message += "\n";
	unsigned n, m = p_list.get_count();
	for(n=0;n<m;n++)
	{
		message += "\n";
		message += p_list[n];
	}
	popup_message::g_show(message,p_title);
}