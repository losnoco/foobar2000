#include "foobar2000.h"


t_io_result unpacker::g_open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p,abort_callback & p_abort)
{
	service_enum_t<unpacker> e;
	service_ptr_t<unpacker> ptr;
	if (e.first(ptr)) do {
		t_io_result io_status;
		io_status = p->seek(0,p_abort);
		if (io_result_failed(io_status)) return io_status;
		io_status = ptr->open(p_out,p,p_abort);
		if (io_status != io_result_error_data) return io_status;
	} while(e.next(ptr));
	return io_result_error_data;

}

t_io_result file::seek2(t_int64 position,int mode,abort_callback & p_abort)
{
	switch(mode)
	{
	case SEEK_CUR:
		{
			t_filesize delta;
			t_io_result status = get_position(delta,p_abort);
			if (io_result_failed(status)) return status;
			position+=delta;
		}
		break;
	case SEEK_END:
		{
			t_filesize delta;
			t_io_result status = get_size(delta,p_abort);
			if (io_result_failed(status)) return status;
			position+=delta;
		}
		break;
	}
	return seek(position,p_abort);
}

t_io_result file::g_transfer(stream_reader * src,stream_writer * dst,t_filesize bytes,t_filesize & transferred,abort_callback & p_abort)
{
	enum {BUFSIZE = 1024*1024};
	mem_block temp;
	if (!temp.set_size((unsigned)(BUFSIZE<bytes ? BUFSIZE : bytes))) return io_result_error_out_of_memory;
	void* ptr = temp.get_ptr();
	t_io_result io_status;
	unsigned io_bytes_done;
	t_filesize done = 0;
	while(done<bytes)
	{
		if (p_abort.is_aborting()) return io_result_aborted;

		t_int64 delta64 = bytes-done;
		if (delta64>BUFSIZE) delta64 = BUFSIZE;
		unsigned delta = (unsigned)delta64;

		io_status = src->read(ptr,delta,io_bytes_done,p_abort);
		if (io_result_failed(io_status)) return io_status;
		if (io_bytes_done<=0) break;
		delta = io_bytes_done;

		io_status = dst->write(ptr,delta,io_bytes_done,p_abort);
		if (io_result_failed(io_status)) return io_status;
		else if (io_bytes_done != delta) return io_result_error_generic;
		done += delta;
	}
	transferred = done;
	return done == bytes ? io_result_success : io_result_eof;
}

t_io_result file::g_transfer_object(stream_reader * src,stream_writer * dst,t_filesize bytes,abort_callback & p_abort)
{
	t_filesize io_bytes_done;
	t_io_result io_status;
	io_status = g_transfer(src,dst,bytes,io_bytes_done,p_abort);
	if (io_result_succeeded(io_status) && io_bytes_done != bytes) io_status = io_result_error_data;
	return io_status;
}


void filesystem::g_get_canonical_path(const char * path,string_base & out)
{
	TRACK_CALL_TEXT("filesystem::g_get_canonical_path");

	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	if (e.first(ptr)) do {
		if (ptr->get_canonical_path(path,out)) return;
	} while(e.next(ptr));
	//noone wants to process this, lets copy over
	out.set_string(path);
}

void filesystem::g_get_display_path(const char * path,string_base & out)
{
	TRACK_CALL_TEXT("filesystem::g_get_display_path");
	service_ptr_t<filesystem> ptr;
	if (!g_get_interface(ptr,path))
	{
		//noone wants to process this, lets copy over
		out.set_string(path);
	}
	else
	{
		if (!ptr->get_display_path(path,out))
			out.set_string(path);
	}
}

bool filesystem::g_get_interface(service_ptr_t<filesystem> & p_out,const char * path)
{
	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	if (e.first(ptr)) do {
		if (ptr->is_our_path(path))
		{
			p_out = ptr;
			return true;
		}
	} while(e.next(ptr));
	return false;
}


t_io_result filesystem::g_open(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("filesystem::g_open");
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path)) return io_result_error_generic;
	return fs->open(p_out,path,mode,p_abort);
}

void filesystem::g_open_e(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort)
{
	t_io_result status;
	status = g_open(p_out,path,mode,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result filesystem::g_open_ex(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	return g_open(p_out,path_c,mode,p_abort);
}

bool filesystem::g_exists(const char * p_path,abort_callback & p_abort)
{
	t_filestats stats;
	bool dummy;
	return io_result_succeeded(g_get_stats(p_path,stats,dummy,p_abort));
}

bool filesystem::g_exists_writeable(const char * p_path,abort_callback & p_abort)
{
	t_filestats stats;
	bool writeable;
	if (!io_result_succeeded(g_get_stats(p_path,stats,writeable,p_abort))) return false;
	return writeable;
}

t_io_result filesystem::g_remove(const char * path,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) return io_result_error_generic;
	return fs->remove(path_c,p_abort);
}

void filesystem::g_create_directory_e(const char * p_path,abort_callback & p_abort) {
	t_io_result status;
	status = g_create_directory(p_path,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}
t_io_result filesystem::g_create_directory(const char * path,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) return io_result_error_generic;
	return fs->create_directory(path_c,p_abort);
}

t_io_result filesystem::g_move(const char * src,const char * dst,abort_callback & p_abort)
{
	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	t_io_result rv = io_result_error_generic;
	if (e.first(ptr)) do {
		if (ptr->is_our_path(src) && ptr->is_our_path(dst))
		{
			rv = ptr->move(src,dst,p_abort);
		}
		if (io_result_succeeded(rv)) break;
	} while(e.next(ptr));
	return rv;
}

t_io_result filesystem::g_move_ex(const char * _src,const char * _dst,abort_callback & p_abort)
{
	string8 src,dst;
	g_get_canonical_path(_src,src);
	g_get_canonical_path(_dst,dst);
	return g_move(src,dst,p_abort);
}

t_io_result filesystem::g_list_directory_ex(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(p_path,path_c);
	return g_list_directory(path_c,p_out,p_abort);
}

t_io_result filesystem::g_list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
{
	service_ptr_t<filesystem> ptr;
	if (!g_get_interface(ptr,p_path)) return io_result_error_not_found;
	return ptr->list_directory(p_path,p_out,p_abort);
}


static void path_pack_string(string_base & out,const char * src)
{
	out.add_char('|');
	out.add_int(strlen(src));
	out.add_char('|');
	out.add_string(src);
	out.add_char('|');
}

static int path_unpack_string(string8 & out,const char * src)
{
	int ptr=0;
	if (src[ptr++]!='|') return -1;
	int len = atoi(src+ptr);
	if (len<=0) return -1;
	while(src[ptr]!=0 && src[ptr]!='|') ptr++;
	if (src[ptr]!='|') return -1;
	ptr++;
	int start = ptr;
	while(ptr-start<len)
	{
		if (src[ptr]==0) return -1;
		ptr++;
	}
	if (src[ptr]!='|') return -1;
	out.add_string(&src[start],len);
	ptr++;	
	return ptr;
}


t_io_result filesystem::g_open_precache(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) return io_result_error_not_found;
	if (fs->is_remote(path)) return io_result_error_data;
	return fs->open(p_out,path_c,open_mode_read,p_abort);
}

bool filesystem::g_is_remote(const char * path)
{
	bool rv = false;
	service_ptr_t<filesystem> fs;
	if (g_get_interface(fs,path))
		rv = fs->is_remote(path);
	return rv;
}


bool filesystem::g_relative_path_create(const char * file_path,const char * playlist_path,string_base & out)
{
	
	bool rv = false;
	service_ptr_t<filesystem> fs;

	if (g_get_interface(fs,file_path))
		rv = fs->relative_path_create(file_path,playlist_path,out);
	
	return rv;
}

bool filesystem::g_relative_path_parse(const char * relative_path,const char * playlist_path,string_base & out)
{
	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	if (e.first(ptr)) do {
		if (ptr->relative_path_parse(relative_path,playlist_path,out)) return true;
	} while(e.next(ptr));
	return false;
}



bool archive_impl::get_canonical_path(const char * path,string_base & out)
{
	if (is_our_path(path))
	{
		string8 archive,file,archive_canonical;
		if (g_parse_unpack_path(path,archive,file))
		{
			g_get_canonical_path(archive,archive_canonical);
			make_unpack_path(out,archive_canonical,file);

			return true;
		}
		else return false;
	}
	else return false;
}

bool archive_impl::is_our_path(const char * path)
{
	if (strncmp(path,"unpack://",9)) return false;
	const char * type = get_archive_type();
	path += 9;
	while(*type)
	{
		if (*type!=*path) return false;
		type++;
		path++;
	}
	if (*path!='|') return false;
	return true;
}

bool archive_impl::get_display_path(const char * path,string_base & out)
{
	string8 archive,file;
	if (g_parse_unpack_path(path,archive,file))
	{
		g_get_display_path(archive,out);
		out.add_string("|");
		out.add_string(file);
		return true;
	}
	else return false;
}

t_io_result archive_impl::open(service_ptr_t<file> & p_out,const char * path,t_open_mode mode, abort_callback & p_abort)
{
	if (mode != open_mode_read) return io_result_error_generic;
	string8 archive,file;
	if (!g_parse_unpack_path(path,archive,file)) return io_result_error_not_found;
	return open_archive(p_out,archive,file,p_abort);
}


t_io_result archive_impl::remove(const char * path,abort_callback & p_abort)
{
	return io_result_error_generic;
}

t_io_result archive_impl::move(const char * src,const char * dst,abort_callback & p_abort)
{
	return io_result_error_generic;
}

bool archive_impl::is_remote(const char * src)
{
	string8 archive,file;
	if (g_parse_unpack_path(src,archive,file))
		return g_is_remote(archive);
	else return false;
}

bool archive_impl::relative_path_create(const char * file_path,const char * playlist_path,string_base & out)
{
	string8 archive,file;
	if (g_parse_unpack_path(file_path,archive,file))
	{
		string8 archive_rel;
		if (g_relative_path_create(archive,playlist_path,archive_rel))
		{
			string8 out_path;
			make_unpack_path(out_path,archive_rel,file);
			out.set_string(out_path);
			return true;
		}
	}
	return false;
}

bool archive_impl::relative_path_parse(const char * relative_path,const char * playlist_path,string_base & out)
{
	if (!is_our_path(relative_path)) return false;
	string8 archive_rel,file;
	if (g_parse_unpack_path(relative_path,archive_rel,file))
	{
		string8 archive;
		if (g_relative_path_parse(archive_rel,playlist_path,archive))
		{
			string8 out_path;
			make_unpack_path(out_path,archive,file);
			out.set_string(out_path);
			return true;
		}
	}
	return false;
}

// "unpack://zip|17|file://c:\unf.rar|meh.mp3"

bool archive_impl::g_parse_unpack_path(const char * path,string8 & archive,string8 & file)
{
	path  = strchr(path,'|');
	if (!path) return false;
	int delta = path_unpack_string(archive,path);
	if (delta<0) return false;
	path += delta;
	file = path;
	return true;
}

void archive_impl::g_make_unpack_path(string_base & path,const char * archive,const char * file,const char * name)
{
	path = "unpack://";
	path += name;
	path_pack_string(path,archive);
	path += file;
}

void archive_impl::make_unpack_path(string_base & path,const char * archive,const char * file) {g_make_unpack_path(path,archive,file,get_archive_type());}


FILE * filesystem::streamio_open(const char * path,const char * flags)
{
	FILE * ret = 0;
	string8 temp;
	g_get_canonical_path(path,temp);
	if (!strncmp(temp,"file://",7))
	{
		ret = _wfopen(pfc::stringcvt::string_wide_from_utf8(path+7),pfc::stringcvt::string_wide_from_utf8(flags));
	}
	return ret;
}


namespace {

	class directory_callback_isempty : public directory_callback
	{
		bool m_isempty;
	public:
		directory_callback_isempty() : m_isempty(true) {}
		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats)
		{
			m_isempty = false;
			return false;
		}
		bool isempty() {return m_isempty;}
	};

	class directory_callback_dummy : public directory_callback
	{
	public:
		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats) {return false;}
	};

}

bool filesystem::g_is_empty_directory(const char * path,abort_callback & p_abort)
{
	directory_callback_isempty callback;
	if (io_result_failed(g_list_directory(path,callback,p_abort ) ) ) return false;
	return callback.isempty();
}

bool filesystem::g_is_valid_directory(const char * path,abort_callback & p_abort)
{
	return io_result_succeeded(g_list_directory(path,directory_callback_dummy(),p_abort));
}

bool directory_callback_i::on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats)
{
	if (is_subdirectory)
	{
		if (m_recur)
		{
			//todo handling error *appropriately*, we should not abort enumeration entirely because of single offending dir
			t_io_result status = owner->list_directory(url,*this,p_abort);
			//if (io_result_failed(status)) return false;
			//if (io_result_failed(status) && status != io_result_error_sharing_violation && status != io_result_error_denied) return false;
		}
		return !p_abort.is_aborting();
	}
	else
	{
		data.add_item(new t_entry(url,p_stats));
		return !p_abort.is_aborting();
	}
}

namespace {
	class directory_callback_impl_copy : public directory_callback
	{
	public:
		directory_callback_impl_copy(const char * p_target)
		{
			m_target = p_target;
			m_target.fix_dir_separator('\\');
			m_status = io_result_success;
		}

		t_io_result get_status() const {return m_status;}

		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats)
		{
			const char * fn = url + string8::g_scan_filename(url);
			unsigned truncat = m_target.length();
			m_target += fn;
			if (is_subdirectory)
			{
				m_target += "\\";
				t_io_result status = owner->list_directory(url,*this,p_abort);
				if (io_result_succeeded(m_status)) m_status = status;
			}
			else
			{
				m_status = filesystem::g_copy(url,m_target,p_abort);
			}
			m_target.truncate(truncat);
			return io_result_succeeded(m_status);
		}
	private:
		string8_fastalloc m_target;
		t_io_result m_status;
	};
}

t_io_result filesystem::g_copy_directory(const char * src,const char * dst,abort_callback & p_abort)
{//UNTESTED
	directory_callback_impl_copy callback(dst);
	t_io_result ret1 = filesystem::g_list_directory(src,callback,p_abort);
	t_io_result ret2 = callback.get_status();
	if (p_abort.is_aborting()) return io_result_aborted;
	else if (ret1 == io_result_aborted && io_result_failed(ret2)) return ret2;
	else if (io_result_succeeded(ret1) && io_result_failed(ret2)) return ret2;
	else return ret1;
}

t_io_result filesystem::g_copy(const char * src,const char * dst,abort_callback & p_abort)
{

	service_ptr_t<file> r_src,r_dst;
	t_filesize size, done;

	t_io_result io_status;

	io_status = g_open(r_src,src,open_mode_read,p_abort);
	if (io_result_failed(io_status)) return io_status;
	io_status = r_src->get_size(size,p_abort);
	if (io_result_failed(io_status)) return io_status;
	if (size == filesize_invalid) return io_result_error_generic;//unknown length, cant transfer
	io_status = g_open(r_dst,dst,open_mode_write_new,p_abort);
	if (io_result_failed(io_status)) return io_status;
	if (size > 0)
	{
		io_status = file::g_transfer(r_src.get_ptr(),r_dst.get_ptr(),size,done,p_abort);
		if (io_result_failed(io_status))
		{
			r_dst.release();
			g_remove(dst,abort_callback_impl());
			return io_status;
		}
	}
	else done = 0;

	return io_result_success;
}

t_io_result filesystem::g_copy_ex(const char * _src,const char * _dst,abort_callback & p_abort)
{
	string8 src,dst;
	g_get_canonical_path(_src,src);
	g_get_canonical_path(_dst,dst);
	return g_copy(src,dst,p_abort);
}


unsigned stream_reader::read_e(void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	unsigned io_bytes_done;
	io_status = read(p_buffer,p_bytes,io_bytes_done,p_abort);
	if (io_result_failed(io_status)) throw exception_io(io_status);
	return io_bytes_done;
}

t_io_result stream_reader::read_object(void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	unsigned io_bytes_done;
	io_status = read(p_buffer,p_bytes,io_bytes_done,p_abort);
	if (io_result_succeeded(io_status) && io_bytes_done != p_bytes) io_status = io_result_error_data;
	return io_status;
}

void stream_reader::read_object_e(void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	io_status = read_object(p_buffer,p_bytes,p_abort);
	if (io_result_failed_or_eof(io_status)) throw exception_io(io_status);
}

unsigned stream_writer::write_e(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	unsigned io_bytes_done;
	io_status = write(p_buffer,p_bytes,io_bytes_done,p_abort);
	if (io_result_failed(io_status)) throw exception_io(io_status);
	return io_bytes_done;
}

void stream_writer::write_object_e(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	io_status = write_object(p_buffer,p_bytes,p_abort);
	if (io_result_failed(io_status)) throw exception_io(io_status);
}

void file::seek_e(t_filesize position,abort_callback & p_abort)
{
	t_io_result status = seek(position,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

void file::seek2_e(t_int64 position,int mode,abort_callback & p_abort)
{
	t_io_result status = seek2(position,mode,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

void file::reopen_e(abort_callback & p_abort)
{
	t_io_result status = reopen(p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result stream_writer::write_object(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort)
{
	t_io_result io_status;
	unsigned io_bytes_done;
	io_status = write(p_buffer,p_bytes,io_bytes_done,p_abort);
	if (io_result_succeeded(io_status) && io_bytes_done != p_bytes) io_status = io_result_error_data;
	return io_status;
}

t_uint64 stream_reader::skip_e(t_uint64 p_bytes,abort_callback & p_abort)
{
	t_uint64 skipped = 0;
	t_io_result status = skip(p_bytes,skipped,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
	return skipped;
}

void stream_reader::skip_object_e(t_uint64 p_bytes,abort_callback & p_abort)
{
	t_io_result status = skip_object(p_bytes,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_filesize file::get_size_e(abort_callback & p_abort)
{
	t_io_result status;
	t_filesize ret;
	status = get_size(ret,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
	return ret;
}

t_filesize file::get_position_e(abort_callback & p_abort)
{
	t_io_result status;
	t_filesize ret;
	status = get_position(ret,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
	return ret;
}

t_io_result file::get_stats(t_filestats & p_stats,abort_callback & p_abort)
{
	t_io_result status;
	status = get_size(p_stats.m_size,p_abort);
	if (io_result_failed(status)) return status;
	status = get_timestamp(p_stats.m_timestamp,p_abort);
	if (io_result_failed(status)) return status;
	return io_result_success;
}

t_filestats file::get_stats_e(abort_callback & p_abort)
{
	t_filestats temp;
	temp.m_size = get_size_e(p_abort);
	temp.m_timestamp = get_timestamp_e(p_abort);
	return temp;
}

t_filetimestamp file::get_timestamp_e(abort_callback & p_abort)
{
	t_io_result status;
	t_filetimestamp ret;
	status = get_timestamp(ret,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
	return ret;
}

t_io_result stream_reader::skip(t_uint64 p_bytes,t_uint64 & p_bytes_skipped,abort_callback & p_abort)
{
	unsigned char temp[256];
	t_uint64 todo = p_bytes, done = 0;
	while(todo > 0)
	{
		unsigned delta,deltadone;
		delta = sizeof(temp);
		if (delta > todo) delta = (unsigned) todo;
		t_io_result status;
		status = read(temp,delta,deltadone,p_abort);
		if (io_result_failed(status)) return status;
		done += deltadone;
		todo -= deltadone;
		if (status == io_result_eof) break;
	}
	p_bytes_skipped = done;
	return done == p_bytes ? io_result_success : io_result_eof;
}

t_io_result stream_reader::skip_object(t_uint64 p_bytes,abort_callback & p_abort)
{
	t_uint64 done = 0;
	t_io_result status;
	status = skip(p_bytes,done,p_abort);
	if (io_result_succeeded(status) && done != p_bytes) status = io_result_error_data;
	return status;
}

void file::set_eof_e(abort_callback & p_abort)
{
	t_io_result status = set_eof(p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

void file::g_transfer_e(stream_reader * src,stream_writer * dst,t_filesize bytes,t_filesize & transferred,abort_callback & p_abort)
{
	t_io_result status = g_transfer(src,dst,bytes,transferred,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

void file::g_transfer_object_e(stream_reader * src,stream_writer * dst,t_filesize bytes,abort_callback & p_abort)
{
	t_io_result status = g_transfer_object(src,dst,bytes,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result filesystem::g_open_write_new(service_ptr_t<file> & p_out,const char * p_path,abort_callback & p_abort)
{
	return g_open(p_out,p_path,open_mode_write_new,p_abort);
}

void filesystem::g_open_write_new_e(service_ptr_t<file> & p_out,const char * p_path,abort_callback & p_abort)
{
	t_io_result code = g_open_write_new(p_out,p_path,p_abort);
	if (io_result_failed(code)) throw exception_io(code);
}

t_io_result file::g_transfer_file(const service_ptr_t<file> & p_from,const service_ptr_t<file> & p_to,abort_callback & p_abort)
{
	t_io_result status;
	t_filesize length;
	if (io_result_failed( status = p_from->get_size(length,p_abort) ) ) return status;
	if (length == filesize_invalid) return io_result_error_data;
	if (io_result_failed( status = p_from->seek(0,p_abort) ) ) return status;
	if (io_result_failed( status = p_to->seek(0,p_abort) ) ) return status;
	if (io_result_failed( status = p_to->set_eof(p_abort) ) ) return status;
	if (length > 0)
	{
		if (io_result_failed( status = g_transfer_object(p_from.get_ptr(),p_to.get_ptr(),length,p_abort) ) ) return status;
	}
	return io_result_success;
}

t_io_result filesystem::g_open_temp(service_ptr_t<file> & p_out,abort_callback & p_abort)
{
	return g_open(p_out,"tempfile://",open_mode_write_new,p_abort);
}

t_io_result filesystem::g_open_tempmem(service_ptr_t<file> & p_out,abort_callback & p_abort)
{
	t_io_result status;
	
	status = g_open(p_out,"tempmem://",open_mode_write_new,p_abort);
	
	if (io_result_failed(status))
		status = g_open_temp(p_out,p_abort);

	return status;
}

void filesystem::g_open_temp_e(service_ptr_t<file> & p_out,abort_callback & p_abort)
{
	t_io_result status = g_open_temp(p_out,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

void filesystem::g_open_tempmem_e(service_ptr_t<file> & p_out,abort_callback & p_abort)
{
	t_io_result status = g_open_tempmem(p_out,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result archive_impl::list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
{
	return io_result_error_not_found;
}

t_io_result archive_impl::create_directory(const char * path,abort_callback &)
{
	return io_result_error_generic;
}

t_io_result filesystem::g_get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("filesystem::g_get_stats");
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,p_path)) return io_result_error_generic;
	return fs->get_stats(p_path,p_stats,p_is_writeable,p_abort);
}

t_io_result filesystem::g_get_stats_ex(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(p_path,path_c);
	return g_get_stats(path_c,p_stats,p_is_writeable,p_abort);
}

t_io_result archive_impl::get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort)
{
	string8 archive,file;
	if (g_parse_unpack_path(p_path,archive,file))
	{
		if (g_is_remote(archive)) return io_result_error_data;
		p_is_writeable = false;
		return get_stats_in_archive(archive,file,p_stats,p_abort);
	}
	else return io_result_error_generic;
}


t_io_result file::is_eof(bool & p_out,abort_callback & p_abort)
{
	t_io_result status;
	t_filesize position,size;
	status = get_position(position,p_abort);
	if (io_result_failed(status)) return status;
	status = get_size(size,p_abort);
	if (io_result_failed(status)) return status;
	if (size == filesize_invalid) return io_result_error_generic;
	p_out = position >= size;
	return io_result_success;
}

bool file::is_eof_e(abort_callback & p_abort)
{
	bool ret;
	t_io_result code;
	code = is_eof(ret,p_abort);
	if (io_result_failed(code)) throw exception_io(code);
	return ret;
}


const char * foobar2000_io::io_result_get_message(t_io_result param)
{
	switch(param)
	{
	case io_result_success:
		return "Success";
	case io_result_eof:
		return "End Of File";
	case io_result_error_generic:
		return "Generic Error";
	case io_result_error_not_found:
		return "Object Not Found";
	case io_result_error_denied:
		return "Access Denied";
	case io_result_error_data:
		return "Unsupported format or corrupted file";
	case io_result_error_sharing_violation:
		return "Sharing Violation";
	case io_result_aborted:
		return "User Abort";
	case io_result_error_device_full:
		return "Device is full";
	case io_result_error_out_of_memory:
		return "Out of memory";
	case io_result_error_bug_check:
		return "Bug check";
	case io_result_error_user:
		return "User error";
	default:
		return "Unknown error code";
	}
}

t_io_result foobar2000_io::io_error_from_win32(DWORD p_code)
{
	switch(p_code)
	{
	case ERROR_ACCESS_DENIED:
	case ERROR_BUSY:
	case ERROR_PATH_BUSY:
		return io_result_error_denied;
	case ERROR_SHARING_VIOLATION:
		return io_result_error_sharing_violation;
	default:
		return io_result_error_generic;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return io_result_error_not_found;
	}
}

t_filetimestamp foobar2000_io::filetimestamp_from_system_timer()
{
	t_filetimestamp ret;
	GetSystemTimeAsFileTime((FILETIME*)&ret);
	return ret;
}

t_io_result stream_reader::read_string(string_base & p_out,abort_callback & p_abort)
{
	t_io_result status;
	t_uint32 length;
	status = read_lendian_t(length,p_abort);
	if (io_result_failed(status)) return status;
	char * ptr = p_out.lock_buffer(length);
	if (ptr == 0) return io_result_error_out_of_memory;
	status = read_object(ptr,length,p_abort);
	p_out.unlock_buffer();
	if (io_result_failed(status)) return status;
	return io_result_success;	
}

void stream_reader::read_string_e(string_base & p_out,abort_callback & p_abort)
{
	t_io_result status;
	status = read_string(p_out,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result stream_reader::read_string_raw(string_base & p_out,abort_callback & p_abort)
{
	enum {delta = 256};
	char buffer[delta];
	t_io_result status;
	p_out.reset();
	for(;;)
	{
		unsigned delta_done = 0;
		status = read(buffer,delta,delta_done,p_abort);
		if (io_result_failed(status)) return status;
		p_out.add_string(buffer,delta_done);
		if (status == io_result_eof) break;
	}
	return io_result_success;
}

void stream_reader::read_string_raw_e(string_base & p_out,abort_callback & p_abort)
{
	t_io_result status;
	status = read_string_raw(p_out,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result stream_writer::write_string(const char * p_string,abort_callback & p_abort)
{
	t_io_result status;
	t_uint32 len = (t_uint32) strlen(p_string);
	status = write_lendian_t(len,p_abort);
	if (io_result_failed(status)) return status;
	status = write_object(p_string,len,p_abort);
	if (io_result_failed(status)) return status;
	return io_result_success;
}

void stream_writer::write_string_e(const char * p_string,abort_callback & p_abort)
{
	t_io_result status;
	status = write_string(p_string,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}

t_io_result stream_writer::write_string_raw(const char * p_string,abort_callback & p_abort)
{
	return write_object(p_string,strlen(p_string),p_abort);
}

void stream_writer::write_string_raw_e(const char * p_string,abort_callback & p_abort)
{
	t_io_result status;
	status = write_string_raw(p_string,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}


t_io_result file::truncate(t_uint64 p_position,abort_callback & p_abort)
{
	t_uint64 backup;
	t_io_result status;
	status = get_position(backup,p_abort);
	if (io_result_failed(status)) return status;
	status = seek(p_position,p_abort);
	if (io_result_failed(status)) return status;
	status = set_eof(p_abort);
	if (io_result_failed(status)) return status;
	if (backup < p_position)
	{
		status = seek(backup,p_abort);
		if (io_result_failed(status)) return status;
	}
	return io_result_success;
}

void file::truncate_e(t_uint64 p_position,abort_callback & p_abort)
{
	t_io_result status = truncate(p_position,p_abort);
	if (io_result_failed(status)) throw exception_io(status);
}


format_filetimestamp::format_filetimestamp(t_filetimestamp p_timestamp) {
#ifdef _WINDOWS
	SYSTEMTIME st; FILETIME ft;
	if (FileTimeToLocalFileTime((FILETIME*)&p_timestamp,&ft)) {
		if (FileTimeToSystemTime(&ft,&st)) {
			m_buffer 
				<< format_uint(st.wYear,4) << "-" << format_uint(st.wMonth,2) << "-" << format_uint(st.wDay,2) << " " 
				<< format_uint(st.wHour,2) << ":" << format_uint(st.wMinute,2) << ":" << format_uint(st.wSecond,2);
		} else m_buffer << "<invalid timestamp>";
	} else m_buffer << "<invalid timestamp>";
#else
#error portme
#endif
}