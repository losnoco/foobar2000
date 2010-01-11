#include "foobar2000.h"


static void g_on_files_deleted_sorted(const list_base_const_t<const char *> & p_items)
{
	static_api_ptr_t<library_manager>()->on_files_deleted_sorted(p_items);
	static_api_ptr_t<playlist_manager>()->on_files_deleted_sorted(p_items);

	service_ptr_t<file_operation_callback> ptr;
	service_enum_t<file_operation_callback> e;
	while(e.next(ptr))
	{
		ptr->on_files_deleted_sorted(p_items);
	}
}

static void g_on_files_moved_sorted(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to)
{
	static_api_ptr_t<playlist_manager>()->on_files_moved_sorted(p_from,p_to);
	static_api_ptr_t<playlist_manager>()->on_files_deleted_sorted(p_from);

	service_ptr_t<file_operation_callback> ptr;
	service_enum_t<file_operation_callback> e;
	while(e.next(ptr))
	{
		ptr->on_files_moved_sorted(p_from,p_to);
	}
}

static void g_on_files_copied_sorted(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to)
{
	service_ptr_t<file_operation_callback> ptr;
	service_enum_t<file_operation_callback> e;
	while(e.next(ptr))
	{
		ptr->on_files_copied_sorted(p_from,p_to);
	}
}

static void generate_permutation(unsigned * ptr,unsigned count)
{
	unsigned n;
	for(n=0;n<count;n++)
		ptr[n] = n;
}

void file_operation_callback::g_on_files_deleted(const list_base_const_t<const char *> & p_items)
{
	if (core_api::assert_main_thread())
	{
		unsigned count = p_items.get_count();
		if (count > 0)
		{
			if (count == 1) g_on_files_deleted_sorted(p_items);
			else
			{
				mem_block_t<unsigned> order(count);
				generate_permutation(order,count);
				p_items.sort_get_permutation_t(metadb::path_compare,order.get_ptr());
				g_on_files_deleted_sorted(list_permutation_t<const char*>(p_items,order.get_ptr(),count));
			}
		}
	}
}

void file_operation_callback::g_on_files_moved(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to)
{
	if (core_api::assert_main_thread())
	{
		assert(p_from.get_count() == p_to.get_count());
		unsigned count = p_from.get_count();
		if (count > 0)
		{
			if (count == 1) g_on_files_moved_sorted(p_from,p_to);
			else
			{
				mem_block_t<unsigned> order(count);
				generate_permutation(order,count);
				p_from.sort_get_permutation_t(metadb::path_compare,order.get_ptr());
				g_on_files_moved_sorted(list_permutation_t<const char*>(p_from,order.get_ptr(),count),list_permutation_t<const char*>(p_to,order.get_ptr(),count));
			}
		}
	}
}

void file_operation_callback::g_on_files_copied(const list_base_const_t<const char *> & p_from,const list_base_const_t<const char *> & p_to)
{
	if (core_api::assert_main_thread())
	{
		assert(p_from.get_count() == p_to.get_count());
		unsigned count = p_from.get_count();
		if (count > 0)
		{
			if (count == 1) g_on_files_copied_sorted(p_from,p_to);
			else
			{
				mem_block_t<unsigned> order(count);
				generate_permutation(order,count);
				p_from.sort_get_permutation_t(metadb::path_compare,order.get_ptr());
				g_on_files_copied_sorted(list_permutation_t<const char*>(p_from,order.get_ptr(),count),list_permutation_t<const char*>(p_to,order.get_ptr(),count));
			}
		}
	}
}
