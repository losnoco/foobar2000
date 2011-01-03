#include "stdafx.h"
#include <lib4pm.h>


// {8F450CB3-A083-4b83-8D85-ADCE5EA6D57F}
static const GUID packet_decoder_owner_alac = 
{ 0x8f450cb3, 0xa083, 0x4b83, { 0x8d, 0x85, 0xad, 0xce, 0x5e, 0xa6, 0xd5, 0x7f } };


bool is_valid_aac_decoder_config(const void * data,unsigned bytes);

const t_uint32 g_codec_alac = 'alac';//meh

static unsigned find_track_to_decode(lib4pm::mp4file & p_file,unsigned & p_codec)
{
    /* find AAC track */
    unsigned n, m = p_file.get_track_count();

    for (n = 0; n < m; n++)
    {
		unsigned type = p_file.get_track_type(n);
		if (type==lib4pm::track_audio)
		{
			p_codec = p_file.get_track_audio_type(n);
			return n;
		}
		else if (type == lib4pm::track_alac)
		{
			p_codec = g_codec_alac;
			return n;
		}
    }
	throw lib4pm::exception("cannot find audio track to decode");
}

static const char * tech_info_fields[] = {"tool"};

static const char * check_tech_info(const char * name)
{
	unsigned n;
	for(n=0;n<tabsize(tech_info_fields);n++)
	{
		if (!stricmp_utf8(name,tech_info_fields[n])) return tech_info_fields[n];
	}
	return 0;
}

static void meta_mp4_to_fb2k(const char * name,const char * value,file_info & out)
{
	{
		const char * p_tech_info = check_tech_info(name);
		if (p_tech_info)
		{
			out.info_set(p_tech_info,value);
			return;
		}
	}

	unsigned index = out.meta_find(name);
	if (index == infinite) out.meta_set(name,value);
	else out.meta_add_value(index,value);
}

class file_callback_impl : public lib4pm::file_callback_readwrite
{
public:
	inline file_callback_impl(const service_ptr_t<file> & p_file,abort_callback & p_abort) : m_file(p_file), m_abort(p_abort) {}


	lib4pm::t_uint64 skip(lib4pm::t_uint64 p_length)
	{
		if (p_length > 256 && m_file->can_seek())
		{
			
			t_uint64 max = m_file->get_size_e(m_abort);
			if (max != filesize_invalid)
			{
				t_uint64 position = m_file->get_position_e(m_abort);
				if (position > max) throw io_result_error_generic;
				max -= position;
				if (p_length > max) p_length = max;

				m_file->seek_e(position + p_length, m_abort);
				return p_length;
			}
		}

		return m_file->skip_e(p_length,m_abort);
	}

	unsigned read(void * p_buffer,unsigned p_length)
	{
		return m_file->read_e(p_buffer,p_length,m_abort);
	}

	void write(const void * p_buffer,unsigned p_length)
	{
		m_file->write_object_e(p_buffer,p_length,m_abort);
	}

	t_uint64 get_position()
	{
		return m_file->get_position_e(m_abort);
	}

	void seek(t_uint64 p_offset)
	{
		if (!m_file->can_seek())
		{
			t_uint64 position = m_file->get_position_e(m_abort);
			if (position > p_offset)
			{
				console::info(uStringPrintf("bad seek attempt: %u => %u",(unsigned)position,(unsigned)p_offset));
				throw lib4pm::exception("stream seeking error");
			}
			else if (position < p_offset)
			{
//				console::info(uStringPrintf("skip-seek attempt: %u => %u",(unsigned)position,(unsigned)p_offset));
				m_file->skip_object_e(p_offset-position,m_abort);
			}
		}
		else
		{
			m_file->seek_e(p_offset,m_abort);
		}
	}
	void truncate()
	{
		m_file->set_eof_e(m_abort);
	}


private:
	abort_callback & m_abort;
	service_ptr_t<file> m_file;

};

class input_mp4 : public input
{
public:
    unsigned m_track;

    lib4pm::mp4file infile;
    unsigned m_current_frame, m_num_frames;
	unsigned m_offset;

	service_ptr_t<file> m_file;

	service_ptr_t<packet_decoder> m_decoder;

	double m_length;
	unsigned m_timescale;
	unsigned m_skip_samples;
	unsigned m_skip_frames;
	double m_timescale_div;
	unsigned m_expected_sample_rate,m_expected_channels;

	dynamic_bitrate_helper m_vbr_handler;

	mem_block m_buffer;

	t_int64 duration_to_samples(t_int64 val)
	{
		return (t_int64) ( (double)val * (double) m_expected_sample_rate * m_timescale_div + 0.5);
	}

	t_int64 samples_to_duration(t_int64 val)
	{
		return (t_int64) ( (double)val * (double)m_timescale / (double)m_expected_sample_rate + 0.5);
	}

	audio_chunk_i m_tempchunk;

	void cleanup()
	{
		try {infile.close();}
		catch(lib4pm::exception const & e) {console::info(e.get_message());}

		m_decoder.release();
		m_vbr_handler.reset();
	}

    input_mp4()
    {

		m_skip_frames = 0;
		m_offset = 0;
		m_skip_samples = 0;

		m_vbr_handler.reset();

    }

    ~input_mp4()
    {
		cleanup();
    }

    inline static bool g_test_filename(const char * fn,const char * ext)
    {
        return !stricmp(ext,"MP4") || !stricmp(ext,"M4A") || !stricmp(ext,"M4P");
    }


	void process_track_info(file_info & p_info)
	{
		lib4pm::t_track_stats stats;
		infile.get_track_stats(m_track,stats);

		m_num_frames = stats.m_total_frames;

		m_expected_sample_rate = stats.m_sample_rate;
		m_expected_channels = stats.m_channels;
		m_timescale = stats.m_timescale;

		if (m_expected_sample_rate == 0)
		{
			console::info("MP4: invalid sample rate.");
			throw io_result_error_data;
		}

		if (m_expected_channels == 0)
		{
			console::info("MP4: invalid channel count.");
			throw io_result_error_data;
		}
		
		if (m_timescale == 0)
		{
			console::info("MP4: invalid time scale.");
			throw io_result_error_data;
		}
/*		if (m_timescale != m_expected_sample_rate)
		{
			console::error("MP4: different sample rate / time scales not supported.");
			return false;
		}*/

		m_timescale_div = 1.0 / (double) m_timescale;


		if (stats.m_duration == (t_uint64)(-1))
			m_length = -1.0;
		else
		{
			m_length = (double)stats.m_duration * m_timescale_div;
		}

		p_info.set_length(m_length);

		if (stats.m_bitrate > 0) p_info.info_set_int("bitrate",(stats.m_bitrate + 500) / 1000);
		
		if (stats.m_bitspersample > 0) p_info.info_set_int("bitspersample",stats.m_bitspersample);

		p_info.info_set_int("channels",m_expected_channels);
		p_info.info_set_int("samplerate",m_expected_sample_rate);
//		p_info.info_set_int("mp4_timescale",m_timescale);
	}

	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_mp4::get_info");

		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		TRACK_CALL_TEXT("input_mp4::open");

		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		try {
			open_internal_e(p_file,p_location,p_info,p_abort,p_decode,p_want_info);
			return io_result_success;
		}
		catch(t_io_result status) {cleanup();return status;}
		catch(lib4pm::exception const & e) {console::info(e.get_message());cleanup();return io_result_error_data;}
	}

	void open_internal_e(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
    {
		unsigned codectype = 0;

		cleanup();

		m_file = p_file;


		
		{
			unsigned flags = 0;
			if (p_decode) flags |= lib4pm::open_flag_decoding;
			if (p_want_info) flags |= lib4pm::open_flag_meta|lib4pm::open_flag_miscinfo;
			infile.open(file_callback_impl(m_file,p_abort),m_file->can_seek(),flags);
		}	
	
		m_track = find_track_to_decode(infile,codectype);

		//info->info_set("codec",codecname);

		process_track_info (p_info);

		{
			lib4pm::array_t<lib4pm::t_uint8> buffer;

			infile.get_track_decoder_config(m_track, buffer);


			bool success;
			
			if (codectype == g_codec_alac)
			{
				if (p_decode) success = packet_decoder::g_open(m_decoder,packet_decoder_owner_alac,0,buffer.ptr(),buffer.size(),p_info);
				else
				{
					p_info.info_set("codec","ALAC");
					success = false;
				}
			}
			else
			{
				success = packet_decoder::g_open(m_decoder,packet_decoder::owner_MP4,codectype,buffer.ptr(),buffer.size(),p_info);
			}

			if (!success && p_decode)
			{
				cleanup();

				if (codectype == g_codec_alac)
				{
					console::info("Error opening ALAC decoder.");
				}
				else
				{
					console::info(uStringPrintf("Unable to find correct packet decoder object (%08X).",codectype));
				}
				throw io_result_error_data;
			}
		}

		m_expected_channels = (unsigned)p_info.info_get_int("channels");
		m_expected_sample_rate = (unsigned)p_info.info_get_int("samplerate");

		if (m_expected_channels==0 || m_expected_sample_rate==0)
		{
			cleanup();
			console::info("Decoder returned invalid info.");
			throw io_result_error_data;
		}


		if (p_want_info)
		{
			lib4pm::t_tagdata const & tag = infile.get_tag();

			unsigned n;

			for(n=0;n<tag.m_meta.size();n++)
			{
				meta_mp4_to_fb2k(tag.m_meta[n].m_name.ptr(),tag.m_meta[n].m_value.ptr(),p_info);
			}

			replaygain_info rg;
			rg.reset();
			if (tag.m_replaygain.is_album_gain_present()) rg.m_album_gain = (float)tag.m_replaygain.m_album_gain / 10000;
			if (tag.m_replaygain.is_track_gain_present()) rg.m_track_gain = (float)tag.m_replaygain.m_track_gain / 10000;
			if (tag.m_replaygain.is_album_peak_present()) rg.m_album_peak = (float)tag.m_replaygain.m_album_peak / 0x1000000;
			if (tag.m_replaygain.is_track_peak_present()) rg.m_track_peak = (float)tag.m_replaygain.m_track_peak / 0x1000000;
			p_info.set_replaygain(rg);

		}


		m_skip_samples = 0;

		m_offset = 0;

		m_current_frame = 0;

		if (p_decode) seek_e(0,true,p_abort);
    }

	bool run_e(audio_chunk * p_chunk,abort_callback & p_abort)
	{
		if (m_decoder.is_empty()) throw io_result_error_generic;

		if (m_current_frame >= m_num_frames) return false;

		bool done = false;

		do		
		{
			unsigned buffer_size = infile.get_frame_size(m_track,m_current_frame);

			if (buffer_size == 0)
			{
				console::info(uStringPrintf("MP4: invalid MP4 frame size: frame %u of %u.",m_current_frame,m_num_frames));
				throw io_result_error_data;
			}

			if (!m_buffer.check_size(buffer_size))
			{
				console::info(uStringPrintf("MP4: error allocating memory for MP4 frame: frame %u of %u.",m_current_frame,m_num_frames));
				throw io_result_error_generic;
			}


			try {
				buffer_size = infile.read_frame(m_track,m_current_frame,file_callback_impl(m_file,p_abort),m_buffer.get_ptr(),buffer_size);
			} catch(t_io_result code) {
				console::info(uStringPrintf("reading from MP4 file failed: frame %u of %u.",m_current_frame,m_num_frames));
				throw code;
			}

			m_tempchunk.reset();
			if (!m_decoder->decode(m_buffer.get_ptr(),buffer_size,&m_tempchunk))
			{
				console::info("decode error.");
				throw io_result_error_data;
			}

			if (m_skip_frames>0) m_skip_frames--;
			else
			{
				unsigned duration = (unsigned)duration_to_samples(infile.get_frame_duration(m_track, m_current_frame));
	//			console::info(uStringPrintf("duration: %u, offset: %u",duration,offset));

				if (m_tempchunk.is_empty())
				{
					if (duration > 0)
					{
						m_tempchunk.set_srate(m_expected_sample_rate);
						m_tempchunk.set_channels(m_expected_channels);
						m_tempchunk.pad_with_silence(duration);
					//	console::warning("MP4: decoder returned empty chunk from a non-empty MP4 frame.");
					}
				}
				else if (!m_tempchunk.is_valid())
				{
					console::info("decoder produced invalid chunk.");
					throw io_result_error_data;
				}
				else
				{
					m_vbr_handler.on_frame(m_tempchunk.get_duration(),buffer_size * 8);

					if (m_tempchunk.get_srate() != m_expected_sample_rate)
					{
						console::info(uStringPrintf("expected sample rate: %u, got: %u.",m_expected_sample_rate,m_tempchunk.get_srate()));
						throw io_result_error_data;
					}
/*					if (m_tempchunk.get_channels() != m_expected_channels)
					{
						cleanup();
						console::error(uStringPrintf("MP4: expected channels: %u, got: %u.",m_expected_channels,m_tempchunk.get_channels()));
						return -1;
					}*/
				}
				unsigned samplerate,channels,decoded_sample_count;

				samplerate = m_tempchunk.get_srate();
				channels = m_tempchunk.get_channels();
				decoded_sample_count = m_tempchunk.get_sample_count();

				if (decoded_sample_count < duration)
				{
					//console::warning("MP4: decoded MP4 frame smaller than expected.");
					decoded_sample_count = duration;
					m_tempchunk.pad_with_silence(decoded_sample_count);
				}

				unsigned offset = 0;

				if (m_skip_samples>0)
				{
					unsigned delta = (unsigned)m_skip_samples;
					if (delta > duration) delta = duration;
					offset += delta;
					duration -= delta;
					m_skip_samples -= delta;
				}


				if (duration > 0)
				{
					p_chunk->set_data_64(m_tempchunk.get_data() + offset * channels,duration,channels,samplerate);
					done = true;
				}
			}
			m_current_frame++;
		}
		while(!done && m_current_frame < m_num_frames);
		
		if (!done) throw io_result_error_data;

		return true;
	}

    t_io_result run(audio_chunk * p_chunk,abort_callback & p_abort)
    {
		TRACK_CALL_TEXT("input_mp4::run");

		try { return run_e(p_chunk,p_abort) ? io_result_success : io_result_eof; }
		catch(t_io_result code) {cleanup();return code;}
		catch(lib4pm::exception const & e) {cleanup();console::info(e.get_message());return io_result_error_data;}
    }

	t_io_result set_info(const service_ptr_t<file> & p_file, const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_mp4::set_info");

		try { set_info_e(p_file,p_location,p_info,p_abort); return io_result_success; }
		catch(t_io_result code) {return code;}
		catch(lib4pm::exception const & e) {console::info(e.get_message());return io_result_error_data;}
	}

    static inline void set_info_e(const service_ptr_t<file> & p_file, const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
    {
	
		lib4pm::t_tagdata tag;

		{
			unsigned total = 0;
			unsigned n, m;
			
			m = p_info.meta_get_count();
			for(n=0;n<m;n++)
				total += p_info.meta_enum_value_count(n);

			m = p_info.info_get_count();

			for(n=0;n<m;n++)
			{
				if (check_tech_info(p_info.info_enum_name(n)))
					total++;
			}
			
			tag.m_meta.resize(total);
		}


		{
			unsigned tagptr = 0;
			unsigned n, m;
			
			m = p_info.meta_get_count();
			for(n=0;n<m;n++)
			{
				const char * name = p_info.meta_enum_name(n);
				unsigned n1; const unsigned m1 = p_info.meta_enum_value_count(n);
				for(n1=0;n1<m1;n1++)
				{
					tag.m_meta[tagptr].m_name = name;
					tag.m_meta[tagptr].m_value = p_info.meta_enum_value(n,n1);
					tagptr++;
				}
			}


			m = p_info.info_get_count();

			for(n=0;n<m;n++)
			{
				const char * p_name = check_tech_info(p_info.info_enum_name(n));
				if (p_name)
				{
					tag.m_meta[tagptr].m_name = p_name;
					tag.m_meta[tagptr].m_value = p_info.info_enum_value(n);
					tagptr++;
				}
			}

		}


		{
			replaygain_info rg = p_info.get_replaygain();
			tag.m_replaygain.reset();
			if (rg.is_album_gain_present()) tag.m_replaygain.m_album_gain = (t_int32)(rg.m_album_gain * 10000);
			if (rg.is_track_gain_present()) tag.m_replaygain.m_track_gain = (t_int32)(rg.m_track_gain * 10000);
			if (rg.is_album_peak_present()) tag.m_replaygain.m_album_peak = (t_uint32)(rg.m_album_peak * 0x1000000);
			if (rg.is_track_peak_present()) tag.m_replaygain.m_track_peak = (t_uint32)(rg.m_track_peak * 0x1000000);
		}

		lib4pm::update_file(file_callback_impl(p_file,p_abort),tag, lib4pm::update_meta | lib4pm::update_replaygain);

    }

	void seek_e(double p_seconds,bool p_firsttime,abort_callback & p_abort)
	{
		if (m_decoder == 0)
		{
			console::info("attempting to seek while not open.");
			throw io_result_error_generic;
		}

        if (p_seconds >= m_length) {
            m_current_frame = m_num_frames;
            return;
        }
		
		unsigned max_frame_dependency = m_decoder->get_max_frame_dependency();
		t_uint64 offset = (t_uint64)(p_seconds * m_timescale + 0.5);
		lib4pm::t_uint32 skip_samples = 0;
		t_uint32 dest_frame;
		
		dest_frame = infile.find_frame(m_track,offset,skip_samples);

		if (dest_frame == (t_uint32)(-1)) throw io_result_error_data;

		if (dest_frame < max_frame_dependency)
		{
			m_skip_frames = dest_frame;
			dest_frame = 0;
		}
		else
		{
			m_skip_frames = max_frame_dependency;
			dest_frame -= max_frame_dependency;
		}

		m_current_frame = dest_frame;
		
		m_skip_samples = (unsigned)duration_to_samples(skip_samples);

		m_offset = 0;

		if (!p_firsttime) m_decoder->reset_after_seek();
		
		m_vbr_handler.reset();
	}

    virtual t_io_result seek(double p_seconds,abort_callback & p_abort)
    {
		TRACK_CALL_TEXT("input_mp4::seek");

		try {
			seek_e(p_seconds,false,p_abort);
		}
		catch(t_io_result code) {return code;}
		catch(lib4pm::exception const & e) {console::info(e.get_message());return io_result_error_data;}
    
		return io_result_success;
	}

    inline static bool g_is_our_content_type(const char *url, const char *type)
    {
        return !stricmp(type, "audio/mp4") || !stricmp(type, "audio/x-mp4");
    }

	bool get_dynamic_info(file_info & p_out,double * p_timestamp_delta,bool * p_track_change)
	{
		return m_vbr_handler.on_update(p_out,p_timestamp_delta);
	}

	bool can_seek()
	{
		return m_file.is_valid() ? m_file->can_seek() : false;
	}

	inline static bool g_needs_reader() {return true;}


	static GUID g_get_guid()
	{
		// {DFA83B4D-5A5F-4211-8FA8-BCA52576DBBE}
		static const GUID guid = 
		{ 0xdfa83b4d, 0x5a5f, 0x4211, { 0x8f, 0xa8, 0xbc, 0xa5, 0x25, 0x76, 0xdb, 0xbe } };
		return guid;
	}

	static const char * g_get_name() {return "MP4 parser";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

};

static input_factory_t<input_mp4> g_input_mp4_factory;

DECLARE_FILE_TYPE("MP4 files","*.MP4;*.M4A");