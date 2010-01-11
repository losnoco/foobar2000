namespace text_file_loader
{
	t_io_result write(const service_ptr_t<file> & p_file,abort_callback & p_abort,const char * p_string,bool is_utf8);
	t_io_result read(const service_ptr_t<file> & p_file,abort_callback & p_abort,string_base & p_out,bool & is_utf8);

	t_io_result write(const char * p_path,abort_callback & p_abort,const char * p_string,bool is_utf8);
	t_io_result read(const char * p_path,abort_callback & p_abort,string_base & p_out,bool & is_utf8);

};