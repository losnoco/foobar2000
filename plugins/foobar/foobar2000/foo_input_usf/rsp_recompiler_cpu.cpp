/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
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
#include "rsp.h"
#include "rsp_recompiler_cpu.h"
#include "rsp_recompiler_ops.h"
#include "rsp_x86.h"
#include "recompiler_cpu.h"

#include "main.h"

/* #define REORDER_BLOCK_VERBOSE */
#define LINK_BRANCHES_VERBOSE /* no choice really */
#define X86_RECOMP_VERBOSE
#define BUILD_BRANCHLABELS_VERBOSE
/*
*/

void Rsp::BuildRecompilerCPU ( void ) {
	RSP_Opcode[ 0] = &Rsp::RSPCompile_SPECIAL;
	RSP_Opcode[ 1] = &Rsp::RSPCompile_REGIMM;
	RSP_Opcode[ 2] = &Rsp::RSPCompile_J;
	RSP_Opcode[ 3] = &Rsp::RSPCompile_JAL;
	RSP_Opcode[ 4] = &Rsp::RSPCompile_BEQ;
	RSP_Opcode[ 5] = &Rsp::RSPCompile_BNE;
	RSP_Opcode[ 6] = &Rsp::RSPCompile_BLEZ;
	RSP_Opcode[ 7] = &Rsp::RSPCompile_BGTZ;
	RSP_Opcode[ 8] = &Rsp::RSPCompile_ADDI;
	RSP_Opcode[ 9] = &Rsp::RSPCompile_ADDIU;
	RSP_Opcode[10] = &Rsp::RSPCompile_SLTI;
	RSP_Opcode[11] = &Rsp::RSPCompile_SLTIU;
	RSP_Opcode[12] = &Rsp::RSPCompile_ANDI;
	RSP_Opcode[13] = &Rsp::RSPCompile_ORI;
	RSP_Opcode[14] = &Rsp::RSPCompile_XORI;
	RSP_Opcode[15] = &Rsp::RSPCompile_LUI;
	RSP_Opcode[16] = &Rsp::RSPCompile_COP0;
	RSP_Opcode[17] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[18] = &Rsp::RSPCompile_COP2;
	RSP_Opcode[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[31] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[32] = &Rsp::RSPCompile_LB;
	RSP_Opcode[33] = &Rsp::RSPCompile_LH;
	RSP_Opcode[34] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[35] = &Rsp::RSPCompile_LW;
	RSP_Opcode[36] = &Rsp::RSPCompile_LBU;
	RSP_Opcode[37] = &Rsp::RSPCompile_LHU;
	RSP_Opcode[38] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[39] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[40] = &Rsp::RSPCompile_SB;
	RSP_Opcode[41] = &Rsp::RSPCompile_SH;
	RSP_Opcode[42] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[43] = &Rsp::RSPCompile_SW;
	RSP_Opcode[44] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[45] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[46] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[47] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[48] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[49] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[50] = &Rsp::RSPCompile_LC2;
	RSP_Opcode[51] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[52] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[53] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[54] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[55] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[56] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[57] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[58] = &Rsp::RSPCompile_SC2;
	RSP_Opcode[59] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[60] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[61] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[62] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Opcode[63] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_Special[ 0] = &Rsp::RSPCompile_Special_SLL;
	RSP_Special[ 1] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[ 2] = &Rsp::RSPCompile_Special_SRL;
	RSP_Special[ 3] = &Rsp::RSPCompile_Special_SRA;
	RSP_Special[ 4] = &Rsp::RSPCompile_Special_SLLV;
	RSP_Special[ 5] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[ 6] = &Rsp::RSPCompile_Special_SRLV;
	RSP_Special[ 7] = &Rsp::RSPCompile_Special_SRAV;
	RSP_Special[ 8] = &Rsp::RSPCompile_Special_JR;
	RSP_Special[ 9] = &Rsp::RSPCompile_Special_JALR;
	RSP_Special[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[11] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[13] = &Rsp::RSPCompile_Special_BREAK;
	RSP_Special[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[16] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[17] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[31] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[32] = &Rsp::RSPCompile_Special_ADD;
	RSP_Special[33] = &Rsp::RSPCompile_Special_ADDU;
	RSP_Special[34] = &Rsp::RSPCompile_Special_SUB;
	RSP_Special[35] = &Rsp::RSPCompile_Special_SUBU;
	RSP_Special[36] = &Rsp::RSPCompile_Special_AND;
	RSP_Special[37] = &Rsp::RSPCompile_Special_OR;
	RSP_Special[38] = &Rsp::RSPCompile_Special_XOR;
	RSP_Special[39] = &Rsp::RSPCompile_Special_NOR;
	RSP_Special[40] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[41] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[42] = &Rsp::RSPCompile_Special_SLT;
	RSP_Special[43] = &Rsp::RSPCompile_Special_SLTU;
	RSP_Special[44] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[45] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[46] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[47] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[48] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[49] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[50] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[51] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[52] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[53] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[54] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[55] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[56] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[57] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[58] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[59] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[60] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[61] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[62] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Special[63] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_RegImm[ 0] = &Rsp::RSPCompile_RegImm_BLTZ;
	RSP_RegImm[ 1] = &Rsp::RSPCompile_RegImm_BGEZ;
	RSP_RegImm[ 2] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 3] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 4] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 5] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 6] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 7] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 8] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[ 9] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[11] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[13] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[16] = &Rsp::RSPCompile_RegImm_BLTZAL;
	RSP_RegImm[17] = &Rsp::RSPCompile_RegImm_BGEZAL;
	RSP_RegImm[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_RegImm[31] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_Cop0[ 0] = &Rsp::RSPCompile_Cop0_MF;
	RSP_Cop0[ 1] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 2] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 3] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 4] = &Rsp::RSPCompile_Cop0_MT;
	RSP_Cop0[ 5] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 6] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 7] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 8] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[ 9] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[11] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[13] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[16] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[17] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop0[31] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_Cop2[ 0] = &Rsp::RSPCompile_Cop2_MF;
	RSP_Cop2[ 1] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[ 2] = &Rsp::RSPCompile_Cop2_CF;
	RSP_Cop2[ 3] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[ 4] = &Rsp::RSPCompile_Cop2_MT;
	RSP_Cop2[ 5] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[ 6] = &Rsp::RSPCompile_Cop2_CT;
	RSP_Cop2[ 7] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[ 8] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[ 9] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[11] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[13] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Cop2[16] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[17] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[18] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[19] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[20] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[21] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[22] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[23] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[24] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[25] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[26] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[27] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[28] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[29] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[30] = &Rsp::RSPCompile_COP2_VECTOR;
	RSP_Cop2[31] = &Rsp::RSPCompile_COP2_VECTOR;

	RSP_Vector[ 0] = &Rsp::RSPCompile_Vector_VMULF;
	RSP_Vector[ 1] = &Rsp::RSPCompile_Vector_VMULU;
	RSP_Vector[ 2] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[ 3] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[ 4] = &Rsp::RSPCompile_Vector_VMUDL;
	RSP_Vector[ 5] = &Rsp::RSPCompile_Vector_VMUDM;
	RSP_Vector[ 6] = &Rsp::RSPCompile_Vector_VMUDN;
	RSP_Vector[ 7] = &Rsp::RSPCompile_Vector_VMUDH;
	RSP_Vector[ 8] = &Rsp::RSPCompile_Vector_VMACF;
	RSP_Vector[ 9] = &Rsp::RSPCompile_Vector_VMACU;
	RSP_Vector[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[11] = &Rsp::RSPCompile_Vector_VMACQ;
	RSP_Vector[12] = &Rsp::RSPCompile_Vector_VMADL;
	RSP_Vector[13] = &Rsp::RSPCompile_Vector_VMADM;
	RSP_Vector[14] = &Rsp::RSPCompile_Vector_VMADN;
	RSP_Vector[15] = &Rsp::RSPCompile_Vector_VMADH;
	RSP_Vector[16] = &Rsp::RSPCompile_Vector_VADD;
	RSP_Vector[17] = &Rsp::RSPCompile_Vector_VSUB;
	RSP_Vector[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[19] = &Rsp::RSPCompile_Vector_VABS;
	RSP_Vector[20] = &Rsp::RSPCompile_Vector_VADDC;
	RSP_Vector[21] = &Rsp::RSPCompile_Vector_VSUBC;
	RSP_Vector[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[29] = &Rsp::RSPCompile_Vector_VSAW;
	RSP_Vector[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[31] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[32] = &Rsp::RSPCompile_Vector_VLT;
	RSP_Vector[33] = &Rsp::RSPCompile_Vector_VEQ;
	RSP_Vector[34] = &Rsp::RSPCompile_Vector_VNE;
	RSP_Vector[35] = &Rsp::RSPCompile_Vector_VGE;
	RSP_Vector[36] = &Rsp::RSPCompile_Vector_VCL;
	RSP_Vector[37] = &Rsp::RSPCompile_Vector_VCH;
	RSP_Vector[38] = &Rsp::RSPCompile_Vector_VCR;
	RSP_Vector[39] = &Rsp::RSPCompile_Vector_VMRG;
	RSP_Vector[40] = &Rsp::RSPCompile_Vector_VAND;
	RSP_Vector[41] = &Rsp::RSPCompile_Vector_VNAND;
	RSP_Vector[42] = &Rsp::RSPCompile_Vector_VOR;
	RSP_Vector[43] = &Rsp::RSPCompile_Vector_VNOR;
	RSP_Vector[44] = &Rsp::RSPCompile_Vector_VXOR;
	RSP_Vector[45] = &Rsp::RSPCompile_Vector_VNXOR;
	RSP_Vector[46] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[47] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[48] = &Rsp::RSPCompile_Vector_VRCP;
	RSP_Vector[49] = &Rsp::RSPCompile_Vector_VRCPL;
	RSP_Vector[50] = &Rsp::RSPCompile_Vector_VRCPH;
	RSP_Vector[51] = &Rsp::RSPCompile_Vector_VMOV;
	RSP_Vector[52] = &Rsp::RSPCompile_Vector_VRSQ;
	RSP_Vector[53] = &Rsp::RSPCompile_Vector_VRSQL;
	RSP_Vector[54] = &Rsp::RSPCompile_Vector_VRSQH;
	RSP_Vector[55] = &Rsp::RSPCompile_Vector_VNOOP;
	RSP_Vector[56] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[57] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[58] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[59] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[60] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[61] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[62] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Vector[63] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_Lc2[ 0] = &Rsp::RSPCompile_Opcode_LBV;
	RSP_Lc2[ 1] = &Rsp::RSPCompile_Opcode_LSV;
	RSP_Lc2[ 2] = &Rsp::RSPCompile_Opcode_LLV;
	RSP_Lc2[ 3] = &Rsp::RSPCompile_Opcode_LDV;
	RSP_Lc2[ 4] = &Rsp::RSPCompile_Opcode_LQV;
	RSP_Lc2[ 5] = &Rsp::RSPCompile_Opcode_LRV;
	RSP_Lc2[ 6] = &Rsp::RSPCompile_Opcode_LPV;
	RSP_Lc2[ 7] = &Rsp::RSPCompile_Opcode_LUV;
	RSP_Lc2[ 8] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[ 9] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[11] = &Rsp::RSPCompile_Opcode_LTV;
	RSP_Lc2[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[13] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[16] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[17] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Lc2[31] = &Rsp::RSPCompile_UnknownOpcode;

	RSP_Sc2[ 0] = &Rsp::RSPCompile_Opcode_SBV;
	RSP_Sc2[ 1] = &Rsp::RSPCompile_Opcode_SSV;
	RSP_Sc2[ 2] = &Rsp::RSPCompile_Opcode_SLV;
	RSP_Sc2[ 3] = &Rsp::RSPCompile_Opcode_SDV;
	RSP_Sc2[ 4] = &Rsp::RSPCompile_Opcode_SQV;
	RSP_Sc2[ 5] = &Rsp::RSPCompile_Opcode_SRV;
	RSP_Sc2[ 6] = &Rsp::RSPCompile_Opcode_SPV;
	RSP_Sc2[ 7] = &Rsp::RSPCompile_Opcode_SUV;
	RSP_Sc2[ 8] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[ 9] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[10] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[11] = &Rsp::RSPCompile_Opcode_STV;
	RSP_Sc2[12] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[13] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[14] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[15] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[16] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[17] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[18] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[19] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[20] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[21] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[22] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[23] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[24] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[25] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[26] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[27] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[28] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[29] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[30] = &Rsp::RSPCompile_UnknownOpcode;
	RSP_Sc2[31] = &Rsp::RSPCompile_UnknownOpcode;

	RSPBlockID = 0;
	#ifdef Log_x86Code
	Start_x86_Log();
	#endif
}

/******************************************************
** ReOrderSubBlock
**
** Desc:
** this can be done, but will be interesting to put
** between branches labels, and actual branches, whichever
** occurs first in code
**
********************************************************/

void Rsp::ReOrderInstructions(uint32_t StartPC, uint32_t EndPC) {
	uint32_t InstructionCount = EndPC - StartPC;
	uint32_t Count, ReorderedOps, CurrentPC;
	RSPOPCODE PreviousOp, CurrentOp, RspOp;

	PreviousOp.Hex = *(uint32_t*)(cpu.IMEM + StartPC);

	if (1 == IsOpcodeBranch(StartPC, PreviousOp)) {
		/* the sub block ends here anyway */
		return;
	}

	if (IsOpcodeNop(StartPC) && IsOpcodeNop(StartPC + 4) && IsOpcodeNop(StartPC + 8)) {
		/* Dont even bother */
		return;
	}

	CPU_Message("***** Doing reorder (%X to %X) *****", StartPC, EndPC);

	if (InstructionCount < 0x0010) { return; }
	if (InstructionCount > 0x0A00) { return; }

	CPU_Message(" Before:");
	for (Count = StartPC; Count < EndPC; Count += 4) {
		RSP_LW_IMEM(Count, &RspOp.Hex);
		//CPU_Message("  %X %s",Count,RSPOpcodeName(RspOp.Hex,Count));
	}

	for (Count = 0; Count < InstructionCount; Count += 4) {
		CurrentPC = StartPC;
		PreviousOp.Hex = *(uint32_t*)(cpu.IMEM + CurrentPC);
		ReorderedOps = 0;

		for (;;) {
			CurrentPC += 4;
			if (CurrentPC >= EndPC) { break; }
			CurrentOp.Hex = *(uint32_t*)(cpu.IMEM + CurrentPC);

			if (1 == CompareInstructions(CurrentPC, &PreviousOp, &CurrentOp)) {
				/* Move current opcode up */
				*(uint32_t*)(cpu.IMEM + CurrentPC - 4) = CurrentOp.Hex;
			 	*(uint32_t*)(cpu.IMEM + CurrentPC) = PreviousOp.Hex;

				ReorderedOps++;
				#ifdef REORDER_BLOCK_VERBOSE
				CPU_Message("Swapped %X and %X", CurrentPC - 4, CurrentPC);
				#endif
			}
			PreviousOp.Hex = *(uint32_t*)(cpu.IMEM + CurrentPC);

			if (IsOpcodeNop(CurrentPC) && IsOpcodeNop(CurrentPC + 4) && IsOpcodeNop(CurrentPC + 8)) {
				CurrentPC = EndPC;
			}
		}

		if (ReorderedOps == 0) {
			Count = InstructionCount;
		}
	}

	CPU_Message(" After:");
	for (Count = StartPC; Count < EndPC; Count += 4) {
		RSP_LW_IMEM(Count, &RspOp.Hex);
		//CPU_Message("  %X %s",Count,RSPOpcodeName(RspOp.Hex,Count));
	}
	CPU_Message("");
}

void Rsp::ReOrderSubBlock(RSP_BLOCK * Block) {
	uint32_t end = 0x0ffc;
	uint32_t count;

	//return; //fixthis
	if (!Compiler.bReOrdering) {
		return;
	}
	if (Block->CurrPC > 0xFF0) {
		return;
	}

	/* find the label or jump closest to us */
	if (RspCode.LabelCount) {
		for (count = 0; count < RspCode.LabelCount; count++) {
			if (RspCode.BranchLabels[count] < end &&
				  RspCode.BranchLabels[count] > Block->CurrPC) {
				end = RspCode.BranchLabels[count];
			}
		}
	}
	if (RspCode.BranchCount) {
		for (count = 0; count < RspCode.BranchCount; count++) {
			if (RspCode.BranchLocations[count] < end &&
				  RspCode.BranchLocations[count] > Block->CurrPC) {
				end = RspCode.BranchLocations[count];
			}
		}
	}

	/* it wont actually re-order the op at the end */
	ReOrderInstructions(Block->CurrPC, end);
}

/******************************************************
** DetectGPRConstants
**
** Desc:
**  this needs to be called on a sub-block basis, like
**  after every time we hit a branch and delay slot
**
********************************************************/

void Rsp::DetectGPRConstants(RSP_CODE * code) {
	uint32_t Count, Constant = 0;

	memset(&code->bIsRegConst, 0, sizeof(int32_t) * 0x20);
	memset(&code->MipsRegConst, 0, sizeof(uint32_t) * 0x20);

	if (!Compiler.bGPRConstants) {
		return;
	}
	CPU_Message("***** Detecting constants *****");

	/*** Setup R0 ***/
	code->bIsRegConst[0] = 1;
	code->MipsRegConst[0] = 0;

	/* Do your global search for them */
	for (Count = 1; Count < 32; Count++) {
		if (IsRegisterConstant(Count, &Constant) == 1) {
			//CPU_Message("Global: %s is a constant of: %08X", GPR_Name(Count), Constant);
			code->bIsRegConst[Count] = 1;
			code->MipsRegConst[Count] = Constant;
		}
	}
	CPU_Message("");
}

/******************************************************
** CompilerToggleBuffer and ClearX86Code
**
** Desc:
**  1> toggles the compiler buffer, useful for poorly
**  taken branches like alignment
**
**  2> clears all the x86 code, jump tables etc
**
********************************************************/

void Rsp::CompilerToggleBuffer(void) {
	if (dwBuffer == MainBuffer) {
		dwBuffer = SecondaryBuffer;
		pLastPrimary = RSPRecompPos;

		if (pLastSecondary == NULL) {
			pLastSecondary = RSPRecompCodeSecondary;
		}

		RSPRecompPos = pLastSecondary;
		CPU_Message("   (Secondary Buffer Active 0x%08X)", pLastSecondary);
	} else {
		dwBuffer = MainBuffer;
		pLastSecondary = RSPRecompPos;

		if (pLastPrimary == NULL) {
			pLastPrimary = RSPRecompCode;
		}

		RSPRecompPos = pLastPrimary;
		CPU_Message("   (Primary Buffer Active 0x%08X)", pLastPrimary);
	}
}

void Rsp::ClearAllx86Code (void) {

	memset(&MapsCRC, 0, sizeof(uint32_t) * 0x20);
	NoOfMaps = 0;
	memset(RSPJumpTables,0,0x2000*32);

	RSPRecompPos = RSPRecompCode;

	pLastPrimary = NULL;
	pLastSecondary = NULL;
}

/******************************************************
** Link Branches
**
** Desc:
**  resolves all the collected branches, x86 style
**
********************************************************/

void Rsp::LinkBranches(RSP_BLOCK * Block) {
	uint32_t Count, Target;
	uint32_t * JumpWord;
	uint8_t * X86Code;
	RSP_BLOCK Save;

	if (!RSPCurrentBlock.ResolveCount) {
		return;
	}
	CPU_Message("***** Linking branches (%i) *****", RSPCurrentBlock.ResolveCount);

	for (Count = 0; Count < RSPCurrentBlock.ResolveCount; Count++) {
		Target = RSPCurrentBlock.BranchesToResolve[Count].TargetPC;
		X86Code = (uint8_t *)*(RSPJumpTable + (Target >> 2));

		if (!X86Code) {
			*PrgCount = Target;
			CPU_Message("");
			CPU_Message("===== (Generate Code: %04X) =====", Target);
			Save = *Block;

			/* compile this block and link */
			CompilerRSPBlock();
			LinkBranches(Block);

			*Block = Save;
			CPU_Message("===== (End Generate Code: %04X) =====", Target);
			CPU_Message("");
			X86Code = (uint8_t *)*(RSPJumpTable + (Target >> 2));
		}

		JumpWord = RSPCurrentBlock.BranchesToResolve[Count].X86JumpLoc;
		RSPx86_SetBranch32b(JumpWord, (uint32_t*)X86Code);

		CPU_Message("Linked RSP branch from x86: %08X, to RSP: %X / x86: %08X",
			JumpWord, Target, X86Code);
	}
	CPU_Message("***** Done Linking Branches *****");
	CPU_Message("");
}

/******************************************************
** BuildBranchLabels
**
** Desc:
**   Branch labels are used to start and stop re-ordering
**   sections as well as set the jump table to points
**   within a block that are safe
**
********************************************************/

void Rsp::BuildBranchLabels(void) {
	RSPOPCODE RspOp;
	uint32_t i, Dest;

	#ifdef BUILD_BRANCHLABELS_VERBOSE
	CPU_Message("***** Building branch labels *****");
	#endif

	for (i = 0; i < 0x1000; i += 4) {
		RspOp.Hex = *(uint32_t*)(cpu.IMEM + i);

		if (1 == IsOpcodeBranch(i, RspOp)) {
			if (RspCode.LabelCount >= 175) {
				CompilerWarning("Out of space for Branch Labels",1);
				return;
			}

			RspCode.BranchLocations[RspCode.BranchCount++] = i;
			if (RspOp.op == RSP_SPECIAL) {
				/* register jump not predictable */
			} else if (RspOp.op == RSP_J || RspOp.op == RSP_JAL) {
				/* for JAL its a sub-block for returns */
				Dest = (RspOp.target << 2) & 0xFFC;
				RspCode.BranchLabels[RspCode.LabelCount] = Dest;
				RspCode.LabelCount += 1;
				#ifdef BUILD_BRANCHLABELS_VERBOSE
				CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
				#endif
			} else {
				Dest = (i + ((int16_t)RspOp.offset << 2) + 4) & 0xFFC;
				RspCode.BranchLabels[RspCode.LabelCount] = Dest;
				RspCode.LabelCount += 1;
				#ifdef BUILD_BRANCHLABELS_VERBOSE
				CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
				#endif
			}
		}
	}

	#ifdef BUILD_BRANCHLABELS_VERBOSE
	CPU_Message("***** End branch labels *****");
	CPU_Message("");
	#endif
}

int32_t Rsp::IsJumpLabel(uint32_t PC) {
	uint32_t Count;

	if (!RspCode.LabelCount) {
		return 0;
	}

	for (Count = 0; Count < RspCode.LabelCount; Count++) {
		if (PC == RspCode.BranchLabels[Count]) {
			return 1;
		}
	}
	return 0;
}

void Rsp::CompilerLinkBlocks(void) {
	uint8_t * KnownCode = (uint8_t *)*(RSPJumpTable + (RSPCompilePC >> 2));

	CPU_Message("***** Linking block to X86: %08X *****", KnownCode);
	RSPNextInstruction = FINISH_BLOCK;

	/* block linking scenario */
	RSPJmpLabel32(0);
	RSPx86_SetBranch32b(RSPRecompPos - 4, KnownCode);
}


void Rsp::CompilerRSPBlock ( void ) {
	uint32_t Count, Padding, X86BaseAddress = (uint32_t)RSPRecompPos;

	RSPNextInstruction = NORMAL;
	RSPCompilePC = *PrgCount;

	memset(&RSPCurrentBlock, 0, sizeof(RSPCurrentBlock));
	RSPCurrentBlock.StartPC = RSPCompilePC;
	RSPCurrentBlock.CurrPC = RSPCompilePC;


	/* Align the block to a boundary */
	if (X86BaseAddress & 7) {
		Padding = (8 - (X86BaseAddress & 7)) & 7;
		for (Count = 0; Count < Padding; Count++) {
			*(RSPRecompPos++) = 0x90;
		}
	}

	if (Compiler.bReOrdering == 1) {
		memcpy(&RSPCurrentBlock.IMEM[0], cpu.IMEM, 0x1000);
		ReOrderSubBlock(&RSPCurrentBlock);
	}

	/* this is for the block about to be compiled */
	*(RSPJumpTable + (RSPCompilePC >> 2)) = RSPRecompPos;


#ifdef USEX64
	RSPMoveConstQwordToX86reg(TLB_Map, x86_R15);
#endif

	do {
		/*
		** Re-Ordering is setup to allow us to have loop labels
		** so here we see if this is one and put it in the jump table
		**/

		if (RSPNextInstruction == NORMAL && IsJumpLabel(RSPCompilePC)) {
			/* jumps come around twice */
			if (NULL == *(RSPJumpTable + (RSPCompilePC >> 2))) {
				CPU_Message("***** Adding Jump Table Entry for PC: %04X at X86: %08X *****", RSPCompilePC, RSPRecompPos);
				CPU_Message("");
				*(RSPJumpTable + (RSPCompilePC >> 2)) = RSPRecompPos;

				/* reorder from here to next label or branch */
				RSPCurrentBlock.CurrPC = RSPCompilePC;
				ReOrderSubBlock(&RSPCurrentBlock);
			} else if (RSPNextInstruction != DELAY_SLOT_DONE) {
				/*
				 * we could link the blocks here, but performance
				 * wise it might be better to just let it run
				 */
			}
		}

		if (Compiler.bSections == 1) {
		//	if (1 == RSP_DoSections()) {
		//		continue;
		//	}
		}

		#ifdef X86_RECOMP_VERBOSE
		if (0 == IsOpcodeNop(RSPCompilePC)) {
			CPU_Message("X86 Address: %08X", RSPRecompPos);
		}
		#endif

		RSP_LW_IMEM(RSPCompilePC, &RSPOpC.Hex);

		if (RSPOpC.Hex == 0xFFFFFFFF) {
			/* i think this pops up an unknown op dialog */
			/* NextInstruction = FINISH_BLOCK; */
		} else {
			(this->*RSP_Opcode[ RSPOpC.op ])();
		}

		////printf("%08x\n", RSPNextInstruction);

		switch (RSPNextInstruction) {
		case NORMAL:
			RSPCompilePC += 4;
			break;
		case DO_DELAY_SLOT:
			RSPNextInstruction = DELAY_SLOT;
			RSPCompilePC += 4;
			break;
		case DELAY_SLOT:
			RSPNextInstruction = DELAY_SLOT_DONE;
			RSPCompilePC -= 4;
			break;
		case FINISH_SUB_BLOCK:
			RSPNextInstruction = NORMAL;
			RSPCompilePC += 8;
			if (RSPCompilePC >= 0x1000) {
				RSPNextInstruction = FINISH_BLOCK;
			} else if (NULL == *(RSPJumpTable + (RSPCompilePC >> 2))) {
				/* this is for the new block being compiled now */
				CPU_Message("**** Continuing static SubBlock (jump table entry added for PC: %04X at X86: %08X) *****", RSPCompilePC, RSPRecompPos);
				*(RSPJumpTable + (RSPCompilePC >> 2)) = RSPRecompPos;

				RSPCurrentBlock.CurrPC = RSPCompilePC;
				/* reorder from after delay to next label or branch */
				ReOrderSubBlock(&RSPCurrentBlock);
			} else {
				CompilerLinkBlocks();
			}
			break;

		case FINISH_BLOCK: break;
		default:
			DisplayError("Rsp Main loop\n\nWTF NextInstruction = %d",RSPNextInstruction);
			RSPCompilePC += 4;
			break;
		}
	} while ( RSPNextInstruction != FINISH_BLOCK && RSPCompilePC < 0x1000);
	CPU_Message("==== end of recompiled code ====");

	if (Compiler.bReOrdering == 1) {
		memcpy(cpu.IMEM, &RSPCurrentBlock.IMEM[0], 0x1000);
	}
}

uint32_t Rsp::RunRecompilerCPU ( uint32_t Cycles ) {
	uint8_t * Block;

	RSP_Running = 1;
	RSPSetJumpTable();



	while (RSP_Running) {
		Block = (uint8_t *)*(RSPJumpTable + (*PrgCount >> 2));

		if (Block == NULL) {
			//__try {
				memset(&RspCode, 0, sizeof(RspCode));
				BuildBranchLabels();
				DetectGPRConstants(&RspCode);
				CompilerRSPBlock();
			/*} __except(EXCEPTION_EXECUTE_HANDLER) {
				DisplayError("Error RSPCompilePC = %08X", RSPCompilePC);
				ClearAllx86Code();
				continue;
			}*/

			Block = (uint8_t *)*(RSPJumpTable + (*PrgCount >> 2));

			/*
			** we are done compiling, but we may have references
			** to fill in still either from this block, or jumps
			** that go out of it, let's rock
			**/

			LinkBranches(&RSPCurrentBlock);
		}
		{
		#ifdef USEX64

			void (*block)();
			block=Block;
			block();
		#else
			_asm {
				pushad
				call Block
				popad
			}
		#endif
		}

	}

//	if (IsMmxEnabled == 1) {
//		_Emms();
//	}
	return Cycles;
}
