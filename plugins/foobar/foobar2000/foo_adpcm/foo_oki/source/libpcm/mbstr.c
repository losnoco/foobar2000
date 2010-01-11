#include "libpcm.h"
#include "mbstr.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>

void LIBPCM_API libpcm_initlocale(void)
{
}

const char * LIBPCM_API libpcm_charnext(const char *s, uint_t count)
{
	const char *p = s ? CharNextA(s) : 0;
	return (p != s) ? p : 0;
}

void LIBPCM_API libpcm_strncpy(char *d, const char *s, uint_t l)
{
	lstrcpyn(d, s, l);
}
uint_t LIBPCM_API libpcm_strlen(const char *s)
{
	return lstrlen(s);
}

#else

#include <stdlib.h>
#include <string.h>
#include <locale.h>

void LIBPCM_API libpcm_initlocale(void)
{
	static const char default_locale[] = "";
	setlocale(LC_ALL, default_locale);
}
#ifdef MB_CUR_MAX
const char * LIBPCM_API libpcm_charnext(const char *s, uint_t count)
{
	int l = mblen(s, (count > MB_CUR_MAX) ? MB_CUR_MAX : count);
	return (l > 0) ? s + l : 0;
}
#else
const char * LIBPCM_API libpcm_charnext(const char  *s, uint_t count)
{
	return (s && *s) ? s + 1 : 0;
}
#endif
void LIBPCM_API libpcm_strncpy(char *d, const char *s, uint_t l)
{
	strncpy(d, s, l);
}
uint_t LIBPCM_API libpcm_strlen(const char *s)
{
	return strlen(s);
}
#endif
