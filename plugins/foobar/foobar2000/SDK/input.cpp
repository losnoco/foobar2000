#include "foobar2000.h"


bool input_entry::g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path)
{
	service_ptr_t<input_entry> ptr;
	service_enum_t<input_entry> e;
	string_extension ext(p_path);
	while(e.next(ptr))
	{
		if (ptr->is_our_path(p_path,ext))
		{
			p_out = ptr;
			return true;
		}
	}
	return false;
}

bool input_entry::g_find_service_by_content_type(service_ptr_t<input_entry> & p_out,const char * p_content_type)
{
	service_ptr_t<input_entry> ptr;
	service_enum_t<input_entry> e;
	while(e.next(ptr))
	{
		if (ptr->is_our_content_type(p_content_type))
		{
			p_out = ptr;
			return true;
		}
	}
	return false;
}



static t_io_result prepare_for_open(service_ptr_t<input_entry> & p_service,service_ptr_t<file> & p_file,const char * p_path,filesystem::t_open_mode p_open_mode,abort_callback & p_abort,bool p_from_redirect)
{
	if (p_file.is_empty())
	{
		service_ptr_t<filesystem> fs;
		if (filesystem::g_get_interface(fs,p_path))
		{
			if (fs->supports_content_types())
			{
				t_io_result status;
				status = fs->open(p_file,p_path,p_open_mode,p_abort);
				if (io_result_failed(status)) return status;
			}
		}
	}

	if (p_file.is_valid())
	{
		string8 content_type;
		if (p_file->get_content_type(content_type))
		{
			if (input_entry::g_find_service_by_content_type(p_service,content_type))
				return io_result_success;
		}
	}

	if (input_entry::g_find_service_by_path(p_service,p_path))
	{
		if (p_from_redirect && p_service->is_redirect()) return io_result_error_data;
		return io_result_success;
	}

	return io_result_error_data;//unsupported format
}


t_io_result input_entry::g_open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect)
{
	t_io_result status;
	service_ptr_t<file> filehint = p_filehint;
	service_ptr_t<input_entry> entry;

	status = prepare_for_open(entry,filehint,p_path,filesystem::open_mode_read,p_abort,p_from_redirect);
	if (io_result_failed(status)) return status;

	return entry->open_for_decoding(p_instance,filehint,p_path,p_abort);
}

t_io_result input_entry::g_open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect)
{
	t_io_result status;
	service_ptr_t<file> filehint = p_filehint;
	service_ptr_t<input_entry> entry;

	status = prepare_for_open(entry,filehint,p_path,filesystem::open_mode_read,p_abort,p_from_redirect);
	if (io_result_failed(status)) return status;

	return entry->open_for_info_read(p_instance,filehint,p_path,p_abort);
}

t_io_result input_entry::g_open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect)
{
	t_io_result status;
	service_ptr_t<file> filehint = p_filehint;
	service_ptr_t<input_entry> entry;

	status = prepare_for_open(entry,filehint,p_path,filesystem::open_mode_write_existing,p_abort,p_from_redirect);
	if (io_result_failed(status)) return status;

	return entry->open_for_info_write(p_instance,filehint,p_path,p_abort);
}

bool input_entry::g_is_supported_path(const char * p_path)
{
	service_ptr_t<input_entry> ptr;
	service_enum_t<input_entry> e;
	string_extension ext(p_path);
	while(e.next(ptr))
	{
		if (ptr->is_our_path(p_path,ext)) return true;
	}
	return false;
}



void input_open_file_helper_e(service_ptr_t<file> & p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort)
{
	t_io_result status;
	status = input_open_file_helper(p_file,p_path,p_reason,p_abort);
	exception_io::g_test(status);
}


t_io_result input_open_file_helper(service_ptr_t<file> & p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort)
{
	if (p_file.is_empty())
	{
		t_io_result status;
		switch(p_reason)
		{
		default:
			status = io_result_error_data;
			break;
		case input_open_info_read:
		case input_open_decode:
			status = filesystem::g_open(p_file,p_path,filesystem::open_mode_read,p_abort);
			break;
		case input_open_info_write:
			status = filesystem::g_open(p_file,p_path,filesystem::open_mode_write_existing,p_abort);
			break;
		}
		if (io_result_failed(status)) return status;
	}
	return io_result_success;
}