/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_r3000_bytecode_evaluator.c
 * Purpose: libupse: r3K machine bytecode evaluator
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

#include <stdlib.h>

#include "upse-internal.h"

extern void upse_ps2_iop_call(u32 callnum);

// These macros are used to assemble the repassembler functions

//        if(!PSXM(ins->cpustate.pc)) puts("Whoops");   
//        Fix this...

//        printf("%08x ", ins->cpustate.pc);

#define execI(ins) { \
	ins->cpustate.code = BFLIP32(PSXMu32(ins, ins->cpustate.pc)); \
	if (0) { _DEBUG("current PC: %x Cycle: %x Code: %d", ins->cpustate.pc, ins->cpustate.cycle, ins->cpustate.code >> 26); } \
	ins->cpustate.pc+= 4; ins->cpustate.cycle++; \
	psxBSC[ins->cpustate.code >> 26](ins); \
}

// Subsets
static void (*psxBSC[64]) (upse_module_instance_t *ins);
static void (*psxSPC[64]) (upse_module_instance_t *ins);
static void (*psxREG[32]) (upse_module_instance_t *ins);
static void (*psxCP0[32]) (upse_module_instance_t *ins);

static void delayRead(upse_module_instance_t *ins, int reg, u32 bpc)
{
    u32 rold, rnew;

    _DEBUG("delayRead, PC: %lx", ins->cpustate.pc);

    rold = ins->cpustate.GPR.r[reg];
    psxBSC[ins->cpustate.code >> 26] (ins);	// branch delay load
    rnew = ins->cpustate.GPR.r[reg];

    ins->cpustate.pc = bpc;

    psxBranchTest(ins);

    ins->cpustate.GPR.r[reg] = rold;
    execI(ins);
    ins->cpustate.GPR.r[reg] = rnew;

    ins->cpustate.branch = 0;
}

static void delayWrite(upse_module_instance_t *ins, int reg, u32 bpc)
{
    _DEBUG("delayWrite, PC: %lx", ins->cpustate.pc);

    psxBSC[ins->cpustate.code >> 26] (ins);

    ins->cpustate.branch = 0;
    ins->cpustate.pc = bpc;

    psxBranchTest(ins);
}

static void delayReadWrite(upse_module_instance_t *ins, int reg, u32 bpc)
{
    _DEBUG("delayReadWrite, PC: %lx", ins->cpustate.pc);

    // the branch delay load is skipped

    ins->cpustate.branch = 0;
    ins->cpustate.pc = bpc;

    psxBranchTest(ins);
}

// this defines shall be used with the tmp 
// of the next func (instead of _Funct_...)
#define _tFunct_  ((tmp      ) & 0x3F)	// The funct part of the instruction register
#define _tRd_     ((tmp >> 11) & 0x1F)	// The rd part of the instruction register
#define _tRt_     ((tmp >> 16) & 0x1F)	// The rt part of the instruction register
#define _tRs_     ((tmp >> 21) & 0x1F)	// The rs part of the instruction register
#define _tSa_     ((tmp >>  6) & 0x1F)	// The sa part of the instruction register

static void psxDelayTest(upse_module_instance_t *ins, u32 reg, u32 bpc)
{
    u32 tmp;

    tmp = BFLIP32(PSXMu32(ins, bpc));
    ins->cpustate.branch = 1;

    switch (tmp >> 26)
    {
      case 0x00:		// SPECIAL
	  switch (_tFunct_)
	  {
	    case 0x00:		// SLL
		if (!tmp)
		    break;	// NOP
	    case 0x02:
	    case 0x03:		// SRL/SRA
		if (_tRd_ == reg && _tRt_ == reg)
		{
		    delayReadWrite(ins, reg, bpc);
		    return;
		}
		else if (_tRt_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		else if (_tRd_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;

	    case 0x08:		// JR
		if (_tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;
	    case 0x09:		// JALR
		if (_tRd_ == reg && _tRs_ == reg)
		{
		    delayReadWrite(ins, reg, bpc);
		    return;
		}
		else if (_tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		else if (_tRd_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;

		// SYSCALL/BREAK just a break;

	    case 0x20:
	    case 0x21:
	    case 0x22:
	    case 0x23:
	    case 0x24:
	    case 0x25:
	    case 0x26:
	    case 0x27:
	    case 0x2a:
	    case 0x2b:		// ADD/ADDU...
	    case 0x04:
	    case 0x06:
	    case 0x07:		// SLLV...
		if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg))
		{
		    delayReadWrite(ins, reg, bpc);
		    return;
		}
		else if (_tRt_ == reg || _tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		else if (_tRd_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;

	    case 0x10:
	    case 0x12:		// MFHI/MFLO
		if (_tRd_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;
	    case 0x11:
	    case 0x13:		// MTHI/MTLO
		if (_tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;

	    case 0x18:
	    case 0x19:
	    case 0x1a:
	    case 0x1b:		// MULT/DIV...
		if (_tRt_ == reg || _tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;
	  }
	  break;

      case 0x01:		// REGIMM
	  switch (_tRt_)
	  {
	    case 0x00:
	    case 0x02:
	    case 0x10:
	    case 0x12:		// BLTZ/BGEZ...
		if (_tRs_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;
	  }
	  break;

	  // J would be just a break;
      case 0x03:		// JAL
	  if (31 == reg)
	  {
	      delayWrite(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x04:
      case 0x05:		// BEQ/BNE
	  if (_tRs_ == reg || _tRt_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x06:
      case 0x07:		// BLEZ/BGTZ
	  if (_tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0c:
      case 0x0d:
      case 0x0e:		// ADDI/ADDIU...
	  if (_tRt_ == reg && _tRs_ == reg)
	  {
	      delayReadWrite(ins, reg, bpc);
	      return;
	  }
	  else if (_tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  else if (_tRt_ == reg)
	  {
	      delayWrite(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x0f:		// LUI
	  if (_tRt_ == reg)
	  {
	      delayWrite(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x10:		// COP0
	  switch (_tFunct_)
	  {
	    case 0x00:		// MFC0
		if (_tRt_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;
	    case 0x02:		// CFC0
		if (_tRt_ == reg)
		{
		    delayWrite(ins, reg, bpc);
		    return;
		}
		break;
	    case 0x04:		// MTC0
		if (_tRt_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;
	    case 0x06:		// CTC0
		if (_tRt_ == reg)
		{
		    delayRead(ins, reg, bpc);
		    return;
		}
		break;
		// RFE just a break;
	  }
	  break;

      case 0x22:
      case 0x26:		// LWL/LWR
	  if (_tRt_ == reg)
	  {
	      delayReadWrite(ins, reg, bpc);
	      return;
	  }
	  else if (_tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x20:
      case 0x21:
      case 0x23:
      case 0x24:
      case 0x25:		// LB/LH/LW/LBU/LHU
	  if (_tRt_ == reg && _tRs_ == reg)
	  {
	      delayReadWrite(ins, reg, bpc);
	      return;
	  }
	  else if (_tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  else if (_tRt_ == reg)
	  {
	      delayWrite(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x28:
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2e:		// SB/SH/SWL/SW/SWR
	  if (_tRt_ == reg || _tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  break;

      case 0x32:
      case 0x3a:		// LWC2/SWC2
	  if (_tRs_ == reg)
	  {
	      delayRead(ins, reg, bpc);
	      return;
	  }
	  break;
    }
    psxBSC[ins->cpustate.code >> 26] (ins);

    ins->cpustate.branch = 0;
    ins->cpustate.pc = bpc;

    psxBranchTest(ins);
}

static void psxNULL(upse_module_instance_t *ins);

static INLINE void doBranch(upse_module_instance_t *ins, u32 tar)
{
    u32 tmp;

    ins->cpustate.branch2 = ins->cpustate.branch = 1;
    ins->cpustate.branchPC = tar;

    ins->cpustate.code = BFLIP32(PSXMu32(ins, ins->cpustate.pc));

    ins->cpustate.pc += 4;
    ins->cpustate.cycle++;

    // check for load delay
    tmp = ins->cpustate.code >> 26;
    switch (tmp)
    {
      case 0x10:		// COP0
	  switch (_Rs_)
	  {
	    case 0x00:		// MFC0
	    case 0x02:		// CFC0
		psxDelayTest(ins, _Rt_, ins->cpustate.branchPC);
		return;
	  }
	  break;
      case 0x32:		// LWC2
	  psxDelayTest(ins, _Rt_, ins->cpustate.branchPC);
	  return;
      default:
	  if (tmp >= 0x20 && tmp <= 0x26)
	  {			// LB/LH/LWL/LW/LBU/LHU/LWR
	      psxDelayTest(ins, _Rt_, ins->cpustate.branchPC);
	      return;
	  }
	  break;
    }

    psxBSC[ins->cpustate.code >> 26] (ins);

    if ((ins->cpustate.pc - 8) == ins->cpustate.branchPC && !(ins->cpustate.code >> 26))
        CounterDeadLoopSkip(ins);

    ins->cpustate.branch = 0;
    ins->cpustate.pc = ins->cpustate.branchPC;

    psxBranchTest(ins);
}

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
static void psxADDI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _u32(_rRs_) + _Imm_;
}				// Rt = Rs + Im         (Exception on Integer Overflow)
static void psxADDIU(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;

    _rRt_ = _u32(_rRs_) + _Imm_;
}				// Rt = Rs + Im
static void psxANDI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _u32(_rRs_) & _ImmU_;
}				// Rt = Rs And Im
static void psxORI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _u32(_rRs_) | _ImmU_;
}				// Rt = Rs Or  Im
static void psxXORI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _u32(_rRs_) ^ _ImmU_;
}				// Rt = Rs Xor Im
static void psxSLTI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _i32(_rRs_) < _Imm_;
}				// Rt = Rs < Im         (Signed)
static void psxSLTIU(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = _u32(_rRs_) < ((u32) _ImmU_);
}				// Rt = Rs < Im         (Unsigned)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
static void psxADD(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}				// Rd = Rs + Rt         (Exception on Integer Overflow)
static void psxADDU(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}				// Rd = Rs + Rt
static void psxSUB(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}				// Rd = Rs - Rt         (Exception on Integer Overflow)
static void psxSUBU(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}				// Rd = Rs - Rt
static void psxAND(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) & _u32(_rRt_);
}				// Rd = Rs And Rt
static void psxOR(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) | _u32(_rRt_);
}				// Rd = Rs Or  Rt
static void psxXOR(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) ^ _u32(_rRt_);
}				// Rd = Rs Xor Rt
static void psxNOR(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = ~(_u32(_rRs_) | _u32(_rRt_));
}				// Rd = Rs Nor Rt
static void psxSLT(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _i32(_rRs_) < _i32(_rRt_);
}				// Rd = Rs < Rt         (Signed)
static void psxSLTU(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRs_) < _u32(_rRt_);
}				// Rd = Rs < Rt         (Unsigned)

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/
static void psxDIV(upse_module_instance_t *ins)
{
    if (_i32(_rRt_) != 0)
    {
	_rLo_ = _i32(_rRs_) / _i32(_rRt_);
	_rHi_ = _i32(_rRs_) % _i32(_rRt_);
    }
}

static void psxDIVU(upse_module_instance_t *ins)
{
    if (_rRt_ != 0)
    {
	_rLo_ = _rRs_ / _rRt_;
	_rHi_ = _rRs_ % _rRt_;
    }
}

static void psxMULT(upse_module_instance_t *ins)
{
    u64 res = (s64) ((s64) _i32(_rRs_) * (s64) _i32(_rRt_));

    ins->cpustate.GPR.n.lo = (u32) (res & 0xffffffff);
    ins->cpustate.GPR.n.hi = (u32) ((res >> 32) & 0xffffffff);
}

static void psxMULTU(upse_module_instance_t *ins)
{
    u64 res = (u64) ((u64) _u32(_rRs_) * (u64) _u32(_rRt_));

    ins->cpustate.GPR.n.lo = (u32) (res & 0xffffffff);
    ins->cpustate.GPR.n.hi = (u32) ((res >> 32) & 0xffffffff);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
#define RepZBranchi32(ins, op)      if(_i32(_rRs_) op 0) doBranch(ins, _BranchTarget_);
#define RepZBranchLinki32(ins, op)  if(_i32(_rRs_) op 0) { _SetLink(31); doBranch(ins, _BranchTarget_); }

static void psxBGEZ(upse_module_instance_t *ins)
{
RepZBranchi32(ins, >=)}		// Branch if Rs >= 0
static void psxBGEZAL(upse_module_instance_t *ins)
{
RepZBranchLinki32(ins, >=)}		// Branch if Rs >= 0 and link
static void psxBGTZ(upse_module_instance_t *ins)
{
RepZBranchi32(ins, >)}		// Branch if Rs >  0
static void psxBLEZ(upse_module_instance_t *ins)
{
RepZBranchi32(ins, <=)}		// Branch if Rs <= 0
static void psxBLTZ(upse_module_instance_t *ins)
{
RepZBranchi32(ins, <)}		// Branch if Rs <  0
static void psxBLTZAL(upse_module_instance_t *ins)
{
RepZBranchLinki32(ins, <)}		// Branch if Rs <  0 and link

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
static void psxSLL(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRt_) << _Sa_;
}				// Rd = Rt << sa
static void psxSRA(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _i32(_rRt_) >> _Sa_;
}				// Rd = Rt >> sa (arithmetic)
static void psxSRL(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRt_) >> _Sa_;
}				// Rd = Rt >> sa (logical)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
static void psxSLLV(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRt_) << _u32(_rRs_);
}				// Rd = Rt << rs
static void psxSRAV(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _i32(_rRt_) >> _u32(_rRs_);
}				// Rd = Rt >> rs (arithmetic)
static void psxSRLV(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _u32(_rRt_) >> _u32(_rRs_);
}				// Rd = Rt >> rs (logical)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
static void psxLUI(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = ins->cpustate.code << 16;
}				// Upper halfword of Rt = Im

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
static void psxMFHI(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _rHi_;
}				// Rd = Hi
static void psxMFLO(upse_module_instance_t *ins)
{
    if (!_Rd_)
	return;
    _rRd_ = _rLo_;
}				// Rd = Lo

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/
static void psxMTHI(upse_module_instance_t *ins)
{
    _rHi_ = _rRs_;
}				// Hi = Rs
static void psxMTLO(upse_module_instance_t *ins)
{
    _rLo_ = _rRs_;
}				// Lo = Rs

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/
static void psxBREAK(upse_module_instance_t *ins)
{
    // Break exception - psx rom doens't handles this
}

static void psxSYSCALL(upse_module_instance_t *ins)
{
    ins->cpustate.pc -= 4;
    psxException(ins, 0x20, ins->cpustate.branch);
}

static void psxRFE(upse_module_instance_t *ins)
{
    ins->cpustate.CP0.n.Status = (ins->cpustate.CP0.n.Status & 0xfffffff0) | ((ins->cpustate.CP0.n.Status & 0x3c) >> 2);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#define RepBranchi32(ins, op)      if(_i32(_rRs_) op _i32(_rRt_)) { doBranch(ins, _BranchTarget_); }

static void psxBEQ(upse_module_instance_t *ins)
{
RepBranchi32(ins, ==)}		// Branch if Rs == Rt
static void psxBNE(upse_module_instance_t *ins)
{
RepBranchi32(ins, !=)}		// Branch if Rs != Rt

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
static void psxJ(upse_module_instance_t *ins)
{
#if 0
    if (((ins->cpustate.code >> 16) & 31) == 0) {
        _DEBUG("IOP call? PC:%lx CODE:%lx", ins->cpustate.pc, ins->cpustate.code);
        upse_ps2_iop_call(ins->cpustate.code & 0xffff);
        return;
    }
#endif

    doBranch(ins, _JumpTarget_);
}

static void psxJAL(upse_module_instance_t *ins)
{
    _SetLink(31);
    doBranch(ins, _JumpTarget_);
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
static void psxJR(upse_module_instance_t *ins)
{
    doBranch(ins, _u32(_rRs_));
}
static void psxJALR(upse_module_instance_t *ins)
{
    if (_Rd_)
    {
	_SetLink(_Rd_);
    }
    doBranch(ins, _u32(_rRs_));
}

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _oB_ (_u32(_rRs_) + _Imm_)

static void psxLB(upse_module_instance_t *ins)
{
    if (_Rt_)
    {
	_rRt_ = (s8) upse_ps1_memory_read_8(ins, _oB_);
    }
    else
    {
	upse_ps1_memory_read_8(ins, _oB_);
    }
}

static void psxLBU(upse_module_instance_t *ins)
{
    if (_Rt_)
    {
	_rRt_ = upse_ps1_memory_read_8(ins, _oB_);
    }
    else
    {
	upse_ps1_memory_read_8(ins, _oB_);
    }
}

static void psxLH(upse_module_instance_t *ins)
{
    if (_Rt_)
    {
	_rRt_ = (s16) upse_ps1_memory_read_16(ins, _oB_);
    }
    else
    {
	upse_ps1_memory_read_16(ins, _oB_);
    }
}

static void psxLHU(upse_module_instance_t *ins)
{
    if (_Rt_)
    {
	_rRt_ = upse_ps1_memory_read_16(ins, _oB_);
    }
    else
    {
	upse_ps1_memory_read_16(ins, _oB_);
    }
}

static void psxLW(upse_module_instance_t *ins)
{
    if (_Rt_)
    {
	_rRt_ = upse_ps1_memory_read_32(ins, _oB_);
    }
    else
    {
	upse_ps1_memory_read_32(ins, _oB_);
    }
}

static u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
static u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };

static void psxLWL(upse_module_instance_t *ins)
{
    u32 addr = _oB_;
    u32 shift = addr & 3;
    u32 mem = upse_ps1_memory_read_32(ins, addr & ~3);

    if (!_Rt_)
	return;
    _rRt_ = (_u32(_rRt_) & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]);

    /*
       Mem = 1234.  Reg = abcd

       0   4bcd   (mem << 24) | (reg & 0x00ffffff)
       1   34cd   (mem << 16) | (reg & 0x0000ffff)
       2   234d   (mem <<  8) | (reg & 0x000000ff)
       3   1234   (mem      ) | (reg & 0x00000000)
     */
}

static u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
static u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };

static void psxLWR(upse_module_instance_t *ins)
{
    u32 addr = _oB_;
    u32 shift = addr & 3;
    u32 mem = upse_ps1_memory_read_32(ins, addr & ~3);

    if (!_Rt_)
	return;
    _rRt_ = (_u32(_rRt_) & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]);

    /*
       Mem = 1234.  Reg = abcd

       0   1234   (mem      ) | (reg & 0x00000000)
       1   a123   (mem >>  8) | (reg & 0xff000000)
       2   ab12   (mem >> 16) | (reg & 0xffff0000)
       3   abc1   (mem >> 24) | (reg & 0xffffff00)
     */
}

static void psxSB(upse_module_instance_t *ins)
{
    upse_ps1_memory_write_8(ins, _oB_, _u8(_rRt_));
}
static void psxSH(upse_module_instance_t *ins)
{
    upse_ps1_memory_write_16(ins, _oB_, _u16(_rRt_));
}
static void psxSW(upse_module_instance_t *ins)
{
    upse_ps1_memory_write_32(ins, _oB_, _u32(_rRt_));
}

static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0 };
static const u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };

static void psxSWL(upse_module_instance_t *ins)
{
    u32 addr = _oB_;
    u32 shift = addr & 3;
    u32 mem = upse_ps1_memory_read_32(ins, addr & ~3);

    upse_ps1_memory_write_32(ins, addr & ~3, (_u32(_rRt_) >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
    /*
       Mem = 1234.  Reg = abcd

       0   123a   (reg >> 24) | (mem & 0xffffff00)
       1   12ab   (reg >> 16) | (mem & 0xffff0000)
       2   1abc   (reg >>  8) | (mem & 0xff000000)
       3   abcd   (reg      ) | (mem & 0x00000000)
     */
}

static const u32 SWR_MASK[4] = { 0, 0xff, 0xffff, 0xffffff };
static const u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };

static void psxSWR(upse_module_instance_t *ins)
{
    u32 addr = _oB_;
    u32 shift = addr & 3;
    u32 mem = upse_ps1_memory_read_32(ins, addr & ~3);

    upse_ps1_memory_write_32(ins, addr & ~3, (_u32(_rRt_) << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));

    /*
       Mem = 1234.  Reg = abcd

       0   abcd   (reg      ) | (mem & 0x00000000)
       1   bcd4   (reg <<  8) | (mem & 0x000000ff)
       2   cd34   (reg << 16) | (mem & 0x0000ffff)
       3   d234   (reg << 24) | (mem & 0x00ffffff)
     */
}

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, fs                                     *
*********************************************************/
static void psxMFC0(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = (int) _rFs_;
}
static void psxCFC0(upse_module_instance_t *ins)
{
    if (!_Rt_)
	return;
    _rRt_ = (int) _rFs_;
}

static INLINE void MTC0(upse_module_instance_t *ins, int reg, u32 val)
{
    switch (reg)
    {
      case 13:			// Cause
	  ins->cpustate.CP0.n.Cause = val & ~(0xfc00);

	  // the next code is untested, if u know please
	  // tell me if it works ok or not (linuzappz)
	  if (ins->cpustate.CP0.n.Cause & ins->cpustate.CP0.n.Status & 0x0300 && ins->cpustate.CP0.n.Status & 0x1)
	  {
	      psxException(ins, ins->cpustate.CP0.n.Cause, 0);
	  }
	  break;

      default:
	  ins->cpustate.CP0.r[reg] = val;
	  break;
    }
}

static void psxMTC0(upse_module_instance_t *ins)
{
    MTC0(ins, _Rd_, _u32(_rRt_));
}
static void psxCTC0(upse_module_instance_t *ins)
{
    MTC0(ins, _Rd_, _u32(_rRt_));
}

/*********************************************************
* Unknow instruction (would generate an exception)       *
* Format:  ?                                             *
*********************************************************/
static void psxNULL(upse_module_instance_t *ins)
{
    _DEBUG("unimplemented opcode %x", ins->cpustate.code >> 26);
}

static void psxSPECIAL(upse_module_instance_t *ins)
{
    psxSPC[_Funct_] (ins);
}

static void psxREGIMM(upse_module_instance_t *ins)
{
    psxREG[_Rt_] (ins);
}

static void psxCOP0(upse_module_instance_t *ins)
{
    psxCP0[_Rs_] (ins);
}

static void psxHLE(upse_module_instance_t *ins)
{
    _DEBUG("HLE - PC: 0x%lx", ins->cpustate.pc & 0xff);
    psxHLEt[ins->cpustate.code & 0xff] (ins);
}

static void (*psxBSC[64]) (upse_module_instance_t *ins) =
{
psxSPECIAL, psxREGIMM, psxJ, psxJAL, psxBEQ, psxBNE, psxBLEZ, psxBGTZ,
	psxADDI, psxADDIU, psxSLTI, psxSLTIU, psxANDI, psxORI, psxXORI, psxLUI,
	psxCOP0, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxLB, psxLH, psxLWL, psxLW, psxLBU, psxLHU, psxLWR, psxNULL,
	psxSB, psxSH, psxSWL, psxSW, psxNULL, psxNULL, psxSWR, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxHLE, psxNULL, psxNULL, psxNULL, psxNULL};


static void (*psxSPC[64]) (upse_module_instance_t *ins) =
{
psxSLL, psxNULL, psxSRL, psxSRA, psxSLLV, psxNULL, psxSRLV, psxSRAV,
	psxJR, psxJALR, psxNULL, psxNULL, psxSYSCALL, psxBREAK, psxNULL, psxNULL,
	psxMFHI, psxMTHI, psxMFLO, psxMTLO, psxNULL, psxNULL, psxNULL, psxNULL,
	psxMULT, psxMULTU, psxDIV, psxDIVU, psxNULL, psxNULL, psxNULL, psxNULL,
	psxADD, psxADDU, psxSUB, psxSUBU, psxAND, psxOR, psxXOR, psxNOR,
	psxNULL, psxNULL, psxSLT, psxSLTU, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL};

static void (*psxREG[32]) (upse_module_instance_t *ins) =
{
psxBLTZ, psxBGEZ, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxBLTZAL, psxBGEZAL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL};

static void (*psxCP0[32]) (upse_module_instance_t *ins) =
{
psxMFC0, psxNULL, psxCFC0, psxNULL, psxMTC0, psxNULL, psxCTC0, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxRFE, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL};


///////////////////////////////////////////

int upse_r3000_cpu_init(upse_module_instance_t *ins)
{
    return 0;
}

void upse_r3000_cpu_reset(upse_module_instance_t *ins)
{
    ins->cpustate.branch = ins->cpustate.branch2 = 0;
}

void upse_r3000_cpu_execute(upse_module_instance_t *ins)
{
    for (;;)
    {
	if (!CounterSPURun(ins))
	{
	    psxShutdown(ins);
	    return;
	}
	upse_ps1_spu_finalize(ins->spu);
	execI(ins);
    }
}

int upse_r3000_cpu_execute_render(upse_module_instance_t *ins, s16 **s)
{
    for (;;)
    {
        int r;

        if (!CounterSPURun(ins))
        {
            psxShutdown(ins);
            return 0;
        }

        r = upse_ps1_spu_finalize_count(ins->spu, s);
        if (r && *s)
            return r;

        execI(ins);
    }
}

void upse_r3000_cpu_execute_block(upse_module_instance_t *ins)
{
    ins->cpustate.branch2 = 0;
    while (!ins->cpustate.branch2)
	execI(ins);
}

void upse_r3000_cpu_clear(upse_module_instance_t *ins, u32 Addr, u32 Size)
{
}

void upse_r3000_cpu_shutdown(upse_module_instance_t *ins)
{
}
