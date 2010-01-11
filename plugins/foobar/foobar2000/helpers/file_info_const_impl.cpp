#include "stdafx.h"


/*
order of things

  meta entries
  meta value map
  info entries 
  string buffer

*/

inline static char* stringbuffer_append(char * & buffer,const char * value)
{
	char * ret = buffer;
	while(*value) *(buffer++) = *(value++);
	*(buffer++) = 0;
	return ret;
}

#ifdef __file_info_const_impl_have_hintmap__

namespace {
	class sort_callback_hintmap_impl : public pfc::sort_callback
	{
	public:
		sort_callback_hintmap_impl(const file_info_const_impl::meta_entry * p_meta,file_info_const_impl::t_hintentry * p_hintmap)
			: m_meta(p_meta), m_hintmap(p_hintmap)
		{
		}
		
		int compare(unsigned p_index1, unsigned p_index2) const
		{
//			profiler(sort_callback_hintmap_impl_compare);
			return stricmp_utf8(m_meta[m_hintmap[p_index1]].m_name,m_meta[m_hintmap[p_index2]].m_name);
		}
		
		void swap(unsigned p_index1, unsigned p_index2)
		{
			pfc::swap_t<file_info_const_impl::t_hintentry>(m_hintmap[p_index1],m_hintmap[p_index2]);
		}
	private:
		const file_info_const_impl::meta_entry * m_meta;
		file_info_const_impl::t_hintentry * m_hintmap;
	};

	class bsearch_callback_hintmap_impl : public pfc::bsearch_callback
	{
	public:
		bsearch_callback_hintmap_impl(
			const file_info_const_impl::meta_entry * p_meta,
			const file_info_const_impl::t_hintentry * p_hintmap,
			const char * p_name,
			unsigned p_name_length)
			: m_meta(p_meta), m_hintmap(p_hintmap), m_name(p_name), m_name_length(p_name_length)
		{
		}

		int test(unsigned p_index) const
		{
			return stricmp_utf8_ex(m_meta[m_hintmap[p_index]].m_name,infinite,m_name,m_name_length);
		}

	private:
		const file_info_const_impl::meta_entry * m_meta;
		const file_info_const_impl::t_hintentry * m_hintmap;
		const char * m_name;
		unsigned m_name_length;
	};
}

#endif//__file_info_const_impl_have_hintmap__

void file_info_const_impl::copy(const file_info & p_source)
{
//	profiler(file_info_const_impl__copy);
	unsigned meta_size = 0;
	unsigned info_size = 0;
	unsigned valuemap_size = 0;
	unsigned stringbuffer_size = 0;
#ifdef __file_info_const_impl_have_hintmap__
	unsigned hintmap_size = 0;
#endif

	enum {metacount_upperlimit = 0xFFFF};

	{
//		profiler(file_info_const_impl__copy__pass1);
		unsigned index;
		m_meta_count = (t_uint16) pfc::min_t<unsigned>(p_source.meta_get_count(),metacount_upperlimit);
		meta_size = m_meta_count * sizeof(meta_entry);
#ifdef __file_info_const_impl_have_hintmap__
		hintmap_size = m_meta_count * sizeof(t_hintentry);
#endif//__file_info_const_impl_have_hintmap__
		for(index = 0; index < m_meta_count; index++ )
		{
			
			stringbuffer_size += strlen(p_source.meta_enum_name(index)) + 1;

			unsigned val; const unsigned val_max = pfc::min_t<unsigned>(p_source.meta_enum_value_count(index),metacount_upperlimit);
			
			valuemap_size += val_max * sizeof(char*);

			for(val = 0; val < val_max; val++ )
			{
				stringbuffer_size += strlen(p_source.meta_enum_value(index,val)) + 1;
			}
		}

		m_info_count = (t_uint16) pfc::min_t<unsigned>(p_source.info_get_count(),metacount_upperlimit);
		info_size = m_info_count * sizeof(info_entry);
		for(index = 0; index < m_info_count; index++ )
		{
			stringbuffer_size += strlen(p_source.info_enum_name(index)) + 1;
			stringbuffer_size += strlen(p_source.info_enum_value(index)) + 1;
		}
	}


	{
//		profiler(file_info_const_impl__copy__alloc);
		m_buffer.set_size(
#ifdef __file_info_const_impl_have_hintmap__
			hintmap_size + 
#endif
			meta_size + info_size + valuemap_size + stringbuffer_size);
	}

	char * walk = m_buffer.get_ptr();

#ifdef __file_info_const_impl_have_hintmap__
	t_hintentry* hintmap = (t_hintentry*) walk;
	walk += hintmap_size;
#endif
	meta_entry * meta = (meta_entry*) walk;
	walk += meta_size;
	char ** valuemap = (char**) walk;
	walk += valuemap_size;
	info_entry * info = (info_entry*) walk;
	walk += info_size;
	char * stringbuffer = walk;

	m_meta = meta;
	m_info = info;
#ifdef __file_info_const_impl_have_hintmap__
	m_hintmap = hintmap;
#endif

	{
//		profiler(file_info_const_impl__copy__pass2);
		unsigned index;
		for( index = 0; index < m_meta_count; index ++ )
		{
			unsigned val; const unsigned val_max = pfc::min_t<unsigned>(p_source.meta_enum_value_count(index),metacount_upperlimit);

			meta[index].m_name = stringbuffer_append(stringbuffer, p_source.meta_enum_name(index) );
			meta[index].m_valuemap = valuemap;
			meta[index].m_valuecount = val_max;
			
			for( val = 0; val < val_max ; val ++ )
				*(valuemap ++ ) = stringbuffer_append(stringbuffer, p_source.meta_enum_value(index,val) );
		}

		for( index = 0; index < m_info_count; index ++ )
		{
			info[index].m_name = stringbuffer_append(stringbuffer, p_source.info_enum_name(index) );
			info[index].m_value = stringbuffer_append(stringbuffer, p_source.info_enum_value(index) );
		}
	}

	m_length = p_source.get_length();
	m_replaygain = p_source.get_replaygain();
#ifdef __file_info_const_impl_have_hintmap__
	{
//		profiler(file_info_const_impl__copy__hintmap);
		for(unsigned n=0;n<m_meta_count;n++) hintmap[n]=n;
		pfc::sort(sort_callback_hintmap_impl(meta,hintmap),m_meta_count);
	}
#endif//__file_info_const_impl_have_hintmap__
}


void file_info_const_impl::reset()
{
	m_meta_count = m_info_count = 0; m_length = 0; m_replaygain.reset();
}

unsigned file_info_const_impl::meta_find_ex(const char * p_name,unsigned p_name_length) const
{
#ifdef __file_info_const_impl_have_hintmap__
	unsigned result = infinite;
	if (!pfc::bsearch(m_meta_count,bsearch_callback_hintmap_impl(m_meta,m_hintmap,p_name,p_name_length),result)) return infinite;
	else return m_hintmap[result];
#else
	return file_info::meta_find_ex(p_name,p_name_length);
#endif
}