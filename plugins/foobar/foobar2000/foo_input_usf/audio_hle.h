/**
 * Mupen64 hle rsp - hle.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifndef HLE_H
#define HLE_H

#ifndef _BIG_ENDIAN
#define S 0
#define S8 0
#else
#define S 1
#define S8 3
#endif
/*
 * Audio flags
 */

#define A_INIT			0x01
#define A_CONTINUE		0x00
#define A_LOOP		  0x02
#define A_OUT		   0x02
#define A_LEFT			0x02
#define	A_RIGHT			0x00
#define A_VOL			0x04
#define A_RATE			0x00
#define A_AUX			0x08
#define A_NOAUX			0x00
#define A_MAIN			0x00
#define A_MIX			0x10

typedef struct
{
   unsigned int type;
   unsigned int flags;

   unsigned int ucode_boot;
   unsigned int ucode_boot_size;

   unsigned int ucode;
   unsigned int ucode_size;

   unsigned int ucode_data;
   unsigned int ucode_data_size;

   unsigned int dram_stack;
   unsigned int dram_stack_size;

   unsigned int output_buff;
   unsigned int output_buff_size;

   unsigned int data_ptr;
   unsigned int data_size;

   unsigned int yield_data_ptr;
   unsigned int yield_data_size;
} OSTask_t;

extern uint32_t inst1, inst2;
//extern uint16_t AudioInBuffer, AudioOutBuffer, AudioCount;
//extern uint16_t AudioAuxA, AudioAuxC, AudioAuxE;
extern uint32_t loopval; // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
//extern uint32_t UCData, UDataLen;

extern uint32_t SEGMENTS[0x10];		// 0x0320
// T8 = 0x360
extern uint16_t AudioInBuffer;		// 0x0000(T8)
extern uint16_t AudioOutBuffer;		// 0x0002(T8)
extern uint16_t AudioCount;			// 0x0004(T8)
extern int16_t Vol_Left;		// 0x0006(T8)
extern int16_t Vol_Right;		// 0x0008(T8)
extern uint16_t AudioAuxA;			// 0x000A(T8)
extern uint16_t AudioAuxC;			// 0x000C(T8)
extern uint16_t AudioAuxE;			// 0x000E(T8)
extern uint32_t loopval;			// 0x0010(T8) // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
extern int16_t VolTrg_Left;	// 0x0010(T8)
extern int32_t VolRamp_Left;	// m_LeftVolTarget
//u16 VolRate_Left;	// m_LeftVolRate
extern int16_t VolTrg_Right;	// m_RightVol
extern int32_t VolRamp_Right;	// m_RightVolTarget
//u16 VolRate_Right;	// m_RightVolRate
extern int16_t Env_Dry;		// 0x001C(T8)
extern int16_t Env_Wet;		// 0x001E(T8)


extern uint8_t BufferSpace[0x10000];

extern short hleMixerWorkArea[256];
extern uint16_t adpcmtable[0x88];
extern int firstHLE, goldeneye;

int audio_ucode ( OSTask_t *task );
//extern unsigned char *RDRAM,*DMEM, *IMEM, *ROM;
//extern unsigned int N64MEM_Pages[0x80];
#include "usf.h"

#include "cpu.h"

extern uint16_t ResampleLUT [0x200];

#define HLECAST(x) static_cast<void (ABIBase::*)(void)>(x);

class ABIBase
{
	public:
		ABIBase(Cpu & mCpu);
		void Execute(uint32_t, uint32_t, uint32_t);
		Cpu & hCpu;

	protected:

		uint32_t inst1, inst2;

		uint8_t BufferSpace[0x10000];
		int16_t hleMixerWorkArea[256];
		uint16_t adpcmtable[0x88];
		uint32_t segments[0x20];
		uint16_t AudioInBuffer;
		uint16_t AudioOutBuffer;
		uint16_t AudioCount;
		uint32_t loopval;
		void (ABIBase::*ABI[0x20])();
};

class Abi1 : public ABIBase
{
	public:
		Abi1(Cpu & mCpu);
		Abi1(Cpu & mCpu, uint32_t);
		~Abi1();
	private:
		int32_t Goldeneye;
		int16_t Vol_Left;
		int16_t Vol_Right;
		int16_t AudioAuxA;
		int16_t AudioAuxC;
		int16_t AudioAuxE;

		int16_t VolTrg_Left;

		int32_t VolRamp_Left;
		int32_t VolRate_Left;
		int16_t VolTrg_Right;
		int32_t VolRamp_Right;
		int32_t VolRate_Right;
		int16_t Env_Dry;
		int16_t Env_Wet;

		void CLEARBUFF();
		void ENVMIXER();
		void ENVMIXERo();
		void RESAMPLE();
		void SETVOL();
		void UNKNOWN();
		void SETLOOP();
		void ADPCM();
		void LOADBUFF();
		void SAVEBUFF();
		void SEGMENT();
		void SETBUFF();
		void DMEMMOVE();
		void LOADADPCM();
		void INTERLEAVE();
		void MIXER();
		void SPNOOP();
};

class Abi2 : public ABIBase
{
	private:
	public:
		uint32_t t3, s5, s6;
		uint16_t env[8];
		int32_t isMKABI;
		int32_t isZeldaABI; //false

		int32_t cnt;
		int16_t *lutt6;
		int16_t *lutt5;


		Abi2(Cpu &);
		~Abi2();

		void LOADADPCM2();
		void SPNOOP();
		void ADPCM2();
		void CLEARBUFF2();
		void UNKNOWN();
		void ADDMIXER();
		void RESAMPLE2();
		void SEGMENT2();
		void SETBUFF2();
		void DUPLICATE2();
		void DMEMMOVE2();
		void MIXER2();
		void INTERLEAVE2();
		void HILOGAIN();
		void SETLOOP2();
		void INTERL2();
		void FILTER2();
		void ENVSETUP1();
		void ENVMIXER2();
		void LOADBUFF2();
		void SAVEBUFF2();
		void ENVSETUP2();
};


class Abi3 : public ABIBase
{
	private:
	public:

		int16_t Env_Dry;
		int16_t Env_Wet;
		int16_t Vol_Left;
		int16_t Vol_Right;
		int16_t VolTrg_Left;
		int32_t VolRamp_Left;
		int16_t VolTrg_Right;
		int32_t VolRamp_Right;

		uint16_t AudioAuxA;
		uint16_t AudioAuxC;
		uint16_t AudioAuxE;

		uint8_t mp3data[0x1000];

		int32_t v[32];

		uint32_t inPtr, outPtr;

		uint32_t t6;
		uint32_t t5;
		uint32_t t4;

		Abi3(Cpu &);
		~Abi3();

		void DISABLE();
		void ADPCM3();
		void CLEARBUFF3();
		void ENVMIXER3();
		void LOADBUFF3();
		void RESAMPLE3();
		void SAVEBUFF3();
		void MP3();
		void MP3ADDY();
		void SETVOL3();
		void DMEMMOVE3();
		void LOADADPCM3();
		void MIXER3();
		void INTERLEAVE3();
		void WHATISTHIS();
		void SETLOOP3();
		void MP3AB0();
		void InnerLoop();
		void SPNOOP();
		void ENVMIXER3o();


};





#endif
