class cfg_guidlist : public cfg_var, public mem_block_list<GUID>
{
public:
	t_io_result get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
	{
		try {
			t_uint32 n, m = get_count();
			p_stream->write_lendian_e_t(m,p_abort);
			for(n=0;n<m;n++) p_stream->write_lendian_e_t(get_item(n),p_abort);
		} catch(exception_io const & e) {return e.get_code();}
		return io_result_success;
	}
	t_io_result set_data_raw(stream_reader * p_stream,unsigned p_sizehint,abort_callback & p_abort)
	{
		t_io_result status;
		t_uint32 n,count;
		status = p_stream->read_lendian_t(count,p_abort);
		if (io_result_failed(status)) return status;
		if (m_buffer.set_size(count)) //else return io_result_error_out_of_memory?
		{
			for(n=0;n<count;n++)
			{
				status = p_stream->read_lendian_t(m_buffer[n],p_abort);
				if (io_result_failed(status)) {m_buffer.set_size(0); return status;}
			}
		}
		return io_result_success;
	}

	void sort()
	{
		sort_t(pfc::guid_compare);
	}


	bool have_item_bsearch(const GUID & p_item)
	{
		unsigned dummy;
		return bsearch_t(pfc::guid_compare,p_item,dummy);
	}


public:
	cfg_guidlist(const GUID & p_guid) : cfg_var(p_guid) {}
};
