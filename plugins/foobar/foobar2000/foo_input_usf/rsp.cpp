
#include <stdlib.h>
#include <stdint.h>
#include "usf.h"
#include "main.h"

#include "rsp.h"


#include "audio.h"
#include "cpu.h"

#include <windows.h>

#include "rsp_recompiler_cpu.h"

#define RSP_RECOMPMEM_SIZE		0x400000
#define RSP_SECRECOMPMEM_SIZE	0x200000

int32_t Rsp::RSPAllocateMemory (void)
{

	RSPRecompCode=(uint8_t *) VirtualAlloc( 0, RSP_RECOMPMEM_SIZE + RSP_SECRECOMPMEM_SIZE, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(RSPRecompCode == NULL) {
		DisplayError("Not enough memory for RSP RSPRecompCode!");
		return 0;
	}


	RSPRecompCodeSecondary = RSPRecompCode + RSP_RECOMPMEM_SIZE;

	RSPJumpTables = (uint8_t *)VirtualAlloc( 0, 0x2000 * MaxMaps, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
	if( RSPJumpTables == NULL ) {
		DisplayError("Not enough memory for Jump Table!");
		return 0;
	}

	memset((uint8_t*)RSPJumpTables, 0, 0x2000 * MaxMaps);
	memset((uint8_t*)RSPRecompCode, 0, 0x00400000);
	memset((uint8_t*)RSPRecompCodeSecondary, 0, 0x00200000);

	RSPJumpTable = (void **)RSPJumpTables;
	RSPRecompPos = RSPRecompCode;
	NoOfMaps = 0;
	return 1;
}

int32_t Rsp::Run(uint32_t cycles)
{

	if(cpu.usf.RspMode == RSP_INTERPRETER)
	{
		RSP_Running = 1;

		while(RSP_Running)
		{
			int32_t last = -1, count = 0, el, del;

			RSP_LW_IMEM(*PrgCount, &RSPOpC.Hex);

			(this->*RSP_Opcode[ RSPOpC.op ])();

			switch (RSP_NextInstruction) {
			case NORMAL:
				*PrgCount = (*PrgCount + 4) & 0xFFC;
				break;
			case DELAY_SLOT:
				RSP_NextInstruction = JUMP;
				*PrgCount = (*PrgCount + 4) & 0xFFC;
				break;
			case JUMP:
				RSP_NextInstruction = NORMAL;

				*PrgCount  = RSP_JumpTo;
				break;
			}
		}

		*PrgCount -= 4;

		return cycles;
	}
	else
	{
		return RunRecompilerCPU(cycles);
	}

}

void Rsp::RSP_SP_DMA_READ (void) {
	uint32_t i, j, Length, Skip, Count;
	uint8_t *Dest, *Source;

	cpu.SP_DRAM_ADDR_REG &= 0x00FFFFFF;

	if (cpu.SP_DRAM_ADDR_REG > 0x800000) {
		////printf("SP DMA READ\nSP_DRAM_ADDR_REG not in RDRam space");
		return;
	}

	if ((cpu.SP_RD_LEN_REG & 0xFFF) + 1  + (cpu.SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
		////printf("SP DMA READ\ncould not fit copy in memory segement\nSP_RD_LEN_REG=%08x\nSP_MEM_ADDR_REG=%08x",cpu.SP_RD_LEN_REG,cpu.SP_MEM_ADDR_REG);
		return;
	}

	Length = ((cpu.SP_RD_LEN_REG & 0xFFF) | 7) + 1;
	Skip = (cpu.SP_RD_LEN_REG >> 20) + Length;
	Count = ((cpu.SP_RD_LEN_REG >> 12) & 0xFF)  + 1;

	if ((cpu.SP_MEM_ADDR_REG & 0x1000) != 0) {
		Dest = cpu.IMEM + ((cpu.SP_MEM_ADDR_REG & 0x0FFF) & ~7);
	} else {
		Dest = cpu.DMEM + ((cpu.SP_MEM_ADDR_REG & 0x0FFF) & ~7);
	}
	Source = cpu.N64MEM + (cpu.SP_DRAM_ADDR_REG & ~7);

#if defined(RSP_SAFE_DMA)
	for (j = 0 ; j < Count; j++) {
		for (i = 0 ; i < Length; i++) {
			*(uint8_t *)(((uint32_t)Dest + j * Length + i) ^ 3) = *(uint8_t *)(((uint32_t)Source + j * Skip + i) ^ 3);
		}
	}
#else
	if ((Skip & 0x3) == 0) {
		for (j = 0; j < Count; j++) {
			memcpy(Dest, Source, Length);
			Source += Skip;
			Dest += Length;
		}
	} else {
		for (j = 0 ; j < Count; j++) {
			for (i = 0 ; i < Length; i++) {
				*(uint8_t *)(((uint32_t)Dest + i) ^ 3) = *(uint8_t *)(((uint32_t)Source + i) ^ 3);
			}
			Source += Skip;
			Dest += Length;
		}
	}
#endif
	if ((cpu.usf.RspMode == RSP_RECOMPILER) && ((cpu.SP_MEM_ADDR_REG & 0x1000) != 0)) {
		RSPSetJumpTable();
	}



	cpu.SP_DMA_BUSY_REG = 0;
	cpu.SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void Rsp::RSP_SP_DMA_WRITE (void) {
	uint32_t i, j, Length, Skip, Count;
	uint8_t *Dest, *Source;

	cpu.SP_DRAM_ADDR_REG &= 0x00FFFFFF;

	if (cpu.SP_DRAM_ADDR_REG > 0x800000) {
		////printf("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
		return;
	}

	if ((cpu.SP_WR_LEN_REG & 0xFFF) + 1  + (cpu.SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
		////printf("SP DMA READ\ncould not fit copy in memory segement\nSP_WR_LEN_REG=%08x\nSP_MEM_ADDR_REG=%08x",cpu.SP_WR_LEN_REG,cpu.SP_MEM_ADDR_REG);
		return;
	}

	Length = ((cpu.SP_WR_LEN_REG & 0xFFF) | 7) + 1;
	Skip = (cpu.SP_WR_LEN_REG >> 20) + Length;
	Count = ((cpu.SP_WR_LEN_REG >> 12) & 0xFF)  + 1;
	Dest = cpu.N64MEM + (cpu.SP_DRAM_ADDR_REG & ~7);
	Source = cpu.DMEM + ((cpu.SP_MEM_ADDR_REG & 0x1FFF) & ~7);

#if defined(RSP_SAFE_DMA)
	for (j = 0 ; j < Count; j++) {
		for (i = 0 ; i < Length; i++) {
			*(uint8_t *)(((uint32_t)Dest + j * Skip + i) ^ 3) = *(uint8_t *)(((uint32_t)Source + j * Length + i) ^ 3);
		}
	}
#else
	if ((Skip & 0x3) == 0) {
		for (j = 0; j < Count; j++) {
			memcpy(Dest, Source, Length);
			Source += Length;
			Dest += Skip;
		}
	} else {
		for (j = 0 ; j < Count; j++) {
			for (i = 0 ; i < Length; i++) {
				*(uint8_t *)(((uint32_t)Dest + i) ^ 3) = *(uint8_t *)(((uint32_t)Source + i) ^ 3);
			}
			Source += Length;
			Dest += Skip;
		}
	}
#endif
	cpu.SP_DMA_BUSY_REG = 0;
	cpu.SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void Rsp::RSP_LB_DMEM ( uint32_t Addr, uint8_t * Value ) {
	* Value = *(uint8_t *)(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) ;
}

void Rsp::RSP_LBV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	RSP_Vect[vect].B[15 - element] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
}

void Rsp::RSP_LDV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count;

	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void Rsp::RSP_LFV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, count;
	VECTOR Temp;

	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}

	Temp.HW[7] = *(cpu.DMEM + (((Addr + element) ^3) & 0xFFF)) << 7;
	Temp.HW[6] = *(cpu.DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[5] = *(cpu.DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[4] = *(cpu.DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[3] = *(cpu.DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[2] = *(cpu.DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[1] = *(cpu.DMEM + (((Addr + ((0x10 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[0] = *(cpu.DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;

	for (count = element; count < (length + element); count ++ ){
		RSP_Vect[vect].B[15 - count] = Temp.B[15 - count];
	}
}

void Rsp::RSP_LH_DMEM ( uint32_t Addr, uint16_t * Value ) {
	if ((Addr & 0x1) != 0) {
		if (Addr > 0xFFE) {
			////printf("hmmmm.... Problem with:\nRSP_LH_DMEM");
			return;
		}
		Addr &= 0xFFF;
		*Value = *(uint8_t *)(cpu.DMEM + (Addr^ 3)) << 8;
		*Value += *(uint8_t *)(cpu.DMEM + ((Addr + 1)^ 3));
		return;
	}
	* Value = *(uint16_t *)(cpu.DMEM + ((Addr ^ 2 ) & 0xFFF));

}

void Rsp::RSP_LHV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	RSP_Vect[vect].HW[7] = *(cpu.DMEM + ((Addr + ((0x10 - element) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(cpu.DMEM + ((Addr + ((0x10 - element + 2) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(cpu.DMEM + ((Addr + ((0x10 - element + 4) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(cpu.DMEM + ((Addr + ((0x10 - element + 6) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(cpu.DMEM + ((Addr + ((0x10 - element + 8) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(cpu.DMEM + ((Addr + ((0x10 - element + 10) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(cpu.DMEM + ((Addr + ((0x10 - element + 12) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(cpu.DMEM + ((Addr + ((0x10 - element + 14) & 0xF) ^3) & 0xFFF)) << 7;
}

void Rsp::RSP_LLV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count;

	length = 4;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void Rsp::RSP_LPV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	RSP_Vect[vect].HW[7] = *(cpu.DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[6] = *(cpu.DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[5] = *(cpu.DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[4] = *(cpu.DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[3] = *(cpu.DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[2] = *(cpu.DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[1] = *(cpu.DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[0] = *(cpu.DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 8;
}

void Rsp::RSP_LRV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count, offset;

	offset = (Addr & 0xF) - 1;
	length = (Addr & 0xF) - element;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[offset - Count] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void Rsp::RSP_LQV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count;

	length = ((Addr + 0x10) & ~0xF) - Addr;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void Rsp::RSP_LSV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count;

	length = 2;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(cpu.DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}
}

void Rsp::RSP_LTV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t del, count, length;

	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}

	Addr = ((Addr + 8) & 0xFF0) + (element & 0x1);
	for (count = 0; count < length; count ++) {
		del = ((8 - (element >> 1) + count) << 1) & 0xF;
		RSP_Vect[vect + count].B[15 - del] = *(cpu.DMEM + (Addr ^ 3));
		RSP_Vect[vect + count].B[14 - del] = *(cpu.DMEM + ((Addr + 1) ^ 3));
		Addr += 2;
	}
}

void Rsp::RSP_LUV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	RSP_Vect[vect].HW[7] = *(cpu.DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(cpu.DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(cpu.DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(cpu.DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(cpu.DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(cpu.DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(cpu.DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(cpu.DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 7;
}

void Rsp::RSP_LW_DMEM ( uint32_t Addr, uint32_t * Value ) {
	if ((Addr & 0x3) != 0) {
		Addr &= 0xFFF;
		if (Addr > 0xFFC) {
			////printf("hmmmm.... Problem with:\nRSP_LW_DMEM");
			return;
		}
		*Value = *(uint8_t *)(cpu.DMEM + (Addr^ 3)) << 0x18;
		*Value += *(uint8_t *)(cpu.DMEM + ((Addr + 1)^ 3)) << 0x10;
		*Value += *(uint8_t *)(cpu.DMEM + ((Addr + 2)^ 3)) << 8;
		*Value += *(uint8_t *)(cpu.DMEM + ((Addr + 3)^ 3));
		return;
	}
	* Value = *(uint32_t *)(cpu.DMEM + (Addr & 0xFFF));
}

void Rsp::RSP_LW_IMEM ( uint32_t Addr, uint32_t * Value ) {
	if ((Addr & 0x3) != 0) {
		//printf("Unaligned RSP_LW_IMEM");
	}
	* Value = *(uint32_t *)(cpu.IMEM + (Addr & 0xFFF));
}

void Rsp::RSP_SB_DMEM ( uint32_t Addr, uint8_t Value ) {
	*(uint8_t *)(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = Value;
}

void Rsp::RSP_SBV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - element];
}

void Rsp::RSP_SDV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void Rsp::RSP_SFV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t offset = Addr & 0xF;
	Addr &= 0xFF0;

	switch (element) {
	case 0:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		break;
	case 1:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		break;
	case 2:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 3:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF)^3))) = 0;
		break;
	case 4:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[7] >> 7;
		break;
	case 5:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		break;
	case 6:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 7:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 8:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		break;
	case 9:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 10:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 11:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[4] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		break;
	case 12:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		break;
	case 13:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 14:
		*(cpu.DMEM + ((Addr + offset)^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 15:
		*(cpu.DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(cpu.DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		break;
	}
}

void Rsp::RSP_SH_DMEM ( uint32_t Addr, uint16_t Value ) {
	if ((Addr & 0x1) != 0) {
		//printf("Unaligned RSP_SH_DMEM");
		return;
	}
	*(uint16_t *)(cpu.DMEM + ((Addr ^ 2) & 0xFFF)) = Value;
}

void Rsp::RSP_SHV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	*(cpu.DMEM + ((Addr^3) & 0xFFF)) = (RSP_Vect[vect].UB[(15 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(14 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 2)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(13 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(12 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 4)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(11 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(10 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 6)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(9 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(8 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 8)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(7 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(6 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 10)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(5 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(4 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 12)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(3 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(2 - element) & 0xF] >> 7);
	*(cpu.DMEM + (((Addr + 14)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(1 - element) & 0xF] << 1) +
		(RSP_Vect[vect].UB[(0 - element) & 0xF] >> 7);
}

void Rsp::RSP_SLV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count;

	for (Count = element; Count < (4 + element); Count ++ ){
		*(cpu.DMEM + ((Addr ^3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void Rsp::RSP_SPV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0xF) << 1)];
		} else {
			*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = (RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7);
		}
		Addr += 1;
	}
}

void Rsp::RSP_SQV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count;

	length = ((Addr + 0x10) & ~0xF) - Addr;
	for (Count = element; Count < (length + element); Count ++ ){
		*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void Rsp::RSP_SRV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t length, Count, offset;

	length = (Addr & 0xF);
	offset = (0x10 - length) & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - ((Count + offset) & 0xF)];
		Addr += 1;
	}
}

void Rsp::RSP_SSV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count;

	for (Count = element; Count < (2 + element); Count ++ ){
		*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void Rsp::RSP_STV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t del, count, length;

	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	length = length << 1;
	del = element >> 1;
	for (count = 0; count < length; count += 2) {
		*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[15 - count];
		*(cpu.DMEM + (((Addr + 1) ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[14 - count];
		del = (del + 1) & 7;
		Addr += 2;
	}
}

void Rsp::RSP_SUV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = (RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7);
		} else {
			*(cpu.DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)];
		}
		Addr += 1;
	}
}

void Rsp::RSP_SW_DMEM ( uint32_t Addr, uint32_t Value ) {
	Addr &= 0xFFF;

	if ((Addr & 0x3) != 0) {
		if (Addr > 0xFFC) {
			//printf("hmmmm.... Problem with:\nRSP_SW_DMEM");
			return;
		}
		*(uint8_t *)(cpu.DMEM + (Addr ^ 3)) = (uint8_t)(Value >> 0x18);
		*(uint8_t *)(cpu.DMEM + ((Addr + 1) ^ 3)) = (uint8_t)(Value >> 0x10);
		*(uint8_t *)(cpu.DMEM + ((Addr + 2) ^ 3)) = (uint8_t)(Value >> 0x8);
		*(uint8_t *)(cpu.DMEM + ((Addr + 3) ^ 3)) = (uint8_t)(Value);
		return;
	}
	*(uint32_t *)(cpu.DMEM + Addr) = Value;
}

void Rsp::RSP_SWV_DMEM ( uint32_t Addr, int32_t vect, int32_t element ) {
	int32_t Count, offset;

	offset = Addr & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (16 + element); Count ++ ){
		*(cpu.DMEM + ((Addr + (offset & 0xF)) ^ 3)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		offset += 1;
	}
}

void Rsp::RSPSetJumpTable (void)
{
	uint32_t CRC, count;

	CRC = 0;
	for (count = 0; count < 0x800; count += 0x40) {
		CRC += *(uint32_t *)(cpu.IMEM + count);
	}

	for (count = 0; count <	NoOfMaps; count++ ) {
		if (CRC == MapsCRC[count]) {
			RSPJumpTable = (void **)(RSPJumpTables + (count * (sizeof(void*) * 0x400)));
			Table = count;
			return;
		}
	}

	if (NoOfMaps == MaxMaps) {
		DisplayError("Used up all the Jump tables in the rsp");
		ExitThread(0);
	}

	MapsCRC[NoOfMaps] = CRC;

	RSPJumpTable = (void **)(RSPJumpTables + (NoOfMaps * (sizeof(void*) * 0x400) ));
	Table = NoOfMaps;
	NoOfMaps += 1;
}


Rsp::Rsp(Cpu & mCpu) : cpu(mCpu)
{
	ESP_RegSave = 0, EBP_RegSave = 0;
	RSPBranchCompare = 0;

	RSPBlockID = 0;
	dwBuffer = MainBuffer;

	VXOR_DynaRegCount = 0;

	pLastSecondary = pLastPrimary = NULL;

}

void Rsp::InitRsp(void)
{
	int32_t i = 0;

	RSP_Running = 0;

	memset(RSP_GPR, 0, sizeof(RSP_GPR));
	memset(RSP_Vect, 0, sizeof(RSP_Vect));

	for(i = 0; i < 64; i++) {
		RSP_Opcode[i]= &Rsp::rsp_UnknownOpcode;
		RSP_RegImm[i>>1]= &Rsp::rsp_UnknownOpcode;
		RSP_Special[i]= &Rsp::rsp_UnknownOpcode;
		RSP_Cop0[i>>1]= &Rsp::rsp_UnknownOpcode;
		RSP_Cop2[i>>1]= &Rsp::rsp_UnknownOpcode;
		RSP_Vector[i] = &Rsp::rsp_UnknownOpcode;
		RSP_Lc2[i>>1] = &Rsp::rsp_UnknownOpcode;
		RSP_Sc2[i>>1] = &Rsp::rsp_UnknownOpcode;
	}


	RSP_Opcode[ 0] = &Rsp::RSP_Opcode_SPECIAL;
	RSP_Opcode[ 1] = &Rsp::RSP_Opcode_REGIMM;
	RSP_Opcode[ 2] = &Rsp::RSP_Opcode_J;
	RSP_Opcode[ 3] = &Rsp::RSP_Opcode_JAL;
	RSP_Opcode[ 4] = &Rsp::RSP_Opcode_BEQ;
	RSP_Opcode[ 5] = &Rsp::RSP_Opcode_BNE;
	RSP_Opcode[ 6] = &Rsp::RSP_Opcode_BLEZ;
	RSP_Opcode[ 7] = &Rsp::RSP_Opcode_BGTZ;
	RSP_Opcode[ 8] = &Rsp::RSP_Opcode_ADDI;
	RSP_Opcode[ 9] = &Rsp::RSP_Opcode_ADDIU;
	RSP_Opcode[10] = &Rsp::RSP_Opcode_SLTI;
	RSP_Opcode[11] = &Rsp::RSP_Opcode_SLTIU;
	RSP_Opcode[12] = &Rsp::RSP_Opcode_ANDI;
	RSP_Opcode[13] = &Rsp::RSP_Opcode_ORI;
	RSP_Opcode[14] = &Rsp::RSP_Opcode_XORI;
	RSP_Opcode[15] = &Rsp::RSP_Opcode_LUI;
	RSP_Opcode[16] = &Rsp::RSP_Opcode_COP0;
	RSP_Opcode[18] = &Rsp::RSP_Opcode_COP2;
	RSP_Opcode[32] = &Rsp::RSP_Opcode_LB;
	RSP_Opcode[33] = &Rsp::RSP_Opcode_LH;
	RSP_Opcode[35] = &Rsp::RSP_Opcode_LW;
	RSP_Opcode[36] = &Rsp::RSP_Opcode_LBU;
	RSP_Opcode[37] = &Rsp::RSP_Opcode_LHU;
	RSP_Opcode[40] = &Rsp::RSP_Opcode_SB;
	RSP_Opcode[41] = &Rsp::RSP_Opcode_SH;
	RSP_Opcode[43] = &Rsp::RSP_Opcode_SW;
	RSP_Opcode[50] = &Rsp::RSP_Opcode_LC2;
	RSP_Opcode[58] = &Rsp::RSP_Opcode_SC2;

	RSP_Special[ 0] = &Rsp::RSP_Special_SLL;
	RSP_Special[ 2] = &Rsp::RSP_Special_SRL;
	RSP_Special[ 3] = &Rsp::RSP_Special_SRA;
	RSP_Special[ 4] = &Rsp::RSP_Special_SLLV;
	RSP_Special[ 6] = &Rsp::RSP_Special_SRLV;
	RSP_Special[ 7] = &Rsp::RSP_Special_SRAV;
	RSP_Special[ 8] = &Rsp::RSP_Special_JR;
	RSP_Special[ 9] = &Rsp::RSP_Special_JALR;
	RSP_Special[13] = &Rsp::RSP_Special_BREAK;
	RSP_Special[32] = &Rsp::RSP_Special_ADD;
	RSP_Special[33] = &Rsp::RSP_Special_ADDU;
	RSP_Special[34] = &Rsp::RSP_Special_SUB;
	RSP_Special[35] = &Rsp::RSP_Special_SUBU;
	RSP_Special[36] = &Rsp::RSP_Special_AND;
	RSP_Special[37] = &Rsp::RSP_Special_OR;
	RSP_Special[38] = &Rsp::RSP_Special_XOR;
	RSP_Special[39] = &Rsp::RSP_Special_NOR;
	RSP_Special[42] = &Rsp::RSP_Special_SLT;
	RSP_Special[43] = &Rsp::RSP_Special_SLTU;

	RSP_RegImm[ 0] = &Rsp::RSP_Opcode_BLTZ;
	RSP_RegImm[ 1] = &Rsp::RSP_Opcode_BGEZ;
	RSP_RegImm[16] = &Rsp::RSP_Opcode_BLTZAL;

	RSP_Cop0[ 0] = &Rsp::RSP_Cop0_MF;
	RSP_Cop0[ 4] = &Rsp::RSP_Cop0_MT;

	RSP_Cop2[ 0] = &Rsp::RSP_Cop2_MF;
	RSP_Cop2[ 2] = &Rsp::RSP_Cop2_CF;
	RSP_Cop2[ 4] = &Rsp::RSP_Cop2_MT;
	RSP_Cop2[ 6] = &Rsp::RSP_Cop2_CT;
	RSP_Cop2[16] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[17] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[18] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[19] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[20] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[21] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[22] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[23] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[24] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[25] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[26] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[27] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[28] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[29] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[30] = &Rsp::RSP_COP2_VECTOR;
	RSP_Cop2[31] = &Rsp::RSP_COP2_VECTOR;

	RSP_Vector[ 0] = &Rsp::RSP_Vector_VMULF;
	RSP_Vector[ 1] = &Rsp::RSP_Vector_VMULU;
	RSP_Vector[ 4] = &Rsp::RSP_Vector_VMUDL;
	RSP_Vector[ 5] = &Rsp::RSP_Vector_VMUDM;
	RSP_Vector[ 6] = &Rsp::RSP_Vector_VMUDN;
	RSP_Vector[ 7] = &Rsp::RSP_Vector_VMUDH;
	RSP_Vector[ 8] = &Rsp::RSP_Vector_VMACF;
	RSP_Vector[ 9] = &Rsp::RSP_Vector_VMACU;
	RSP_Vector[11] = &Rsp::RSP_Vector_VMACQ;
	RSP_Vector[12] = &Rsp::RSP_Vector_VMADL;
	RSP_Vector[13] = &Rsp::RSP_Vector_VMADM;
	RSP_Vector[14] = &Rsp::RSP_Vector_VMADN;
	RSP_Vector[15] = &Rsp::RSP_Vector_VMADH;
	RSP_Vector[16] = &Rsp::RSP_Vector_VADD;
	RSP_Vector[17] = &Rsp::RSP_Vector_VSUB;
	RSP_Vector[19] = &Rsp::RSP_Vector_VABS;
	RSP_Vector[20] = &Rsp::RSP_Vector_VADDC;
	RSP_Vector[21] = &Rsp::RSP_Vector_VSUBC;
	RSP_Vector[29] = &Rsp::RSP_Vector_VSAW;
	RSP_Vector[32] = &Rsp::RSP_Vector_VLT;
	RSP_Vector[33] = &Rsp::RSP_Vector_VEQ;
	RSP_Vector[34] = &Rsp::RSP_Vector_VNE;
	RSP_Vector[35] = &Rsp::RSP_Vector_VGE;
	RSP_Vector[36] = &Rsp::RSP_Vector_VCL;
	RSP_Vector[37] = &Rsp::RSP_Vector_VCH;
	RSP_Vector[38] = &Rsp::RSP_Vector_VCR;
	RSP_Vector[39] = &Rsp::RSP_Vector_VMRG;
	RSP_Vector[40] = &Rsp::RSP_Vector_VAND;
	RSP_Vector[41] = &Rsp::RSP_Vector_VNAND;
	RSP_Vector[42] = &Rsp::RSP_Vector_VOR;
	RSP_Vector[43] = &Rsp::RSP_Vector_VNOR;
	RSP_Vector[44] = &Rsp::RSP_Vector_VXOR;
	RSP_Vector[45] = &Rsp::RSP_Vector_VNXOR;
	RSP_Vector[48] = &Rsp::RSP_Vector_VRCP;
	RSP_Vector[49] = &Rsp::RSP_Vector_VRCPL;
	RSP_Vector[50] = &Rsp::RSP_Vector_VRCPH;
	RSP_Vector[51] = &Rsp::RSP_Vector_VMOV;
	RSP_Vector[52] = &Rsp::RSP_Vector_VRSQ;
	RSP_Vector[53] = &Rsp::RSP_Vector_VRSQL;
	RSP_Vector[54] = &Rsp::RSP_Vector_VRSQH;
	RSP_Vector[55] = &Rsp::RSP_Vector_VNOOP;

	RSP_Lc2[ 0] = &Rsp::RSP_Opcode_LBV;
	RSP_Lc2[ 1] = &Rsp::RSP_Opcode_LSV;
	RSP_Lc2[ 2] = &Rsp::RSP_Opcode_LLV;
	RSP_Lc2[ 3] = &Rsp::RSP_Opcode_LDV;
	RSP_Lc2[ 4] = &Rsp::RSP_Opcode_LQV;
	RSP_Lc2[ 5] = &Rsp::RSP_Opcode_LRV;
	RSP_Lc2[ 6] = &Rsp::RSP_Opcode_LPV;
	RSP_Lc2[ 7] = &Rsp::RSP_Opcode_LUV;
	RSP_Lc2[ 8] = &Rsp::RSP_Opcode_LHV;
	RSP_Lc2[ 9] = &Rsp::RSP_Opcode_LFV;
	RSP_Lc2[11] = &Rsp::RSP_Opcode_LTV;

	RSP_Sc2[ 0] = &Rsp::RSP_Opcode_SBV;
	RSP_Sc2[ 1] = &Rsp::RSP_Opcode_SSV;
	RSP_Sc2[ 2] = &Rsp::RSP_Opcode_SLV;
	RSP_Sc2[ 3] = &Rsp::RSP_Opcode_SDV;
	RSP_Sc2[ 4] = &Rsp::RSP_Opcode_SQV;
	RSP_Sc2[ 5] = &Rsp::RSP_Opcode_SRV;
	RSP_Sc2[ 6] = &Rsp::RSP_Opcode_SPV;
	RSP_Sc2[ 7] = &Rsp::RSP_Opcode_SUV;
	RSP_Sc2[ 8] = &Rsp::RSP_Opcode_SHV;
	RSP_Sc2[ 9] = &Rsp::RSP_Opcode_SFV;
	RSP_Sc2[10] = &Rsp::RSP_Opcode_SWV;
	RSP_Sc2[11] = &Rsp::RSP_Opcode_STV;

	EleSpec[ 0].DW = 0;
	EleSpec[ 1].DW = 0;
	EleSpec[ 2].DW = 0;
	EleSpec[ 3].DW = 0;
	EleSpec[ 4].DW = 0;
	EleSpec[ 5].DW = 0;
	EleSpec[ 6].DW = 0;
	EleSpec[ 7].DW = 0;
	EleSpec[ 8].DW = 0;
	EleSpec[ 9].DW = 0;
	EleSpec[10].DW = 0;
	EleSpec[11].DW = 0;
	EleSpec[12].DW = 0;
	EleSpec[13].DW = 0;
	EleSpec[14].DW = 0;
	EleSpec[15].DW = 0;
	EleSpec[16].DW = 0x0001020304050607ULL; /* None */
	EleSpec[17].DW = 0x0001020304050607ULL; /* None */
	EleSpec[18].DW = 0x0000020204040606ULL; /* 0q */
	EleSpec[19].DW = 0x0101030305050707ULL; /* 1q */
	EleSpec[20].DW = 0x0000000004040404ULL; /* 0h */
	EleSpec[21].DW = 0x0101010105050505ULL; /* 1h */
	EleSpec[22].DW = 0x0202020206060606ULL; /* 2h */
	EleSpec[23].DW = 0x0303030307070707ULL; /* 3h */
	EleSpec[24].DW = 0x0000000000000000ULL; /* 0 */
	EleSpec[25].DW = 0x0101010101010101ULL; /* 1 */
	EleSpec[26].DW = 0x0202020202020202ULL; /* 2 */
	EleSpec[27].DW = 0x0303030303030303ULL; /* 3 */
	EleSpec[28].DW = 0x0404040404040404ULL; /* 4 */
	EleSpec[29].DW = 0x0505050505050505ULL; /* 5 */
	EleSpec[30].DW = 0x0606060606060606ULL; /* 6 */
	EleSpec[31].DW = 0x0707070707070707ULL; /* 7 */

	Indx[ 0].DW = 0;
	Indx[ 1].DW = 0;
	Indx[ 2].DW = 0;
	Indx[ 3].DW = 0;
	Indx[ 4].DW = 0;
	Indx[ 5].DW = 0;
	Indx[ 6].DW = 0;
	Indx[ 7].DW = 0;
	Indx[ 8].DW = 0;
	Indx[ 9].DW = 0;
	Indx[10].DW = 0;
	Indx[11].DW = 0;
	Indx[12].DW = 0;
	Indx[13].DW = 0;
	Indx[14].DW = 0;
	Indx[15].DW = 0;

	Indx[16].DW = 0x0001020304050607ULL; /* None */
	Indx[17].DW = 0x0001020304050607ULL; /* None */
	Indx[18].DW = 0x0103050700020406ULL; /* 0q */
	Indx[19].DW = 0x0002040601030507ULL; /* 1q */
	Indx[20].DW = 0x0102030506070004ULL; /* 0h */
	Indx[21].DW = 0x0002030406070105ULL; /* 1h */
	Indx[22].DW = 0x0001030405070206ULL; /* 2h */
	Indx[23].DW = 0x0001020405060307ULL; /* 3h */
	Indx[24].DW = 0x0102030405060700ULL; /* 0 */
	Indx[25].DW = 0x0002030405060701ULL; /* 1 */
	Indx[26].DW = 0x0001030405060702ULL; /* 2 */
	Indx[27].DW = 0x0001020405060703ULL; /* 3 */
	Indx[28].DW = 0x0001020305060704ULL; /* 4 */
	Indx[29].DW = 0x0001020304060705ULL; /* 5 */
	Indx[30].DW = 0x0001020304050706ULL; /* 6 */
	Indx[31].DW = 0x0001020304050607ULL; /* 7 */

	for (i = 16; i < 32; i ++) {
		int32_t count;

		for (count = 0; count < 8; count ++) {
			Indx[i].B[count] = 7 - Indx[i].B[count];
			EleSpec[i].B[count] = 7 - EleSpec[i].B[count];
		}
		for (count = 0; count < 4; count ++) {
			uint8_t Temp;

			Temp = Indx[i].B[count];
			Indx[i].B[count] = Indx[i].B[7 - count];
			Indx[i].B[7 - count] = Temp;
		}
	}

	PrgCount = &cpu.SP_PC_REG;

	memset(&Compiler, 0, sizeof(Compiler));

	Compiler.bAlignGPR = 1;
	Compiler.bAlignVector = 1;
	Compiler.bFlags = 1;
	Compiler.bReOrdering = 1;
	Compiler.bSections = 0; //do i really need to do this?
	Compiler.bDest = 1;
	Compiler.bAccum = 1;
	Compiler.bGPRConstants = 1;

	if(cpu.usf.RspMode == RSP_RECOMPILER)
		RSPAllocateMemory();

	memset(RSP_GPR,0,sizeof(RSP_GPR));
	memset(RSP_Vect,0,sizeof(RSP_Vect));

	uint32_t Cpu_Features = 0;

	_asm
	{
		mov eax, 1
		cpuid
		mov Cpu_Features, eax
	}

	Compiler.mmx2 = 0;
	Compiler.sse = 0;
	Compiler.mmx = 0;


	if (Cpu_Features & 0x02000000) {
		Compiler.mmx2 = 1;
		Compiler.sse = 1;
	}
	if (Cpu_Features & 0x00800000) {
		Compiler.mmx = 1;
	}
	if (Cpu_Features & 0x40000000) {
		Compiler.mmx2 = 1;
	}
	if (Cpu_Features & 0x00008000) {
		ConditionalMove = 1;
	} else {
		ConditionalMove = 0;
	}

	DMEM = cpu.DMEM;

	if(cpu.usf.RspMode == RSP_RECOMPILER)
		BuildRecompilerCPU();
}

Rsp::~Rsp()
{
	VirtualFree( RSPRecompCode, 0, MEM_RELEASE);

	VirtualFree( RSPJumpTables, 0, MEM_RELEASE);

}

