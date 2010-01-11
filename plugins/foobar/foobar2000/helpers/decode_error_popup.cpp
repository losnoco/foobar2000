#include "stdafx.h"

void decode_error_popup::show_error_decode(const char * p_label,metadb_handle_ptr p_item,double p_offset,t_io_result p_code)
{
	if (io_result_failed(p_code) && p_code != io_result_aborted)
	{
		console::info(uStringPrintf("Decode error at %s (%s)",(const char*)format_time_ex(p_offset),io_result_get_message(p_code)));
		console::info_location(p_item);
		popup_message::g_show_file_error(uStringPrintf("Decode error at %s",(const char*)format_time_ex(p_offset)),p_code,p_item->get_path(),p_label);
	}
}


void decode_error_popup::show_error_open(const char * p_label,metadb_handle_ptr p_item,t_io_result p_code)
{
	if (io_result_failed(p_code) && p_code != io_result_aborted)
	{
		console::info(uStringPrintf("Error opening file for decoding (%s)",io_result_get_message(p_code)));
		console::info_location(p_item);
		popup_message::g_show_file_error("Error opening file for decoding",p_code,p_item->get_path(),p_label);
	}
}
