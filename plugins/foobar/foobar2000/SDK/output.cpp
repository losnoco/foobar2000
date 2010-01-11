#include "foobar2000.h"

bool output_entry::g_find(service_ptr_t<output_entry> & p_out,const GUID & p_guid)
{
	service_enum_t<output_entry> e;
	service_ptr_t<output_entry> ptr;
	while(e.next(ptr))
	{
		if (ptr->get_guid() == p_guid)
		{
			p_out = ptr;
			return true;
		}
	}
	return false;
}

output_impl::output_impl() : m_is_open(false) {m_incoming_buffer.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);}

t_io_result output_impl::process_samples(const t_pcmspec & p_spec,const void * p_buffer,unsigned p_bytes)
{
	if (p_bytes == 0) return io_result_success;
	m_incoming_buffer_ptr = 0;
	if (!m_incoming_buffer.set_data(p_buffer,p_bytes))
	{
		m_incoming_buffer.set_size(0);
		return io_result_error_out_of_memory;
	}

	m_incoming_pcmspec = p_spec;

	return io_result_success;
}

t_io_result output_impl::update(bool & p_ready)
{
	t_io_result status;
	status = on_update();
	if (io_result_failed(status)) return status;
	
	if (m_incoming_buffer_ptr < m_incoming_buffer.get_size())
	{
		bool wait = false;
		if (!m_is_open || m_pcmspec != m_incoming_pcmspec)
		{
			if (m_is_open)
			{
				status = force_play();
				if (io_result_failed(status)) return status;
			}
		
			if (m_is_open && get_latency_bytes() > 0)
				wait = true;
			else
			{
				status = open(m_incoming_pcmspec);
				if (io_result_failed(status)) return status;
				m_is_open = true;
				m_pcmspec = m_incoming_pcmspec;
			}
		}

		if (!wait)
		{
			unsigned delta = pfc::min_t<unsigned>(can_write_bytes(),m_incoming_buffer.get_size()-m_incoming_buffer_ptr);
			if (delta > 0)
			{
				status = write((const char*)m_incoming_buffer.get_ptr() + m_incoming_buffer_ptr,delta);
				if (io_result_failed(status)) return status;
				m_incoming_buffer_ptr += delta;
			}
		}
	}
	
	p_ready = m_incoming_buffer_ptr >= m_incoming_buffer.get_size();
	return io_result_success;
}

t_io_result output_impl::flush()
{
	m_incoming_buffer_ptr = m_incoming_buffer.get_size();
	return on_flush();
}

double output_impl::get_latency()
{
	double ret = 0;
	if (m_incoming_pcmspec.is_valid()) ret += m_incoming_pcmspec.bytes_to_time(m_incoming_buffer.get_size() - m_incoming_buffer_ptr);
	if (m_pcmspec.is_valid()) ret += m_pcmspec.bytes_to_time(get_latency_bytes());
	return ret;
}