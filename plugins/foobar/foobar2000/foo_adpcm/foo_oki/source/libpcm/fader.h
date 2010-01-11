typedef struct LIBPCM_FADER_tag LIBPCM_FADER;

#ifdef __cplusplus
extern "C"
{
#endif

LIBPCM_FADER * LIBPCM_API libpcm_fader_initialize(LIBPCM_DECODER *d);
void LIBPCM_API libpcm_fader_terminate(LIBPCM_FADER *f);
void LIBPCM_API libpcm_fader_configure(LIBPCM_FADER *f, uint_t loopcount, uint32_t fadetime_ms);
uint_t LIBPCM_API libpcm_fader_read(LIBPCM_FADER *f, void *buf, uint_t nsamples);
uint32_t LIBPCM_API libpcm_fader_get_length_ms(LIBPCM_FADER *f);
LIBPCM_DECODER * LIBPCM_API libpcm_fader_get_decoder(LIBPCM_FADER *f);
#ifdef __cplusplus
}
#endif
