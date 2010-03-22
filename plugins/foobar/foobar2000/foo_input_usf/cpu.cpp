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

#include <stdint.h>

#include "main.h"
#include "usf.h"
#include "audio_hle.h"
#include "recompiler_cpu.h"
#include "audio.h"
#include "rsp.h"
#include "cpu.h"

Cpu::Cpu(Usf & mUsf) : usf(mUsf), rsp(*this), audio(NULL)
{
	printf("called the Cpu constructor\n");

	RdramSize = 0x800000;
	RomFileSize = 0x4000000;
	NextInstruction = NORMAL;
	JumpToLocation = AudioIntrReg = 0;
	CPU_Action = NULL;
	Timers = NULL;
	CPURunning = SPHack = 0;
	WaitMode = NULL;
	CpuPerc = 0;
	OLD_VI_V_SYNC_REG = 0;
	VI_INTR_TIME = 500000;
	HalfLine = 0;

	WrittenToRom = MemoryState = WroteToRom = TempValue = 0;


	LLBit = LLAddr = 0;

	Allocate_Memory();
}

Cpu::~Cpu()
{
	Release_Memory();
	//delete audio;
}

void Cpu::ChangeCompareTimer(void) {
	uint32_t NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) {  NextCompare = 0x7FFFFFFF; }
	if (NextCompare == 0) { NextCompare = 0x1; }
	ChangeTimer(CompareTimer,NextCompare);
}

void Cpu::ChangeTimer(int32_t Type, int32_t Value) {
	if (Value == 0) {
		Timers->NextTimer[Type] = 0;
		Timers->Active[Type] = 0;
		return;
	}
	Timers->NextTimer[Type] = Value - Timers->Timer;
	Timers->Active[Type] = 1;
	CheckTimer();
}

void Cpu::CheckTimer (void) {
	int32_t count;

	for (count = 0; count < MaxTimers; count++) {
		if (!Timers->Active[count]) { continue; }
		if (!(count == CompareTimer && Timers->NextTimer[count] == 0x7FFFFFFF)) {
			Timers->NextTimer[count] += Timers->Timer;
		}
	}
	Timers->CurrentTimerType = -1;
	Timers->Timer = 0x7FFFFFFF;
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers->Active[count]) { continue; }
		if (Timers->NextTimer[count] >= Timers->Timer) { continue; }
		Timers->Timer = Timers->NextTimer[count];
		Timers->CurrentTimerType = count;
	}
	if (Timers->CurrentTimerType == -1) {
		DisplayError("No active timers ???\nEmulation Stoped");
		StopEmulation();
	}
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers->Active[count]) { continue; }
		if (!(count == CompareTimer && Timers->NextTimer[count] == 0x7FFFFFFF)) {
			Timers->NextTimer[count] -= Timers->Timer;
		}
	}

	if (Timers->NextTimer[CompareTimer] == 0x7FFFFFFF) {
		uint32_t NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
		if ((NextCompare & 0x80000000) == 0 && NextCompare != 0x7FFFFFFF) {
			ChangeCompareTimer();
		}
	}
}

void Cpu::CloseCpu (void) {
	uint32_t ExitCode, count, OldProtect;

	if (!CPURunning) { return; }


	for (count = 0; count < 20; count ++ )
	{
		CPU_Action->CloseCPU = 1;
		CPU_Action->DoSomething = 1;
		UsfSleep(100);
		StopEmulation();
	}

	CPURunning = 0;

}

int32_t Cpu::DelaySlotEffectsCompare (uint32_t PC, uint32_t Reg1, uint32_t Reg2) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(PC + 4, (uint32_t*)&Command.Hex)) {
		return 1;
	}

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_SLL:
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (Command.rd == 0) { return 0; }
			if (Command.rd == Reg1) { return 1; }
			if (Command.rd == Reg2) { return 1; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			break;
		default:
			return 1;
		}
		break;
	case R4300i_CP0:
		switch (Command.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (Command.rt == 0) { return 0; }
			if (Command.rt == Reg1) { return 1; }
			if (Command.rt == Reg2) { return 1; }
			break;
		default:
			if ( (Command.rs & 0x10 ) != 0 ) {
				switch( Opcode.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default:
					return 1;
				}
				return 1;
			}
		}
		break;
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_MF:
			if (Command.rt == 0) { return 0; }
			if (Command.rt == Reg1) { return 1; }
			if (Command.rt == Reg2) { return 1; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
			return 1;
		}
		break;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (Command.rt == 0) { return 0; }
		if (Command.rt == Reg1) { return 1; }
		if (Command.rt == Reg2) { return 1; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	default:

		return 1;
	}
	return 0;
}

void Cpu::DoSomething ( void ) {
	if (CPU_Action->CloseCPU) {
		StopEmulation();
	}
	if (CPU_Action->CheckInterrupts) {
		CPU_Action->CheckInterrupts = 0;
		CheckInterrupts();
	}
	if (CPU_Action->DoInterrupt) {
		CPU_Action->DoInterrupt = 0;
		DoIntrException(0);
	}


	CPU_Action->DoSomething = 0;

	if (CPU_Action->DoInterrupt) { CPU_Action->DoSomething = 1; }
}

void Cpu::InPermLoop (void) {
	// *** Changed ***/
	if (CPU_Action->DoInterrupt) { return; }

	/* Interrupts enabled */
	if (( STATUS_REGISTER & STATUS_IE  ) == 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_EXL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_ERL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & 0xFF00) == 0) { goto InterruptsDisabled; }

	/* check sound playing */

	/* check RSP running */
	/* check RDP running */
	if (Timers->Timer > 0) {
		COUNT_REGISTER += Timers->Timer + 1;
		Timers->Timer = -1;
	}
	return;

InterruptsDisabled:
	_asm int 3
	DisplayError("Stuck in Permanent Loop");
	StopEmulation();
}

void ReadFromMem(const void * source, void * target, uint32_t length, uint32_t *offset) {
	memcpy((uint8_t*)target,((uint8_t*)source)+*offset,length);
	*offset+=length;
}


uint32_t Cpu::Machine_LoadStateFromRAM(void * savestatespace)
{
	uint8_t LoadHeader[0x40];
	uint32_t Value, count, SaveRDRAMSize, offset=0;

	ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
	if (Value != 0x23D8A6C8) { return 0; }
	ReadFromMem( savestatespace,&SaveRDRAMSize,sizeof(SaveRDRAMSize),&offset);
	ReadFromMem( savestatespace,&LoadHeader,0x40,&offset);

	Timers->CurrentTimerType = -1;
	Timers->Timer = 0;
	for (count = 0; count < MaxTimers; count ++) { Timers->Active[count] = 0; }

	//fix rdram size
	if (SaveRDRAMSize != RdramSize)
	{
		savestatespace = realloc(savestatespace, 0x40275c);
		VirtualFree(N64MEM, 0, MEM_DECOMMIT);
		VirtualAlloc(N64MEM, 0x00400000, MEM_COMMIT, PAGE_READWRITE);
		VirtualFree(JumpTable, 0, MEM_RELEASE);
		JumpTable = (void **) VirtualAlloc( 0, 0x400000, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}

	RdramSize = SaveRDRAMSize;

	ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
	ChangeTimer(ViTimer,Value);
	ReadFromMem( savestatespace,&PROGRAM_COUNTER,sizeof(PROGRAM_COUNTER),&offset);
	ReadFromMem( savestatespace,GPR,sizeof(int64_t)*32,&offset);
	ReadFromMem( savestatespace,FPR,sizeof(int64_t)*32,&offset);
	ReadFromMem( savestatespace,CP0,sizeof(uint32_t)*32,&offset);
	ReadFromMem( savestatespace,FPCR,sizeof(uint32_t)*32,&offset);
	ReadFromMem( savestatespace,&HI,sizeof(int64_t),&offset);
	ReadFromMem( savestatespace,&LO,sizeof(int64_t),&offset);
	ReadFromMem( savestatespace,RegRDRAM,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,RegSP,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,RegDPC,sizeof(uint32_t)*10,&offset);
	ReadFromMem( savestatespace,RegMI,sizeof(uint32_t)*4,&offset);
	ReadFromMem( savestatespace,RegVI,sizeof(uint32_t)*14,&offset);
	ReadFromMem( savestatespace,RegAI,sizeof(uint32_t)*6,&offset);
	ReadFromMem( savestatespace,RegPI,sizeof(uint32_t)*13,&offset);
	ReadFromMem( savestatespace,RegRI,sizeof(uint32_t)*8,&offset);
	ReadFromMem( savestatespace,RegSI,sizeof(uint32_t)*4,&offset);
	ReadFromMem( savestatespace,tlb,sizeof(TLB)*32,&offset);
	ReadFromMem( savestatespace,(uint8_t*)PIF_Ram,0x40,&offset);
	ReadFromMem( savestatespace,RDRAM,SaveRDRAMSize,&offset);
	ReadFromMem( savestatespace,DMEM,0x1000,&offset);
	ReadFromMem( savestatespace,IMEM,0x1000,&offset);

	CP0[32] = 0;

	SetupTLB();
	ChangeCompareTimer();
	AI_STATUS_REG = 0;

	switch (LoadHeader[0x3D])
	{
		case 0x44: //Germany
		case 0x46: //french
		case 0x49: //Italian
		case 0x50: //Europe
		case 0x53: //Spanish
		case 0x55: //Australia
		case 0x58: // X (PAL)
		case 0x59: // X (PAL)
			TimerV = 49656530;
			TimerHz = 50;
			*(int*)(RDRAM + 0x300) = 0;
			break;
		case 0x37: // 7 (Beta)
		case 0x41: // A (NTSC)
		case 0x45: //USA
		case 0x4A: //Japan
		default:
			TimerV = 48681812;
			TimerHz = 60;
			*(int*)(RDRAM + 0x300) = 1;
		break;
	}

	SetFpuLocations(); // important if FR=1
	CheckInterrupts();

	return 1;
}

uint32_t Cpu::LoadCpu(uint8_t * savestate, uint8_t * rom_pages[0x400])
{
	uint32_t ThreadID, count;

	ROMPages = rom_pages;

	CloseCpu();

	memset(CPU_Action,0,sizeof(CPU_Action));
	memset(Timers,0,sizeof(SYSTEM_TIMERS));
	WrittenToRom = 0;

	InitilizeTLB();

	SetupRegisters(Registers);

	Timers->CurrentTimerType = -1;
	Timers->Timer = 0;

	for (count = 0; count < MaxTimers; count ++) { Timers->Active[count] = 0; }
	ChangeTimer(ViTimer, 5000);
	ChangeCompareTimer();
	ViFieldNumber = 0;
	CPURunning = 1;
	*WaitMode = 0;

	Machine_LoadStateFromRAM(savestate);

	audio = new Audio(*this);

	audio->AiDacrateChanged(AI_DACRATE_REG);

	if(usf.EnableFIFOFull)
	{
		const float VSyncTiming = 789000.0f;
		double BytesPerSecond = (double)TimerV / (AI_DACRATE_REG + 1) * 4;
		double CountsPerSecond = (double)(((double)VSyncTiming) * (double)TimerHz);
		double CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
		unsigned int IntScheduled = (unsigned int)((double)AI_LEN_REG * CountsPerByte);

		ChangeTimer(AiTimer, IntScheduled);
		AI_STATUS_REG |= 0x40000000;
	}

	rsp.InitRsp();

	return 0;
}


void Cpu::RefreshScreen (void )
{

	if (OLD_VI_V_SYNC_REG != VI_V_SYNC_REG) {
		if (VI_V_SYNC_REG == 0) {
			VI_INTR_TIME = 500000;
		} else {
			VI_INTR_TIME = (VI_V_SYNC_REG + 1) * 1500;
			if ((VI_V_SYNC_REG % 1) != 0) {
				VI_INTR_TIME -= 38;
			}
		}
	}

	ChangeTimer(ViTimer,Timers->Timer + Timers->NextTimer[ViTimer] + VI_INTR_TIME);

	if ((VI_STATUS_REG & 0x10) != 0) {
		if (ViFieldNumber == 0) {
			ViFieldNumber = 1;
		} else {
			ViFieldNumber = 0;
		}
	} else {
		ViFieldNumber = 0;
	}

}

void Cpu::RunRsp (void) {
	if((SP_STATUS_REG & SP_STATUS_HALT) == 0)
	{
		if((SP_STATUS_REG & SP_STATUS_BROKE) == 0)
		{
			uint32_t Task = *( unsigned int *)(DMEM + 0xFC0);
			int32_t hle = -1;
			switch (Task)
			{
				case 1:
					MI_INTR_REG |= 0x20;

					SP_STATUS_REG |= (0x0203 );
					if ((SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0 )
						MI_INTR_REG |= 1;

					CheckInterrupts();

					DPC_STATUS_REG &= ~0x0002;
					return;
					break;

				case 2:

					if(usf.AudioHle && !(usf.Seeking && usf.FastSeek))
					{
						OSTask_t * task = (OSTask_t*)(DMEM + 0xFC0);
						hle = audio->audio_ucode(task);
					}

					SP_STATUS_REG |= 0x0203 ;
					if((SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
					{
						MI_INTR_REG |= 1;
						CheckInterrupts();
					}

					if(hle && !(usf.Seeking && usf.FastSeek))
						rsp.Run(100);

					break;
				default:
					break;
			}
		}
	}
}


void Cpu::TimerDone (void) {
	switch (Timers->CurrentTimerType) {
	case CompareTimer:
		if(usf.EnableCompare)
			FAKE_CAUSE_REGISTER |= CAUSE_IP7;
		CheckInterrupts();
		ChangeCompareTimer();
		break;
	case ViTimer:
		RefreshScreen();
		MI_INTR_REG |= MI_INTR_VI;
		CheckInterrupts();
		//CompileCheckInterrupts();
		*WaitMode = 0;
		break;
	case AiTimer:
		ChangeTimer(AiTimer,0);
		AI_STATUS_REG = 0;
		AudioIntrReg |= 4;
		CheckInterrupts();
		break;
	}
	CheckTimer();
}

#ifdef USEX64
void Recompiler::RecompileTimerDone(void) {
	uint8_t * Jump, * Jump2, *Jump3 = 0, * ExitJump1 = 0, * ExitJump2 = 0;
	//Int3();
	//BreakPoint();
	Pushad();
	Push(x86_EAX);

	CompConstToVariable(CompareTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	if(usf.EnableCompare) {
		OrConstToVariable(CAUSE_IP7, &FAKE_CAUSE_REGISTER);
	}

	Call_Direct(ChangeCompareTimer);


	JmpLabel32(0);
	ExitJump1 = RecompPos - 4;

	// ****************************************************************************
	SetJump8(Jump, RecompPos);
	CompConstToVariable(ViTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	Call_Direct(RefreshScreen);

	OrConstToVariable(MI_INTR_VI, &MI_INTR_REG);

	Call_Direct(CheckInterrupts);


	MoveConstToVariable(0, WaitMode);


	JmpLabel32(0);
	ExitJump2 = RecompPos - 4;

	// ****************************************************************************
	SetJump8(Jump, RecompPos);
	CompConstToVariable(AiTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	Push(x86_ECX);
	Push(x86_EDX);
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	MoveConstToX86reg(AiTimer, x86_ECX);
	Call_Direct(ChangeTimer);
	MoveConstToVariable(0, &AI_STATUS_REG);
	OrConstToVariable(4, &AudioIntrReg);
	Pop(x86_ECX);
	Pop(x86_EDX);


	// ****************************************************************************

	SetJump8(Jump, RecompPos);
	SetJump32(ExitJump1, RecompPos);
	SetJump32(ExitJump2, RecompPos);

	Call_Direct(CheckTimer);

	Pop(x86_EAX);
	Popad();
	//MoveVariableToX86reg(&Timers->CurrentTimerType, x86_R8);
	//CompConstToX86reg(x86_R8, CompareTimer);
}


void RecompileTimerDone(void) {
	uint8_t * Jump, * Jump2, *Jump3 = 0, * ExitJump1 = 0, * ExitJump2 = 0;
	//Int3();
	//BreakPoint();
	Pushad();
	Push(x86_EAX);

	CompConstToVariable(CompareTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	if(usf.EnableCompare) {
		OrConstToVariable(CAUSE_IP7, &FAKE_CAUSE_REGISTER);
	}

	Call_Direct(ChangeCompareTimer);


	JmpLabel32(0);
	ExitJump1 = RecompPos - 4;

	// ****************************************************************************
	SetJump8(Jump, RecompPos);
	CompConstToVariable(ViTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	Call_Direct(RefreshScreen);

	OrConstToVariable(MI_INTR_VI, &MI_INTR_REG);

	Call_Direct(CheckInterrupts);


	MoveConstToVariable(0, WaitMode);


	JmpLabel32(0);
	ExitJump2 = RecompPos - 4;

	// ****************************************************************************
	SetJump8(Jump, RecompPos);
	CompConstToVariable(AiTimer, &Timers->CurrentTimerType);
	JneLabel8(0);
	Jump = RecompPos - 1;

	Push(x86_ECX);
	Push(x86_EDX);
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	MoveConstToX86reg(AiTimer, x86_ECX);
	Call_Direct(ChangeTimer);
	MoveConstToVariable(0, &AI_STATUS_REG);
	OrConstToVariable(4, &AudioIntrReg);
	Pop(x86_ECX);
	Pop(x86_EDX);

	// ****************************************************************************

	SetJump8(Jump, RecompPos);
	SetJump32(ExitJump1, RecompPos);
	SetJump32(ExitJump2, RecompPos);

	Call_Direct(CheckTimer);

	Pop(x86_EAX);
	Popad();
	//MoveVariableToX86reg(&Timers->CurrentTimerType, x86_R8);
	//CompConstToX86reg(x86_R8, CompareTimer);
}

void RecompileDoSomething(void) {
	uint8_t * Jump;

	CompConstToVariable(0, &CPU_Action->DoInterrupt);
	JeLabel8(0);
	Jump = RecompPos - 1;


	MoveConstToVariable(0, &CPU_Action->DoInterrupt);
	Push(x86_RCX);
	XorX86RegToX86Reg(x86_RCX, x86_RCX);
	CompileDoIntrException();
	Pop(x86_RCX);

	SetJump8(Jump, RecompPos);

	MoveConstToVariable(0, &CPU_Action->DoSomething);

	CompConstToVariable(0, &CPU_Action->DoInterrupt);
	JeLabel8(0);
	Jump = RecompPos - 1;

	MoveConstToVariable(1, &CPU_Action->DoSomething);

	SetJump8(Jump, RecompPos);

}

#else

void Int3() {
	_asm int 3
}

void DoBswap(int * number) {
	int number2 = *number;
	_asm {
		pushad
		mov eax, number2
		bswap eax
		mov number2, eax
		popad
	}
	*number = number2;
}

void _Emms() {
	_asm emms
}


#endif


