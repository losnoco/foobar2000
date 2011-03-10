/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_counters.c
 * Purpose: libupse: PS1 timekeeping implementation
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

#include <string.h>

#include "upse-internal.h"

static void upse_ps1_counter_update_fast(upse_module_instance_t *ins, u32 index)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[index].sCycle = ins->cpustate.cycle;
    if (((!(ctrstate->psxCounters[index].mode & 1)) || (index != 2)) && ctrstate->psxCounters[index].mode & 0x30)
    {
	if (ctrstate->psxCounters[index].mode & 0x10)
	{			// Interrupt on target
	    ctrstate->psxCounters[index].Cycle = ((ctrstate->psxCounters[index].target - ctrstate->psxCounters[index].count) * ctrstate->psxCounters[index].rate) / BIAS;
	}
	else
	{			// Interrupt on 0xffff
	    ctrstate->psxCounters[index].Cycle = ((0xffff - ctrstate->psxCounters[index].count) * ctrstate->psxCounters[index].rate) / BIAS;
	}
    }
    else
	ctrstate->psxCounters[index].Cycle = 0xffffffff;
}

static void upse_ps1_counter_reset(upse_module_instance_t *ins, u32 index)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[index].count = 0;
    upse_ps1_counter_update_fast(ins, index);

    psxHu32(ins, 0x1070) |= BFLIP32(ctrstate->psxCounters[index].interrupt);
    if (!(ctrstate->psxCounters[index].mode & 0x40))
    {				// Only 1 interrupt
	ctrstate->psxCounters[index].Cycle = 0xffffffff;
    }
}

static void upse_ps1_counter_set(upse_module_instance_t *ins)
{
    int i;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxNextCounter = 0x7fffffff;
    ctrstate->psxNextsCounter = ins->cpustate.cycle;

    for (i = 0; i < 4; i++)
    {
	s32 count;

	if (ctrstate->psxCounters[i].Cycle == 0xffffffff)
	    continue;

	count = ctrstate->psxCounters[i].Cycle - (ins->cpustate.cycle - ctrstate->psxCounters[i].sCycle);

	if (count < 0)
	{
	    ctrstate->psxNextCounter = 0;
	    break;
	}

	if (count < (s32) ctrstate->psxNextCounter)
	{
	    ctrstate->psxNextCounter = count;
	}
    }
}

void upse_ps1_counter_init(upse_module_instance_t *ins)
{
    upse_psx_counter_state_t *ctrstate;

    ctrstate = calloc(sizeof(upse_psx_counter_state_t), 1);

    ctrstate->psxCounters[0].rate = 1;
    ctrstate->psxCounters[0].interrupt = 0x10;
    ctrstate->psxCounters[1].rate = 1;
    ctrstate->psxCounters[1].interrupt = 0x20;
    ctrstate->psxCounters[2].rate = 1;
    ctrstate->psxCounters[2].interrupt = 64;

    ctrstate->psxCounters[3].interrupt = 1;
    ctrstate->psxCounters[3].mode = 0x58;	// The VSync counter mode
    ctrstate->psxCounters[3].target = 1;

    ins->ctrstate = ctrstate;

    upse_ps1_set_vsync(ins, 60);

    upse_ps1_counter_update_fast(ins, 0);
    upse_ps1_counter_update_fast(ins, 1);
    upse_ps1_counter_update_fast(ins, 2);
    upse_ps1_counter_update_fast(ins, 3);
    upse_ps1_counter_set(ins);
    ctrstate->last = 0;
}

void upse_ps1_counter_sleep(upse_module_instance_t *ins)
{
    s32 min, x, lmin;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    lmin = 0x7FFFFFFF;

    for (x = 0; x < 4; x++)
    {
	if (ctrstate->psxCounters[x].Cycle != 0xffffffff)
        {
            min = ctrstate->psxCounters[x].Cycle;
            min -= (ins->cpustate.cycle - ctrstate->psxCounters[x].sCycle);
            if (min < lmin)
                lmin = min;
        }
    }

    if (lmin > 0)
        ins->cpustate.cycle += lmin;
}

int upse_ps1_counter_run(upse_module_instance_t *ins)
{
    u32 cycles;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    if (ins->cpustate.cycle < ctrstate->last)
    {
	cycles = 0xFFFFFFFF - ctrstate->last;
	cycles += ins->cpustate.cycle;
    }
    else
	cycles = ins->cpustate.cycle - ctrstate->last;

    if (cycles >= 16)
    {
	if (!upse_ps1_spu_render(ins->spu, cycles))
	    return (0);
	ctrstate->last = ins->cpustate.cycle;
    }
    return (1);
}

void upse_ps1_set_vsync(upse_module_instance_t *ins, int refresh)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[3].rate = (PSXCLK / refresh);

	ctrstate->lines = (refresh == 60) ? 262 : 312;
	ctrstate->lines_visible = (refresh == 60) ? 224 : 240;
	ctrstate->refresh_rate = refresh;
}

void upse_ps1_counter_update(upse_module_instance_t *ins)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    if ((ins->cpustate.cycle - ctrstate->psxCounters[3].sCycle) >= ctrstate->psxCounters[3].Cycle)
    {
	upse_ps1_counter_update_fast(ins, 3);
	psxHu32(ins, 0x1070) |= BFLIP32(1);
    }
    if ((ins->cpustate.cycle - ctrstate->psxCounters[0].sCycle) >= ctrstate->psxCounters[0].Cycle)
    {
	upse_ps1_counter_reset(ins, 0);
    }

    if ((ins->cpustate.cycle - ctrstate->psxCounters[1].sCycle) >= ctrstate->psxCounters[1].Cycle)
    {
	upse_ps1_counter_reset(ins, 1);
    }

    if ((ins->cpustate.cycle - ctrstate->psxCounters[2].sCycle) >= ctrstate->psxCounters[2].Cycle)
    {
	upse_ps1_counter_reset(ins, 2);
    }

    upse_ps1_counter_set(ins);
}

void upse_ps1_counter_set_count(upse_module_instance_t *ins, u32 index, u32 value)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[index].count = value;
    upse_ps1_counter_update_fast(ins, index);
    upse_ps1_counter_set(ins);
}

void upse_ps1_counter_set_mode(upse_module_instance_t *ins, u32 index, u32 value)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[index].mode = value;
    ctrstate->psxCounters[index].count = 0;

    if (index == 0)
    {
	switch (value & 0x300)
	{
	  case 0x100:
	      ctrstate->psxCounters[index].rate = ((ctrstate->psxCounters[3].rate /** BIAS*/ ) / 386) / ctrstate->lines;	// seems ok
	      break;
	  default:
	      ctrstate->psxCounters[index].rate = 1;
	}
    }
    else if (index == 1)
    {
	switch (value & 0x300)
	{
	  case 0x100:
	      ctrstate->psxCounters[index].rate = (ctrstate->psxCounters[3].rate /** BIAS*/ ) / ctrstate->lines;	// seems ok
	      //ctrstate->psxCounters[index].rate = (PSXCLK / 60)/262; //(ctrstate->psxCounters[3].rate*16/ctrstate->lines);
	      //printf("%d\n",ctrstate->psxCounters[index].rate);
	      break;
	  default:
	      ctrstate->psxCounters[index].rate = 1;
	}
    }
    else if (index == 2)
    {
	switch (value & 0x300)
	{
	  case 0x200:
	      ctrstate->psxCounters[index].rate = 8;	// 1/8 speed
	      break;
	  default:
	      ctrstate->psxCounters[index].rate = 1;	// normal speed
	}
    }

    // Need to set a rate and target
    upse_ps1_counter_update_fast(ins, index);
    upse_ps1_counter_set(ins);
}

void upse_ps1_counter_set_target(upse_module_instance_t *ins, u32 index, u32 value)
{
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    ctrstate->psxCounters[index].target = value;
    upse_ps1_counter_update_fast(ins, index);
    upse_ps1_counter_set(ins);
}

u32 upse_ps1_counter_get_count(upse_module_instance_t *ins, u32 index)
{
    u32 ret;
    upse_psx_counter_state_t *ctrstate = ins->ctrstate;

    if (ctrstate->psxCounters[index].mode & 0x08)
    {				// Wrap at target
	ret = (ctrstate->psxCounters[index].count + BIAS * ((ins->cpustate.cycle - ctrstate->psxCounters[index].sCycle) / ctrstate->psxCounters[index].rate)) & 0xffff;
    }
    else
    {				// Wrap at 0xffff
	ret = (ctrstate->psxCounters[index].count + BIAS * (ins->cpustate.cycle / ctrstate->psxCounters[index].rate)) & 0xffff;
    }

    return ret;
}
