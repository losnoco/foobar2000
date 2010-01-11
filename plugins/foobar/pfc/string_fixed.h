#ifndef _PFC_STRING_FIXED_H_
#define _PFC_STRING_FIXED_H_

template<unsigned t_size>
class string_fixed_t : public string_base
{
public:
	inline string_fixed_t() {init();}
	inline string_fixed_t(const string_fixed_t<t_size> & p_source) {init(); *this = p_source;}
	inline string_fixed_t(const char * p_source) {init(); set_string(p_source);}
	
	inline const string_fixed_t<t_size> & operator=(const string_fixed_t<t_size> & p_source) {set_string(p_source);return *this;}
	inline const string_fixed_t<t_size> & operator=(const char * p_source) {set_string(p_source);return *this;}

	inline operator const char * () const {return m_data;}
	
	const char * get_ptr() const {return m_data;}

	void add_string(const char * ptr,unsigned len)
	{
		unsigned read = 0;
		while(m_length < max_length && read < len)
		{
			if (ptr[read] == 0) break;
			m_data[m_length++] = ptr[read++];
		}
		m_data[m_length] = 0;
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
	unsigned length() const {return m_length;}

private:
	inline void init() {m_length = 0; m_data[0] = 0;}
	enum {max_length = t_size - 1};
	unsigned m_length;
	char m_data[t_size];
};

#endif