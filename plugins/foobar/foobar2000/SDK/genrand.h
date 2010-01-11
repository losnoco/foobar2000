#ifndef _SDK_GENRAND_H_
#define _SDK_GENRAND_H_

class NOVTABLE genrand_service : public service_base
{
public:
	virtual void seed(unsigned val) = 0;
	virtual unsigned genrand(unsigned range)=0;//returns random value N, where 0 <= N < range

	static bool g_create(service_ptr_t<genrand_service> & p_out) {return service_enum_t<genrand_service>().first(p_out);}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};


#endif //_SDK_GENRAND_H_