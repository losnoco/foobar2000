typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned char   uint8_t;
typedef signed char     int8_t;

extern "C" {
#include "dca.h"
}

class parser_dts
{
	unsigned              packet_size;
	unsigned              buffer_filled;
	pfc::array_t<t_uint8> buffer;

	dca_state_t         * dca_temp;

public:
	parser_dts();
	~parser_dts();

	void reset();

	unsigned sync( service_ptr_t<file> &, int & flags, int & sample_rate, int & bit_rate, int & frame_length, abort_callback & );

	inline t_uint8 const* packet_get() { return buffer.get_ptr(); }
	inline t_filesize     packet_offset( t_filesize p_position ) { return p_position - buffer_filled; }
	void                  packet_flush();

private:
	void buffer_fill( service_ptr_t<file> &, abort_callback & );
	void buffer_remove( size_t );
};