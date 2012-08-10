#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void fir_init();

void fir_shutdown();

typedef float (*callback_get_sample)( void * context, size_t channel, size_t offset );
typedef void (*callback_put_sample)( void * context, size_t channel, size_t offset, float sample );

int fir_latency( float ratio );
int fir_required_input( size_t output_count, float ratio, float frequency_accumulator );
int fir_resample( callback_get_sample getter, callback_put_sample putter, void * context, size_t channels, size_t output_count, float ratio, float * frequency_accumulator );

#ifdef __cplusplus
}
#endif