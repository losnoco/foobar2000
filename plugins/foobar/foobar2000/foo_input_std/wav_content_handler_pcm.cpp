#include "stdafx.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "wav_content_handler.h"
#include "wav_content_handler_pcm.h"

void wav_content_handler_pcm::init_e(const WAVEFORMATEX * p_wfx, bool p_is_float)
{
	m_is_float = p_is_float;
	m_sample_rate = p_wfx->nSamplesPerSec;
	m_channels = p_wfx->nChannels;
	m_channel_config = audio_chunk::g_guess_channel_config(m_channels);
	m_bits_per_sample = p_wfx->wBitsPerSample;
	m_align = (m_bits_per_sample/8) * m_channels;
}

void wav_content_handler_pcm::open_e(const WAVEFORMATEX * p_wfx,service_ptr_t<file> p_reader,t_uint64 p_data_base,t_uint64 p_data_size,abort_callback & p_abort)
{
	m_reader = p_reader;
	m_data_base = p_data_base;
	m_data_size = p_data_size;

	if (p_wfx->wFormatTag == WAVE_FORMAT_PCM)
	{
		init_e(p_wfx,false);
	}
	else if (p_wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
	{
		init_e(p_wfx,true);
	}
	else if (p_wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		if (p_wfx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) throw io_result_error_data;
		const WAVEFORMATEXTENSIBLE * wfxe = reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(p_wfx);
		if (wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
		{
			init_e(&wfxe->Format,false);
			m_channel_config = audio_chunk::g_channel_config_from_wfx(wfxe->dwChannelMask);
		}
		else if (wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			init_e(&wfxe->Format,true);
			m_channel_config = audio_chunk::g_channel_config_from_wfx(wfxe->dwChannelMask);
		}
		else
		{
			throw io_result_error_data;
		}
	}
	else
	{
		throw io_result_error_data;
	}


	if (m_channels < 1 || m_channels > 256 
		|| m_sample_rate < 1000 || m_sample_rate > 1000000
		) throw io_result_error_data;

	if (m_is_float)
	{
		if (m_bits_per_sample != 32 && m_bits_per_sample != 64)
			throw io_result_error_data;
	}
	else
	{
		if (m_bits_per_sample != 8 && m_bits_per_sample != 16 && m_bits_per_sample != 24 && m_bits_per_sample != 32)
			throw io_result_error_data;
	}

	m_position_current = infinite64;
	m_position_target = 0;

	m_data_size -= m_data_size % m_align;
}

bool wav_content_handler_pcm::run_e(audio_chunk * p_chunk,abort_callback & p_abort)
{
	assert( m_data_size % m_align == 0 );
	assert( m_position_target % m_align == 0);

	if (m_position_target != m_position_current)
	{
		m_reader->seek_e(m_data_base + m_position_target,p_abort);
		m_position_current = m_position_target;
	}

	unsigned delta = 1024 * m_align;
	if (m_position_current + delta > m_data_size)
	{
		delta = (unsigned)(m_data_size - m_position_current);
		assert( delta % m_align == 0 );
	}

	if (delta == 0) return false;
	
	t_uint8 * buffer = m_buffer.check_size(delta);

	if (buffer == 0) throw io_result_error_data;
	
	unsigned delta_read;
	delta_read = m_reader->read_e(buffer,delta,p_abort);
	delta_read -= delta_read % m_align;

	if (delta_read != delta)
	{
		m_data_size = m_position_current + delta_read;
	}

	if (delta_read == 0) return false;

	m_position_current += delta_read;
	m_position_target += delta_read;

	if (m_is_float)
	{
		if (!p_chunk->set_data_floatingpoint_ex(
			buffer,delta_read,m_sample_rate,m_channels,m_bits_per_sample,
			audio_chunk::FLAG_LITTLE_ENDIAN,
			m_channel_config))
			throw io_result_error_data;
		
	}
	else
	{
		if (!p_chunk->set_data_fixedpoint_ex(
			buffer,delta_read,m_sample_rate,m_channels,m_bits_per_sample,
			audio_chunk::FLAG_LITTLE_ENDIAN | ( m_bits_per_sample == 8 ? audio_chunk::FLAG_UNSIGNED : audio_chunk::FLAG_SIGNED ),
			m_channel_config))
			throw io_result_error_data;
	}

	
	return true;
}

void wav_content_handler_pcm::seek_e(double p_seconds,abort_callback & p_abort)
{
	if (p_seconds < 0) throw io_result_error_data;
	t_uint64 new_offset = m_align * (t_uint64) ( p_seconds * m_sample_rate );
	if (new_offset > m_data_size) throw io_result_error_data;
	m_position_target = new_offset;
}
