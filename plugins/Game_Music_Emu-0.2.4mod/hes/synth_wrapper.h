#ifndef SYNTH_WRAPPER_H
#define SYNTH_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void synth_offset( void * blip_synth, long timestamp, int delta, void * blip_buffer );

#ifdef __cplusplus
}
#endif

#endif
