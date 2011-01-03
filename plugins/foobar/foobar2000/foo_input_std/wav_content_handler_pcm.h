class wav_content_handler_pcm : public wav_content_handler
{
public:
	void open_e(const WAVEFORMATEX * p_wfx,service_ptr_t<file> p_reader,t_uint64 p_data_base,t_uint64 p_data_size,abort_callback & p_abort);
	bool run_e(audio_chunk * p_chunk,abort_callback & p_abort);
	void seek_e(double p_seconds,abort_callback & p_abort);
private:

	void init_e(const WAVEFORMATEX * p_wfx,bool p_is_float);

	service_ptr_t<file> m_reader;
	t_uint64 m_data_base,m_data_size, m_position_target, m_position_current;
	
	unsigned m_sample_rate, m_channels, m_channel_config, m_bits_per_sample, m_align;
	bool m_is_float;
	mem_block_t<t_uint8> m_buffer;
};
