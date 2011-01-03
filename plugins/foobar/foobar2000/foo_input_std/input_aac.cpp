#include "stdafx.h"
#include <neaacdec.h>

//#define DBG_OUT(A) OutputDebugString(A)
#define DBG_OUT(A)

static const char *object_type_string(int type)
{
    static const char *types[31] = {
        "AAC Main",
        "AAC LC",
        "AAC SSR",
        "AAC LTP",
        "AAC HE",
        "AAC Scalable",
        "TwinVQ",
        "CELP",
        "HVXC",
        "Reserved",
        "Reserved",
        "TTSI",
        "Main synthetic",
        "Wavetable synthesis",
        "General MIDI",
        "Algorithmic Synthesis and Audio FX",
        "ER AAC LC",
        "Reserved",
        "ER AAC LTP",
        "ER AAC scalable",
        "ER TwinVQ",
        "ER BSAC",
        "ER AAC LD",
        "ER CELP",
        "ER HVXC",
        "ER HILN",
        "ER Parametric",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
    };

    if (type<1 || type>31) return NULL;

    return types[type-1];
}


struct seek_list
{
    seek_list *next;
    t_int64 offset;
};

class input_aac : public input
{
public:

	static bool g_needs_reader() {return true;}

    static bool g_test_filename(const char * fn,const char * ext)
    {
        return !stricmp_utf8(ext,"AAC");
    }

	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		try {
			open_internal_e(p_file,p_location,p_info,p_abort,p_decode,p_want_info);
			return io_result_success;
		} catch(t_io_result status) {
			return status;
		}
	}

	void open_internal_e(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
    {
		t_uint64 tagsize = 0;
        int tmp = 0;
        double length = 1.;
        t_int64 bitrate = 128;
        unsigned char channels = 0;
        unsigned long samplerate = 0;
        int sbr = 0;
        int header_type = 0;
        int profile = 0;

		t_io_result io_status;
		unsigned io_bytes_done;

        m_reader = p_file;
		if (!service_query_t(m_reader_v2,m_reader)) m_reader_v2.release();

        io_status = tag_processor_id3v2::g_skip(m_reader,tagsize,p_abort);
		if (io_result_failed(io_status)) throw io_status;

        if (!(m_aac_buffer = (unsigned char*)malloc(768*6)))
        {
            throw io_result_error_data;
        }

        for (int init=0; init<2; init++)
        {
            faacDecConfigurationPtr config;

            hDecoder = faacDecOpen();
            if (!hDecoder)
            {
                console::error("Failed to open FAAD2 library.");
                throw io_result_error_data;
            }

            config = faacDecGetCurrentConfiguration(hDecoder);
            config->outputFormat = FAAD_FMT_DOUBLE;
			config->dontUpSampleImplicitSBR = 1;
            faacDecSetConfiguration(hDecoder, config);

            memset(m_aac_buffer, 0, 768*6);
            io_status = m_reader->read(m_aac_buffer, 768*6,io_bytes_done,p_abort);
			if (io_result_failed(io_status)) throw io_status;
            m_aac_bytes_into_buffer = io_bytes_done;
            m_aac_bytes_consumed = 0;
            m_file_offset = tagsize;
            m_last_offset = -1;
            m_at_eof = (io_bytes_done != 768*6) ? 1 : 0;

            if (init==0)
            {
                faacDecFrameInfo frameInfo;

                io_status = fill_buffer(p_abort);
				if (io_result_failed(io_status)) throw io_status;

				// PLAY TIME!

				if (memcmp(m_aac_buffer, "ADIF", 4) &&
				!((m_aac_buffer[0] == 0xFF)&&((m_aac_buffer[1] & 0xF6) == 0xF0)))
				{
					// find syncword within limit

					while (m_aac_bytes_into_buffer > 2)
					{
						m_aac_bytes_consumed++;
						m_aac_bytes_into_buffer--;
						if ((m_aac_buffer[m_aac_bytes_consumed] == 0xFF)&&((m_aac_buffer[m_aac_bytes_consumed+1] & 0xF6) == 0xF0))
						{
							m_file_offset += m_aac_bytes_consumed;
							tagsize += m_aac_bytes_consumed;
							io_status = fill_buffer(p_abort);
							if (io_result_failed(io_status)) throw io_status;
							break;
						}
					}

					if (m_aac_bytes_into_buffer < 3)
					{
						m_aac_bytes_into_buffer += m_aac_bytes_consumed;
						m_aac_bytes_consumed = 0;
					}
				}

				if ((m_aac_bytes_consumed = faacDecInit(hDecoder,
                    m_aac_buffer, m_aac_bytes_into_buffer,
                    &samplerate, &channels)) < 0)
                {
                    console::error("Can't initialize decoder library.");
                    throw io_result_error_data;
                }
                advance_buffer(m_aac_bytes_consumed);

                do {
                    memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
                    io_status = fill_buffer(p_abort);
					if (io_result_failed(io_status)) throw io_status;
                    faacDecDecode(hDecoder, &frameInfo, m_aac_buffer, m_aac_bytes_into_buffer);
                } while (!frameInfo.samples && !frameInfo.error);

                if (frameInfo.error)
                {
                    console::error(faacDecGetErrorMessage(frameInfo.error));
                    throw io_result_error_data;
                }

                m_samplerate = frameInfo.samplerate;
                m_framesize = (frameInfo.channels != 0) ? frameInfo.samples/frameInfo.channels : 0;
                sbr = frameInfo.sbr;
                profile = frameInfo.object_type;
                header_type = frameInfo.header_type;

                faacDecClose(hDecoder);
                io_status = m_reader->seek(tagsize,p_abort);
				if (io_result_failed(io_status)) throw io_status;
            }
        }

        m_head = (struct seek_list*)malloc(sizeof(struct seek_list));
        m_tail = m_head;
        m_tail->next = NULL;

        m_header_type = 0;
        if ((m_aac_buffer[0] == 0xFF) && ((m_aac_buffer[1] & 0xF6) == 0xF0))
        {
            if (m_reader->can_seek())
            {
                io_status = adts_parse(&bitrate, &length,p_abort);
				if (io_result_failed(io_status)) throw io_status;

                m_reader->seek_e(tagsize,p_abort);

                io_bytes_done = m_reader->read_e(m_aac_buffer,768*6,p_abort);

                if (io_bytes_done != 768*6)
                    m_at_eof = 1;
                else
                    m_at_eof = 0;
                m_aac_bytes_into_buffer = io_bytes_done;
                m_aac_bytes_consumed = 0;

                m_header_type = 1;
            }
        } else if (memcmp(m_aac_buffer, "ADIF", 4) == 0) {
            int skip_size = (m_aac_buffer[4] & 0x80) ? 9 : 0;
            bitrate = ((unsigned int)(m_aac_buffer[4 + skip_size] & 0x0F)<<19) |
                ((unsigned int)m_aac_buffer[5 + skip_size]<<11) |
                ((unsigned int)m_aac_buffer[6 + skip_size]<<3) |
                ((unsigned int)m_aac_buffer[7 + skip_size] & 0xE0);

            length = (double)m_reader->get_size_e(p_abort);
            if (length == -1.)
            {
                length = 1;
            } else {
                length = ((double)length*8.)/((double)bitrate) + 0.5;
            }

            bitrate = (t_int64)((double)bitrate/1000.0 + 0.5);

            m_header_type = 2;
        }

        if (!m_reader->can_seek())
        {
            length = 0;
        }

        io_status = fill_buffer(p_abort);
		if (io_result_failed(io_status)) throw io_status;

        if ((m_aac_bytes_consumed = faacDecInit(hDecoder,
            m_aac_buffer, m_aac_bytes_into_buffer,
            &samplerate, &channels)) < 0)
        {
            console::error("Can't initialize decoder library.");
            throw io_result_error_data;
        }
        advance_buffer(m_aac_bytes_consumed);

        m_length = length;
        
        if (p_want_info) {
			p_info.set_length(m_length);

            const char *profile_str = object_type_string(profile);
            const char *header_str = NULL;

            p_info.info_set_int("bitrate", bitrate);
            p_info.info_set_int("channels", (t_int64)channels);
            p_info.info_set_int("samplerate", (t_int64)m_samplerate);

            if (profile_str)
                p_info.info_set("aac_profile", profile_str);

            if (header_type == RAW)
                header_str = "RAW";
            else if (header_type == ADIF)
                header_str = "ADIF";
            else if (header_type == ADTS)
                header_str = "ADTS";

            if (header_str)
                p_info.info_set("aac_header_type", header_str);

            if (sbr == 1 || sbr == 2) /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
                p_info.info_set("codec", "AAC+SBR");
            else
                p_info.info_set("codec", "AAC");


			t_uint64 offset_backup = m_reader->get_position_e(p_abort);

			t_io_result status;
			status = tag_processor::read_id3v2_trailing(m_reader, p_info,p_abort);
			if (status != io_result_error_not_found && status != io_result_error_data && io_result_failed(status)) throw status;

			m_reader->seek_e(offset_backup,p_abort);
        }

    }

    input_aac()
    {
        m_head = NULL;
        m_tail = NULL;
        m_samplerate = 0;
        hDecoder = NULL;
        m_aac_buffer = NULL;
        m_samples = 0;
        m_samplepos = 0;
        m_seekskip = 0;
        m_eof = false;
		m_reader = 0;
		m_reader_v2 = 0;
    }

    ~input_aac()
    {
        struct seek_list *target = m_head;

        if (hDecoder)
            faacDecClose(hDecoder);
        if (m_aac_buffer)
            free(m_aac_buffer);

        while (target)
        {
            struct seek_list *tmp = target;
            target = target->next;
            if (tmp) free(tmp);
        }
    }

    virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
    {
		t_io_result io_status;
        while (1) {
            if (m_eof || (m_samples > 0 && m_samplepos >= m_samples)) return io_result_eof; // gapless playback

            if (m_aac_bytes_into_buffer == 0) return io_result_eof;

            faacDecFrameInfo frameInfo;
            audio_sample *sample_buffer = 0;

            memset(&frameInfo, 0, sizeof(faacDecFrameInfo));

            do
            {
                io_status = fill_buffer(p_abort);
				if (io_result_failed(io_status)) return io_status;

                if (m_aac_bytes_into_buffer != 0)
                {
                    sample_buffer = (audio_sample*)faacDecDecode(hDecoder, &frameInfo,
                        m_aac_buffer, m_aac_bytes_into_buffer);

                    if (m_header_type != 1)
                    {
                        if (m_last_offset < m_file_offset)
                        {
                            m_tail->offset = m_file_offset;
                            m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                            m_tail = m_tail->next;
                            m_tail->next = NULL;
                            m_last_offset = m_file_offset;
                        }
                    }

                    advance_buffer(frameInfo.bytesconsumed);
                }

                if (frameInfo.error || !sample_buffer)
                {
                    if (!frameInfo.error) return io_result_eof; // EOF
                    const char *msg = faacDecGetErrorMessage(frameInfo.error);
                    if (msg) console::error(msg);
                    return io_result_eof;
                }

                if (m_aac_bytes_into_buffer == 0) break;
            } while (!frameInfo.samples || !frameInfo.channels);

            if (!frameInfo.samples || !frameInfo.channels) return io_result_eof;

            unsigned int samples = frameInfo.samples/frameInfo.channels;

            m_samplerate = frameInfo.samplerate;
            m_framesize = samples;

            if (m_samples > 0) { // gapless playback
                if (m_samplepos + samples > m_samples) samples = (unsigned int)(m_samples - m_samplepos);
            }

            m_samplepos += samples;

            if ((unsigned)m_seekskip < samples) {
                if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
                {
                    //channel order for 5.1: L/R/C/LF/BL/BR
                    audio_sample r1, r2, r3, r4, r5, r6;
                    for (unsigned int i = 0; i < frameInfo.samples; i += frameInfo.channels)
                    {
                        r1 = sample_buffer[i];
                        r2 = sample_buffer[i+1];
                        r3 = sample_buffer[i+2];
                        r4 = sample_buffer[i+3];
                        r5 = sample_buffer[i+4];
                        r6 = sample_buffer[i+5];
                        sample_buffer[i] = r2;
                        sample_buffer[i+1] = r3;
                        sample_buffer[i+2] = r1;
                        sample_buffer[i+3] = r6;
                        sample_buffer[i+4] = r4;
                        sample_buffer[i+5] = r5;
                    }
                }

                samples -= m_seekskip;
                if (chunk)
                {
                    chunk->set_data((audio_sample*)sample_buffer + m_seekskip*frameInfo.channels,
                        samples, frameInfo.channels, frameInfo.samplerate);
                }
                m_seekskip = 0;
                break;
            } else {
                m_seekskip -= samples;
            }
        }

        return io_result_success;
    }

    virtual t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort)
    {
		return tag_processor::write_apev2(r,info,p_abort);
    }

    virtual t_io_result seek(double seconds,abort_callback & p_abort)
    {
        unsigned int i, frames;
        unsigned io_bytes_done;
		t_io_result io_status;
        struct seek_list *target = m_head;

        if (seconds >= m_length) {
            m_eof = true;
            return io_result_success;
        }

        double cur_pos_sec = (double)(t_int64)m_samplepos / (double)(t_int64)m_samplerate;

        if (m_reader->can_seek() && ((m_header_type == 1) || (seconds < cur_pos_sec)))
        {
            frames = (unsigned int)(seconds*((double)m_samplerate/(double)m_framesize));
            if (frames > 1) frames--;

            for (i = 0; i < frames; i++)
            {
                if (target->next)
                    target = target->next;
                else
                    return io_result_error_data;
            }
            if (target->offset == 0 && frames > 0)
                return io_result_error_data;
            m_file_offset = target->offset;
            m_reader->seek(m_file_offset,p_abort);

            io_status = m_reader->read(m_aac_buffer, 768*6,io_bytes_done,p_abort);
			if (io_result_failed(io_status)) return io_status;
            if (io_bytes_done != 768*6)
                m_at_eof = 1;
            else
                m_at_eof = 0;
            m_aac_bytes_into_buffer = io_bytes_done;
            m_aac_bytes_consumed = 0;
            m_file_offset += io_bytes_done;
            m_samplepos =(frames > 1) ? (t_uint64)(frames-1) * m_framesize : 0;
            m_seekskip = (int)((t_uint64)(seconds * m_samplerate + 0.5) - m_samplepos);// + m_framesize;
            if (m_seekskip < 0) return io_result_error_data; // should never happen
            faacDecPostSeekReset(hDecoder, -1);

            faacDecFrameInfo frameInfo;
            memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
            io_status = fill_buffer(p_abort);
			if (io_result_failed(io_status)) return io_status;
            faacDecDecode(hDecoder, &frameInfo, m_aac_buffer, m_aac_bytes_into_buffer);

            return io_result_success;
        } else {
            if (seconds > cur_pos_sec)
            {
                frames = (unsigned int)((seconds - cur_pos_sec)*((double)m_samplerate/(double)m_framesize));

                if (frames > 0)
                {
                    for (i = 0; i < frames; i++)
                    {
						io_status = run(NULL,p_abort);
						if (io_result_failed(io_status)) return io_result_error_data;
                    }
                }

                m_seekskip = (int)((t_uint64)(seconds * m_samplerate + 0.5) - m_samplepos);
                if (m_seekskip < 0) return io_result_error_data; // should never happen
                faacDecPostSeekReset(hDecoder, -1);
            }
            return io_result_success;
        }
    }

	virtual bool can_seek()
	{
		if (m_reader.is_valid()) return m_reader->can_seek();
		else return false;
	}

    static bool g_is_our_content_type(const char *url, const char *type)
    {
        return !stricmp_utf8(type, "audio/aac") || !stricmp_utf8(type, "audio/x-aac") || !stricmp_utf8(type, "audio/aacp");
    }

	virtual bool get_dynamic_info(file_info & out,double * timestamp_delta,bool * b_track_change)
	{
		bool ret = false;
		if (m_reader_v2.is_valid() && m_reader_v2->is_dynamic_info_enabled())
		{
			*timestamp_delta = 0;
			ret = m_reader_v2->get_dynamic_info(out, b_track_change);
		}
		return ret;
	}

	static GUID g_get_guid()
	{
		// {A2227771-E75C-4461-A415-750EBFEC514F}
		static const GUID guid = 
		{ 0xa2227771, 0xe75c, 0x4461, { 0xa4, 0x15, 0x75, 0xe, 0xbf, 0xec, 0x51, 0x4f } };
		return guid;
	}

	static const char * g_get_name() {return "Raw AAC parser";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

private:

    service_ptr_t<file> m_reader;
	service_ptr_t<file_dynamicinfo> m_reader_v2;

    faacDecHandle hDecoder;

    long m_aac_bytes_into_buffer;
    long m_aac_bytes_consumed;
    t_int64 m_file_offset;
    t_int64 m_last_offset;
    unsigned char *m_aac_buffer;
    int m_at_eof;

    unsigned long m_samplerate;
    int m_header_type;

    struct seek_list *m_head;
    struct seek_list *m_tail;

    t_uint64 m_samples;
    unsigned int m_framesize;
    t_uint64 m_samplepos;
    int m_seekskip;
    double m_length;
    bool m_eof;

    t_io_result fill_buffer(abort_callback & p_abort)
    {
        unsigned io_bytes_done;
		t_io_result io_status;

        if (m_aac_bytes_consumed > 0)
        {
            if (m_aac_bytes_into_buffer)
            {
                memmove((void*)m_aac_buffer, (void*)(m_aac_buffer + m_aac_bytes_consumed),
                    m_aac_bytes_into_buffer*sizeof(unsigned char));
            }

            if (!m_at_eof)
            {
                io_status = m_reader->read((void*)(m_aac_buffer + m_aac_bytes_into_buffer),
                    m_aac_bytes_consumed,io_bytes_done,p_abort);
				if (io_result_failed(io_status)) return io_status;

                if (io_bytes_done != (unsigned)m_aac_bytes_consumed)
                    m_at_eof = 1;

                m_aac_bytes_into_buffer += io_bytes_done;
            }

            m_aac_bytes_consumed = 0;

            if (m_aac_bytes_into_buffer > 3)
            {
                if (memcmp(m_aac_buffer, "TAG", 3) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
            if (m_aac_bytes_into_buffer > 11)
            {
                if (memcmp(m_aac_buffer, "LYRICSBEGIN", 11) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
            if (m_aac_bytes_into_buffer > 8)
            {
                if (memcmp(m_aac_buffer, "APETAGEX", 8) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
        }

        return io_result_success;
    }

    void advance_buffer(int bytes)
    {
        m_file_offset += bytes;
        m_aac_bytes_consumed = bytes;
        m_aac_bytes_into_buffer -= bytes;
    }

    t_io_result adts_parse(t_int64 *bitrate, double *length,abort_callback & p_abort)
    {
        static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
        int frames, frame_length;
        int t_framelength = 0;
        int samplerate;
        double frames_per_sec, bytes_per_frame;
		t_io_result io_status;

        /* Read all frames to ensure correct time and bitrate */
        for (frames = 0; /* */; frames++)
        {
            io_status = fill_buffer(p_abort);
			if (io_result_failed(io_status)) return io_status;

            if (m_aac_bytes_into_buffer > 7)
            {
                /* check syncword */
                if (!((m_aac_buffer[0] == 0xFF)&&((m_aac_buffer[1] & 0xF6) == 0xF0)))
                    break;

                m_tail->offset = m_file_offset;
                m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                m_tail = m_tail->next;
                m_tail->next = NULL;

                if (frames == 0)
                    samplerate = sample_rates[(m_aac_buffer[2]&0x3c)>>2];

                frame_length = ((((unsigned int)m_aac_buffer[3] & 0x3)) << 11)
                    | (((unsigned int)m_aac_buffer[4]) << 3) | (m_aac_buffer[5] >> 5);

                t_framelength += frame_length;

                if (frame_length > m_aac_bytes_into_buffer)
                    break;

                advance_buffer(frame_length);
            } else {
                break;
            }
        }

        frames_per_sec = (double)samplerate/1024.0;
        if (frames != 0)
            bytes_per_frame = (double)t_framelength/(double)(frames*1000);
        else
            bytes_per_frame = 0;
        *bitrate = (t_int64)(8. * bytes_per_frame * frames_per_sec + 0.5);
        if (frames_per_sec != 0)
            *length = (double)frames/frames_per_sec;
        else
            *length = 1;

        return io_result_success;
    }
	
};

static input_factory_t<input_aac> g_input_aac_factory;


DECLARE_COMPONENT_VERSION ("MPEG-4 AAC decoder",
                           "2.1",
                           "Based on FAAD2 v" FAAD2_VERSION "\nCopyright (C) 2002-2003 http://www.audiocoding.com" );


DECLARE_FILE_TYPE("AAC files","*.AAC");