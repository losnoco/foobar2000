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
	
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	config_io_callback() {}
	~config_io_callback() {}
};

#endif //_config_io_callback_h_