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

/************************* OpCode functions *************************/
void RSPCALL RSPCompile_SPECIAL	   ( void );
void RSPCALL RSPCompile_REGIMM		( void );
void RSPCALL RSPCompile_J			 ( void );
void RSPCALL RSPCompile_JAL		   ( void );
void RSPCALL RSPCompile_BEQ		   ( void );
void RSPCALL RSPCompile_BNE		   ( void );
void RSPCALL RSPCompile_BLEZ		  ( void );
void RSPCALL RSPCompile_BGTZ		  ( void );
void RSPCALL RSPCompile_ADDI		  ( void );
void RSPCALL RSPCompile_ADDIU		 ( void );
void RSPCALL RSPCompile_SLTI		  ( void );
void RSPCALL RSPCompile_SLTIU		 ( void );
void RSPCALL RSPCompile_ANDI		  ( void );
void RSPCALL RSPCompile_ORI		   ( void );
void RSPCALL RSPCompile_XORI		  ( void );
void RSPCALL RSPCompile_LUI		   ( void );
void RSPCALL RSPCompile_COP0		  ( void );
void RSPCALL RSPCompile_COP2		  ( void );
void RSPCALL RSPCompile_LB			( void );
void RSPCALL RSPCompile_LH			( void );
void RSPCALL RSPCompile_LW			( void );
void RSPCALL RSPCompile_LBU		   ( void );
void RSPCALL RSPCompile_LHU		   ( void );
void RSPCALL RSPCompile_SB			( void );
void RSPCALL RSPCompile_SH			( void );
void RSPCALL RSPCompile_SW			( void );
void RSPCALL RSPCompile_LC2		   ( void );
void RSPCALL RSPCompile_SC2		   ( void );
/********************** R4300i OpCodes: Special **********************/
void RSPCALL RSPCompile_Special_SLL   ( void );
void RSPCALL RSPCompile_Special_SRL   ( void );
void RSPCALL RSPCompile_Special_SRA   ( void );
void RSPCALL RSPCompile_Special_SLLV  ( void );
void RSPCALL RSPCompile_Special_SRLV  ( void );
void RSPCALL RSPCompile_Special_SRAV  ( void );
void RSPCALL RSPCompile_Special_JR	( void );
void RSPCALL RSPCompile_Special_JALR  ( void );
void RSPCALL RSPCompile_Special_BREAK ( void );
void RSPCALL RSPCompile_Special_ADD   ( void );
void RSPCALL RSPCompile_Special_ADDU  ( void );
void RSPCALL RSPCompile_Special_SUB   ( void );
void RSPCALL RSPCompile_Special_SUBU  ( void );
void RSPCALL RSPCompile_Special_AND   ( void );
void RSPCALL RSPCompile_Special_OR	( void );
void RSPCALL RSPCompile_Special_XOR   ( void );
void RSPCALL RSPCompile_Special_NOR   ( void );
void RSPCALL RSPCompile_Special_SLT   ( void );
void RSPCALL RSPCompile_Special_SLTU  ( void );
/********************** R4300i OpCodes: RegImm **********************/
void RSPCALL RSPCompile_RegImm_BLTZ   ( void );
void RSPCALL RSPCompile_RegImm_BGEZ   ( void );
void RSPCALL RSPCompile_RegImm_BLTZAL ( void );
void RSPCALL RSPCompile_RegImm_BGEZAL ( void );
/************************** Cop0 functions *************************/
void RSPCALL RSPCompile_Cop0_MF	   ( void );
void RSPCALL RSPCompile_Cop0_MT	   ( void );
/************************** Cop2 functions *************************/
void RSPCALL RSPCompile_Cop2_MF	   ( void );
void RSPCALL RSPCompile_Cop2_CF	   ( void );
void RSPCALL RSPCompile_Cop2_MT	   ( void );
void RSPCALL RSPCompile_Cop2_CT	   ( void );
void RSPCALL RSPCompile_COP2_VECTOR  ( void );
/************************** Vect functions **************************/
void RSPCALL RSPCompile_Vector_VMULF  ( void );
void RSPCALL RSPCompile_Vector_VMULU  ( void );
void RSPCALL RSPCompile_Vector_VMUDL  ( void );
void RSPCALL RSPCompile_Vector_VMUDM  ( void );
void RSPCALL RSPCompile_Vector_VMUDN  ( void );
void RSPCALL RSPCompile_Vector_VMUDH  ( void );
void RSPCALL RSPCompile_Vector_VMACF  ( void );
void RSPCALL RSPCompile_Vector_VMACU  ( void );
void RSPCALL RSPCompile_Vector_VMACQ  ( void );
void RSPCALL RSPCompile_Vector_VMADL  ( void );
void RSPCALL RSPCompile_Vector_VMADM  ( void );
void RSPCALL RSPCompile_Vector_VMADN  ( void );
void RSPCALL RSPCompile_Vector_VMADH  ( void );
void RSPCALL RSPCompile_Vector_VADD   ( void );
void RSPCALL RSPCompile_Vector_VSUB   ( void );
void RSPCALL RSPCompile_Vector_VABS   ( void );
void RSPCALL RSPCompile_Vector_VADDC  ( void );
void RSPCALL RSPCompile_Vector_VSUBC  ( void );
void RSPCALL RSPCompile_Vector_VSAW   ( void );
void RSPCALL RSPCompile_Vector_VLT	( void );
void RSPCALL RSPCompile_Vector_VEQ	( void );
void RSPCALL RSPCompile_Vector_VNE	( void );
void RSPCALL RSPCompile_Vector_VGE	( void );
void RSPCALL RSPCompile_Vector_VCL	( void );
void RSPCALL RSPCompile_Vector_VCH	( void );
void RSPCALL RSPCompile_Vector_VCR	( void );
void RSPCALL RSPCompile_Vector_VMRG   ( void );
void RSPCALL RSPCompile_Vector_VAND   ( void );
void RSPCALL RSPCompile_Vector_VNAND  ( void );
void RSPCALL RSPCompile_Vector_VOR	( void );
void RSPCALL RSPCompile_Vector_VNOR   ( void );
void RSPCALL RSPCompile_Vector_VXOR   ( void );
void RSPCALL RSPCompile_Vector_VNXOR  ( void );
void RSPCALL RSPCompile_Vector_VRCP   ( void );
void RSPCALL RSPCompile_Vector_VRCPL  ( void );
void RSPCALL RSPCompile_Vector_VRCPH  ( void );
void RSPCALL RSPCompile_Vector_VMOV   ( void );
void RSPCALL RSPCompile_Vector_VRSQ   ( void );
void RSPCALL RSPCompile_Vector_VRSQL  ( void );
void RSPCALL RSPCompile_Vector_VRSQH  ( void );
void RSPCALL RSPCompile_Vector_VNOOP  ( void );
/************************** lc2 functions **************************/
void RSPCALL RSPCompile_Opcode_LBV	( void );
void RSPCALL RSPCompile_Opcode_LSV	( void );
void RSPCALL RSPCompile_Opcode_LLV	( void );
void RSPCALL RSPCompile_Opcode_LDV	( void );
void RSPCALL RSPCompile_Opcode_LQV	( void );
void RSPCALL RSPCompile_Opcode_LRV	( void );
void RSPCALL RSPCompile_Opcode_LPV	( void );
void RSPCALL RSPCompile_Opcode_LUV	( void );
void RSPCALL RSPCompile_Opcode_LHV	( void );
void RSPCALL RSPCompile_Opcode_LFV	( void );
void RSPCALL RSPCompile_Opcode_LTV	( void );
/************************** sc2 functions **************************/
void RSPCALL RSPCompile_Opcode_SBV	( void );
void RSPCALL RSPCompile_Opcode_SSV	( void );
void RSPCALL RSPCompile_Opcode_SLV	( void );
void RSPCALL RSPCompile_Opcode_SDV	( void );
void RSPCALL RSPCompile_Opcode_SQV	( void );
void RSPCALL RSPCompile_Opcode_SRV	( void );
void RSPCALL RSPCompile_Opcode_SPV	( void );
void RSPCALL RSPCompile_Opcode_SUV	( void );
void RSPCALL RSPCompile_Opcode_SHV	( void );
void RSPCALL RSPCompile_Opcode_SFV	( void );
void RSPCALL RSPCompile_Opcode_SWV	( void );
void RSPCALL RSPCompile_Opcode_STV	( void );
/************************** Other functions **************************/
void RSPCALL RSPCompile_UnknownOpcode (void);
