#include "libpcm_config.h"

typedef struct libpcm_IReader
{
	const struct libpcm_IReaderVtbl *lpVtbl;
} libpcm_IReader;
typedef struct libpcm_IReaderVtbl
{
	void * (LIBPCM_CALLBACK * AddRef)(void * This);
	void (LIBPCM_CALLBACK * Release)(void * This);
	uint32_t (LIBPCM_CALLBACK * Size)(void * This);
	uint_t (LIBPCM_CALLBACK * Read)(void * This, void *buf, uint_t nbytes);
	void (LIBPCM_CALLBACK * Seek)(void * This, uint32_t nbytes);
	const char *(LIBPCM_CALLBACK * Path)(void * This);
} libpcm_IReaderVtbl;

typedef struct LIBPCM_DECODER_tag LIBPCM_DECODER;

#ifdef __cplusplus
extern "C"
{
#endif

void LIBPCM_API libpcm_initlocale(void);
void LIBPCM_API libpcm_close(LIBPCM_DECODER *d);
const char * LIBPCM_API libpcm_get_title(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_length(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_length_ms(LIBPCM_DECODER *d);
uint_t LIBPCM_API libpcm_get_samplerate(LIBPCM_DECODER *d);
uint_t LIBPCM_API libpcm_get_bitspersample(LIBPCM_DECODER *d);
uint_t LIBPCM_API libpcm_get_numberofchannels(LIBPCM_DECODER *d);
uint_t LIBPCM_API libpcm_get_blockalign(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_currentposition(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_currentposition_ms(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_bitrate(LIBPCM_DECODER *d);
void LIBPCM_API libpcm_switch_loop(LIBPCM_DECODER *d, int sw);
uint32_t LIBPCM_API libpcm_get_looplength(LIBPCM_DECODER *d);
uint32_t LIBPCM_API libpcm_get_looplength_ms(LIBPCM_DECODER *d);
uint_t LIBPCM_API libpcm_get_codec(LIBPCM_DECODER *d);
const char * LIBPCM_API libpcm_get_codecname(int codec);

#ifdef __cplusplus
}
#endif

LIBPCM_DECODER * LIBPCM_API libpcm_open_from_stream(libpcm_IReader *preader);
LIBPCM_DECODER * LIBPCM_API libpcm_open_from_file(const char *path);
uint_t LIBPCM_API libpcm_read(LIBPCM_DECODER *d, void *buf, uint_t nsamples);
void LIBPCM_API libpcm_seek(LIBPCM_DECODER *d, uint32_t nsamples);
void LIBPCM_API libpcm_seek_ms(LIBPCM_DECODER *d, uint32_t ms);
