#include "stdafx.h"

class input_opus : public input_entry
{
private:
	service_ptr_t<input_entry> m_instance;

	inline void init_instance()
	{
		if ( m_instance.is_empty() )
		{
			if ( !input_entry::g_find_service_by_content_type( m_instance, "application/ogg" ) ) throw exception_io_data();
		}
	}

public:
	bool is_our_content_type(const char * p_type) { return false; }
	bool is_our_path(const char * p_full_path,const char * p_extension) { return stricmp_utf8( p_extension, "opus" ) == 0; }
	
	void open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		init_instance();
		m_instance->open_for_decoding( p_instance, p_filehint, p_path, p_abort );
	}

	void open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		init_instance();
		m_instance->open_for_info_read( p_instance, p_filehint, p_path, p_abort );
	}

	void open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) {
		init_instance();
		m_instance->open_for_info_write( p_instance, p_filehint, p_path, p_abort );
	}

	void get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort) {
		init_instance();
		m_instance->get_extended_data( p_filehint, p_location, p_guid, p_out, p_abort );
	}

	unsigned get_flags()
	{
		init_instance();
		return m_instance->get_flags();
	}
};

static service_factory_single_t< input_opus > g_service_factory_input_opus;

DECLARE_FILE_TYPE("Ogg Opus files", "*.OPUS");
