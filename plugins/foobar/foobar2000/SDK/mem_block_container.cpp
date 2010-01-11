#include "foobar2000.h"

t_io_result mem_block_container::from_stream(stream_reader * p_stream,unsigned p_bytes,abort_callback & p_abort)
{
	if (p_bytes == 0) {set_size(0); return io_result_success;}
	if (!set_size(p_bytes)) return io_result_error_out_of_memory;
	return p_stream->read_object(get_ptr(),p_bytes,p_abort);
}

bool mem_block_container::set(const void * p_buffer,unsigned p_size)
{
	if (!set_size(p_size)) return false;
	memcpy(get_ptr(),p_buffer,p_size);
	return true;
}
