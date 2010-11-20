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

void
upse_spu_lowpass_filter_reset(upse_spu_state_t *spu)
{
    spu->lowpass.lx1 = 0;
    spu->lowpass.lx2 = 0;
    spu->lowpass.ly1 = 0;
    spu->lowpass.ly2 = 0;

    spu->lowpass.hx1[0] = 0;
    spu->lowpass.hx2[0] = 0;
    spu->lowpass.hy1[0] = 0;
    spu->lowpass.hy2[0] = 0;

    spu->lowpass.hx1[1] = 0;
    spu->lowpass.hx2[1] = 0;
    spu->lowpass.hy1[1] = 0;
    spu->lowpass.hy2[1] = 0;
}

void
upse_spu_lowpass_filter_redesign(upse_spu_state_t *spu, int samplerate)
{
    switch (samplerate)
    {
    case 48000:
        spu->lowpass.la0 = 1.00320890889339290000;
        spu->lowpass.la1 = -1.97516434134506300000;
        spu->lowpass.la2 = 0.97243484967313087000;
        spu->lowpass.lb1 = -1.97525280404731810000;
        spu->lowpass.lb2 = 0.97555529586426892000;

        spu->lowpass.ha0 = 1.52690772687271160000;
        spu->lowpass.ha1 = -1.62653918974914990000;
        spu->lowpass.ha2 = 0.57997976029249387000;
        spu->lowpass.hb1 = -0.80955590379048203000;
        spu->lowpass.hb2 = 0.28990420120653748000;

        break;

    default:
        spu->lowpass.la0 = 1.00349314378906680000;
        spu->lowpass.la1 = -1.97295980267143170000;
        spu->lowpass.la2 = 0.97003400595243994000;
        spu->lowpass.lb1 = -1.97306449030610280000;
        spu->lowpass.lb2 = 0.97342246210683581000;

        spu->lowpass.ha0 = 1.50796284998687450000;
        spu->lowpass.ha1 = -1.48628361940858910000;
        spu->lowpass.ha2 = 0.52606706092412581000;
        spu->lowpass.hb1 = -0.71593574211151134000;
        spu->lowpass.hb2 = 0.26368203361392234000;

        break;
    }

    upse_spu_lowpass_filter_reset(spu);
}

#define EPSILON (1e-10f)
#define OVERALL_SCALE (0.87f)
#define OVERFLOW_CHECK(x) if(fabsf((x)) < EPSILON) (x) = 0
#define CLIP(x) { if(x > 32767) x = 32767; if(x < -32767) x = -32767; }

void
upse_spu_lowpass_filter_process(upse_spu_state_t *spu, s16 *samplebuf, int samplecount)
{
    int i;

    OVERFLOW_CHECK(spu->lowpass.lx1);
    OVERFLOW_CHECK(spu->lowpass.lx2);
    OVERFLOW_CHECK(spu->lowpass.ly1);
    OVERFLOW_CHECK(spu->lowpass.ly2);

    OVERFLOW_CHECK(spu->lowpass.hx1[0]);
    OVERFLOW_CHECK(spu->lowpass.hx2[0]);
    OVERFLOW_CHECK(spu->lowpass.hy1[0]);
    OVERFLOW_CHECK(spu->lowpass.hy2[0]);

    OVERFLOW_CHECK(spu->lowpass.hx1[1]);
    OVERFLOW_CHECK(spu->lowpass.hx2[1]);
    OVERFLOW_CHECK(spu->lowpass.hy1[1]);
    OVERFLOW_CHECK(spu->lowpass.hy2[1]);

    for (i = 0; i < samplecount; i++)
    {
        s32 in, out;
        s32 l, r;
		s32 mid, side;

        l = samplebuf[0];
        r = samplebuf[1];

        mid  = l + r;
        side = l - r;

        in = mid;
        out = spu->lowpass.la0 * in + spu->lowpass.la1 * spu->lowpass.lx1 + spu->lowpass.la2 * spu->lowpass.lx2 - spu->lowpass.lb1 * spu->lowpass.ly1 - spu->lowpass.lb2 * spu->lowpass.ly2;

        spu->lowpass.lx2 = spu->lowpass.lx1;
        spu->lowpass.lx1 = in;

        spu->lowpass.ly2 = spu->lowpass.ly1;
        spu->lowpass.ly1 = out;

        mid = out;

        l = ((0.5) * (OVERALL_SCALE)) * (mid + side);
        r = ((0.5) * (OVERALL_SCALE)) * (mid - side);

        in = l;
        out = spu->lowpass.ha0 * in + spu->lowpass.ha1 * spu->lowpass.hx1[0] + spu->lowpass.ha2 * spu->lowpass.hx2[0] - spu->lowpass.hb1 * spu->lowpass.hy1[0] - spu->lowpass.hb2 * spu->lowpass.hy2[0];
        spu->lowpass.hx2[0] = spu->lowpass.hx1[0]; spu->lowpass.hx1[0] = in;
        spu->lowpass.hy2[0] = spu->lowpass.hy1[0]; spu->lowpass.hy1[0] = out;
        l = out;

        in = r;
        out = spu->lowpass.ha0 * in + spu->lowpass.ha1 * spu->lowpass.hx1[1] + spu->lowpass.ha2 * spu->lowpass.hx2[1] - spu->lowpass.hb1 * spu->lowpass.hy1[1] - spu->lowpass.hb2 * spu->lowpass.hy2[1];
        spu->lowpass.hx2[1] = spu->lowpass.hx1[1]; spu->lowpass.hx1[1] = in;
        spu->lowpass.hy2[1] = spu->lowpass.hy1[1]; spu->lowpass.hy1[1] = out;
        r = out;

        CLIP(l);
        CLIP(r);

        samplebuf[0] = l;
        samplebuf[1] = r;
        samplebuf += 2;
    }
}
