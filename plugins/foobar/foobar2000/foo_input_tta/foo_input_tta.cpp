/*
 * foo_input_tta.cpp
 *
 * Description: TTAv1 input plug-in for Foobar2000
 * Copyright (c) 2010 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#include "foo_input_tta.h"

#define TAGTYPES_TOTAL 5
#define DEFAULT_TAG	0

// {8CDBB31F-4B6C-4147-B452-AFE8AA349F6B}
static const GUID guid_cfg_tag_type = 
{ 0x8cdbb31f, 0x4b6c, 0x4147, { 0xb4, 0x52, 0xaf, 0xe8, 0xaa, 0x34, 0x9f, 0x6b } };
static cfg_int cfg_tagging(guid_cfg_tag_type, DEFAULT_TAG);

// {88743B34-31E6-4775-AA46-D98CAA79FB9C}
static const GUID guid_cfg_bitrate = 
{ 0x88743b34, 0x31e6, 0x4775, { 0xaa, 0x46, 0xd9, 0x8c, 0xaa, 0x79, 0xfb, 0x9c } };
static cfg_int cfg_bitrate(guid_cfg_bitrate, 1);

static const char * tag_name[TAGTYPES_TOTAL] = {
	"APEv2 tags only",
	"APEv2 and ID3v1",
	"ID3v1 tags only",
	"ID3v2 tags only",
	"ID3v2 and ID3v1"
};

// {9861B4FD-666D-4773-90FE-0C98A2CDC469}
static const GUID input_tta_preferences_guid = 
{ 0x9861b4fd, 0x666d, 0x4773, { 0x90, 0xfe, 0xc, 0x98, 0xa2, 0xcd, 0xc4, 0x69 } };

class input_tta_config : public preferences_page {
	static BOOL CALLBACK ConfigProc(HWND dialog,UINT msg,WPARAM wp,LPARAM lp) {
		HWND dlg_tag = uGetDlgItem(dialog, IDC_TAG);
		int indx;

		switch (msg) {
		case WM_INITDIALOG:
			for (indx = 0; indx < TAGTYPES_TOTAL; indx++)
				uSendMessageText(dlg_tag, CB_ADDSTRING, 0, tag_name[indx]);

			if (cfg_tagging >= 0 && cfg_tagging < TAGTYPES_TOTAL)
				uSendMessage(dlg_tag, CB_SETCURSEL, cfg_tagging, 0);
			else uSendMessage(dlg_tag, CB_SETCURSEL, 3, 0);

			CheckDlgButton(dialog, IDC_RT_BITRATE, cfg_bitrate);
			uSetDlgItemText(dialog, IDC_VERSION,
				"True Audio format version " TTA_FORMAT_VERSION "\n"
				"Foobar plug-in version " PLUGIN_VERSION);

		return TRUE;

		case WM_COMMAND:
			switch (wp) {
			case IDC_TAG | (CBN_SELCHANGE<<16):
				indx = uSendMessage(dlg_tag, CB_GETCURSEL, 0, 0);
				if (indx >= 0 && indx < TAGTYPES_TOTAL)
					cfg_tagging = indx;
				break;

			case IDC_RT_BITRATE:
				cfg_bitrate = !cfg_bitrate;
				break;
			}
			break;
        	}

		return FALSE;
	}

	virtual HWND create(HWND parent) {
		return CreateDialogParam(core_api::get_my_instance(),
			MAKEINTRESOURCE(IDD_CONFIG), parent, (DLGPROC) ConfigProc, (LPARAM) this);
	}
	virtual const char * get_name() { return "TTA input"; }
	virtual GUID get_guid() { return input_tta_preferences_guid; }
	virtual GUID get_parent_guid() {return guid_input;}
	virtual bool reset_query() {return false;}
	virtual void reset() {}
	virtual bool get_help_url(pfc::string_base & p_out) {
		return false;
	}
};

static preferences_page_factory_t<input_tta_config> g_input_tta_config_factory;

///////////////////////////////// callbacks /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int CALLBACK read_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io;
	return iocb->infile->read(buffer, size, iocb->p_abort);
} // read_callback

int CALLBACK write_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io; 
	iocb->infile->write(buffer, size, iocb->p_abort);
	return size;
} // write_callback

__int64 CALLBACK seek_callback(TTA_io_callback *io, TTAint64 offset) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io; 
	iocb->infile->seek(offset, iocb->p_abort);
	return offset;
} // seek_in_callback

int CALLBACK read_mem_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size) {
	TTA_mem_callback_wrapper *iocb = (TTA_mem_callback_wrapper *)io;
	int read_bytes = (size < iocb->size)? size : iocb->size;
	if (read_bytes && iocb->p_buffer) {
		CopyMemory(buffer, iocb->p_buffer, read_bytes);
		iocb->p_buffer += read_bytes;
		iocb->size -= read_bytes;
	}
	return read_bytes;
} // read_callback

/////////////////////////// input_tta functions /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void input_tta::tta_error(TTA_CODEC_STATUS m_error) {
	switch (m_error) {
		case TTA_OPEN_ERROR:	throw pfc::exception("Can't open file");
		case TTA_FORMAT_ERROR: 	throw pfc::exception("Unsupported file format");
		case TTA_FILE_ERROR:	throw pfc::exception("File is corrupted");
		case TTA_READ_ERROR:	throw pfc::exception("Can't read from file");
		case TTA_WRITE_ERROR:	throw pfc::exception("Can't write to file");
		case TTA_SEEK_ERROR:	throw pfc::exception("File seek error");
		case TTA_MEMORY_ERROR:	throw pfc::exception("Insufficient memory available");
		default:				throw pfc::exception("Unknown error");
	}
}

void input_tta::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort) {	
	infile = p_filehint;
	input_open_file_helper(infile, p_path, p_reason, p_abort);
}

void input_tta::get_info(file_info & p_info, abort_callback & p_abort) {
	t_filesize size = infile->get_size_ex(p_abort);
	t_filesize current_pos = infile->get_position(p_abort);
	t_filesize tag_size, tag_offset;
	t_uint32 tta_id;

	// Also subtract trailing tag size for bitrate calculation
	{
		file_info_impl temp;
		try
		{
			tag_processor::read_trailing_ex(infile, temp, tag_offset, p_abort);
			size = tag_offset;
		}
		catch (exception_tag_not_found e) {}
	}

	// skip id3v2 header
	infile->reopen(p_abort);
	tag_processor::skip_id3v2(infile, tag_size, p_abort);

	// read tta header
	infile->read_lendian_t(tta_id, p_abort);

	if (tta_id == 0x31415454) {
		t_uint16 format;
		t_uint16 nch;
		t_uint16 bps;
		t_uint32 sps;
		t_uint32 samples;
		double duration;

		infile->read_lendian_t(format, p_abort);
		infile->read_lendian_t(nch, p_abort);
		infile->read_lendian_t(bps, p_abort);
		infile->read_lendian_t(sps, p_abort);
		infile->read_lendian_t(samples, p_abort);
		duration = (double) samples / sps;

		p_info.set_length(duration);
		p_info.info_set_int("samplerate", sps);
		p_info.info_set_int("channels", nch);
		p_info.info_set_int("bitspersample", bps);
		p_info.info_set("codec", "True Audio");
		p_info.info_set("encoding", "lossless");
		p_info.info_set("homepage", PROJECT_URL);
		p_info.info_calculate_bitrate(size - tag_size, duration);
	}

	// read headers if present
	try {
		tag_processor::read_id3v2_trailing(infile, p_info, p_abort);
	} catch (exception_tag_not_found e) {}

	// return to saved position
	if (infile->can_seek())
		infile->seek(current_pos, p_abort);
}

t_filestats input_tta::get_file_stats(abort_callback & p_abort) {
	return infile->get_stats(p_abort);
}

void input_tta::decode_initialize(unsigned p_flags, abort_callback & p_abort) {
	infile->reopen(p_abort);

	io.iocb.read = &read_callback;
	io.iocb.seek = &seek_callback;
	io.iocb.write = NULL;
	io.infile = infile.get_ptr();
	io.p_abort = abort_callback_dummy();

	try {
		TTA = new tta_decoder((TTA_io_callback *) &io);
		TTA->decoder_init();
	}  catch (tta_exception ex) {
		tta_error(ex.code());
	}
}

bool input_tta::decode_can_seek() {
	return infile->can_seek();
}

void input_tta::decode_seek(double p_seconds, abort_callback & p_abort) {
	infile->ensure_seekable();
	double frame_start = TTA->set_position(p_seconds);
	seek_skip = (unsigned long)((p_seconds - frame_start) * TTA->info.sps + 0.5);
	if (seek_skip > TTA->info.flen) seek_skip = TTA->info.flen;
}

bool input_tta::decode_run(audio_chunk & p_chunk, abort_callback & p_abort) {
	unsigned long len = 0;

	try {
		while (seek_skip >= PCM_BUFFER_LENGTH) {
			if (!(len = TTA->decode_stream(pcm_buffer, PCM_BUFFER_LENGTH))) {
				return false;
			}
			seek_skip -= len;
		}
		if (!(len = TTA->decode_stream(pcm_buffer, PCM_BUFFER_LENGTH))) {
			return false;
		}
	} catch (tta_exception ex) {
		tta_error(ex.code());
	}

	if (len <= seek_skip) {
		return false;
	}

	unsigned long smp_size = TTA->info.nch * TTA->info.depth;
	unsigned char *data = pcm_buffer + seek_skip * smp_size;

	len -= seek_skip;
	seek_skip = 0;

	p_chunk.set_data_fixedpoint(data, len * smp_size,
		TTA->info.sps, TTA->info.nch, TTA->info.bps, audio_chunk::g_guess_channel_config(TTA->info.nch));

	return true;
}

bool input_tta::decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
	throw pfc::exception_not_implemented();
}

bool input_tta::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
	if (!cfg_bitrate) return false;
	if (TTA->info.rate == p_out.info_get_bitrate_vbr()) return false;
	p_out.info_set_bitrate_vbr(TTA->info.rate);
	return true;
}

bool input_tta::decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
	return decode_get_dynamic_info(p_out, p_timestamp_delta);
}

void input_tta::decode_on_idle(abort_callback & p_abort) {
	infile->on_idle(p_abort);
}

void input_tta::retag(const file_info & p_info, abort_callback & p_abort) {
	t_uint64 len = infile->get_size(p_abort);
	if (len <= 0) throw exception_io_unsupported_format();
	
	switch(cfg_tagging) {
		case 0: tag_processor::write_apev2(infile, p_info, p_abort); break;
		case 1: tag_processor::write_apev2_id3v1(infile, p_info, p_abort); break;
		case 2: tag_processor::write_id3v1(infile, p_info, p_abort); break;
		case 3: tag_processor::write_id3v2(infile, p_info, p_abort); break;
		case 4: tag_processor::write_id3v2_id3v1(infile, p_info, p_abort); break;
	}
}

void input_tta::remove_tags(abort_callback & p_abort) {
	tag_processor::remove_id3v2_trailing(infile, p_abort);
}

bool input_tta::g_is_our_content_type (const char * p_content_type) {
		return !_stricmp(p_content_type, "audio/x-tta")
			|| !_stricmp(p_content_type, "audio/tta");
}

bool input_tta::g_is_our_path(const char * p_path, const char * p_extension) {
	return (stricmp_utf8(p_extension, "TTA") == 0);
}

GUID input_tta::g_get_guid() {
	return { 0x1f4cd0f7, 0x473b, 0x4d69,{ 0x9e, 0xcf, 0xa1, 0x4d, 0xd8, 0xcd, 0xa3, 0xc6 } };
}

const char * input_tta::g_get_name() {
	return "TTA input";
}

GUID input_tta::g_get_preferences_guid() {
	return input_tta_preferences_guid;
}

static input_cuesheet_factory_t<input_tta> g_input_tta_factory;
static service_factory_single_t<album_art_extractor_impl_stdtags> g_album_art_extractor_impl_stdtags_factory("TTA");

/////////////////////// packet_decoder_tta functions ////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool packet_decoder_tta::g_is_our_setup(const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size) {
	if (p_owner == owner_matroska) {
		if (p_param2size == sizeof(matroska_setup)) {
			const matroska_setup * setup = (const matroska_setup*) p_param2;
			if (!strncmp(setup->codec_id, "A_TTA1", 6)) return true;
				else return false;
		} else return false;
	} else return false;
}

void packet_decoder_tta::open(const GUID & p_owner, bool p_decode,
	t_size p_param1, const void * p_param2, t_size p_param2size, abort_callback & p_abort) {
	const matroska_setup * setup = (const matroska_setup *) p_param2;

	cleanup();

	io.iocb.read = &read_mem_callback;
	io.iocb.write = NULL;
	io.iocb.seek = NULL;
	io.p_buffer = NULL;
	io.size = 0;

	pcm_buffer_len = tta_decoder::get_frame_length(setup->sample_rate);

	try {
		TTA = new tta_decoder((TTA_io_callback *) &io);
		TTA->decoder_init(setup->channels, MAX_BPS, setup->sample_rate, pcm_buffer_len);
	} catch (tta_exception ex) {
		cleanup();
		input_tta::tta_error(ex.code());
	}

	pcm_buffer_size = (pcm_buffer_len * MAX_DEPTH * TTA->info.nch) + 4;
	pcm_buffer = (unsigned char *) malloc(pcm_buffer_size);
	if (pcm_buffer == NULL) input_tta::tta_error(TTA_MEMORY_ERROR);
}

void packet_decoder_tta::get_info(file_info & p_info) {
	p_info.info_set("codec", "True Audio");
	p_info.info_set("encoding", "lossless");
	p_info.info_set("homepage", PROJECT_URL);
}

t_size packet_decoder_tta::set_stream_property(const GUID & p_type,	t_size p_param1, const void * p_param2, t_size p_param2size) {
	if (p_type == property_bitspersample &&
		p_param1 != 0) {
			TTA->info.bps = p_param1;
			TTA->info.depth = (p_param1 + 7) / 8;
	}

	return 0;
}

void packet_decoder_tta::analyze_first_frame(const void *p_buffer, t_size p_bytes, abort_callback &p_abort) {
	// check for supported formats
	if (TTA->info.bps < MIN_BPS || TTA->info.bps > MAX_BPS)
		input_tta::tta_error(TTA_FORMAT_ERROR);
}

void packet_decoder_tta::decode(const void * p_buffer, t_size p_bytes, audio_chunk & p_chunk, abort_callback & p_abort) {
	int samples;

	io.p_buffer = (unsigned char *)p_buffer;
	io.size = p_bytes;

	try {
		TTA->frame_init(0, (TTA_io_callback *) &io);
		samples = TTA->decode_frame(p_bytes, pcm_buffer, pcm_buffer_size);
	} catch (tta_exception ex) {
		cleanup();
		input_tta::tta_error(ex.code());
	}

	p_chunk.set_data_fixedpoint(pcm_buffer, samples * TTA->info.nch * TTA->info.depth,
		TTA->info.sps, TTA->info.nch, TTA->info.bps, audio_chunk::g_guess_channel_config(TTA->info.nch));
}

static packet_decoder_factory_t<packet_decoder_tta> g_packet_decorder_tta_factory;

DECLARE_COMPONENT_VERSION("TTA Audio Decoder", PLUGIN_VERSION, COPYRIGHT);
DECLARE_FILE_TYPE("TTA audio file", "*.TTA");
VALIDATE_COMPONENT_FILENAME("foo_input_tta.dll");

/* eof */
