#include "stdafx.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <msacm.h>
#include "wav_content_handler.h"
#include "wav_content_handler_acm.h"


wav_content_handler_acm::wav_content_handler_acm()
: m_abort(0)
{
}

void wav_content_handler_acm::open_e(const WAVEFORMATEX * p_wfx,service_ptr_t<file> p_reader,t_uint64 p_data_base,t_uint64 p_data_size,abort_callback & p_abort)
{
	m_reader = p_reader;
	m_data_base = p_data_base;
	m_data_size = p_data_size;
	m_position = 0;

	vartoggle_t<abort_callback*> l_abort(m_abort,&p_abort);
	WAVEFORMATEX wfx_dest;
	memset(&wfx_dest,0,sizeof(wfx_dest));
	wfx_dest.wFormatTag=WAVE_FORMAT_PCM;
	if (acmFormatSuggest(0,const_cast<WAVEFORMATEX*>(p_wfx),&wfx_dest,sizeof(wfx_dest),ACM_FORMATSUGGESTF_WFORMATTAG))
	{//no codec
		console::info("Missing ACM codec.");
		throw io_result_error_data;
	}

	if (!acm_init(const_cast<WAVEFORMATEX*>(p_wfx),&wfx_dest)) throw io_result_error_data;

	m_out_sample_rate = wfx_dest.nSamplesPerSec;
	m_out_channels = wfx_dest.nChannels;
	m_out_channel_config = audio_chunk::g_guess_channel_config(m_out_channels);
	m_out_bits_per_sample = wfx_dest.wBitsPerSample;
	
	m_align = p_wfx->nBlockAlign;
	m_byterate = p_wfx->nAvgBytesPerSec;

	m_reader->seek_e(m_data_base + m_position, p_abort);
}

bool wav_content_handler_acm::run_e(audio_chunk * p_chunk,abort_callback & p_abort)
{
	vartoggle_t<abort_callback*> l_abort(m_abort,&p_abort);

	unsigned delta = 576 * (m_out_bits_per_sample/8) * m_out_channels;

	t_uint8 * ptr = data.check_size(delta);

	delta = acm_read(ptr,delta);

	if (delta>0)
	{
		if (!p_chunk->set_data_fixedpoint(ptr,delta,m_out_sample_rate,m_out_channels,m_out_bits_per_sample,m_out_channel_config)) throw io_result_error_generic;
	
		return true;
	}
	else return false;
}

void wav_content_handler_acm::seek_e(double p_seconds,abort_callback & p_abort)
{
	vartoggle_t<abort_callback*> l_abort(m_abort,&p_abort);
	m_position = (t_int64)(p_seconds * (double)m_byterate);
	m_position -= m_position % m_align;
	
	if (m_position>m_data_size) m_position=m_data_size;

	acm_flush();

	m_reader->seek_e(m_data_base + m_position, p_abort);
}

unsigned wav_content_handler_acm::acm_read_callback(void * buf,unsigned size)
{
	assert(m_abort);
	unsigned delta = size;
	if ((t_uint64)delta > m_data_size - m_position) delta = (unsigned)(m_data_size - m_position);
	unsigned io_bytes_done;
	if (io_result_failed(m_reader->read(buf,delta,io_bytes_done,*m_abort))) return 0;
	m_position += io_bytes_done;
	return (unsigned) io_bytes_done;
}
