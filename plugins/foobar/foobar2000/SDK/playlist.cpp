#include "foobar2000.h"


namespace {
	class enum_items_callback_retrieve_item : public playlist_manager::enum_items_callback
	{
		metadb_handle_ptr m_item;
	public:
		enum_items_callback_retrieve_item() : m_item(0) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			assert(m_item.is_empty());
			m_item = p_location;
			return false;
		}
		inline const metadb_handle_ptr & get_item() {return m_item;}
	};

	class enum_items_callback_retrieve_selection : public playlist_manager::enum_items_callback
	{
		bool m_state;
	public:
		enum_items_callback_retrieve_selection() : m_state(false) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			m_state = b_selected;
			return false;
		}
		inline bool get_state() {return m_state;}
	};

	class enum_items_callback_retrieve_selection_mask : public playlist_manager::enum_items_callback
	{
		bit_array_var & m_out;
	public:
		enum_items_callback_retrieve_selection_mask(bit_array_var & p_out) : m_out(p_out) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			m_out.set(p_index,b_selected);
			return true;
		}
	};

	class enum_items_callback_retrieve_all_items : public playlist_manager::enum_items_callback
	{
		list_base_t<metadb_handle_ptr> & m_out;
	public:
		enum_items_callback_retrieve_all_items(list_base_t<metadb_handle_ptr> & p_out) : m_out(p_out) {m_out.remove_all();}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			m_out.add_item(p_location);
			return true;
		}
	};

	class enum_items_callback_retrieve_selected_items : public playlist_manager::enum_items_callback
	{
		list_base_t<metadb_handle_ptr> & m_out;
	public:
		enum_items_callback_retrieve_selected_items(list_base_t<metadb_handle_ptr> & p_out) : m_out(p_out) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			if (b_selected) m_out.add_item(p_location);
			return true;
		}
	};

	class enum_items_callback_count_selection : public playlist_manager::enum_items_callback
	{
		unsigned m_counter,m_max;
	public:
		enum_items_callback_count_selection(unsigned p_max) : m_max(p_max), m_counter(0) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			if (b_selected) 
			{
				if (++m_counter >= m_max) return false;
			}
			return true;
		}
		
		inline unsigned get_count() {return m_counter;}
	};

}

void playlist_manager::playlist_get_all_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out)
{
	playlist_get_items(p_playlist,out,bit_array_true());
}

void playlist_manager::playlist_get_selected_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out)
{
	playlist_enum_items(p_playlist,&enum_items_callback_retrieve_selected_items(out),bit_array_true());
}

void playlist_manager::playlist_get_selection_mask(unsigned p_playlist,bit_array_var & out)
{
	playlist_enum_items(p_playlist,&enum_items_callback_retrieve_selection_mask(out),bit_array_true());
}

bool playlist_manager::playlist_is_item_selected(unsigned p_playlist,unsigned p_item)
{
	enum_items_callback_retrieve_selection callback;
	playlist_enum_items(p_playlist,&callback,bit_array_one(p_item));
	return callback.get_state();
}

bool playlist_manager::playlist_get_item_handle(metadb_handle_ptr & p_out,unsigned p_playlist,unsigned p_item)
{
	enum_items_callback_retrieve_item callback;
	playlist_enum_items(p_playlist,&callback,bit_array_one(p_item));
	p_out = callback.get_item();
	return p_out.is_valid();
}

bool playlist_manager::playlist_move_selection(unsigned p_playlist,int p_delta)
{
	if (p_delta==0) return true;
	
	unsigned count = playlist_get_item_count(p_playlist);
	
	mem_block_t<unsigned> order(count);
	mem_block_t<bool> selection(count);
	
	{
		unsigned n;
		for(n=0;n<count;n++) order[n]=n;
	}

	playlist_get_selection_mask(p_playlist,bit_array_var_table(selection));

	if (p_delta<0)
	{
		for(;p_delta<0;p_delta++)
		{
			unsigned idx;
			for(idx=1;idx<count;idx++)
			{
				if (selection[idx] && !selection[idx-1])
				{
					order.swap(idx,idx-1);
					selection.swap(idx,idx-1);
				}
			}
		}
	}
	else
	{
		for(;p_delta>0;p_delta--)
		{
			unsigned idx;
			for(idx=count-2;(int)idx>=0;idx--)
			{
				if (selection[idx] && !selection[idx+1])
				{
					order.swap(idx,idx+1);
					selection.swap(idx,idx+1);
				}
			}
		}
	}

	return playlist_reorder_items(p_playlist,order,count);
}

//retrieving status
unsigned playlist_manager::activeplaylist_get_item_count()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return 0;
	else return playlist_get_item_count(playlist);
}

void playlist_manager::activeplaylist_enum_items(enum_items_callback * p_callback,const bit_array & p_mask)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_enum_items(playlist,p_callback,p_mask);
}

unsigned playlist_manager::activeplaylist_get_focus_item()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return infinite;
	else return playlist_get_focus_item(playlist);
}

bool playlist_manager::activeplaylist_get_name(string_base & p_out)
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return false;
	else return playlist_get_name(playlist,p_out);
}

//modifying playlist
bool playlist_manager::activeplaylist_reorder_items(const unsigned * order,unsigned count)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_reorder_items(playlist,order,count);
	else return false;
}

void playlist_manager::activeplaylist_set_selection(const bit_array & affected,const bit_array & status)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_set_selection(playlist,affected,status);
}

bool playlist_manager::activeplaylist_remove_items(const bit_array & mask)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_remove_items(playlist,mask);
	else return false;
}

bool playlist_manager::activeplaylist_replace_item(unsigned p_item,const metadb_handle_ptr & p_new_item)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_replace_item(playlist,p_item,p_new_item);
	else return false;
}

void playlist_manager::activeplaylist_set_focus_item(unsigned p_item)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_set_focus_item(playlist,p_item);
}

unsigned playlist_manager::activeplaylist_insert_items(unsigned p_base,const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_insert_items(playlist,p_base,data,p_selection);
	else return infinite;
}

void playlist_manager::activeplaylist_ensure_visible(unsigned p_item)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_ensure_visible(playlist,p_item);
}

bool playlist_manager::activeplaylist_rename(const char * p_name,unsigned p_name_len)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_rename(playlist,p_name,p_name_len);
	else return false;
}

bool playlist_manager::activeplaylist_is_item_selected(unsigned p_item)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_is_item_selected(playlist,p_item);
	else return false;
}

bool playlist_manager::activeplaylist_get_item_handle(metadb_handle_ptr & p_out,unsigned p_item)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_get_item_handle(p_out,playlist,p_item);
	else return false;
}

void playlist_manager::activeplaylist_move_selection(int p_delta)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_move_selection(playlist,p_delta);
}

void playlist_manager::activeplaylist_get_selection_mask(bit_array_var & out)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_get_selection_mask(playlist,out);
}

void playlist_manager::activeplaylist_get_all_items(list_base_t<metadb_handle_ptr> & out)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_get_all_items(playlist,out);
}

void playlist_manager::activeplaylist_get_selected_items(list_base_t<metadb_handle_ptr> & out)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_get_selected_items(playlist,out);
}

bool playlist_manager::g_get(service_ptr_t<playlist_manager> & p_out)
{
	return service_enum_create_t(p_out,0);
}


bool playlist_manager::remove_playlist(unsigned idx)
{
	return remove_playlists(bit_array_one(idx));
}

bool playlist_incoming_item_filter::process_location(const char * url,list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_mask,const char * p_exclude,HWND p_parentwnd)
{
	return process_locations(list_single_ref_t<const char*>(url),out,filter,p_mask,p_exclude,p_parentwnd);
}

void playlist_manager::playlist_clear(unsigned p_playlist)
{
	playlist_remove_items(p_playlist,bit_array_true());
}

void playlist_manager::activeplaylist_clear()
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_clear(playlist);
}

bool playlist_manager::playlist_add_items(unsigned playlist,const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	return playlist_insert_items(playlist,infinite,data,p_selection) != infinite;
}

bool playlist_manager::activeplaylist_add_items(const list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_add_items(playlist,data,p_selection);
	else return false;
}

bool playlist_manager::playlist_insert_items_filter(unsigned p_playlist,unsigned p_base,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	metadb_handle_list temp;
	service_ptr_t<playlist_incoming_item_filter> api;
	if (!playlist_incoming_item_filter::g_get(api)) return false;
	if (!api->filter_items(p_data,temp))
		return false;
	return playlist_insert_items(p_playlist,p_base,temp,bit_array_val(p_select)) != infinite;
}

bool playlist_manager::activeplaylist_insert_items_filter(unsigned p_base,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_insert_items_filter(playlist,p_base,p_data,p_select);
	else return false;
}

bool playlist_manager::playlist_insert_locations(unsigned p_playlist,unsigned p_base,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd)
{
	metadb_handle_list temp;
	service_ptr_t<playlist_incoming_item_filter> api;
	if (!playlist_incoming_item_filter::g_get(api)) return false;
	if (!api->process_locations(p_urls,temp,true,0,0,p_parentwnd)) return false;
	return playlist_insert_items(p_playlist,p_base,temp,bit_array_val(p_select)) != infinite;
}

bool playlist_manager::activeplaylist_insert_locations(unsigned p_base,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_insert_locations(playlist,p_base,p_urls,p_select,p_parentwnd);
	else return false;
}

bool playlist_manager::playlist_add_items_filter(unsigned p_playlist,const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	return playlist_insert_items_filter(p_playlist,infinite,p_data,p_select);
}

bool playlist_manager::activeplaylist_add_items_filter(const list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	return activeplaylist_insert_items_filter(infinite,p_data,p_select);
}

bool playlist_manager::playlist_add_locations(unsigned p_playlist,const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd)
{
	return playlist_insert_locations(p_playlist,infinite,p_urls,p_select,p_parentwnd);
}
bool playlist_manager::activeplaylist_add_locations(const list_base_const_t<const char*> & p_urls,bool p_select,HWND p_parentwnd)
{
	return activeplaylist_insert_locations(infinite,p_urls,p_select,p_parentwnd);
}

void playlist_manager::reset_playing_playlist()
{
	set_playing_playlist(get_active_playlist());
}

void playlist_manager::playlist_clear_selection(unsigned p_playlist)
{
	playlist_set_selection(p_playlist,bit_array_true(),bit_array_false());
}

void playlist_manager::activeplaylist_clear_selection()
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_clear_selection(playlist);
}

void playlist_manager::activeplaylist_undo_backup()
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_undo_backup(playlist);
}

bool playlist_manager::activeplaylist_undo_restore()
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_undo_restore(playlist);
	else return false;
}

bool playlist_manager::activeplaylist_redo_restore()
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_redo_restore(playlist);
	else return false;
}

void playlist_manager::playlist_remove_selection(unsigned p_playlist,bool p_crop)
{
	bit_array_bittable table(playlist_get_item_count(p_playlist));
	playlist_get_selection_mask(p_playlist,table);
	if (p_crop) playlist_remove_items(p_playlist,bit_array_not(table));
	else playlist_remove_items(p_playlist,table);
}

void playlist_manager::activeplaylist_remove_selection(bool p_crop)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_remove_selection(playlist,p_crop);
}

void playlist_manager::activeplaylist_item_format_title(unsigned p_item,titleformat_hook * p_hook,string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter)
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) out = "[playlist index out of range]";
	else playlist_item_format_title(playlist,p_item,p_hook,out,p_script,p_filter);
}

void playlist_manager::playlist_set_selection_single(unsigned p_playlist,unsigned p_item,bool p_state)
{
	playlist_set_selection(p_playlist,bit_array_one(p_item),bit_array_val(p_state));
}

void playlist_manager::activeplaylist_set_selection_single(unsigned p_item,bool p_state)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_set_selection_single(playlist,p_item,p_state);
}

unsigned playlist_manager::playlist_get_selection_count(unsigned p_playlist,unsigned p_max)
{
	enum_items_callback_count_selection callback(p_max);
	playlist_enum_items(p_playlist,&callback,bit_array_true());
	return callback.get_count();
}

unsigned playlist_manager::activeplaylist_get_selection_count(unsigned p_max)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_get_selection_count(playlist,p_max);
	else return 0;
}

bool playlist_manager::playlist_get_focus_item_handle(metadb_handle_ptr & p_out,unsigned p_playlist)
{
	unsigned index = playlist_get_focus_item(p_playlist);
	if (index == infinite) return false;
	return playlist_get_item_handle(p_out,p_playlist,index);
}

bool playlist_manager::activeplaylist_get_focus_item_handle(metadb_handle_ptr & p_out)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_get_focus_item_handle(p_out,playlist);
	else return false;
}

unsigned playlist_manager::find_playlist(const char * p_name,unsigned p_name_length)
{
	unsigned n, m = get_playlist_count();
	string8_fastalloc temp;
	for(n=0;n<m;n++)
	{
		if (!playlist_get_name(n,temp)) break;
		if (!stricmp_utf8_ex(temp,temp.length(),p_name,p_name_length)) return n;
	}
	return infinite;
}

unsigned playlist_manager::find_or_create_playlist(const char * p_name,unsigned p_name_length)
{
	unsigned index = find_playlist(p_name,p_name_length);
	if (index != infinite) return index;
	return create_playlist(p_name,p_name_length,infinite);
}

bool playlist_manager::activeplaylist_sort_by_format(const char * spec,bool p_sel_only)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) return playlist_sort_by_format(playlist,spec,p_sel_only);
	else return false;
}

bool playlist_manager::highlight_playing_item()
{
	unsigned playlist,item;
	if (!get_playing_item_location(&playlist,&item)) return false;
	set_active_playlist(playlist);
	playlist_set_focus_item(playlist,item);
	playlist_set_selection(playlist,bit_array_true(),bit_array_one(item));
	playlist_ensure_visible(playlist,item);
	return true;
}

void playlist_manager::playlist_get_items(unsigned p_playlist,list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask)
{
	playlist_enum_items(p_playlist,&enum_items_callback_retrieve_all_items(out),p_mask);
}

void playlist_manager::activeplaylist_get_items(list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask)
{
	unsigned playlist = get_active_playlist();
	if (playlist != infinite) playlist_get_items(playlist,out,p_mask);
}

void playlist_manager::active_playlist_fix()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite)
	{
		unsigned max = get_playlist_count();
		if (max == 0)
		{
			create_playlist("New playlist",infinite,infinite);
		}
		set_active_playlist(0);
	}
}

namespace {
	class enum_items_callback_remove_list : public playlist_manager::enum_items_callback
	{
		const metadb_handle_list & m_data;
		bit_array_var & m_table;
		unsigned m_found;
	public:
		enum_items_callback_remove_list(const metadb_handle_list & p_data,bit_array_var & p_table) : m_data(p_data), m_table(p_table), m_found(0) {}
		bool on_item(unsigned p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			bool found = m_data.bsearch_by_pointer(p_location) != infinite;
			m_table.set(p_index,found);
			if (found) m_found++;
			return true;
		}
		
		inline unsigned get_found() const {return m_found;}
	};
}

void playlist_manager::remove_items_from_all_playlists(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	unsigned playlist_num, playlist_max = get_playlist_count();
	if (playlist_max != infinite)
	{
		metadb_handle_list temp;
		temp.add_items(p_data);
		temp.sort_by_pointer();
		for(playlist_num = 0; playlist_num < playlist_max; playlist_num++ )
		{
			unsigned playlist_item_count = playlist_get_item_count(playlist_num);
			if (playlist_item_count == infinite) break;
			bit_array_bittable table(playlist_item_count);
			enum_items_callback_remove_list callback(temp,table);
			playlist_enum_items(playlist_num,&callback,bit_array_true());
			if (callback.get_found()>0)
				playlist_remove_items(playlist_num,table);
		}
	}
}

bool playlist_manager::get_all_items(list_base_t<metadb_handle_ptr> & out)
{
	unsigned n, m = get_playlist_count();
	if (m == infinite) return false;
	enum_items_callback_retrieve_all_items callback(out);
	for(n=0;n<m;n++)
	{
		playlist_enum_items(n,&callback,bit_array_true());
	}
	return true;
}

t_uint32 playlist_manager::activeplaylist_lock_get_filter_mask()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return ~0;
	else return playlist_lock_get_filter_mask(playlist);
}

bool playlist_manager::activeplaylist_is_undo_available()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return false;
	else return playlist_is_undo_available(playlist);
}

bool playlist_manager::activeplaylist_is_redo_available()
{
	unsigned playlist = get_active_playlist();
	if (playlist == infinite) return false;
	else return playlist_is_redo_available(playlist);
}

bool playlist_manager::remove_playlist_switch(unsigned idx)
{
	bool need_switch = get_active_playlist() == idx;
	if (remove_playlist(idx))
	{
		if (need_switch)
		{
			unsigned total = get_playlist_count();
			if (total > 0)
			{
				if (idx >= total) idx = total-1;
				set_active_playlist(idx);
			}
		}
		return true;
	}
	else return false;
}