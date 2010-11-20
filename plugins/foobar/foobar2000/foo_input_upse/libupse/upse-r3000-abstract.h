/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-r3000-abstract.h
 * Purpose: libupse: r3K abstract implementation factory headers
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

#ifndef _UPSE__LIBUPSE__UPSE_R3000_ABSTRACT_H__GUARD
#define _UPSE__LIBUPSE__UPSE_R3000_ABSTRACT_H__GUARD

#include <stdio.h>

#include "upse-internal.h"

typedef enum
{
    UPSE_PSX_REV_PS1 = 1,
    UPSE_PSX_REV_PS2_IOP = 2
} upse_psx_revision_t;

#define _i32(x) (s32)x
#define _u32(x) (u32)x

#define _i16(x) (s16)x
#define _u16(x) (u32)x

#define _i8(x) (s8)x
#define _u8(x) (u8)x

/**** R3000A Instruction Macros ****/
#define _PC_       ins->cpustate.pc	// The next PC to be executed

#define _Funct_  ((ins->cpustate.code      ) & 0x3F)	// The funct part of the instruction register
#define _Rd_     ((ins->cpustate.code >> 11) & 0x1F)	// The rd part of the instruction register
#define _Rt_     ((ins->cpustate.code >> 16) & 0x1F)	// The rt part of the instruction register
#define _Rs_     ((ins->cpustate.code >> 21) & 0x1F)	// The rs part of the instruction register
#define _Sa_     ((ins->cpustate.code >>  6) & 0x1F)	// The sa part of the instruction register
#define _Im_     ((u16)ins->cpustate.code)	// The immediate part of the instruction register
#define _Target_ (ins->cpustate.code & 0x03ffffff)	// The target part of the instruction register

#define _Imm_	((s16)ins->cpustate.code)	// sign-extended immediate
#define _ImmU_	(ins->cpustate.code&0xffff)	// zero-extended immediate

#define _rRs_   ins->cpustate.GPR.r[_Rs_]	// Rs register
#define _rRt_   ins->cpustate.GPR.r[_Rt_]	// Rt register
#define _rRd_   ins->cpustate.GPR.r[_Rd_]	// Rd register
#define _rSa_   ins->cpustate.GPR.r[_Sa_]	// Sa register
#define _rFs_   ins->cpustate.CP0.r[_Rd_]	// Fs register

#define _c2dRs_ ins->cpustate.CP2D.r[_Rs_]	// Rs cop2 data register
#define _c2dRt_ ins->cpustate.CP2D.r[_Rt_]	// Rt cop2 data register
#define _c2dRd_ ins->cpustate.CP2D.r[_Rd_]	// Rd cop2 data register
#define _c2dSa_ ins->cpustate.CP2D.r[_Sa_]	// Sa cop2 data register

#define _rHi_   ins->cpustate.GPR.n.hi	// The HI register
#define _rLo_   ins->cpustate.GPR.n.lo	// The LO register

#define _JumpTarget_    ((_Target_ * 4) + (_PC_ & 0xf0000000))	// Calculates the target during a jump instruction
#define _BranchTarget_  ((s16)_Im_ * 4 + _PC_)	// Calculates the target during a branch instruction

#define _SetLink(x)     ins->cpustate.GPR.r[x] = _PC_ + 4;	// Sets the return address in the link register

int psxInit(upse_module_instance_t *ins);
void psxReset(upse_module_instance_t *ins, upse_psx_revision_t rev);
void psxShutdown(upse_module_instance_t *ins);
void psxException(upse_module_instance_t *ins, u32 code, u32 bd);
void psxBranchTest(upse_module_instance_t *ins);
void psxExecuteBios(upse_module_instance_t *ins);

int upse_r3000_cpu_init(upse_module_instance_t *ins);
void upse_r3000_cpu_reset(upse_module_instance_t *ins);
void upse_r3000_cpu_execute(upse_module_instance_t *ins);
int upse_r3000_cpu_execute_render(upse_module_instance_t *ins, s16 **);
void upse_r3000_cpu_execute_block(upse_module_instance_t *ins);
void upse_r3000_cpu_clear(upse_module_instance_t *ins, u32 Addr, u32 Size);
void upse_r3000_cpu_shutdown(upse_module_instance_t *ins);

#endif /* __R3000A_H__ */
