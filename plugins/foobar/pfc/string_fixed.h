#ifndef _PFC_STRING_FIXED_H_
#define _PFC_STRING_FIXED_H_

template<unsigned t_size>
class string_fixed_t : public string_base
{
public:
	inline string_fixed_t() {init();}
	inline string_fixed_t(const string_fixed_t<t_size> & p_source) {init(); *this = p_source;}
	inline string_fixed_t(const char * p_source) {init(); set_string_e(p_source);}
	
	inline const string_fixed_t<t_size> & operator=(const string_fixed_t<t_size> & p_source) {set_string_e(p_source);return *this;}
	inline const string_fixed_t<t_size> & operator=(const char * p_source) {set_string_e(p_source);return *this;}

	char * lock_buffer(unsigned p_requested_length)
	{
		if (p_requested_length >= t_size) return NULL;
		memset(m_data,0,sizeof(m_data));
		return m_data;
	}
	void unlock_buffer()
	{
		m_length = strlen(m_data);
	}

	inline operator const char * () const {return m_data;}
	
	const char * get_ptr() const {return m_data;}

	bool add_string(const char * ptr,unsigned len) {
		len = strlen_max(ptr,len);
		if (m_length + len > max_length) return false;
		for(unsigned n=0;n<len;n++) {
			m_data[m_length++] = ptr[n];
		}
		m_data[m_length] = 0;
		return true;
	}
	void truncate(unsigned len)
	{
		if (len > max_length) len = max_length;
		if (m_length > len)
		{
			m_length = len;
			m_data[len] = 0;
		}
	}
	unsigned get_length() const {return m_length;}

private:
	inline void init() {m_length = 0; m_data[0] = 0;}
	enum {max_length = t_size - 1};
	unsigned m_length;
	char m_data[t_size];
};

#endif