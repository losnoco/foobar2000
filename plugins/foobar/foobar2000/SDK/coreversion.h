#ifndef _COREVERSION_H_
#define _COREVERSION_H_

class NOVTABLE core_version_info : public service_base
{
public:
	virtual const char * get_version_string()=0;

	static const char * g_get_version_string()
	{
		const char * ret = "";
		service_ptr_t<core_version_info> ptr;
		if (service_enum_create_t(ptr,0))
			ret = ptr->get_version_string();
		return ret;
	}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	core_version_info() {}
	~core_version_info() {}
};

#endif //_COREVERSION_H_
