class hasher_crc16
{
	uint16_t state;

public:
	inline hasher_crc16(t_uint16 initializer = 0) { state = initializer; }
	void process(const void * p_buffer, size_t p_bytes);
	inline uint16_t get_result() const { return state; }
};