class parser_ac3
{
	unsigned              packet_size;
	unsigned              buffer_filled;
	pfc::array_t<t_uint8> buffer;

public:
	parser_ac3();

	void reset();

	unsigned sync( service_ptr_t<file> &, int & flags, int & sample_rate, int & bit_rate, abort_callback & );

	inline t_uint8 const* packet_get() { return buffer.get_ptr(); }
	inline t_filesize     packet_offset( t_filesize p_position ) { return p_position - buffer_filled; }
	void                  packet_flush();

private:
	void buffer_fill( service_ptr_t<file> &, abort_callback & );
	void buffer_remove( size_t );
};