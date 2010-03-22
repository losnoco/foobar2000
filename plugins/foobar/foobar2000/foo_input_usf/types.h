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
#ifndef __Types_h
#define __Types_h

#include <stdint.h>
typedef uint64_t QWORD;

typedef union tagVect {
	double				FD[2];
	int64_t				DW[2];
	uint64_t			UDW[2];
	int32_t				W[4];
	float				FS[4];
	uint32_t			UW[4];
	int16_t				HW[8];
	uint16_t			UHW[8];
	int8_t				B[16];
	uint8_t				UB[16];
} VECTOR;

typedef union tagUWORD {
	int32_t		W;
	uint32_t	UW;
	int16_t		HW[2];
	uint16_t	UHW[2];
	int8_t		B[4];
	uint8_t		UB[4];
	float F;
} MIPS_WORD;

typedef union tagUDWORD {
	double				D;
	int64_t				DW;
	uint64_t			UDW;
	int32_t				W[2];
	uint32_t			UW[2];
	int16_t				HW[4];
	uint16_t			UHW[4];
	int8_t				B[8];
	uint8_t				UB[8];
	float 				F[2];
} MIPS_DWORD;

typedef MIPS_WORD	MIPSUWORD;
typedef MIPS_DWORD	MIPSUDWORD;

typedef struct
{
	uint32_t	  PROGRAM_COUNTER;
	MIPS_DWORD	GPR[32];
	MIPS_DWORD	FPR[32];
	uint32_t	  CP0[33];
	uint32_t	  FPCR[32];
	MIPS_DWORD	  HI;
	MIPS_DWORD 	  LO;
	uint32_t	  RDRAM[10];
	uint32_t	  SP[10];
	uint32_t	  DPC[10];
	uint32_t	  MI[4];
	uint32_t	  VI[14];
	uint32_t	  AI[6];
	uint32_t	  PI[13];
	uint32_t	  RI[8];
	uint32_t	  SI[4];
	int8_t	   PIF_Ram[0x40];
} N64_REGISTERS;


typedef struct {
	uint32_t NoOfRDRamBlocks[2048];
	uint32_t NoOfDMEMBlocks;
	uint32_t NoOfIMEMBlocks;
	uint32_t NoOfPifRomBlocks;
} N64_Blocks_t;


typedef struct {
	//r4k
	int32_t			MIPS_RegState[32];
	MIPS_DWORD		MIPS_RegVal[32];

	uint32_t		x86reg_MappedTo[64];
	uint32_t		x86reg_MapOrder[64];
	uint32_t		x86reg_Protected[64];

	uint32_t		CycleCount;
	uint32_t		RandomModifier;

	//FPU
	uint32_t		Stack_TopPos;
	uint32_t		x86fpu_MappedTo[16];
	uint32_t		x86fpu_State[16];
	uint32_t		x86fpu_RoundingModel[16];

	uint32_t		Fpu_Used;
	uint32_t	   RoundingModel;
} REG_INFO;

typedef struct {
	uint32_t		TargetPC;
	//uint8_t *		BranchLabel;
	uint8_t *		LinkLocation;
	uint8_t *		LinkLocation2;
	uint32_t		FallThrough;
	uint32_t		PermLoop;
	uint32_t		DoneDelaySlot;
	REG_INFO	RegSet;
} JUMP_INFO;

typedef struct {
	/* Block Connection info */
	void **		ParentSection;
	void *		ContinueSection;
	void *		JumpSection;
	uint8_t *		CompiledLocation;


	uint32_t		SectionID;
	uint32_t		Test;
	uint32_t		Test2;
	uint32_t		InLoop;

	uint32_t		StartPC;
	uint32_t		CompilePC;

	/* Register Info */
	REG_INFO	RegStart;
	REG_INFO	RegWorking;

	/* Jump Info */
	JUMP_INFO   Jump;
	JUMP_INFO   Cont;

	uint8_t * N64RAM;

} BLOCK_SECTION;

typedef struct {
	BLOCK_SECTION * Parent;
	JUMP_INFO	 * JumpInfo;
} BLOCK_PARENT;

typedef struct {
	uint32_t	TargetPC;
	REG_INFO ExitRegSet;
	int32_t	  reason;
	int32_t	  NextInstruction;
	uint8_t *   JumpLoc; //32bit jump
} EXIT_INFO;

typedef struct {
	uint32_t	 	  StartVAddr;
	uint8_t *		  CompiledLocation;
	int32_t		   NoOfSections;
	BLOCK_SECTION BlockInfo;
	EXIT_INFO  ** ExitInfo;
	int32_t		   ExitCount;
} BLOCK_INFO;

typedef struct {
	void * CodeBlock;
	QWORD  OriginalMemory;
} TARGET_INFO;

typedef struct {
	uint32_t PAddr;
	uint32_t VAddr;
	uint32_t OriginalValue;
	void * CompiledLocation;
} ORIGINAL_MEMMARKER;





#endif
