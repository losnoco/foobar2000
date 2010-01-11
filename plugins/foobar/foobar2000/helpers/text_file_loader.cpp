#include "StdAfx.h"

static const unsigned char utf8_header[3] = {0xEF,0xBB,0xBF};

namespace text_file_loader
{

	t_io_result write(const service_ptr_t<file> & p_file,abort_callback & p_abort,const char * p_string,bool is_utf8)
	{
		t_io_result status;
		status = p_file->seek(0,p_abort);
		if (io_result_failed(status)) return status;
		status = p_file->set_eof(p_abort);
		if (io_result_failed(status)) return status;
		if (is_utf8)
		{
			status = p_file->write_object(utf8_header,sizeof(utf8_header),p_abort);
			if (io_result_failed(status)) return status;
			status = p_file->write_object(p_string,strlen(p_string),p_abort);
			if (io_result_failed(status)) return status;
		}
		else
		{
			pfc::stringcvt::string_ansi_from_utf8 bah(p_string);
			status = p_file->write_object(bah,bah.length(),p_abort);
			if (io_result_failed(status)) return status;
		}
		return io_result_success;		
	}
	t_io_result read(const service_ptr_t<file> & p_file,abort_callback & p_abort,string_base & p_out,bool & is_utf8)
	{
		t_io_result status;

		p_out.reset();

		if (p_file->can_seek())
		{
			status = p_file->seek(0,p_abort);
			if (io_result_failed(status)) return status;
		}
		
		mem_block_t<char> mem;
		t_filesize size64;
		status = p_file->get_size(size64,p_abort);
		if (io_result_failed(status)) return status;
		if (size64 == filesize_invalid)//typically HTTP
		{
			string8 ansitemp;
			is_utf8 = false;
			enum {delta = 1024*64, max = 1024*512};
			char temp[3];
			unsigned done;
			status = p_file->read(temp,3,done,p_abort);
			if (io_result_failed(status)) return status;
			if (done != 3)
			{
				if (done > 0) p_out = pfc::stringcvt::string_utf8_from_ansi(temp,done);
				return io_result_success;
			}
			if (!memcmp(utf8_header,temp,3)) is_utf8 = true;
			else ansitemp.add_string(temp,3);

			if (mem.set_size(delta) == 0) return io_result_error_generic;
			
			for(;;)
			{
				status = p_file->read(mem.get_ptr(),delta,done,p_abort);
				if (io_result_failed(status)) return status;
				if (done > 0)
				{
					if (is_utf8) p_out.add_string(mem,done);
					else ansitemp.add_string(mem,done);
				}
				if (done < delta) break;
			}

			if (!is_utf8)
			{
				pfc::stringcvt::string_utf8_from_ansi cvt;
				if (!cvt.convert(ansitemp)) return io_result_error_out_of_memory;
				p_out = cvt;
			}

			return io_result_success;
		}
		else
		{
			if (size64>1024*1024*128) return io_result_error_data;//hard limit
			unsigned size = (unsigned)size64;
			if (!mem.set_size(size+1)) return io_result_error_out_of_memory;
			char * asdf = mem.get_ptr();
			status = p_file->read_object(asdf,size,p_abort);
			if (io_result_failed(status)) return status;
			asdf[size]=0;
			if (size>3 && !memcmp(utf8_header,asdf,3)) {is_utf8 = true; p_out.add_string(asdf+3); }
			else {
				is_utf8 = false;
				pfc::stringcvt::string_utf8_from_ansi cvt;
				if (!cvt.convert(asdf)) return io_result_error_out_of_memory;
				p_out = cvt;
			}
			return io_result_success;
		}
	}

	t_io_result write(const char * p_path,abort_callback & p_abort,const char * p_string,bool is_utf8)
	{
		service_ptr_t<file> f;
		t_io_result status;
		status = filesystem::g_open_write_new(f,p_path,p_abort);
		if (io_result_failed(status)) return status;
		return write(f,p_abort,p_string,is_utf8);
	}

	t_io_result read(const char * p_path,abort_callback & p_abort,string_base & p_out,bool & is_utf8)
	{
		service_ptr_t<file> f;
		t_io_result status;
		status = filesystem::g_open_read(f,p_path,p_abort);
		if (io_result_failed(status)) return status;
		return read(f,p_abort,p_out,is_utf8);
	}

}