#ifndef _config_io_callback_h_
#define _config_io_callback_h_

class NOVTABLE config_io_callback : public service_base
{
public:
	virtual void on_read() = 0;
	virtual void on_write(bool reset) = 0;

	//for core use only
	static void g_on_read();
	static void g_on_write(bool reset);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

#endif //_config_io_callback_h_