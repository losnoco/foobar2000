#ifndef _RAR_UNICODE_
#define _RAR_UNICODE_

#ifndef _EMX
#define MBFUNCTIONS
#endif

#if defined(MBFUNCTIONS) || defined(_WIN_32) || defined(_EMX) && !defined(_DJGPP)
#define UNICODE_SUPPORTED
#endif

#ifdef _WIN_32
#define DBCS_SUPPORTED
#endif

#ifdef _EMX
int uni_init(int codepage);
int uni_done();
#endif

bool CharToWide(const char *Src,wchar_t *Dest,int DestSize=0x1000000);

void WideToUtf(const wchar_t *Src,char *Dest,int DestSize);

#endif
