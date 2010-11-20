/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_gpu.c
 * Purpose: libupse: PS1 bogus GPU emulator (for lame replayers)
 *
 * Copyright (c) 2008 William Pitcock <nenolod@sacredspiral.co.uk>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include "upse-internal.h"

typedef enum {
    UPSE_PS1_GPU_INITIALIZED = 0x14802000,

    /* primitive ops */
    UPSE_PS1_GPU_DITHER      = 0x00000200,
    UPSE_PS1_GPU_CANDRAW     = 0x00000400,
    UPSE_PS1_GPU_MASKDRAWN   = 0x00000800,
    UPSE_PS1_GPU_MASKENABLE  = 0x00001000,

    /* generic flags */
    UPSE_PS1_GPU_BITWIDTH    = 0x00070000,
    UPSE_PS1_GPU_DBLHEIGHT   = 0x00080000,
    UPSE_PS1_GPU_PAL         = 0x00100000,
    UPSE_PS1_GPU_TRUECOLOR   = 0x00200000,
    UPSE_PS1_GPU_INTERLACED  = 0x00400000,
    UPSE_PS1_GPU_NODISPLAY   = 0x00800000,

    /* status flags */
    UPSE_PS1_GPU_IDLE        = 0x04000000,
    UPSE_PS1_GPU_VRAMXFER    = 0x08000000,
    UPSE_PS1_GPU_READY       = 0x10000000,
    UPSE_PS1_GPU_DMA         = 0x60000000,
    UPSE_PS1_GPU_ODDLINES    = 0x80000000
} upse_gpu_status_t;

static struct {
    upse_gpu_status_t status;
} upse_gpu_state = { UPSE_PS1_GPU_INITIALIZED };

void
upse_ps1_gpu_set_status(u32 cmd)
{
    switch(cmd)
    {
        default:
            _DEBUG("GPU: Unknown cmd [0x%x]", cmd);
            return;
    }
}

u32
upse_ps1_gpu_get_status(void)
{
    _DEBUG("GPU status: 0x%x", upse_gpu_state.status);

    return upse_gpu_state.status;
}
