#ifndef _INPUT_HELPERS_H_
#define _INPUT_HELPERS_H_

class input_helper
{
public:
	input_helper();
	t_io_result open(metadb_handle_ptr p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect = false,bool p_skip_hints = false);
	t_io_result open(const playable_location & p_location,unsigned p_flags,abort_callback & p_abort,bool p_from_redirect = false,bool p_skip_hints = false);

	void close();
	bool is_open();
	t_io_result run(audio_chunk & p_chunk,abort_callback & p_abort);
	t_io_result seek(double seconds,abort_callback & p_abort);
	bool can_seek();
	void set_full_buffer(t_filesize val);
	void on_idle(abort_callback & p_abort);
	bool get_dynamic_info(file_info & p_out,double & p_timestamp_delta,bool & p_track_change);

	const char * get_path() const;
	
	t_io_result get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);

	static t_io_result g_get_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect = false);
	static t_io_result g_set_info(const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_from_redirect = false);


	static bool g_mark_dead(const list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask,abort_callback & p_abort);

private:
	service_ptr_t<input_decoder> m_input;
	string8 m_path;
	t_filesize m_fullbuffer;
};

class NOVTABLE dead_item_filter : public abort_callback
{
public:
	virtual void on_progress(unsigned p_position,unsigned p_total) = 0;

	bool run(const list_base_const_t<metadb_handle_ptr> & p_list,bit_array_var & p_mask);
};

class input_info_read_helper
{
public:
	input_info_read_helper() : m_open_status(io_result_error_data) {}
	t_io_result get_info(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,abort_callback & p_abort);
	t_io_result get_info_check(const playable_location & p_location,file_info & p_info,t_filestats & p_stats,bool & p_reloaded,abort_callback & p_abort);
private:
	t_io_result open(const char * p_path,abort_callback & p_abort);

	string8 m_path;
	t_io_result m_open_status;
	service_ptr_t<input_info_reader> m_input;
};

#endif