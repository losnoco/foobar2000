#include "foobar2000.h"

t_io_result ogg_stream_handler::g_open(service_ptr_t<ogg_stream_handler> & p_out,service_ptr_t<file> p_reader,t_input_open_reason p_reason,abort_callback & p_abort)
{
	service_enum_t<ogg_stream_handler> e;
	service_ptr_t<ogg_stream_handler> ptr;
	bool need_reset = false;
	while(e.next(ptr))
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		t_io_result status;
		if (need_reset)
		{
			status = p_reader->reopen(p_abort);
			if (io_result_failed(status)) return status;
			need_reset = false;
		}

		status = ptr->open(p_reader,p_reason,p_abort);
		if (io_result_succeeded(status))
		{
			p_out = ptr;
			return io_result_success;
		}
		if (status != io_result_error_data) return status;

		need_reset = true;
	}
	return io_result_error_data;
}
