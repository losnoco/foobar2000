// DTS Coherent Acoustics decoder for foobar2000
// Based on dtsdec, Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
//                            (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
//                            (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
// foobar component copyright (C) 2004-2006 Janne Hyv‰rinen
//
// Changes:
//  0.2.3  (2009-03-30): Fixed tag writing
//  0.2.2  (2008-10-26): Restricted silence generation to DTS WAV files
//  0.2.1  (2008-09-28): Seeking accuracy improvements, SDK's Decoder Validator was broken and didn't notice the bugs
//  0.2.0  (2008-09-13): Added preliminary DSP decoder for use with DTS audio CDs and WAVs without changing extension
//  0.1.9  (2008-09-02): Small change: output silence instead of nothing when decoding a block fails
//  0.1.8  (2008-09-02): Tagging configuration is back (in Advanced preferences), updated libdca to version 0.0.5, seeking accuracy improved
//  0.1.7  (2006-07-22): Added support for internal cuesheets
//  0.1.6  (2006-07-12): Playback from cuesheets always started from the beginning of the file, fixed
//  0.1.5  (2006-07-09): foobar2000 v0.9.x conversion
//  0.1.4  (2005-07-24): Accepts header with reversed word order
//  0.1.3  (2005-03-28): Smarter channel order fixing. Now uses 9 channel output only for files that output to center rear speaker.
//                       Added channel order hack also to packet decoder, added configuration
//  0.1.2  (2005-03-28): Fixed crashing with invalid files
//  0.1.1  (2005-03-26): Hack to fix output channel order for odd channel modes
//  0.1    (2005-03-20): Fixed incorrect handling for files that had no LFE channel, more accurate seeking
//  0.0.9  (2005-01-01): Fixed hanging at the end of DTS files
//  0.0.8  (2004-12-27): Improved DTS-in-wav detection
//  0.0.7  (2004-10-23): Added DTS header seeking to support more DTS-in-wav files
//  0.0.6  (2004-10-20): Removed most of the changes for 0.0.5 and replaced with hacks for now
//  0.0.5  (2004-10-17): Fixes to raw DTS handling
//  0.0.4  (2004-10-15): Simplified packet decoder, added codec reporting, fixed typo in version number
//  0.0.3  (2004-10-15): Added Matroska packet decoder support

#define FD_VERSION  "0.2.3"

//#define DTS_DEBUG // print status info to console

#include "../SDK/foobar2000.h"
using namespace pfc;
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

#define BUFFER_SIZE 24576
#define HEADER_SIZE 14
#define FRAME_SAMPLES 256

#define cfg_drc 0   // disable dynamic range compression
//cfg_int cfg_drc ( "drc", 0 );   // Dynamic range compression defaults to off
//cfg_int cfg_tag ( "tag", 1 );   // Write APEv2 tags by default

static const GUID guid_dts_branch = { 0x879389e3, 0x8c4, 0x4db0, { 0x8c, 0xb7, 0xec, 0x51, 0x3d, 0xb5, 0xd9, 0x14 } };
static const GUID guid_enable_tag = { 0x39e19ab2, 0xfb84, 0x4657, { 0xbf, 0x8c, 0x7a, 0x55, 0x7a, 0x35, 0xce, 0x72 } };
//static const GUID guid_enable_drc = { 0xe254b211, 0xc3f4, 0x40b6, { 0x91, 0xc, 0xa8, 0x3e, 0xe, 0x7f, 0x61, 0x2f } };
static advconfig_branch_factory dts_tagging_branch("DTS", guid_dts_branch, advconfig_branch::guid_branch_tagging, 0);
static advconfig_checkbox_factory g_cfg_tag("Enable APEv2 tag writing", guid_enable_tag, guid_dts_branch, 0, true);
//static advconfig_checkbox_factory g_cfg_drc("DTS - Use dynamic range compression", guid_enable_drc, advconfig_branch::guid_branch_decoding, 0, false);

// -------------------------------------

static void parse_tagtype_internal(const char *p_tagtype, bool &p_have_id3v1, bool &p_have_id3v2, bool &p_have_apev2)
{
    const char *tagtype = p_tagtype;
    while(*tagtype)
    {
        unsigned delta = 0;
        while(tagtype[delta] != 0 && tagtype[delta] != '|') delta++;
        if (delta > 0)
        {
            if (!stricmp_utf8_ex(tagtype, delta, "apev1", infinite) || !stricmp_utf8_ex(tagtype, delta, "apev2", infinite))
                p_have_apev2 = true;
            else if (!stricmp_utf8_ex(tagtype, delta, "id3v1", infinite))
                p_have_id3v1 = true;
            else if (!stricmp_utf8_ex(tagtype, delta, "id3v2", infinite))
                p_have_id3v2 = true;
        }
        tagtype += delta;
        while(*tagtype == '|') tagtype++;
    }
}

class input_dts {
private:
    service_ptr_t<file> r, r_;
    long frame_size;
    dca_state_t *state;
    t_filesize header_pos;
    bool eof, iswav;

    pfc::array_t<audio_sample> output;
    uint8_t buffer[BUFFER_SIZE];
    /*
    uint8_t buf[BUFFER_SIZE];
    uint8_t *bufptr, *bufpos;
    */

    t_filesize tag_offset;
    int dts_flags, nch, srate, bitrate, frame_length;
    double real_bitrate;
    unsigned int skip_samples;
    unsigned int channel_mask;
    unsigned int silence_bytes;
#ifdef DTS_DEBUG
    __int64 skipped_bytes;
    __int64 decoded_bytes;
#endif

    __int64 skip_wav_header(abort_callback &p_abort)
    {
        t_filesize pos = r->get_position(p_abort);
        t_filesize filesize = r->get_size(p_abort);

        for (;;) {
            char temp[4];
            DWORD tmp;
            r->read(temp, 4, p_abort);
            if (memcmp(temp, "RIFF", 4)) break;
            r->read(temp, 4, p_abort);
            r->read(temp, 4, p_abort);
            if (memcmp(temp, "WAVE", 4)) break;

            for (;;) {
                if (r->read(temp, 4, p_abort) != 4) break;
                if (!memcmp(temp, "fmt ", 4)) break; //success
                if (!memcmp(temp, "data", 4)) break; //already got data chunk but no fmt
                if (r->read(&tmp, 4, p_abort) != 4) break;
                if (tmp & 1) tmp++;
                if (tmp == 0 || r->get_position(p_abort) + tmp > filesize - 8) break;
                r->seek(tmp + r->get_position(p_abort), p_abort);
            }

            if (memcmp(temp, "fmt ", 4)) break;

            __int64 position = r->get_position(p_abort) - 4;

            if (r->read(&tmp, 4, p_abort) != 4) break;
            if (tmp < 14 || tmp > 64*1024) break;
            r->seek(tmp + r->get_position(p_abort), p_abort);

            char code[4];

            do {
                position += 8 + tmp + (tmp&1); //word-align all blocksizes
                r->seek(position, p_abort);
                if (r->read(code, 4, p_abort) != 4) break;
                if (r->read(temp, 4, p_abort) != 4) break;
            } while (memcmp(code, "data", 4));

            if (memcmp(code, "data", 4)) break;

            position += 8;
            r->seek(position, p_abort);
            return position - pos;
        }

        r->seek(pos, p_abort);
        return 0;
    }

    int find_dts_header(const unsigned char *buf, unsigned int size)
    {
        unsigned int size_to_search = size-15;
        if (size_to_search > size) {
            // not enough data for one header
            return -1;
        }

        for (unsigned int offset = 0; offset < size_to_search; offset++) {
            // sync words appear aligned in the bit stream
            if ((buf[offset + 0] == 0x7f) && // 16 bit SYNC
                (buf[offset + 1] == 0xfe) &&
                (buf[offset + 2] == 0x80) &&
                (buf[offset + 3] == 0x01)) return offset;

            if ((buf[offset + 0] == 0xfe) && // 16 bit SYNC, reversed word order
                (buf[offset + 1] == 0x7f) &&
                (buf[offset + 2] == 0x01) &&
                (buf[offset + 3] == 0x80)) return offset;

            if ((buf[offset + 0] == 0xff) && // 14 bit SYNC
                (buf[offset + 1] == 0x1f) &&
                (buf[offset + 2] == 0x00) &&
                (buf[offset + 3] == 0xe8) &&
                ((buf[offset + 4] & 0xf0) == 0xf0) &&
                (buf[offset + 5] == 0x07)) return offset;

            if ((buf[offset + 0] == 0x1f) && // 14 bit SYNC, reversed word order
                (buf[offset + 1] == 0xff) &&
                (buf[offset + 2] == 0xe8) &&
                (buf[offset + 3] == 0x00) &&
                (buf[offset + 4] == 0x07) &&
                ((buf[offset + 5] & 0xf0) == 0xf0)) return offset;
        }

        return -1;
    }

    int find_first_dts_header(unsigned int max_offset, abort_callback &p_abort)
    {
        int offset = 0;

        while (offset+sizeof(buffer) <= max_offset) {
            if (r->read(buffer, sizeof(buffer), p_abort) != sizeof(buffer)) return -1;
            int pos = find_dts_header(buffer, sizeof(buffer));
            if (pos >= 0) return offset + pos;
            offset += sizeof(buffer);
        }

        return -1;
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
        //else if (tmp == DCA_4F2R) channels = 6; // not supported (0.0.5)

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

public:
    input_dts()
    {
        state = 0;
    }

    ~input_dts()
    {
#ifdef DTS_DEBUG
        if (state) {
            if (skipped_bytes > 0) console::formatter() << "DTS: skipped " << skipped_bytes << " bytes";
            if (decoded_bytes > 0) console::formatter() << "DTS: decoded " << decoded_bytes << " bytes";
            if (skipped_bytes + decoded_bytes > 0) console::formatter() << "DTS: total " << skipped_bytes+decoded_bytes << " bytes";
        }
#endif
        if (state) dca_free(state);
    }

    bool decode_can_seek() { return r->can_seek(); }
    bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta) { return false; }
    bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta) { return false; }
    void decode_on_idle(abort_callback &p_abort) { r->on_idle(p_abort); }

    static bool g_is_our_content_type(const char *p_content_type)
    {
        return (!strcmp(p_content_type, "audio/x-dts")) || (!strcmp(p_content_type, "audio/dts"));
    }

    static bool g_is_our_path(const char *p_path, const char *p_extension)
    {
        return (stricmp_utf8(p_extension, "dts") == 0) || (stricmp_utf8(p_extension, "dtswav") == 0);
    }

    t_filestats get_file_stats(abort_callback &p_abort) { return r->get_stats(p_abort); }

    void open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort)
    {
        if (p_reason==input_open_info_write && g_cfg_tag.get_static_instance().get_state()==false) throw exception_io_unsupported_format();

        r_ = p_filehint;
        input_open_file_helper(r_, p_path, p_reason, p_abort);

        tag_offset = 0;

        try {
            file_info_impl t;
            tag_processor::read_trailing_ex(r_, t, tag_offset, p_abort);
        } catch(exception_io_data) { /*file had no trailing tags */ }

        if (tag_offset == 0) tag_offset = r_->get_size(p_abort);

        //r = reader_limited::g_create(r_, 0, tag_offset, p_abort);
		service_ptr_t<reader_limited> r = new service_impl_t<reader_limited>();
		r->init( r_, 0, tag_offset, p_abort );
		this->r = r;

        state = dca_init(0);
        if (!state) {
            throw exception_io_data("Failed to initialize DTS decoder");
        }

        header_pos = 0;
        tag_processor::skip_id3v2(r, header_pos, p_abort);
        __int64 t = skip_wav_header(p_abort);
        if (t > 0) iswav = true; else iswav = false;
        header_pos += t;

        nch = srate = dts_flags = frame_size = 0;
        skip_samples = 0;
        silence_bytes = 0;
#ifdef DTS_DEBUG
        skipped_bytes = 0;
        decoded_bytes = 0;
#endif
        eof = false;
        channel_mask = 0;

        t = find_first_dts_header(2*1024*1024, p_abort);
        if (t < 0) {
            dca_free(state);
            state = 0;
            throw exception_io_data();
        }

#ifdef DTS_DEBUG
        if (t > 0 && p_reason==input_open_decode) console::formatter() << "DTS: header found at offset " << t+header_pos;
#endif

        t = find_dts_header(buffer, sizeof(buffer));
        if (t < 0) {
            dca_free(state);
            state = 0;
            throw exception_io_data();
        }

#ifdef DTS_DEBUG
        if (t > 0 && p_reason==input_open_decode) console::formatter() << "DTS: find_dts_header returned " << t;
#endif

        frame_size = find_dts_header(buffer+t+1, (int)(sizeof(buffer)-t-1)) + 1;
        if (frame_size <= 0) {
            dca_free(state);
            state = 0;
            throw exception_io_data();
        }

#ifdef DTS_DEBUG
        if (p_reason==input_open_decode) console::formatter() << "DTS: frame_size = " << frame_size;
#endif

        t = dca_syncinfo(state, buffer+t, &dts_flags, &srate, &bitrate, &frame_length);
        if (t == 0) {
            dca_free(state);
            state = 0;
            throw exception_io_data();
        }

#ifdef DTS_DEBUG
        if (p_reason==input_open_decode) {
            console::formatter() << "DTS: frame_size = " << frame_size;
            console::formatter() << "DTS: bitrate = " << bitrate;
            console::formatter() << "DTS: frame_length = " << frame_length;
        }
#endif

        nch = get_channel_count(dts_flags);
        if (!srate || !bitrate || !frame_length || !nch) {
            dca_free(state);
            state = 0;
            throw exception_io_data();
        }

        channel_mask = get_channel_map_mask(dts_flags);
        real_bitrate = (double)(frame_size * 8) * ((double)srate / frame_length);
    }

    void get_info(file_info &p_info, abort_callback &p_abort)
    {
        static const char *dts_mode_list[10] = {
            { "Mono" },
            { "Dual Mono" },
            { "Stereo" },
            { "Stereo (SUMDIFF)" },
            { "Stereo (TOTAL)" },
            { "3 front channels" },
            { "2 front, 1 rear surround channel" },
            { "3 front, 1 rear surround channel" },
            { "2 front, 2 rear surround channels" },
            { "3 front, 2 rear surround channels" }
        };

        t_filesize current_pos = r->get_position(p_abort);
        //t_filesize tag_offset = r->get_size(p_abort);

        try {
            //tag_processor::read_id3v2(r, p_info, p_abort);
            tag_processor::read_id3v2_trailing(r_, p_info, p_abort);
        } catch(exception_io_data) { /*file had no tags */ }

        //try {
        //    tag_processor::read_trailing_ex(r, p_info, tag_offset, p_abort);
        //} catch(exception_io_data) { /*file had no trailing tags */ }
        
        r->seek(current_pos, p_abort);

        double length = 0.0;

        if (tag_offset != filesize_invalid) {
            length = (double)(tag_offset - header_pos) * 8.0 / real_bitrate;
        }

        p_info.set_length(length);
        p_info.info_set_int("samplerate", srate);
        p_info.info_set_int("channels", nch);
        p_info.info_set_int("bitrate", (int)((real_bitrate+500)/1000));
        p_info.info_set("codec", "DTS");
        p_info.info_set("encoding", "lossy");
        p_info.info_set("channel_mode", pfc::string_formatter() << dts_mode_list[dts_flags & DCA_CHANNEL_MASK] << (dts_flags & DCA_LFE ? " + LFE" : ""));
    }

    void decode_initialize(unsigned p_flags, abort_callback &p_abort)
    {
        r->reopen(p_abort); //equivalent to seek to zero, except it also works on nonseekable streams
        r->seek(header_pos, p_abort);
    }

    bool decode_run(audio_chunk &chunk, abort_callback &p_abort)
    {
        /*
        Win32:
        0. Front Left - FL
        1. Front Right - FR
        2. Front Center - FC
        3. Low Frequency - LF
        4. Back Left - BL
        5. Back Right - BR
        6. Front Left of Center - FLC
        7. Front Right of Center - FRC
        8. Back Center - BC
        
        DTS:
        0. center
        1. left
        2. right
        3. left surround
        4. right surround
        5. LFE
        */
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

        if (eof) return false;

        while (1) {
            uint8_t *buf = buffer;
            uint8_t *end = buffer;

            unsigned int samples = 0;
            int skipped = 0;
            t_filesize pos = r->get_position(p_abort);

            while (samples + (skipped/4) < 1024) {
                if (end-buf < HEADER_SIZE) {
                    //int len = r->read(end, sizeof(buffer)-(end-buf), p_abort);
                    int len = r->read(end, HEADER_SIZE-(end-buf), p_abort);
                    end += len;
                }

                if (end-buf < HEADER_SIZE) {
                    skipped += end-buf;
                    buf += end-buf;
                    break;
                }

                int length = dca_syncinfo(state, buf, &dts_flags, &srate, &bitrate, &frame_length);
                if (!length) {
                    buf++;
                    skipped++;
                    if (skipped == HEADER_SIZE) skipped+=13;
                    continue;
                }

#ifdef DTS_DEBUG
                decoded_bytes += length;
#endif

                if (end-buf < length) {
                    //int len = r->read(end, sizeof(buffer)-(end-buf), p_abort);
                    int len = r->read(end, length-(end-buf), p_abort);
#ifdef DTS_DEBUG
                    decoded_bytes -= length;
                    decoded_bytes += (end-buf) + len;
#endif
                    if (len < length-(end-buf)) {
                        skipped += len;
                        buf += len;
                        end += len;
                        break;
                    }
                    end += len;
                }

                nch = get_channel_count(dts_flags);
                channel_mask = get_channel_map_mask(dts_flags);

                level_t level = 1.0;
                sample_t bias = 0;
                dts_flags &= DCA_CHANNEL_MASK | DCA_LFE;

                if (dca_frame(state, buf, &dts_flags, &level, bias)) { // error
                    console::formatter() << "DTS: dca_frame returned error (file offset: " << pos << ")";
                    buf += length;
                    continue;
                }

                if (cfg_drc == 0) dca_dynrng(state, NULL, NULL);

                if (skipped > 0) {
					if (iswav) {
						int silence = (skipped+silence_bytes) / 4;
						silence_bytes = (skipped+silence_bytes) % 4;
						output.grow_size((samples + silence) * nch);
						audio_sample *out = (audio_sample *)output.get_ptr() + samples * nch;
						memset(out, 0, silence * nch * sizeof(audio_sample));
						samples += silence;
#ifdef DTS_DEBUG
						console::formatter() << "DTS: added " << silence << " samples of silence (silence bytes remaining: " << silence_bytes << ")";
						console::formatter() << "DTS: skipped " << skipped << " bytes (file offset: " << pos << ")";
						skipped_bytes += skipped;
#endif
					}
					skipped = 0;
                }

                for (int n = 0; n < dca_blocks_num(state); n++) {
                    output.grow_size((samples + FRAME_SAMPLES) * nch);
                    audio_sample *out = (audio_sample *)output.get_ptr() + samples * nch;

                    if (!dca_block(state)) {
                        float *src = dca_samples(state);
                        int tmp = dts_flags & (~DCA_LFE);

                        if (dts_flags & DCA_LFE) {
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
                    } else {
                        memset(out, 0, FRAME_SAMPLES * nch * sizeof(audio_sample));
                        console::formatter() << "DTS: decode error in dca_block " << n << " (file offset: " << pos << ")";
                    }

                    samples += FRAME_SAMPLES;
                }

                buf += length;
            }

#ifdef DTS_DEBUG
            skipped_bytes += skipped;
#endif

            if (skipped > 0 || (samples == 0 && silence_bytes > 0)) {
                int silence = (skipped+silence_bytes) / 4;
                silence_bytes = (skipped+silence_bytes) % 4;
                if (samples == 0 && silence == 0) { // if silence_bytes left at the end, insert them as one silent sample
                    silence++;
                    silence_bytes = 0;
                }
                output.grow_size((samples + silence) * nch);
                audio_sample *out = (audio_sample *)output.get_ptr() + samples * nch;
                memset(out, 0, silence * nch * sizeof(audio_sample));
                samples += silence;
#ifdef DTS_DEBUG
                console::formatter() << "DTS: added " << silence << " samples of silence (silence bytes remaining: " << silence_bytes << ")";
                console::formatter() << "DTS: skipped " << skipped << " bytes (file offset: " << pos << ")";
                skipped_bytes += skipped;
#endif
            }

            if (samples > 0) {
                if (samples > skip_samples) {
                    chunk.set_data((audio_sample *)(output.get_ptr() + skip_samples * nch), samples - skip_samples, nch, srate, channel_mask);
                    skip_samples = 0;
                    return true;
                } else {
                    skip_samples -= samples;
                }
            } /*else if (iswav) { // hack to output silence with DTS in WAV (as CD players would do)
                samples = sizeof(buffer) / (2*sizeof(short));
                if (samples > skip_samples) {
                    output.grow_size((samples + skip_samples) * nch);
                    memset((audio_sample *)(output.get_ptr() + skip_samples * nch), 0, (samples-skip_samples) * nch * sizeof(audio_sample));
                    chunk.set_data((audio_sample *)(output.get_ptr() + skip_samples * nch), samples - skip_samples, nch, srate, channel_mask);
                    skip_samples = 0;
                    return true;
                } else {
                    skip_samples -= samples;
                }
            } */ else {
                return false;
            }
        }
    }

    void decode_seek(double newpos, abort_callback &p_abort)
    {
        r->ensure_seekable(); //throw exceptions if someone called decode_seek() despite of our input having reported itself as nonseekable.

        __int64 sample_pos = (__int64)(newpos*srate+0.5);
        t_filesize filesize = r->get_size(p_abort);

#ifdef DTS_DEBUG
        console::formatter() << "DTS: seek to sample " << sample_pos;
#endif

        if (sample_pos >= 0 && frame_size > 0) {
            eof = false;
            skip_samples = 0;
            silence_bytes = 0;
#ifdef DTS_DEBUG
            decoded_bytes = 0;
            skipped_bytes = 0;
#endif

            __int64 offset = (__int64)((newpos * real_bitrate/8.0) + 0.5);
            if (offset+header_pos >= (__int64)(filesize)) {
#ifdef DTS_DEBUG
                console::formatter() << "DTS: seek past eof";
#endif
                r->seek(filesize, p_abort);
                eof = true;
                return;
            }

            dca_free(state);
            state = dca_init(0);
            if (!state) {
                throw exception_io_data("Failed to initialize DTS decoder");
            }

            offset -= offset % frame_size;
            //if (offset >= frame_size) offset-=frame_size;
            if (offset >= frame_size*20) offset-=frame_size*20; else offset=0;

            r->seek(offset+header_pos, p_abort);

            skip_samples = (unsigned int)(sample_pos - (__int64)((offset / (real_bitrate/8.0) * srate)+0.5));
#ifdef DTS_DEBUG
            console::formatter() << "DTS: skip_samples: " << skip_samples;
#endif
        }
    }

    void retag(const file_info &p_info, abort_callback &p_abort)
    {
        if (g_cfg_tag.get_static_instance().get_state() == false) throw exception_io_unsupported_format(); // exit out when tags are not wanted
        r->ensure_seekable();

        const char *tagtype = p_info.info_get("tagtype");
        if (tagtype == 0) tagtype = "";

        bool id3v1=false, id3v2=false, apev2=false;
        parse_tagtype_internal(tagtype, id3v1, id3v2, apev2);

        if (!id3v2 && !apev2) apev2 = true;

        tag_processor::write_multi(r_, p_info, p_abort, id3v1, id3v2, apev2);
    }

    bool decode_run_raw(audio_chunk &p_chunk, mem_block_container &p_raw, abort_callback &p_abort) { throw pfc::exception_not_implemented(); }
    //void set_logger(event_logger::ptr ptr) {}
};

static input_cuesheet_factory_t<input_dts> g_input_dts_factory;

DECLARE_COMPONENT_VERSION("DTS decoder", FD_VERSION,
  "DTS decoding powered by libdca v0.0.5 by Gildas Bazin.\n"
  "Homepage for libdca: http://developers.videolan.org/libdca.html\n"
  "\n"
  "foobar component by Janne Hyv√§rinen.\n"
  "Licensed under GNU GPL.\n");

DECLARE_FILE_TYPE("DTS files", "*.DTS;*.DTSWAV");
