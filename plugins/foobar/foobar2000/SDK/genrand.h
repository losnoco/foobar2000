#ifndef _SDK_GENRAND_H_
#define _SDK_GENRAND_H_

class NOVTABLE genrand_service : public service_base
{
public:
	virtual void seed(unsigned val) = 0;
	virtual unsigned genrand(unsigned range)=0;//returns random value N, where 0 <= N < range

	static bool g_create(service_ptr_t<genrand_service> & p_out) {return service_enum_t<genrand_service>().first(p_out);}


	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	genrand_service() {}
	~genrand_service() {}
};


#endif //_SDK_GENRAND_H_