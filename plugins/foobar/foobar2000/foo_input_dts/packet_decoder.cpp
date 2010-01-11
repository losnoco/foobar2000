#include "../SDK/foobar2000.h"

typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned char   uint8_t;
typedef signed char     int8_t;

extern "C" {
#include "dca.h"
}

#define BUFFER_SIZE 24576
#define HEADER_SIZE 14
#define FRAME_SAMPLES 256

class packet_decoder_dts : public packet_decoder {
    bool init()
    {
        cleanup();

        state = dca_init(0);
        if (!state) {
            console::error("Failed to initialize DTS decoder");
	        return false;
        }

        return true;
    }

    void cleanup()
    {
        if (state) {
            dca_free(state);
            state = NULL;
        }

        bufptr = buf;
        bufpos = buf + HEADER_SIZE;
        dts_flags = nch = srate = bitrate = 0;
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

    dca_state_t *state;

    pfc::array_t<audio_sample> output;
    uint8_t buf[BUFFER_SIZE];
    uint8_t *bufptr, *bufpos;

    int dts_flags, nch, srate, bitrate, frame_length;
    unsigned int channel_mask;

public:
    packet_decoder_dts()
    {
        state = 0;

        cleanup();
    }

    ~packet_decoder_dts()
    {
        cleanup();
    }

    virtual t_size set_stream_property ( const GUID &p_type, t_size p_param1, const void *p_param2, t_size p_param2size )
    {
        return 0;
    }

    virtual void get_info ( file_info &p_info )
    {
        p_info.info_set_int("samplerate", srate);
        p_info.info_set_int("channels", nch);
        p_info.info_set("codec", "DTS");
        p_info.info_set("encoding", "lossy");
    }

    void open(const GUID &p_owner, bool p_decode, t_size p_param1, const void *p_param2, t_size p_param2size, abort_callback &p_abort)
    {
        const matroska_setup *setup = (const matroska_setup *)p_param2;
        if (!init()) throw exception_io_data();
        nch = setup->channels;
        srate = setup->sample_rate;
    }

    static bool g_is_our_setup(const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size)
    {
        if (p_owner == owner_matroska) {
            if (p_param2size == sizeof(matroska_setup)) {
                const matroska_setup *setup = (const matroska_setup *)p_param2;
                if (!strncmp(setup->codec_id,"A_DTS", 5)) return true; else return false;
            } else return false;
        } else return false;
    }

    virtual unsigned get_max_frame_dependency() { return 0; }
    virtual double get_max_frame_dependency_time() { return 0; }

    virtual void reset_after_seek() {}

    virtual bool analyze_first_frame_supported() { return false; }

    virtual void analyze_first_frame(const void *p_buffer, t_size p_bytes, abort_callback &p_abort) { return; }

    virtual void decode ( const void *data, t_size bytes, audio_chunk &p_chunk, abort_callback &p_abort )
    {
        //if ( !state ) return false;

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
            int len = end - start;
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
                        for (bufptr = buf; bufptr < buf + HEADER_SIZE-1; bufptr++) bufptr[0] = bufptr[1];
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
                                        //out[i * nch + chan_map[tmp][j]] = src[j * nch + i];
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
                    console::warning("DTS: error");
                    bufptr = buf;
                    bufpos = buf + HEADER_SIZE;
                }
            }
        }

        if (samples > 0) {
            p_chunk.set_data(output.get_ptr(), samples, nch, srate, channel_mask);
        }
    }
};

static packet_decoder_factory_t<packet_decoder_dts> g_packet_decoder_dts_factory;
