#include "foobar2000.h"


//void on_change(const char * p_name,const char * p_value,unsigned p_value_length) ;

void titleformat_config_callback::g_on_change(const GUID & p_guid,const char * p_name,const char * p_value,unsigned p_value_length)
{
	service_ptr_t<titleformat_config_callback> ptr;
	service_enum_t<titleformat_config_callback> e;
	while(e.next(ptr))
		ptr->on_change(p_guid,p_name,p_value,p_value_length);
}

const char * titleformat_config_impl::get_name()
{
	return m_name;
}

void titleformat_config_impl::get_data(string_base & p_out)
{
	insync(m_sync);
	p_out = m_value;
}

void titleformat_config_impl::set_data(const char * p_string,unsigned p_string_length)
{
	if (core_api::assert_main_thread())
	{
		{
			insync(m_sync);
			m_value.set_string(p_string,p_string_length);
			m_compilation_failed = false;
			m_instance.release();
		}
		titleformat_config_callback::g_on_change(m_guid,m_name,p_string,p_string_length);
	}
}

bool titleformat_config_impl::compile(service_ptr_t<titleformat_object> & p_out)
{
	insync(m_sync);
	if (m_instance.is_empty())
	{
		if (m_compilation_failed) return false;
		if (!static_api_ptr_t<titleformat>()->compile(m_instance,m_value))
		{
			m_compilation_failed = true;
			return false;
		}
	}
	p_out = m_instance;
	return true;
}

void titleformat_config_impl::reset()
{
	set_data(m_value_default,infinite);
}

bool titleformat_config_impl::get_raw_data(write_config_callback * out)
{
	out->write(m_value.get_ptr(),m_value.length());
	return true;
}

void titleformat_config_impl::set_raw_data(const void * data,int size)
{
	m_value.set_string((const char*)data,(unsigned)size);
	m_instance.release();
	m_compilation_failed = false;
}

titleformat_config_impl::titleformat_config_impl(const GUID & p_guid,const char * p_name,const char * p_initvalue,double p_order) :
cfg_var(p_guid),  m_guid(p_guid), m_name(p_name), m_value(p_initvalue), m_value_default(p_initvalue), m_compilation_failed(false), m_order(p_order) {}


GUID titleformat_config_impl::get_guid() {return m_guid;}


bool titleformat_config::g_find(const GUID & p_guid,service_ptr_t<titleformat_config> & p_out)
{
	service_ptr_t<titleformat_config> ptr;
	service_enum_t<titleformat_config> e;
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

bool titleformat_config::g_compile(const GUID & p_guid,service_ptr_t<titleformat_object> & p_out)
{
	service_ptr_t<titleformat_config> ptr;
	if (!g_find(p_guid,ptr)) return false;
	return ptr->compile(p_out);
}

bool titleformat_config::g_get_data(const GUID & p_guid,string_base & p_out)
{
	service_ptr_t<titleformat_config> ptr;
	if (!g_find(p_guid,ptr)) return false;
	ptr->get_data(p_out);
	return true;
}

double titleformat_config_impl::get_order_priority() {return m_order;}