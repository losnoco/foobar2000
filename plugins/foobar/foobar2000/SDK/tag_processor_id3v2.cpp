#include "foobar2000.h"

bool tag_processor_id3v2::g_get(service_ptr_t<tag_processor_id3v2> & p_out)
{
	return service_enum_t<tag_processor_id3v2>().first(p_out);
}

t_io_result tag_processor_id3v2::g_remove(const service_ptr_t<file> & p_file,t_uint64 & p_size_removed,abort_callback & p_abort)
{
	if (!p_file->can_seek()) return io_result_error_generic;
	t_io_result io_status;


	t_filesize len;
	
	io_status = p_file->get_size(len,p_abort);
	if (io_result_failed(io_status)) return io_status;

	if (len == filesize_invalid) return io_result_error_generic;
	
	io_status = p_file->seek(0,p_abort);
	if (io_result_failed(io_status)) return io_status;
	
	t_uint64 offset;
	io_status = g_skip(p_file,offset,p_abort);
	if (io_result_failed(io_status)) return io_status;

	if (offset>0 && offset<len)
	{
		len-=offset;
		service_ptr_t<file> temp;
		if (len > 16*1024*1024) io_status = filesystem::g_open_temp(temp,p_abort);
		else io_status = filesystem::g_open_tempmem(temp,p_abort);
		if (io_result_failed(io_status)) {console::info("error creating temp file for id3v2 remover");return io_status;}
		io_status = file::g_transfer_object(p_file.get_ptr(),temp.get_ptr(),len,p_abort);
		if (io_result_failed(io_status)) return io_status;
		io_status = p_file->seek(0,p_abort);
		if (io_result_failed(io_status)) return io_status;
		io_status = p_file->set_eof(p_abort);
		if (io_result_failed(io_status)) return io_status;
		io_status = temp->seek(0,p_abort);
		if (io_result_failed(io_status)) return io_status;
		io_status = file::g_transfer_object(temp.get_ptr(),p_file.get_ptr(),len,p_abort);
		if (io_result_failed(io_status)) return io_status;
	}
	p_size_removed = offset;
	return io_result_success;
}

t_io_result tag_processor_id3v2::g_skip(const service_ptr_t<file> & p_file,t_uint64 & p_size_skipped,abort_callback & p_abort)
{
	t_io_result status;
	
	unsigned char  tmp[10];

	unsigned io_bytes_done;

	status = p_file->seek ( 0, p_abort );
	if (io_result_failed(status)) return status;

	status = p_file->read( tmp, sizeof(tmp), io_bytes_done, p_abort);
	if (io_result_failed(status)) return status;
	if (io_bytes_done != sizeof(tmp)) 
	{
		status = p_file->seek ( 0, p_abort );
		if (io_result_failed(status)) return status;
		p_size_skipped = 0;
		return io_result_success;
	}

	// check id3-tag
	if ( 0 != memcmp ( tmp, "ID3", 3) ) {
		status = p_file->seek ( 0, p_abort );
		if (io_result_failed(status)) return status;
		p_size_skipped = 0;
		return io_result_success;
	}

	// read flags
	int Unsynchronisation = tmp[5] & 0x80;
	int ExtHeaderPresent  = tmp[5] & 0x40;
	int ExperimentalFlag  = tmp[5] & 0x20;
	int FooterPresent     = tmp[5] & 0x10;

	if ( tmp[5] & 0x0F ) {                              // not (yet???) allowed
		status = p_file->seek ( 0, p_abort );
		if (io_result_failed(status)) return status;
		p_size_skipped = 0;
		return io_result_success;
	}

	if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 ) { // not allowed
		status = p_file->seek ( 0, p_abort );
		if (io_result_failed(status)) return status;
		p_size_skipped = 0;
		return io_result_success;
	}

	// read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
	unsigned int ret;
	ret  = tmp[6] << 21;
	ret += tmp[7] << 14;
	ret += tmp[8] <<  7;
	ret += tmp[9]      ;
	ret += 10;
	if ( FooterPresent ) ret += 10;

	status = p_file->seek ( ret, p_abort );
	if (io_result_failed(status)) return status;

	p_size_skipped = ret;

	return io_result_success;
}
