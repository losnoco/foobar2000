template<bool p_seekable,bool p_writeable>
class file_win32_wrapper_t : public file
{
public:
	file_win32_wrapper_t(HANDLE p_handle) : m_handle(p_handle), m_position(0) {}

	static bool g_create(service_ptr_t<file> & p_out,HANDLE p_handle)
	{
		p_out = new service_impl_p1_t<file_win32_wrapper_t<p_seekable,p_writeable>,HANDLE>(p_handle);
		return p_out.is_valid();
	}


	t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort)
	{
		if (!p_writeable) return io_result_error_generic;
		if (p_abort.is_aborting()) return io_result_aborted;

		DWORD bytes_written = 0;
		
		SetLastError(ERROR_SUCCESS);
		
		if (!WriteFile(m_handle,p_buffer,p_bytes,&bytes_written,0)) return io_error_from_win32(GetLastError());
		
		p_bytes_written = bytes_written;
		m_position += bytes_written;

		return bytes_written == p_bytes ? io_result_success : io_result_eof;
	}
	
	t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		
		DWORD bytes_read = 0;
		
		SetLastError(ERROR_SUCCESS);
		
		if (!ReadFile(m_handle,p_buffer,p_bytes,&bytes_read,0)) return io_error_from_win32(GetLastError());
		
		p_bytes_read = bytes_read;
		m_position += bytes_read;

		return bytes_read == p_bytes ? io_result_success : io_result_eof;
	}


	t_io_result get_size(t_filesize & p_length,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;

		union {
			t_uint64 val64;
			t_uint32 val32[2];
		} u;

		u.val64 = 0;
		SetLastError(NO_ERROR);
		u.val32[0] = GetFileSize(m_handle,reinterpret_cast<DWORD*>(&u.val32[1]));
		if (GetLastError()!=NO_ERROR) return io_error_from_win32(GetLastError());
		p_length = u.val64;
		return io_result_success;
	}

	t_io_result get_position(t_uint64 & p_position,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		p_position = m_position;
		return io_result_success;
	}
	
	t_io_result set_eof(abort_callback & p_abort)
	{
		if (!p_writeable) return io_result_error_generic;
		if (p_abort.is_aborting()) return io_result_aborted;

		SetLastError(ERROR_SUCCESS);
		return SetEndOfFile(m_handle) ? io_result_success : io_error_from_win32(GetLastError());
	}

	t_io_result seek(t_filesize p_position,abort_callback & p_abort)
	{
		return seek2((t_int64)p_position,FILE_BEGIN,p_abort);
	}

	t_io_result seek2(t_int64 p_position,int p_mode,abort_callback & p_abort)
	{
		if (!p_seekable) return io_result_error_generic;
		if (p_abort.is_aborting()) return io_result_aborted;

		union 
		{
			t_int64 temp64;
			struct
			{
				DWORD temp_lo;
				LONG temp_hi;
			};

		};

		temp64 = p_position;

		SetLastError(ERROR_SUCCESS);
		
		temp_lo = SetFilePointer(m_handle,temp_lo,&temp_hi,p_mode);

		if (GetLastError() != ERROR_SUCCESS) return io_error_from_win32(GetLastError());

        m_position = (t_filesize) temp64;

		return io_result_success;

	}

	bool can_seek() {return p_seekable;}
	bool get_content_type(string_base & out) {return false;}
	bool is_in_memory() {return false;}
	void on_idle(abort_callback & p_abort) {}
	
	t_io_result get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort)
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		FlushFileBuffers(m_handle);
		SetLastError(ERROR_SUCCESS);
		if (!GetFileTime(m_handle,0,0,(FILETIME*)&p_timestamp)) return io_error_from_win32(GetLastError());
		return io_result_success;
	}

	bool is_remote() {return false;}

	~file_win32_wrapper_t() {CloseHandle(m_handle);}
private:
	HANDLE m_handle;
	t_uint64 m_position;
};
