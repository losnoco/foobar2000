#include "libpcm.h"
#include "util.h"


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>

sint32_t LIBPCM_API libpcm_mulfix(sint32_t m, sint32_t n, int d)
{
	return (sint32_t)Int64ShraMod32(Int32x32To64(m, n), d);
}
sint32_t LIBPCM_API libpcm_muldiv(sint32_t m, sint32_t n, sint32_t d)
{
	return MulDiv(m, n, d);
}

#else

sint32_t LIBPCM_API libpcm_mulfix(sint32_t m, sint32_t n, int d)
{
	return (sint32_t)((((sint64_t)m) * ((sint64_t)n)) >> d);
}
sint32_t LIBPCM_API libpcm_muldiv(sint32_t m, sint32_t n, sint32_t d)
{
	return (sint32_t)((((sint64_t)m) * ((sint64_t)n)) / d);
}

#endif
