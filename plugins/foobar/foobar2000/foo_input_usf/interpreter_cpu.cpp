/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <float.h>
#include "main.h"
#include "cpu.h"
#include "usf.h"

#include "audio.h"

#include <stdio.h>

void Interpreter::R4300i_opcode_SPECIAL (void) {
	(this->*R4300i_Special[ Opcode.funct ])();
}

void Interpreter::R4300i_opcode_REGIMM (void) {
	(this->*R4300i_Regimm[ Opcode.rt ])();
}

void Interpreter::R4300i_opcode_COP0 (void) {
	(this->*R4300i_CoP0[ Opcode.rs ])();
}

void Interpreter::R4300i_opcode_COP0_CO (void) {
	(this->*R4300i_CoP0_Function[ Opcode.funct ])();
}

void Interpreter::R4300i_opcode_COP1 (void) {
	(this->*R4300i_CoP1[ Opcode.fmt ])();
}

void Interpreter::R4300i_opcode_COP1_BC (void) {
	(this->*R4300i_CoP1_BC[ Opcode.ft ])();
}

void Interpreter::R4300i_opcode_COP1_S (void) {
	_controlfp(RoundingModel,_MCW_RC);
	(this->*R4300i_CoP1_S[ Opcode.funct ])();
}

void Interpreter::R4300i_opcode_COP1_D (void) {
	_controlfp(RoundingModel,_MCW_RC);
	(this->*R4300i_CoP1_D[ Opcode.funct ])();
}

void Interpreter::R4300i_opcode_COP1_W (void) {
	(this->*R4300i_CoP1_W[ Opcode.funct ])();
}

void Interpreter::R4300i_opcode_COP1_L (void) {
	(this->*R4300i_CoP1_L[ Opcode.funct ])();
}


Interpreter::Interpreter(Usf & mUsf) : Cpu(mUsf)
{
	RoundingModel = _RC_NEAR;

	R4300i_Opcode[ 0] = &Interpreter::R4300i_opcode_SPECIAL;
	R4300i_Opcode[ 1] = &Interpreter::R4300i_opcode_REGIMM;
	R4300i_Opcode[ 2] = &Interpreter::r4300i_J;
	R4300i_Opcode[ 3] = &Interpreter::r4300i_JAL;
	R4300i_Opcode[ 4] = &Interpreter::r4300i_BEQ;
	R4300i_Opcode[ 5] = &Interpreter::r4300i_BNE;
	R4300i_Opcode[ 6] = &Interpreter::r4300i_BLEZ;
	R4300i_Opcode[ 7] = &Interpreter::r4300i_BGTZ;
	R4300i_Opcode[ 8] = &Interpreter::r4300i_ADDI;
	R4300i_Opcode[ 9] = &Interpreter::r4300i_ADDIU;
	R4300i_Opcode[10] = &Interpreter::r4300i_SLTI;
	R4300i_Opcode[11] = &Interpreter::r4300i_SLTIU;
	R4300i_Opcode[12] = &Interpreter::r4300i_ANDI;
	R4300i_Opcode[13] = &Interpreter::r4300i_ORI;
	R4300i_Opcode[14] = &Interpreter::r4300i_XORI;
	R4300i_Opcode[15] = &Interpreter::r4300i_LUI;
	R4300i_Opcode[16] = &Interpreter::R4300i_opcode_COP0;
	R4300i_Opcode[17] = &Interpreter::R4300i_opcode_COP1;
	R4300i_Opcode[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[20] = &Interpreter::r4300i_BEQL;
	R4300i_Opcode[21] = &Interpreter::r4300i_BNEL;
	R4300i_Opcode[22] = &Interpreter::r4300i_BLEZL;
	R4300i_Opcode[23] = &Interpreter::r4300i_BGTZL;
	R4300i_Opcode[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[25] = &Interpreter::r4300i_DADDIU;
	R4300i_Opcode[26] = &Interpreter::r4300i_LDL;
	R4300i_Opcode[27] = &Interpreter::r4300i_LDR;
	R4300i_Opcode[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[32] = &Interpreter::r4300i_LB;
	R4300i_Opcode[33] = &Interpreter::r4300i_LH;
	R4300i_Opcode[34] = &Interpreter::r4300i_LWL;
	R4300i_Opcode[35] = &Interpreter::r4300i_LW;
	R4300i_Opcode[36] = &Interpreter::r4300i_LBU;
	R4300i_Opcode[37] = &Interpreter::r4300i_LHU;
	R4300i_Opcode[38] = &Interpreter::r4300i_LWR;
	R4300i_Opcode[39] = &Interpreter::r4300i_LWU;
	R4300i_Opcode[40] = &Interpreter::r4300i_SB;
	R4300i_Opcode[41] = &Interpreter::r4300i_SH;
	R4300i_Opcode[42] = &Interpreter::r4300i_SWL;
	R4300i_Opcode[43] = &Interpreter::r4300i_SW;
	R4300i_Opcode[44] = &Interpreter::r4300i_SDL;
	R4300i_Opcode[45] = &Interpreter::r4300i_SDR;
	R4300i_Opcode[46] = &Interpreter::r4300i_SWR;
	R4300i_Opcode[47] = &Interpreter::r4300i_CACHE;
	R4300i_Opcode[48] = &Interpreter::r4300i_LL;
	R4300i_Opcode[49] = &Interpreter::r4300i_LWC1;
	R4300i_Opcode[50] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[51] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[52] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[53] = &Interpreter::r4300i_LDC1;
	R4300i_Opcode[54] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[55] = &Interpreter::r4300i_LD;
	R4300i_Opcode[56] = &Interpreter::r4300i_SC;
	R4300i_Opcode[57] = &Interpreter::r4300i_SWC1;
	R4300i_Opcode[58] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[59] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[60] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[61] = &Interpreter::r4300i_SDC1;
	R4300i_Opcode[62] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Opcode[63] = &Interpreter::r4300i_SD;

	R4300i_Special[ 0] = &Interpreter::r4300i_SPECIAL_SLL;
	R4300i_Special[ 1] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[ 2] = &Interpreter::r4300i_SPECIAL_SRL;
	R4300i_Special[ 3] = &Interpreter::r4300i_SPECIAL_SRA;
	R4300i_Special[ 4] = &Interpreter::r4300i_SPECIAL_SLLV;
	R4300i_Special[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[ 6] = &Interpreter::r4300i_SPECIAL_SRLV;
	R4300i_Special[ 7] = &Interpreter::r4300i_SPECIAL_SRAV;
	R4300i_Special[ 8] = &Interpreter::r4300i_SPECIAL_JR;
	R4300i_Special[ 9] = &Interpreter::r4300i_SPECIAL_JALR;
	R4300i_Special[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[12] = &Interpreter::r4300i_SPECIAL_SYSCALL;
	R4300i_Special[13] = &Interpreter::r4300i_SPECIAL_BREAK;
	R4300i_Special[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[15] = &Interpreter::r4300i_SPECIAL_SYNC;
	R4300i_Special[16] = &Interpreter::r4300i_SPECIAL_MFHI;
	R4300i_Special[17] = &Interpreter::r4300i_SPECIAL_MTHI;
	R4300i_Special[18] = &Interpreter::r4300i_SPECIAL_MFLO;
	R4300i_Special[19] = &Interpreter::r4300i_SPECIAL_MTLO;
	R4300i_Special[20] = &Interpreter::r4300i_SPECIAL_DSLLV;
	R4300i_Special[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[22] = &Interpreter::r4300i_SPECIAL_DSRLV;
	R4300i_Special[23] = &Interpreter::r4300i_SPECIAL_DSRAV;
	R4300i_Special[24] = &Interpreter::r4300i_SPECIAL_MULT;
	R4300i_Special[25] = &Interpreter::r4300i_SPECIAL_MULTU;
	R4300i_Special[26] = &Interpreter::r4300i_SPECIAL_DIV;
	R4300i_Special[27] = &Interpreter::r4300i_SPECIAL_DIVU;
	R4300i_Special[28] = &Interpreter::r4300i_SPECIAL_DMULT;
	R4300i_Special[29] = &Interpreter::r4300i_SPECIAL_DMULTU;
	R4300i_Special[30] = &Interpreter::r4300i_SPECIAL_DDIV;
	R4300i_Special[31] = &Interpreter::r4300i_SPECIAL_DDIVU;
	R4300i_Special[32] = &Interpreter::r4300i_SPECIAL_ADD;
	R4300i_Special[33] = &Interpreter::r4300i_SPECIAL_ADDU;
	R4300i_Special[34] = &Interpreter::r4300i_SPECIAL_SUB;
	R4300i_Special[35] = &Interpreter::r4300i_SPECIAL_SUBU;
	R4300i_Special[36] = &Interpreter::r4300i_SPECIAL_AND;
	R4300i_Special[37] = &Interpreter::r4300i_SPECIAL_OR;
	R4300i_Special[38] = &Interpreter::r4300i_SPECIAL_XOR;
	R4300i_Special[39] = &Interpreter::r4300i_SPECIAL_NOR;
	R4300i_Special[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[42] = &Interpreter::r4300i_SPECIAL_SLT;
	R4300i_Special[43] = &Interpreter::r4300i_SPECIAL_SLTU;
	R4300i_Special[44] = &Interpreter::r4300i_SPECIAL_DADD;
	R4300i_Special[45] = &Interpreter::r4300i_SPECIAL_DADDU;
	R4300i_Special[46] = &Interpreter::r4300i_SPECIAL_DSUB;
	R4300i_Special[47] = &Interpreter::r4300i_SPECIAL_DSUBU;
	R4300i_Special[48] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[49] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[50] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[51] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[52] = &Interpreter::r4300i_SPECIAL_TEQ;
	R4300i_Special[53] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[54] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[55] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[56] = &Interpreter::r4300i_SPECIAL_DSLL;
	R4300i_Special[57] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[58] = &Interpreter::r4300i_SPECIAL_DSRL;
	R4300i_Special[59] = &Interpreter::r4300i_SPECIAL_DSRA;
	R4300i_Special[60] = &Interpreter::r4300i_SPECIAL_DSLL32;
	R4300i_Special[61] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Special[62] = &Interpreter::r4300i_SPECIAL_DSRL32;
	R4300i_Special[63] = &Interpreter::r4300i_SPECIAL_DSRA32;

	R4300i_Regimm[ 0] = &Interpreter::r4300i_REGIMM_BLTZ;
	R4300i_Regimm[ 1] = &Interpreter::r4300i_REGIMM_BGEZ;
	R4300i_Regimm[ 2] = &Interpreter::r4300i_REGIMM_BLTZL;
	R4300i_Regimm[ 3] = &Interpreter::r4300i_REGIMM_BGEZL;
	R4300i_Regimm[ 4] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[ 6] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[16] = &Interpreter::r4300i_REGIMM_BLTZAL;
	R4300i_Regimm[17] = &Interpreter::r4300i_REGIMM_BGEZAL;
	R4300i_Regimm[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_Regimm[31] = &Interpreter::R4300i_UnknownOpcode;

	R4300i_CoP0[ 0] = &Interpreter::r4300i_COP0_MF;
	R4300i_CoP0[ 1] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 2] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 3] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 4] = &Interpreter::r4300i_COP0_MT;
	R4300i_CoP0[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 6] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0[16] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[17] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[18] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[19] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[20] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[21] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[22] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[23] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[24] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[25] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[26] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[27] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[28] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[29] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[30] = &Interpreter::R4300i_opcode_COP0_CO;
	R4300i_CoP0[31] = &Interpreter::R4300i_opcode_COP0_CO;

	R4300i_CoP0_Function[ 0] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 1] = &Interpreter::r4300i_COP0_CO_TLBR;
	R4300i_CoP0_Function[ 2] = &Interpreter::r4300i_COP0_CO_TLBWI;
	R4300i_CoP0_Function[ 3] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 4] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 6] = &Interpreter::r4300i_COP0_CO_TLBWR;
	R4300i_CoP0_Function[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[ 8] = &Interpreter::r4300i_COP0_CO_TLBP;
	R4300i_CoP0_Function[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[24] = &Interpreter::r4300i_COP0_CO_ERET;
	R4300i_CoP0_Function[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[32] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[33] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[34] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[35] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[36] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[37] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[38] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[39] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[42] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[43] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[44] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[45] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[46] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[47] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[48] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[49] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[50] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[51] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[52] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[53] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[54] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[55] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[56] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[57] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[58] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[59] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[60] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[61] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[62] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP0_Function[63] = &Interpreter::R4300i_UnknownOpcode;

	R4300i_CoP1[ 0] = &Interpreter::r4300i_COP1_MF;
	R4300i_CoP1[ 1] = &Interpreter::r4300i_COP1_DMF;
	R4300i_CoP1[ 2] = &Interpreter::r4300i_COP1_CF;
	R4300i_CoP1[ 3] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[ 4] = &Interpreter::r4300i_COP1_MT;
	R4300i_CoP1[ 5] = &Interpreter::r4300i_COP1_DMT;
	R4300i_CoP1[ 6] = &Interpreter::r4300i_COP1_CT;
	R4300i_CoP1[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[ 8] = &Interpreter::R4300i_opcode_COP1_BC;
	R4300i_CoP1[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[16] = &Interpreter::R4300i_opcode_COP1_S;
	R4300i_CoP1[17] = &Interpreter::R4300i_opcode_COP1_D;
	R4300i_CoP1[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[20] = &Interpreter::R4300i_opcode_COP1_W;
	R4300i_CoP1[21] = &Interpreter::R4300i_opcode_COP1_L;
	R4300i_CoP1[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1[31] = &Interpreter::R4300i_UnknownOpcode;

	R4300i_CoP1_BC[ 0] = &Interpreter::r4300i_COP1_BCF;
	R4300i_CoP1_BC[ 1] = &Interpreter::r4300i_COP1_BCT;
	R4300i_CoP1_BC[ 2] = &Interpreter::r4300i_COP1_BCFL;
	R4300i_CoP1_BC[ 3] = &Interpreter::r4300i_COP1_BCTL;
	R4300i_CoP1_BC[ 4] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 6] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_BC[31] = &Interpreter::R4300i_UnknownOpcode;

	R4300i_CoP1_S[ 0] = &Interpreter::r4300i_COP1_S_ADD;
	R4300i_CoP1_S[ 1] = &Interpreter::r4300i_COP1_S_SUB;
	R4300i_CoP1_S[ 2] = &Interpreter::r4300i_COP1_S_MUL;
	R4300i_CoP1_S[ 3] = &Interpreter::r4300i_COP1_S_DIV;
	R4300i_CoP1_S[ 4] = &Interpreter::r4300i_COP1_S_SQRT;
	R4300i_CoP1_S[ 5] = &Interpreter::r4300i_COP1_S_ABS;
	R4300i_CoP1_S[ 6] = &Interpreter::r4300i_COP1_S_MOV;
	R4300i_CoP1_S[ 7] = &Interpreter::r4300i_COP1_S_NEG;
	R4300i_CoP1_S[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[ 9] = &Interpreter::r4300i_COP1_S_TRUNC_L;
	R4300i_CoP1_S[10] = &Interpreter::r4300i_COP1_S_CEIL_L;		//added by Witten
	R4300i_CoP1_S[11] = &Interpreter::r4300i_COP1_S_FLOOR_L;		//added by Witten
	R4300i_CoP1_S[12] = &Interpreter::r4300i_COP1_S_ROUND_W;
	R4300i_CoP1_S[13] = &Interpreter::r4300i_COP1_S_TRUNC_W;
	R4300i_CoP1_S[14] = &Interpreter::r4300i_COP1_S_CEIL_W;		//added by Witten
	R4300i_CoP1_S[15] = &Interpreter::r4300i_COP1_S_FLOOR_W;
	R4300i_CoP1_S[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[32] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[33] = &Interpreter::r4300i_COP1_S_CVT_D;
	R4300i_CoP1_S[34] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[35] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[36] = &Interpreter::r4300i_COP1_S_CVT_W;
	R4300i_CoP1_S[37] = &Interpreter::r4300i_COP1_S_CVT_L;
	R4300i_CoP1_S[38] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[39] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[42] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[43] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[44] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[45] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[46] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[47] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_S[48] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[49] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[50] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[51] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[52] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[53] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[54] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[55] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[56] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[57] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[58] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[59] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[60] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[61] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[62] = &Interpreter::r4300i_COP1_S_CMP;
	R4300i_CoP1_S[63] = &Interpreter::r4300i_COP1_S_CMP;

	R4300i_CoP1_D[ 0] = &Interpreter::r4300i_COP1_D_ADD;
	R4300i_CoP1_D[ 1] = &Interpreter::r4300i_COP1_D_SUB;
	R4300i_CoP1_D[ 2] = &Interpreter::r4300i_COP1_D_MUL;
	R4300i_CoP1_D[ 3] = &Interpreter::r4300i_COP1_D_DIV;
	R4300i_CoP1_D[ 4] = &Interpreter::r4300i_COP1_D_SQRT;
	R4300i_CoP1_D[ 5] = &Interpreter::r4300i_COP1_D_ABS;
	R4300i_CoP1_D[ 6] = &Interpreter::r4300i_COP1_D_MOV;
	R4300i_CoP1_D[ 7] = &Interpreter::r4300i_COP1_D_NEG;
	R4300i_CoP1_D[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[ 9] = &Interpreter::r4300i_COP1_D_TRUNC_L;		//added by Witten
	R4300i_CoP1_D[10] = &Interpreter::r4300i_COP1_D_CEIL_L;		//added by Witten
	R4300i_CoP1_D[11] = &Interpreter::r4300i_COP1_D_FLOOR_L;		//added by Witten
	R4300i_CoP1_D[12] = &Interpreter::r4300i_COP1_D_ROUND_W;
	R4300i_CoP1_D[13] = &Interpreter::r4300i_COP1_D_TRUNC_W;
	R4300i_CoP1_D[14] = &Interpreter::r4300i_COP1_D_CEIL_W;		//added by Witten
	R4300i_CoP1_D[15] = &Interpreter::r4300i_COP1_D_FLOOR_W;		//added by Witten
	R4300i_CoP1_D[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[32] = &Interpreter::r4300i_COP1_D_CVT_S;
	R4300i_CoP1_D[33] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[34] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[35] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[36] = &Interpreter::r4300i_COP1_D_CVT_W;
	R4300i_CoP1_D[37] = &Interpreter::r4300i_COP1_D_CVT_L;
	R4300i_CoP1_D[38] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[39] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[42] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[43] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[44] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[45] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[46] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[47] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_D[48] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[49] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[50] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[51] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[52] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[53] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[54] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[55] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[56] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[57] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[58] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[59] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[60] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[61] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[62] = &Interpreter::r4300i_COP1_D_CMP;
	R4300i_CoP1_D[63] = &Interpreter::r4300i_COP1_D_CMP;

	R4300i_CoP1_W[ 0] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 1] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 2] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 3] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 4] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 6] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[32] = &Interpreter::r4300i_COP1_W_CVT_S;
	R4300i_CoP1_W[33] = &Interpreter::r4300i_COP1_W_CVT_D;
	R4300i_CoP1_W[34] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[35] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[36] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[37] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[38] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[39] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[42] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[43] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[44] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[45] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[46] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[47] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[48] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[49] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[50] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[51] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[52] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[53] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[54] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[55] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[56] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[57] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[58] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[59] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[60] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[61] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[62] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_W[63] = &Interpreter::R4300i_UnknownOpcode;

	R4300i_CoP1_L[ 0] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 1] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 2] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 3] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 4] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 5] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 6] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 7] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 8] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[ 9] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[10] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[11] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[12] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[13] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[14] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[15] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[16] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[17] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[18] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[19] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[20] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[21] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[22] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[23] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[24] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[25] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[26] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[27] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[28] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[29] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[30] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[31] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[32] = &Interpreter::r4300i_COP1_L_CVT_S;
	R4300i_CoP1_L[33] = &Interpreter::r4300i_COP1_L_CVT_D;
	R4300i_CoP1_L[34] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[35] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[36] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[37] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[38] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[39] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[40] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[41] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[42] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[43] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[44] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[45] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[46] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[47] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[48] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[49] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[50] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[51] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[52] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[53] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[54] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[55] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[56] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[57] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[58] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[59] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[60] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[61] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[62] = &Interpreter::R4300i_UnknownOpcode;
	R4300i_CoP1_L[63] = &Interpreter::R4300i_UnknownOpcode;
}

void Interpreter::StartCPU(void)
{

	NextInstruction = NORMAL;
}



void Interpreter::ExecuteInterpreterOpCode(void)
 {

	if (*WaitMode) Timers->Timer = -1;

	if (!r4300i_LW_VAddr(PROGRAM_COUNTER, &Opcode.Hex)) {
		DoTLBMiss(NextInstruction == JUMP,PROGRAM_COUNTER);
		NextInstruction = NORMAL;
		return;
	}


	COUNT_REGISTER += 2;
	Timers->Timer -= 2;

	RANDOM_REGISTER -= 1;
	if ((int)RANDOM_REGISTER < (int)WIRED_REGISTER) {
		RANDOM_REGISTER = 31;
	}

	(this->*R4300i_Opcode[ Opcode.op ])();

	if (GPR[0].DW != 0) {
		GPR[0].DW = 0;
	}

	switch (NextInstruction) {
	case NORMAL:
		PROGRAM_COUNTER += 4;
		break;
	case DELAY_SLOT:
		NextInstruction = JUMP;
		PROGRAM_COUNTER += 4;
		break;
	case JUMP:
		PROGRAM_COUNTER  = JumpToLocation;
		NextInstruction = NORMAL;

		if ((int)Timers->Timer < 0) {  TimerDone(); }
		if (CPU_Action->DoSomething) { DoSomething(); }

	}
}

void Interpreter::TestInterpreterJump (uint32_t PC, uint32_t TargetPC, int32_t Reg1, int32_t Reg2) {
	if (PC != TargetPC) { return; }
	if (DelaySlotEffectsCompare(PC,Reg1,Reg2)) { return; }
	if (usf.CpuMode != CPU_INTERPRETER) { return; }
	InPermLoop();
}


void * Interpreter::RunCpu(uint32_t * Bytes)
{

	audio->BufferPos = 0;
	audio->BufferSize = *Bytes;

	__try
	{
		while(audio->BufferPos < audio->BufferSize)
		{
			ExecuteInterpreterOpCode();
		}
	}

	__except( r4300i_CPU_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) )
	{
		DisplayError("Unhandled Segfault in Interpreter");
		*Bytes = audio->BufferPos;
		return (void *)audio->AudioBuffer;
	}

	*Bytes = audio->BufferPos << 2;
	return (void *)audio->AudioBuffer;

}

