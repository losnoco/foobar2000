#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include "foobar2000.h"

class foobar2000_client
{
public:
	enum {FOOBAR2000_CLIENT_VERSION_COMPATIBLE = 60, FOOBAR2000_CLIENT_VERSION = 62};	//changes everytime global compatibility is broken
	virtual int get_version() {return FOOBAR2000_CLIENT_VERSION;}
	virtual service_factory_base * get_service_list() {return service_factory_base::list_get_pointer();}

	virtual void get_config(cfg_var::write_config_callback * out)
	{
		cfg_var::config_write_file(out);
	}

	virtual void set_config(const void * data,int size)
	{
		cfg_var::config_read_file(data,size);
	}

	virtual void set_library_path(const char * path,const char * name);

	virtual void services_init(bool val);

};

class NOVTABLE foobar2000_api
{
public:
	virtual service_class_ref service_enum_find_class(const GUID & p_guid) = 0;
	virtual bool service_enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,unsigned p_index) = 0;
	virtual unsigned service_enum_get_count(service_class_ref p_class) = 0;
	virtual HWND get_main_window()=0;
	virtual bool assert_main_thread()=0;
	virtual bool is_main_thread()=0;
	virtual bool is_shutting_down()=0;
	virtual const char * get_profile_path()=0;
	virtual bool is_initializing() = 0;
};

extern foobar2000_client g_client;
extern foobar2000_api * g_api;

#endif