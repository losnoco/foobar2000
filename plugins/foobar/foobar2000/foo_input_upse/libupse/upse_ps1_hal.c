/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_hal.c
 * Purpose: libupse: PS1 HAL implementation
 *
 * Copyright (c) 2007 William Pitcock <nenolod@sacredspiral.co.uk>
 * Portions copyright (c) 1999-2002 Pcsx Team
 * Portions copyright (c) 2004 "Xodnizel"
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include <stdio.h>
#include <string.h>

#include "upse-internal.h"

#define HW_DMA4_MADR (psxHu32(ins, 0x10c0))	// SPU DMA
#define HW_DMA4_BCR  (psxHu32(ins, 0x10c4))
#define HW_DMA4_CHCR (psxHu32(ins, 0x10c8))

#define HW_DMA_PCR   (psxHu32(ins, 0x10f0))
#define HW_DMA_ICR   (psxHu32(ins, 0x10f4))

void upse_ps1_hal_reset(upse_module_instance_t *ins)
{
    memset(ins->psxH, 0, 0x10000);
    psxRcntInit(ins);
}

u8 upse_ps1_hal_read_8(upse_module_instance_t *ins, u32 add)
{
    u8 hard;

    switch (add)
    {
      default:
	  hard = psxHu8(ins, add);
	  return hard;
    }
    return hard;
}

u16 upse_ps1_hal_read_16(upse_module_instance_t *ins, u32 add)
{
    u16 hard;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    switch (add)
    {
      case 0x1f801100:
	  hard = psxRcntRcount(ins, 0);
	  return hard;
      case 0x1f801104:
	  hard = ctrstate->psxCounters[0].mode;
	  return hard;
      case 0x1f801108:
	  hard = ctrstate->psxCounters[0].target;
	  return hard;
      case 0x1f801110:
	  hard = psxRcntRcount(ins, 1);
	  return hard;
      case 0x1f801114:
	  hard = ctrstate->psxCounters[1].mode;
	  return hard;
      case 0x1f801118:
	  hard = ctrstate->psxCounters[1].target;
	  return hard;
      case 0x1f801120:
	  hard = psxRcntRcount(ins, 2);
	  return hard;
      case 0x1f801124:
	  hard = ctrstate->psxCounters[2].mode;
	  return hard;
      case 0x1f801128:
	  hard = ctrstate->psxCounters[2].target;
	  return hard;
      case 0x1f801070:
          hard = psxHu16(ins, 0x1070);
	  return hard;
      case 0x1f801074:
          hard = psxHu16(ins, 0x1074);
	  return hard;
      case 0x1f8010f0:
          hard = psxHu16(ins, 0x10f0);
	  return hard;
      case 0x1f8010f4:
          hard = psxHu16(ins, 0x10f4);
	  return hard;
      default:
	  if (add >= 0x1f801c00 && add < 0x1f801e00)
	  {
	      hard = SPUreadRegister(ins->spu, add);
	  }
	  else
	  {
              _DEBUG("unknown address [0x%x]", add);
	      hard = BFLIP16(psxHu16(ins, add));
	  }
	  return hard;
    }
    return hard;
}

u32 upse_ps1_hal_read_32(upse_module_instance_t *ins, u32 add)
{
    u32 hard;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    switch (add)
    {
      case 0x1f801014:
          hard = 0;
          return hard;
	  // time for rootcounters :)
      case 0x1f801100:
	  hard = psxRcntRcount(ins, 0);
	  return hard;
      case 0x1f801104:
	  hard = ctrstate->psxCounters[0].mode;
	  return hard;
      case 0x1f801108:
	  hard = ctrstate->psxCounters[0].target;
	  return hard;
      case 0x1f801110:
	  hard = psxRcntRcount(ins, 1);
	  return hard;
      case 0x1f801114:
	  hard = ctrstate->psxCounters[1].mode;
	  return hard;
      case 0x1f801118:
	  hard = ctrstate->psxCounters[1].target;
	  return hard;
      case 0x1f801120:
	  hard = psxRcntRcount(ins, 2);
	  return hard;
      case 0x1f801124:
	  hard = ctrstate->psxCounters[2].mode;
	  return hard;
      case 0x1f801128:
	  hard = ctrstate->psxCounters[2].target;
	  return hard;
      case 0x1f801070:
          hard = psxHu32(ins, 0x1070);
	  return hard;
      case 0x1f801074:
          hard = psxHu32(ins, 0x1074);
	  return hard;
      case 0x1f8010f0:
          hard = psxHu32(ins, 0x10f0);
	  return hard;
      case 0x1f8010f4:
          hard = psxHu32(ins, 0x10f4);
	  return hard;
      case 0x1f801814:
          hard = BFLIP32(upse_ps1_gpu_get_status());
          return hard;
      default:
          _DEBUG("unknown address [0x%x]", add);
	  hard = BFLIP32(psxHu32(ins, add));
	  return hard;
    }
    return hard;
}

void upse_ps1_hal_write_8(upse_module_instance_t *ins, u32 add, u8 value)
{
    switch (add)
    {
      default:
	  psxHu8(ins, add) = value;
	  return;
    }
    psxHu8(ins, add) = value;
}

void upse_ps1_hal_write_16(upse_module_instance_t *ins, u32 add, u16 value)
{
    switch (add)
    {
      case 0x1f801100:
	  psxRcntWcount(ins, 0, value);
	  return;
      case 0x1f801104:
	  psxRcntWmode(ins, 0, value);
	  return;
      case 0x1f801108:
	  psxRcntWtarget(ins, 0, value);
	  return;

      case 0x1f801110:
	  psxRcntWcount(ins, 1, value);
	  return;
      case 0x1f801114:
	  psxRcntWmode(ins, 1, value);
	  return;
      case 0x1f801118:
	  psxRcntWtarget(ins, 1, value);
	  return;

      case 0x1f801120:
	  psxRcntWcount(ins, 2, value);
	  return;
      case 0x1f801124:
	  psxRcntWmode(ins, 2, value);
	  return;
      case 0x1f801128:
	  psxRcntWtarget(ins, 2, value);
	  return;

      case 0x1f801070:
          psxHu16(ins, 0x1070) |= BFLIP16(0x200);
          psxHu16(ins, 0x1070) &= BFLIP16((psxHu16(ins, 0x1074) & value));
          return;

      case 0x1f801074:
          psxHu16(ins, 0x1074) = BFLIP16(value);
          ins->cpustate.interrupt |= 0x80000000;
          return;

      default:
	  if (add >= 0x1f801c00 && add < 0x1f801e00)
	  {
	      SPUwriteRegister(ins->spu, add, value);
	      return;
	  }

          _DEBUG("unknown address [0x%x]", add);
	  psxHu16(ins, add) = BFLIP16(value);
	  return;
    }
    psxHu16(ins, add) = BFLIP16(value);
}

#define	DMA_INTERRUPT(ins, n) \
	if (BFLIP32(HW_DMA_ICR) & (1 << (16 + n))) { \
		HW_DMA_ICR|= BFLIP32(1 << (24 + n)); \
		psxHu32(ins, 0x1070) |= BFLIP32(8); \
	}

#define DmaExec(ins, n) { \
	if (BFLIP32(HW_DMA##n##_CHCR) & 0x01000000 && BFLIP32(HW_DMA_PCR) & (8 << (n * 4))) { \
		psxDma##n(ins, BFLIP32(HW_DMA##n##_MADR), BFLIP32(HW_DMA##n##_BCR), BFLIP32(HW_DMA##n##_CHCR)); \
		HW_DMA##n##_CHCR &= BFLIP32(~0x01000000); \
		DMA_INTERRUPT(ins, n); \
	} \
}

void upse_ps1_hal_write_32(upse_module_instance_t *ins, u32 add, u32 value)
{
    switch (add)
    {
      case 0x1f8010c8:
	  HW_DMA4_CHCR = BFLIP32(value);	// DMA4 chcr (SPU DMA)
	  DmaExec(ins, 4);
	  return;
      case 0x1f8010f4:
      {
	  u32 tmp = (~value) & BFLIP32(HW_DMA_ICR);
	  HW_DMA_ICR = BFLIP32(((tmp ^ value) & 0xffffff) ^ tmp);
	  return;
      }

      case 0x1f801100:
	  psxRcntWcount(ins, 0, value & 0xffff);
	  return;
      case 0x1f801104:
	  psxRcntWmode(ins, 0, value);
	  return;
      case 0x1f801108:
	  psxRcntWtarget(ins, 0, value & 0xffff);
	  return;
	  //  HW_DMA_ICR&= (~value)&0xff000000;

      case 0x1f801110:
	  psxRcntWcount(ins, 1, value & 0xffff);
	  return;
      case 0x1f801114:
	  psxRcntWmode(ins, 1, value);
	  return;
      case 0x1f801118:
	  psxRcntWtarget(ins, 1, value & 0xffff);
	  return;

      case 0x1f801120:
	  psxRcntWcount(ins, 2, value & 0xffff);
	  return;
      case 0x1f801124:
	  psxRcntWmode(ins, 2, value);
	  return;
      case 0x1f801128:
	  psxRcntWtarget(ins, 2, value & 0xffff);
	  return;

      case 0x1f8010c0:
          _DEBUG("DMA4 MADR 32bit write %x", value);
          HW_DMA4_MADR = BFLIP32(value);
          return;
      case 0x1f8010c4:
          _DEBUG("DMA4 BCR 32bit write %lx", value);
          HW_DMA4_BCR  = BFLIP32(value);
          return;

      case 0x1f801070:
          psxHu32(ins, 0x1070) |= BFLIP32(0x200);
          psxHu32(ins, 0x1070) &= BFLIP32((psxHu32(ins, 0x1074) & value));
          return;
      case 0x1f801074:
          psxHu32(ins, 0x1074) = BFLIP32(value);
          ins->cpustate.interrupt |= 0x80000000;
          return;

      case 0x1f801814:
          upse_ps1_gpu_set_status(BFLIP32(value));
          return;

      default:
          _DEBUG("unknown address [0x%x]", add);
	  psxHu32(ins, add) = BFLIP32(value);
	  return;
    }
    psxHu32(ins, add) = BFLIP32(value);
}

void SPUirq(upse_module_instance_t *ins)
{
    psxHu32(ins, 0x1070) |= BFLIP32(0x200);
}
