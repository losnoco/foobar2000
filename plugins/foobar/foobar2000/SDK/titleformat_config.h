#ifndef _TITLEFORMAT_CONFIG_H_
#define _TITLEFORMAT_CONFIG_H_


class NOVTABLE titleformat_config : public service_base
{
public:
	virtual GUID get_guid() = 0;
	virtual const char * get_name() = 0;
	virtual void get_data(string_base & p_out) = 0;
	virtual void set_data(const char * p_string,unsigned p_string_length) = 0;
	virtual void reset() = 0;
	virtual bool compile(service_ptr_t<titleformat_object> & p_out) = 0;
	virtual double get_order_priority() = 0;



	static bool g_find(const GUID & p_guid,service_ptr_t<titleformat_config> & p_out);
	static bool g_get_data(const GUID & p_guid,string_base & p_out);
	static bool g_compile(const GUID & p_guid,service_ptr_t<titleformat_object> & p_out);


	static const GUID config_playlist,config_copy,config_statusbar,config_systray,config_windowtitle;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class titleformat_config_callback : public service_base
{
public:
	virtual void on_change(const GUID & p_guid,const char * p_name,const char * p_value,unsigned p_value_length) = 0;

	static void g_on_change(const GUID & p_guid,const char * p_name,const char * p_value,unsigned p_value_length);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class titleformat_config_impl : public titleformat_config, private cfg_var
{
public:
	GUID get_guid();
	const char * get_name();
	void get_data(string_base & p_out);
	void set_data(const char * p_string,unsigned p_string_length);
	void reset();
	bool compile(service_ptr_t<titleformat_object> & p_out);
	double get_order_priority();
	titleformat_config_impl(const GUID &,const char * p_name,const char * p_initvalue,double p_order);
private:

	virtual bool get_raw_data(write_config_callback * out);
	virtual void set_raw_data(const void * data,int size);

	string_simple m_name,m_value,m_value_default;
	service_ptr_t<titleformat_object> m_instance;
	bool m_compilation_failed;
	GUID m_guid;
	double m_order;
	critical_section m_sync;
};

typedef service_factory_single_transparent_p4_t<titleformat_config,titleformat_config_impl,const GUID&,const char*,const char*,double> titleformat_config_factory;

//usage:
//static titleformat_config_factory g_mytitleformatconfig("this will show up in titleformat config page","%blah%");

#endif //_TITLEFORMAT_CONFIG_H_