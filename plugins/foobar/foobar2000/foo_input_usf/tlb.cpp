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
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cpu.h"


// map source in mem space to dest in N64MEM
void Cpu::MapReg(uint32_t source, uint32_t dest) {
	TLB_Map[source >> 12] = ((uint32_t)N64MEM + (dest & 0x1FFFFFFF)) - source;
}



uint32_t Cpu::UnmapReg(uint32_t source) {
   	// 0xa4000000-0xa4001000 -> 0x800000-0x801000
	if (source >= 0x800000 && source < 0x801000) return source-0x800000+0x04000000;
	// 0xa4001000-0xa4002000 -> 0x801000-0x802000
	if (source >= 0x801000 && source < 0x802000) return source-0x801000+0x04001000;
	// 0xa4004000-0xa4005000 -> 0x802000-0x803000
	if (source >= 0x802000 && source < 0x803000) return source-0x802000+0x04004000;
	// 0xa4008000-0xa4009000 -> 0x803000-0x804000
	if (source >= 0x803000 && source < 0x804000) return source-0x803000+0x04008000;
	// 0xa4100000-0xa4101000 -> 0x804000-0x805000
	if (source >= 0x804000 && source < 0x805000) return source-0x804000+0x04100000;
	// 0xa4200000-0xa4201000 -> 0x805000-0x806000
	if (source >= 0x805000 && source < 0x806000) return source-0x805000+0x04200000;
	// 0xa4300000-0xa4301000 -> 0x806000-0x807000
	if (source >= 0x806000 && source < 0x807000) return source-0x806000+0x04300000;
	// 0xa4400000-0xa4401000 -> 0x807000-0x808000
	if (source >= 0x807000 && source < 0x808000) return source-0x807000+0x04400000;
	// 0xa4500000-0xa4501000 -> 0x808000-0x809000
	if (source >= 0x808000 && source < 0x809000) return source-0x808000+0x04500000;
	// 0xa4600000-0xa4601000 -> 0x809000-0x80a000
	if (source >= 0x809000 && source < 0x80a000) return source-0x809000+0x04600000;
	// 0xa4700000-0xa4701000 -> 0x80a000-0x80b000
	if (source >= 0x80a000 && source < 0x80b000) return source-0x80a000+0x04700000;
	// 0xa4800000-0xa4801000 -> 0x80b000-0x80c000
	if (source >= 0x80b000 && source < 0x80c000) return source-0x80b000+0x04800000;
	// 0xbfc00000-0xbfc01000 -> 0x80c000-0x80d000
	if (source >= 0x80c000 && source < 0x80d000) return source-0x80c000+0x1fc00000;

	//DisplayError("fell through");

	return source;
}


uint32_t Cpu::AddressDefined ( uintptr_t VAddr) {
	uint32_t i;

	if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF) {
		return 1;
	}

	for (i = 0; i < 64; i++) {
		if (FastTlb[i].ValidEntry == 0) { continue; }
		if (VAddr >= FastTlb[i].VSTART && VAddr <= FastTlb[i].VEND) {
			return 1;
		}
	}
	return 0;
}

void Cpu::	InitilizeTLB (void) {
	uint32_t count;

	for (count = 0; count < 32; count++) { tlb[count].EntryDefined = 0; }
	for (count = 0; count < 64; count++) { FastTlb[count].ValidEntry = 0; }
	SetupTLB();
}

void Cpu::SetupTLB (void) {
	uint32_t count;

	memset(TLB_Map,0,(0xFFFFF * sizeof(uintptr_t)));
	for (count = 0x80000000; count < 0xC0000000; count += 0x1000) {
		TLB_Map[count >> 12] = ((uintptr_t)N64MEM + (count & 0x1FFFFFFF)) - count;
	}

#if 1
		// setup register ranges as follows (all uncommitted offsets in N64MEM)
	// 0xa4000000-0xa4001000 -> 0x800000-0x801000
	MapReg(0xa4000000,0x800000);
	MapReg(0x84000000,0x800000);
	// 0xa4001000-0xa4002000 -> 0x801000-0x802000
	MapReg(0xa4001000,0x801000);
	MapReg(0x84001000,0x801000);
	// 0xa4004000-0xa4005000 -> 0x802000-0x803000
	MapReg(0xa4004000,0x802000);
	MapReg(0x84004000,0x802000);
	// 0xa4008000-0xa4009000 -> 0x803000-0x804000
	MapReg(0xa4008000,0x803000);
	MapReg(0x84008000,0x803000);
	// 0xa4100000-0xa4101000 -> 0x804000-0x805000
	MapReg(0xa4100000,0x804000);
	MapReg(0x84100000,0x804000);
	// 0xa4200000-0xa4201000 -> 0x805000-0x806000
	MapReg(0xa4200000,0x805000);
	MapReg(0x84200000,0x805000);
	// 0xa4300000-0xa4301000 -> 0x806000-0x807000
	MapReg(0xa4300000,0x806000);
	MapReg(0x84300000,0x806000);
	// 0xa4400000-0xa4401000 -> 0x807000-0x808000
	MapReg(0xa4400000,0x807000);
	MapReg(0x84400000,0x807000);
	// 0xa4500000-0xa4501000 -> 0x808000-0x809000
	MapReg(0xa4500000,0x808000);
	MapReg(0x84500000,0x808000);
	// 0xa4600000-0xa4601000 -> 0x809000-0x80a000
	MapReg(0xa4600000,0x809000);
	MapReg(0x84600000,0x809000);
	// 0xa4700000-0xa4701000 -> 0x80a000-0x80b000
	MapReg(0xa4700000,0x80a000);
	MapReg(0x84700000,0x80a000);
	// 0xa4800000-0xa4801000 -> 0x80b000-0x80c000
	MapReg(0xa4800000,0x80b000);
	MapReg(0x84800000,0x80b000);
	// 0xbfc00000-0xbfc01000 -> 0x80c000-0x80d000
	MapReg(0xbfc00000,0x80c000);
	MapReg(0x9fc00000,0x80c000);
#endif
	for (count = 0; count < 32; count ++) { SetupTLB_Entry(count); }
}

void Cpu::SetupTLB_Entry (int Entry) {
	uint32_t FastIndx;


	if (!tlb[Entry].EntryDefined) { return; }
	FastIndx = Entry << 1;
	FastTlb[FastIndx].VSTART=tlb[Entry].EntryHi.VPN2 << 13;
	FastTlb[FastIndx].VEND = FastTlb[FastIndx].VSTART + (tlb[Entry].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].PHYSSTART = tlb[Entry].EntryLo0.PFN << 12;
	FastTlb[FastIndx].VALID = tlb[Entry].EntryLo0.V;
	FastTlb[FastIndx].DIRTY = tlb[Entry].EntryLo0.D;
	FastTlb[FastIndx].GLOBAL = tlb[Entry].EntryLo0.GLOBAL & tlb[Entry].EntryLo1.GLOBAL;
	FastTlb[FastIndx].ValidEntry = 0;

	FastIndx = (Entry << 1) + 1;
	FastTlb[FastIndx].VSTART=(tlb[Entry].EntryHi.VPN2 << 13) + ((tlb[Entry].PageMask.Mask << 12) + 0xFFF + 1);
	FastTlb[FastIndx].VEND = FastTlb[FastIndx].VSTART + (tlb[Entry].PageMask.Mask << 12) + 0xFFF;
	FastTlb[FastIndx].PHYSSTART = tlb[Entry].EntryLo1.PFN << 12;
	FastTlb[FastIndx].VALID = tlb[Entry].EntryLo1.V;
	FastTlb[FastIndx].DIRTY = tlb[Entry].EntryLo1.D;
	FastTlb[FastIndx].GLOBAL = tlb[Entry].EntryLo0.GLOBAL & tlb[Entry].EntryLo1.GLOBAL;
	FastTlb[FastIndx].ValidEntry = 0;

	for ( FastIndx = Entry << 1; FastIndx <= (Entry << 1) + 1; FastIndx++) {
		uint32_t count;

		if (!FastTlb[FastIndx].VALID) {
			FastTlb[FastIndx].ValidEntry = 1;
			continue;
		}
		if (FastTlb[FastIndx].VEND <= FastTlb[FastIndx].VSTART) {
			continue;
		}
		if (FastTlb[FastIndx].VSTART >= 0x80000000 && FastTlb[FastIndx].VEND <= 0xBFFFFFFF) {
			continue;
		}
		if (FastTlb[FastIndx].PHYSSTART > 0x1FFFFFFF) {
			continue;
		}

		//test if overlap
		FastTlb[FastIndx].ValidEntry = 1;
		for (count = FastTlb[FastIndx].VSTART; count < FastTlb[FastIndx].VEND; count += 0x1000) {
			TLB_Map[count >> 12] = ((uintptr_t)N64MEM + (count - FastTlb[FastIndx].VSTART + FastTlb[FastIndx].PHYSSTART)) - count;
		}
	}
}

void Cpu::TLB_Probe (void) {
	uint32_t Counter;


	INDEX_REGISTER |= 0x80000000;
	for (Counter = 0; Counter < 32; Counter ++) {
		uint32_t TlbValue = tlb[Counter].EntryHi.Value & (~tlb[Counter].PageMask.Mask << 13);
		uint32_t EntryHi = ENTRYHI_REGISTER & (~tlb[Counter].PageMask.Mask << 13);

		if (TlbValue == EntryHi) {
			uint32_t Global = (tlb[Counter].EntryHi.Value & 0x100) != 0;
			uint32_t SameAsid = ((tlb[Counter].EntryHi.Value & 0xFF) == (ENTRYHI_REGISTER & 0xFF));

			if (Global || SameAsid) {
				INDEX_REGISTER = Counter;
				return;
			}
		}
	}
}

void Cpu::TLB_Read (void) {
	uint32_t index = INDEX_REGISTER & 0x1F;

	PAGE_MASK_REGISTER = tlb[index].PageMask.Value ;
	ENTRYHI_REGISTER = (tlb[index].EntryHi.Value & ~tlb[index].PageMask.Value) ;
	ENTRYLO0_REGISTER = tlb[index].EntryLo0.Value;
	ENTRYLO1_REGISTER = tlb[index].EntryLo1.Value;
}

uint32_t Cpu::TranslateVaddr ( uintptr_t * Addr) {
	if (TLB_Map[((*Addr) & 0xffffffff) >> 12] == 0) { return 0; }
	*Addr = (uintptr_t)((uint8_t *)(TLB_Map[((*Addr) & 0xffffffff) >> 12] + ((*Addr) & 0xffffffff)) - (uintptr_t)N64MEM);
	return 1;
}

void Cpu::WriteTLBEntry (int32_t index) {
	uint32_t FastIndx;

	FastIndx = index << 1;
	if ((PROGRAM_COUNTER >= FastTlb[FastIndx].VSTART &&
		PROGRAM_COUNTER < FastTlb[FastIndx].VEND &&
		FastTlb[FastIndx].ValidEntry && FastTlb[FastIndx].VALID)
		||
		(PROGRAM_COUNTER >= FastTlb[FastIndx + 1].VSTART &&
		PROGRAM_COUNTER < FastTlb[FastIndx + 1].VEND &&
		FastTlb[FastIndx + 1].ValidEntry && FastTlb[FastIndx + 1].VALID))
	{
		return;
	}

	if (tlb[index].EntryDefined) {
		uint32_t count;

		for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
			if (!FastTlb[FastIndx].ValidEntry) { continue; }
			if (!FastTlb[FastIndx].VALID) { continue; }
			for (count = FastTlb[FastIndx].VSTART; count < FastTlb[FastIndx].VEND; count += 0x1000) {
				TLB_Map[count >> 12] = 0;
			}
		}
	}
	tlb[index].PageMask.Value = PAGE_MASK_REGISTER;
	tlb[index].EntryHi.Value = ENTRYHI_REGISTER;
	tlb[index].EntryLo0.Value = ENTRYLO0_REGISTER;
	tlb[index].EntryLo1.Value = ENTRYLO1_REGISTER;
	tlb[index].EntryDefined = 1;


	SetupTLB_Entry(index);
}
