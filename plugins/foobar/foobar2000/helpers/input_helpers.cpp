#include "stdafx.h"

t_io_result input_helper::open(metadb_handle_ptr p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints)
{
	return open(p_location->get_location(),p_flags,p_abort,p_from_redirect,p_skip_hints);
}

static t_io_result process_fullbuffer(service_ptr_t<file> & p_file,const char * p_path,t_filesize p_fullbuffer,abort_callback & p_abort)
{
	t_io_result status;

	
	if (p_fullbuffer > 0)
	{
		if (p_file.is_empty())
		{
			service_ptr_t<filesystem> fs;
			if (filesystem::g_get_interface(fs,p_path))
			{
				status = fs->open(p_file,p_path,filesystem::open_mode_read,p_abort);
				if (io_result_failed(status)) return status;
			}
		}

		if (p_file.is_valid())
		{
			t_filesize size;
			status = p_file->get_size(size,p_abort);
			if (io_result_failed(status)) return status;
			if (size != filesize_invalid && size <= p_fullbuffer)
			{
				try {
					service_ptr_t<file> l_file_buffered;
					if (reader_membuffer_mirror::g_create_e(l_file_buffered,p_file,p_abort))
					{
						p_file = l_file_buffered;
					}
				}
				catch(t_io_result code) {return code;}
			}
		}
	}
	
	return io_result_success;
}

t_io_result input_helper::open(const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect,bool p_skip_hints)
{
	t_io_result status;

	if (p_abort.is_aborting()) return io_result_aborted;


	if (m_input.is_empty() || metadb::path_compare(p_location.get_path(),m_path) != 0)
	{
		service_ptr_t<file> l_file;

		status = process_fullbuffer(l_file,p_location.get_path(),m_fullbuffer,p_abort);
		if (io_result_failed(status)) return status;

		status = input_entry::g_open_for_decoding(m_input,l_file,p_location.get_path(),p_abort,p_from_redirect);
		if (io_result_failed(status)) return status;

		m_path = p_location.get_path();

		if (p_abort.is_aborting()) return io_result_aborted;

		if (!p_skip_hints) static_api_ptr_t<metadb_io>()->hint_reader(m_input.get_ptr(),p_location.get_path(),p_abort);
	}

	if (p_abort.is_aborting()) return io_result_aborted;

	status = m_input->initialize(p_location.get_subsong_index(),p_flags,p_abort);
	if (io_result_failed(status)) return status;

	return io_result_success;
}


void input_helper::close()
{
	m_input.release();
}

bool input_helper::is_open()
{
	return m_input.is_valid();
}

t_io_result input_helper::run(audio_chunk & p_chunk,abort_callback & p_abort)
{
	if (m_input.is_valid())
		return m_input->run(p_chunk,p_abort);
	else
		return io_result_error_data;
}

t_io_result input_helper::seek(double seconds,abort_callback & p_abort)
{
	if (m_input.is_valid())
		return m_input->seek(seconds,p_abort);
	else
		return io_result_error_data;
}

bool input_helper::can_seek()
{
	return m_input.is_valid() ? m_input->can_seek() : false;
}

void input_helper::set_full_buffer(t_filesize val)
{
	m_fullbuffer = val;
}
void input_helper::on_idle(abort_callback & p_abort)
{
	if (m_input.is_valid()) m_input->on_idle(p_abort);
}

bool input_helper::get_dynamic_info(file_info & p_out,double & p_timestamp_delta,bool & p_track_change)
{
	if (m_input.is_valid())
		return m_input->get_dynamic_info(p_out,p_timestamp_delta,p_track_change);
	else
		return false;
}

t_io_result input_helper::get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort)
{
	if (m_input.is_valid())
	{
		return m_input->get_info(p_subsong,p_info,p_abort);
	}
	else return io_result_error_data;
}

const char * input_helper::get_path() const
{
	return m_path;
}


input_helper::input_helper() : m_fullbuffer(0)
{
}


t_io_result input_helper::g_get_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect)
{
	t_io_result status;
	service_ptr_t<input_info_reader> instance;
	status = input_entry::g_open_for_info_read(instance,0,p_location.get_path(),p_abort,p_from_redirect);
	if (io_result_failed(status)) return status;
	status = instance->get_info(p_location.get_subsong_index(),p_info,p_abort);
	if (io_result_failed(status)) return status;
	return io_result_success;
}

t_io_result input_helper::g_set_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect)
{
	t_io_result status;
	service_ptr_t<input_info_writer> instance;
	status = input_entry::g_open_for_info_write(instance,0,p_location.get_path(),p_abort,p_from_redirect);
	if (io_result_failed(status)) return status;
	status = instance->set_info(p_location.get_subsong_index(),p_info,p_abort);
	if (io_result_failed(status)) return status;
	status = instance->commit(p_abort);
	if (io_result_failed(status)) return status;
	return io_result_success;
}


bool dead_item_filter::run(const list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask)
{
	file_list_helper::file_list_from_metadb_handle_list path_list;
	path_list.init_from_list(p_list);
	metadb_handle_list valid_handles;
	for(unsigned pathidx=0;pathidx<path_list.get_count();pathidx++)
	{
		if (is_aborting()) return false;
		on_progress(pathidx,path_list.get_count());
		metadb_handle_list temp;
		t_filestats stats = filestats_invalid;
		if (io_result_succeeded(track_indexer::g_get_tracks_simple(path_list[pathidx],0,stats,temp,*this)))
			valid_handles.add_items(temp);
	}

	valid_handles.sort_by_pointer();
	for(unsigned listidx=0;listidx<p_list.get_count();listidx++)
	{
		p_mask.set(listidx,valid_handles.bsearch_by_pointer(p_list[listidx]) == infinite);
	}
	return !is_aborting();
}

namespace {

class dead_item_filter_impl_simple : public dead_item_filter
{
public:
	inline dead_item_filter_impl_simple(abort_callback & p_abort) : m_abort(p_abort) {}
	bool is_aborting() {return m_abort.is_aborting();}
	void on_progress(unsigned p_position,unsigned p_total) {}
private:
	abort_callback & m_abort;
};

}

bool input_helper::g_mark_dead(const list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask,abort_callback & p_abort)
{
	dead_item_filter_impl_simple filter(p_abort);
	return filter.run(p_list,p_mask);
}

t_io_result input_info_read_helper::open(const char * p_path,abort_callback & p_abort)
{
	//we're likely called repeatedly with same path. if something fails to open once, don't bother trying again, in case it's slow or something.
	if (metadb::path_compare(m_path,p_path) != 0)
	{
		m_path = p_path;

		m_open_status = input_entry::g_open_for_info_read(m_input,0,p_path,p_abort);
	}
	return m_open_status;
}

t_io_result input_info_read_helper::get_info(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,abort_callback & p_abort)
{
	t_io_result status;

	status = open(p_location.get_path(),p_abort);
	if (io_result_failed(status)) return status;

	status = m_input->get_file_stats(p_stats,p_abort);
	if (io_result_failed(status)) return status;

	status = m_input->get_info(p_location.get_subsong_index(),p_info,p_abort);
	if (io_result_failed(status)) return status;

	return io_result_success;
}

t_io_result input_info_read_helper::get_info_check(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,bool & p_reloaded,abort_callback & p_abort)
{
	t_io_result status;
	
	status = open(p_location.get_path(),p_abort);
	if (io_result_failed(status)) return status;

	t_filestats newstats;
	status = m_input->get_file_stats(newstats,p_abort);
	if (io_result_failed(status)) return status;

	if (metadb_handle::g_should_reload(p_stats,newstats,true))
	{
		p_stats = newstats;
		status = m_input->get_info(p_location.get_subsong_index(),p_info,p_abort);
		if (io_result_failed(status)) return status;
		p_reloaded = true;
	}
	else
	{
		p_reloaded = false;
	}

	return io_result_success;
}
