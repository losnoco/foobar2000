template<typename T>
class cfg_structlist_t : public cfg_var, public mem_block_list<T>
{
public:
	t_io_result get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
	{
		t_io_result status;
		t_uint32 n, m = get_count();
		status = p_stream->write_lendian_t(m,p_abort);
		if (io_result_failed(status)) return status;
		for(n=0;n<m;n++)
		{
			status = p_stream->write_object(&m_buffer[n],sizeof(T),p_abort);
			if (io_result_failed(status)) return status;
		}
		return io_result_success;
	}
	
	t_io_result set_data_raw(stream_reader * p_stream,abort_callback & p_abort)
	{
		t_io_result status;
		t_uint32 n,count;
		status = p_stream->read_lendian_t(count,p_abort);
		if (io_result_failed(status)) return status;
		if (m_buffer.set_size(count))
		{
			for(n=0;n<count;n++)
			{
				status = p_stream->read_object(&m_buffer[n],sizeof(T),p_abort);
				if (io_result_failed(status))
				{
					m_buffer.set_size(0);
					return status;
				}
			}
		}
		return status;
	}

public:
	cfg_structlist_t(const GUID & p_guid) : cfg_var(p_guid) {}
};