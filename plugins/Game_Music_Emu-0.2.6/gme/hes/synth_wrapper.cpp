#include "synth_wrapper.h"

#include "../Blip_Buffer.h"
#include "../Blip_Synth.h"
#include "../Hes_Emu.h"

void synth_offset( void * blip_synth, long timestamp, int delta, void * blip_buffer )
{
	( ( Hes_Emu::synth_t * ) blip_synth )->offset_inline( ( blip_time_t ) timestamp, delta, ( Blip_Buffer * ) blip_buffer );
}
