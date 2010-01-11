#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "../../pfc/pfc.h"

#include "core_api.h"

typedef const void* service_class_ref;

PFC_DECLARE_EXCEPTION(exception_service_not_found,pfc::exception,"Service not found");

#ifdef _WINDOWS

inline long interlocked_increment(long * var)
{
	PFC_ASSERT(!((t_size)var&3));
	return InterlockedIncrement(var);
}

inline long interlocked_decrement(long * var)
{
	PFC_ASSERT(!((t_size)var&3));
	return InterlockedDecrement(var);
}

#else

#error portme

#endif

#ifdef _MSC_VER
#define FOOGUIDDECL __declspec(selectany)	//hack against msvc linker stupidity - seems to be needed to make unreferenced GUIDs removed from binary
#else
#define FOOGUIDDECL
#endif


#define DECLARE_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME = {A,S,D,{F,G,H,J,K,L,Z,X}};
#define DECLARE_CLASS_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME::class_guid = {A,S,D,{F,G,H,J,K,L,Z,X}};

#define service_ptr_t pfc::refcounted_object_ptr_t

template<class T>
class static_api_ptr_t {
public:
	explicit static_api_ptr_t() {if (!service_enum_create_t(m_ptr,0)) throw pfc::exception_bug_check();}
	T* operator->() const {return m_ptr.get_ptr();}
	T* get_ptr() const {return m_ptr.get_ptr();}
private:
	service_ptr_t<T> m_ptr;
};

template<class T, class E>
class static_api_ptr_ex_t {
public:
	explicit static_api_ptr_ex_t() {
		service_ptr_t<T> temp;
		if (!service_enum_create_t(temp,0)) throw pfc::exception_bug_check();
		if (!temp->service_query_t(m_ptr)) throw pfc::exception_bug_check();
	}

	E* operator->() const {return m_ptr.get_ptr();}
	E* get_ptr() const {return m_ptr.get_ptr();}
private:
	service_ptr_t<E> m_ptr;
};

template<typename T, template<typename> class t_alloc = pfc::alloc_fast>
class service_list_t : public pfc::list_t<service_ptr_t<T>, t_alloc >
{
};

class service_base;
typedef service_base * pservice_base;

//! Base class for all service classes.
//! Provides interfaces for reference counter and querying for different interfaces supported by the object.
class NOVTABLE service_base
{
public:	
	//! Decrements reference count; deletes the object if reference count reaches zero. This is normally not called directly but managed by service_ptr_t<> template.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int FB2KAPI service_release() = 0;
	//! Increments reference count. This is normally not called directly but managed by service_ptr_t<> template.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int FB2KAPI service_add_ref() = 0;
	//! Queries whether the object supports specific interface and retrieves a pointer to that interface. This is normally not called directly but managed by service_query_t<> function template.
	//! Typical implementation checks the parameter against GUIDs of interfaces supported by this object, if the GUID is one of supported interfaces, p_out is set to service_base pointer that can be static_cast<>'ed to queried interface and the method returns true; otherwise the method returns false.
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {return false;}

	//! Queries whether the object supports specific interface and retrieves a pointer to that interface.
	//! @param p_out Receives pointer to queried interface on success.
	//! returns true on success, false on failure (interface not supported by the object).
	template<class T>
	bool FB2KAPI service_query_t(service_ptr_t<T> & p_out)
	{
		service_ptr_t<service_base> temp;
		if (!service_query(temp,T::class_guid)) return false;
		p_out.__unsafe_set(static_cast<T*>(temp.__unsafe_detach()));
		return true;
	}

	
	//! For compatibility with pfc::refcounted_ptr_t<>.
	inline void FB2KAPI refcount_add_ref() {service_add_ref();}
	//! For compatibility with pfc::refcounted_ptr_t<>.
	inline void FB2KAPI refcount_release() {service_release();}
protected:
	service_base() {}
	~service_base() {}
private:
	service_base(const service_base&) {throw pfc::exception_not_implemented();}
	const service_base & operator=(const service_base&) {throw pfc::exception_not_implemented();}
};

#include "service_impl.h"

class NOVTABLE service_factory_base {
protected:
	inline service_factory_base(const GUID & p_guid) : m_guid(p_guid) {assert(!core_api::are_services_available());__internal__next=__internal__list;__internal__list=this;}
public:
	inline const GUID & get_class_guid() const {return m_guid;}

	static service_class_ref FB2KAPI enum_find_class(const GUID & p_guid);
	static bool FB2KAPI enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,t_size p_index);
	static t_size FB2KAPI enum_get_count(service_class_ref p_class);

	inline static bool is_service_present(const GUID & g) {return enum_get_count(enum_find_class(g))>0;}

	//! Throws std::bad_alloc or another exception on failure.
	virtual void FB2KAPI instance_create(service_ptr_t<service_base> & p_out) = 0;

	//! FOR INTERNAL USE ONLY
	static service_factory_base *__internal__list;
	//! FOR INTERNAL USE ONLY
	service_factory_base * __internal__next;
private:
	const GUID & m_guid;
};


template<typename B>
class service_factory_base_t : public service_factory_base {
public:
	service_factory_base_t() : service_factory_base(B::class_guid) {}

};




template<class T> static bool FB2KAPI service_enum_create_t(service_ptr_t<T> & p_out,t_size p_index) {
	service_ptr_t<service_base> ptr;
	if (service_factory_base::enum_create(ptr,service_factory_base::enum_find_class(T::class_guid),p_index)) {
		p_out = static_cast<T*>(ptr.get_ptr());
		return true;
	} else {
		p_out.release();
		return false;
	}
}

template<typename T> static service_ptr_t<T> standard_api_create_t() {
	service_ptr_t<T> temp;
	if (!service_enum_create_t(temp,0)) throw exception_service_not_found();
	return temp;
}

#define service_enum_get_count_t(T) (service_factory_base::enum_get_count(service_factory_base::enum_find_class(T::class_guid)))
#define service_enum_is_present(g) (service_factory_base::is_service_present(g))
#define service_enum_is_present_t(T) (service_factory_base::is_service_present(T::class_guid))

template<typename T> class service_class_helper_t {
public:
	service_class_helper_t() : m_class(service_factory_base::enum_find_class(T::class_guid)) {}
	t_size get_count() const {
		return service_factory_base::enum_get_count(m_class);
	}

	bool create(service_ptr_t<T> & p_out,t_size p_index) const {
		service_ptr_t<service_base> temp;
		if (!service_factory_base::enum_create(temp,m_class,p_index)) return false;
		p_out.__unsafe_set(static_cast<T*>(temp.__unsafe_detach()));
		return true;
	}

	service_ptr_t<T> create(t_size p_index) {
		service_ptr_t<T> temp;
		if (!create(temp,p_index)) throw pfc::exception_bug_check();
		return temp;
	}
private:
	service_class_ref m_class;
};

//! Helper; simulates array with instance of each available implementation of given service class.
template<typename T> class service_instance_array_t {
public:
	typedef service_ptr_t<T> t_ptr;
	service_instance_array_t() {
		service_class_helper_t<T> helper;
		const t_size count = helper.get_count();
		m_data.set_size(count);
		for(t_size n=0;n<count;n++) m_data[n] = helper.create(n);
	}

	t_size get_size() const {return m_data.get_size();}
	const t_ptr & operator[](t_size p_index) const {return m_data[p_index];}

	//nonconst version to allow sorting/bsearching; do not abuse
	t_ptr & operator[](t_size p_index) {return m_data[p_index];}
private:
	pfc::array_t<t_ptr> m_data;
};

template<class B>
class service_enum_t
{
public:
	service_enum_t() : m_index(0) {}
	void FB2KAPI reset() {m_index = 0;}
	bool FB2KAPI first(service_ptr_t<B> & p_out)
	{
		reset();
		return next(p_out);
	}
	bool FB2KAPI next(service_ptr_t<B> & p_out)
	{
		return m_helper.create(p_out,m_index++);
	}
private:
	unsigned m_index;
	service_class_helper_t<B> m_helper;
};


template<class B,class T>
class service_factory_t : public service_factory_base_t<B>
{
public:
	void FB2KAPI instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::safe_cast<service_base*>(pfc::safe_cast<B*>(pfc::safe_cast<T*>(  new service_impl_t<T>  )));
	}
};

template<class B,class T>
class service_factory_single_t : public service_factory_base_t<B>
{
	service_impl_single_t<T> g_instance;
public:
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(service_factory_single_t,g_instance)

	void FB2KAPI instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::safe_cast<service_base*>(pfc::safe_cast<B*>(pfc::safe_cast<T*>(&g_instance)));
	}

	inline T& get_static_instance() {return g_instance;}
};

template<class B,class T>
class service_factory_single_ref_t : public service_factory_base_t<B>
{
private:
	T & instance;
public:
	service_factory_single_ref_t(T& param) : instance(param) {}

	virtual void FB2KAPI instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::safe_cast<service_base*>(&instance);
	}

	inline T& get_static_instance() {return instance;}

	virtual void instance_release(service_base * ptr) {assert(0);}
};


template<class B,class T>
class service_factory_single_transparent_t : public service_factory_base_t<B>, public service_impl_single_t<T>
{	
public:
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(service_factory_single_transparent_t,service_impl_single_t<T>)

	virtual void FB2KAPI instance_create(service_ptr_t<service_base> & p_out) {
		p_out = pfc::safe_cast<service_base*>(pfc::safe_cast<B*>(pfc::safe_cast<T*>(this)));
	}

	inline T& get_static_instance() {return *(T*)this;}
};

#endif