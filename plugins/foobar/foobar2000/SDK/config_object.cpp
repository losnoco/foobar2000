#include "foobar2000.h"

void config_object_notify_manager::g_on_changed(const service_ptr_t<config_object> & p_object)
{
	if (core_api::assert_main_thread())
	{
		service_enum_t<config_object_notify_manager> e;
		service_ptr_t<config_object_notify_manager> ptr;
		while(e.next(ptr))
			ptr->on_changed(p_object);
	}
}

bool config_object::g_find(service_ptr_t<config_object> & p_out,const GUID & p_guid)
{
	service_ptr_t<config_object> ptr;
	service_enum_t<config_object> e;
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

t_io_result config_object::g_get_data_string(const GUID & p_guid,string_base & p_out)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->get_data_string(p_out);
}

t_io_result config_object::g_set_data_string(const GUID & p_guid,const char * p_data,unsigned p_length)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->set_data_string(p_data,p_length);
}

t_io_result config_object::get_data_int32(t_int32 & p_out)
{
	t_int32 temp;
	t_io_result status = get_data_struct_t<t_int32>(temp);
	if (io_result_failed(status)) return status;
	byte_order::order_le_to_native_t(temp);
	p_out = temp;
	return io_result_success;
}

t_io_result config_object::set_data_int32(t_int32 p_val)
{
	t_int32 temp = p_val;
	byte_order::order_native_to_le_t(temp);
	return set_data_struct_t<t_int32>(temp);
}

bool config_object::get_data_bool_simple(bool p_default)
{
	bool ret;
	t_io_result status = get_data_bool(ret);
	return io_result_failed(status) ? p_default : ret;
}

t_int32 config_object::get_data_int32_simple(t_int32 p_default)
{
	t_int32 ret;
	t_io_result status = get_data_int32(ret);
	return io_result_failed(status) ? p_default : ret;
}

t_io_result config_object::g_get_data_int32(const GUID & p_guid,t_int32 & p_out)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->get_data_int32(p_out);
}

t_io_result config_object::g_set_data_int32(const GUID & p_guid,t_int32 p_val)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->set_data_int32(p_val);
}

bool config_object::g_get_data_bool_simple(const GUID & p_guid,bool p_default)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return p_default;
	return ptr->get_data_bool_simple(p_default);
}

t_int32 config_object::g_get_data_int32_simple(const GUID & p_guid,t_int32 p_default)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return p_default;
	return ptr->get_data_int32_simple(p_default);
}

t_io_result config_object::get_data_bool(bool & p_out) {return get_data_struct_t<bool>(p_out);}
t_io_result config_object::set_data_bool(bool p_val) {return set_data_struct_t<bool>(p_val);}

t_io_result config_object::g_get_data_bool(const GUID & p_guid,bool & p_out) {return g_get_data_struct_t<bool>(p_guid,p_out);}
t_io_result config_object::g_set_data_bool(const GUID & p_guid,bool p_val) {return g_set_data_struct_t<bool>(p_guid,p_val);}


namespace {
	class config_object_read_callback_memblock : public config_object_read_callback
	{
	public:
		t_io_result get_data(void * p_buffer,abort_callback & p_abort)
		{
			memcpy(p_buffer,m_data,m_size);
			return io_result_success;
		}
		config_object_read_callback_memblock(const void * p_data,unsigned p_size) : m_data(p_data), m_size(p_size) {}
	private:
		const void * m_data;
		unsigned m_size;
	};

	class config_object_write_callback_string : public config_object_write_callback
	{
	public:
		t_io_result set_data(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
		{
			m_out.set_string((const char*)p_buffer,p_bytes);
			return io_result_success;
		}
		config_object_write_callback_string(string_base & p_out) : m_out(p_out) {}
	private:
		string_base & m_out;
	};

	class config_object_write_callback_raw : public config_object_write_callback
	{
	public:
		t_io_result set_data(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
		{
			unsigned todo = p_bytes > m_bytes ? m_bytes : p_bytes;
			memcpy(m_out,p_buffer,todo);
			m_bytes_read = todo;
			return io_result_success;
		}
		config_object_write_callback_raw(void * p_out,unsigned p_bytes,unsigned & p_bytes_read) : m_out(p_out), m_bytes(p_bytes), m_bytes_read(p_bytes_read) {}
	private:
		void * m_out;
		unsigned m_bytes;
		unsigned & m_bytes_read;
	};

	class config_object_write_callback_get_length : public config_object_write_callback
	{
	public:
		t_io_result set_data(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
		{
			m_length = p_bytes;
			return io_result_success;
		}
		config_object_write_callback_get_length(unsigned & p_length) : m_length(p_length) {}
	private:
		unsigned & m_length;
	};
};

t_io_result config_object::get_data_raw(void * p_out,unsigned p_bytes,unsigned & p_bytes_used)
{
	return get_data(&config_object_write_callback_raw(p_out,p_bytes,p_bytes_used),abort_callback_impl());
}

t_io_result config_object::get_data_raw_length(unsigned & p_length)
{
	return get_data(&config_object_write_callback_get_length(p_length),abort_callback_impl());
}

t_io_result config_object::set_data_raw(const void * p_data,unsigned p_bytes, bool p_notify)
{
	return set_data(&config_object_read_callback_memblock(p_data,p_bytes),p_bytes,abort_callback_impl(),p_notify);
}

t_io_result config_object::set_data_string(const char * p_data,unsigned p_length)
{
	return set_data_raw(p_data,strlen_max(p_data,p_length));
}

t_io_result config_object::get_data_string(string_base & p_out)
{
	return get_data(&config_object_write_callback_string(p_out),abort_callback_impl());
}


//config_object_impl stuff

GUID config_object_impl::get_guid() const
{
	return m_guid;
}

t_io_result config_object_impl::get_data(config_object_write_callback * p_callback,abort_callback & p_abort) const
{
	insync(m_sync);
	return p_callback->set_data(m_data.get_ptr(),m_data.get_size(),p_abort);
}

t_io_result config_object_impl::set_data(config_object_read_callback * p_callback,unsigned p_bytes,abort_callback & p_abort,bool p_notify)
{
	if (!core_api::assert_main_thread()) return io_result_error_generic;

	{
		insync(m_sync);
		if (p_bytes == 0)
			m_data.set_size(0);
		else
		{
			if (!m_data.set_size(p_bytes)) return io_result_error_generic;
			t_io_result status = p_callback->get_data(m_data.get_ptr(),p_abort);
			if (io_result_failed(status)) return status;
		}
	}

	if (p_notify) config_object_notify_manager::g_on_changed(this);

	return io_result_success;
}

config_object_impl::config_object_impl(const GUID & p_guid,const void * p_data,unsigned p_bytes) : m_guid(p_guid)
{
	m_data.set_data(p_data,p_bytes);
}
