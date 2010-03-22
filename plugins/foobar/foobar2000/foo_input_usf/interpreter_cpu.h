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
#include "cpu.h"
#include "x86.h"

#ifdef __cplusplus

#define INTERPRETER_CALL __thiscall Interpreter::

class Interpreter : public X86, public Cpu
{
	public:
		Interpreter(Usf & mUsf);
		void ExecuteInterpreterOpCode ( void );
		void StartCPU	  ( void );
		void TestInterpreterJump	  ( uint32_t PC, uint32_t TargetPC, int32_t Reg1, int32_t Reg2 );
		void * RunCpu(uint32_t * Bytes);

	//private:
		void (INTERPRETER_CALL * R4300i_Opcode[64])();
		void (INTERPRETER_CALL * R4300i_Special[64])();
		void (INTERPRETER_CALL * R4300i_Regimm[32])();
		void (INTERPRETER_CALL * R4300i_CoP0[32])();
		void (INTERPRETER_CALL * R4300i_CoP0_Function[64])();
		void (INTERPRETER_CALL * R4300i_CoP1[32])();
		void (INTERPRETER_CALL * R4300i_CoP1_BC[32])();
		void (INTERPRETER_CALL * R4300i_CoP1_S[64])();
		void (INTERPRETER_CALL * R4300i_CoP1_D[64])();
		void (INTERPRETER_CALL * R4300i_CoP1_W[64])();
		void (INTERPRETER_CALL * R4300i_CoP1_L[64])();

		void R4300i_opcode_SPECIAL (void);
		void R4300i_opcode_REGIMM (void);
		void R4300i_opcode_COP0 (void);
		void R4300i_opcode_COP0_CO (void);
		void R4300i_opcode_COP1 (void);
		void R4300i_opcode_COP1_BC (void);
		void R4300i_opcode_COP1_S (void);
		void R4300i_opcode_COP1_D (void);
		void R4300i_opcode_COP1_W (void);
		void R4300i_opcode_COP1_L (void);


		#include "interpreter_ops.h"

		int32_t RoundingModel;


};

extern uint32_t SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
extern uint64_t SDR_MASK[8], SDL_MASK[8], LDR_MASK[8], LDL_MASK[8];
extern int32_t SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
extern int32_t SDR_SHIFT[8], SDL_SHIFT[8], LDL_SHIFT[8], LDR_SHIFT[8];
extern int32_t RoundingModel;

#endif

