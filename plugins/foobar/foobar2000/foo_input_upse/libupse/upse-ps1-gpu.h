/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-ps1-gpu.h
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

#ifndef __UPSE__LIBUPSE__UPSE_PS1_GPU_H__GUARD
#define __UPSE__LIBUPSE__UPSE_PS1_GPU_H__GUARD

extern void upse_ps1_gpu_set_status(u32 cmd);
extern u32  upse_ps1_gpu_get_status(void);

#endif
