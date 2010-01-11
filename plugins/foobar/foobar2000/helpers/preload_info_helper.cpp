#include "stdafx.h"

t_io_result preload_info_helper::preload_info(metadb_handle_ptr p_item,HWND p_parent_window,bool p_showerror)
{
	if (p_item->is_info_loaded()) return io_result_success;
	return static_api_ptr_t<metadb_io>()->load_info(p_item,metadb_io::load_info_default,p_parent_window,p_showerror);
}

bool preload_info_helper::are_all_loaded(const list_base_const_t<metadb_handle_ptr> & p_items)
{
	unsigned n, m = p_items.get_count();
	for(n=0;n<m;n++)
	{
		if (!p_items[n]->is_info_loaded()) return false;
	}
	return true;
}

bool preload_info_helper::preload_info_multi(const list_base_const_t<metadb_handle_ptr> & p_items,HWND p_parent_window,bool p_showerror)
{
	if (are_all_loaded(p_items)) return true;
	return static_api_ptr_t<metadb_io>()->load_info_multi(p_items,metadb_io::load_info_default,p_parent_window,p_showerror,0);
}

bool preload_info_helper::preload_info_multi_modalcheck(const list_base_const_t<metadb_handle_ptr> & p_items,HWND p_parent_window,bool p_showerror)
{
	if (are_all_loaded(p_items)) return true;
	if (!modal_dialog_scope::can_create()) return false;
	return static_api_ptr_t<metadb_io>()->load_info_multi(p_items,metadb_io::load_info_default,p_parent_window,p_showerror,0);
}
