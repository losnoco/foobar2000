/*
 * foo_input_tta.h
 *
 * Description: TTAv1 plug-in definitions and prototypes
 * Copyright (c) 2010 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

/*

	change log

2016-12-30 21:07 UTC - kode54
- Removed file size caching and checking from file info reader. It
  should be fast enough to simply re-read the tags any time the core
  wants them.
- Added component filename validation, because it's important for
  crash minidump loading.
- Use trailing tag position when calculating file bitrate, to subtract
  it from the reported average.
- Version is now 3.4

*/

#ifndef FOO_INPUT_TTA_H_
#define FOO_INPUT_TTA_H_

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include <mmreg.h>
#include "resource.h"
#include "libtta/libtta.h"

#define PCM_BUFFER_LENGTH 5120
#define PCM_BUFFER_SIZE	(PCM_BUFFER_LENGTH*MAX_DEPTH*MAX_NCH) + 4

using namespace tta;

#define  PLUGIN_VERSION	"3.5"
#define  TTA_FORMAT_VERSION "1"
#define  COPYRIGHT "Copyright (c) 2015 Aleksander Djuric. All rights reserved.\nCopyright (c) 2018 Christopher Snowhill. All rights reserved."
#define  PROJECT_URL "http://www.true-audio.com"

typedef struct {
	TTA_io_callback iocb;
	service_ptr_t<file> infile;
	abort_callback_dummy p_abort;
} TTA_io_callback_wrapper;

typedef struct {
	TTA_io_callback iocb;
	unsigned char *p_buffer;
	unsigned int size;
} TTA_mem_callback_wrapper;

class input_tta : public input_stubs {
public:
	input_tta() {
		TTA = NULL;
		seek_skip = 0;
	}
	~input_tta() {
		if (TTA) delete TTA;
	}

	void open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort);
	void get_info(file_info &p_info, abort_callback &p_abort);
	t_filestats get_file_stats(abort_callback &p_abort);
	void decode_initialize(unsigned p_flags, abort_callback &p_abort);
	bool decode_can_seek();
	void decode_seek(double p_seconds, abort_callback &p_abort);
	bool decode_run (audio_chunk &p_chunk, abort_callback &p_abort);
	bool decode_run_raw(audio_chunk &p_chunk, mem_block_container &p_raw, abort_callback &p_abort);
	bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta);
	bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta);
	void decode_on_idle(abort_callback &p_abort);
	void retag(const file_info &p_info, abort_callback &p_abort);
	void remove_tags(abort_callback &p_abort);
	static bool g_is_our_content_type(const char *p_content_type);
	static bool g_is_our_path(const char *p_path, const char *p_extension);
	void set_logger(event_logger::ptr) {}
	static void tta_error(TTA_CODEC_STATUS m_error);
	static GUID g_get_guid();
	static const char * g_get_name();
	static GUID g_get_preferences_guid();

private:
	tta_decoder *TTA;
	TTA_io_callback_wrapper io;
	service_ptr_t<file> infile;
	unsigned char pcm_buffer[PCM_BUFFER_SIZE];	// PCM buffer
	unsigned long seek_skip;	// number of samples to skip after seek
};

class packet_decoder_tta : public packet_decoder {
public:
    packet_decoder_tta() {
		TTA = NULL;
		pcm_buffer = NULL;
	}
    ~packet_decoder_tta() {
		cleanup();
	}

	static bool g_is_our_setup(const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size);
	void open(const GUID &p_owner, bool p_decode, t_size p_param1, const void *p_param2, t_size p_param2size, abort_callback &p_abort);
	virtual void get_info(file_info &p_info);
	virtual void decode(const void *p_buffer, t_size p_bytes, audio_chunk &p_chunk, abort_callback &p_abort);

	virtual bool analyze_first_frame_supported() { return true; }
	virtual void analyze_first_frame(const void *p_buffer, t_size p_bytes, abort_callback &p_abort);
	virtual t_size set_stream_property(const GUID &p_type,	t_size p_param1, const void *p_param2, t_size p_param2size);
	virtual unsigned get_max_frame_dependency() { return 0; }
	virtual double get_max_frame_dependency_time() { return 0; }
	virtual void reset_after_seek()	{}

private:
	tta_decoder *TTA;
	TTA_mem_callback_wrapper io;
	unsigned long pcm_buffer_len;
	unsigned char *pcm_buffer;
	unsigned long pcm_buffer_size;
	void cleanup() {
		if (TTA) delete TTA;
		if (pcm_buffer) {
			delete[] pcm_buffer;
			pcm_buffer = NULL;
		}
	}
};

#endif /* FOO_INPUT_TTA_H_ */
