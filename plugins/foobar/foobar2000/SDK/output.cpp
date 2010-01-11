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

output_impl::output_impl() : m_is_open(false) {}

void output_impl::process_samples(const t_pcmspec & p_spec,const void * p_buffer,t_size p_bytes) {
	if (p_bytes == 0) return;
	m_incoming_buffer_ptr = 0;
	m_incoming_buffer.set_data_fromptr((const t_uint8*)p_buffer,p_bytes);
	m_incoming_pcmspec = p_spec;
}

void output_impl::update(bool & p_ready)
{
	on_update();
	
	if (m_incoming_buffer_ptr < m_incoming_buffer.get_size()) {
		bool wait = false;
		if (!m_is_open || m_pcmspec != m_incoming_pcmspec)
		{
			if (m_is_open)
			{
				force_play();
			}
		
			if (m_is_open && get_latency_bytes() > 0)
				wait = true;
			else {
				open(m_incoming_pcmspec);
				m_is_open = true;
				m_pcmspec = m_incoming_pcmspec;
			}
		}

		if (!wait)
		{
			t_size delta = pfc::min_t<t_size>(can_write_bytes(),m_incoming_buffer.get_size()-m_incoming_buffer_ptr);
			if (delta > 0)
			{
				write((const char*)m_incoming_buffer.get_ptr() + m_incoming_buffer_ptr,delta);
				m_incoming_buffer_ptr += delta;
			}
		}
	}
	
	p_ready = m_incoming_buffer_ptr >= m_incoming_buffer.get_size();
}

void output_impl::flush()
{
	m_incoming_buffer_ptr = m_incoming_buffer.get_size();
	on_flush();
}

double output_impl::get_latency()
{
	double ret = 0;
	if (m_incoming_pcmspec.is_valid()) ret += m_incoming_pcmspec.bytes_to_time(m_incoming_buffer.get_size() - m_incoming_buffer_ptr);
	if (m_pcmspec.is_valid()) ret += m_pcmspec.bytes_to_time(get_latency_bytes());
	return ret;
}