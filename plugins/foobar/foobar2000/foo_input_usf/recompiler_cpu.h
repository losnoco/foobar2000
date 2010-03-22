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
#ifndef _RECOMPILER_CPU_H_
#define _RECOMPILER_CPU_H_

#ifdef USEX64
#define X86_REGS	64
#else
#define X86_REGS	10
#endif

#include "types.h"

#pragma pack(push,1)

#define MaxCodeBlocks			50000
#define MaxOrigMem				65000

#define NotMapped				0
#define GPR_Mapped				1
#define Temp_Mapped				2
#define Stack_Mapped			3

#define BranchTypeCop1			0
#define BranchTypeRs			1
#define BranchTypeRsRt			2

//Exit Block Methods
#define Normal					0
#define Normal_NoSysCheck		1
#define DoCPU_Action			2
#define COP1_Unuseable			3
#define DoSysCall				4
#define TLBReadMiss				5
#define ExitResetRecompCode		6
#define DoBreak					7

#define STATE_KNOWN_VALUE		1
//#define STATE_UNKNOW_VALUE

#define STATE_X86_MAPPED		2
//#define STATE_CONST

#define STATE_SIGN				4
//#define STATE_ZERO

#define STATE_32BIT				8
//#define STATE_64BIT

#define STATE_UNKNOWN			0

//STATE_MAPPED_64 = 3;
//STATE_MAPPED_32_ZERO = 11
//STATE_MAPPED_32_SIGN = 15
#define STATE_MAPPED_64			(STATE_KNOWN_VALUE | STATE_X86_MAPPED)
#define STATE_MAPPED_32_ZERO	(STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT)
#define STATE_MAPPED_32_SIGN	(STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT | STATE_SIGN)

//STATE_CONST_64 = 1
//STATE_CONST_32 = 13
#define STATE_CONST_64			(STATE_KNOWN_VALUE)
#define STATE_CONST_32			(STATE_KNOWN_VALUE | STATE_32BIT | STATE_SIGN)

#define IsKnown(Reg)			((MipsRegState(Reg) & STATE_KNOWN_VALUE) != 0)
#define IsUnknown(Reg)			(!IsKnown(Reg))

#define IsMapped(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0)
#define IsConst(Reg)			(IsKnown(Reg) && !IsMapped(Reg))

#define IsSigned(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_SIGN) != 0)
#define IsUnsigned(Reg)			(IsKnown(Reg) && !IsSigned(Reg))

#define Is32Bit(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_32BIT) != 0)
#define Is64Bit(Reg)			(IsKnown(Reg) && !Is32Bit(Reg))

#define Is32BitMapped(Reg)		(Is32Bit(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0)
#define Is64BitMapped(Reg)		(Is64Bit(Reg) && !Is32BitMapped(Reg))

#define MipsRegState(Reg)		Section->RegWorking.MIPS_RegState[Reg]
#define MipsReg(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UDW
#define MipsReg_S(Reg)			Section->RegWorking.MIPS_RegVal[Reg].DW
#define MipsRegLo(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UW[0]
#define MipsRegLo_S(Reg)		Section->RegWorking.MIPS_RegVal[Reg].W[0]
#define MipsRegHi(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UW[1]
#define MipsRegHi_S(Reg)		Section->RegWorking.MIPS_RegVal[Reg].W[1]

#define x86MapOrder(Reg)		Section->RegWorking.x86reg_MapOrder[Reg]
#define x86Protected(Reg)		Section->RegWorking.x86reg_Protected[Reg]
#define x86Mapped(Reg)			Section->RegWorking.x86reg_MappedTo[Reg]

#define BlockCycleCount			Section->RegWorking.CycleCount
#define BlockRandomModifier		Section->RegWorking.RandomModifier


#define StackTopPos				Section->RegWorking.Stack_TopPos
#define FpuMappedTo(Reg)		Section->RegWorking.x86fpu_MappedTo[Reg]
#define FpuState(Reg)			Section->RegWorking.x86fpu_State[Reg]
#define FpuRoundingModel(Reg)	Section->RegWorking.x86fpu_RoundingModel[Reg]
#define FpuBeenUsed				Section->RegWorking.Fpu_Used
#define CurrentRoundingModel	Section->RegWorking.RoundingModel

#pragma pack(pop)

uint8_t *Compiler4300iBlock	( void );
uint8_t *CompileDelaySlot	  ( void );
void CompileExit			( uint32_t TargetPC, REG_INFO * ExitRegSet, int32_t reason, int32_t CompileNow, void (*x86Jmp)(uintptr_t Value));
void CompileSystemCheck	 ( uint32_t TimerModifier, uint32_t TargetPC, REG_INFO RegSet );
void FixRandomReg		   ( void );
void FreeSection			( BLOCK_SECTION * Section, BLOCK_SECTION * Parent);
void StartRecompilerCPU	 ( void );
void GenerateSectionLinkage ( BLOCK_SECTION * Section );
void InitilizeInitialCompilerVariable ( void);
int UnMap_TempRegSet (REG_INFO * RegWorking);


#define SetJump32(Loc,JumpLoc) *(uint32_t *)(Loc)= (int32_t)(((int32_t)(JumpLoc)) - (((int32_t)(Loc)) + 4));
#define SetJump8(Loc,JumpLoc)  *(uint8_t  *)(Loc)= (int8_t )(((int8_t )(JumpLoc)) - (((int8_t )(Loc)) + 1));

#ifdef __cplusplus

#include "x86.h"
#include "cpu.h"

class Recompiler : public X86, public Cpu {
	public:
		Recompiler(Usf &);
		~Recompiler();

		void ResetRecompCode			 ( void );

		void InitRecompilerMemory();
		void AnalyseBlock (void);
		void (*Compiler4300iBlock(void))();
		void (*CompileDelaySlot(void))();
		void CompileExit (uint32_t TargetPC, REG_INFO * ExitRegSet, int reason, int CompileNow, void (Recompiler::*x86Jmp)(uint32_t Value));
		void CompileSystemCheck (uint32_t TimerModifier, uint32_t TargetPC, REG_INFO RegSet);
		void CreateSectionLinkage (BLOCK_SECTION * Section);
		void FillSectionInfo(BLOCK_SECTION * Section);
		void FixConstants (BLOCK_SECTION * Section, uint32_t Test, int * Changed);
		void GenerateSectionLinkage (BLOCK_SECTION * Section);
		uint32_t GenerateX86Code (BLOCK_SECTION * Section, uint32_t Test);
		uint32_t GetNewTestValue(void);
		uint32_t InheritParentInfo (BLOCK_SECTION * Section);
		void MarkCodeBlock (uint32_t PAddr);
		void SyncRegState (BLOCK_SECTION * Section, REG_INFO * SyncTo);
		void DetermineLoop(BLOCK_SECTION * Section, uint32_t Test, uint32_t Test2, uint32_t TestID);
		void FreeSection (BLOCK_SECTION * Section, BLOCK_SECTION * Parent);
		void ChangeDefaultRoundingModel(void);
		void CompileCop1Test (BLOCK_SECTION * Section);
		void RecompileTimerDone (void);

		#include "recompiler_ops.h"

		void ChangeFPURegFormat	   ( BLOCK_SECTION * Section, int32_t Reg, int32_t OldFormat, int32_t NewFormat, int32_t RoundingModel );
		void COMPILER_CALL ChangeMiIntrMask		 ( uint32_t mask );
		void ChangeMiModeReg		  ( void );
		void COMPILER_CALL ChangeSpStatus		   ( uint32_t status);
		void InitalizeR4300iRegisters ( void );
		uint32_t Is8BitReg				( int32_t x86Reg);
		void Load_FPR_ToTop		   ( BLOCK_SECTION * Section, int32_t Reg, int32_t RegToLoad, int32_t Format);
		void Map_GPR_32bit			( BLOCK_SECTION * Section, int32_t Reg, uint32_t SignValue, int32_t MipsRegToLoad );
		void Map_GPR_64bit			( BLOCK_SECTION * Section, int32_t Reg, int32_t MipsRegToLoad );
		int32_t  Map_TempReg			  ( BLOCK_SECTION * Section, int32_t x86Reg, int32_t MipsReg, uint32_t LoadHiWord );
		uint32_t RegInStack			   ( BLOCK_SECTION * Section, int32_t Reg, int32_t Format );
		void ProtectGPR			   ( BLOCK_SECTION * Section, uint32_t Reg );
		int32_t  StackPosition			( BLOCK_SECTION * Section, int32_t Reg );
		int UnMap_8BitTempReg (BLOCK_SECTION * Section);
		void UnMap_AllFPRs			( BLOCK_SECTION * Section );
		void UnMap_FPR				( BLOCK_SECTION * Section, int32_t Reg, int32_t WriteBackValue );
		void UnMap_GPR				( BLOCK_SECTION * Section, uint32_t Reg, int32_t WriteBackValue );
		uint32_t UnMap_X86reg			 ( BLOCK_SECTION * Section, uint32_t x86Reg );
		void UnProtectGPR			 ( BLOCK_SECTION * Section, uint32_t Reg );
		void WriteBackRegisters	   ( BLOCK_SECTION * Section );
		int32_t Free8BitX86Reg (BLOCK_SECTION * Section);
		int32_t FreeX86Reg (BLOCK_SECTION * Section);
		int UnMap_TempReg (BLOCK_SECTION * Section) ;
		int UnMap_TempRegSet (REG_INFO * RegWorking);
		void FixRandomReg();

		void COMPILER_CALL r4300i_DADDIU (void);
		void COMPILER_CALL r4300i_SDL (void);
		void COMPILER_CALL r4300i_SDR (void);
		void COMPILER_CALL r4300i_SPECIAL_DMULT (void);
		void COMPILER_CALL r4300i_SPECIAL_DDIV (void);
		void COMPILER_CALL r4300i_SPECIAL_DDIVU (void);
		void COMPILER_CALL R4300i_UnknownOpcode (void);
		void COMPILER_CALL r4300i_LDL2(void);
		void COMPILER_CALL r4300i_LDR2(void);


		// memory

		void Compile_LB				  ( int32_t Reg, uint32_t Addr, uint32_t SignExtend );
		void Compile_LH				  ( int32_t Reg, uint32_t Addr, uint32_t SignExtend );
		void Compile_LW				  ( int32_t Reg, uint32_t Addr );
		void Compile_SB_Const			( uint8_t Value, uint32_t Addr );
		void Compile_SB_Register		 ( int32_t x86Reg, uint32_t Addr );
		void Compile_SH_Const			( uint16_t Value, uint32_t Addr );
		void Compile_SH_Register		 ( int32_t x86Reg, uint32_t Addr );
		void Compile_SW_Const			( uint32_t Value, uint32_t Addr );
		void Compile_SW_Register		 ( int32_t x86Reg, uint32_t Addr );

		void StartCPU();
		void * RunCpu(uint32_t * Bytes);

		uint32_t LastTest;
		int32_t EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
		uint16_t FPU_RoundingMode;
		uint32_t BranchCompare;
		REG_INFO RegBeforeDelay;
		void COMPILER_CALL testy(int testy3, int testy4);
};


#else

#endif

#define		CALL_CPUFUNC(x) \
	PushImm32((uint32_t)dynamic_cast<Cpu*>(this)); \
	Call_Direct(GetAddress(0, &Cpu::x)); \
	AddConstToX86Reg(x86_ESP, 4); \



#endif
