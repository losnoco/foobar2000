#include "foobar2000.h"


static t_uint32 read_dword_le_fromptr(const void * src) {return byte_order::dword_le_to_native(*(t_uint32*)src);}

cfg_var * cfg_var::list=0;


static int cfg_var_guid_compare(const cfg_var * p_var1,const cfg_var * p_var2)
{
	return pfc::guid_compare(p_var1->get_guid(),p_var2->get_guid());
}

static int cfg_var_guid_compare_search(const cfg_var * p_var1,const GUID & p_var2)
{
	return pfc::guid_compare(p_var1->get_guid(),p_var2);
}

t_io_result cfg_var::config_read_file(stream_reader * p_stream,abort_callback & p_abort)
{
#if 0
	t_uint32 size;
	GUID guid;

	list_t<cfg_var*> vars;

	{
		unsigned count = 0;
		for(cfg_var * ptr = list; ptr; ptr = ptr->next) count++;
		vars.set_count(count);

		count = 0;

		for(cfg_var * ptr = list; ptr; ptr = ptr->next)
			vars[count++] = ptr;

		vars.sort_t(cfg_var_guid_compare);
	}

	while(r.get_remaining() >= sizeof(guid) + sizeof(size))
	{
		if (!r.read_guid_le(guid)) break;
		if (!r.read_dword_le(size)) break;
		if (size > r.get_remaining()) break;

		unsigned index;

		if (vars.bsearch_t(cfg_var_guid_compare_search,guid,index))
		{
			vars[index]->set_raw_data(r.get_ptr(),size);
		}
		r.advance(size);
	}
#else

	t_io_result status;
	try {
		for(;;)
		{
			GUID guid;
			t_uint32 size;
			status = p_stream->read_lendian_t(guid,p_abort);
			if (io_result_failed(status))
			{
				if (status == io_result_error_data) break;
				throw status;
			}
			p_stream->read_lendian_e_t(size,p_abort);

			bool found = false;
			cfg_var * ptr;
			for(ptr = list; ptr; ptr=ptr->next)
			{
				if (ptr->get_guid() == guid)
				{
					stream_reader_limited_ref wrapper(p_stream,size);
					status = ptr->set_data_raw(&wrapper,p_abort);
					if (io_result_failed(status) && status != io_result_error_data) throw status;
					status = wrapper.flush_remaining(p_abort);
					if (io_result_failed(status)) throw status;
					found = true;
					break;
				}
			}
			if (!found)
				p_stream->skip_object_e(size,p_abort);
		}
	}
	catch(t_io_result code) {return code;}
	return io_result_success;

#endif
}

t_io_result cfg_var::config_write_file(stream_writer * p_stream,abort_callback & p_abort)
{
	cfg_var * ptr;
	mem_block_fast_aggressive temp;
	t_io_result status;
	for(ptr = list; ptr; ptr=ptr->next)
	{
		temp.set_size(0);
		status = ptr->get_data_raw(&stream_writer_buffer_append_ref_t<mem_block_fast_aggressive>(temp),p_abort);
		if (io_result_failed(status)) return status;
		status = p_stream->write_lendian_t(ptr->get_guid(),p_abort);
		if (io_result_failed(status)) return status;
		status = p_stream->write_lendian_t((t_uint32)temp.get_size(),p_abort);
		if (io_result_failed(status)) return status;
		if (temp.get_size() > 0)
		{
			status = p_stream->write_object(temp.get_ptr(),temp.get_size(),p_abort);
			if (io_result_failed(status)) return status;
		}
	}
	return io_result_success;
}



#if 0
t_io_result stream_writer_wrapper_write_config_callback::write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_done,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	m_out->write(p_buffer,p_bytes);
	p_bytes_done = p_bytes;
	return io_result_success;
}
#endif

t_io_result cfg_string::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	return p_stream->write_object(get_ptr(),length(),p_abort);
}

t_io_result cfg_string::set_data_raw(stream_reader * p_stream,abort_callback & p_abort)
{
	string8_fastalloc temp;
	t_io_result status;
	status = p_stream->read_string_raw(temp,p_abort);
	if (io_result_succeeded(status)) set_string(temp);
	return status;
}
