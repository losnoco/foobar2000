class wav_content_handler
{
public:
	virtual void open_e(const WAVEFORMATEX * p_wfx,service_ptr_t<file> p_reader,t_uint64 p_data_base,t_uint64 p_data_size,abort_callback & p_abort) = 0;
	virtual bool run_e(audio_chunk * p_chunk,abort_callback & p_abort) = 0;
	virtual void seek_e(double p_seconds,abort_callback & p_abort) = 0;
	virtual ~wav_content_handler() {}
};
