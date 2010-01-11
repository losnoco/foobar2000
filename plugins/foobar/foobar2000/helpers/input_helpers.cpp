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
				catch(exception_io const & e) {return e.get_code();}
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

		TRACK_CODE("input_entry::g_open_for_decoding",
			status = input_entry::g_open_for_decoding(m_input,l_file,p_location.get_path(),p_abort,p_from_redirect)
			);
		if (io_result_failed(status)) return status;

		m_path = p_location.get_path();

		if (p_abort.is_aborting()) return io_result_aborted;

		if (!p_skip_hints) static_api_ptr_t<metadb_io>()->hint_reader(m_input.get_ptr(),p_location.get_path(),p_abort);
	}

	if (p_abort.is_aborting()) return io_result_aborted;

	TRACK_CODE("input_decoder::initialize",status = m_input->initialize(p_location.get_subsong_index(),p_flags,p_abort));
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
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::run",return m_input->run(p_chunk,p_abort));
	} else {
		return io_result_error_data;
	}
}

t_io_result input_helper::seek(double seconds,abort_callback & p_abort)
{
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::seek",return m_input->seek(seconds,p_abort));
	} else {
		return io_result_error_data;
	}
}

t_io_result input_helper::can_seek(bool & p_value)
{
	return m_input.is_valid() ? m_input->can_seek(p_value) : io_result_error_data;
}

void input_helper::set_full_buffer(t_filesize val)
{
	m_fullbuffer = val;
}
t_io_result input_helper::on_idle(abort_callback & p_abort)
{
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::on_idle",return m_input->on_idle(p_abort));
	} else {
		return io_result_error_data;
	}
}

t_io_result input_helper::get_dynamic_info(file_info & p_out,double & p_timestamp_delta,bool & p_changed)
{
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::get_dynamic_info",return m_input->get_dynamic_info(p_out,p_timestamp_delta,p_changed));
	} else {
		return io_result_error_data;
	}
}

t_io_result input_helper::get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta,bool & p_changed)
{
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::get_dynamic_info_track",return m_input->get_dynamic_info_track(p_out,p_timestamp_delta,p_changed));
	} else {
		return io_result_error_data;
	}
}

t_io_result input_helper::get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort)
{
	if (m_input.is_valid()) {
		TRACK_CODE("input_decoder::get_info",return m_input->get_info(p_subsong,p_info,p_abort));
	} else {
		return io_result_error_data;
	}
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
	static_api_ptr_t<metadb> l_metadb;
	for(unsigned pathidx=0;pathidx<path_list.get_count();pathidx++)
	{
		if (is_aborting()) return false;
		on_progress(pathidx,path_list.get_count());
		
		try {
			service_ptr_t<input_info_reader> reader;
			const char * path = path_list[pathidx];
			exception_io::g_test(input_entry::g_open_for_info_read(reader,0,path,*this));
			unsigned count;
			exception_io::g_test( reader->get_subsong_count(count) );
			for(unsigned n=0;n<count && !is_aborting();n++) {
				metadb_handle_ptr ptr;
				t_uint32 index;
				exception_io::g_test( reader->get_subsong(n,index) );
				if (!l_metadb->handle_create(ptr,make_playable_location(path,index))) 
					throw std::bad_alloc();
				valid_handles.add_item_e(ptr);
			}
		} catch(std::exception const &) {}
	}

	if (is_aborting()) return false;;

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
	bool FB2KAPI is_aborting() {return m_abort.is_aborting();}
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

		TRACK_CODE("input_entry::g_open_for_info_read",m_open_status = input_entry::g_open_for_info_read(m_input,0,p_path,p_abort));
	}
	return m_open_status;
}

t_io_result input_info_read_helper::get_info(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,abort_callback & p_abort)
{
	t_io_result status;

	status = open(p_location.get_path(),p_abort);
	if (io_result_failed(status)) return status;

	TRACK_CODE("input_info_reader::get_file_stats",status = m_input->get_file_stats(p_stats,p_abort));
	if (io_result_failed(status)) return status;

	TRACK_CODE("input_info_reader::get_info",status = m_input->get_info(p_location.get_subsong_index(),p_info,p_abort));
	if (io_result_failed(status)) return status;

	return io_result_success;
}

t_io_result input_info_read_helper::get_info_check(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,bool & p_reloaded,abort_callback & p_abort)
{
	t_io_result status;
	
	status = open(p_location.get_path(),p_abort);
	if (io_result_failed(status)) return status;

	t_filestats newstats;
	TRACK_CODE("input_info_reader::get_file_stats",status = m_input->get_file_stats(newstats,p_abort));
	if (io_result_failed(status)) return status;

	if (metadb_handle::g_should_reload(p_stats,newstats,true))
	{
		p_stats = newstats;
		TRACK_CODE("input_info_reader::get_info",status = m_input->get_info(p_location.get_subsong_index(),p_info,p_abort));
		if (io_result_failed(status)) return status;
		p_reloaded = true;
	}
	else
	{
		p_reloaded = false;
	}

	return io_result_success;
}






t_io_result input_helper_cue::open(const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,double p_start,double p_length)
{
	m_start = p_start;
	m_position = 0;
	m_dynamic_info_trigger = false;
	t_io_result status;
	
	status = m_input.open(p_location,p_flags,p_abort,true,true);
	if (io_result_failed(status)) return status;
	
	{
		bool seekable = false;
		status = m_input.can_seek(seekable);
		if (io_result_failed(status)) return status;
		if (!seekable) return io_result_error_data;
	}
	if (m_start > 0)
	{
		status = m_input.seek(m_start,p_abort);
		if (io_result_failed(status)) return status;
	}

	if (p_length > 0)
	{
		m_length = p_length;
	}
	else
	{
		file_info_impl temp;
		status = m_input.get_info(0,temp,p_abort);
		if (io_result_failed(status)) return status;
		double ref_length = temp.get_length();
		if (ref_length <= 0) return io_result_error_data;
		m_length = ref_length - m_start + p_length /* negative or zero */;
		if (m_length <= 0) return io_result_error_data;
	}

	return io_result_success;
}

void input_helper_cue::close() {m_input.close();}
bool input_helper_cue::is_open() {return m_input.is_open();}

t_io_result input_helper_cue::run(audio_chunk & p_chunk,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;


	if (m_length > 0)
	{
		if (m_position >= m_length) return io_result_eof;

		m_dynamic_info_trigger = true;

		t_io_result status; 
		status = m_input.run(p_chunk,p_abort);
		if (io_result_failed_or_eof(status)) return status;

		t_uint64 max = (t_uint64) audio_math::time_to_samples(m_length - m_position, p_chunk.get_sample_rate());
		if (max == 0)
		{//handle rounding accidents, this normally shouldn't trigger
			m_position = m_length;
			return io_result_eof;
		}

		unsigned samples = p_chunk.get_sample_count();
		if ((t_uint64)samples > max)
		{
			p_chunk.set_sample_count((unsigned)max);
			m_position = m_length;
		}
		else
		{
			m_position += p_chunk.get_duration();
		}
		return io_result_success;
	}
	else
	{
		t_io_result status;
		status = m_input.run(p_chunk,p_abort);
		if (io_result_failed_or_eof(status)) return status;
		m_position += p_chunk.get_duration();
		return io_result_success;
	}
}

t_io_result input_helper_cue::seek(double p_seconds,abort_callback & p_abort)
{
	m_dynamic_info_trigger = false;
	if (m_length <= 0 || p_seconds < m_length)
	{
		t_io_result status;
		status = m_input.seek(p_seconds + m_start,p_abort);
		if (io_result_failed(status)) return status;
		m_position = p_seconds;
		return io_result_success;
	}
	else
	{
		m_position = m_length;
		return io_result_success;
	}
}

bool input_helper_cue::can_seek()
{
	return true;
}
void input_helper_cue::set_full_buffer(t_filesize val) {m_input.set_full_buffer(val);}

void input_helper_cue::on_idle(abort_callback & p_abort) {m_input.on_idle(p_abort);}

bool input_helper_cue::get_dynamic_info(file_info & p_out,double & p_timestamp_delta)
{
	if (m_dynamic_info_trigger)
	{
		m_dynamic_info_trigger = false;
		bool changed = false;
		exception_io::g_test(m_input.get_dynamic_info(p_out,p_timestamp_delta,changed));
		return changed;
	}
	else return false;
}

bool input_helper_cue::get_dynamic_info_track(file_info & p_out,double & p_timestamp_delta) {
	return false;
}

const char * input_helper_cue::get_path() const {return m_input.get_path();}
	
t_io_result input_helper_cue::get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {return m_input.get_info(p_subsong,p_info,p_abort);}



bool input_helper::run_e(audio_chunk & p_chunk,abort_callback & p_abort) {
	t_io_result status = run(p_chunk,p_abort);
	exception_io::g_test(status);
	return status != io_result_eof;
}

void input_helper::seek_e(double p_seconds,abort_callback & p_abort) {
	exception_io::g_test(seek(p_seconds,p_abort));
}
	

void input_helper::get_info_e(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
	exception_io::g_test(get_info(p_subsong,p_info,p_abort));
}
