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
#include <windows.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "cpu.h"
#include "audio.h"
#include "rsp.h"


static uint8_t EmptySpace = 0;

inline uint8_t * Cpu::PageROM(uint32_t addr)
{
	return (ROMPages[addr/0x10000])?ROMPages[addr/0x10000]+(addr%0x10000):&EmptySpace;
}

int32_t Cpu::Allocate_Memory ( void ) {
	uint32_t i = 0;

	// Allocate the N64MEM and TLB_Map so that they are in each others 4GB range
	// Also put the registers there :) (x86_64 only)

	TLB_Map = (uintptr_t *) VirtualAlloc( NULL, (0x100000 * sizeof(uintptr_t)) + 0x40000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//VirtualAlloc( TLB_Map, (0x100000 * sizeof(uintptr_t)) + 0x840000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (TLB_Map == NULL)
	{
		return 0;
	}

	memset(TLB_Map, 0, (0x100000 * sizeof(uintptr_t)) + 0x40000);

	N64MEM = (unsigned char *) VirtualAlloc( NULL, 0x820000, MEM_RESERVE, PAGE_READWRITE );
	VirtualAlloc(N64MEM, 0x00800000, MEM_COMMIT, PAGE_READWRITE);
	if(N64MEM==NULL) {
		DisplayError("Failed to allocate N64MEM");
		return 0;
	}

	memset(N64MEM,0,0x800000);

	Registers = (N64_REGISTERS*)((uintptr_t)TLB_Map + (0x100000 * sizeof(uintptr_t)) + 0x1000);
	TLBLoadAddress = (uint32_t *)((uintptr_t)Registers + 0x600);
	Timers = (SYSTEM_TIMERS *)(TLBLoadAddress + 4);
	WaitMode = (uint32_t *)(Timers + sizeof(SYSTEM_TIMERS));
	CPU_Action = (CPU_ACTION *)(WaitMode + 4);
	rsp.RSP_GPR = (MIPSUWORD *)((MIPS_WORD *)CPU_Action + sizeof(CPU_ACTION));
	DMEM = (uint8_t *)(rsp.RSP_GPR + (32 * 8));
	rsp.RSP_ACCUM = (MIPSUDWORD *)(DMEM + 0x2000);
	rsp.RSP_Vect = (VECTOR *)((char*)rsp.RSP_ACCUM + (sizeof(REGISTER)*32));

	JumpTable = (void **) VirtualAlloc( 0, 0x800000, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if( JumpTable == NULL )
		return 0;

	memset(JumpTable, 0, 0x800000);

	(void **)DelaySlotTable =   (void **) VirtualAlloc( 0, ((0x800000) >> 0xA), MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if( DelaySlotTable == NULL )
		return 0;

	memset(DelaySlotTable, 0, ((0x800000) >> 0xA) );

	RDRAM = (uint8_t *)(N64MEM);
	IMEM  = DMEM + 0x1000;

	MemoryState = 1;

	return 1;
}

#define NormalCompileBufferSize	 0x1400000

void Recompiler::InitRecompilerMemory()
{
	RecompCode = (BYTE *) VirtualAlloc(0, (NormalCompileBufferSize), MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	memset(RecompCode, 0, NormalCompileBufferSize);
}

int32_t Cpu::r4300i_LB_NonMemory ( uint32_t PAddr, uint32_t * Value, uint32_t SignExtend ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000)
	{
		if (WrittenToRom) { return 0; }
		if ((PAddr & 2) == 0) { PAddr = (PAddr + 4) ^ 2; }
		if ((PAddr - 0x10000000) < RomFileSize) {
			if (SignExtend) {
				*Value = (char)*PageROM((PAddr - 0x10000000)^3);

			} else {
				*Value = *PageROM((PAddr - 0x10000000)^3);
			}
			return 1;
		} else {
			*Value = 0;
			return 0;
		}
	}

	switch (PAddr & 0xFFF00000) {
	default:
		* Value = 0;
		return 0;
		break;
	}
	return 1;
}

uint32_t Cpu::r4300i_LB_VAddr ( uint32_t VAddr, uint8_t * Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*Value = *(uint8_t *)(TLB_Map[VAddr >> 12] + (VAddr ^ 3));
	return 1;
}

uint32_t Cpu::r4300i_LD_VAddr ( uint32_t VAddr, uint64_t * Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*((uint32_t *)(Value) + 1) = *(uint32_t *)(TLB_Map[VAddr >> 12] + VAddr);
	*((uint32_t *)(Value)) = *(uint32_t *)(TLB_Map[VAddr >> 12] + VAddr + 4);
	return 1;
}

int32_t Cpu::r4300i_LH_NonMemory ( uint32_t PAddr, uint32_t * Value, int32_t SignExtend ) {
	switch (PAddr & 0xFFF00000) {
	default:
		* Value = 0;
		return 0;
		break;
	}
	return 1;
}

uint32_t Cpu::r4300i_LH_VAddr ( uint32_t VAddr, uint16_t * Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*Value = *(uint16_t *)(TLB_Map[VAddr >> 12] + (VAddr ^ 2));
	return 1;
}

int32_t Cpu::r4300i_LW_NonMemory ( uint32_t PAddr, uint32_t * Value ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if (WrittenToRom) {
			*Value = WroteToRom;
			//LogMessage("%X: Read crap from Rom %X from %X",PROGRAM_COUNTER,*Value,PAddr);
			WrittenToRom = 0;
			return 1;
		}

		if ((PAddr - 0x10000000) < RomFileSize) {
			*Value = *(uint32_t *)PageROM((PAddr - 0x10000000));
			return 1;
		} else {
			*Value = PAddr & 0xFFFF;
			*Value = (*Value << 16) | *Value;
			return 0;
		}
	}

	switch (PAddr & 0xFFF00000) {
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: * Value = RDRAM_CONFIG_REG; break;
		case 0x03F00004: * Value = RDRAM_DEVICE_ID_REG; break;
		case 0x03F00008: * Value = RDRAM_DELAY_REG; break;
		case 0x03F0000C: * Value = RDRAM_MODE_REG; break;
		case 0x03F00010: * Value = RDRAM_REF_INTERVAL_REG; break;
		case 0x03F00014: * Value = RDRAM_REF_ROW_REG; break;
		case 0x03F00018: * Value = RDRAM_RAS_INTERVAL_REG; break;
		case 0x03F0001C: * Value = RDRAM_MIN_INTERVAL_REG; break;
		case 0x03F00020: * Value = RDRAM_ADDR_SELECT_REG; break;
		case 0x03F00024: * Value = RDRAM_DEVICE_MANUF_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04000000:
		switch (PAddr) {
		case 0x04040010: *Value = SP_STATUS_REG; break;
		case 0x04040014: *Value = SP_DMA_FULL_REG; break;
		case 0x04040018: *Value = SP_DMA_BUSY_REG; break;
		case 0x04080000: *Value = SP_PC_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x0410000C: *Value = DPC_STATUS_REG; break;
		case 0x04100010: *Value = DPC_CLOCK_REG; break;
		case 0x04100014: *Value = DPC_BUFBUSY_REG; break;
		case 0x04100018: *Value = DPC_PIPEBUSY_REG; break;
		case 0x0410001C: *Value = DPC_TMEM_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		* Value = 0;
		return 1;
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000: * Value = MI_MODE_REG; break;
		case 0x04300004: * Value = MI_VERSION_REG; break;
		case 0x04300008: * Value = MI_INTR_REG; break;
		case 0x0430000C: * Value = MI_INTR_MASK_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04400000:
		switch (PAddr) {
		case 0x04400000: *Value = VI_STATUS_REG; break;
		case 0x04400004: *Value = VI_ORIGIN_REG; break;
		case 0x04400008: *Value = VI_WIDTH_REG; break;
		case 0x0440000C: *Value = VI_INTR_REG; break;
		case 0x04400010:
			*Value = 0;
			//UpdateCurrentHalfLine();
			//*Value = HalfLine;
			break;
		case 0x04400014: *Value = VI_BURST_REG; break;
		case 0x04400018: *Value = VI_V_SYNC_REG; break;
		case 0x0440001C: *Value = VI_H_SYNC_REG; break;
		case 0x04400020: *Value = VI_LEAP_REG; break;
		case 0x04400024: *Value = VI_H_START_REG; break;
		case 0x04400028: *Value = VI_V_START_REG ; break;
		case 0x0440002C: *Value = VI_V_BURST_REG; break;
		case 0x04400030: *Value = VI_X_SCALE_REG; break;
		case 0x04400034: *Value = VI_Y_SCALE_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04500000:
		switch (PAddr) {
		case 0x04500004: *Value = audio->AiReadLength(); break;
		case 0x0450000C: *Value = AI_STATUS_REG; break;
		default:
			* Value = 0;
			return 1;
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600010: *Value = PI_STATUS_REG; break;
		case 0x04600014: *Value = PI_DOMAIN1_REG; break;
		case 0x04600018: *Value = PI_BSD_DOM1_PWD_REG; break;
		case 0x0460001C: *Value = PI_BSD_DOM1_PGS_REG; break;
		case 0x04600020: *Value = PI_BSD_DOM1_RLS_REG; break;
		case 0x04600024: *Value = PI_DOMAIN2_REG; break;
		case 0x04600028: *Value = PI_BSD_DOM2_PWD_REG; break;
		case 0x0460002C: *Value = PI_BSD_DOM2_PGS_REG; break;
		case 0x04600030: *Value = PI_BSD_DOM2_RLS_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: * Value = RI_MODE_REG; break;
		case 0x04700004: * Value = RI_CONFIG_REG; break;
		case 0x04700008: * Value = RI_CURRENT_LOAD_REG; break;
		case 0x0470000C: * Value = RI_SELECT_REG; break;
		case 0x04700010: * Value = RI_REFRESH_REG; break;
		case 0x04700014: * Value = RI_LATENCY_REG; break;
		case 0x04700018: * Value = RI_RERROR_REG; break;
		case 0x0470001C: * Value = RI_WERROR_REG; break;
		default:
			* Value = 0;
			return 0;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800018: *Value = SI_STATUS_REG; break;
		default:
			*Value = 0;
			return 0;
		}
		break;
	case 0x05000000:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return 0;
	case 0x08000000:
		*Value = 0;
		break;

	default:
		*Value = PAddr & 0xFFFF;
		*Value = (*Value << 16) | *Value;
		return 0;
		break;
	}
	return 1;
}

void Cpu::r4300i_LW_PAddr ( uint32_t PAddr, uint32_t * Value ) {
	*Value = *(uint32_t *)(N64MEM+PAddr);
}

uint32_t Cpu::r4300i_LW_VAddr ( uint32_t VAddr, uint32_t * Value ) {
	uintptr_t address = (TLB_Map[VAddr >> 12] + VAddr);

	if (TLB_Map[VAddr >> 12] == 0) { return 0; }

	/*if((address - (uintptr_t)RDRAM) > RdramSize) {
		address = address - (uintptr_t)RDRAM;
		return r4300i_LW_NonMemory(address, Value);
	}*/
	*Value = *(uint32_t *)address;
	return 1;
}

int32_t Cpu::r4300i_SB_NonMemory ( uint32_t PAddr, uint8_t Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < RdramSize) {
			uint32_t OldProtect;

			//if (VirtualProtect((N64MEM + PAddr), 1, PAGE_READWRITE, &OldProtect) == 0) {
			//	DisplayError("Failed to unprotect %X\n5", PAddr);
			//}
			*(uint8_t *)(N64MEM+PAddr) = Value;
			if (N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] == 0) { break; }
			N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] = 0;
			memset(JumpTable+((PAddr & 0xFFFFF000) >> 2),0,0x1000);
			*(DelaySlotTable + ((PAddr & 0xFFFFF000) >> 12)) = NULL;
		}
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

uint32_t Cpu::r4300i_SB_VAddr ( uint32_t VAddr, uint8_t Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*(uint8_t *)(TLB_Map[VAddr >> 12] + (VAddr ^ 3)) = Value;

	return 1;
}

int32_t Cpu::r4300i_SH_NonMemory ( uint32_t PAddr, uint16_t Value ) {
	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < RdramSize) {
			uint32_t OldProtect;

//			if (VirtualProtect((N64MEM + PAddr), 2, PAGE_READWRITE, &OldProtect) == 0) {
//				DisplayError("Failed to unprotect %X\n4", PAddr);
//			}
			*(uint16_t *)(N64MEM+PAddr) = Value;
			if (N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] == 0) { break; }
			N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] = 0;
			memset(JumpTable+((PAddr & 0xFFFFF000) >> 2),0,0x1000);
			*(DelaySlotTable + ((PAddr & 0xFFFFF000) >> 12)) = NULL;
		}
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

uint32_t Cpu::r4300i_SD_VAddr ( uint32_t VAddr, uint64_t Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*(uint32_t *)(TLB_Map[VAddr >> 12] + VAddr) = *((uint32_t *)(&Value) + 1);
	*(uint32_t *)(TLB_Map[VAddr >> 12] + VAddr + 4) = *((uint32_t *)(&Value));
	return 1;
}

uint32_t Cpu::r4300i_SH_VAddr ( uint32_t VAddr, uint16_t Value ) {
	if (TLB_Map[VAddr >> 12] == 0) { return 0; }
	*(uint16_t *)(TLB_Map[VAddr >> 12] + (VAddr ^ 2)) = Value;
	return 1;
}

int32_t Cpu::r4300i_SW_NonMemory ( uint32_t PAddr, uint32_t Value ) {
	if (PAddr >= 0x10000000 && PAddr < 0x16000000) {
		if ((PAddr - 0x10000000) < RomFileSize) {
			WrittenToRom = 1;
			WroteToRom = Value;
		} else {
			return 0;
		}
	}

	PAddr=UnmapReg(PAddr);

	switch (PAddr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		if (PAddr < RdramSize) {
			uint32_t OldProtect;

//			if (VirtualProtect((N64MEM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) {
//				DisplayError("Failed to unprotect %X\n3", PAddr);
//			}
			*(uint32_t *)(N64MEM+PAddr) = Value;
			if (N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] == 0) { break; }
			N64_Blocks.NoOfRDRamBlocks[(PAddr & 0x00FFFFF0) >> 12] = 0;
			memset(JumpTable+((PAddr & 0xFFFFF000) >> 2),0,0x1000);
			*(DelaySlotTable + ((PAddr & 0xFFFFF000) >> 12)) = NULL;
		}
		break;
	case 0x03F00000:
		switch (PAddr) {
		case 0x03F00000: RDRAM_CONFIG_REG = Value; break;
		case 0x03F00004: RDRAM_DEVICE_ID_REG = Value; break;
		case 0x03F00008: RDRAM_DELAY_REG = Value; break;
		case 0x03F0000C: RDRAM_MODE_REG = Value; break;
		case 0x03F00010: RDRAM_REF_INTERVAL_REG = Value; break;
		case 0x03F00014: RDRAM_REF_ROW_REG = Value; break;
		case 0x03F00018: RDRAM_RAS_INTERVAL_REG = Value; break;
		case 0x03F0001C: RDRAM_MIN_INTERVAL_REG = Value; break;
		case 0x03F00020: RDRAM_ADDR_SELECT_REG = Value; break;
		case 0x03F00024: RDRAM_DEVICE_MANUF_REG = Value; break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		default:
			return 0;
		}
		break;
	case 0x04000000:
		if (PAddr < 0x04002000) {
			uint32_t OldProtect;

//			if (VirtualProtect((N64MEM + PAddr), 4, PAGE_READWRITE, &OldProtect) == 0) {
//				DisplayError("Failed to unprotect %X\n2", PAddr);
//			}
			*(uint32_t *)(N64MEM+PAddr) = Value;
			if (PAddr < 0x04001000) {
				if (N64_Blocks.NoOfDMEMBlocks == 0) { break; }
				N64_Blocks.NoOfDMEMBlocks = 0;
			} else {
				if (N64_Blocks.NoOfIMEMBlocks == 0) { break; }
				N64_Blocks.NoOfIMEMBlocks = 0;
			}
			memset(JumpTable+((PAddr & 0xFFFFF000) >> 2),0,0x1000);
			*(DelaySlotTable + ((PAddr & 0xFFFFF000) >> 12)) = NULL;
			return 1;
		}
		switch (PAddr) {
		case 0x04040000: SP_MEM_ADDR_REG = Value; break;
		case 0x04040004: SP_DRAM_ADDR_REG = Value; break;
		case 0x04040008:
			SP_RD_LEN_REG = Value;
			SP_DMA_READ();
			break;
		case 0x0404000C:
			SP_WR_LEN_REG = Value;
			SP_DMA_WRITE();
			break;
		case 0x04040010:
			if ( ( Value & SP_CLR_HALT ) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
			if ( ( Value & SP_SET_HALT ) != 0) { SP_STATUS_REG |= SP_STATUS_HALT;  }
			if ( ( Value & SP_CLR_BROKE ) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
			if ( ( Value & SP_CLR_INTR ) != 0) {
				MI_INTR_REG &= ~MI_INTR_SP;
				CheckInterrupts();
			}
			if ( ( Value & SP_CLR_SSTEP ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
			if ( ( Value & SP_SET_SSTEP ) != 0) { SP_STATUS_REG |= SP_STATUS_SSTEP;  }
			if ( ( Value & SP_CLR_INTR_BREAK ) != 0) { SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
			if ( ( Value & SP_SET_INTR_BREAK ) != 0) { SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
			if ( ( Value & SP_CLR_SIG0 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG0; }
			if ( ( Value & SP_SET_SIG0 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG0;  }
			if ( ( Value & SP_CLR_SIG1 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG1; }
			if ( ( Value & SP_SET_SIG1 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG1;  }
			if ( ( Value & SP_CLR_SIG2 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG2; }
			if ( ( Value & SP_SET_SIG2 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG2;  }
			if ( ( Value & SP_CLR_SIG3 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG3; }
			if ( ( Value & SP_SET_SIG3 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG3;  }
			if ( ( Value & SP_CLR_SIG4 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG4; }
			if ( ( Value & SP_SET_SIG4 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG4;  }
			if ( ( Value & SP_CLR_SIG5 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG5; }
			if ( ( Value & SP_SET_SIG5 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG5;  }
			if ( ( Value & SP_CLR_SIG6 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG6; }
			if ( ( Value & SP_SET_SIG6 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG6;  }
			if ( ( Value & SP_CLR_SIG7 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG7; }
			if ( ( Value & SP_SET_SIG7 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG7;  }

			RunRsp();
//printf("ViTimer\n");

			break;
		case 0x0404001C: SP_SEMAPHORE_REG = 0; break;
		case 0x04080000: SP_PC_REG = Value & 0xFFC; break;
		default:
			return 0;
		}
		break;
	case 0x04100000:
		switch (PAddr) {
		case 0x04100000:
			DPC_START_REG = Value;
			DPC_CURRENT_REG = Value;
			break;
		case 0x04100004:
			DPC_END_REG = Value;
			//if (ProcessRDPList) { ProcessRDPList(); }
			break;
		//case 0x04100008: DPC_CURRENT_REG = Value; break;
		case 0x0410000C:
			if ( ( Value & DPC_CLR_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA; }
			if ( ( Value & DPC_SET_XBUS_DMEM_DMA ) != 0) { DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FREEZE; }
			if ( ( Value & DPC_SET_FREEZE ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FREEZE;  }
			if ( ( Value & DPC_CLR_FLUSH ) != 0) { DPC_STATUS_REG &= ~DPC_STATUS_FLUSH; }
			if ( ( Value & DPC_SET_FLUSH ) != 0) { DPC_STATUS_REG |= DPC_STATUS_FLUSH;  }
			if ( ( Value & DPC_CLR_FREEZE ) != 0)
			{
				if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0)
				{
					if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 )
					{
						RunRsp();
					}
				}
			}
			break;
		default:
			return 0;
		}
		break;
	case 0x04300000:
		switch (PAddr) {
		case 0x04300000:
			MI_MODE_REG &= ~0x7F;
			MI_MODE_REG |= (Value & 0x7F);
			if ( ( Value & MI_CLR_INIT ) != 0 ) { MI_MODE_REG &= ~MI_MODE_INIT; }
			if ( ( Value & MI_SET_INIT ) != 0 ) { MI_MODE_REG |= MI_MODE_INIT; }
			if ( ( Value & MI_CLR_EBUS ) != 0 ) { MI_MODE_REG &= ~MI_MODE_EBUS; }
			if ( ( Value & MI_SET_EBUS ) != 0 ) { MI_MODE_REG |= MI_MODE_EBUS; }
			if ( ( Value & MI_CLR_DP_INTR ) != 0 ) {
				MI_INTR_REG &= ~MI_INTR_DP;
				CheckInterrupts();
			}
			if ( ( Value & MI_CLR_RDRAM ) != 0 ) { MI_MODE_REG &= ~MI_MODE_RDRAM; }
			if ( ( Value & MI_SET_RDRAM ) != 0 ) { MI_MODE_REG |= MI_MODE_RDRAM; }
			break;
		case 0x0430000C:
			if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
			if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
			if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
			if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
			if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
			if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
			if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
			break;
		default:
			return 0;
		}
		break;
	case 0x04400000:
		switch (PAddr) {
		case 0x04400000:
			//if (VI_STATUS_REG != Value) {
			//	VI_STATUS_REG = Value;
			//	if (ViStatusChanged != NULL ) { ViStatusChanged(); }
			//}
			break;
		case 0x04400004:

			//VI_ORIGIN_REG = (Value & 0xFFFFFF);
			//if (UpdateScreen != NULL ) { UpdateScreen(); }
			break;
		case 0x04400008:
			//if (VI_WIDTH_REG != Value) {
				//VI_WIDTH_REG = Value;
			//	if (ViWidthChanged != NULL ) { ViWidthChanged(); }
			//}
			break;
		case 0x0440000C: VI_INTR_REG = Value; break;
		case 0x04400010:
			MI_INTR_REG &= ~MI_INTR_VI;
			CheckInterrupts();
			break;
		case 0x04400014: VI_BURST_REG = Value; break;
		case 0x04400018: VI_V_SYNC_REG = Value; break;
		case 0x0440001C: VI_H_SYNC_REG = Value; break;
		case 0x04400020: VI_LEAP_REG = Value; break;
		case 0x04400024: VI_H_START_REG = Value; break;
		case 0x04400028: VI_V_START_REG = Value; break;
		case 0x0440002C: VI_V_BURST_REG = Value; break;
		case 0x04400030: VI_X_SCALE_REG = Value; break;
		case 0x04400034: VI_Y_SCALE_REG = Value; break;
		default:
			return 0;
		}
		break;
	case 0x04500000:
		switch (PAddr) {
		case 0x04500000: AI_DRAM_ADDR_REG = Value; break;
		case 0x04500004:
			AI_LEN_REG = Value;
			audio->AiLenChanged();
			break;
		case 0x04500008: AI_CONTROL_REG = (Value & 0x1); break;
		case 0x0450000C:
			/* Clear Interrupt */;
			MI_INTR_REG &= ~MI_INTR_AI;
			AudioIntrReg &= ~MI_INTR_AI;
			CheckInterrupts();
			break;
		case 0x04500010:
			AI_DACRATE_REG = Value;
			audio->AiDacrateChanged(Value);
			break;
		case 0x04500014:  AI_BITRATE_REG = Value; break;
		default:
			return 0;
		}
		break;
	case 0x04600000:
		switch (PAddr) {
		case 0x04600000: PI_DRAM_ADDR_REG = Value; break;
		case 0x04600004: PI_CART_ADDR_REG = Value; break;
		case 0x04600008:
			PI_RD_LEN_REG = Value;
			PI_DMA_READ();
			break;
		case 0x0460000C:
			PI_WR_LEN_REG = Value;
			PI_DMA_WRITE();
			break;
		case 0x04600010:
			//if ((Value & PI_SET_RESET) != 0 ) { DisplayError("reset Controller"); }
			if ((Value & PI_CLR_INTR) != 0 ) {
				MI_INTR_REG &= ~MI_INTR_PI;
				CheckInterrupts();
			}
			break;
		case 0x04600014: PI_DOMAIN1_REG = (Value & 0xFF); break;
		case 0x04600018: PI_BSD_DOM1_PWD_REG = (Value & 0xFF); break;
		case 0x0460001C: PI_BSD_DOM1_PGS_REG = (Value & 0xFF); break;
		case 0x04600020: PI_BSD_DOM1_RLS_REG = (Value & 0xFF); break;
		default:
			return 0;
		}
		break;
	case 0x04700000:
		switch (PAddr) {
		case 0x04700000: RI_MODE_REG = Value; break;
		case 0x04700004: RI_CONFIG_REG = Value; break;
		case 0x04700008: RI_CURRENT_LOAD_REG = Value; break;
		case 0x0470000C: RI_SELECT_REG = Value; break;
		case 0x04700010: RI_REFRESH_REG = Value; break;
		case 0x04700014: RI_LATENCY_REG = Value; break;
		case 0x04700018: RI_RERROR_REG = Value; break;
		case 0x0470001C: RI_WERROR_REG = Value; break;
		default:
			return 0;
		}
		break;
	case 0x04800000:
		switch (PAddr) {
		case 0x04800000: SI_DRAM_ADDR_REG = Value; break;
		case 0x04800004:
			SI_PIF_ADDR_RD64B_REG = Value;
			SI_DMA_READ ();
			break;
		case 0x04800010:
			SI_PIF_ADDR_WR64B_REG = Value;
			SI_DMA_WRITE();
			break;
		case 0x04800018:
			MI_INTR_REG &= ~MI_INTR_SI;
			SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
			CheckInterrupts();
			break;
		default:
			return 0;
		}
		break;
	case 0x08000000:
		if (PAddr != 0x08010000) { return 0; }
		//WriteToFlashCommand(Value);
		break;
	case 0x1FC00000:
		if (PAddr < 0x1FC007C0) {
			return 0;
		} else if (PAddr < 0x1FC00800) {
		_asm {
				mov eax,Value
				bswap eax
				mov Value,eax
			}
			*(uint32_t *)(&PIF_Ram[PAddr - 0x1FC007C0]) = Value;
			if (PAddr == 0x1FC007FC) {
				PifRamWrite();
			}
			return 1;
		}
		return 0;
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

uint32_t Cpu::r4300i_SW_VAddr ( uint32_t VAddr, uint32_t Value ) {
	uintptr_t address = (TLB_Map[VAddr >> 12] + VAddr);

	if (TLB_Map[VAddr >> 12] == 0) { return 0; }

	/*if((address - (uintptr_t)RDRAM) > RdramSize) {
		address = address - (uintptr_t)RDRAM;
		return r4300i_SW_NonMemory(address, Value);
	}*/
	*(uint32_t *)address = Value;
	return 1;
}

void Cpu::Release_Memory ( void ) {
	uint32_t i;

	for (i = 0; i < 0x400; i++)
	{
		if (ROMPages[i]) {
			free(ROMPages[i]); ROMPages[i] = 0;
		}
	}

	MemoryState = 0;

	if (TLB_Map != NULL) { VirtualFree(TLB_Map, 0, MEM_RELEASE); TLB_Map=NULL;}

	if (N64MEM != NULL) { VirtualFree(N64MEM, 0, MEM_RELEASE); N64MEM=NULL;}

	if (DelaySlotTable != NULL) { VirtualFree(DelaySlotTable, 0, MEM_RELEASE); DelaySlotTable=NULL;}
	if (JumpTable != NULL) { VirtualFree(JumpTable, 0, MEM_RELEASE); JumpTable=NULL;}

	if(reinterpret_cast<Recompiler*>(this)->RecompCode)
	{
		VirtualFree(reinterpret_cast<Recompiler*>(this)->RecompCode, 0, MEM_RELEASE);
		reinterpret_cast<Recompiler*>(this)->RecompCode = NULL;
	}
}


void Recompiler::ResetRecompCode (void) {
	uint32_t count, OldProtect;
	RecompPos = (uint8_t*)(((uintptr_t)RecompCode & ~0xFFFF) + 0x10000);

	//Jump Table
	for (count = 0; count < (RdramSize >> 12); count ++ )
	{
		if (N64_Blocks.NoOfRDRamBlocks[count] > 0) {
			N64_Blocks.NoOfRDRamBlocks[count] = 0;
			memset(JumpTable + (count << 10),0,0x1000);
			*(DelaySlotTable + count) = NULL;
		}
	}

	if (N64_Blocks.NoOfDMEMBlocks > 0) {
		N64_Blocks.NoOfDMEMBlocks = 0;
		memset(JumpTable + (0x04000000 >> 2),0,0x1000);
		*(DelaySlotTable + (0x04000000 >> 12)) = NULL;
	}
	if (N64_Blocks.NoOfIMEMBlocks > 0) {
		N64_Blocks.NoOfIMEMBlocks = 0;
		memset(JumpTable + (0x04001000 >> 2),0,0x1000);
		*(DelaySlotTable + (0x04001000 >> 12)) = NULL;
	}
}

extern int gmap[64];
#include <windows.h>
#ifdef USEX64
#error X64!
int r4300i_CPU_MemoryFilter64( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	uintptr_t MemAddress = ((char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (uintptr_t)N64MEM);
	uint8_t *ip = (uint8_t *)lpEP->ContextRecord->Rip;

	if(lpEP->ExceptionRecord->ExceptionInformation[1] == 0) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if((*ip & 0x40) && (*(ip+1) == 0xf) && (*(ip+2) == 0xb7)) {
		uint8_t dest_reg = (*(ip+3) % 0x40) / 8;
		int32_t half = 0;
		if(*ip & 4) dest_reg += 8;
		//r4300i_LH_NonMemory(MemAddress, &half, 1);
		_asm int 3

		((uint64_t*)(&lpEP->ContextRecord->Rax))[dest_reg] = (int32_t)half;

		lpEP->ContextRecord->Rip+=4;

		if((*(ip+3) & 0x7)==4)
			lpEP->ContextRecord->Rip++;

		if((*(ip+3) & 0xC0) == 0x80)
			lpEP->ContextRecord->Rip+=4;
		else if((*(ip+3) & 0xC0) == 0x40)
			lpEP->ContextRecord->Rip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;

	} else if((*ip & 0x40) && (*(ip+1) ==0x89)) { // MOV [Rxx + Rxx], Exx
		uint8_t dest_reg = (*(ip+2) % 0x40) / 8;
		uint64_t dest = 0;

		if(*ip & 4) dest_reg += 8;

		dest = ((uint64_t*)(&lpEP->ContextRecord->Rax))[dest_reg];

		//r4300i_SW_NonMemory(MemAddress, dest);
		_asm int 3

		if((*(ip+2) & 0x7)==4)
			lpEP->ContextRecord->Rip+=4;
		else
			lpEP->ContextRecord->Rip+=3;

		if((*(ip+2) & 0xC0) == 0x80)
			lpEP->ContextRecord->Rip+=4;
		else if((*(ip+2) & 0xC0) == 0x40)
			lpEP->ContextRecord->Rip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;

	} else if((*ip & 0x40) && (*(ip+1) ==0xC7)) { // MOV [Rxx + Rxx], Imm32
		uint32_t imm32 = *(uint32_t*)(ip+4);
		//r4300i_SW_NonMemory(MemAddress, imm32);
		_asm int 3
		lpEP->ContextRecord->Rip+=7;


		// 40 C7 04 07 0F 00 00 00

		//if(*(ip+2)&0x4)
		if((*(ip+2) & 0x7)==4)
			lpEP->ContextRecord->Rip++;
		if((*(ip+2) & 0xC0) == 0x80)
			lpEP->ContextRecord->Rip+=4;
		else if((*(ip+2) & 0xC0) == 0x40)
			lpEP->ContextRecord->Rip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;
	} else if ((*ip & 0x40) && (*(ip + 1) == 0x8B )) {
		uint8_t dest_reg = (*(ip+2) % 0x40) / 8;
		uint32_t word = 0;
		uint64_t *dest = 0;

		//41 8B BF 30 D0 00 01


		if(*ip & 4) dest_reg += 8;

		dest = &((uint64_t*)(&lpEP->ContextRecord->Rax))[dest_reg];
		//r4300i_LW_NonMemory(MemAddress, &word);
		_asm int 3
		*dest = word;
		*dest &= 0x00000000ffffffff;
		//*dest = 0;
		//if(*(ip+2) & 0x4)
		if((*(ip+2) & 0x7)==4)
			lpEP->ContextRecord->Rip+=4;
		else
			lpEP->ContextRecord->Rip+=3;

		if((*(ip+2) & 0xC0) == 0x80)
			lpEP->ContextRecord->Rip+=4;
		else if((*(ip+2) & 0xC0) == 0x40)
			lpEP->ContextRecord->Rip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;
	}


return EXCEPTION_CONTINUE_EXECUTION;
	return EXCEPTION_CONTINUE_SEARCH;

}




#else

static int32_t CONV_REG(int32_t dest_reg) {
	switch(dest_reg) {
		case 0: return 5; break;
		case 1: return 4; break;
		case 2: return 3; break;
		case 3: return 2; break;
		case 6: return 1; break;
		case 7: return 0; break;
		default:
			_asm int 3;
			 break;
	}
}

/*int32_t Cpu::r4300i_CPU_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	uintptr_t MemAddress = (uintptr_t)((char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (uintptr_t)N64MEM);
	uint8_t *ip = (uint8_t *)lpEP->ContextRecord->Eip;

	if(lpEP->ExceptionRecord->ExceptionInformation[1] == 0) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if((*(ip) == 0xf) && (*(ip+1) == 0xb7)) {
		uint8_t dest_reg = (*(ip+2) % 0x40) / 8;
		uint32_t half = 0;
		r4300i_LH_NonMemory((uint32_t)MemAddress, &half, 1);

		((uint32_t*)(&lpEP->ContextRecord->Edi))[CONV_REG(dest_reg)] = (int32_t)half;

		lpEP->ContextRecord->Eip+=3;

		if((*(ip+2) & 0x7)==4)
			lpEP->ContextRecord->Eip++;
		else if((*(ip+1) & 0x7)==5)
			lpEP->ContextRecord->Eip+=4;


		if((*(ip+2) & 0xC0) == 0x80)
			lpEP->ContextRecord->Eip+=4;
		else if((*(ip+2) & 0xC0) == 0x40)
			lpEP->ContextRecord->Eip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;

	} else if((*(ip) ==0x89)) { // MOV [Rxx + Rxx], Exx
		uint8_t dest_reg = (*(ip+1) % 0x40) / 8;
		uint32_t dest = 0;

		dest = ((uint32_t*)(&lpEP->ContextRecord->Edi))[CONV_REG(dest_reg)];

		r4300i_SW_NonMemory(MemAddress, dest);

		if((*(ip+1) & 0x7)==4)
			lpEP->ContextRecord->Eip+=3;
		else
			lpEP->ContextRecord->Eip+=2;

		if((*(ip+1) & 0xC0) == 0x80)
			lpEP->ContextRecord->Eip+=4;
		else if((*(ip+1) & 0xC0) == 0x40)
			lpEP->ContextRecord->Eip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;

	} else if((*(ip) ==0xC7)) { // MOV [Rxx + Rxx], Imm32
		uint32_t imm32 = *(uint32_t*)(ip+2);
		if((*(ip+1) & 0x7)==4)
			imm32 = *(uint32_t*)(ip+3);

		r4300i_SW_NonMemory(MemAddress, imm32);
		lpEP->ContextRecord->Eip+=6;


		if((*(ip+1) & 0x7)==4)
			lpEP->ContextRecord->Eip++;
		if((*(ip+1) & 0xC0) == 0x80)
			lpEP->ContextRecord->Eip+=4;
		else if((*(ip+1) & 0xC0) == 0x40)
			lpEP->ContextRecord->Eip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;
	} else if (*ip == 0x8B ) {
		uint8_t dest_reg = CONV_REG((*(ip+1) % 0x40) / 8);
		uint32_t word = 0;
		uint32_t *dest = 0;

		dest = &((uint32_t*)(&lpEP->ContextRecord->Edi))[dest_reg];
		r4300i_LW_NonMemory(MemAddress, &word);
		*dest = word;
		//*dest = 0;
		//if(*(ip+2) & 0x4)
		if((*(ip+1) & 0x7)==4)
			lpEP->ContextRecord->Eip+=3;
		else if((*(ip+1) & 0x7)==5)
			lpEP->ContextRecord->Eip+=6;
		else
			lpEP->ContextRecord->Eip+=2;

		if((*(ip+1) & 0xC0) == 0x80)
			lpEP->ContextRecord->Eip+=4;
		else if((*(ip+1) & 0xC0) == 0x40)
			lpEP->ContextRecord->Eip+=1;

		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}
*/

int32_t Cpu::r4300i_CPU_MemoryFilter( DWORD dwExptCode, LPEXCEPTION_POINTERS lpEP) {
	DWORD MemAddress = UnmapReg((char *)lpEP->ExceptionRecord->ExceptionInformation[1] - (char *)N64MEM);


	EXCEPTION_RECORD exRec;
	BYTE * ReadPos, *TypePos;
	void * Reg;

	if (dwExptCode != EXCEPTION_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	TypePos = (unsigned char *)lpEP->ContextRecord->Eip;
	exRec = *lpEP->ExceptionRecord;

    if ((int)(MemAddress) < 0 || MemAddress > 0x1FFFFFFF) { return EXCEPTION_CONTINUE_SEARCH; }

	if (*TypePos == 0xF3 && *(TypePos + 1) == 0xA5) {
		DWORD Start, End, count, OldProtect;
		Start = (lpEP->ContextRecord->Edi - (DWORD)N64MEM);
		End = (Start + (lpEP->ContextRecord->Ecx << 2) - 1);
		if ((int)Start < 0) {
			return EXCEPTION_CONTINUE_SEARCH;
		}
		if ((int)End < RdramSize) {
			for ( count = Start; count < End; count += 0x1000 ) {
				if (N64_Blocks.NoOfRDRamBlocks[(count >> 12)] > 0) {
					N64_Blocks.NoOfRDRamBlocks[(count >> 12)] = 0;
					memset(JumpTable + ((count & 0x00FFFFF0) >> 2),0,0x1000);
					*(DelaySlotTable + count) = NULL;
					if (VirtualProtect(N64MEM + count, 4, PAGE_READWRITE, &OldProtect) == 0) {
					}
				}
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04000000 && End < 0x04001000) {
			N64_Blocks.NoOfDMEMBlocks = 0;
			memset(JumpTable + (0x04000000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04000000 >> 12)) = NULL;
			if (VirtualProtect(N64MEM + 0x04000000, 4, PAGE_READWRITE, &OldProtect) == 0) {
			}
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (Start >= 0x04001000 && End < 0x04002000) {
			N64_Blocks.NoOfIMEMBlocks = 0;
			/*memset(JumpTable + (0x04001000 >> 2),0,0x1000);
			*(DelaySlotTable + (0x04001000 >> 12)) = NULL;
			if (VirtualProtect(N64MEM + 0x04001000, 4, PAGE_READWRITE, &OldProtect) == 0) {

			} */
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (*TypePos == 0x0F && *(TypePos + 1) == 0xB6) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xB7) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBE) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x0F && *(TypePos + 1) == 0xBF) {
		ReadPos = TypePos + 2;
	} else if (*TypePos == 0x66) {
		ReadPos = TypePos + 2;
	} else {
		ReadPos = TypePos + 1;
	}

	switch ((*ReadPos & 0x38)) {
	case 0x00: Reg = &lpEP->ContextRecord->Eax; break;
	case 0x08: Reg = &lpEP->ContextRecord->Ecx; break;
	case 0x10: Reg = &lpEP->ContextRecord->Edx; break;
	case 0x18: Reg = &lpEP->ContextRecord->Ebx; break;
	case 0x20: Reg = &lpEP->ContextRecord->Esp; break;
	case 0x28: Reg = &lpEP->ContextRecord->Ebp; break;
	case 0x30: Reg = &lpEP->ContextRecord->Esi; break;
	case 0x38: Reg = &lpEP->ContextRecord->Edi; break;
	}

	switch ((*ReadPos & 0xC7)) {
	case 0: ReadPos += 1; break;
	case 1: ReadPos += 1; break;
	case 2: ReadPos += 1; break;
	case 3: ReadPos += 1; break;
	case 4:
		ReadPos += 1;
		switch ((*ReadPos & 0xC7)) {
		case 0: ReadPos += 1; break;
		case 1: ReadPos += 1; break;
		case 2: ReadPos += 1; break;
		case 3: ReadPos += 1; break;
		case 6: ReadPos += 1; break;
		case 7: ReadPos += 1; break;
		default:
			_asm int 3
		}
		break;
	case 5: ReadPos += 5; break;
	case 6: ReadPos += 1; break;
	case 7: ReadPos += 1; break;
	case 0x40: ReadPos += 2; break;
	case 0x41: ReadPos += 2; break;
	case 0x42: ReadPos += 2; break;
	case 0x43: ReadPos += 2; break;
	case 0x46: ReadPos += 2; break;
	case 0x47: ReadPos += 2; break;
	case 0x80: ReadPos += 5; break;
	case 0x81: ReadPos += 5; break;
	case 0x82: ReadPos += 5; break;
	case 0x83: ReadPos += 5; break;
	case 0x86: ReadPos += 5; break;
	case 0x87: ReadPos += 5; break;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}

	switch(*TypePos) {
	case 0x0F:
		switch(*(TypePos + 1)) {
		case 0xB6:
			if (!r4300i_LB_NonMemory(MemAddress,(uint32_t*)Reg,FALSE)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		case 0xB7:
			if (!r4300i_LH_NonMemory(MemAddress,(uint32_t*)Reg,FALSE)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		case 0xBE:
			if (!r4300i_LB_NonMemory(MemAddress,(uint32_t*)Reg,TRUE)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		case 0xBF:
			if (!r4300i_LH_NonMemory(MemAddress,(uint32_t*)Reg,TRUE)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		default:
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x66:
		switch(*(TypePos + 1)) {
		case 0x8B:
			if (!r4300i_LH_NonMemory(MemAddress,(uint32_t*)Reg,FALSE)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		case 0x89:
			if (!r4300i_SH_NonMemory(MemAddress,*(WORD *)Reg)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)ReadPos;
			return EXCEPTION_CONTINUE_EXECUTION;
		case 0xC7:
			if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
			if (!r4300i_SH_NonMemory(MemAddress,*(WORD *)ReadPos)) {
			}
			lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 2);
			return EXCEPTION_CONTINUE_EXECUTION;
		default:
			return EXCEPTION_CONTINUE_SEARCH;
		}
		break;
	case 0x88:
		if (!r4300i_SB_NonMemory(MemAddress,*(BYTE *)Reg)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;
	case 0x8A:
		if (!r4300i_LB_NonMemory(MemAddress,(uint32_t*)Reg,FALSE)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;
	case 0x8B:
		if (!r4300i_LW_NonMemory(MemAddress,(uint32_t*)Reg)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;
	case 0x89:
		if (!r4300i_SW_NonMemory(MemAddress,*(DWORD *)Reg)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)ReadPos;
		return EXCEPTION_CONTINUE_EXECUTION;
	case 0xC6:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!r4300i_SB_NonMemory(MemAddress,*(BYTE *)ReadPos)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 1);
		return EXCEPTION_CONTINUE_EXECUTION;
	case 0xC7:
		if (Reg != &lpEP->ContextRecord->Eax) { return EXCEPTION_CONTINUE_SEARCH; }
		if (!r4300i_SW_NonMemory(MemAddress,*(DWORD *)ReadPos)) {
		}
		lpEP->ContextRecord->Eip = (DWORD)(ReadPos + 4);
		return EXCEPTION_CONTINUE_EXECUTION;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}




void Recompiler::Compile_LB ( int32_t Reg, uint32_t addr, uint32_t SignExtend ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		MoveConstToX86reg(0,Reg);
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
	case 0x10000000:

		if (SignExtend) {
			MoveSxVariableToX86regByte(Addr + N64MEM,Reg);
		} else {
			MoveZxVariableToX86regByte(Addr + N64MEM,Reg);
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
	}
}

void Recompiler::Compile_LH ( int32_t Reg, uint32_t addr, uint32_t SignExtend) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		MoveConstToX86reg(0,Reg);
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
	case 0x10000000:

		if (SignExtend) {
			MoveSxVariableToX86regHalf(Addr + N64MEM,Reg);
		} else {
			MoveZxVariableToX86regHalf(Addr + N64MEM,Reg);
		}
		break;
	default:
		MoveConstToX86reg(0,Reg);
	}
}

void Recompiler::Compile_LW ( int32_t Reg, uint32_t addr ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		MoveConstToX86reg(0,Reg);
	}

	Addr=UnmapReg(Addr);

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
	case 0x10000000:
		MoveVariableToX86reg(Addr + N64MEM,Reg);
		break;
	case 0x04000000:
		if (Addr < 0x04002000) {
			Addr|=0xa0000000;
			TranslateVaddr(&Addr);
			MoveVariableToX86reg(Addr + N64MEM,Reg);
			break;
		}
		switch (Addr) {
		case 0x04040010: MoveVariableToX86reg(&SP_STATUS_REG,Reg); break;
		case 0x04040014: MoveVariableToX86reg(&SP_DMA_FULL_REG,Reg); break;
		case 0x04040018: MoveVariableToX86reg(&SP_DMA_BUSY_REG,Reg); break;
		case 0x04080000: MoveVariableToX86reg(&SP_PC_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04100000:
		Addr|=0xa0000000;
		TranslateVaddr(&Addr);
		MoveVariableToX86reg(Addr + N64MEM,Reg);
		break;
	case 0x04300000:
		switch (Addr) {
		case 0x04300000: MoveVariableToX86reg(&MI_MODE_REG,Reg); break;
		case 0x04300004: MoveVariableToX86reg(&MI_VERSION_REG,Reg); break;
		case 0x04300008: MoveVariableToX86reg(&MI_INTR_REG,Reg); break;
		case 0x0430000C: MoveVariableToX86reg(&MI_INTR_MASK_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04400000:
		switch (Addr) {
		case 0x04400010:
			//Pushad();
			//CALL_CPUFUNC(UpdateCurrentHalfLine);
			//Popad();
			//MoveVariableToX86reg(&HalfLine,Reg);
			MoveConstToX86reg(0,Reg);

			break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04500000: /* AI registers */
		switch (Addr) {
		case 0x04500004:
			Pushad();

			PushImm32((uint32_t)audio);
			Call_Direct(GetAddress(0, &Audio::AiReadLength));
			AddConstToX86Reg(x86_ESP, 4);

			MoveX86regToVariable(x86_EAX,&TempValue);
			Popad();
			MoveVariableToX86reg(&TempValue,Reg);
			break;
		case 0x0450000C: MoveVariableToX86reg(&AI_STATUS_REG,Reg); break;
		case 0x04500010: MoveVariableToX86reg(&AI_DACRATE_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04600000:
		switch (Addr) {
		case 0x04600010: MoveVariableToX86reg(&PI_STATUS_REG,Reg); break;
		case 0x04600014: MoveVariableToX86reg(&PI_DOMAIN1_REG,Reg); break;
		case 0x04600018: MoveVariableToX86reg(&PI_BSD_DOM1_PWD_REG,Reg); break;
		case 0x0460001C: MoveVariableToX86reg(&PI_BSD_DOM1_PGS_REG,Reg); break;
		case 0x04600020: MoveVariableToX86reg(&PI_BSD_DOM1_RLS_REG,Reg); break;
		case 0x04600024: MoveVariableToX86reg(&PI_DOMAIN2_REG,Reg); break;
		case 0x04600028: MoveVariableToX86reg(&PI_BSD_DOM2_PWD_REG,Reg); break;
		case 0x0460002C: MoveVariableToX86reg(&PI_BSD_DOM2_PGS_REG,Reg); break;
		case 0x04600030: MoveVariableToX86reg(&PI_BSD_DOM2_RLS_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04700000:
		switch (Addr) {
		case 0x0470000C: MoveVariableToX86reg(&RI_SELECT_REG,Reg); break;
		case 0x04700010: MoveVariableToX86reg(&RI_REFRESH_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x04800000:
		switch (Addr) {
		case 0x04800018: MoveVariableToX86reg(&SI_STATUS_REG,Reg); break;
		default: MoveConstToX86reg(0,Reg); break;
		}
		break;
	case 0x1FC00000:
		Addr|=0xa0000000;
		TranslateVaddr(&Addr);
		MoveVariableToX86reg(Addr + N64MEM,Reg);
		break;
	default: MoveConstToX86reg(((Addr & 0xFFFF) << 16) | (Addr & 0xFFFF),Reg);
	}
}

void Recompiler::Compile_SB_Const ( uint8_t Value, uint32_t addr ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveConstByteToVariable(Value,Addr + N64MEM);
		break;
	}
}

void Recompiler::Compile_SB_Register ( int32_t x86Reg, uint32_t addr ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveX86regByteToVariable(x86Reg,Addr + N64MEM);
		break;
	}
}

void Recompiler::Compile_SH_Const ( uint16_t Value, uint32_t addr ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveConstHalfToVariable(Value,Addr + N64MEM);
		break;
	}
}

void Recompiler::Compile_SH_Register ( int32_t x86Reg, uint32_t addr ) {
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		return;
	}

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveX86regHalfToVariable(x86Reg,Addr + N64MEM);
		break;
	}
}

void Recompiler::Compile_SW_Const ( uint32_t Value, uint32_t addr ) {
	uint8_t * Jump = 0;
	uintptr_t Addr = addr;
	if (!TranslateVaddr(&Addr)) {
		return;
	}

	Addr=UnmapReg(Addr);

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveConstToVariable(Value,Addr + N64MEM);
		break;
	case 0x03F00000:
		switch (Addr) {
		case 0x03F00000: MoveConstToVariable(Value,&RDRAM_CONFIG_REG); break;
		case 0x03F00004: MoveConstToVariable(Value,&RDRAM_DEVICE_ID_REG); break;
		case 0x03F00008: MoveConstToVariable(Value,&RDRAM_DELAY_REG); break;
		case 0x03F0000C: MoveConstToVariable(Value,&RDRAM_MODE_REG); break;
		case 0x03F00010: MoveConstToVariable(Value,&RDRAM_REF_INTERVAL_REG); break;
		case 0x03F00014: MoveConstToVariable(Value,&RDRAM_REF_ROW_REG); break;
		case 0x03F00018: MoveConstToVariable(Value,&RDRAM_RAS_INTERVAL_REG); break;
		case 0x03F0001C: MoveConstToVariable(Value,&RDRAM_MIN_INTERVAL_REG); break;
		case 0x03F00020: MoveConstToVariable(Value,&RDRAM_ADDR_SELECT_REG); break;
		case 0x03F00024: MoveConstToVariable(Value,&RDRAM_DEVICE_MANUF_REG); break;
		case 0x03F04004: break;
		case 0x03F08004: break;
		case 0x03F80004: break;
		case 0x03F80008: break;
		case 0x03F8000C: break;
		case 0x03F80014: break;
		}
		break;
	case 0x04000000:
		if (Addr < 0x04002000) {
			Addr|=0xa0000000;
			TranslateVaddr(&Addr);
			MoveConstToVariable(Value,Addr + N64MEM);
			break;
		}
		switch (Addr) {
		case 0x04040000: MoveConstToVariable(Value,&SP_MEM_ADDR_REG); break;
		case 0x04040004: MoveConstToVariable(Value,&SP_DRAM_ADDR_REG); break;
		case 0x04040008:
			MoveConstToVariable(Value,&SP_RD_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SP_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04040010:
			{
				uint32_t ModValue = 0;
				if ( ( Value & SP_CLR_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_CLR_BROKE ) != 0 ) { ModValue |= SP_STATUS_BROKE; }
				if ( ( Value & SP_CLR_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_CLR_INTR_BREAK ) != 0 ) { ModValue |= SP_STATUS_INTR_BREAK; }
				if ( ( Value & SP_CLR_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_CLR_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_CLR_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_CLR_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_CLR_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_CLR_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_CLR_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_CLR_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }

				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&SP_STATUS_REG);
				}

				ModValue = 0;
				if ( ( Value & SP_SET_HALT ) != 0 ) { ModValue |= SP_STATUS_HALT; }
				if ( ( Value & SP_SET_SSTEP ) != 0 ) { ModValue |= SP_STATUS_SSTEP; }
				if ( ( Value & SP_SET_INTR_BREAK ) != 0) { ModValue |= SP_STATUS_INTR_BREAK;  }
				if ( ( Value & SP_SET_SIG0 ) != 0 ) { ModValue |= SP_STATUS_SIG0; }
				if ( ( Value & SP_SET_SIG1 ) != 0 ) { ModValue |= SP_STATUS_SIG1; }
				if ( ( Value & SP_SET_SIG2 ) != 0 ) { ModValue |= SP_STATUS_SIG2; }
				if ( ( Value & SP_SET_SIG3 ) != 0 ) { ModValue |= SP_STATUS_SIG3; }
				if ( ( Value & SP_SET_SIG4 ) != 0 ) { ModValue |= SP_STATUS_SIG4; }
				if ( ( Value & SP_SET_SIG5 ) != 0 ) { ModValue |= SP_STATUS_SIG5; }
				if ( ( Value & SP_SET_SIG6 ) != 0 ) { ModValue |= SP_STATUS_SIG6; }
				if ( ( Value & SP_SET_SIG7 ) != 0 ) { ModValue |= SP_STATUS_SIG7; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&SP_STATUS_REG);
				}

				if ( ( Value & SP_CLR_INTR ) != 0) {
					AndConstToVariable(~MI_INTR_SP,&MI_INTR_REG);
					Pushad();
					CALL_CPUFUNC(RunRsp);
					CALL_CPUFUNC(CheckInterrupts);
					Popad();
				} else {
					Pushad();
					CALL_CPUFUNC(RunRsp);
					Popad();
				}
			}
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG); break;
		case 0x04080000: MoveConstToVariable(Value & 0xFFC,&SP_PC_REG); break;
		}
		break;
	case 0x04300000:
		switch (Addr) {
		case 0x04300000:
			{
				uint32_t ModValue = 0x7F;
				if ( ( Value & MI_CLR_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_CLR_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_CLR_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_MODE_REG);
				}

				ModValue = (Value & 0x7F);
				if ( ( Value & MI_SET_INIT ) != 0 ) { ModValue |= MI_MODE_INIT; }
				if ( ( Value & MI_SET_EBUS ) != 0 ) { ModValue |= MI_MODE_EBUS; }
				if ( ( Value & MI_SET_RDRAM ) != 0 ) { ModValue |= MI_MODE_RDRAM; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_MODE_REG);
				}
				if ( ( Value & MI_CLR_DP_INTR ) != 0 ) {
					AndConstToVariable(~MI_INTR_DP,&MI_INTR_REG);
				}
			}
			break;
		case 0x0430000C:
			{
				uint32_t ModValue;
				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_CLR_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_CLR_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_CLR_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_CLR_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_CLR_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_CLR_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					AndConstToVariable(~ModValue,&MI_INTR_MASK_REG);
				}

				ModValue = 0;
				if ( ( Value & MI_INTR_MASK_SET_SP ) != 0 ) { ModValue |= MI_INTR_MASK_SP; }
				if ( ( Value & MI_INTR_MASK_SET_SI ) != 0 ) { ModValue |= MI_INTR_MASK_SI; }
				if ( ( Value & MI_INTR_MASK_SET_AI ) != 0 ) { ModValue |= MI_INTR_MASK_AI; }
				if ( ( Value & MI_INTR_MASK_SET_VI ) != 0 ) { ModValue |= MI_INTR_MASK_VI; }
				if ( ( Value & MI_INTR_MASK_SET_PI ) != 0 ) { ModValue |= MI_INTR_MASK_PI; }
				if ( ( Value & MI_INTR_MASK_SET_DP ) != 0 ) { ModValue |= MI_INTR_MASK_DP; }
				if (ModValue != 0) {
					OrConstToVariable(ModValue,&MI_INTR_MASK_REG);
				}
			}
			break;
		}
		break;
	case 0x04400000:
		switch (Addr) {
		case 0x04400000:
			MoveConstToVariable(Value,&VI_STATUS_REG);
			break;
		case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF),&VI_ORIGIN_REG); break;
		case 0x04400008:
			MoveConstToVariable(Value,&VI_WIDTH_REG);
			break;
		case 0x0440000C: MoveConstToVariable(Value,&VI_INTR_REG); break;
		case 0x04400010:
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		case 0x04400014: MoveConstToVariable(Value,&VI_BURST_REG); break;
		case 0x04400018: MoveConstToVariable(Value,&VI_V_SYNC_REG); break;
		case 0x0440001C: MoveConstToVariable(Value,&VI_H_SYNC_REG); break;
		case 0x04400020: MoveConstToVariable(Value,&VI_LEAP_REG); break;
		case 0x04400024: MoveConstToVariable(Value,&VI_H_START_REG); break;
		case 0x04400028: MoveConstToVariable(Value,&VI_V_START_REG); break;
		case 0x0440002C: MoveConstToVariable(Value,&VI_V_BURST_REG); break;
		case 0x04400030: MoveConstToVariable(Value,&VI_X_SCALE_REG); break;
		case 0x04400034: MoveConstToVariable(Value,&VI_Y_SCALE_REG); break;
		}
		break;
	case 0x04500000: /* AI registers */
		switch (Addr) {
		case 0x04500000: MoveConstToVariable(Value,&AI_DRAM_ADDR_REG); break;
		case 0x04500004:
			MoveConstToVariable(Value,&AI_LEN_REG);
			Pushad();
			PushImm32((uint32_t)audio);
			Call_Direct(GetAddress(0, &Audio::AiLenChanged));
			AddConstToX86Reg(x86_ESP, 4);

			Popad();
			break;
		case 0x04500008: MoveConstToVariable((Value & 1),&AI_CONTROL_REG); break;
		case 0x0450000C:
			/* Clear Interrupt */;
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG);
			AndConstToVariable(~MI_INTR_AI,&AudioIntrReg);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		case 0x04500010:
			Addr|=0xa0000000;
			TranslateVaddr(&Addr);
			MoveConstToVariable(Value,Addr + N64MEM);
			break;
		case 0x04500014: MoveConstToVariable(Value,&AI_BITRATE_REG); break;
		}
		break;
	case 0x04600000:
		switch (Addr) {
		case 0x04600000: MoveConstToVariable(Value,&PI_DRAM_ADDR_REG); break;
		case 0x04600004: MoveConstToVariable(Value,&PI_CART_ADDR_REG); break;
		case 0x04600008:
			MoveConstToVariable(Value,&PI_RD_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::PI_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x0460000C:
			MoveConstToVariable(Value,&PI_WR_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::PI_DMA_WRITE));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04600010:
			if ((Value & PI_CLR_INTR) != 0 ) {
				AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG);
				Pushad();
				CALL_CPUFUNC(CheckInterrupts);
				Popad();
			}
			break;
		case 0x04600014: MoveConstToVariable((Value & 0xFF),&PI_DOMAIN1_REG); break;
		case 0x04600018: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PWD_REG); break;
		case 0x0460001C: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_PGS_REG); break;
		case 0x04600020: MoveConstToVariable((Value & 0xFF),&PI_BSD_DOM1_RLS_REG); break;
		}
		break;
	case 0x04700000:
		switch (Addr) {
		case 0x04700000: MoveConstToVariable(Value,&RI_MODE_REG); break;
		case 0x04700004: MoveConstToVariable(Value,&RI_CONFIG_REG); break;
		case 0x04700008: MoveConstToVariable(Value,&RI_CURRENT_LOAD_REG); break;
		case 0x0470000C: MoveConstToVariable(Value,&RI_SELECT_REG); break;
		}
		break;
	case 0x04800000:
		switch (Addr) {
		case 0x04800000: MoveConstToVariable(Value,&SI_DRAM_ADDR_REG); break;
		case 0x04800004:
			MoveConstToVariable(Value,&SI_PIF_ADDR_RD64B_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SI_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04800010:
			MoveConstToVariable(Value,&SI_PIF_ADDR_WR64B_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SI_DMA_WRITE));
			AddConstToX86Reg(x86_ESP, 4);

			Popad();
			break;
		case 0x04800018:
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG);
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		}
		break;
	}
}

void Recompiler::Compile_SW_Register ( int32_t x86Reg, uint32_t addr ) {
	int8_t * Jump = 0;
	uintptr_t Addr = addr;


	if (!TranslateVaddr(&Addr)) {
		return;
	}

	Addr=UnmapReg(Addr);

	switch (Addr & 0xFFF00000) {
	case 0x00000000:
	case 0x00100000:
	case 0x00200000:
	case 0x00300000:
	case 0x00400000:
	case 0x00500000:
	case 0x00600000:
	case 0x00700000:
		MoveX86regToVariable(x86Reg,Addr + N64MEM);
		break;
	case 0x04000000:
		switch (Addr) {
		case 0x04040000: MoveX86regToVariable(x86Reg,&SP_MEM_ADDR_REG); break;
		case 0x04040004: MoveX86regToVariable(x86Reg,&SP_DRAM_ADDR_REG); break;
		case 0x04040008:
			MoveX86regToVariable(x86Reg,&SP_RD_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SP_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x0404000C:
			MoveX86regToVariable(x86Reg,&SP_WR_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SP_DMA_WRITE));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04040010:
			MoveX86regToVariable(x86Reg,&RegModValue);
			//BreakPoint();
			Pushad();
			Push(x86Reg);
			PushImm32((uint32_t)this);
			Call_Direct(GetAddress(0, &Recompiler::ChangeSpStatus));
			AddConstToX86Reg(x86_ESP,8);
			Popad();
			break;
		case 0x0404001C: MoveConstToVariable(0,&SP_SEMAPHORE_REG); break;
		case 0x04080000:
			MoveX86regToVariable(x86Reg,&SP_PC_REG);
			AndConstToVariable(0xFFC,&SP_PC_REG);
			break;
		default:
			if (Addr < 0x04002000) {
			Addr|=0xa0000000;
				TranslateVaddr(&Addr);

				MoveX86regToVariable(x86Reg,Addr + N64MEM);
			}
		}
		break;
	case 0x04100000:
		Addr|=0xa0000000;
		TranslateVaddr(&Addr);
		MoveX86regToVariable(x86Reg,Addr + N64MEM);
		break;
		//MoveX86regToVariable(x86Reg,Addr + N64MEM);
	case 0x04300000:
		switch (Addr) {
		case 0x04300000:
			MoveX86regToVariable(x86Reg,&RegModValue);
			//Pushad();
			//Call_Direct(GetAddress(0, &Recompiler::ChangeMiIntrMask));
			//Popad();
			Pushad();
			//Push(x86Reg);
			PushImm32((uint32_t)this);
			Call_Direct(GetAddress(0, &Recompiler::ChangeMiModeReg));
			AddConstToX86Reg(x86_ESP,4);
			Popad();
			break;
		case 0x0430000C:
			MoveX86regToVariable(x86Reg,&RegModValue);
			//Pushad();
			//Call_Direct(GetAddress(0, &Recompiler::ChangeMiIntrMask));
			//Popad();
			//BreakPoint();
			Pushad();
			Push(x86Reg);
			PushImm32((uint32_t)this);
			Call_Direct(GetAddress(0, &Recompiler::ChangeMiIntrMask));
			AddConstToX86Reg(x86_ESP,8);
			Popad();
			break;
		}
		break;

	case 0x04400000:
		switch (Addr) {
		case 0x04400000:
			MoveX86regToVariable(x86Reg,&VI_STATUS_REG);
			break;
		case 0x04400004:
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG);
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG);
			break;
		case 0x04400008:
			MoveX86regToVariable(x86Reg,&VI_WIDTH_REG);
			break;
		case 0x0440000C: MoveX86regToVariable(x86Reg,&VI_INTR_REG); break;
		case 0x04400010:
			AndConstToVariable(~MI_INTR_VI,&MI_INTR_REG);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		case 0x04400014: MoveX86regToVariable(x86Reg,&VI_BURST_REG); break;
		case 0x04400018: MoveX86regToVariable(x86Reg,&VI_V_SYNC_REG); break;
		case 0x0440001C: MoveX86regToVariable(x86Reg,&VI_H_SYNC_REG); break;
		case 0x04400020: MoveX86regToVariable(x86Reg,&VI_LEAP_REG); break;
		case 0x04400024: MoveX86regToVariable(x86Reg,&VI_H_START_REG); break;
		case 0x04400028: MoveX86regToVariable(x86Reg,&VI_V_START_REG); break;
		case 0x0440002C: MoveX86regToVariable(x86Reg,&VI_V_BURST_REG); break;
		case 0x04400030: MoveX86regToVariable(x86Reg,&VI_X_SCALE_REG); break;
		case 0x04400034: MoveX86regToVariable(x86Reg,&VI_Y_SCALE_REG); break;
		}
		break;
	case 0x04500000: /* AI registers */
		switch (Addr) {
		case 0x04500000: MoveX86regToVariable(x86Reg,&AI_DRAM_ADDR_REG); break;
		case 0x04500004:
			MoveX86regToVariable(x86Reg,&AI_LEN_REG);
			Pushad();

			PushImm32((uint32_t)audio);
			Call_Direct(GetAddress(0, &Audio::AiLenChanged));
			AddConstToX86Reg(x86_ESP, 4);

			Popad();
			break;
		case 0x04500008:
			MoveX86regToVariable(x86Reg,&AI_CONTROL_REG);
			AndConstToVariable(1,&AI_CONTROL_REG);
		case 0x0450000C:
			/* Clear Interrupt */;
			AndConstToVariable(~MI_INTR_AI,&MI_INTR_REG);
			AndConstToVariable(~MI_INTR_AI,&AudioIntrReg);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		case 0x04500010:
			Addr|=0xa0000000;
			TranslateVaddr(&Addr);
			MoveX86regToVariable(x86Reg,Addr + N64MEM);
			break;
		default:
			Addr|=0xa0000000;
			TranslateVaddr(&Addr);
			MoveX86regToVariable(x86Reg,Addr + N64MEM);
			break;
		}
		break;
	case 0x04600000:
		switch (Addr) {
		case 0x04600000: MoveX86regToVariable(x86Reg,&PI_DRAM_ADDR_REG); break;
		case 0x04600004: MoveX86regToVariable(x86Reg,&PI_CART_ADDR_REG); break;
		case 0x04600008:
			MoveX86regToVariable(x86Reg,&PI_RD_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::PI_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x0460000C:
			MoveX86regToVariable(x86Reg,&PI_WR_LEN_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::PI_DMA_WRITE));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04600010:
			AndConstToVariable(~MI_INTR_PI,&MI_INTR_REG);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
			MoveX86regToVariable(x86Reg,&VI_ORIGIN_REG);
			AndConstToVariable(0xFFFFFF,&VI_ORIGIN_REG);
		case 0x04600014:
			MoveX86regToVariable(x86Reg,&PI_DOMAIN1_REG);
			AndConstToVariable(0xFF,&PI_DOMAIN1_REG);
			break;
		case 0x04600018:
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PWD_REG);
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PWD_REG);
			break;
		case 0x0460001C:
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_PGS_REG);
			AndConstToVariable(0xFF,&PI_BSD_DOM1_PGS_REG);
			break;
		case 0x04600020:
			MoveX86regToVariable(x86Reg,&PI_BSD_DOM1_RLS_REG);
			AndConstToVariable(0xFF,&PI_BSD_DOM1_RLS_REG);
			break;
		}
		break;
	case 0x04700000:
		switch (Addr) {
		case 0x04700010: MoveX86regToVariable(x86Reg,&RI_REFRESH_REG); break;
		}
		break;
	case 0x04800000:
		switch (Addr) {
		case 0x04800000: MoveX86regToVariable(x86Reg,&SI_DRAM_ADDR_REG); break;
		case 0x04800004:
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_RD64B_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SI_DMA_READ));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04800010:
			MoveX86regToVariable(x86Reg,&SI_PIF_ADDR_WR64B_REG);
			Pushad();
			PushImm32((uint32_t)dynamic_cast<Cpu*>(this));
			Call_Direct(GetAddress(0, &Cpu::SI_DMA_WRITE));
			AddConstToX86Reg(x86_ESP, 4);
			Popad();
			break;
		case 0x04800018:
			AndConstToVariable(~MI_INTR_SI,&MI_INTR_REG);
			AndConstToVariable(~SI_STATUS_INTERRUPT,&SI_STATUS_REG);
			Pushad();
			CALL_CPUFUNC(CheckInterrupts);
			Popad();
			break;
		}
		break;
	case 0x1FC00000:
	Addr|=0xa0000000;
			TranslateVaddr(&Addr);

		MoveX86regToVariable(x86Reg,Addr + N64MEM);
		break;
	}
}



#endif
