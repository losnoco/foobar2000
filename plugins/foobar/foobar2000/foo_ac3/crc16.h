class hasher_crc16
{
	t_uint16 state;

public:
	inline hasher_crc16(t_uint16 initializer = 0) { state = initializer; }
	void process(const void * p_buffer, t_size p_bytes);
	inline t_uint16 get_result() const { return state; }
};