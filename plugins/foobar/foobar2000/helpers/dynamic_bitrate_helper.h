class dynamic_bitrate_helper
{
public:
	dynamic_bitrate_helper();
	void on_frame(double p_duration,unsigned p_bits);
	bool on_update(file_info & p_out, double & p_timestamp_delta);
	void reset();
private:
	void init();
	double m_last_duration;
	unsigned m_update_bits;
	double m_update_time;
	double m_update_interval;
	bool m_inited, m_enabled;
};