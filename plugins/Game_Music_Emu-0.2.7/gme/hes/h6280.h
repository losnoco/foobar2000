/*****************************************************************************

	h6280.h Portable Hu6280 emulator interface

	Copyright (c) 1999 Bryan McPhail, mish@tendril.co.uk

	This source code is based (with permission!) on the 6502 emulator by
	Juergen Buchmueller.  It is released as part of the Mame emulator project.
	Let me know if you intend to use this code in any other project.

******************************************************************************/

#ifndef _H6280_H
#define _H6280_H

#include "osd_cpu.h"

enum {
	H6280_PC=1, H6280_S, H6280_P, H6280_A, H6280_X, H6280_Y,
	H6280_IRQ_MASK, H6280_TIMER_STATE,
    H6280_NMI_STATE, H6280_IRQ1_STATE, H6280_IRQ2_STATE, H6280_IRQT_STATE,
    H6280_MPR0,
    H6280_MPR1,
    H6280_MPR2,
    H6280_MPR3,
    H6280_MPR4,
    H6280_MPR5,
    H6280_MPR6,
    H6280_MPR7
};

//#define LAZY_FLAGS  1

#define H6280_INT_NONE	0
#define H6280_INT_NMI	1
#define H6280_INT_TIMER	2
#define H6280_INT_IRQ1	3
#define H6280_INT_IRQ2	4

#define H6280_RESET_VEC	0xfffe
#define H6280_NMI_VEC	0xfffc
#define H6280_TIMER_VEC	0xfffa
#define H6280_IRQ1_VEC	0xfff8
#define H6280_IRQ2_VEC	0xfff6			/* Aka BRK vector */

typedef struct
{
	PAIR  ppc;                      /* previous program counter */
    PAIR  pc;           /* program counter */
    PAIR  sp;           /* stack pointer (always 100 - 1FF) */
    PAIR  zp;           /* zero page address */
    PAIR  ea;           /* effective address */
    UINT8 a;            /* Accumulator */
    UINT8 x;            /* X index register */
    UINT8 y;            /* Y index register */
    UINT8 p;            /* Processor status */
    UINT32 mmr[8];       /* Hu6280 memory mapper registers */
    UINT8 irq_mask;     /* interrupt enable/disable */
    UINT8 timer_status; /* timer status */
        UINT8 timer_ack;        /* timer acknowledge */
    int timer_value;    /* timer interrupt */
    int timer_load;             /* reload value */
        int extra_cycles;       /* cycles used taking an interrupt */
    int nmi_state;
    int irq_state[3];

#if LAZY_FLAGS
    int NZ;             /* last value (lazy N and Z flag) */
#endif

    /* Default state of HuC6280 clock (1=7.16MHz, 0=3.58MHz) */
    int speed;

    int ICount;
    int timestamp;

    unsigned char *ram;
    unsigned char **read_ptr;
    unsigned char **write_ptr;

    void *private;
}   h6280_Regs;

extern void h6280_reset(h6280_Regs *, void *, unsigned char *, unsigned char **, unsigned char **);			/* Reset registers to the initial values */
extern void h6280_exit(void);					/* Shut down CPU */
extern int h6280_execute(h6280_Regs *, int cycles);			/* Execute cycles - returns number of cycles actually run */
extern unsigned h6280_get_pc(h6280_Regs *); 			/* Get program counter */
extern void h6280_set_pc(h6280_Regs *, unsigned val); 		/* Set program counter */
extern unsigned h6280_get_sp(h6280_Regs *); 			/* Get stack pointer */
extern void h6280_set_sp(h6280_Regs *, unsigned val); 		/* Set stack pointer */
extern unsigned h6280_get_reg (h6280_Regs *, int regnum);
extern void h6280_set_reg (h6280_Regs *, int regnum, unsigned val);
extern void h6280_set_nmi_line(h6280_Regs *, int state);
extern void h6280_set_irq_line(h6280_Regs *, int irqline, int state);

int H6280_irq_status_r(h6280_Regs *, int offset);
void H6280_irq_status_w(h6280_Regs *, int offset, int data);
int H6280_timer_r(h6280_Regs *, int offset);
void H6280_timer_w(h6280_Regs *, int offset, int data);


#endif /* _H6280_H */
