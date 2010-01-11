#ifndef _CONFIG_OBJECT_IMPL_H_
#define _CONFIG_OBJECT_IMPL_H_

//template function bodies from config_object class

template<class T>
t_io_result config_object::get_data_struct_t(T& p_out)
{
	unsigned done;
	t_io_result status = get_data_raw(&p_out,sizeof(T),done);
	if (io_result_failed(status)) return status;
	if (done != sizeof(T)) return io_result_error_data;
	return io_result_success;
}

template<class T>
t_io_result config_object::set_data_struct_t(const T& p_in)
{
	return set_data_raw(&p_in,sizeof(T));
}

template<class T>
t_io_result config_object::g_get_data_struct_t(const GUID & p_guid,T & p_out)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->get_data_struct_t<T>(p_out);
}

template<class T>
t_io_result config_object::g_set_data_struct_t(const GUID & p_guid,const T & p_in)
{
	service_ptr_t<config_object> ptr;
	if (!g_find(ptr,p_guid)) return io_result_error_not_found;
	return ptr->set_data_struct_t<T>(p_in);
}


class config_object_impl : public config_object
{
public:
	GUID get_guid() const;
	t_io_result get_data(stream_writer * p_stream,abort_callback & p_abort) const ;
	t_io_result set_data(stream_reader * p_stream,abort_callback & p_abort,bool p_notify);

	config_object_impl(const GUID & p_guid,const void * p_data,unsigned p_bytes);

private:
	mutable critical_section m_sync;
	GUID m_guid;
	mem_block m_data;	
};

typedef service_factory_single_transparent_p3_t<config_object,config_object_impl,const GUID&,const void*,unsigned> config_object_factory;

template<unsigned t_size>
class config_object_fixed_impl_t : public config_object
{
public:
	GUID get_guid() const {return m_guid;}
	
	t_io_result get_data(stream_writer * p_stream,abort_callback & p_abort) const
	{
		insync(m_sync);
		return p_stream->write_object(m_data,t_size,p_abort);
	}

	t_io_result set_data(stream_reader * p_stream,abort_callback & p_abort,bool p_notify)
	{
		if (!core_api::assert_main_thread()) return io_result_error_generic;
		
		{
			t_uint8 temp[t_size];
			insync(m_sync);
			t_io_result status = p_stream->read_object(temp,t_size,p_abort);
			if (io_result_failed(status)) return status;
			memcpy(m_data,temp,t_size);
		}

		if (p_notify) config_object_notify_manager::g_on_changed(this);

		return io_result_success;
	}

	config_object_fixed_impl_t (const GUID & p_guid,const void * p_data)
		: m_guid(p_guid)
	{
		memcpy(m_data,p_data,t_size);
	}

private:
	mutable critical_section m_sync;
	GUID m_guid;
	t_uint8 m_data[t_size];
	
};


template<unsigned t_size>
class config_object_fixed_factory_t : public service_factory_single_transparent_p2_t<config_object,config_object_fixed_impl_t<t_size>,const GUID&,const void*>
{
public:
	config_object_fixed_factory_t(const GUID & p_guid,const void * p_initval)
		:
		service_factory_single_transparent_p2_t<config_object,config_object_fixed_impl_t<t_size>,const GUID&,const void*>
		(p_guid,p_initval)
	{}
};


class config_object_string_factory : public config_object_factory
{
public:
	config_object_string_factory(const GUID & p_guid,const char * p_string,unsigned p_string_length = infinite)
		: config_object_factory(p_guid,p_string,strlen_max(p_string,infinite)) {}

};

class config_object_bool_factory : public config_object_fixed_factory_t<1>
{
public:
	config_object_bool_factory(const GUID & p_guid,bool p_initval)
		: config_object_fixed_factory_t<1>(p_guid,&p_initval) {}
};

template<class T>
class config_object_int_factory_t : public config_object_fixed_factory_t<sizeof(T)>
{
private:
	template<class T>
	struct t_initval
	{
		T m_initval;
		t_initval(T p_initval) : m_initval(p_initval) {byte_order::order_native_to_le_t(m_initval);}
		T * get_ptr() {return &m_initval;}
	};
public:
	config_object_int_factory_t(const GUID & p_guid,T p_initval)
		: config_object_fixed_factory_t<sizeof(T)>(p_guid,t_initval<T>(p_initval).get_ptr() )
	{}
};

typedef config_object_int_factory_t<t_int32> config_object_int32_factory;



class config_object_notify_impl_simple : public config_object_notify
{
public:
	unsigned get_watched_object_count() {return 1;}
	GUID get_watched_object(unsigned p_index) {return m_guid;}
	void on_watched_object_changed(const service_ptr_t<config_object> & p_object) {m_func(p_object);}
	
	typedef void (*t_func)(const service_ptr_t<config_object> &);

	config_object_notify_impl_simple(const GUID & p_guid,t_func p_func) : m_guid(p_guid), m_func(p_func) {}
private:
	GUID m_guid;
	t_func m_func;	
};

typedef service_factory_single_transparent_p2_t<config_object_notify,config_object_notify_impl_simple,const GUID&,config_object_notify_impl_simple::t_func> config_object_notify_simple_factory;

#endif _CONFIG_OBJECT_IMPL_H_