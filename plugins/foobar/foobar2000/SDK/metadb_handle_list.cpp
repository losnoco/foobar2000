#include "foobar2000.h"




namespace {
	struct custom_sort_data
	{
		HANDLE text;
		int subsong;
		unsigned index;
	};
}
static int __cdecl custom_sort_compare(const custom_sort_data & elem1, const custom_sort_data & elem2 )
{//depends on unicode/ansi, nonportable (win32 lstrcmpi)
	int ret = uSortStringCompare(elem1.text,elem2.text);//uStringCompare
//	if (ret == 0) ret = elem1.subsong - elem2.subsong;
	if (ret == 0) ret = elem1.index - elem2.index;
	return ret;
}

void metadb_handle_list_helper::sort_by_format_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,const char * spec,titleformat_hook * p_hook)
{
	service_ptr_t<titleformat_object> script;
	if (static_api_ptr_t<titleformat_compiler>()->compile(script,spec))
		sort_by_format_partial(p_list,base,count,script,p_hook);
}

void metadb_handle_list_helper::sort_by_format_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order,const char * spec,titleformat_hook * p_hook)
{
	service_ptr_t<titleformat_object> script;
	if (static_api_ptr_t<titleformat_compiler>()->compile(script,spec))
		sort_by_format_get_order_partial(p_list,base,count,order,script,p_hook);
}

void metadb_handle_list_helper::sort_by_format_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook)
{
	mem_block_t<unsigned> order(count);
	sort_by_format_get_order_partial(p_list,base,count,order,p_script,p_hook);
	p_list.reorder_partial(base,order,count);
}

void metadb_handle_list_helper::sort_by_format_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook)
{
	assert(base+count<=p_list.get_count());
	unsigned n;
	array_t<custom_sort_data> data(count);
	
	string8 temp;
	string8 temp2;
	temp.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
	temp.prealloc(512);
	temp2.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
	for(n=0;n<count;n++)
	{
		metadb_handle_ptr item;
		p_list.get_item_ex(item,base+n);
		assert(item.is_valid());

		item->format_title(p_hook,temp,p_script,0);
		data[n].index = n;
		data[n].text = uSortStringCreate(temp);
		data[n].subsong = item->get_subsong_index();
	}

	pfc::sort_t(data,custom_sort_compare,count);
	//qsort(data.get_ptr(),count,sizeof(custom_sort_data),(int (__cdecl *)(const void *elem1, const void *elem2 ))custom_sort_compare);

	for(n=0;n<count;n++)
	{
		order[n]=data[n].index;
		uSortStringFree(data[n].text);
	}
}

void metadb_handle_list_helper::sort_by_relative_path_partial(list_base_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count)
{
	assert(base+count<=p_list.get_count());
	mem_block_t<unsigned> order(count);
	sort_by_relative_path_get_order_partial(p_list,base,count,order);
	p_list.reorder_partial(base,order,count);
}

void metadb_handle_list_helper::sort_by_relative_path_get_order_partial(const list_base_const_t<metadb_handle_ptr> & p_list,unsigned base,unsigned count,unsigned* order)
{
	assert(base+count<=p_list.get_count());
	unsigned n;
	array_t<custom_sort_data> data(count);
	service_ptr_t<library_manager> api;
	library_manager::g_get(api);
	
	string8 temp;
	temp.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
	temp.prealloc(512);
	for(n=0;n<count;n++)
	{
		metadb_handle_ptr item;
		p_list.get_item_ex(item,base+n);
		if (!api->get_relative_path(item,temp)) temp = "";
		data[n].index = n;
		data[n].text = uSortStringCreate(temp);
		data[n].subsong = item->get_subsong_index();
	}

	pfc::sort_t(data,custom_sort_compare,count);
	//qsort(data.get_ptr(),count,sizeof(custom_sort_data),(int (__cdecl *)(const void *elem1, const void *elem2 ))custom_sort_compare);

	for(n=0;n<count;n++)
	{
		order[n]=data[n].index;
		uSortStringFree(data[n].text);
	}
}

void metadb_handle_list_helper::remove_duplicates(list_base_t<metadb_handle_ptr> & p_list)
{
	unsigned count = p_list.get_count();
	if (count>0)
	{
		bit_array_bittable mask(count);
		mem_block_t<unsigned> order(count);
		order_helper::g_fill(order,count);

		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr>,order.get_ptr());
		
		unsigned n;
		bool found = false;
		for(n=0;n<count-1;n++)
		{
			if (p_list.get_item(order[n])==p_list.get_item(order[n+1]))
			{
				found = true;
				mask.set(order[n+1],true);
			}
		}
		
		if (found) p_list.remove_mask(mask);
	}
}

void metadb_handle_list_helper::sort_by_pointer_remove_duplicates(list_base_t<metadb_handle_ptr> & p_list)
{
	unsigned count = p_list.get_count();
	if (count>0)
	{
		sort_by_pointer(p_list);
		bool b_found = false;
		unsigned n;
		for(n=0;n<count-1;n++)
		{
			if (p_list.get_item(n)==p_list.get_item(n+1))
			{
				b_found = true;
				break;
			}
		}

		if (b_found)
		{
			bit_array_bittable mask(count);
			unsigned n;
			for(n=0;n<count-1;n++)
			{
				if (p_list.get_item(n)==p_list.get_item(n+1))
					mask.set(n+1,true);
			}
			p_list.remove_mask(mask);
		}
	}
}

void metadb_handle_list_helper::sort_by_path_quick(list_base_t<metadb_handle_ptr> & p_list)
{
	p_list.sort_t(metadb::path_compare_metadb_handle);
}


void metadb_handle_list_helper::sort_by_pointer(list_base_t<metadb_handle_ptr> & p_list)
{
	//it seems MSVC71 /GL does something highly retarded here
	//p_list.sort_t(pfc::compare_t<metadb_handle_ptr>);
	p_list.sort();
}

unsigned metadb_handle_list_helper::bsearch_by_pointer(const list_base_const_t<metadb_handle_ptr> & p_list,const metadb_handle_ptr & val)
{
	unsigned blah;
	if (p_list.bsearch_t(pfc::compare_t<metadb_handle_ptr>,val,blah)) return blah;
	else return infinite;
}


void metadb_handle_list_helper::sorted_by_pointer_extract_difference(metadb_handle_list const & p_list_1,metadb_handle_list const & p_list_2,metadb_handle_list & p_list_1_specific,metadb_handle_list & p_list_2_specific)
{
	unsigned found_1, found_2;
	const unsigned count_1 = p_list_1.get_count(), count_2 = p_list_2.get_count();
	unsigned ptr_1, ptr_2;

	found_1 = found_2 = 0;
	ptr_1 = ptr_2 = 0;
	while(ptr_1 < count_1 || ptr_2 < count_2)
	{
		while(ptr_1 < count_1 && (ptr_2 == count_2 || p_list_1[ptr_1] < p_list_2[ptr_2]))
		{
			found_1++;
			unsigned ptr_1_new = ptr_1 + 1;
			while(ptr_1_new < count_1 && p_list_1[ptr_1_new] == p_list_1[ptr_1]) ptr_1_new++;
			ptr_1 = ptr_1_new;
		}
		while(ptr_2 < count_2 && (ptr_1 == count_1 || p_list_2[ptr_2] < p_list_1[ptr_1]))
		{
			found_2++;
			unsigned ptr_2_new = ptr_2 + 1;
			while(ptr_2_new < count_2 && p_list_2[ptr_2_new] == p_list_2[ptr_2]) ptr_2_new++;
			ptr_2 = ptr_2_new;
		}
		while(ptr_1 < count_1 && ptr_2 < count_2 && p_list_1[ptr_1] == p_list_2[ptr_2]) {ptr_1++; ptr_2++;}
	}

	

	p_list_1_specific.set_count(found_1);
	p_list_2_specific.set_count(found_2);
	if (found_1 > 0 || found_2 > 0)
	{
		found_1 = found_2 = 0;
		ptr_1 = ptr_2 = 0;

		while(ptr_1 < count_1 || ptr_2 < count_2)
		{
			while(ptr_1 < count_1 && (ptr_2 == count_2 || p_list_1[ptr_1] < p_list_2[ptr_2]))
			{
				p_list_1_specific[found_1++] = p_list_1[ptr_1];
				unsigned ptr_1_new = ptr_1 + 1;
				while(ptr_1_new < count_1 && p_list_1[ptr_1_new] == p_list_1[ptr_1]) ptr_1_new++;
				ptr_1 = ptr_1_new;
			}
			while(ptr_2 < count_2 && (ptr_1 == count_1 || p_list_2[ptr_2] < p_list_1[ptr_1]))
			{
				p_list_2_specific[found_2++] = p_list_2[ptr_2];
				unsigned ptr_2_new = ptr_2 + 1;
				while(ptr_2_new < count_2 && p_list_2[ptr_2_new] == p_list_2[ptr_2]) ptr_2_new++;
				ptr_2 = ptr_2_new;
			}
			while(ptr_1 < count_1 && ptr_2 < count_2 && p_list_1[ptr_1] == p_list_2[ptr_2]) {ptr_1++; ptr_2++;}
		}

	}
}

double metadb_handle_list_helper::calc_total_duration(const list_base_const_t<metadb_handle_ptr> & p_list)
{
	double ret = 0;
	unsigned n, m = p_list.get_count();
	for(n=0;n<m;n++)
	{
		double temp = p_list.get_item(n)->get_length();
		if (temp > 0) ret += temp;
	}
	return ret;
}

void metadb_handle_list_helper::sort_by_path(list_base_t<metadb_handle_ptr> & p_list)
{
	sort_by_format_partial(p_list,0,p_list.get_count(),"%path_sort%",0);
}
