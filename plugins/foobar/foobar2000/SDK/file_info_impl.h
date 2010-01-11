#ifndef _FOOBAR2000_SDK_FILE_INFO_IMPL_H_
#define _FOOBAR2000_SDK_FILE_INFO_IMPL_H_


class info_storage
{
public:
	unsigned add_item(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	void remove_mask(const bit_array & p_mask);	
	inline unsigned get_count() const {return m_info.get_count();}
	inline const char * get_name(unsigned p_index) const {return m_info[p_index].get_name();}
	inline const char * get_value(unsigned p_index) const {return m_info[p_index].get_value();}
	void copy_from(const file_info & p_info);
	~info_storage();
private:
	struct info_entry
	{
	
		void init(const char * p_name,unsigned p_name_len,const char * p_value,unsigned p_value_len);
		void deinit();
		inline const char * get_name() const {return m_name;}
		inline const char * get_value() const {return m_value;}
		
	
		char * m_name;
		char * m_value;
	};
	mem_block_list_t<info_entry> m_info;
};

class meta_storage
{
public:
	meta_storage();
	~meta_storage();

	unsigned add_entry(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	void insert_value(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length);
	void modify_value(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length);
	void remove_values(unsigned p_index,const bit_array & p_mask);
	void remove_mask(const bit_array & p_mask);
	void copy_from(const file_info & p_info);

	inline void reorder(const unsigned * p_order);

	inline unsigned get_count() const {return m_data.get_size();}
	
	inline const char * get_name(unsigned p_index) const {assert(p_index < m_data.get_size()); return m_data[p_index].get_name();}
	inline const char * get_value(unsigned p_index,unsigned p_value_index) const {assert(p_index < m_data.get_size()); return m_data[p_index].get_value(p_value_index);}
	inline unsigned get_value_count(unsigned p_index) const {assert(p_index < m_data.get_size()); return m_data[p_index].get_value_count();}

	struct meta_entry
	{
		meta_entry() {}
		meta_entry(const char * p_name,unsigned p_name_len,const char * p_avlue,unsigned p_value_len);

		void remove_values(const bit_array & p_mask);
		void insert_value(unsigned p_value_index,const char * p_value,unsigned p_value_length);
		void modify_value(unsigned p_value_index,const char * p_value,unsigned p_value_length);

		inline const char * get_name() const {return m_name;}
		inline const char * get_value(unsigned p_index) const {return m_values[p_index];}
		inline unsigned get_value_count() const {return m_values.get_size();}
		

		string_simple m_name;
		array_hybrid_t<string_simple,1,array_fast_t<string_simple> > m_values;

	};
private:
	

	array_hybrid_t<meta_entry,10, array_fast_t<meta_entry> > m_data;
};

namespace pfc
{
	template<>
	inline void swap_t(meta_storage::meta_entry & p_item1,meta_storage::meta_entry & p_item2)
	{
		swap_t(p_item1.m_name,p_item2.m_name);
		swap_t(p_item1.m_values,p_item2.m_values);
	}
};

class file_info_impl : public file_info
{
public:
	file_info_impl(const file_info_impl & p_source);
	file_info_impl(const file_info & p_source);
	file_info_impl();
	~file_info_impl();

	double		get_length() const;
	void		set_length(double p_length);

	void		copy_meta(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	void		copy_info(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	
	unsigned	meta_get_count() const;
	const char*	meta_enum_name(unsigned p_index) const;
	unsigned	meta_enum_value_count(unsigned p_index) const;
	const char*	meta_enum_value(unsigned p_index,unsigned p_value_number) const;
	unsigned	meta_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	void		meta_insert_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length);
	void		meta_remove_mask(const bit_array & p_mask);
	void		meta_reorder(const unsigned * p_order);
	void		meta_remove_values(unsigned p_index,const bit_array & p_mask);
	void		meta_modify_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length);

	unsigned	info_get_count() const;
	const char*	info_enum_name(unsigned p_index) const;
	const char*	info_enum_value(unsigned p_index) const;
	unsigned	info_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	void		info_remove_mask(const bit_array & p_mask);

	const file_info_impl & operator=(const file_info_impl & p_source);

	replaygain_info	get_replaygain() const;
	void			set_replaygain(const replaygain_info & p_info);

protected:
	unsigned	meta_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	unsigned	info_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
private:


	meta_storage m_meta;
	info_storage m_info;
	

	double m_length;

	replaygain_info m_replaygain;
};

typedef file_info_impl file_info_i;//for compatibility

#endif