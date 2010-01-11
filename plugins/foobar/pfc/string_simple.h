template<typename T>
class string_simple_t//simplified string class, less efficient but smaller; could make it derive from string_base but it wouldn't be so light anymore (vtable)
{
private:
	T * ptr;

	void do_realloc(t_size size)
	{
		ptr = pfc::realloc_t(ptr,size);
	}

	static t_size t_strlen(const T* src,t_size max = ~0)
	{
		unsigned ret;
		for(ret=0;ret<max && src[ret];ret++);
		return ret;
	}

	static void t_strcpy(T* dst,const T* src,t_size max = ~0)
	{
		unsigned ptr;
		for(ptr=0;ptr<max && src[ptr];ptr++)
			dst[ptr]=src[ptr];
		dst[ptr]=0;
	}

	static T* t_strdup(const T* src,t_size max = ~0)
	{
		T* ret = pfc::malloc_t<T>(t_strlen(src,max)+1);
		t_strcpy(ret,src,max);
		return ret;
	}

public:
	inline const T * get_ptr() const {return ptr ? ptr : pfc::empty_string_t<T>();}
	inline operator const T * () const {return get_ptr();}

	inline string_simple_t(const string_simple_t<T> & p_source) {ptr = p_source.ptr ? t_strdup(p_source.ptr) : 0;}
	inline string_simple_t(const T * param,t_size len = ~0) {ptr = t_strdup(param,len);}
	inline string_simple_t() {ptr=0;}

	inline ~string_simple_t() {if (ptr) pfc::free_t(ptr);}

	inline const string_simple_t<T> & operator= (const T * param) {set_string(param);return *this;}
	inline const string_simple_t<T> & operator= (const string_simple_t<T> & param) {set_string(param);return *this;}
	inline const string_simple_t<T> & operator+= (const T * param) {add_string(param);return *this;}
	inline const string_simple_t<T> & operator+= (const string_simple_t<T> & param) {add_string(param);return *this;}

	inline void reset() {if (ptr) {pfc::free_t(ptr);ptr=0;}}

	inline bool is_empty() const {return !ptr || !*ptr;}

	inline t_size get_length(t_size p_max = ~0) const {return t_strlen(get_ptr(),p_max);}
	inline t_size length(t_size p_max = ~0) const {return get_length(p_max);}

	void add_string(const T * param,t_size len = ~0)
	{
		if (ptr)
		{
			t_size oldlen = length();
			if (param >= ptr && param <= ptr + oldlen)
			{
				add_string(string_simple_t<T>(param,len));
				return;
			}
		}
		len = t_strlen(param,len);
		if (len>0)
		{
			t_size old_len = length();
			do_realloc(old_len + len + 1);
			pfc::memcpy_t(ptr+old_len,param,len);
			ptr[old_len+len]=0;
		}
	}
	
	void set_string(const T * param,t_size len = ~0)
	{
		if (ptr)
		{
			t_size oldlen = length();
			if (param >= ptr && param <= ptr + oldlen)
			{
				set_string(string_simple_t<T>(param,len));
				return;
			}
		}
		len = t_strlen(param,len);
		if (len>0)
		{
			do_realloc(len + 1);//will malloc if old ptr was null
			pfc::memcpy_t(ptr,param,len);
			ptr[len]=0;
		}
		else reset();
	}


	void truncate(t_size len)
	{
		if (len<length(len))
		{
			do_realloc(len+1);
			ptr[len]=0;
		}
	}

	inline static void g_swap(string_simple_t<T> & p_item1,string_simple_t<T> & p_item2)
	{
		pfc::swap_t(p_item1.ptr,p_item2.ptr);
	}
};

typedef string_simple_t<char> string_simple;
typedef string_simple_t<WCHAR> w_string_simple;
typedef string_simple_t<TCHAR> t_string_simple;
