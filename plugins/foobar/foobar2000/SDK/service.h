#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "../../pfc/pfc.h"

#include "utf8api.h"

#include "core_api.h"

typedef const void* service_class_ref;

long interlocked_increment(long * var);//note: return values arent reliable, they can be used only to determine if new value is <0, 0 or >0
long interlocked_decrement(long * var);

#ifdef _MSC_VER
#define FOOGUIDDECL __declspec(selectany)	//hack against msvc linker stupidity
#else
#define FOOGUIDDECL
#endif


#define DECLARE_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME = {A,S,D,{F,G,H,J,K,L,Z,X}};
#define DECLARE_CLASS_GUID(NAME,A,S,D,F,G,H,J,K,L,Z,X) FOOGUIDDECL const GUID NAME::class_guid = {A,S,D,{F,G,H,J,K,L,Z,X}};

#define service_ptr_t pfc::refcounted_ptr_t

template<class T>
class static_api_ptr_t
{
public:
	explicit inline static_api_ptr_t() {service_enum_create_t(m_ptr,0);}
	inline bool is_valid() const {return m_ptr.is_valid();}
	inline T* operator->() const {assert(is_valid());return m_ptr.get_ptr();}
	inline T* get_ptr() const {return m_ptr.get_ptr();}
private:
	void operator=(const static_api_ptr_t<T> & ) {assert(0);}
	service_ptr_t<T> m_ptr;
};


template<typename T, typename A = array_fast_t<service_ptr_t<T> > >
class service_list_t : public list_t<service_ptr_t<T>, A>
{
public:
	inline static void g_swap(service_list_t<T,A> & item1, service_list_t<T,A> & item2)
	{
		pfc::swap_t(
			*(list_t<service_ptr_t<T>, A>*)&item1,
			*(list_t<service_ptr_t<T>, A>*)&item2
			);
	}
};

template<typename T>
class service_list_fast_aggressive_t : public service_list_t<service_ptr_t<T>, array_fast_aggressive_t<T> >
{
public:
	inline static void g_swap(service_list_fast_aggressive_t<T> & item1, service_list_fast_aggressive_t<T> & item2)
	{
		pfc::swap_t(
			*(service_list_t<service_ptr_t<T>, array_fast_aggressive_t<T> >*)&item1,
			*(service_list_t<service_ptr_t<T>, array_fast_aggressive_t<T> >*)&item2
			);
	}
};

namespace pfc {
	template<typename S,typename A>
	inline void swap_t(service_list_t<S,A> & item1, service_list_t<S,A> & item2)
	{
		service_list_t<S,A>::g_swap(item1,item2);
	}

	template<typename S>
	inline void swap_t(service_list_fast_aggressive_t<S> & item1, service_list_fast_aggressive_t<S> & item2)
	{
		service_list_fast_aggressive_t<S>::g_swap(item1,item2);
	}

};


//! Base class for all service classes.
//! Provides interfaces for reference counter and querying for different interfaces supported by the object.
class NOVTABLE service_base
{
public:	
	//! Decrements reference count; deletes the object if reference count reaches zero. This is normally not called directly but managed by service_ptr_t<> template.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int service_release() = 0;
	//! Increments reference count. This is normally not called directly but managed by service_ptr_t<> template.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	virtual int service_add_ref() = 0;
	//! Queries whether the object supports specific interface and retrieves a pointer to that interface. This is normally not called directly but managed by service_query_t<> function template.
	//! Typical implementation checks the parameter against GUIDs of interfaces supported by this object, increments reference count and returns this pointer when requested interface is supported, otherwise returns null pointer.
	virtual service_base * service_query(const GUID & p_guid) {return 0;}

	//! Queries whether the object supports specific interface and retrieves a pointer to that interface.
	//! @param p_out Receives pointer to queried interface on success.
	//! returns true on success, false on failure (interface not supported by the object).
	template<class T>
	bool service_query_t(service_ptr_t<T> & p_out)
	{
		service_base * temp = service_query(T::class_guid);
		if (temp == 0) return false;
		p_out.set( static_cast<T*>(temp) );
		return true;
	}

	
	//! For compatibility with pfc::refcounted_ptr_t<>.
	inline void refcount_add_ref() {service_add_ref();}
	//! For compatibility with pfc::refcounted_ptr_t<>.
	inline void refcount_release() {service_release();}
};

#include "service_impl.h"

class NOVTABLE service_factory_base
{
private:
	static service_factory_base *list;
	service_factory_base * next;	
	const GUID & m_guid;
protected:
	inline service_factory_base(const GUID & p_guid) : m_guid(p_guid) {assert(!core_api::are_services_available());next=list;list=this;}
public:

	inline const GUID & get_class_guid() const {return m_guid;}

	inline static service_factory_base * list_get_pointer() {return list;}
	inline service_factory_base * list_get_next() {return next;}

	static service_class_ref enum_find_class(const GUID & p_guid);
	static bool enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,unsigned p_index);
	static unsigned enum_get_count(service_class_ref p_class);

	inline static bool is_service_present(const GUID & g) {return enum_get_count(enum_find_class(g))>0;}


#ifdef FOOBAR2000_EXE
	static void process_components_directory(const char * path,service_factory_base ** & blah);
	static void on_app_init(const char * exe_path);
	static void on_app_shutdown();
	static void config_reset(const char * name = 0);
	static void on_app_post_init();
	static void on_saveconfig(bool b_reset=false);
#endif

	virtual bool instance_create(service_ptr_t<service_base> & p_out) = 0;

};






template<class T>
static bool service_enum_create_t(service_ptr_t<T> & p_out,unsigned p_index)
{
	service_ptr_t<service_base> ptr;
	if (service_factory_base::enum_create(ptr,service_factory_base::enum_find_class(T::class_guid),p_index))
	{
		p_out = static_cast<T*>(ptr.get_ptr());
		return true;
	}
	else
	{
		p_out.release();
		return false;
	}
}

#define service_enum_get_count_t(T) (service_factory_base::enum_get_count(service_factory_base::enum_find_class(T::class_guid)))
#define service_enum_is_present(g) (service_factory_base::is_service_present(g))
#define service_enum_is_present_t(T) (service_factory_base::is_service_present(T::class_guid))

template<class T>
class service_class_helper_t
{
public:
	service_class_helper_t() : m_class(service_factory_base::enum_find_class(T::class_guid)) {}
	unsigned get_count() const
	{
		return service_factory_base::enum_get_count(m_class);
	}

	bool create(service_ptr_t<T> & p_out,unsigned p_index) const
	{
		service_ptr_t<service_base> temp;
		if (!service_factory_base::enum_create(temp,m_class,p_index)) return false;
		p_out.set(static_cast<T*>(temp.duplicate_ptr_release()));
		return true;
	}

private:
	service_class_ref m_class;
};

template<class B>
class service_enum_t
{
public:
	service_enum_t() : m_index(0) {}
	void reset() {m_index = 0;}
	bool first(service_ptr_t<B> & p_out)
	{
		reset();
		return next(p_out);
	}
	bool next(service_ptr_t<B> & p_out)
	{
		return m_helper.create(p_out,m_index++);
	}
private:
	unsigned m_index;
	service_class_helper_t<B> m_helper;
};


template<class B,class T>
class service_factory_t : public service_factory_base
{
public:
	service_factory_t() : service_factory_base(B::class_guid)
	{
	}

	~service_factory_t()
	{
	}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		service_impl_t<T> * item = new service_impl_t<T>;
		if (item == 0) return false;
		p_out = (service_base*)(B*)(T*)item;
		return true;
	}
};

template<class B,class T>
class service_factory_single_t : public service_factory_base
{
	service_impl_single_t<T> g_instance;
public:
	service_factory_single_t() : service_factory_base(B::class_guid) {}

	~service_factory_single_t() {}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)&g_instance;
		return true;
	}

	inline T& get_static_instance() const {return (T&)g_instance;}
};

template<class B,class T>
class service_factory_single_ref_t : public service_factory_base
{
private:
	T & instance;
public:
	service_factory_single_ref_t(T& param) : instance(param), service_factory_base(B::class_guid) {}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = dynamic_cast<service_base*>(&instance);
		return true;
	}

	inline T& get_static_instance() {return instance;}

	virtual void instance_release(service_base * ptr) {assert(0);}
};


template<class B,class T>
class service_factory_single_transparent_t : public service_factory_base, public service_impl_single_t<T>
{	
public:
	service_factory_single_transparent_t() : service_factory_base(B::class_guid) {}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)this;
		return true;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};

template<class B,class T,class P1>
class service_factory_single_transparent_p1_t : public service_factory_base, public service_impl_single_p1_t<T,P1>
{	
public:
	service_factory_single_transparent_p1_t(P1 p1) : service_factory_base(B::class_guid), service_impl_single_p1_t<T,P1>(p1) {}


	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)this;
		return true;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};

template<class B,class T,class P1,class P2>
class service_factory_single_transparent_p2_t : public service_factory_base, public service_impl_single_p2_t<T,P1,P2>
{	
public:
	service_factory_single_transparent_p2_t(P1 p1,P2 p2) : service_factory_base(B::class_guid), service_impl_single_p2_t<T,P1,P2>(p1,p2) {}


	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)this;
		return true;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};

template<class B,class T,class P1,class P2,class P3>
class service_factory_single_transparent_p3_t : public service_factory_base, public service_impl_single_p3_t<T,P1,P2,P3>
{	
public:
	service_factory_single_transparent_p3_t(P1 p1,P2 p2,P3 p3) : service_factory_base(B::class_guid), service_impl_single_p3_t<T,P1,P2,P3>(p1,p2,p3) {}


	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)this;
		return true;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};

template<class B,class T,class P1,class P2,class P3,class P4>
class service_factory_single_transparent_p4_t : public service_factory_base, public service_impl_single_p4_t<T,P1,P2,P3,P4>
{	
public:
	service_factory_single_transparent_p4_t(P1 p1,P2 p2,P3 p3,P4 p4) : service_factory_base(B::class_guid), service_impl_single_p4_t<T,P1,P2,P3,P4>(p1,p2,p3,p4) {}


	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(B*)(T*)this;
		return true;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};

#endif