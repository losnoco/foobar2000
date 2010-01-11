
enum {
	input_flag_no_seeking = 1,
	input_flag_no_looping = 2,
	input_flag_playback = 4,
};

//! Class providing interface for retrieval of information (metadata, duration, replaygain, other tech infos) from files. Also see: file_info.

class input_info_reader : public service_base
{
public:
	//! Retrieves count of subsongs in the file. 1 for non-multisubsong-enabled inputs.
	//! Note: multi-subsong handling is disabled for remote files (see: filesystem::is_remote) for performance reasons. Remote files are always assumed to be single-subsong, with null index.
	virtual unsigned get_subsong_count() = 0;
	
	//! Retrieves identifier of specified subsong; this identifier is meant to be used in playable_location as well as a parameter for input_info_reader::get_info().
	//! @param p_index Index of subsong to query. Must be >=0 and < get_subsong_count().
	virtual t_uint32 get_subsong(unsigned p_index) = 0;
	
	//! Retrieves information about specified subsong.
	//! @param p_subsong Identifier of the subsong to query. See: input_info_reader::get_subsong(), playable_location.
	//! @param p_info file_info object to fill.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) = 0;

	//! Retrieves file stats. Equivalent to calling get_stats() on file object.
	//! @param p_stats Receives file stats.
	virtual t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	input_info_reader() {}
	~input_info_reader() {}
};

//! Class providing interface for retrieval of PCM audio data from files.

class input_decoder : public input_info_reader
{
public:
	//! Prepares to decode specified subsong; resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader). \n
	//! It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.\n
	//! Warning: this interface inherits from input_info_reader, it is legal to call any input_info_reader methods even during decoding! Call order is not defined, other than initialize() requirement before calling other input_decoder methods.\n
	//! @param p_subsong Subsong to decode. Should always be 0 for non-multi-subsong-enabled inputs.
	//!	@param p_flags Specifies additional hints for decoding process. It can be null, or a combination of one or more following constants: \n
	//!		input_flag_no_seeking - Indicates that seek() will never be called. Can be used to avoid building potentially expensive seektables when only sequential reading is needed.\n
	//!		input_flag_no_looping - Certain input implementations can be configured to utilize looping info from file formats they process and keep playing single file forever, or keep repeating it specified number of times. This flag indicates that such features should be disabled, for e.g. ReplayGain scan or conversion.\n
	//!		input_flag_playback	- Indicates that decoding process will be used for realtime playback rather than conversion. This can be used to reconfigure features that are relevant only for conversion and take a lot of resources, such as very slow secure CDDA reading.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) = 0;

	//! Reads/decodes one chunk of audio data. Use io_result_eof return value to signal end of file (no more data to return). Before calling run(), decoding must be initialized by initialize() call.
	//! @param p_chunk audio_chunk object receiving decoded data. Contents are valid only when io_result_success is returned, not with io_result_eof or one of failure codes.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result run(audio_chunk & p_chunk,abort_callback & p_abort) = 0;

	//! Seeks to specified time offset. Before seeking or other decoding calls, decoding must be initialized with initialize() call.
	//! @param p_seconds Time to seek to, in seconds. If p_seconds exceeds length of the object being decoded, return io_result_success, and then return io_result_eof from next run() call.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result seek(double p_seconds,abort_callback & p_abort) = 0;
	
	//! Queries whether resource being read/decoded is seekable. If can_seek() returns false, all seek() calls will fail. Before calling can_seek() or other decoding calls, decoding must be initialized with initialize() call.
	virtual bool can_seek() = 0;

	//! This function is used to signal dynamic VBR bitrate, etc. Called after each run() (or not called at all if caller doesn't care about dynamic info).
	//! @param p_out - initially contains currently displayed info (either last get_dynamic_info result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta - Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns Return false to keep old info, or true to indicate that you've made changes to p_info and those should be displayed.
	virtual bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) = 0;

	//! This function is used to signal dynamic live stream song titles etc. Called after each run() (or not called at all if caller doesn't care about dynamic info). The difference between this and get_dynamic_info() is frequency and relevance of dynamic info changes - get_dynamic_info_track() returns new info only on track change in the stream, returning new titles etc.
	//! @param p_out - initially contains currently displayed info (either last get_dynamic_info_track result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta - Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns Return false to keep old info, or true to indicate that you've made changes to p_info and those should be displayed.
	virtual bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) = 0;

	//! Called from playback thread before sleeping.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void on_idle(abort_callback & p_abort) = 0;


	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return input_info_reader::service_query(p_out,p_guid);
	}
protected:
	input_decoder() {}
	~input_decoder() {}
};

//! Class providing interface for writing metadata and replaygain info to files. Also see: file_info.

class input_info_writer : public input_info_reader
{
public:
	//! Tells the service to update file tags with new info for specified subsong.
	//! @param p_subsong Subsong to update. Should be always 0 for non-multisubsong-enabled inputs.
	//! @param p_info New info to write. Sometimes not all contents of p_info can be written. Caller will typically read info back after successful write, so e.g. tech infos that change with retag are properly maintained.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual t_io_result set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) = 0;
	
	//! Commits pending updates. In case of multisubsong inputs, set_info should queue the update and perform actual file access in commit(). Otherwise, actual writing can be done in set_info() and then Commit() can just do nothing and always succeed.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual t_io_result commit(abort_callback & p_abort) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return input_info_reader::service_query(p_out,p_guid);
	}
protected:
	input_info_writer() {}
	~input_info_writer() {}
};

class input_entry : public service_base
{
public:
	//! Determines whether specified content type can be handled by this input.
	//! @param p_type Content type string to test.
	virtual bool is_our_content_type(const char * p_type)=0;

	//! Determines whether specified file type can be handled by this input.
	//! @param p_full_path Full URL of file being tested.
	//! @param p_extension Extension of file being tested, provided by caller for performance reasons.
	virtual bool is_our_path(const char * p_full_path,const char * p_extension)=0;
	
	//! Opens specified resource for decoding.
	//! @param p_instance Receives new input_decoder instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for reading info.
	//! @param p_instance Receives new input_info_reader instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for writing info.
	//! @param p_instance Receives new input_info_writer instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual t_io_result open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Reserved for future use. Do nothing and return io_result_success until specifications are finalized.
	virtual t_io_result get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort) = 0;

	

	enum {
		//! Indicates that this service implements some kind of redirector that opens another input for decoding, used to avoid circular call possibility.
		flag_redirect = 1,
		//! Indicates that multi-CPU optimizations should not be used.
		flag_parallel_reads_slow = 2,
	};
	//! See flag_* enums.
	virtual unsigned get_flags() = 0;

	inline bool is_redirect() {return (get_flags() & flag_redirect) != 0;}
	inline bool are_parallel_reads_slow() {return (get_flags() & flag_parallel_reads_slow) != 0;}

	static bool g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path);
	static bool g_find_service_by_content_type(service_ptr_t<input_entry> & p_out,const char * p_content_type);
	static t_io_result g_open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static t_io_result g_open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static t_io_result g_open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static bool g_is_supported_path(const char * p_path);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	input_entry() {}
	~input_entry() {}
};

enum t_input_open_reason {
	input_open_info_read,
	input_open_decode,
	input_open_info_write
};

//! Helper function for input implementation use; ensures that file is open with relevant access mode. This is typically called from input_impl::open() and such.
//! @param p_file File object pointer to process. If passed pointer is non-null, the function does nothing and always succeeds; otherwise it attempts to open the file using filesystem API methods.
//! @param p_path Path to the file.
//! @param p_reason Type of input operation requested. See: input_impl::open() parameters.
//! @param p_abort abort_callback object signaling user aborting the operation.
t_io_result input_open_file_helper(service_ptr_t<file> & p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

//! Helper function; calls input_open_file_helper() with specified parameters and throws an exception_io exception on failure. See input_open_file_helper().
void input_open_file_helper_e(service_ptr_t<file> & p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

//! This is a class that just declares prototypes of functions that each input needs to implement. See input_decoder / input_info_reader / input_info_writer interfaces for full descriptions of member functions. Since input implementation class is instantiated using a template, you don't need to derive from input_impl as virtual functions are not used on implementation class level. Use input_factory_t template to register input class based on input_impl.
class input_impl
{
public:
	//! Opens specified file for info read / decoding / info write. This is called only once, immediately after object creation, before any other methods, and no other methods are called if open() fails.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the implementation should handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example). Typically, an input implementation that requires file access calls input_open_file_helper() function to ensure that file is open with relevant access mode (read or read/write).
	//! @param p_path URL of resource being opened.
	//! @param p_reason Type of operation requested. Possible values are: \n
	//! - input_open_info_read - info retrieval methods are valid; \n
	//! - input_open_decode - info retrieval and decoding methods are valid; \n
	//! - input_open_info_write - info retrieval and retagging methods are valid; \n
	//! Note that info retrieval methods are valid in all cases, and they may be called at any point of decoding/retagging process. Results of info retrieval methods (other than get_subsong_count() / get_subsong()) between retag_set_info() and retag_commit() are undefined however; those should not be called during that period.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	t_io_result open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

	//! See: input_info_reader::get_subsong_count(). Valid after open() with any reason.
	unsigned get_subsong_count();
	//! See: input_info_reader::get_subsong(). Valid after open() with any reason.
	t_uint32 get_subsong(unsigned p_index);
	//! See: input_info_reader::get_info(). Valid after open() with any reason.
	t_io_result get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort);
	//! See: input_info_reader::get_file_stats(). Valid after open() with any reason.
	t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort);

	//! See: input_decoder::initialize(). Valid after open() with input_open_decode reason.
	t_io_result decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort);
	//! See: input_decoder::run(). Valid after decode_initialize().
	t_io_result decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
	//! See: input_decoder::seek(). Valid after decode_initialize().
	t_io_result decode_seek(double p_seconds,abort_callback & p_abort);
	//! See: input_decoder::can_seek(). Valid after decode_initialize().
	bool decode_can_seek();
	//! See: input_decoder::get_dynamic_info(). Valid after decode_initialize().
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::get_dynamic_info_track(). Valid after decode_initialize().
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::on_idle(). Valid after decode_initialize().
	void decode_on_idle(abort_callback & p_abort);

	//! See: input_info_writer::set_info(). Valid after open() with input_open_info_write reason.
	t_io_result retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort);
	//! See: input_info_writer::commit(). Valid after open() with input_open_info_write reason.
	t_io_result retag_commit(abort_callback & p_abort);	
	
	//! See: input_entry::is_our_content_type().
	static bool g_is_our_content_type(const char * p_content_type);
	//! See: input_entry::is_our_path().
	static bool g_is_our_path(const char * p_path,const char * p_extension);

protected:
	input_impl() {}
	~input_impl() {}
};

//! This is a class that just declares prototypes of functions that each non-multitrack-enabled input needs to implement. See input_decoder / input_info_reader / input_info_writer interfaces for full descriptions of member functions. Since input implementation class is instantiated using a template, you don't need to derive from input_singletrack_impl as virtual functions are not used on implementation class level. Use input_singletrack_factory_t template to register input class based on input_singletrack_impl.
class input_singletrack_impl
{
public:
	//! Opens specified file for info read / decoding / info write. This is called only once, immediately after object creation, before any other methods, and no other methods are called if open() fails.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the implementation should handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example). Typically, an input implementation that requires file access calls input_open_file_helper() function to ensure that file is open with relevant access mode (read or read/write).
	//! @param p_path URL of resource being opened.
	//! @param p_reason Type of operation requested. Possible values are: \n
	//! - input_open_info_read - info retrieval methods are valid; \n
	//! - input_open_decode - info retrieval and decoding methods are valid; \n
	//! - input_open_info_write - info retrieval and retagging methods are valid; \n
	//! Note that info retrieval methods are valid in all cases, and they may be called at any point of decoding/retagging process. Results of info retrieval methods (other than get_subsong_count() / get_subsong()) between retag_set_info() and retag_commit() are undefined however; those should not be called during that period.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	t_io_result open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);

	//! See: input_info_reader::get_info(). Valid after open() with any reason. \n
	//! Implementation warning: this is typically also called immediately after tag update and should return newly written content then.
	t_io_result get_info(file_info & p_info,abort_callback & p_abort);
	//! See: input_info_reader::get_file_stats(). Valid after open() with any reason. \n
	//! Implementation warning: this is typically also called immediately after tag update and should return new values then.
	t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort);

	//! See: input_decoder::initialize(). Valid after open() with input_open_decode reason.
	t_io_result decode_initialize(unsigned p_flags,abort_callback & p_abort);
	//! See: input_decoder::run(). Valid after decode_initialize().
	t_io_result decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
	//! See: input_decoder::seek(). Valid after decode_initialize().
	t_io_result decode_seek(double p_seconds,abort_callback & p_abort);
	//! See: input_decoder::can_seek(). Valid after decode_initialize().
	bool decode_can_seek();
	//! See: input_decoder::get_dynamic_info(). Valid after decode_initialize().
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::get_dynamic_info_track(). Valid after decode_initialize().
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
	//! See: input_decoder::on_idle(). Valid after decode_initialize().
	void decode_on_idle(abort_callback & p_abort);

	//! See: input_info_writer::set_info(). Note that input_info_writer::commit() call isn't forwarded because it's useless in case of non-multitrack-enabled inputs. Valid after open() with input_open_info_write reason.
	t_io_result retag(const file_info & p_info,abort_callback & p_abort);
	
	//! See: input_entry::is_our_content_type().
	static bool g_is_our_content_type(const char * p_content_type);
	//! See: input_entry::is_our_path().
	static bool g_is_our_path(const char * p_path,const char * p_extension);

protected:
	input_singletrack_impl() {}
	~input_singletrack_impl() {}
};


//! Misc helper, documentme.
class input_singletrack_file_impl
{
public:
	t_io_result open(service_ptr_t<file> p_file,t_input_open_reason p_reason,abort_callback & p_abort);

	t_io_result get_info(file_info & p_info,abort_callback & p_abort);
	t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort);

	t_io_result decode_initialize(unsigned p_flags,abort_callback & p_abort);
	t_io_result decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
	t_io_result decode_seek(double p_seconds,abort_callback & p_abort);
	bool decode_can_seek();
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
	void decode_on_idle(abort_callback & p_abort);

	t_io_result retag(file_info & p_info,abort_callback & p_abort);
	
	static bool g_is_our_content_type(const char * p_content_type);
	static bool g_is_our_path(const char * p_path,const char * p_extension);

protected:
	input_singletrack_file_impl() {}
	~input_singletrack_file_impl() {}
};

//! Used internally by standard input_entry implementation; do not use directly. Translates input_decoder / input_info_reader / input_info_writer calls to input_impl calls.
template<class I, class t_base>
class input_impl_interface_wrapper_t : public t_base
{
public:
	t_io_result open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		return m_instance.open(p_filehint,p_path,p_reason,p_abort);
	}

	// input_info_reader methods

	unsigned get_subsong_count() {
		return m_instance.get_subsong_count();
	}
	
	t_uint32 get_subsong(unsigned p_index) {
		return m_instance.get_subsong(p_index);
	}

	
	t_io_result get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		return m_instance.get_info(p_subsong,p_info,p_abort);
	}

	t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort) {
		return m_instance.get_file_stats(p_stats,p_abort);
	}

	// input_decoder methods

	t_io_result initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		return m_instance.decode_initialize(p_subsong,p_flags,p_abort);
	}

	t_io_result run(audio_chunk & p_chunk,abort_callback & p_abort) {
		return m_instance.decode_run(p_chunk,p_abort);
	}

	t_io_result seek(double p_seconds,abort_callback & p_abort) {
		return m_instance.decode_seek(p_seconds,p_abort);
	}
	
	bool can_seek() {
		return m_instance.decode_can_seek();
	}

	bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		return m_instance.decode_get_dynamic_info(p_out,p_timestamp_delta);
	}

	bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) 	{
		return m_instance.decode_get_dynamic_info_track(p_out,p_timestamp_delta);
	}

	void on_idle(abort_callback & p_abort) {
		m_instance.decode_on_idle(p_abort);
	}

	// input_info_writer methods

	t_io_result set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) {
		return m_instance.retag_set_info(p_subsong,p_info,p_abort);
	}
	
	t_io_result commit(abort_callback & p_abort) {
		return m_instance.retag_commit(p_abort);
	}
	
private:
	I m_instance;
};

//! Helper used by input_singletrack_factory_t, do not use directly. Translates input_impl calls to input_singletrack_impl calls. 
template<class I>
class input_wrapper_singletrack_t
{
public:
	input_wrapper_singletrack_t() {}

	t_io_result open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort)
	{
		return m_instance.open(p_filehint,p_path,p_reason,p_abort);
	}

	t_io_result get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort)
	{
		if (p_subsong != 0) return io_result_error_data;

		return m_instance.get_info(p_info,p_abort);
	}

	

	inline unsigned get_subsong_count()
	{
		return 1;
	}
	
	inline t_uint32 get_subsong(unsigned p_index)
	{
		assert(p_index == 0);
		return 0;
	}

	t_io_result get_file_stats(t_filestats & p_stats,abort_callback & p_abort)
	{
		return m_instance.get_file_stats(p_stats,p_abort);
	}

	t_io_result decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort)
	{
		if (p_subsong != 0) return io_result_error_data;
		return m_instance.decode_initialize(p_flags,p_abort);
	}

	inline t_io_result decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {return m_instance.decode_run(p_chunk,p_abort);}
	inline t_io_result decode_seek(double p_seconds,abort_callback & p_abort) {return m_instance.decode_seek(p_seconds,p_abort);}
	inline bool decode_can_seek() {return m_instance.decode_can_seek();}
	inline bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {return m_instance.decode_get_dynamic_info(p_out,p_timestamp_delta);}
	inline bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {return m_instance.decode_get_dynamic_info_track(p_out,p_timestamp_delta);}
	inline void decode_on_idle(abort_callback & p_abort) {m_instance.decode_on_idle(p_abort);}

	t_io_result retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort)
	{
		if (p_subsong != 0) return io_result_error_data;
		return m_instance.retag(p_info,p_abort);
	}
	
	t_io_result retag_commit(abort_callback & p_abort)
	{
		return io_result_success;
	}

	static bool g_is_our_content_type(const char * p_content_type) {return I::g_is_our_content_type(p_content_type);}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return I::g_is_our_path(p_path,p_extension);}

	
private:
	I m_instance;
};

//! Helper; standard input_entry implementation. Do not instantiate this directly, use input_factory_t or one of other input_*_factory_t helpers instead.
template<class I,unsigned t_flags>
class input_entry_impl_t : public input_entry
{
private:
	
	template<class T>
	t_io_result instantiate_t(service_ptr_t<T> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort)
	{
		service_impl_t<input_impl_interface_wrapper_t<I,T> > * temp;
		temp = new service_impl_t<input_impl_interface_wrapper_t<I,T> >();
		if (temp == 0) return io_result_error_out_of_memory;
		t_io_result status;
		status = temp->open(p_filehint,p_path,p_reason,p_abort);
		if (io_result_failed(status)) {delete temp;return status;}
		p_instance = temp;
		return io_result_success;
	}
public:
	bool is_our_content_type(const char * p_type) {return I::g_is_our_content_type(p_type);}
	bool is_our_path(const char * p_full_path,const char * p_extension) {return I::g_is_our_path(p_full_path,p_extension);}
	
	t_io_result open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort)
	{
		return instantiate_t(p_instance,p_filehint,p_path,input_open_decode,p_abort);
	}

	t_io_result open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort)
	{
		return instantiate_t(p_instance,p_filehint,p_path,input_open_info_read,p_abort);
	}

	t_io_result open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort)
	{
		return instantiate_t(p_instance,p_filehint,p_path,input_open_info_write,p_abort);
	}

	t_io_result get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort)
	{
		p_out.reset();
		return io_result_success;
	}

	unsigned get_flags() {return t_flags;}
};


//! Stardard input factory. For reference of functions that must be supported by registered class, see input_impl.\n Usage: static input_factory_t<myinputclass> g_myinputclass_factory;\n Note that input classes can't be registered through service_factory_t template directly.
template<class T>
class input_factory_t : public service_factory_single_t<input_entry,input_entry_impl_t<T,0> > {};

//! Non-multitrack-enabled input factory (helper) - hides multitrack management functions from input implementation; use this for inputs that handle file types where each physical file can contain only one user-visible playable track. For reference of functions that must be supported by registered class, see input_singletrack_impl.\n Usage: static input_singletrack_factory_t<myinputclass> g_myinputclass_factory;\n Note that input classes can't be registered through service_factory_t template directly.template<class T>
template<class T>
class input_singletrack_factory_t : public service_factory_single_t<input_entry,input_entry_impl_t<input_wrapper_singletrack_t<T>,0> > {};

//! Extended version of input_factory_t, with non-default flags. See: input_factory_t, input_entry::get_flags().
template<class T,unsigned t_flags>
class input_factory_ex_t : public service_factory_single_t<input_entry,input_entry_impl_t<T,t_flags> > {};

//! Extended version of input_singletrack_factory_t, with non-default flags. See: input_singletrack_factory_t, input_entry::get_flags().
template<class T,unsigned t_flags>
class input_singletrack_factory_ex_t : public service_factory_single_t<input_entry,input_entry_impl_t<input_wrapper_singletrack_t<T>,t_flags> > {};