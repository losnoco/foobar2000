#ifndef _PFC_OTHER_H_
#define _PFC_OTHER_H_

template<class T>
class vartoggle_t
{
	T oldval;
	T & var;
public:
	vartoggle_t(T & p_var,T val) : var(p_var)
	{
		oldval = var;
		var = val;
	}
	~vartoggle_t() {var = oldval;}
};

typedef vartoggle_t<bool> booltoggle;

class order_helper
{
	mem_block_t<unsigned> data;
public:
	order_helper(unsigned size) : data(size)
	{
		unsigned n;
		for(n=0;n<size;n++) data[n]=n;
	}

	order_helper(const order_helper & p_order) {*this = p_order;}

	static void g_fill(unsigned * p_order,unsigned p_count);

	unsigned get_item(unsigned ptr) const {return data[ptr];}

	unsigned & operator[](unsigned ptr) {return data[ptr];}
	unsigned operator[](unsigned ptr) const {return data[ptr];}

	static void g_swap(unsigned * data,unsigned ptr1,unsigned ptr2);
	inline void swap(unsigned ptr1,unsigned ptr2) {g_swap(data,ptr1,ptr2);}

	const unsigned * get_ptr() const {return data;}

	static unsigned g_find_reverse(const unsigned * order,unsigned val);
	inline unsigned find_reverse(unsigned val) {return g_find_reverse(data,val);}

	static void g_reverse(unsigned * order,unsigned base,unsigned count);
	inline void reverse(unsigned base,unsigned count) {g_reverse(data,base,count);}

	unsigned get_count() const {return data.get_size();}
};

class fpu_control
{
	unsigned old_val;
	unsigned mask;
public:
	inline fpu_control(unsigned p_mask,unsigned p_val)
	{
		mask = p_mask;
		old_val = _controlfp(p_val,mask);
	}
	inline ~fpu_control()
	{
		_controlfp(old_val,mask);
	}
};

class fpu_control_roundnearest : private fpu_control
{
public:
	fpu_control_roundnearest() : fpu_control(_MCW_RC,_RC_NEAR) {}
};

class fpu_control_flushdenormal : private fpu_control
{
public:
	fpu_control_flushdenormal() : fpu_control(_MCW_DN,_DN_FLUSH) {}
};

class fpu_control_default : private fpu_control
{
public:
	fpu_control_default() : fpu_control(_MCW_DN|_MCW_RC,_DN_FLUSH|_RC_NEAR) {}
};


namespace pfc {
	template<typename T, void t_freefunc (T*) = pfc::delete_t<T>, T* t_clonefunc (T*) = pfc::clone_t<T> >
	class autoptr_t
	{
		T* m_ptr;
	public:
		inline autoptr_t(const autoptr_t<T> & p_param) : m_ptr(0) {copy(p_param);}
		inline autoptr_t(T* p_ptr) {m_ptr = p_ptr ? t_clonefunc(p_ptr) : 0;}
		inline autoptr_t() : m_ptr(0) {}
		inline ~autoptr_t() {release();}

		inline T* operator->() const {return m_ptr;}
		inline T* get_ptr() const {return m_ptr;}

		inline bool is_empty() const {return m_ptr == 0;}
		inline bool is_valid() const {return m_ptr != 0;}
	

		inline const autoptr_t<T,t_freefunc,t_clonefunc> & operator=(const autoptr_t & p_param) {copy(p_param);return *this;}
		inline const autoptr_t<T,t_freefunc,t_clonefunc> & operator=(T* p_ptr) {copy(p_ptr); return *this;}

		inline bool operator==(const autoptr_t & param) const {return m_ptr == param.m_ptr;}
		
		inline void copy(const autoptr_t & p_param)
		{
			release();
			if (p_param.m_ptr) m_ptr = t_clonefunc(p_param.m_ptr);
		}

		inline void set(T* p_ptr) {release(); m_ptr = p_ptr;}

		inline void release()
		{
			if (m_ptr) t_freefunc(m_ptr);
			m_ptr = 0;
		}
	};

	template<typename T,void t_freefunc(T*) = pfc::delete_t<T> >
	class ptrholder_t
	{
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(0) {}
		inline ~ptrholder_t() {if (m_ptr != 0) t_freefunc(m_ptr);}
		inline bool is_valid() const {return m_ptr != 0;}
		inline bool is_empty() const {return m_ptr == 0;}
		inline T* operator->() const {return m_ptr;}
		inline T* get_ptr() const {return m_ptr;}
		inline void release() {if (m_ptr) t_freefunc(m_ptr); m_ptr = 0;}
		inline void set(T * p_ptr) {if (m_ptr) t_freefunc(m_ptr); m_ptr = p_ptr;}
		inline const ptrholder_t<T,t_freefunc> & operator=(T * p_ptr) {set(p_ptr);return *this;}
		inline void detach() {m_ptr = 0;}
	private:
		ptrholder_t(const ptrholder_t<T,t_freefunc> &) {assert(0);}
		const ptrholder_t<T,t_freefunc> & operator=(const ptrholder_t<T,t_freefunc> & ) {assert(0); return *this;}

		T* m_ptr;
	};

	template<typename T,void t_destructor(T &) >
	class autodestructor_t : public T
	{
	public:
		autodestructor_t() {}
		autodestructor_t(const T & p_source) : T(p_source) {}
		autodestructor_t(const autodestructor_t<T,t_destructor> & p_source) : T(*(T*)&p_source) {}
		
		const autodestructor_t<T,t_destructor> & operator=(const autodestructor_t<T,t_destructor> & p_source) {*(T*)this = *(T*)&p_source; return *this;}
		const autodestructor_t<T,t_destructor> & operator=(const T p_source) {*(T*)this = p_source; return *this;}
		
		~autodestructor_t() {t_destructor(*this);}
	};

	class exception_text
	{
	public:
		inline exception_text(const char * p_message) : m_message(p_message) {}
		inline exception_text(const exception_text & p_exception) : m_message(p_exception.m_message) {}
		inline const exception_text & operator=(const char * p_message) {m_message = p_message;return *this;}
		inline const exception_text & operator=(const exception_text & p_exception) {m_message = p_exception.m_message;return *this;}
		
		inline const char * get_message() const {return m_message;}
	private:
		const char * m_message;
	};


	void crash();

}

#endif