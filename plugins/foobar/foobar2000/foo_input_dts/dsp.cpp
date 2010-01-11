#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned char   uint8_t;
typedef signed char     int8_t;

extern "C" {
#include "dca.h"
}

enum {
	BUFFER_SIZE = 24576,
	HEADER_SIZE = 14,
	FRAME_SAMPLES = 256
};

class dts_dsp : public dsp_impl_base {
    bool init()
    {
        cleanup();

        state = dca_init(0);
        if (!state) {
            console::error("Failed to initialize DTS decoder");
	        return false;
        }

        srate = 44100;

        return true;
    }

    void cleanup()
    {
        if (state) {
            dca_free(state);
            state = 0;
        }

        bufptr = buf;
        bufpos = buf + HEADER_SIZE;
        chunk_count = dts_flags = nch = srate = bitrate = 0;
        valid_scale_found = false;
    }

    int get_channel_count(int flags)
    {
        int channels = 0, tmp = flags & DCA_CHANNEL_MASK;

        if (tmp > DCA_CHANNEL_MAX) return 0;

        if (tmp == DCA_MONO) channels = 1;
        else if (tmp < DCA_3F) channels = 2;
        else if (tmp < DCA_3F1R) channels = 3;
        else if (tmp < DCA_3F2R) channels = 4;
        else if (tmp < DCA_4F2R) channels = 5;
        //else if (tmp == DCA_4F2R) channels = 6; // not supported (version 0.0.5)

        if (channels && (flags & DCA_LFE)) channels++;

        return channels;
    }

    unsigned int get_channel_map_mask(int flags)
    {
        static const unsigned int output_nch[10] = {
            audio_chunk::channel_front_center,                                  // DCA_MONO
            audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_CHANNEL
            audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO
            audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO_SUMDIFF
            audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO_TOTAL
            audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center, // DCA_3F
            audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_center,  // DCA_2F1R
            audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_center, // DCA_3F1R
            audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left    | audio_chunk::channel_back_right,  // DCA_2F2R
            audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right, // DCA_3F2R
        };

        unsigned int map = output_nch[flags & DCA_CHANNEL_MASK];
        if (flags & DCA_LFE) map |= audio_chunk::channel_lfe;
        return map;
    }

    bool decode(const void *data, t_size bytes, audio_chunk &p_chunk, abort_callback &p_abort)
    {
        static const int chan_map[10][6] = {
            { 0, 0, 0, 0, 0, 0 },       // DCA_MONO
            { 0, 1, 0, 0, 0, 0 },       // DCA_CHANNEL
            { 0, 1, 0, 0, 0, 0 },       // DCA_STEREO
            { 0, 1, 0, 0, 0, 0 },       // DCA_STEREO_SUMDIFF
            { 0, 1, 0, 0, 0, 0 },       // DCA_STEREO_TOTAL
            { 2, 0, 1, 0, 0, 0 },       // DCA_3F
            { 0, 1, 2, 0, 0, 0 },       // DCA_2F1R
            { 2, 0, 1, 3, 0, 0 },       // DCA_3F1R
            { 0, 1, 2, 3, 0, 0 },       // DCA_2F2R
            { 2, 0, 1, 3, 4, 0 },       // DCA_3F2R
        };
        static const int chan_map_lfe[10][6] = {
            { 0, 1, 0, 0, 0, 0 },       // DCA_MONO
            { 0, 1, 2, 0, 0, 0 },       // DCA_CHANNEL
            { 0, 1, 2, 0, 0, 0 },       // DCA_STEREO
            { 0, 1, 2, 0, 0, 0 },       // DCA_STEREO_SUMDIFF
            { 0, 1, 2, 0, 0, 0 },       // DCA_STEREO_TOTAL
            { 2, 0, 1, 3, 0, 0 },       // DCA_3F
            { 0, 1, 3, 2, 0, 0 },       // DCA_2F1R
            { 2, 0, 1, 4, 3, 0 },       // DCA_3F1R
            { 0, 1, 3, 4, 2, 0 },       // DCA_2F2R
            { 2, 0, 1, 4, 5, 3 },       // DCA_3F2R
        };

        uint8_t *start = (uint8_t *)data;
        uint8_t *end = (uint8_t *)data + bytes;
        unsigned int samples = 0;

        while (1) {
            unsigned len = end - start;
            if (!len) break;
            if (len > bufpos - bufptr) len = bufpos - bufptr;

            memcpy(bufptr, start, len);
            bufptr += len;
            start += len;

            if (bufptr == bufpos) {
                if (bufpos == buf + HEADER_SIZE) {
                    int length = dca_syncinfo(state, buf, &dts_flags, &srate, &bitrate, &frame_length);
                    if (!length) {
                        //console::warning("DTS: skip");
                        for (bufptr = buf; bufptr < buf + HEADER_SIZE - 1; bufptr++) bufptr[0] = bufptr[1];
                        continue;
                    }

                    nch = get_channel_count(dts_flags);
                    channel_mask = get_channel_map_mask(dts_flags);

                    bufpos = buf + length;
                } else {
                    int n;
                    audio_sample *out = 0;
                    level_t level = 1.0;
                    sample_t bias = 0;

                    dts_flags &= DCA_CHANNEL_MASK | DCA_LFE;
                    if (dca_frame(state, buf, &dts_flags, &level, bias)) goto error;
                    //if (use_drc==0)
                    dca_dynrng(state, NULL, NULL);

                    for (n = 0; n < dca_blocks_num(state); n++) {
                        if (dca_block(state)) goto error;
                        output.grow_size((samples + FRAME_SAMPLES) * nch);
                        out = (audio_sample *)output.get_ptr() + samples * nch;
                        float *src = dca_samples(state);
                        if (src) {
                            int tmp = dts_flags & (~DCA_LFE);

                            if ( dts_flags & DCA_LFE ) {
                                for (int j = 0; j < nch; j++) {
                                    for (int i = 0; i < FRAME_SAMPLES; i++) {
                                        out[i * nch + chan_map_lfe[tmp][j]] = src[j * FRAME_SAMPLES + i];
                                    }
                                }
                            } else {
                                for (int j = 0; j < nch; j++) {
                                    for (int i = 0; i < FRAME_SAMPLES; i++) {
                                        out[i * nch + chan_map[tmp][j]] = src[j * FRAME_SAMPLES + i];
                                    }
                                }
                            }

                            samples += FRAME_SAMPLES;
                        }
                    }

                    bufptr = buf;
                    bufpos = buf + HEADER_SIZE;
					continue;

                    error:
                    return false;
                    //console::warning("DTS: error");
                    bufptr = buf;
                    bufpos = buf + HEADER_SIZE;
                }
            }
        }

		if (samples >= FRAME_SAMPLES) {
            p_chunk.set_data(output.get_ptr(), samples, nch, srate, channel_mask);
            return true;
        }
        return false;
    }

    bool find_dts_header(const audio_sample *buf, unsigned int samples, audio_sample &scale)
    {
        if (samples < 4) return false;

		audio_sample min_scale = 32768., min_scale_adjusted = 32767.;
		audio_sample work_scale;

        for (unsigned int i = 0; i < samples-4; i++) {
            if (buf[i+0] == 0.0) continue;

            if (fabs((double)buf[i+1] - (double)buf[i+0] * (double)384/(double)(-385)) < 0.00001) {
                work_scale = (double)(384/32768.0) / (double)buf[i+0];
                //console::formatter() << "1. scale = " << scale;
                //return true;
				if ( fabs( work_scale - 1.0f ) < min_scale_adjusted ) {
					min_scale = work_scale;
					min_scale_adjusted = fabs( work_scale - 1.0f );
				}
            }

            if (fabs((double)buf[i+1] - (double)buf[i+0] * (double)(-32767)/(double)32766) < 0.00001) {
                work_scale = (double)(32766/32768.0) / (double)buf[i+0];
                //console::formatter() << "2. scale = " << scale;
                //return true;
				if ( fabs( work_scale - 1.0f ) < min_scale_adjusted ) {
					min_scale = work_scale;
					min_scale_adjusted = fabs( work_scale - 1.0f );
				}
            }

            if (fabs((double)buf[i+1] - (double)buf[i+0] * (double)(-6144)/(double)8191) < 0.00001) {
                work_scale = (double)(8191/32768.0) / (double)buf[i+0];
                t_int16 tmp[1];
                audio_math::convert_to_int16(buf+i+2, 1, tmp, work_scale);
                const unsigned char *b = (const unsigned char *)tmp;

                if (((b[0] & 0xf0) == 0xf0) && (b[1] == 0x07)) {
                    //console::formatter() << "3. scale = " << scale;
                    //return true;
					if ( fabs( work_scale - 1.0f ) < min_scale_adjusted ) {
						min_scale = work_scale;
						min_scale_adjusted = fabs( work_scale - 1.0f );
					}
				}
            }

            if ((fabs((double)buf[i+1] - (double)buf[i+0] * (double)232/(double)(-225)) < 0.00001) &&
                (fabs((double)buf[i+2] - (double)buf[i+0] * (double)1792/(double)(-225)) < 0.00001)) {
                work_scale = (double)(232/32768.0) / (double)buf[i+0];
                t_int16 tmp[1];
                audio_math::convert_to_int16(buf+i+2, 1, tmp, work_scale);
                const unsigned char *b = (const unsigned char *)tmp;

                if ((b[0] == 0x07) && ((b[1] & 0xf0) == 0xf0)) {
                    //console::formatter() << "4. scale = " << scale;
                    //return true;
					if ( fabs( work_scale - 1.0f ) < min_scale_adjusted ) {
						min_scale = work_scale;
						min_scale_adjusted = fabs( work_scale - 1.0f );
					}
                }
            }
        }

		if ( min_scale_adjusted < 32767. ) {
			scale = min_scale;
			return true;
		}

        return false;
    }

    audio_chunk_impl m_chunk[3];

    dca_state_t *state;

    pfc::array_t<audio_sample> output;
    pfc::array_t<uint8_t> buffer;
    uint8_t buf[BUFFER_SIZE];
    uint8_t *bufptr, *bufpos;
    audio_sample scale;
    bool valid_scale_found;

    int dts_flags, nch, srate, bitrate, frame_length;
    unsigned int channel_mask, chunk_count;

public:
    dts_dsp()
    {
        state = 0;
        cleanup();
    }

    ~dts_dsp()
    {
        cleanup();
    }

    static GUID g_get_guid()
    {
        static const GUID guid = { 0xf93b1dbe, 0xc616, 0x430c, { 0xab, 0x46, 0x5, 0x2c, 0x6e, 0x9c, 0x83, 0xcf } };
        return guid;
    }

    static void g_get_name(pfc::string_base &p_out)
    {
        p_out = "DTS decoder";
    }

    virtual void on_endoftrack(abort_callback &p_abort)
    {
        flush();
    }

    virtual void on_endofplayback(abort_callback &p_abort)
    {
        flush();
    }

    virtual bool on_chunk(audio_chunk *chunk, abort_callback &p_abort)
    {
        if (chunk->get_channels() != 2 || chunk->get_srate() != 44100) {
			flush_chunk();
            flush();
            return true;
        }

        metadb_handle_ptr fh;
        if (!get_cur_file(fh)) {
			flush_chunk();
            flush();
            return true;
        }

        file_info_impl i;
        if (!fh->get_info_async(i)) {
			flush_chunk();
            flush();
            return true;
        }

        if (i.info_get_decoded_bps() != 16) {
			flush_chunk();
            flush();
            return true;
        }

        if (!state) {
            if (!find_dts_header(chunk->get_data(), chunk->get_sample_count()*2, scale)) {
				flush_chunk();
                flush();
                return true;
            }
            if (!init()) return true;
        }

        if (!valid_scale_found) find_dts_header(chunk->get_data(), chunk->get_sample_count()*2, scale);

        int data = chunk->get_sample_count() * 4;
        buffer.grow_size(data);
        audio_math::convert_to_int16(chunk->get_data(), chunk->get_sample_count()*2, (t_int16 *)buffer.get_ptr(), scale);

		if (!valid_scale_found)
		{
			if ( !chunk_count ) m_chunk[ 2 ].copy( *chunk );
			if ( decode( buffer.get_ptr(), data, m_chunk[ chunk_count ], p_abort ) ) {
				if ( chunk_count ) {
					chunk_count = 0;
					valid_scale_found = true;
					audio_chunk * out = insert_chunk( m_chunk[ 0 ].get_sample_count() );
					out->copy( m_chunk[ 0 ] );
					out = insert_chunk( m_chunk[ 1 ].get_sample_count() );
					out->copy( m_chunk[ 1 ] );
				} else {
					chunk_count++;
				}
				return false;
			} else {
				/*if ( bufptr - buf >= chunk->get_sample_count() * 2 ) {
					return false;
				} else*/ {
					flush_chunk();
					flush();
					return true;
				}
			}
		} else {
			if (decode(buffer.get_ptr(), data, m_chunk[ 0 ], p_abort)) {
				audio_chunk *out = insert_chunk(m_chunk[ 0 ].get_sample_count());
				out->copy(m_chunk[ 0 ]);
				return false;
			} else {
				if ( bufptr - buf >= chunk->get_sample_count() * 2 ) {
					return false;
				} else {
					flush_chunk();
					flush();
					return true;
				}
			}
		}
    }

    virtual void flush()
    {
		valid_scale_found = false;
		chunk_count = 0;
    }

	inline void flush_chunk()
	{
		if ( chunk_count ) {
			audio_chunk * out = insert_chunk( m_chunk[ 2 ].get_sample_count() );
			out->copy( m_chunk[ 2 ] );
		}
	}

    virtual double get_latency()
    {
		double latency = 0.;
		if ( bufptr > buf && srate ) {
			latency += (double)((bufptr - buf + 3) / 4) / (double)srate;
		}
		if ( chunk_count ) {
			latency += m_chunk[ 0 ].get_duration();
		}
        return latency; // return (double)(buffer_data / 4) / 44100.0;
    }

    virtual bool need_track_change_mark()
    {
        return false;
    }
};

static dsp_factory_nopreset_t<dts_dsp> foo_dts_dsp;
