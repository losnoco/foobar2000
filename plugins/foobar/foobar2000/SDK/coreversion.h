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
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
protected:
	inline core_version_info() {}
	inline ~core_version_info() {}
};

#endif //_COREVERSION_H_
