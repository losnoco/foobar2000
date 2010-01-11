#ifndef _PFC_STRING_LIST_H_
#define _PFC_STRING_LIST_H_

typedef list_base_const_t<const char*> string_list_const;

class string_list_impl : public string_list_const
{
public:
	unsigned get_count() const {return m_data.get_size();}
	void get_item_ex(const char* & p_out, unsigned n) const {p_out = m_data[n];}

	inline const char * operator[] (unsigned n) const {return m_data[n];}

	void add_item(const char * p_string)
	{
		unsigned idx = m_data.get_size();
		m_data.set_size(idx + 1);
		m_data[idx] = strdup(p_string);
	}

	void add_items(const string_list_const & p_source) {_append(p_source);}

	void remove_all() 
	{
		for(unsigned n=0;n<m_data.get_size();n++) free(m_data[n]);
		m_data.set_size(0);
	}

	~string_list_impl() {remove_all();}

	inline string_list_impl() {}
	inline string_list_impl(const string_list_impl & p_source) {_copy(p_source);}
	inline string_list_impl(const string_list_const & p_source) {_copy(p_source);}
	inline const string_list_impl & operator=(const string_list_impl & p_source) {_copy(p_source);return *this;}
	inline const string_list_impl & operator=(const string_list_const & p_source) {_copy(p_source);return *this;}
	inline const string_list_impl & operator+=(const string_list_impl & p_source) {_append(p_source);return *this;}
	inline const string_list_impl & operator+=(const string_list_const & p_source) {_append(p_source);return *this;}

private:

	void _append(const string_list_const & p_source)
	{
		const unsigned toadd = p_source.get_count(), base = m_data.get_size();
		m_data.set_size(base+toadd);
		for(unsigned n=0;n<toadd;n++) m_data[base+n] = strdup(p_source[n]);
	}

	void _copy(const string_list_const & p_source)
	{
		const unsigned newcount = p_source.get_count();
		for(unsigned n=0;n<m_data.get_size();n++) free(m_data[n]);		
		m_data.set_size(newcount);
		for(unsigned n=0;n<newcount;n++) m_data[n] = strdup(p_source[n]);
	}

	mem_block_fast_t<char*> m_data;
};

#endif //_PFC_STRING_LIST_H_