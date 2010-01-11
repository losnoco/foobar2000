#include "foobar2000.h"


bool metadb::g_get(service_ptr_t<metadb> & p_out)
{
	return service_enum_create_t(p_out,0);
}



bool metadb::handle_create_replace_path_canonical(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path)
{
	return handle_create(p_out,make_playable_location(p_new_path,p_source->get_subsong_index()));
}

bool metadb::handle_create_replace_path(metadb_handle_ptr & p_out,const metadb_handle_ptr & p_source,const char * p_new_path)
{
	string8 path;
	filesystem::g_get_canonical_path(p_new_path,path);
	return handle_create_replace_path_canonical(p_out,p_source,path);
}

bool metadb::handle_replace_path_canonical(metadb_handle_ptr & p_out,const char * p_new_path)
{
	metadb_handle_ptr blah(p_out);
	return handle_create_replace_path_canonical(p_out,blah,p_new_path);
}


t_io_result metadb_io::load_info(metadb_handle_ptr p_item,t_load_info_type p_type,HWND p_parent_window,bool p_show_errors)
{
	t_io_result retcode;
	if (!load_info_multi(list_single_ref_t<metadb_handle_ptr>(p_item),p_type,p_parent_window,p_show_errors,&retcode))
		return io_result_aborted;
	else
		return retcode;
}

t_io_result metadb_io::update_info(metadb_handle_ptr p_item,file_info & p_info,HWND p_parent_window,bool p_show_errors)
{
	t_io_result retcode;
	file_info * blah = &p_info;
	if (!update_info_multi(list_single_ref_t<metadb_handle_ptr>(p_item),list_single_ref_t<file_info*>(blah),p_parent_window,p_show_errors,&retcode))
		return io_result_aborted;
	else
		return retcode;
}

file_info_update_helper::file_info_update_helper(metadb_handle_ptr p_item)
{
	const unsigned count = 1;
	m_data.add_item(p_item);

	m_infos.set_size(count);
	m_mask.set_size(count);
	unsigned n;
	for(n=0;n<count;n++) m_mask[n] = false;
}

file_info_update_helper::file_info_update_helper(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	const unsigned count = p_data.get_count();
	m_data.add_items(p_data);

	m_infos.set_size(count);
	m_mask.set_size(count);
	unsigned n;
	for(n=0;n<count;n++) m_mask[n] = false;
}

bool file_info_update_helper::read_infos(HWND p_parent,bool p_show_errors)
{
	static_api_ptr_t<metadb_io> api;
	if (!api->load_info_multi(m_data,metadb_io::load_info_default,p_parent,p_show_errors,0)) return false;
	unsigned n; const unsigned m = m_data.get_count();
	unsigned loaded_count = 0;
	for(n=0;n<m;n++)
	{
		bool val = api->pending_query_ex(m_data[n],m_infos[n]);
		if (val) loaded_count++;
		m_mask[n] = val;
	}
	return loaded_count == m;
}

file_info_update_helper::t_write_result file_info_update_helper::write_infos(HWND p_parent,bool p_show_errors)
{
	unsigned n, outptr = 0; const unsigned count = m_data.get_count();
	array_t<metadb_handle_ptr> items_to_update(count);
	array_t<file_info *> infos_to_write(count);
	
	for(n=0;n<count;n++)
	{
		if (m_mask[n])
		{
			items_to_update[outptr] = m_data[n];
			infos_to_write[outptr] = &m_infos[n];
			outptr++;
		}
	}

	if (outptr == 0) return write_ok;
	else
	{
		array_t<t_io_result> retcodes(outptr);

		static_api_ptr_t<metadb_io> api;
		if (!api->update_info_multi(
			list_const_array_t<metadb_handle_ptr,const array_t<metadb_handle_ptr>&>(items_to_update,outptr),
			list_const_array_t<file_info*,const array_t<file_info*>&>(infos_to_write,outptr),
			p_parent,
			true,
			retcodes.get_ptr())) return write_aborted;

		bool got_errors = false, got_busy = false;
		bool pending_enabled = api->pending_is_enabled();
		for(n=0;n<outptr;n++)
		{
			t_io_result code = retcodes[n];
			if (io_result_failed(code))
			{
				if (code == io_result_error_sharing_violation && pending_enabled) got_busy = true;
				else got_errors = true;
			}
		}

		if (got_errors) return write_error;
		else if (got_busy) return write_busy;
		else return write_ok;
	}
}

unsigned file_info_update_helper::get_item_count() const
{
	return m_data.get_count();
}

bool file_info_update_helper::is_item_valid(unsigned p_index) const
{
	return m_mask[p_index];
}
	
file_info & file_info_update_helper::get_item(unsigned p_index)
{
	return m_infos[p_index];
}

metadb_handle_ptr file_info_update_helper::get_item_handle(unsigned p_index) const
{
	return m_data[p_index];
}

void file_info_update_helper::invalidate_item(unsigned p_index)
{
	m_mask[p_index] = false;
}


void metadb_io::hint_async(metadb_handle_ptr p_item,const file_info & p_info,const t_filestats & p_stats,bool p_fresh)
{
	const file_info * blargh = &p_info;
	hint_multi_async(list_single_ref_t<metadb_handle_ptr>(p_item),list_single_ref_t<const file_info *>(blargh),list_single_ref_t<t_filestats>(p_stats),bit_array_val(p_fresh));
}


bool metadb_io::pending_query_ex(metadb_handle_ptr p_item,file_info & p_out)
{
	if (pending_query(p_item,p_out)) return true;
	return p_item->get_info(p_out);
}