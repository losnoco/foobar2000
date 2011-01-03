#include "stdafx.h"
#include <mmreg.h>
#include <msacm.h>
#include <ks.h>
#include <ksmedia.h>
#include <msacm.h>

#include "wav_content_handler.h"
#include "wav_content_handler_pcm.h"
#include "wav_content_handler_acm.h"


struct t_riffcode
{
	char m_data[4];

	static bool g_test(const t_riffcode & p_code,const char * p_name)
	{
		return p_code.m_data[0] == p_name[0] 
			&& p_code.m_data[1] == p_name[1]
			&& p_code.m_data[2] == p_name[2]
			&& p_code.m_data[3] == p_name[3];
	}

	bool test(const char * p_name)
	{
		return g_test(*this,p_name);
	}
};





class input_wav
{
public:


	bool can_seek() {return true;}

	void set_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		throw io_result_error_data;
	}

	static bool g_is_our_content_type(const char * p_url,const char * p_type) {return false;}
	static bool g_test_filename(const char * p_path,const char * p_extension) {return !stricmp_utf8(p_extension,"WAV");}
	static bool g_needs_reader() {return true;}
	static GUID g_get_guid()
	{
		// {27520C0B-3D97-468b-BD3E-FA403D1B155A}
		static const GUID guid = 
		{ 0x27520c0b, 0x3d97, 0x468b, { 0xbd, 0x3e, 0xfa, 0x40, 0x3d, 0x1b, 0x15, 0x5a } };
		return guid;
	}
	static const char * g_get_name() {return "WAV parser";}

	void seek(double p_seconds,abort_callback & p_abort);
	bool run(audio_chunk * p_chunk,abort_callback & p_abort);
	void open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags);
	void get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort);

	bool get_dynamic_info(file_info & p_out, double * p_timestamp_delta,bool * p_track_change) {return false;}
	inline static void g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {throw io_result_error_data;}
private:

	void initialize_e(const service_ptr_t<file> & p_reader,abort_callback & p_abort);
	void process_info_e(file_info & p_info,abort_callback & p_abort);

	const WAVEFORMATEX * get_wfx() const {return reinterpret_cast<const WAVEFORMATEX*>(m_wfx_data.get_ptr());}
	unsigned get_wfx_bytes() const {return m_wfx_data.get_size();}

	service_ptr_t<file> m_reader;
	mem_block_t<t_uint8> m_wfx_data;

	pfc::autoptr_t<wav_content_handler> m_handler;

	t_uint64 m_riff_main_size, m_riff_data_size, m_riff_main_base, m_riff_data_base;
	
};

static input_factory_eh_t<input_wav> g_input_wav_factory;

void input_wav::initialize_e(const service_ptr_t<file> & p_reader,abort_callback & p_abort)
{
	t_riffcode code;
	t_uint32 size;
	t_uint64 riff_main_offset;

	m_reader = p_reader;

	if (!m_reader->can_seek()) throw io_result_error_data;
	m_reader->seek_e(0,p_abort);

	m_reader->read_object_e_t(code,p_abort);
	if (!code.test("RIFF")) throw io_result_error_data;

	m_reader->read_lendian_e_t(size,p_abort);
	m_riff_main_size = size;
	m_riff_main_base = 8;
	riff_main_offset = 0;

	m_reader->read_object_e_t(code,p_abort);
	riff_main_offset += 4;
	
	if (!code.test("WAVE")) throw io_result_error_data;

	bool fmt_found = false, data_found = false;

	while(riff_main_offset < m_riff_main_size)
	{
		m_reader->read_object_e_t(code,p_abort);
		riff_main_offset += 4;
		m_reader->read_object_e_t(size,p_abort);
		riff_main_offset += 4;

		if (code.test("fmt "))
		{
			if (size == 16)//non-compliant(?) WAVs, apparently very common
			{
				WAVEFORMATEX wfx;
				m_reader->read_object_e(&wfx,16,p_abort);
				wfx.cbSize = 0;
				if (0 == m_wfx_data.set_data(reinterpret_cast<const t_uint8*>(&wfx),sizeof(wfx))) throw io_result_error_data;
			}
			else
			{
				if (size < sizeof(WAVEFORMATEX) || size > 1024*1024) throw io_result_error_data;

				t_uint8 * buffer = m_wfx_data.set_size(size);
				if (buffer == 0) throw io_result_error_data;

				m_reader->read_object_e(buffer,size,p_abort);

			}

			const WAVEFORMATEX * wfx = get_wfx();
			if (get_wfx_bytes() < sizeof(WAVEFORMATEX)) /*shouldn't ever trigger here*/ throw io_result_error_data;
			if (wfx->cbSize > get_wfx_bytes() - sizeof(WAVEFORMATEX)) throw io_result_error_data;

			if (wfx->nChannels < 1 || wfx->nChannels > 256
				|| wfx->nSamplesPerSec < 1000 || wfx->nSamplesPerSec > 1000000
				|| wfx->nAvgBytesPerSec == 0
				|| wfx->nBlockAlign == 0
				) throw io_result_error_data;

			fmt_found = true;
		}
		else if (code.test("data"))
		{
			m_riff_data_base = m_riff_main_base + riff_main_offset;
			m_riff_data_size = size;

			data_found = true;
		}
		
		riff_main_offset += size;
		if (riff_main_offset & 1) riff_main_offset++;

		m_reader->seek_e(m_riff_main_base + riff_main_offset,p_abort);
	}

	if (!fmt_found || !data_found) throw io_result_error_data;
}

void input_wav::process_info_e(file_info & p_info,abort_callback & p_abort)
{
	const WAVEFORMATEX * wfx = get_wfx();
	
	{
		t_io_result status;
		status = tag_processor::read_trailing(m_reader,p_info,p_abort);
		if (status != io_result_error_data && status != io_result_error_not_found && io_result_failed(status)) throw status;
	}

	p_info.set_length((double)m_riff_data_size / (double)wfx->nAvgBytesPerSec);
	p_info.info_set_int("bitrate",wfx->nAvgBytesPerSec/125);
	p_info.info_set_int("samplerate",wfx->nSamplesPerSec);
	p_info.info_set_int("channels",wfx->nChannels);

	if (wfx->wFormatTag==WAVE_FORMAT_PCM)
	{
		p_info.info_set("codec","PCM");
	}
	else if (wfx->wFormatTag==WAVE_FORMAT_IEEE_FLOAT)
	{
		p_info.info_set("codec","PCM (floating-point)");
	}
	else
	{
		ACMFORMATTAGDETAILS afd;
		memset(&afd,0,sizeof(afd));
		afd.cbStruct = sizeof(afd);
		afd.dwFormatTag = wfx->wFormatTag;
		if (SUCCEEDED(acmFormatTagDetails(0,&afd,ACM_FORMATTAGDETAILSF_FORMATTAG)))
		{
			p_info.info_set("codec",string_utf8_from_ansi(afd.szFormatTag));
		}
	}

	if (wfx->wBitsPerSample>0) p_info.info_set_int("bitspersample",wfx->wBitsPerSample);

	if (wfx->wFormatTag != WAVE_FORMAT_PCM && wfx->wFormatTag != WAVE_FORMAT_IEEE_FLOAT && wfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEX wfx_dest;
		memset(&wfx_dest,0,sizeof(wfx_dest));
		wfx_dest.wFormatTag=WAVE_FORMAT_PCM;
		if (!acmFormatSuggest(0,const_cast<WAVEFORMATEX*> /* wtf isn't this param decalred as const ??? */ (wfx),&wfx_dest,sizeof(wfx_dest),ACM_FORMATSUGGESTF_WFORMATTAG))
		{
			if (wfx_dest.wBitsPerSample>0) p_info.info_set_int("decoded_bitspersample",wfx_dest.wBitsPerSample);
		}
	}

}

void input_wav::seek(double p_seconds,abort_callback & p_abort)
{
	if (m_handler.is_empty()) throw io_result_error_data;
	m_handler->seek_e(p_seconds,p_abort);
}

bool input_wav::run(audio_chunk * p_chunk,abort_callback & p_abort)
{
	if (m_handler.is_empty()) throw io_result_error_data;
	return m_handler->run_e(p_chunk,p_abort);
}

void input_wav::open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
{
	initialize_e(p_reader,p_abort);
	if (p_flags & input::OPEN_FLAG_GET_INFO) process_info_e(p_info,p_abort);
	
	const WAVEFORMATEX * wfx = get_wfx();
	if (wfx->wFormatTag == WAVE_FORMAT_PCM || wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
	{
		m_handler = new wav_content_handler_pcm;
	}
	else if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		if (wfx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) throw io_result_error_data;
		const WAVEFORMATEXTENSIBLE * wfxe = reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(wfx);
		if (wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_PCM || wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			m_handler = new wav_content_handler_pcm;
		}
		else
		{
			m_handler = new wav_content_handler_acm;
		}
	}
	else
	{
		m_handler = new wav_content_handler_acm;
	}

	if (m_handler.is_empty()) throw io_result_error_data;
	

	try {
		m_handler->open_e(wfx,m_reader,m_riff_data_base,m_riff_data_size,p_abort);
	} catch(t_io_result) {
		m_handler = 0;
		throw;
	}
}

void input_wav::get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
{
	initialize_e(p_reader,p_abort);
	process_info_e(p_info,p_abort);
}

DECLARE_FILE_TYPE("WAV files","*.WAV");
