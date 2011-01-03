#include "acm.h"

class wav_content_handler_acm : public wav_content_handler, private acm_converter
{
public:
	wav_content_handler_acm();
	void open_e(const WAVEFORMATEX * p_wfx,service_ptr_t<file> p_reader,t_uint64 p_data_base,t_uint64 p_data_size,abort_callback & p_abort);
	bool run_e(audio_chunk * p_chunk,abort_callback & p_abort);
	void seek_e(double p_seconds,abort_callback & p_abort);
private:
	unsigned acm_read_callback(void * buf,unsigned size);

	unsigned m_out_channels,m_out_channel_config,m_out_bits_per_sample,m_out_sample_rate,m_align,m_byterate;

	t_uint64 m_data_base,m_data_size,m_position;

	service_ptr_t<file> m_reader;

	mem_block_t<t_uint8> data;

	abort_callback * m_abort;

};
