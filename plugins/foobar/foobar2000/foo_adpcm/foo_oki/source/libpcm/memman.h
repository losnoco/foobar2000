#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) && defined(_DEBUG)
#define libpcm_malloc(s) libpcm_malloc_dbg(s,__FILE__,__LINE__)
#define libpcm_realloc libpcm_realloc_dbg
#define libpcm_mfree libpcm_mfree_dbg
void * LIBPCM_API libpcm_malloc_dbg(uint32_t size, char *f, int l);
void * LIBPCM_API libpcm_realloc_dbg(void *p, uint32_t size);
void LIBPCM_API libpcm_mfree_dbg(void *p);
int LIBPCM_API libpcm_leakcheck_dbg(void);
#else
void * LIBPCM_API libpcm_malloc(uint32_t size);
void * LIBPCM_API libpcm_realloc(void *p, uint32_t size);
void LIBPCM_API libpcm_mfree(void *p);
#endif

#ifdef __cplusplus
}
#endif
