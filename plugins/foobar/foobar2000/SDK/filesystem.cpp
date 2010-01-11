#include "foobar2000.h"


void unpacker::g_open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p,abort_callback & p_abort)
{
	service_enum_t<unpacker> e;
	service_ptr_t<unpacker> ptr;
	if (e.first(ptr)) do {
		p->reopen(p_abort);
		try {
			ptr->open(p_out,p,p_abort);
			return;
		} catch(exception_io_data const &) {}
	} while(e.next(ptr));
	throw exception_io_data();
}

void file::seek_ex(t_sfilesize p_position, file::t_seek_mode p_mode, abort_callback &p_abort) {
	switch(p_mode) {
	case seek_from_beginning:
		seek(p_position,p_abort);
		break;
	case seek_from_current:
		seek(p_position + get_position(p_abort),p_abort);
		break;
	case seek_from_eof:
		seek(p_position + get_size(p_abort),p_abort);
		break;
	default:
		throw exception_io_data();
	}
}

t_filesize file::g_transfer(stream_reader * p_src,stream_writer * p_dst,t_filesize p_bytes,abort_callback & p_abort) {
	enum {BUFSIZE = 1024*1024};
	pfc::array_t<t_uint8> temp;
	temp.set_size((t_size)pfc::min_t<t_filesize>(BUFSIZE,p_bytes));
	void* ptr = temp.get_ptr();
	t_filesize done = 0;
	while(done<p_bytes) {
		p_abort.check_e();
		t_size delta = (t_size)pfc::min_t<t_filesize>(BUFSIZE,p_bytes-done);
		delta = p_src->read(ptr,delta,p_abort);
		if (delta<=0) break;
		p_dst->write(ptr,delta,p_abort);
		done += delta;
	}
	return done;
}

void file::g_transfer_object(stream_reader * p_src,stream_writer * p_dst,t_filesize p_bytes,abort_callback & p_abort) {
	if (g_transfer(p_src,p_dst,p_bytes,p_abort) != p_bytes)
		throw exception_io_data_truncation();
}


void filesystem::g_get_canonical_path(const char * path,pfc::string_base & out)
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

void filesystem::g_get_display_path(const char * path,pfc::string_base & out)
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


void filesystem::g_open(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort)
{
	TRACK_CALL_TEXT("filesystem::g_open");
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path)) throw exception_io_no_handler_for_path();
	fs->open(p_out,path,mode,p_abort);
}

void filesystem::g_open_ex(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	g_open(p_out,path_c,mode,p_abort);
}

bool filesystem::g_exists(const char * p_path,abort_callback & p_abort)
{
	t_filestats stats;
	bool dummy;
	try {
		g_get_stats(p_path,stats,dummy,p_abort);
	} catch(std::exception const &) {return false;}
	return true;
}

bool filesystem::g_exists_writeable(const char * p_path,abort_callback & p_abort)
{
	t_filestats stats;
	bool writeable;
	try {
		g_get_stats(p_path,stats,writeable,p_abort);
	} catch(std::exception const &) {return false;}
	return writeable;
}

void filesystem::g_remove(const char * path,abort_callback & p_abort) {
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) throw exception_io_no_handler_for_path();
	fs->remove(path_c,p_abort);
}

void filesystem::g_create_directory(const char * path,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) throw exception_io_no_handler_for_path();
	fs->create_directory(path_c,p_abort);
}

void filesystem::g_move(const char * src,const char * dst,abort_callback & p_abort) {
	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	if (e.first(ptr)) do {
		if (ptr->is_our_path(src) && ptr->is_our_path(dst)) {
			ptr->move(src,dst,p_abort);
			return;
		}
	} while(e.next(ptr));
	throw exception_io_no_handler_for_path();
}

void filesystem::g_move_ex(const char * _src,const char * _dst,abort_callback & p_abort)
{
	string8 src,dst;
	g_get_canonical_path(_src,src);
	g_get_canonical_path(_dst,dst);
	return g_move(src,dst,p_abort);
}

void filesystem::g_list_directory_ex(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
{
	string8 path_c;
	g_get_canonical_path(p_path,path_c);
	g_list_directory(path_c,p_out,p_abort);
}

void filesystem::g_list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
{
	service_ptr_t<filesystem> ptr;
	if (!g_get_interface(ptr,p_path)) throw exception_io_no_handler_for_path();
	ptr->list_directory(p_path,p_out,p_abort);
}


static void path_pack_string(pfc::string_base & out,const char * src)
{
	out.add_char('|');
	out << strlen(src);
	out.add_char('|');
	out << src;
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


void filesystem::g_open_precache(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort) {
	string8 path_c;
	g_get_canonical_path(path,path_c);
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,path_c)) throw exception_io_no_handler_for_path();
	if (fs->is_remote(path)) throw exception_io_object_is_remote();
	fs->open(p_out,path_c,open_mode_read,p_abort);
}

bool filesystem::g_is_remote(const char * p_path) {
	service_ptr_t<filesystem> fs;
	if (g_get_interface(fs,p_path)) return fs->is_remote(p_path);
	else throw exception_io_no_handler_for_path();
}

bool filesystem::g_is_remote_safe(const char * p_path) {
	service_ptr_t<filesystem> fs;
	if (g_get_interface(fs,p_path)) return fs->is_remote(p_path);
	else return false;
}

bool filesystem::g_is_remote_or_unrecognized(const char * p_path) {
	service_ptr_t<filesystem> fs;
	if (g_get_interface(fs,p_path)) return fs->is_remote(p_path);
	else return true;
}

bool filesystem::g_relative_path_create(const char * file_path,const char * playlist_path,pfc::string_base & out)
{
	
	bool rv = false;
	service_ptr_t<filesystem> fs;

	if (g_get_interface(fs,file_path))
		rv = fs->relative_path_create(file_path,playlist_path,out);
	
	return rv;
}

bool filesystem::g_relative_path_parse(const char * relative_path,const char * playlist_path,pfc::string_base & out)
{
	service_enum_t<filesystem> e;
	service_ptr_t<filesystem> ptr;
	if (e.first(ptr)) do {
		if (ptr->relative_path_parse(relative_path,playlist_path,out)) return true;
	} while(e.next(ptr));
	return false;
}



bool archive_impl::get_canonical_path(const char * path,pfc::string_base & out)
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

bool archive_impl::get_display_path(const char * path,pfc::string_base & out)
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

void archive_impl::open(service_ptr_t<file> & p_out,const char * path,t_open_mode mode, abort_callback & p_abort)
{
	if (mode != open_mode_read) throw exception_io_denied();
	string8 archive,file;
	if (!g_parse_unpack_path(path,archive,file)) throw exception_io_not_found();
	open_archive(p_out,archive,file,p_abort);
}


void archive_impl::remove(const char * path,abort_callback & p_abort) {
	throw exception_io_denied();
}

void archive_impl::move(const char * src,const char * dst,abort_callback & p_abort) {
	throw exception_io_denied();
}

bool archive_impl::is_remote(const char * src) {
	string8 archive,file;
	if (g_parse_unpack_path(src,archive,file)) return g_is_remote(archive);
	else throw exception_io_not_found();
}

bool archive_impl::relative_path_create(const char * file_path,const char * playlist_path,pfc::string_base & out) {
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

bool archive_impl::relative_path_parse(const char * relative_path,const char * playlist_path,pfc::string_base & out)
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

void archive_impl::g_make_unpack_path(pfc::string_base & path,const char * archive,const char * file,const char * name)
{
	path = "unpack://";
	path += name;
	path_pack_string(path,archive);
	path += file;
}

void archive_impl::make_unpack_path(pfc::string_base & path,const char * archive,const char * file) {g_make_unpack_path(path,archive,file,get_archive_type());}


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
	try {
		g_list_directory(path,callback,p_abort);
	} catch(exception_io const &) {return false;}
	return callback.isempty();
}

bool filesystem::g_is_valid_directory(const char * path,abort_callback & p_abort) {
	try {
		g_list_directory(path,directory_callback_dummy(),p_abort);
		return true;
	} catch(exception_io const &) {return false;}
}

bool directory_callback_impl::on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats) {
	p_abort.check_e();
	if (is_subdirectory) {
		if (m_recur) {
			try {
				owner->list_directory(url,*this,p_abort);
			} catch(exception_io const &) {}
		}
	} else {
		m_data.add_item(pfc::rcnew_t<t_entry>(url,p_stats));
	}
	return true;
}

namespace {
	class directory_callback_impl_copy : public directory_callback
	{
	public:
		directory_callback_impl_copy(const char * p_target)
		{
			m_target = p_target;
			m_target.fix_dir_separator('\\');
		}

		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats) {
			const char * fn = url + pfc::scan_filename(url);
			t_size truncat = m_target.length();
			m_target += fn;
			if (is_subdirectory) {
				try {
					filesystem::g_create_directory(m_target,p_abort);
				} catch(exception_io_already_exists) {}
				m_target += "\\";
				owner->list_directory(url,*this,p_abort);
			} else {
				filesystem::g_copy(url,m_target,p_abort);
			}
			m_target.truncate(truncat);
			return true;
		}
	private:
		string8_fastalloc m_target;
//		t_io_result m_status;
	};
}

void filesystem::g_copy_directory(const char * src,const char * dst,abort_callback & p_abort) {
	//UNTESTED
	filesystem::g_list_directory(src,directory_callback_impl_copy(dst),p_abort);
}

void filesystem::g_copy(const char * src,const char * dst,abort_callback & p_abort) {
	service_ptr_t<file> r_src,r_dst;
	t_filesize size;

	g_open(r_src,src,open_mode_read,p_abort);
	size = r_src->get_size(p_abort);
	if (size == filesize_invalid) throw exception_io_no_length();
	g_open(r_dst,dst,open_mode_write_new,p_abort);
	
	if (size > 0) {
		try {
			file::g_transfer_object(r_src.get_ptr(),r_dst.get_ptr(),size,p_abort);
		} catch(std::exception) {
			r_dst.release();
			try {g_remove(dst,abort_callback_impl());} catch(std::exception) {}
			throw;
		}
	}
}

void filesystem::g_copy_ex(const char * _src,const char * _dst,abort_callback & p_abort) {
	string8 src,dst;
	g_get_canonical_path(_src,src);
	g_get_canonical_path(_dst,dst);
	g_copy(src,dst,p_abort);
}

void stream_reader::read_object(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
	if (read(p_buffer,p_bytes,p_abort) != p_bytes) throw exception_io_data_truncation();
}

t_filestats file::get_stats(abort_callback & p_abort)
{
	t_filestats temp;
	temp.m_size = get_size(p_abort);
	temp.m_timestamp = get_timestamp(p_abort);
	return temp;
}

t_filesize stream_reader::skip(t_filesize p_bytes,abort_callback & p_abort)
{
	t_uint8 temp[256];
	t_filesize todo = p_bytes, done = 0;
	while(todo > 0) {
		t_size delta,deltadone;
		delta = sizeof(temp);
		if (delta > todo) delta = (t_size) todo;
		deltadone = read(temp,delta,p_abort);
		done += deltadone;
		todo -= deltadone;
		if (deltadone < delta) break;
	}
	return done;
}

void stream_reader::skip_object(t_filesize p_bytes,abort_callback & p_abort) {
	if (skip(p_bytes,p_abort) != p_bytes) throw exception_io_data_truncation();
}

void filesystem::g_open_write_new(service_ptr_t<file> & p_out,const char * p_path,abort_callback & p_abort) {
	g_open(p_out,p_path,open_mode_write_new,p_abort);
}
void file::g_transfer_file(const service_ptr_t<file> & p_from,const service_ptr_t<file> & p_to,abort_callback & p_abort) {
	t_filesize length;
	length = p_from->get_size(p_abort);
	if (length == filesize_invalid) throw exception_io_no_length();
	p_from->seek(0,p_abort);
	p_to->seek(0,p_abort);
	p_to->set_eof(p_abort);
	if (length > 0) {
		g_transfer_object(p_from.get_ptr(),p_to.get_ptr(),length,p_abort);
	}
}

void filesystem::g_open_temp(service_ptr_t<file> & p_out,abort_callback & p_abort) {
	g_open(p_out,"tempfile://",open_mode_write_new,p_abort);
}

void filesystem::g_open_tempmem(service_ptr_t<file> & p_out,abort_callback & p_abort) {
	g_open(p_out,"tempmem://",open_mode_write_new,p_abort);
}

void archive_impl::list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort) {
	throw exception_io_not_found();
}

void archive_impl::create_directory(const char * path,abort_callback &) {
	throw exception_io_denied();
}

void filesystem::g_get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort) {
	TRACK_CALL_TEXT("filesystem::g_get_stats");
	service_ptr_t<filesystem> fs;
	if (!g_get_interface(fs,p_path)) throw exception_io_no_handler_for_path();
	return fs->get_stats(p_path,p_stats,p_is_writeable,p_abort);
}

void filesystem::g_get_stats_ex(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort) {
	string8 path_c;
	g_get_canonical_path(p_path,path_c);
	g_get_stats(path_c,p_stats,p_is_writeable,p_abort);
}

void archive_impl::get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort) {
	string8 archive,file;
	if (g_parse_unpack_path(p_path,archive,file)) {
		if (g_is_remote(archive)) throw exception_io_object_is_remote();
		p_is_writeable = false;
		p_stats = get_stats_in_archive(archive,file,p_abort);
	}
	else throw exception_io_not_found();
}


bool file::is_eof(abort_callback & p_abort) {
	t_filesize position,size;
	position = get_position(p_abort);
	size = get_size(p_abort);
	if (size == filesize_invalid) return false;
	return position >= size;
}


t_filetimestamp foobar2000_io::filetimestamp_from_system_timer()
{
	t_filetimestamp ret;
	GetSystemTimeAsFileTime((FILETIME*)&ret);
	return ret;
}

void stream_reader::read_string(pfc::string_base & p_out,abort_callback & p_abort)
{
	t_uint32 length;
	read_lendian_t(length,p_abort);
	char * ptr = p_out.lock_buffer(length);
	try {
		read_object(ptr,length,p_abort);
	} catch(...) {
		p_out.unlock_buffer();
		throw;
	}
	p_out.unlock_buffer();
}

void stream_reader::read_string_raw(pfc::string_base & p_out,abort_callback & p_abort) {
	enum {delta = 256};
	char buffer[delta];
	p_out.reset();
	for(;;) {
		t_size delta_done;
		delta_done = read(buffer,delta,p_abort);
		p_out.add_string(buffer,delta_done);
		if (delta_done < delta) break;
	}
}

void stream_writer::write_string(const char * p_string,abort_callback & p_abort) {
	t_uint32 len = pfc::downcast_guarded<t_uint32>(strlen(p_string));
	write_lendian_t(len,p_abort);
	write_object(p_string,len,p_abort);
}

void stream_writer::write_string_raw(const char * p_string,abort_callback & p_abort) {
	write_object(p_string,strlen(p_string),p_abort);
}

void file::truncate(t_uint64 p_position,abort_callback & p_abort) {
	if (p_position < get_size(p_abort)) resize(p_position,p_abort);
}


format_filetimestamp::format_filetimestamp(t_filetimestamp p_timestamp) {
#ifdef _WIN32
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

#ifdef _WIN32
void foobar2000_io::exception_io_from_win32(DWORD p_code) {
	switch(p_code) {
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:
		throw exception_io_already_exists();
	case ERROR_ACCESS_DENIED:
	case ERROR_BUSY:
	case ERROR_PATH_BUSY:
		throw exception_io_denied();
	case ERROR_SHARING_VIOLATION:
		throw exception_io_sharing_violation();
	default:
		throw exception_io(string_formatter() << "I/O error (win32 #" << (t_uint32)p_code << ")");
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		throw exception_io_not_found();
	}
}
#endif

t_filesize file::get_size_ex(abort_callback & p_abort) {
	t_filesize temp = get_size(p_abort);
	if (temp == filesize_invalid) throw exception_io_no_length();
	return temp;
}

void file::ensure_seekable() {
	if (!can_seek()) throw exception_io_object_not_seekable();
}

bool filesystem::g_is_recognized_path(const char * p_path) {
	return g_get_interface(service_ptr_t<filesystem>(),p_path);
}

t_filesize file::get_remaining(abort_callback & p_abort) {
	t_filesize length = get_size_ex(p_abort);
	t_filesize position = get_position(p_abort);
	pfc::dynamic_assert(position <= length);
	return length - position;
}
