static void strncpy_addnull(char * dest,const char * src,int max)
{
	int n;
	for(n=0;n<max-1 && src[n];n++)
		dest[n] = src[n];
	dest[n] = 0;
}

static void wcsncpy_addnull(WCHAR * dest,const WCHAR * src,int max)
{
	int n;
	for(n=0;n<max-1 && src[n];n++)
		dest[n] = src[n];
	dest[n] = 0;
}

#ifdef UNICODE
#define tcsncpy_addnull wcsncpy_addnull
#else
#define tcsncpy_addnull strncpy_addnull
#endif
