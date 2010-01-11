#ifndef _PFC_STRING_H_
#define _PFC_STRING_H_

bool is_path_separator(unsigned c);
bool is_path_bad_char(unsigned c);
bool is_valid_utf8(const char * param);
bool is_lower_ascii(const char * param);
bool has_path_bad_chars(const char * param);
void recover_invalid_utf8(const char * src,char * out,unsigned replace);//out must be enough to hold strlen(char) + 1, or appropiately bigger if replace needs multiple chars
void convert_to_lower_ascii(const char * src,unsigned max,char * out,char replace = '?');//out should be at least strlen(src)+1 long

class NOVTABLE string_base	//cross-dll safe string for returing strings from api functions etc
{//all char* are UTF-8 unless local comments state otherwise
protected:
	string_base() {}
	~string_base() {}
public:
	virtual const char * get_ptr() const = 0;
	virtual bool add_string(const char * p_string,unsigned p_length = infinite) = 0;
	virtual bool set_string(const char * p_string,unsigned p_length = infinite) {reset();return add_string(p_string,p_length);}
	virtual void truncate(unsigned len)=0;
	virtual unsigned get_length() const {return strlen(get_ptr());}
	virtual char * lock_buffer(unsigned p_requested_length) = 0;
	virtual void unlock_buffer() = 0;

	void add_string_e(const char * p_string,unsigned p_length = infinite);
	void set_string_e(const char * p_string,unsigned p_length = infinite);

	//! For compatibility with old conventions.
	inline unsigned length() const {return get_length();}
	
	inline void reset() {truncate(0);}
	
	inline void add_byte(char c) {add_string(&c,1);}
	inline bool is_empty() const {return *get_ptr()==0;}
	
	
	void skip_trailing_char(unsigned c = ' ');

	inline void add_chars(unsigned c,unsigned count) {for(;count;count--) add_char(c);}

	bool add_char(unsigned c);//adds unicode char to the string
	bool add_int(t_int64 val,unsigned base = 10);
	bool add_uint(t_uint64 val,unsigned base = 10);
	bool add_float(double val,unsigned digits);

	void add_char_e(unsigned c) {if (!add_char(c)) throw std::bad_alloc();}
	void add_int_e(t_int64 val,unsigned base = 10) {if (!add_int(val,base)) throw std::bad_alloc();}
	void add_uint_e(t_uint64 val,unsigned base = 10) {if (!add_uint(val,base)) throw std::bad_alloc();}
	void add_float_e(double val,unsigned digits) {if (!add_float(val,digits)) throw std::bad_alloc();}

	bool is_valid_utf8() {return ::is_valid_utf8(get_ptr());}

	void convert_to_lower_ascii(const char * src,char replace = '?');



	inline const string_base & operator= (const char * src) {set_string_e(src);return *this;}
	inline const string_base & operator+= (const char * src) {add_string_e(src);return *this;}
	inline const string_base & operator= (const string_base & src) {set_string_e(src);return *this;}
	inline const string_base & operator+= (const string_base & src) {add_string_e(src);return *this;}

	inline operator const char * () const {return get_ptr();}
};

class string8 : public string_base
{
protected:
	mem_block_t<char> data;
	unsigned used;

	inline bool makespace(unsigned s)
	{
		unsigned old_size = data.get_size();
		if (old_size < s)
			return data.set_size(s+16);
		else if (old_size > s + 32)
			return data.set_size(s);
		else
			return true;
	}

public:
	inline const string8 & operator= (const char * src) {set_string_e(src);return *this;}
	inline const string8 & operator+= (const char * src) {add_string_e(src);return *this;}
	inline const string8 & operator= (const string_base & src) {set_string_e(src);return *this;}
	inline const string8 & operator+= (const string_base & src) {add_string_e(src);return *this;}
	inline const string8 & operator= (const string8 & src) {set_string_e(src);return *this;}
	inline const string8 & operator+= (const string8 & src) {add_string_e(src);return *this;}

	inline operator const char * () const {return get_ptr();}

	inline string8() : used(0) {}
	inline string8(const char * p_string) : used(0) {set_string_e(p_string);}
	inline string8(const char * p_string,unsigned p_length) : used(0) {set_string_e(p_string,p_length);}
	inline string8(const string8 & p_string) : used(0) {set_string_e(p_string);}
	inline string8(const string_base & p_string) : used(0) {set_string_e(p_string);}

	inline void set_mem_logic(mem_block::t_mem_logic v) {data.set_mem_logic(v);}
	inline int get_mem_logic() const {return data.get_mem_logic();}
	void prealloc(unsigned s) {s++;if (s>used) makespace(s);}

	const char * get_ptr() const {
		return used > 0 ? data.get_ptr() : "";
	}

	bool add_string(const char * p_string,unsigned p_length = infinite);
	bool set_string(const char * p_string,unsigned p_length = infinite);

	void truncate(unsigned len)
	{
		if (used>len) {used=len;data[len]=0;makespace(used+1);}
	}

	unsigned get_length() const {return used;}


	void set_char(unsigned offset,char c);
	int find_first(char c,int start=0);	//return -1 if not found
	int find_last(char c,int start = -1);
	int find_first(const char * str,int start = 0);
	int find_last(const char * str,int start = -1);
	unsigned replace_nontext_chars(char p_replace = '_');
	unsigned replace_char(unsigned c1,unsigned c2,unsigned start = 0);
	unsigned replace_byte(char c1,char c2,unsigned start = 0);
	static unsigned g_scan_filename(const char * ptr);
	unsigned scan_filename();
	void fix_filename_chars(char def = '_',char leave=0);//replace "bad" characters, leave can be used to keep eg. path separators
	void fix_dir_separator(char c);
	void _xor(char x);//renamed from "xor" to keep weird compilers happy	
	void remove_chars(unsigned first,unsigned count); //slow
	void insert_chars(unsigned first,const char * src, unsigned count);//slow
	void insert_chars(unsigned first,const char * src);
	bool truncate_eol(unsigned start = 0);
	bool fix_eol(const char * append = " (...)",unsigned start = 0);
	bool limit_length(unsigned length_in_chars,const char * append = " (...)");

	//for string_buffer class
	char * lock_buffer(unsigned n)
	{
		makespace(n+1);
		data.zeromemory();
		return data;
	}

	void unlock_buffer()
	{
		used=strlen(data);
		makespace(used+1);
	}

	void force_reset() {used=0;data.force_reset();}

	inline static void g_swap(string8 & p_item1,string8 & p_item2)
	{
		pfc::swap_t(p_item1.data,p_item2.data);
		pfc::swap_t(p_item1.used,p_item2.used);
	}
};

class string8_fastalloc : public string8
{
public:
	explicit string8_fastalloc(unsigned p_prealloc = 0) {set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);if (p_prealloc) prealloc(p_prealloc);}
	inline const char * operator=(const char * src) {set_string(src);return get_ptr();}
	inline const char * operator+=(const char * src) {add_string(src);return get_ptr();}
};

class string_buffer
{
private:
	string_base & m_owner;
	char * m_buffer;
public:
	explicit string_buffer(string_base & p_string,unsigned p_requeted_length) : m_owner(p_string) {m_buffer = m_owner.lock_buffer(p_requeted_length);}
	~string_buffer() {m_owner.unlock_buffer();}
	operator char* () {return m_buffer;}
};

class string_printf : public string8_fastalloc
{
public:
	static void g_run(string_base & out,const char * fmt,va_list list);
	inline void run(const char * fmt,va_list list) {g_run(*this,fmt,list);}

	explicit string_printf(const char * fmt,...);
};

class string_printf_va : public string8_fastalloc
{
public:
	explicit string_printf_va(const char * fmt,va_list list) {string_printf::g_run(*this,fmt,list);}
};


class format_time
{
protected:
	char buffer[128];
public:
	format_time(t_int64 s);
	operator const char * () const {return buffer;}
};


class format_time_ex : private format_time
{
public:
	format_time_ex(double s,unsigned extra = 3) : format_time((t_int64)s)
	{
		if (extra>0)
		{
			unsigned mul = 1, n;
			for(n=0;n<extra;n++) mul *= 10;

			
			unsigned val = (unsigned)((t_int64)(s*mul) % mul);
			char fmt[16];
			sprintf(fmt,".%%0%uu",extra);
			sprintf(buffer + strlen(buffer),fmt,val);
		}
	}
	const char * get_ptr() const {return buffer;}
	operator const char * () const {return buffer;}
};

unsigned atoui_ex(const char * ptr,unsigned max);
t_int64 atoi64_ex(const char * ptr,unsigned max);

unsigned strlen_max(const char * ptr,unsigned max);
unsigned wcslen_max(const WCHAR * ptr,unsigned max);

unsigned strlen_utf8(const char * s,unsigned num=-1);//returns number of characters in utf8 string; num - no. of bytes (optional)
unsigned utf8_char_len(const char * s,unsigned max=-1);//returns size of utf8 character pointed by s, in bytes, 0 on error
unsigned utf8_char_len_from_header(char c);
unsigned utf8_chars_to_bytes(const char * string,unsigned count);

unsigned strcpy_utf8_truncate(const char * src,char * out,unsigned maxbytes);

unsigned utf8_decode_char(const char * src,unsigned * out,unsigned src_bytes = infinite);//returns length in bytes
unsigned utf8_encode_char(unsigned c,char * out);//returns used length in bytes, max 6
unsigned utf16_decode_char(const wchar_t * p_source,unsigned * p_out,unsigned p_source_length = infinite);
unsigned utf16_encode_char(unsigned c,WCHAR * out);


unsigned strstr_ex(const char * p_string,unsigned p_string_len,const char * p_substring,unsigned p_substring_len);

int strcmp_partial(const char * s1,const char * s2);
unsigned skip_utf8_chars(const char * ptr,unsigned count);
char * strdup_n(const char * src,unsigned len);
int stricmp_ascii(const char * s1,const char * s2);

int strcmp_ex(const char* p1,unsigned n1,const char* p2,unsigned n2);

unsigned utf8_get_char(const char * src);

inline bool utf8_advance(const char * & var)
{
	UINT delta = utf8_char_len(var);
	var += delta;
	return delta>0;
}

inline bool utf8_advance(char * & var)
{
	UINT delta = utf8_char_len(var);
	var += delta;
	return delta>0;
}

inline const char * utf8_char_next(const char * src) {return src + utf8_char_len(src);}
inline char * utf8_char_next(char * src) {return src + utf8_char_len(src);}


template<class T>
class string_simple_t//simplified string class, less efficient but smaller; could make it derive from string_base but it wouldn't be so light anymore (vtable)
{
private:
	T * ptr;

	void do_realloc(unsigned size)
	{
		ptr = mem_ops<T>::realloc(ptr,size);
	}

	static unsigned t_strlen(const T* src,unsigned max = -1)
	{
		unsigned ret;
		for(ret=0;ret<max && src[ret];ret++);
		return ret;
	}

	static void t_strcpy(T* dst,const T* src,unsigned max=-1)
	{
		unsigned ptr;
		for(ptr=0;ptr<max && src[ptr];ptr++)
			dst[ptr]=src[ptr];
		dst[ptr]=0;
	}

	static T* t_strdup(const T* src,unsigned max=-1)
	{
		T* ret = mem_ops<T>::alloc(t_strlen(src,max)+1);
		if (ret) t_strcpy(ret,src,max);
		return ret;
	}

	inline static void t_memcpy(T* dst,const T* src,unsigned len) {mem_ops<T>::copy(dst,src,len);}

public:
	inline const T * get_ptr() const {return ptr ? ptr : reinterpret_cast<const T*>("\0\0\0\0");}
	inline operator const T * () const {return get_ptr();}

	inline string_simple_t(const string_simple_t<T> & p_source) {ptr = p_source.ptr ? t_strdup(p_source.ptr) : 0;}
	inline string_simple_t(const T * param,unsigned len = infinite) {ptr = t_strdup(param,len);}
	inline string_simple_t() {ptr=0;}

	inline ~string_simple_t() {if (ptr) mem_ops<T>::free(ptr);}

	inline const string_simple_t<T> & operator= (const T * param) {set_string(param);return *this;}
	inline const string_simple_t<T> & operator= (const string_simple_t<T> & param) {set_string(param);return *this;}
	inline const string_simple_t<T> & operator+= (const T * param) {add_string(param);return *this;}
	inline const string_simple_t<T> & operator+= (const string_simple_t<T> & param) {add_string(param);return *this;}

	inline void reset() {if (ptr) {mem_ops<T>::free(ptr);ptr=0;}}

	inline bool is_empty() const {return !ptr || !*ptr;}

	inline unsigned get_length() const {return t_strlen(get_ptr());}
	inline unsigned length() const {return get_length();}

	void add_string(const T * param,unsigned len = -1)
	{
		if (ptr)
		{
			unsigned oldlen = length();
			if (param >= ptr && param <= ptr + oldlen)
			{
				add_string(string_simple_t<T>(param,len));
				return;
			}
		}
		len = t_strlen(param,len);
		if (len>0)
		{
			unsigned old_len = length();
			do_realloc(old_len + len + 1);
			t_memcpy(ptr+old_len,param,len);
			ptr[old_len+len]=0;
		}
	}
	
	void set_string(const T * param,unsigned len = -1)
	{
		if (ptr)
		{
			unsigned oldlen = length();
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
			t_memcpy(ptr,param,len);
			ptr[len]=0;
		}
		else reset();
	}


	void truncate(unsigned len)
	{
		if (len<length())
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

#define string_simple string_simple_t<char>
#define w_string_simple string_simple_t<WCHAR>
#define t_string_simple string_simple_t<TCHAR>

class string_filename : public string_simple
{
public:
	explicit string_filename(const char * fn);
};

class string_filename_ext : public string_simple
{
public:
	explicit string_filename_ext(const char * fn);
};

class string_extension
{
	char buffer[32];
public:
	inline const char * get_ptr() const {return buffer;}
	inline unsigned length() const {return strlen(buffer);}
	inline operator const char * () const {return buffer;}
	explicit string_extension(const char * src);
};


class string_replace_extension
{
public:
	string_replace_extension(const char * p_path,const char * p_ext);
	inline operator const char*() const {return m_data;}
private:
	string8 m_data;
};

class string_directory
{
public:
	string_directory(const char * p_path);
	inline operator const char*() const {return m_data;}
private:
	string8 m_data;
};

#define string_extension_8 string_extension

template<class T>
class string_buffer_t
{
private:
	string_simple_t<T> & owner;
	mem_block_t<T> data;
	unsigned size;
public:
	string_buffer_t(string_simple_t<T> & p_owner,unsigned p_size) : owner(p_owner), data(p_size+1), size(size) {data.zeromemory();}
	operator T* () {return data.get_ptr();}
	~string_buffer_t() {owner.set_string(data,size);}
};

#define a_string_buffer string_buffer_t<char>
#define t_string_buffer string_buffer_t<TCHAR>
#define w_string_buffer string_buffer_t<WCHAR>

class make_string_n	//trick for passing truncated char* to api that takes null-terminated strings, needs to temporarily modify string data
{
	char * ptr, *ptr0, old;
public:
	inline explicit make_string_n(char * src,unsigned len)
	{
		ptr = src; ptr0 = src+len;
		old = *ptr0;
		*ptr0 = 0;
	}
	inline ~make_string_n() {*ptr0 = old;}
	inline const char * get_ptr() const {return ptr;}
	inline operator const char * () const {return ptr;}	
};

void pfc_float_to_string(char * out,unsigned out_max,double val,unsigned precision,bool force_sign = false);//doesnt add E+X etc, has internal range limits, useful for storing float numbers as strings without having to bother with international coma/dot settings BS
double pfc_string_to_float(const char * src,unsigned len = infinite);

class string_list_nulldelimited
{
	mem_block_fast_t<char> data;
	unsigned len;
public:
	string_list_nulldelimited();
	inline const char * get_ptr() const {return data;}
	inline operator const char * () const {return data;}
	void add_string(const char *);
	void add_string_multi(const char *);
	void reset();
};

namespace pfc {
	template<>
	inline void swap_t(string8 & p_item1,string8 & p_item2)
	{
		string8::g_swap(p_item1,p_item2);
	}

	template<typename T>
	inline void swap_t(string_simple_t<T> & p_item1,string_simple_t<T> & p_item2)
	{
		string_simple_t<T>::g_swap(p_item1,p_item2);
	}
};

class format_float
{
public:
	format_float(double p_val,unsigned p_width,unsigned p_prec);
	format_float(const format_float & p_source) {*this = p_source;}

	inline const char * get_ptr() const {return m_buffer.get_ptr();}
	inline operator const char*() const {return m_buffer.get_ptr();}
private:
	string8 m_buffer;
};

class format_int
{
public:
	format_int(t_int64 p_val,unsigned p_width = 0,unsigned p_base = 10);
	format_int(const format_int & p_source) {*this = p_source;}
	inline const char * get_ptr() const {return m_buffer;}
	inline operator const char*() const {return m_buffer;}
private:
	char m_buffer[64];
};


class format_uint
{
public:
	format_uint(t_uint64 p_val,unsigned p_width = 0,unsigned p_base = 10);
	format_uint(const format_int & p_source) {*this = p_source;}
	inline const char * get_ptr() const {return m_buffer;}
	inline operator const char*() const {return m_buffer;}
private:
	char m_buffer[64];
};

class format_hex
{
public:
	format_hex(t_uint64 p_val,unsigned p_width = 0);
	format_hex(const format_hex & p_source) {*this = p_source;}
	inline const char * get_ptr() const {return m_buffer;}
	inline operator const char*() const {return m_buffer;}
private:
	char m_buffer[17];
};


typedef string8_fastalloc string_formatter;

class format_hexdump
{
public:
	format_hexdump(const void * p_buffer,unsigned p_bytes,const char * p_spacing = " ");

	inline const char * get_ptr() const {return m_formatter;}
	inline operator const char * () const {return m_formatter;}

private:
	string_formatter m_formatter;
};

class format_fixedpoint
{
public:
	format_fixedpoint(t_int64 p_val,unsigned p_point);
	format_fixedpoint(const format_fixedpoint & p_source) {*this = p_source;}
	inline const char * get_ptr() const {return m_buffer;}
	inline operator const char*() const {return m_buffer;}
private:
	string_formatter m_buffer;
};

inline string_base & operator<<(string_base & p_fmt,const char * p_source) {p_fmt.add_string_e(p_source); return p_fmt;}
inline string_base & operator<<(string_base & p_fmt,int p_val) {p_fmt.add_int_e(p_val); return p_fmt;}
inline string_base & operator<<(string_base & p_fmt,unsigned p_val) {p_fmt.add_uint_e(p_val); return p_fmt;}
inline string_base & operator<<(string_base & p_fmt,t_int64 p_val) {p_fmt.add_int_e(p_val); return p_fmt;}
inline string_base & operator<<(string_base & p_fmt,t_uint64 p_val) {p_fmt.add_uint_e(p_val); return p_fmt;}
inline string_base & operator<<(string_base & p_fmt,double p_val) {return p_fmt << format_float(p_val,0,7);}

inline string_base & operator<<(string_base & p_fmt,std::exception const & p_exception) {return p_fmt << p_exception.what();}

#endif //_PFC_STRING_H_