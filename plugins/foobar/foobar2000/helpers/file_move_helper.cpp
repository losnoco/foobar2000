#include "stdafx.h"

static bool grab_items_by_path(list_base_t<metadb_handle_ptr> & p_out,const char * p_path,abort_callback & p_abort)
{
	try {
		string8 path;
		filesystem::g_get_canonical_path(p_path,path);
		p_out.remove_all();
		service_ptr_t<input_info_reader> reader;
		exception_io::g_test( input_entry::g_open_for_info_read(reader,0,path,p_abort) );

		static_api_ptr_t<metadb> l_metadb;
		
		const unsigned count = reader->get_subsong_count_e();
		for(unsigned n=0;n<count;n++) {
			p_abort.check_e();;
			metadb_handle_ptr ptr;
			l_metadb->handle_create_e(ptr,make_playable_location(path,reader->get_subsong_e(n)));
			p_out.add_item_e(ptr);
		}

		return p_out.get_count() > 0;
	} catch(std::exception const &) {return false;}
}

file_move_helper::file_move_helper() {}
file_move_helper::~file_move_helper() {}
file_move_helper::file_move_helper(const file_move_helper & p_src) {*this = p_src;}

bool file_move_helper::take_snapshot(const char * p_path,abort_callback & p_abort)
{
	in_metadb_sync blah;

	m_source_handles.remove_all();
	m_data.set_size(0);
	if (!grab_items_by_path(m_source_handles,p_path,p_abort)) return false;
	unsigned n, m = m_source_handles.get_count();
	m_data.set_size(m);	
	for(n=0;n<m;n++)
	{
		m_data[n].m_location = m_source_handles[n]->get_location();
		m_data[n].m_have_info = m_source_handles[n]->get_info(m_data[n].m_info);
		m_data[n].m_stats = m_source_handles[n]->get_filestats();
		
	}
	return true;
}

bool file_move_helper::on_moved(const char * p_path,abort_callback & p_abort)
{
	bool ret;
	file_move_callback_manager cb;
	ret = on_moved(p_path,p_abort,cb);
	cb.run_callback();
	return ret;
}

bool file_move_helper::on_copied(const char * p_path,abort_callback & p_abort)
{
	bool ret;
	file_move_callback_manager cb;
	ret = on_copied(p_path,p_abort,cb);
	cb.run_callback();
	return ret;
}

bool file_move_helper::on_moved(const char * p_path,abort_callback & p_abort,file_move_callback_manager & p_cb)
{
	file_path_canonical path(p_path);
	if (m_data.get_size() == 0) return false;
	if (!metadb::path_compare(path,get_source_path())) return true;

	metadb_handle_list items;
	if (!make_new_item_list(items,path,p_cb)) return false;
	

	/*if (p_update_db)*/
	p_cb.on_library_add_items(items);
	p_cb.on_library_remove_items(m_source_handles);

	p_cb.on_moved(list_single_ref_t<const char*>(get_source_path()),list_single_ref_t<const char*>(path));

	return true;
}

bool file_move_helper::on_copied(const char * p_path,abort_callback & p_abort,file_move_callback_manager & p_cb)
{
	file_path_canonical path(p_path);
	if (m_data.get_size() == 0) return false;
	if (!metadb::path_compare(path,get_source_path())) return true;
	metadb_handle_list items;
	if (!make_new_item_list(items,path,p_cb)) return false;
	/*if (p_update_db)*/ p_cb.on_library_add_items(items);
	p_cb.on_copied(list_single_ref_t<const char*>(get_source_path()),list_single_ref_t<const char*>(path));
	return true;
}

bool file_move_helper::g_on_deleted(const list_base_const_t<const char *> & p_files)
{
	file_operation_callback::g_on_files_deleted(p_files);
	return true;
}

bool file_move_helper::make_new_item_list(list_base_t<metadb_handle_ptr> & p_out,const char * p_new_path,file_move_callback_manager & p_cb)
{
	string8 new_path;
	filesystem::g_get_canonical_path(p_new_path,new_path);

	unsigned n; const unsigned m = m_data.get_size();
	static_api_ptr_t<metadb> api;
	array_t<metadb_handle_ptr> hint_handles(m);
	array_t<const file_info*> hint_infos(m);
	array_t<t_filestats> hint_stats(m);
	unsigned hintptr = 0;
	for(n=0;n<m;n++)
	{
		metadb_handle_ptr temp;
		if (!api->handle_create(temp,make_playable_location(new_path,m_data[n].m_location.get_subsong()))) return false;
		if (m_data[n].m_have_info)
		{
			hint_handles[hintptr] = temp;
			hint_infos[hintptr] = &m_data[n].m_info;
			hint_stats[hintptr] = m_data[n].m_stats;

			hintptr++;
		}
		p_out.add_item(temp);
	}

	if (hintptr > 0)
	{
		p_cb.on_hint(
			list_const_array_t<metadb_handle_ptr,const array_t<metadb_handle_ptr> &>(hint_handles,hintptr),
			list_const_array_t<const file_info *,const array_t<const file_info *> &>(hint_infos,hintptr),
			list_const_array_t<t_filestats,const array_t<t_filestats> &>(hint_stats,hintptr)
			);
/*
		static_api_ptr_t<metadb_io>()->hint_multi(
			list_const_array_t<metadb_handle_ptr,const array_t<metadb_handle_ptr> &>(hint_handles,hintptr),
			list_const_array_t<const file_info *,const array_t<const file_info *> &>(hint_infos,hintptr),
			list_const_array_t<t_filestats,const array_t<t_filestats> &>(hint_stats,hintptr),
			bit_array_false());*/
	}

	return true;
}

const char * file_move_helper::get_source_path() const
{
	return m_data.get_size() > 0 ? m_data[0].m_location.get_path() : 0;
}

unsigned file_move_helper::g_filter_dead_files_sorted_make_mask(list_base_t<metadb_handle_ptr> & p_data,const list_base_const_t<const char*> & p_dead,bit_array_var & p_mask)
{
	unsigned n, m = p_data.get_count();
	unsigned found = 0;
	for(n=0;n<m;n++)
	{
		unsigned dummy;
		bool dead = p_dead.bsearch_t(metadb::path_compare,p_data.get_item(n)->get_path(),dummy);
		if (dead) found++;
		p_mask.set(n,dead);
	}
	return found;
}

unsigned file_move_helper::g_filter_dead_files_sorted(list_base_t<metadb_handle_ptr> & p_data,const list_base_const_t<const char*> & p_dead)
{
	bit_array_bittable mask(p_data.get_count());
	unsigned found = g_filter_dead_files_sorted_make_mask(p_data,p_dead,mask);
	if (found > 0) p_data.remove_mask(mask);
	return found;
}

unsigned file_move_helper::g_filter_dead_files(list_base_t<metadb_handle_ptr> & p_data,const list_base_const_t<const char*> & p_dead)
{
	ptr_list_t<const char> temp;
	temp.add_items(p_dead);
	temp.sort_t(metadb::path_compare);
	return g_filter_dead_files_sorted(p_data,temp);
}


void file_move_callback_manager::on_library_add_items(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	m_added.add_items(p_data);
}

void file_move_callback_manager::on_library_remove_items(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	m_removed.add_items(p_data);
}

void file_move_callback_manager::on_moved(const string_list_const & p_from,const string_list_const & p_to)
{
	assert(p_from.get_count() == p_to.get_count());
	m_move_from += p_from;
	m_move_to += p_to;
}

void file_move_callback_manager::on_copied(const string_list_const & p_from,const string_list_const & p_to)
{
	assert(p_from.get_count() == p_to.get_count());
	m_copy_from += p_from;
	m_copy_to += p_to;
}

void file_move_callback_manager::on_hint(const list_base_const_t<metadb_handle_ptr> & p_list,const list_base_const_t<const file_info*> & p_infos,const list_base_const_t<t_filestats> & p_stats)
{
	m_hint_handles.add_items(p_list);
	m_hint_stats.add_items(p_stats);

	{
		unsigned old_count = m_hint_infos.get_count(), delta = p_infos.get_count();
		m_hint_infos.set_count(old_count + delta);
		for(unsigned n=0;n<delta;n++)
			m_hint_infos[old_count+n] = *p_infos[n];
	}
	
/*	metadb_handle_list m_hint_handles;
	list_t<file_info_impl_const> m_hint_infos;
	list_t<t_filestats> m_hint_stats;*/
	
}


void file_move_callback_manager::run_callback()
{
	if (m_hint_handles.get_count() > 0)
	{
		static_api_ptr_t<metadb_io>()->hint_multi(
			m_hint_handles,
			ptr_list_const_array_t<const file_info,const list_t<file_info_const_impl> &>(m_hint_infos,m_hint_infos.get_count()),
			m_hint_stats,
			bit_array_false());

		m_hint_infos.remove_all();
		m_hint_stats.remove_all();

		//trick to make sure values don't get wiped
		//m_hint_handles.remove_all();

	}

	assert(m_copy_from.get_count() == m_copy_to.get_count());
	assert(m_move_from.get_count() == m_move_to.get_count());

	static_api_ptr_t<library_manager> api_library_manager;

	if (m_added.get_count() > 0)
	{
		api_library_manager->add_items(m_added);
		m_added.remove_all();
	}

	if (m_removed.get_count())
	{
		api_library_manager->remove_items(m_removed);
		m_removed.remove_all();
	}

	if (m_copy_from.get_count() > 0)
	{
		file_operation_callback::g_on_files_copied(m_copy_from,m_copy_to);
		m_copy_from.remove_all();
		m_copy_to.remove_all();
	}

	if (m_move_from.get_count() > 0)
	{
		file_operation_callback::g_on_files_moved(m_move_from,m_move_to);
		m_move_from.remove_all();
		m_move_to.remove_all();
	}

	//trick to make sure values don't get wiped
	m_hint_handles.remove_all();
}
