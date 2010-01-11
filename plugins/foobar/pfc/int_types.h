#if !defined(_MSC_VER) && !defined(_EVC_VER)
#include <stdint.h>
#else
typedef __int64 t_int64;
typedef unsigned __int64 t_uint64;
typedef __int32 t_int32;
typedef unsigned __int32 t_uint32;
typedef __int16 t_int16;
typedef unsigned __int16 t_uint16;
typedef __int8 t_int8;
typedef unsigned __int8 t_uint8;
#endif

typedef int t_int;
typedef unsigned int t_uint;

typedef float t_float32;
typedef double t_float64;



#if defined(_WIN32) && !defined(_WIN64)
#define __PFC_WP64 __w64
#else
#define __PFC_WP64
#endif


namespace pfc {
	template<unsigned t_bytes>
	class sized_int_t;

	template<> class sized_int_t<1> {
	public:
		typedef t_uint8 t_unsigned;
		typedef t_int8 t_signed;
	};

	template<> class sized_int_t<2> {
	public:
		typedef t_uint16 t_unsigned;
		typedef t_int16 t_signed;
	};

	template<> class sized_int_t<4> {
	public:
		typedef t_uint32 t_unsigned;
		typedef t_int32 t_signed;
	};

	template<> class sized_int_t<8> {
	public:
		typedef t_uint64 t_unsigned;
		typedef t_int64 t_signed;
	};
}


typedef pfc::sized_int_t<sizeof(void*)>::t_unsigned __PFC_WP64 t_size;
typedef pfc::sized_int_t<sizeof(void*)>::t_signed __PFC_WP64 t_ssize;


#define infinite (~0)

const t_uint16 infinite16 = (t_uint16)(~0);
const t_uint32 infinite32 = (t_uint32)(~0);
const t_uint64 infinite64 = (t_uint64)(~0);
const t_size infinite_size = (t_size)(~0);


#if defined(_WIN32) && !defined(_WIN64)
inline t_size MulDiv_Size(t_size x,t_size y,t_size z) {return (t_size) ( ((t_uint64)x * (t_uint64)y) / (t_uint64)z );}
#elif defined(_WIN64)
inline t_size MulDiv_Size(t_size x,t_size y,t_size z) {return (x*y)/z;}
#else
#error portme
#endif
