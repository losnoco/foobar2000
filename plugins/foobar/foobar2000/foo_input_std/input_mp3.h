unsigned get_frame_size_dword(unsigned long newhead);
static unsigned get_frame_size(const void * src) {return get_frame_size_dword(byte_order::dword_be_to_native(*(DWORD*)src));}

class mp3header
{
	char bytes[4];
public:

	inline mp3header(const mp3header & src) {memcpy(bytes,src.bytes,4);}
	inline mp3header(const void * src) {memcpy(bytes,src,4);}
	inline mp3header() {}

	inline void operator=(const void* src) {memcpy(bytes,src,4);}
	
	inline DWORD get_dword() const {return byte_order::dword_be_to_native(*(DWORD*)bytes);}
	inline operator DWORD() const {return get_dword();}
	
	inline void get_bytes(void * out) {memcpy(out,bytes,4);}
	inline unsigned get_frame_size() const {return ::get_frame_size(bytes);}
};


class input_mp3 : public input
{
private:
	playable_location_i m_file;
	t_filetimestamp m_timestamp;
	char buf[16384];
	audio_chunk_i m_tempchunk;
	bool eof;
	bool is_seekable,noseeking;
	class mp3_seektable * p_seektable;
	bool have_accurate_length;
	bool is_mp2,is_vbr;
	int skip_start,skip_end;
	t_uint64 id3v2_size;
	dynamic_bitrate_helper m_vbr_handler;

	service_ptr_t<packet_decoder> m_decoder;
	service_ptr_t<file> m_reader;
	service_ptr_t<file_dynamicinfo> m_reader_v2;
	t_int64 first_frame;
	t_int64 total_frames,total_bytes;
	t_int64 frames_done;
	t_int64 skip_frames;
	t_int64 skip_samples,samples_done,total_samples;
	double total_length;
	int sample_rate;
	int samples_per_frame;


	t_int64 scan_for_frame_e(const service_ptr_t<file> & r,mp3header * p_header,abort_callback & p_abort);
	t_int64 find_end_of_stream_e(t_int64 first,DWORD header,abort_callback & p_abort);
	void getinfo_e(file_info & info,bool b_full_open,bool b_want_tags,abort_callback & p_abort);
	unsigned advance_frame_e(bool read,bool ignorelength,abort_callback & p_abort);

	void seek_to_start(abort_callback & p_abort);

	t_io_result open_internal(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & info,abort_callback & p_abort,unsigned p_flags,bool p_decode);

public:
	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags);
	t_io_result get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort);
	static bool g_is_our_content_type(const char * url,const char * type);
	static bool g_test_filename(const char * fn,const char * ext) ;

	
	t_io_result set_info(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort);

	bool get_dynamic_info(file_info & out,double * timestamp_delta,bool * b_track_change);

	input_mp3();

	~input_mp3();

	t_io_result run(audio_chunk * chunk,abort_callback & p_abort);

	t_io_result seek(double seconds,abort_callback & p_abort);

	bool can_seek() {return m_reader.is_valid() ? m_reader->can_seek() : false;}

	void process_lamefix_e(const service_ptr_t<file> & p_temp,unsigned p_delay,t_int64 wanted_total_samples,bool b_rebuild,abort_callback & p_abort);

	static inline bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {11FC15D8-3404-4bf8-8EF9-F66F684268CF}
		static const GUID guid = 
		{ 0x11fc15d8, 0x3404, 0x4bf8, { 0x8e, 0xf9, 0xf6, 0x6f, 0x68, 0x42, 0x68, 0xcf } };
		return guid;
	}

	static const char * g_get_name() {return "MP2/MP3 parser";}

	static void g_purge_seektable(const char * p_path);

	static void g_parse_tagtype(const file_info & p_info,bool & p_have_id3v1,bool & p_have_id3v2,bool & p_have_apev2);

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};
