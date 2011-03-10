/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-ps1-counters.h
 * Purpose: libupse: PS1 timekeeping implementation headers
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

#ifndef __PSXCOUNTERS_H__
#define __PSXCOUNTERS_H__

typedef struct
{
    u32 count, mode, target;
    u32 sCycle, Cycle, rate, interrupt;
} upse_psx_counter_t;

typedef struct
{
    upse_psx_counter_t psxCounters[5];
    u32 psxNextCounter, psxNextsCounter, last;
	u32 lines, lines_visible, refresh_rate;
} upse_psx_counter_state_t;

void upse_ps1_counter_init(upse_module_instance_t *ins);
void upse_ps1_counter_update(upse_module_instance_t *ins);
void upse_ps1_counter_set_count(upse_module_instance_t *ins, u32 index, u32 value);
void upse_ps1_counter_set_mode(upse_module_instance_t *ins, u32 index, u32 value);
void upse_ps1_counter_set_target(upse_module_instance_t *ins, u32 index, u32 value);
u32 upse_ps1_counter_get_count(upse_module_instance_t *ins, u32 index);

void upse_ps1_set_vsync(upse_module_instance_t *ins, int refresh);

int upse_ps1_counter_run(upse_module_instance_t *ins);
void upse_ps1_counter_sleep(upse_module_instance_t *ins);

#endif /* __PSXCOUNTERS_H__ */
