/*****************************************************************************

	h6280.c - Portable HuC6280 emulator

	Copyright (c) 1999, 2000 Bryan McPhail, mish@tendril.co.uk

	This source code is based (with permission!) on the 6502 emulator by
	Juergen Buchmueller.  It is released as part of the Mame emulator project.
	Let me know if you intend to use this code in any other project.


	NOTICE:

	This code is around 99% complete!  Several things are unimplemented,
	some due to lack of time, some due to lack of documentation, mainly
	due to lack of programs using these features.

	csh, csl opcodes are not supported.
	set opcode and T flag behaviour are not supported.

	I am unsure if instructions like SBC take an extra cycle when used in
	decimal mode.  I am unsure if flag B is set upon execution of rti.

	Cycle counts should be quite accurate, illegal instructions are assumed
	to take two cycles.


	Changelog, version 1.02:
		JMP + indirect X (0x7c) opcode fixed.
		SMB + RMB opcodes fixed in disassembler.
		change_pc function calls removed.
		TSB & TRB now set flags properly.
		BIT opcode altered.

	Changelog, version 1.03:
		Swapped IRQ mask for IRQ1 & IRQ2 (thanks Yasuhiro)

	Changelog, version 1.04, 28/9/99-22/10/99:
		Adjusted RTI (thanks Karl)
 		TST opcodes fixed in disassembler (missing break statements in a case!).
		TST behaviour fixed.
		SMB/RMB/BBS/BBR fixed in disassembler.

	Changelog, version 1.05, 8/12/99-16/12/99:
		Added CAB's timer implementation (note: irq ack & timer reload are changed).
		Fixed STA IDX.
		Fixed B flag setting on BRK.
		Assumed CSH & CSL to take 2 cycles each.

		Todo:  Performance could be improved by precalculating timer fire position.

	Changelog, version 1.06, 4/5/00 - last opcode bug found?
		JMP indirect was doing a EAL++; instead of EAD++; - Obviously causing
		a corrupt read when L = 0xff!  This fixes Bloody Wolf and Trio The Punch!

	Changelog, version 1.07, 3/9/00:
		Changed timer to be single shot - fixes Crude Buster music in level 1.

    Changelog, version 1.08, 1/18/01: (Charles MacDonald)
        Added h6280_speed to reflect current CPU speed set by CSL/CSH.
        Added implementation of SET opcode.
        Changed INLINE in tblh6280.h to 'static __inline__' (Allegro conflict)
        Added FAST_MEM routines for quicker ROM/RAM access.

******************************************************************************/

#include "cpuintrf.h"
#include "h6280.h"
#include <stdio.h>
#include <string.h>

/* We want the timer to be reloaded and fire continually, not as one-shot */
#define TIMER_HACK

#define CYCTOCK(__x) {unsigned int tik = __x; h6280->ICount -= tik; h6280->timestamp += tik; }

#include "h6280ops.h"
//#include "tblh6280.c"

/*****************************************************************************/

void h6280_reset(h6280_Regs *h6280, void *private, unsigned char *ram, unsigned char **read_ptr, unsigned char **write_ptr)
{
	int i;

	/* wipe out the h6280 structure */
	memset(h6280, 0, sizeof(h6280_Regs));

        h6280->private = private;

        h6280->ram = ram;
        h6280->read_ptr = read_ptr;
        h6280->write_ptr = write_ptr;

	/* set I and Z flags */
	P = _fI | _fZ;

	/* stack starts at 0x01ff */
	h6280->sp.d = 0x1ff;

	/* read the reset vector into PC */
	PCL = RDMEM(H6280_RESET_VEC);
	PCH = RDMEM((H6280_RESET_VEC+1));

	/* timer off by default */
	h6280->timer_status=0;
	h6280->timer_ack=1;

	/* clear pending interrupts */
	for (i = 0; i < 3; i++)
		h6280->irq_state[i] = CLEAR_LINE;

	h6280->speed = 1; /* default = 7.16MHz (?) */
}

void h6280_exit(void)
{
	/* nothing */
}

int h6280_execute(h6280_Regs *h6280, int cycles)
{
	int in,lastcycle,deltacycle;
	h6280->ICount += cycles;

	/* Subtract cycles used for taking an interrupt */
	CYCTOCK(h6280->extra_cycles);
	h6280->extra_cycles = 0;
	lastcycle = h6280->ICount;

	/* Execute instructions */
	do
	{
		h6280->ppc = h6280->pc;

		/* Execute 1 instruction */
		in=RDOP();

		PCW++;
		switch(in)
		{
		 #include "tblh6280.c"
		}
		//insnh6280[in](h6280);

		/* Check internal timer */
		if(h6280->timer_status)
		{
			deltacycle = lastcycle - h6280->ICount;
			h6280->timer_value -= deltacycle;
			if(h6280->timer_value<=0 && h6280->timer_ack==1)
			{
				#ifndef TIMER_HACK
		                h6280->timer_ack=h6280->timer_status=0;
				#else 
                		h6280->timer_ack=0;
				#endif
				h6280_set_irq_line(h6280, 2,ASSERT_LINE);
			}
		}
		lastcycle = h6280->ICount;

	} while (h6280->ICount > 0);

	/* Subtract cycles used for taking an interrupt */
    CYCTOCK(h6280->extra_cycles);
    h6280->extra_cycles = 0;

    return cycles - h6280->ICount;
}

unsigned h6280_get_pc (h6280_Regs *h6280)
{
    return PCD;
}

void h6280_set_pc (h6280_Regs *h6280, unsigned val)
{
	PCW = val;
}

unsigned h6280_get_sp (h6280_Regs *h6280)
{
	return S;
}

void h6280_set_sp (h6280_Regs *h6280, unsigned val)
{
	S = val;
}

unsigned h6280_get_reg (h6280_Regs *h6280, int regnum)
{
	switch( regnum )
	{
		case H6280_PC: return PCD;
		case H6280_S: return S;
		case H6280_P: return P;
		case H6280_A: return A;
		case H6280_X: return X;
		case H6280_Y: return Y;
		case H6280_IRQ_MASK: return h6280->irq_mask;
		case H6280_TIMER_STATE: return h6280->timer_status;
		case H6280_NMI_STATE: return h6280->nmi_state;
		case H6280_IRQ1_STATE: return h6280->irq_state[0];
		case H6280_IRQ2_STATE: return h6280->irq_state[1];
		case H6280_IRQT_STATE: return h6280->irq_state[2];
        case H6280_MPR0: return (h6280->mmr[0] >> 13);
        case H6280_MPR1: return (h6280->mmr[1] >> 13);
        case H6280_MPR2: return (h6280->mmr[2] >> 13);
        case H6280_MPR3: return (h6280->mmr[3] >> 13);
        case H6280_MPR4: return (h6280->mmr[4] >> 13);
        case H6280_MPR5: return (h6280->mmr[5] >> 13);
        case H6280_MPR6: return (h6280->mmr[6] >> 13);
        case H6280_MPR7: return (h6280->mmr[7] >> 13);
		case REG_PREVIOUSPC: return h6280->ppc.d;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
					return RDMEM( offset ) | ( RDMEM( offset+1 ) << 8 );
			}
	}
	return 0;
}

void h6280_set_reg (h6280_Regs *h6280, int regnum, unsigned val)
{
	switch( regnum )
	{
		case H6280_PC: PCW = val; break;
		case H6280_S: S = val; break;
		case H6280_P: P = val; break;
		case H6280_A: A = val; break;
		case H6280_X: X = val; break;
		case H6280_Y: Y = val; break;
		case H6280_IRQ_MASK: h6280->irq_mask = val; CHECK_IRQ_LINES; break;
		case H6280_TIMER_STATE: h6280->timer_status = val; break;
		case H6280_NMI_STATE: h6280_set_nmi_line(h6280, val ); break;
		case H6280_IRQ1_STATE: h6280_set_irq_line(h6280, 0, val ); break;
		case H6280_IRQ2_STATE: h6280_set_irq_line(h6280, 1, val ); break;
		case H6280_IRQT_STATE: h6280_set_irq_line(h6280, 2, val ); break;
        case H6280_MPR0: h6280->mmr[0] = (val << 13); break;
        case H6280_MPR1: h6280->mmr[1] = (val << 13); break;
        case H6280_MPR2: h6280->mmr[2] = (val << 13); break;
        case H6280_MPR3: h6280->mmr[3] = (val << 13); break;
        case H6280_MPR4: h6280->mmr[4] = (val << 13); break;
        case H6280_MPR5: h6280->mmr[5] = (val << 13); break;
        case H6280_MPR6: h6280->mmr[6] = (val << 13); break;
        case H6280_MPR7: h6280->mmr[7] = (val << 13); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
				{
					WRMEM( offset, val & 0xff );
					WRMEM( offset+1, (val >> 8) & 0xff );
				}
			}
    }
}

/*****************************************************************************/

void h6280_set_nmi_line(h6280_Regs *h6280, int state)
{
	if (h6280->nmi_state == state) return;
	h6280->nmi_state = state;
	if (state != CLEAR_LINE)
	{
		DO_INTERRUPT(h6280, H6280_NMI_VEC);
	}
}

void h6280_set_irq_line(h6280_Regs *h6280, int irqline, int state)
{
    h6280->irq_state[irqline] = state;

	/* If line is cleared, just exit */
	if (state == CLEAR_LINE) return;

	/* Check if interrupts are enabled and the IRQ mask is clear */
	CHECK_IRQ_LINES;
}

int H6280_irq_status_r (h6280_Regs *h6280, int offset)
{
	int status;

	switch (offset)
	{
		case 0: /* Read irq mask */
			return h6280->irq_mask;

		case 1: /* Read irq status */
			status=0;
			if(h6280->irq_state[1]!=CLEAR_LINE) status|=1; /* IRQ 2 */
			if(h6280->irq_state[0]!=CLEAR_LINE) status|=2; /* IRQ 1 */
			if(h6280->irq_state[2]!=CLEAR_LINE) status|=4; /* TIMER */
			return status;
	}

	return 0;
}

void H6280_irq_status_w (h6280_Regs *h6280, int offset, int data)
{
	switch (offset)
	{
		case 0: /* Write irq mask */
			h6280->irq_mask=data&0x7;
			CHECK_IRQ_LINES;
			break;

		case 1: /* Timer irq ack - timer is reloaded here */
			h6280->timer_value = h6280->timer_load;
			h6280->timer_ack=1; /* Timer can't refire until ack'd */
			break;
	}
}

int H6280_timer_r (h6280_Regs *h6280, int offset)
{
	switch (offset) {
		case 0: /* Counter value */
			return (h6280->timer_value/1024)&127;

		case 1: /* Read counter status */
			return h6280->timer_status;
	}

	return 0;
}

void H6280_timer_w (h6280_Regs *h6280, int offset, int data)
{
	switch (offset) {
		case 0: /* Counter preload */
			h6280->timer_load=h6280->timer_value=((data&127)+1)*1024;
			return;

		case 1: /* Counter enable */
			if(data&1)
			{	/* stop -> start causes reload */
				if(h6280->timer_status==0) h6280->timer_value=h6280->timer_load;
			}
			h6280->timer_status=data&1;
			return;
	}
}

/*****************************************************************************/




