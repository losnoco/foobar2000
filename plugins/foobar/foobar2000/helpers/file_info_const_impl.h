//#define __file_info_const_impl_have_hintmap__

class file_info_const_impl : public file_info
{
public:
	file_info_const_impl(const file_info & p_source) {copy(p_source);}
	file_info_const_impl(const file_info_const_impl & p_source) {copy(p_source);}
	file_info_const_impl() {m_meta_count = m_info_count = 0; m_length = 0; m_replaygain.reset();}

	double		get_length() const {return m_length;}
	
	unsigned	meta_get_count() const {return m_meta_count;}
	const char*	meta_enum_name(unsigned p_index) const {return m_meta[p_index].m_name;}
	unsigned	meta_enum_value_count(unsigned p_index) const;
	const char*	meta_enum_value(unsigned p_index,unsigned p_value_number) const;
	unsigned	meta_find_ex(const char * p_name,unsigned p_name_length) const;

	unsigned	info_get_count() const {return m_info_count;}
	const char*	info_enum_name(unsigned p_index) const {return m_info[p_index].m_name;}
	const char*	info_enum_value(unsigned p_index) const {return m_info[p_index].m_value;}


	const file_info_const_impl & operator=(const file_info & p_source) {copy(p_source); return *this;}
	const file_info_const_impl & operator=(const file_info_const_impl & p_source) {copy(p_source); return *this;}
	void copy(const file_info & p_source);
	void reset();

	replaygain_info	get_replaygain() const {return m_replaygain;}

private:
	void		set_length(double p_length) {assert(0);}

	unsigned	meta_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) {assert(0); return infinite;}
	void		meta_insert_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length) {assert(0);}
	void		meta_remove_mask(const bit_array & p_mask) {assert(0);}
	void		meta_reorder(const unsigned * p_order) {assert(0);}
	void		meta_remove_values(unsigned p_index,const bit_array & p_mask) {assert(0);}
	void		meta_modify_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length) {assert(0);}

	unsigned	info_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) {assert(0); return infinite;}
	void		info_remove_mask(const bit_array & p_mask) {assert(0);}

	void			set_replaygain(const replaygain_info & p_info) {assert(0);}
	
	unsigned	meta_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) {assert(0); return infinite;}
	unsigned	info_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) {assert(0); return infinite;}
public:
	struct meta_entry {
		const char * m_name;
		unsigned m_valuecount;
		const char * const * m_valuemap;
	};

	struct info_entry {
		const char * m_name;
		const char * m_value;
	};

#ifdef __file_info_const_impl_have_hintmap__
	typedef t_uint16 t_hintentry;
#endif//__file_info_const_impl_have_hintmap__
private:
	mem_block_t<char> m_buffer;
	t_uint16 m_meta_count;
	t_uint16 m_info_count;
	
	const meta_entry * m_meta;
	const info_entry * m_info;

#ifdef __file_info_const_impl_have_hintmap__
	const t_hintentry * m_hintmap;
#endif

	double m_length;
	replaygain_info m_replaygain;
};
