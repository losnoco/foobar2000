#ifndef _FOOBAR2000_SDK_FILESYSTEM_H_
#define _FOOBAR2000_SDK_FILESYSTEM_H_

class playlist_loader_callback;
class file_info;

//! Contains various I/O related structures and interfaces.

namespace foobar2000_io
{
	//! Type used for file size related variables.
	typedef t_uint64 t_filesize;
	//! Type used for file timestamp related variables. 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601; 0 for invalid/unknown time.
	typedef t_uint64 t_filetimestamp;
	//! Invalid/unknown file timestamp constant. Also see: t_filetimestamp.
	const t_filetimestamp filetimestamp_invalid = 0;
	//! Invalid/unknown file size constant. Also see: t_filesize.
	const t_filesize filesize_invalid = (t_filesize)(-1);


	//! Type used for I/O return codes.
	enum t_io_result
	{
		//! Success
		io_result_success = 0,
		//! End-Of-File; note that EOF is considered a succeess by common conventions
		io_result_eof,
		//! Generic unexpected error
		io_result_error_generic,
		//! Object not found
		io_result_error_not_found,
		//! Access denied
		io_result_error_denied,
		//! Unexpected data read from file; corrupted file or unsupported file format
		io_result_error_data,
		//! Sharing violation
		io_result_error_sharing_violation,
		//! Error, device being written to is full
		io_result_error_device_full,
		//! Out of memory (malloc failure etc)
		io_result_error_out_of_memory,
		//! User error
		io_result_error_user,
		//! Bug in the code
		io_result_error_bug_check,
		//! User abort (see: abort_callback)
		io_result_aborted,
	};


	//! Helper function; determines whether specified t_io_result indicates a failure.
	inline static bool io_result_failed(t_io_result param) {return param >= io_result_error_generic;}
	//! Helper function; determines whether specified t_io_result indicates a success.
	inline static bool io_result_succeeded(t_io_result param) {return param < io_result_error_generic;}
	//! Helper function; determines whether specified t_io_result indicates a failure or EOF.
	inline static bool io_result_failed_or_eof(t_io_result param) {return param >= io_result_eof;}

	//! Helper function; retrieves human-readable message for specified t_io_result.
	const char * io_result_get_message(t_io_result param);
	//! Helper function; converts win32 error code to t_io_result.
	t_io_result io_error_from_win32(DWORD p_code);

	class exception_io : public std::exception {
	public:
		exception_io(t_io_result p_code) : exception(io_result_get_message(p_code),0), m_code(p_code) {}
		exception_io(const exception_io & p_src) {*this = p_src;}
		t_io_result get_code() const {return m_code;}
		static void g_test(t_io_result code) {if (io_result_failed(code)) throw exception_io(code);}
	private:
		t_io_result m_code;
	};

	//! Stores file stats (size and timestamp).
	struct t_filestats
	{
		//! Size of the file.
		t_filesize m_size;
		//! Time of last file modification.
		t_filetimestamp m_timestamp;

		inline bool operator==(const t_filestats & param) const {return m_size == param.m_size && m_timestamp == param.m_timestamp;}
		inline bool operator!=(const t_filestats & param) const {return m_size != param.m_size || m_timestamp != param.m_timestamp;}
	};

	//! Invalid/unknown file stats constant. See: t_filestats.
	static const t_filestats filestats_invalid = {filesize_invalid,filetimestamp_invalid};

	//! Generic interface to read data from a nonseekable stream. Also see: stream_writer, file.
	class NOVTABLE stream_reader
	{
	protected:
		stream_reader() {}
		~stream_reader() {}
	public:

		//! Reads data from a stream. Primary API function.
		//! @param p_buffer Pointer to buffer receiving data.
		//! @param p_bytes Number of bytes to read.
		//! @param p_bytes_read Receives number of bytes actually read on success (may be less than or equal to p_bytes). Initial value is ignored. 
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns io_result_success on success and amount of bytes retrieved equal to amount of bytes requested, io_result_eof on success but less bytes retrieved than asked for (even if no actual data has been retrieved and p_bytes_read set to 0), one of other t_io_result error codes on failure.
		virtual t_io_result read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort) = 0;
		
		//! Helper function; reads specified number of bytes from the stream, and returns an error (io_result_error_data) if EOF was hit before requested amount of data could be retrieved. On successful return, no further checks for amount of data read are needed.
		//! @param p_buffer Pointer to buffer receiving data.
		//! @param p_bytes Number of bytes to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		t_io_result read_object(void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper function built around read(); reads specified amount of bytes from a stream; throws an exception_io exception on failure and returns number of bytes read.
		//! @param p_buffer Pointer to buffer receiving data.
		//! @param p_bytes Number of bytes to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Number of bytes read.
		unsigned read_e(void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper function built around read_object(); reads specified amount of bytes from a stream; throws an exception_io exception on failure or if EOF was hit before requested amount of data could be retrieved (io_result_error_data).
		//! @param p_buffer Pointer to buffer receiving data.
		//! @param p_bytes Number of bytes to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		void read_object_e(void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper function; skips specified amount of bytes in the stream.
		//! @param p_bytes Number of bytes to skip.
		//! @param p_bytes_skipped Receives number of bytes actually skipped on success (may be less than or equal to p_bytes). Initial value is ignored.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		t_io_result skip(t_uint64 p_bytes,t_uint64 & p_bytes_skipped,abort_callback & p_abort);

		//! Helper function built around skip(); skips specified amount of bytes in the stream; returns an error (io_result_error_data) if EOF was hit before requested amount of data could be skipped. On successful return, no further checks for amount of data skipped are needed.
		//! @param p_bytes Number of bytes to skip.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		t_io_result skip_object(t_uint64 p_bytes,abort_callback & p_abort);

		//! Helper function built around skip(); throws an exception_io exception on failure and returns number of bytes skipped.
		//! @param p_bytes Number of bytes to skip.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Number of bytes skipped.
		t_uint64 skip_e(t_uint64 p_bytes,abort_callback & p_abort);

		//! Helper function built around skip_object(); skips specified amount of bytes in the stream; throws an exception_io exception on failure or if EOF was hit before requested amount of data could be skipped (io_result_error_data).
		//! @param p_bytes Number of bytes to skip.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		void skip_object_e(t_uint64 p_bytes,abort_callback & p_abort);


		//! Helper function template built around read_object; reads entire raw variable of any type (int, struct, etc) from the stream. Note: not byte order safe.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result read_object_t(T& p_object,abort_callback & p_abort) {return read_object(&p_object,sizeof(p_object),p_abort);}

		//! Helper function template built around read_object_t; reads entire variable of any type (int, struct, etc) from the stream, using byte_order namespace functions to convert it from little endian byte order to host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_le_to_native_t for relevant types.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result read_lendian_t(T& p_object,abort_callback & p_abort) {t_io_result ret = read_object_t(p_object,p_abort); if (io_result_succeeded(ret)) byte_order::order_le_to_native_t(p_object);return ret;}
		
		//! Helper function template built around read_object_t; reads entire variable of any type (int, struct, etc) from the stream, using byte_order namespace functions to convert it from big endian byte order to host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_be_to_native_t for relevant types.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result read_bendian_t(T& p_object,abort_callback & p_abort) {t_io_result ret = read_object_t(p_object,p_abort); if (io_result_succeeded(ret)) byte_order::order_be_to_native_t(p_object);return ret;}

		//! Helper function template built around read_object_e; reads entire raw variable of any type (int, struct, etc) from the stream. Note: not byte order safe. This helper function throws an exception_io exception on failure.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_object_e_t(T& p_object,abort_callback & p_abort) {read_object_e(&p_object,sizeof(p_object),p_abort);}

		//! Helper function template built around read_object_e_t; reads entire variable of any type (int, struct, etc) from the stream, using byte_order namespace functions to convert it from little endian byte order to host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_le_to_native_t for relevant types. This helper function throws an exception_io exception on failure.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_lendian_e_t(T& p_object,abort_callback & p_abort) {read_object_e_t(p_object,p_abort); byte_order::order_le_to_native_t(p_object); }

		//! Helper function template built around read_object_e_t; reads entire variable of any type (int, struct, etc) from the stream, using byte_order namespace functions to convert it from big endian byte order to host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_be_to_native_t for relevant types. This helper function throws an exception_io exception on failure.
		//! @param p_object Object of any type to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_bendian_e_t(T& p_object,abort_callback & p_abort) {read_object_e_t(p_object,p_abort); byte_order::order_be_to_native_t(p_object); }

		//! Helper function; reads string (with 32-bit header indicating length in bytes followed by UTF-8 encoded data without null terminator).
		t_io_result read_string(string_base & p_out,abort_callback & p_abort);

		//! Helper function built around read_string(); throws an exception on failure.
		void read_string_e(string_base & p_out,abort_callback & p_abort);

		//! Helper function; alternate way of storing strings; assumes string takes space up to end of stream.
		t_io_result read_string_raw(string_base & p_out,abort_callback & p_abort);
		
		//! Helper function built around read_string_raw(); throws an exception on failure.
		void read_string_raw_e(string_base & p_out,abort_callback & p_abort);
	};


	//! Generic interface to write data to a nonseekable stream. Also see: stream_reader, file.
	class NOVTABLE stream_writer
	{
	protected:
		stream_writer() {}
		~stream_writer() {}
	public:
		//! Writes data to the stream. Primary API function.
		//! @param p_buffer Pointer to buffer containing data to write.
		//! @param p_bytes Number of bytes to write.
		//! @param p_bytes_written On success, receives amount of bytes written. Except for rare failure cases or uncommon stream_writer implementations, should be same to p_bytes.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result write(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_written,abort_callback & p_abort) = 0;

		//! Helper built around write(). Writes data to the stream, returns error if less data could be written than requested.
		//! @param p_buffer Pointer to buffer containing data to write.
		//! @param p_bytes Number of bytes to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		t_io_result write_object(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper built around write(); writes data to the stream and throws an exception_io exception on failure.
		//! @param p_buffer Pointer to buffer containing data to write.
		//! @param p_bytes Number of bytes to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Number of bytes written.
		unsigned write_e(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper built around write_object(); writes data to the stream and throws an exception_io exception on failure or when less data than requested could be written.
		//! @param p_buffer Pointer to buffer containing data to write.
		//! @param p_bytes Number of bytes to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		void write_object_e(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort);

		//! Helper template built around write_object(); writes entire raw object of any type (int, struct, etc) to the stream. Note: not byte order safe.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result write_object_t(const T & p_object,abort_callback & p_abort) {return write_object(&p_object,sizeof(p_object),p_abort);}

		//! Helper template built around write_object_t(); writes entire raw object of any type (int, struct, etc) to the stream, using byte_order namespace functions to convert it to little endian byte order from host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_native_to_le_t for relevant types.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result write_lendian_t(const T & p_object,abort_callback & p_abort) {T temp = p_object; byte_order::order_native_to_le_t(temp);return write_object_t(temp,p_abort);}

		//! Helper template built around write_object_t(); writes entire raw object of any type (int, struct, etc) to the stream, using byte_order namespace functions to convert it to big endian byte order from host byte order. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_native_to_be_t for relevant types.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline t_io_result write_bendian_t(const T & p_object,abort_callback & p_abort) {T temp = p_object; byte_order::order_native_to_be_t(temp);return write_object_t(temp,p_abort);}

		//! Helper template built around write_object_e(); writes entire raw object of any type (int, struct, etc) to the stream and throws an exception_io exception on failure. Note: not byte order safe.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_object_e_t(const T & p_object,abort_callback & p_abort) {write_object_e(&p_object,sizeof(p_object),p_abort);}

		//! Helper template built around write_object_t(); writes entire raw object of any type (int, struct, etc) to the stream, using byte_order namespace functions to convert it to little endian byte order from host byte order; throws an exception_io exception on failure. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_native_to_le_t for relevant types.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_lendian_e_t(const T & p_object,abort_callback & p_abort) {T temp = p_object; byte_order::order_native_to_le_t(temp);write_object_e_t(temp,p_abort);}

		//! Helper template built around write_object_t(); writes entire raw object of any type (int, struct, etc) to the stream, using byte_order namespace functions to convert it to big endian byte order from host byte order; throws an exception_io exception on failure. If you want to deal with non-native types using this function, you can add your own specializations of byte_order::order_native_to_be_t for relevant types.
		//! @param p_object Object to be written.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_bendian_e_t(const T & p_object,abort_callback & p_abort) {T temp = p_object; byte_order::order_native_to_be_t(temp);write_object_e_t(temp,p_abort);}

		//! Helper function; writes string (with 32-bit header indicating length in bytes followed by UTF-8 encoded data without null terminator).
		t_io_result write_string(const char * p_string,abort_callback & p_abort);

		//! Helper function built around write_string(); throws an exception on failure.
		void write_string_e(const char * p_string,abort_callback & p_abort);

		//! Helper function; writes raw string to the stream, with no length info or null terminators.
		t_io_result write_string_raw(const char * p_string,abort_callback & p_abort);

		//! Helper function built around write_string_raw(); throws an exception on failure.
		void write_string_raw_e(const char * p_string,abort_callback & p_abort);

	};

	class NOVTABLE file : public service_base, public stream_reader, public stream_writer
	{
	public:
		//! Retrieves size of the file.
		//! @param p_size Receives file size on success; filesize_invalid if unknown (nonseekable stream etc).
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result get_size(t_filesize & p_size,abort_callback & p_abort)=0;


		//! Retrieves read/write cursor position in the file. In case of non-seekable stream, this should return number of bytes read so far.
		//! @param p_position Receives read/write cursor position.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result get_position(t_uint64 & p_position,abort_callback & p_abort)=0;

		//! Truncates the file at current read/write cursor position.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result set_eof(abort_callback & p_abort) = 0;

		//! Sets read/write cursor position to specific offset.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result seek(t_filesize p_position,abort_callback & p_abort)=0;
		virtual t_io_result seek2(t_int64 p_position,int mode,abort_callback & p_abort);
		
		//! Returns whether the file is seekable or not. If can_seek() returns false, all seek() calls will fail.
		virtual bool can_seek() = 0;

		//! Retrieves mime type of the file.
		//! @param p_out Receives content type string on success.
		//! @returns true on success, false on failure or unknown type.
		virtual bool get_content_type(string_base & p_out) {return false;}
		
		//! Hint, returns whether the file is already fully buffered into memory.
		virtual bool is_in_memory() {return false;}
		
		//! Optional, called by owner thread before sleeping.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void on_idle(abort_callback & p_abort) {};

		//! Retrieves last modificaiton time of the file.
		//! @param p_timestamp Receives last modification time of the file if successful.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result get_timestamp(t_filetimestamp & p_timestamp,abort_callback & p_abort)=0;
		
		//! Resets non-seekable stream, or seeks to zero on seekable file.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual t_io_result reopen(abort_callback & p_abort) {return seek(0,p_abort);}

		//! Indicates whether the file is a remote resource and non-sequential access may be slowed down by lag. This is typically returns to true on non-seekable sources but may also return true on seekable sources indicating that seeking is supported but will be relatively slow.
		virtual bool is_remote() = 0;
		
		t_io_result get_stats(t_filestats & p_stats,abort_callback & p_abort);
		t_filestats get_stats_e(abort_callback & p_abort);
		t_filesize get_size_e(abort_callback & p_abort);
		t_filesize get_position_e(abort_callback & p_abort);
		t_filetimestamp get_timestamp_e(abort_callback & p_abort);


		t_io_result is_eof(bool & p_out,abort_callback & p_abort);

		bool is_eof_e(abort_callback & p_abort);

		void seek_e(t_filesize position,abort_callback & p_abort);
		void seek2_e(t_int64 position,int mode,abort_callback & p_abort);
		void reopen_e(abort_callback & p_abort);
		void set_eof_e(abort_callback & p_abort);

		t_io_result truncate(t_uint64 p_position,abort_callback & p_abort);
		void truncate_e(t_uint64 p_position,abort_callback & p_abort);

		//helper
		static t_io_result g_transfer(stream_reader * src,stream_writer * dst,t_filesize bytes,t_filesize & transferred,abort_callback & p_abort);
		static t_io_result g_transfer_object(stream_reader * src,stream_writer * dst,t_filesize bytes,abort_callback & p_abort);
		static void g_transfer_e(stream_reader * src,stream_writer * dst,t_filesize bytes,t_filesize & transferred,abort_callback & p_abort);
		static void g_transfer_object_e(stream_reader * src,stream_writer * dst,t_filesize bytes,abort_callback & p_abort);

		static t_io_result g_transfer_file(const service_ptr_t<file> & p_from,const service_ptr_t<file> & p_to,abort_callback & p_abort);

		
		static const GUID class_guid;

		virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
			if (p_guid == class_guid) {p_out = this; return true;}
			else return service_base::service_query(p_out,p_guid);
		}
	protected:
		file() {}
		~file() {}
	};

	class file_dynamicinfo : public file//for shoutcast metadata nonsense
	{
	public:

		virtual bool get_static_info(class file_info & p_out) = 0;

		virtual bool is_dynamic_info_enabled()=0;//checks if currently open stream gets dynamic metadata

		virtual bool get_dynamic_info(class file_info & p_out)=0;//see input::get_dynamic_info_track

		static const GUID class_guid;

		virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
			if (p_guid == class_guid) {p_out = this; return true;}
			else return file::service_query(p_out,p_guid);
		}
	protected:
		file_dynamicinfo() {}
		~file_dynamicinfo() {}
	};

	class filesystem;

	class NOVTABLE directory_callback
	{
	public:
		virtual bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats)=0;//return true to continue, false to abort
	};

	class NOVTABLE filesystem : public service_base
	{
	public:

		enum t_open_mode
		{
			open_mode_read,
			open_mode_write_existing,
			open_mode_write_new,
		};

		virtual bool get_canonical_path(const char * path,string_base & out)=0;
		virtual bool is_our_path(const char * path)=0;

		virtual bool get_display_path(const char * path,string_base & out)=0;

		virtual t_io_result open(service_ptr_t<file> & p_out,const char * path, t_open_mode mode,abort_callback & p_abort)=0;
		virtual t_io_result remove(const char * path,abort_callback & p_abort)=0;
		virtual t_io_result move(const char * src,const char * dst,abort_callback & p_abort)=0;
		virtual bool is_remote(const char * src) = 0;

		virtual t_io_result get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort) = 0;
		

		virtual bool relative_path_create(const char * file_path,const char * playlist_path,string_base & out) {return 0;}
		virtual bool relative_path_parse(const char * relative_path,const char * playlist_path,string_base & out) {return 0;}

		virtual t_io_result create_directory(const char * path,abort_callback &) = 0;

		virtual t_io_result list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)=0;

		virtual bool supports_content_types() = 0;

		static void g_get_canonical_path(const char * path,string_base & out);
		static void g_get_display_path(const char * path,string_base & out);

		static bool g_get_interface(service_ptr_t<filesystem> & p_out,const char * path);//path is AFTER get_canonical_path
		static bool g_is_remote(const char * path);//path is AFTER get_canonical_path
		//these below do get_canonical_path internally
		static t_io_result g_open(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort);
		static void g_open_e(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort);
		static t_io_result g_open_ex(service_ptr_t<file> & p_out,const char * path,t_open_mode mode,abort_callback & p_abort);//get_canonical_path + open
		static t_io_result g_open_write_new(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort);
		static void g_open_write_new_e(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort);
		static t_io_result g_open_read(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort) {return g_open(p_out,path,open_mode_read,p_abort);}
		static t_io_result g_open_precache(service_ptr_t<file> & p_out,const char * path,abort_callback & p_abort);//open only for precaching data (eg. will fail on http etc)
		static bool g_exists(const char * p_path,abort_callback & p_abort);
		static bool g_exists_writeable(const char * p_path,abort_callback & p_abort);
		static t_io_result g_remove(const char * path,abort_callback & p_abort);
		static t_io_result g_move(const char * src,const char * dst,abort_callback & p_abort);//needs canonical path
		static t_io_result g_move_ex(const char * src,const char * dst,abort_callback & p_abort);//converts to canonical path first
		static t_io_result g_copy(const char * src,const char * dst,abort_callback & p_abort);//needs canonical path
		static t_io_result g_copy_directory(const char * src,const char * dst,abort_callback & p_abort);//needs canonical path
		static t_io_result g_copy_ex(const char * src,const char * dst,abort_callback & p_abort);//converts to canonical path first
		static t_io_result g_get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort);
		static t_io_result g_get_stats_ex(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort);
		static bool g_relative_path_create(const char * file_path,const char * playlist_path,string_base & out);
		static bool g_relative_path_parse(const char * relative_path,const char * playlist_path,string_base & out);
		
		static t_io_result g_create_directory(const char * p_path,abort_callback & p_abort);
		static void g_create_directory_e(const char * p_path,abort_callback & p_abort);

		static FILE * streamio_open(const char * path,const char * flags); // if for some bloody reason you ever need stream io compatibility, use this, INSTEAD of calling fopen() on the path string you've got; will only work with file:// (and not with http://, unpack:// or whatever)

		static t_io_result g_open_temp(service_ptr_t<file> & p_out,abort_callback & p_abort);
		static t_io_result g_open_tempmem(service_ptr_t<file> & p_out,abort_callback & p_abort);

		static void g_open_temp_e(service_ptr_t<file> & p_out,abort_callback & p_abort);
		static void g_open_tempmem_e(service_ptr_t<file> & p_out,abort_callback & p_abort);

		static t_io_result g_list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort);// path must be canonical
		static t_io_result g_list_directory_ex(const char * p_path,directory_callback & p_out,abort_callback & p_abort);// gets canonical path first

		static bool g_is_valid_directory(const char * path,abort_callback & p_abort);
		static bool g_is_empty_directory(const char * path,abort_callback & p_abort);


		static const GUID class_guid;

		virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
			if (p_guid == class_guid) {p_out = this; return true;}
			else return service_base::service_query(p_out,p_guid);
		}
	protected:
		filesystem() {}
		~filesystem() {}
	};

	class directory_callback_i : public directory_callback
	{
		struct t_entry
		{
			string_simple m_path;
			t_filestats m_stats;
			t_entry(const char * p_path, const t_filestats & p_stats) : m_path(p_path), m_stats(p_stats) {}
		};
		ptr_list_t<t_entry> data;
		bool m_recur;

		static int sortfunc(const t_entry * p1, const t_entry* p2) {return stricmp_utf8(p1->m_path,p2->m_path);}
	public:
		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats);

		directory_callback_i(bool p_recur) : m_recur(p_recur) {}
		~directory_callback_i() {data.delete_all();}
		unsigned get_count() {return data.get_count();}
		const char * operator[](unsigned n) {return data[n]->m_path;}
		const char * get_item(unsigned n) {return data[n]->m_path;}
		const t_filestats & get_item_stats(unsigned n) {return data[n]->m_stats;}
		void sort() {data.sort_t(sortfunc);}
	};

	class archive;

	class NOVTABLE archive_callback : public abort_callback
	{
	public:
		virtual bool on_entry(archive * owner,const char * url,const t_filestats & p_stats,const service_ptr_t<file> & p_reader) = 0;
	};

	class NOVTABLE archive : public filesystem//dont derive from this, use archive_i class below
	{
	public:
		virtual t_io_result archive_list(const char * p_path,const service_ptr_t<file> & p_reader,archive_callback & p_callback,bool p_want_readers) = 0;
		
		static const GUID class_guid;

		virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
			if (p_guid == class_guid) {p_out = this; return true;}
			else return filesystem::service_query(p_out,p_guid);
		}
	protected:
		archive() {}
		~archive() {}
	};

	class NOVTABLE archive_impl : public archive // derive from this
	{
	private:
		//do not override these
		bool get_canonical_path(const char * path,string_base & out);
		bool is_our_path(const char * path);
		bool get_display_path(const char * path,string_base & out);
		int exists(const char * path,abort_callback & p_abort);
		t_io_result remove(const char * path,abort_callback & p_abort);
		t_io_result move(const char * src,const char * dst,abort_callback & p_abort);
		bool is_remote(const char * src);
		bool relative_path_create(const char * file_path,const char * playlist_path,string_base & out);
		bool relative_path_parse(const char * relative_path,const char * playlist_path,string_base & out);
		t_io_result open(service_ptr_t<file> & p_out,const char * path, t_open_mode mode,abort_callback & p_abort);
		t_io_result create_directory(const char * path,abort_callback &);
		t_io_result list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort);
		t_io_result get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort);
	protected:
		//override these
		virtual const char * get_archive_type()=0;//eg. "zip", must be lowercase
		virtual t_io_result get_stats_in_archive(const char * p_archive,const char * p_file,t_filestats & p_stats,abort_callback & p_abort) = 0;
		virtual t_io_result open_archive(service_ptr_t<file> & p_out,const char * archive,const char * file, abort_callback & p_abort)=0;//opens for reading
	public:
		//override these
		
		virtual t_io_result archive_list(const char * path,const service_ptr_t<file> & p_reader,archive_callback & p_out,bool p_want_readers)=0;
		//playlist_loader_callback ONLY for on_progress calls


		static bool g_parse_unpack_path(const char * path,string8 & archive,string8 & file);
		static void g_make_unpack_path(string_base & path,const char * archive,const char * file,const char * name);
		void make_unpack_path(string_base & path,const char * archive,const char * file);

	};

	template<class T>
	class archive_factory_t : public service_factory_single_t<filesystem,T>
	{
	};


	t_filetimestamp filetimestamp_from_system_timer();

	//! Warning: this formats according to system timezone settings, created strings should be used for display only, never for storage.
	class format_filetimestamp {
	public:
		format_filetimestamp(t_filetimestamp p_timestamp);
		operator const char*() const {return m_buffer;}
		const char * get_ptr() const {return m_buffer;}
	private:
		string_fixed_t<32> m_buffer;
	};
}


using namespace foobar2000_io;

//inline string_base & operator<<(string_base & p_fmt,t_filetimestamp p_timestamp) {return p_fmt << format_timestamp(p_timestamp);}
inline string_base & operator<<(string_base & p_fmt,t_io_result p_code) {return p_fmt << io_result_get_message(p_code);}

#include "filesystem_helper.h"

#endif//_FOOBAR2000_SDK_FILESYSTEM_H_