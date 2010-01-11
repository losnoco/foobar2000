//only meant to be included from service.h

template<class T>
class service_impl_t : public T
{
public:
	int FB2KAPI service_release() {long ret = --m_counter; if (ret == 0) delete this; return ret;}
	int FB2KAPI service_add_ref() {return ++m_counter;}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(service_impl_t,T)
private:
	pfc::refcounter m_counter;
};

template<class T>
class service_impl_single_t : public T
{
public:
	int FB2KAPI service_release() {return 1;}
	int FB2KAPI service_add_ref() {return 1;}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(service_impl_single_t,T)
};


#undef __implement_service_base
