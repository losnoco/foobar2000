#include "foobar2000.h"

static unsigned merge_tags_calc_rating(const char * field,const file_info * info)
{
	unsigned field_index = info->meta_find(field);
	unsigned ret = 0;
	if (field_index != infinite)
	{
		unsigned n,m = info->meta_enum_value_count(field_index);
		for(n=0;n<m;n++)
			ret += strlen(info->meta_enum_value(field_index,n));//yes, strlen on utf8 data
	}
	return ret;
}

static void merge_tags_copy_info(const char * field,const file_info * from,file_info * to)
{
	const char * val = from->info_get(field);
	if (val) to->info_set(field,val);
}

void file_info::merge(const list_base_const_t<const file_info*> & p_in)
{
	unsigned in_count = p_in.get_count();
	if (in_count == 0)
	{
		meta_remove_all();
		return;
	}
	else if (in_count == 1)
	{
		const file_info * info = p_in[0];

		copy_meta(*info);

		set_replaygain(replaygain_info::g_merge(get_replaygain(),info->get_replaygain()));

		overwrite_info(*info);

		//copy_info_single_by_name(*info,"tagtype");
		
		return;
	}

	ptr_list_t<const char> fieldnames;

	{
		unsigned in_ptr;
		for(in_ptr = 0; in_ptr < in_count; in_ptr++ )
		{
			const file_info * info = p_in[in_ptr];
			unsigned field_ptr, field_max = info->meta_get_count();
			for(field_ptr = 0; field_ptr < field_max; field_ptr++ )
				fieldnames.add_item(info->meta_enum_name(field_ptr));
		}
	}
	
	fieldnames.sort_t(stricmp_utf8);
	
	meta_remove_all();

	{
		unsigned fieldnames_ptr, fieldnames_max = fieldnames.get_count();
		for(fieldnames_ptr = 0; fieldnames_ptr < fieldnames_max; )
		{
			const char * fieldname = fieldnames[fieldnames_ptr];
			unsigned in_ptr, in_best = infinite, in_best_rating = 0;
			for(in_ptr = 0; in_ptr < in_count; in_ptr++)//SLOW
			{
				unsigned rating = merge_tags_calc_rating(fieldname,p_in[in_ptr]);
				if (rating > in_best_rating) {in_best = in_ptr; in_best_rating = rating;}
			}

			if (in_best != infinite)
			{
				copy_meta_single_by_name_nocheck(*p_in[in_best],fieldname);
			}

			do {
				fieldnames_ptr++;
			} while(fieldnames_ptr < fieldnames_max && !stricmp_utf8(fieldnames[fieldnames_ptr],fieldname));
		}
	}

	{
		string8_fastalloc tagtype;
		replaygain_info rg = get_replaygain();
		unsigned in_ptr;
		for(in_ptr = 0; in_ptr < in_count; in_ptr++ )
		{
			const file_info * info = p_in[in_ptr];
			rg = replaygain_info::g_merge(rg, info->get_replaygain());
			unsigned field_ptr, field_max = info->info_get_count();
			for(field_ptr = 0; field_ptr < field_max; field_ptr++ )
			{
				const char * field_name = info->info_enum_name(field_ptr), * field_value = info->info_enum_value(field_ptr);
				if (*field_value)
				{
					if (!stricmp_utf8(field_name,"tagtype"))
					{
						if (!tagtype.is_empty()) tagtype += "|";
						tagtype += field_value;
					}
				}
			}
		}
		if (!tagtype.is_empty()) info_set("tagtype",tagtype);
		set_replaygain(rg);
	}
}

void file_info::overwrite_info(const file_info & p_source) {
	unsigned count = p_source.info_get_count();
	for(unsigned n=0;n<count;n++) {
		info_set(p_source.info_enum_name(n),p_source.info_enum_value(n));
	}
}