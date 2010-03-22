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
#ifndef _CPU_H_
#define _CPU_H_

#ifdef WIN32
#define COMPILER_CALL	_cdecl
#else
#error not ready yet
#define COMPILER_CALL

#endif

#include <windows.h>
#include <stdarg.h>

#include "types.h"
#include "opcode.h"
#include "rsp.h"
#include "tlb.h"


typedef struct {
	int32_t DoSomething;
	int32_t CloseCPU;
	int32_t CheckInterrupts;
	int32_t DoInterrupt;
} CPU_ACTION;

#define MaxTimers				3
#define CompareTimer			0
#define ViTimer					1
#define AiTimer					2

typedef struct
{
	int32_t NextTimer[MaxTimers];
	int32_t Active[MaxTimers];
	int32_t CurrentTimerType;
	int32_t Timer;
} SYSTEM_TIMERS;



class Cpu
{
	public:
		Cpu(class Usf &);
		~Cpu();
		class Usf & usf;

		/* General functions */
		int32_t DelaySlotEffectsCompare (uint32_t PC, uint32_t Reg1, uint32_t Reg2);

		void COMPILER_CALL RunRsp (void) ;
		uint32_t LoadCpu(uint8_t * savestate, uint8_t * rom_pages[0x400]);

		/* Memory Functions */
		int32_t Allocate_Memory ( void );
		int32_t  r4300i_LB_NonMemory		 ( uint32_t PAddr, uint32_t * Value, uint32_t SignExtend );
		uint32_t r4300i_LB_VAddr			 ( uint32_t VAddr, uint8_t * Value );
		uint32_t r4300i_LD_VAddr			 ( uint32_t VAddr, uint64_t * Value );
		int32_t  r4300i_LH_NonMemory		 ( uint32_t PAddr, uint32_t * Value, int32_t SignExtend );
		uint32_t r4300i_LH_VAddr			 ( uint32_t VAddr, uint16_t * Value );
		int32_t  r4300i_LW_NonMemory		 ( uint32_t PAddr, uint32_t * Value );
		void r4300i_LW_PAddr			 ( uint32_t PAddr, uint32_t * Value );
		uint32_t r4300i_LW_VAddr			 ( uint32_t VAddr, uint32_t * Value );
		int32_t  r4300i_SB_NonMemory		 ( uint32_t PAddr, uint8_t Value );
		uint32_t r4300i_SB_VAddr			 ( uint32_t VAddr, uint8_t Value );
		uint32_t r4300i_SD_VAddr			( uint32_t VAddr, uint64_t Value );
		int32_t  r4300i_SH_NonMemory		 ( uint32_t PAddr, uint16_t Value );
		uint32_t r4300i_SH_VAddr			 ( uint32_t VAddr, uint16_t Value );
		int32_t  r4300i_SW_NonMemory		 ( uint32_t PAddr, uint32_t Value );
		uint32_t r4300i_SW_VAddr			 ( uint32_t VAddr, uint32_t Value );
		uint8_t * PageROM(uint32_t addr);
		void Release_Memory(void);
		int32_t r4300i_CPU_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP);

		/* TLb functions*/
		void SetupTLB_Entry (int Entry);
		void COMPILER_CALL WriteTLBEntry(int32_t index);
		uint32_t COMPILER_CALL AddressDefined(uintptr_t VAddr);
		void InitilizeTLB(void);
		void SetupTLB(void);
		void COMPILER_CALL TLB_Probe(void);
		void COMPILER_CALL TLB_Read(void);
		uint32_t COMPILER_CALL TranslateVaddr ( uintptr_t * Addr);


		/* Dma Functions */
		void COMPILER_CALL PI_DMA_READ  ( void );
		void COMPILER_CALL PI_DMA_WRITE ( void );
		void COMPILER_CALL SI_DMA_READ  ( void );
		void COMPILER_CALL SI_DMA_WRITE ( void );
		void COMPILER_CALL SP_DMA_READ  ( void );
		void COMPILER_CALL SP_DMA_WRITE ( void );

		/* Exceptions Functions */

		void COMPILER_CALL CheckInterrupts		( void );
		void COMPILER_CALL DoAddressError		 ( uint32_t DelaySlot, uint32_t BadVaddr, uint32_t FromRead );
		void COMPILER_CALL DoBreakException	   ( uint32_t DelaySlot );
		void COMPILER_CALL DoCopUnusableException ( uint32_t DelaySlot, uint32_t Coprocessor );
		void COMPILER_CALL DoIntrException		( uint32_t DelaySlot );
		void COMPILER_CALL DoTLBMiss			  ( uint32_t DelaySlot, uint32_t BadVaddr );
		void COMPILER_CALL DoSysCallException		( uint32_t DelaySlot);

		/* Cpu Functions */
		void COMPILER_CALL ChangeCompareTimer ( void );
		void COMPILER_CALL ChangeTimer		( int32_t Type, int32_t Value );
		void COMPILER_CALL CheckTimer		 ( void );
		void COMPILER_CALL CloseCpu		   ( void );
		void COMPILER_CALL TimerDone		  ( void );
		void COMPILER_CALL InPermLoop		 ( void );
		void COMPILER_CALL DoSomething		( void );
		uint32_t Machine_LoadStateFromRAM(void * savestatespace);
		void COMPILER_CALL RefreshScreen(void);
		void COMPILER_CALL StopEmulation(void);

		/* Register Functions */
		void SetupRegisters(N64_REGISTERS * n64_Registers);
		void COMPILER_CALL SetFpuLocations( void );
		void COMPILER_CALL UpdateCurrentHalfLine(void);

		class Audio * audio;
		class Rsp rsp;

		/* Cpu & Registers */
		N64_REGISTERS * Registers;
		uint32_t PROGRAM_COUNTER, *CP0, *FPCR, *RegRDRAM, *RegSP, *RegDPC, *RegMI;
		uint32_t *RegVI, *RegAI, *RegPI, *RegRI,*RegSI, HalfLine, RegModValue;
		uint32_t ViFieldNumber, LLBit, LLAddr;
		void * FPRDoubleLocation[32], * FPRFloatLocation[32];
		MIPS_DWORD *GPR, *FPR, HI, LO;
		int32_t fpuControl;

		uint32_t NextInstruction, JumpToLocation, AudioIntrReg;
		CPU_ACTION * CPU_Action;
		SYSTEM_TIMERS * Timers;
		OPCODE Opcode;
		uint32_t CPURunning, SPHack;
		uint32_t * WaitMode;
		uint32_t TimerHz, TimerV;
		uint32_t CpuPerc, LastCount;

		uint8_t AudioBuffer[262144];

		N64_Blocks_t N64_Blocks;
		uint32_t * TLBLoadAddress;
		BLOCK_INFO BlockInfo;

		/* Memory */
		uint8_t * N64MEM, *RDRAM, *DMEM, *IMEM, ** ROMPages;
		void ** JumpTable, ** DelaySlotTable;
		//uint8_t * RecompCode;
		uint32_t WrittenToRom, MemoryState, WroteToRom, TempValue;

		uintptr_t * TLB_Map;
		uint32_t RdramSize, RomFileSize;

		unsigned long OLD_VI_V_SYNC_REG, VI_INTR_TIME;

		FASTTLB FastTlb[64];
		TLB tlb[32];

		uint8_t * PIF_Ram;

		int32_t  GetCicChipID ( int8_t * RomData );
		int32_t  LoadPifRom   ( int32_t country );
		void PifRamWrite  ( void );
		void PifRamRead   ( void );
		void ProcessControllerCommand ( int32_t Control, int8_t * Command);

		void MapReg(uint32_t source, uint32_t dest);
		uint32_t UnmapReg(uint32_t source);

		virtual void StartCPU() = 0;
		virtual void * RunCpu(uint32_t * Bytes) = 0;

};

#include "registers.h"
#include "interpreter_cpu.h"
#include "tlb.h"

#include "exception.h"


#define NORMAL					0
#define DO_DELAY_SLOT			1
#define DO_END_DELAY_SLOT		2
#define DELAY_SLOT				3
#define END_DELAY_SLOT			4
#define LIKELY_DELAY_SLOT		5
#define JUMP	 				6
#define DELAY_SLOT_DONE			7
#define LIKELY_DELAY_SLOT_DONE	8
#define END_BLOCK 				9

enum SaveType {
	Auto,
	Eeprom_4K,
	Eeprom_16K,
	Sram,
	FlashRam
};

#endif
