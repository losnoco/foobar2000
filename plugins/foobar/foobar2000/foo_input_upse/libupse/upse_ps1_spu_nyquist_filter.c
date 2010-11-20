/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_spu_lowpass_filter.c
 * Purpose: libupse: PS1 SPU band-limiting filter.
 *
 * Copyright (c) 2010 William Pitcock <nenolod@sacredspiral.co.uk>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#define _IN_SPU

#include "upse-types.h"
#include "upse-internal.h"
#include "upse-ps1-spu-base.h"
#include "upse-spu-internal.h"
#include "upse-ps1-spu-register-io.h"
#include "upse-ps1-spu-reverb.h"
#include "upse-ps1-spu-adsr-filter.h"

#include "upse.h"
#include "upse-ps1-memory-manager.h"

#include <math.h>

#define CLIP(x) { if(x > 32767) x = 32767; if(x < -32767) x = -32767; }

void
upse_spu_nyquist_filter_process(upse_spu_state_t *spu, s16 *samplebuf, int samplecount)
{
    int i;

    for (i = 0; i < samplecount; i++)
    {
        s32 l, r;

        l = samplebuf[0];
        r = samplebuf[1];

        l += (l - spu->nyquist.hist[0]);
        r += (r - spu->nyquist.hist[1]);

        CLIP(l);
        CLIP(r);

        spu->nyquist.hist[0] = samplebuf[0];
        spu->nyquist.hist[1] = samplebuf[1];

        samplebuf[0] = l;
        samplebuf[1] = r;

        samplebuf += 2;
    }
}
