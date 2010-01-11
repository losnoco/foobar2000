#ifndef _PFC_OTHER_H_
#define _PFC_OTHER_H_

namespace pfc {
	template<class T>
	class vartoggle_t {
		T oldval; T & var;
	public:
		vartoggle_t(T & p_var,T val) : var(p_var) {
			oldval = var;
			var = val;
		}
		~vartoggle_t() {var = oldval;}
	};

	typedef vartoggle_t<bool> booltoggle;
};

class order_helper
{
	pfc::array_t<t_size> m_data;
public:
	order_helper(t_size p_size) {
		m_data.set_size(p_size);
		for(t_size n=0;n<p_size;n++) m_data[n]=n;
	}

	order_helper(const order_helper & p_order) {*this = p_order;}


	template<typename t_int>
	static void g_fill(t_int * p_order,const t_size p_count) {
		t_size n; for(n=0;n<p_count;n++) p_order[n] = (t_int)n;
	}

	template<typename t_array>
	static void g_fill(t_array & p_array) {
		t_size n; const t_size max = pfc::array_size_t(p_array);
		for(n=0;n<max;n++) p_array[n] = n;
	}


	t_size get_item(t_size ptr) const {return m_data[ptr];}

	t_size & operator[](t_size ptr) {return m_data[ptr];}
	t_size operator[](t_size ptr) const {return m_data[ptr];}

	static void g_swap(t_size * p_data,t_size ptr1,t_size ptr2);
	inline void swap(t_size ptr1,t_size ptr2) {pfc::swap_t(m_data[ptr1],m_data[ptr2]);}

	const t_size * get_ptr() const {return m_data.get_ptr();}

	static t_size g_find_reverse(const t_size * order,t_size val);
	inline t_size find_reverse(t_size val) {return g_find_reverse(m_data.get_ptr(),val);}

	static void g_reverse(t_size * order,t_size base,t_size count);
	inline void reverse(t_size base,t_size count) {g_reverse(m_data.get_ptr(),base,count);}

	t_size get_count() const {return m_data.get_size();}
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

#ifdef _M_IX86
class sse_control {
public:
	sse_control(unsigned p_mask,unsigned p_val) : m_mask(p_mask) {
		__control87_2(p_val,p_mask,NULL,&m_oldval);
	}
	~sse_control() {
		__control87_2(m_oldval,m_mask,NULL,&m_oldval);
	}
private:
	unsigned m_mask,m_oldval;
};
class sse_control_flushdenormal : private sse_control {
public:
	sse_control_flushdenormal() : sse_control(_MCW_DN,_DN_FLUSH) {}
};
#endif

namespace pfc {
	class refcounter {
	public:
		refcounter(long p_val = 0) : m_val(p_val) {}
		long operator++() {return InterlockedIncrement(&m_val);}
		long operator--() {return InterlockedDecrement(&m_val);}
	private:
		long m_val;
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
		inline T* detach() {T * temp = m_ptr; m_ptr = 0; return temp;}
		inline T& operator*() {return *m_ptr;}
		inline const T& operator*() const {return *m_ptr;}
	private:
		ptrholder_t(const ptrholder_t<T,t_freefunc> &) {assert(0);}
		const ptrholder_t<T,t_freefunc> & operator=(const ptrholder_t<T,t_freefunc> & ) {assert(0); return *this;}

		T* m_ptr;
	};

	//HACK: strip down methods where regular implementation breaks when used with void pointer
	template<void t_freefunc(void*)>
	class ptrholder_t<void,t_freefunc> {
	private:
		typedef void T;
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(0) {}
		inline ~ptrholder_t() {if (m_ptr != 0) t_freefunc(m_ptr);}
		inline bool is_valid() const {return m_ptr != 0;}
		inline bool is_empty() const {return m_ptr == 0;}
		inline T* get_ptr() const {return m_ptr;}
		inline void release() {if (m_ptr) t_freefunc(m_ptr); m_ptr = 0;}
		inline void set(T * p_ptr) {if (m_ptr) t_freefunc(m_ptr); m_ptr = p_ptr;}
		inline const ptrholder_t<T,t_freefunc> & operator=(T * p_ptr) {set(p_ptr);return *this;}
		inline T* detach() {T * temp = m_ptr; m_ptr = 0; return temp;}
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





	void crash();
}

#endif