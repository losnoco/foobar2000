/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_spu_dma_handler.c
 * Purpose: libupse: PS1 SPU DMA handling engine
 *
 * Copyright (c) 2007 William Pitcock <nenolod@sacredspiral.co.uk>
 * Portions copyright (c) 2002 Pete Bernert <BlackDove@addcom.de>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#define _IN_DMA

#include "upse-types.h"
#include "upse-internal.h"
#include "upse-ps1-spu-base.h"
#include "upse-spu-internal.h"

#include "upse.h"
#include "upse-ps1-memory-manager.h"

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

void SPUreadDMAMem(upse_spu_state_t *spu, u32 usPSXMem, int iSize)
{
    int i;

    for (i = 0; i < iSize; i++)
    {
	*(u16 *) PSXM(spu->ins, usPSXMem) = spu->spuMem[spu->spuAddr >> 1];	// spu addr got by writeregister
	usPSXMem += 2;
	spu->spuAddr += 2;		// inc spu addr
	if (spu->spuAddr > 0x7ffff)
	    spu->spuAddr = 0;	// wrap
    }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////

void SPUwriteDMAMem(upse_spu_state_t *spu, u32 usPSXMem, int iSize)
{
    int i;

    for (i = 0; i < iSize; i++)
    {
	spu->spuMem[spu->spuAddr >> 1] = *(u16 *) PSXM(spu->ins, usPSXMem);
	usPSXMem += 2;		// spu addr got by writeregister
	spu->spuAddr += 2;		// inc spu addr
	if (spu->spuAddr > 0x7ffff)
	    spu->spuAddr = 0;	// wrap
    }
}

////////////////////////////////////////////////////////////////////////
