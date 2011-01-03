#include "upse-spu-internal.h"

#ifndef __UPSE_PS1_SPU_BASE_H__
#define __UPSE_PS1_SPU_BASE_H__

upse_spu_state_t *upse_ps1_spu_open(upse_module_instance_t *ins);
void upse_ps1_spu_setlength(upse_spu_state_t *spu, s32 stop, s32 fade);
void upse_ps1_spu_close(upse_spu_state_t *spu);
void upse_ps1_spu_finalize(upse_spu_state_t *spu);

// External, called by SPU code.
void upse_ps1_spu_irq_callback(upse_module_instance_t *ins);
void upse_ps1_spu_setvolume(upse_spu_state_t *spu, int volume);
void upse_ps1_spu_stop(upse_module_instance_t *ins);

int upse_ps1_spu_finalize_count(upse_spu_state_t *spu, s16 **s);

void upse_ps1_spu_set_audio_callback(upse_module_instance_t *ins, upse_audio_callback_func_t func, const void *user_data);
int upse_ps1_spu_seek(upse_module_instance_t *ins, u32 t);

#endif
