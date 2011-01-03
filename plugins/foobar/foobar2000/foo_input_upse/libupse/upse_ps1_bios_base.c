/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_bios_base.c
 * Purpose: libupse: UPSE PS1 FakeBIOS base code
 *
 * Copyright (c) 2007 - 2010 William Pitcock <nenolod@dereferenced.org>
 * Portions copyright (c) 1999-2002 Pcsx Team
 * Portions copyright (c) 2004 "Xodnizel"
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "upse-internal.h"
#include "upse-ps1-bios-private.h"

//We try to emulate bios :) HELP US :P

char *biosA0n[256] = {
	/* 0x00 */	"open",
	/* 0x01 */	"lseek",
	/* 0x02 */	"read",
	/* 0x03 */	"write",
	/* 0x04 */	"close",
	/* 0x05 */	"ioctl",
	/* 0x06 */	"exit",
	/* 0x07 */	"bios_a0_07",
	/* 0x08 */	"getc",
	/* 0x09 */	"putc",
	/* 0x0a */	"todigit",
	/* 0x0b */	"atof",
	/* 0x0c */	"strtoul",
	/* 0x0d */	"strtol",
	/* 0x0e */	"abs",
	/* 0x0f */	"labs",
	/* 0x10 */	"atoi",
	/* 0x11 */	"atol",
	/* 0x12 */	"atob",
	/* 0x13 */	"setjmp",
	/* 0x14 */	"longjmp",
	/* 0x15 */	"strcat",
	/* 0x16 */	"strncat",
	/* 0x17 */	"strcmp",
	/* 0x18 */	"strncmp",
	/* 0x19 */	"strcpy",
	/* 0x1a */	"strnpy",
	/* 0x1b */	"strlen",
	/* 0x1c */	"index",
	/* 0x1d */	"rindex",
	/* 0x1e */	"strchr",
	/* 0x1f */	"strrchr",
	/* 0x20 */	"strpbrk",
	/* 0x21 */	"strspn",
	/* 0x22 */	"strcspn",
	/* 0x23 */	"strtok",
	/* 0x24 */	"strstr",
	/* 0x25 */	"toupper",
	/* 0x26 */	"tolower",
	/* 0x27 */	"bcopy",
	/* 0x28 */	"bzero",
	/* 0x29 */	"bcmp",
	/* 0x2a */	"memcpy",
	/* 0x2b */	"memset",
	/* 0x2c */	"memmove",
	/* 0x2d */	"memcmp",
	/* 0x2e */	"memchr",
	/* 0x2f */	"rand",
	/* 0x30 */	"srand",
	/* 0x31 */	"qsort",
	/* 0x32 */	"strtod",
	/* 0x33 */	"malloc",
	/* 0x34 */	"free",
	/* 0x35 */	"lsearch",
	/* 0x36 */	"bsearch",
	/* 0x37 */	"calloc",
	/* 0x38 */	"realloc",
	/* 0x39 */	"InitHeap",
	/* 0x3a */	"_exit",
	/* 0x3b */	"getchar",
	/* 0x3c */	"putchar",
	/* 0x3d */	"gets",
	/* 0x3e */	"puts",
	/* 0x3f */	"printf",
	/* 0x40 */	"bios_a0_40",
	/* 0x41 */	"LoadTest",
	/* 0x42 */	"Load",
	/* 0x43 */	"Exec",
	/* 0x44 */	"FlushCache",
	/* 0x45 */	"InstallInterruptHandler",
	/* 0x46 */	"GPU_dw",
	/* 0x47 */	"mem2vram",
	/* 0x48 */	"SendGPU",
	/* 0x49 */	"GPU_cw",
	/* 0x4a */	"GPU_cwb",
	/* 0x4b */	"SendPackets",
	/* 0x4c */	"bios_a0_4c",
	/* 0x4d */	"GetGPUStatus",
	/* 0x4e */	"GPU_sync",
	/* 0x4f */	"bios_a0_4f",
	/* 0x50 */	"bios_a0_50",
	/* 0x51 */	"LoadExec",
	/* 0x52 */	"GetSysSp",
	/* 0x53 */	"bios_a0_53",
	/* 0x54 */	"_96_init",
	/* 0x55 */	"_bu_init",
	/* 0x56 */	"_96_remove",
	/* 0x57 */	"bios_a0_57",
	/* 0x58 */	"bios_a0_58",
	/* 0x59 */	"bios_a0_59",
	/* 0x5a */	"bios_a0_5a",
	/* 0x5b */	"dev_tty_init",
	/* 0x5c */	"dev_tty_open",
	/* 0x5d */	"dev_tty_5d",
	/* 0x5e */	"dev_tty_ioctl",
	/* 0x5f */	"dev_cd_open",
	/* 0x60 */	"dev_cd_read",
	/* 0x61 */	"dev_cd_close",
	/* 0x62 */	"dev_cd_firstfile",
	/* 0x63 */	"dev_cd_nextfile",
	/* 0x64 */	"dev_cd_chdir",
	/* 0x65 */	"dev_card_open",
	/* 0x66 */	"dev_card_read",
	/* 0x67 */	"dev_card_write",
	/* 0x68 */	"dev_card_close",
	/* 0x69 */	"dev_card_firstfile",
	/* 0x6a */	"dev_card_nextfile",
	/* 0x6b */	"dev_card_erase",
	/* 0x6c */	"dev_card_undelete",
	/* 0x6d */	"dev_card_format",
	/* 0x6e */	"dev_card_rename",
	/* 0x6f */	"dev_card_6f",
	/* 0x70 */	"_bu_init(a0_70)",
	/* 0x71 */	"_96_init(a0_71)",
	/* 0x72 */	"_96_remove(a0_72)",
	/* 0x73 */	"bios_a0_73",
	/* 0x74 */	"bios_a0_74",
	/* 0x75 */	"bios_a0_75",
	/* 0x76 */	"bios_a0_76",
	/* 0x77 */	"bios_a0_77",
	/* 0x78 */	"_96_CdSeekL",
	/* 0x79 */	"bios_a0_79",
	/* 0x7a */	"bios_a0_7a",
	/* 0x7b */	"bios_a0_7b",
	/* 0x7c */	"_96_CdGetStatus",
	/* 0x7d */	"bios_a0_7d",
	/* 0x7e */	"_96_CdRead",
	/* 0x7f */	"bios_a0_7f",
	/* 0x80 */	"bios_a0_80",
	/* 0x81 */	"bios_a0_81",
	/* 0x82 */	"bios_a0_82",
	/* 0x83 */	"bios_a0_83",
	/* 0x84 */	"bios_a0_84",
	/* 0x85 */	"_96_CdStop",
	/* 0x86 */	"bios_a0_86",
	/* 0x87 */	"bios_a0_87",
	/* 0x88 */	"bios_a0_88",
	/* 0x89 */	"bios_a0_89",
	/* 0x8a */	"bios_a0_8a",
	/* 0x8b */	"bios_a0_8b",
	/* 0x8c */	"bios_a0_8c",
	/* 0x8d */	"bios_a0_8d",
	/* 0x8e */	"bios_a0_8e",
	/* 0x8f */	"bios_a0_8f",
	/* 0x90 */	"bios_a0_90",
	/* 0x91 */	"bios_a0_91",
	/* 0x92 */	"bios_a0_92",
	/* 0x93 */	"bios_a0_93",
	/* 0x94 */	"bios_a0_94",
	/* 0x95 */	"bios_a0_95",
	/* 0x96 */	"AddCDROMDevice",
	/* 0x97 */	"AddMemCardDevice",
	/* 0x98 */	"DisableKernelIORedirection",
	/* 0x99 */	"EnableKernelIORedirection",
	/* 0x9a */	"bios_a0_9a",
	/* 0x9b */	"bios_a0_9b",
	/* 0x9c */	"SetConf",
	/* 0x9d */	"GetConf",
	/* 0x9e */	"bios_a0_9e",
	/* 0x9f */	"SetMem",
	/* 0xa0 */	"_boot",
	/* 0xa1 */	"SystemError",
	/* 0xa2 */	"EnqueueCdIntr",
	/* 0xa3 */	"DequeueCdIntr",
	/* 0xa4 */	"bios_a0_a4",
	/* 0xa5 */	"ReadSector",
	/* 0xa6 */	"get_cd_status",
	/* 0xa7 */	"bufs_cb_0",
	/* 0xa8 */	"bufs_cb_1",
	/* 0xa9 */	"bufs_cb_2",
	/* 0xaa */	"bufs_cb_3",
	/* 0xab */	"_card_info",
	/* 0xac */	"_card_load",
	/* 0xad */	"_card_auto",
	/* 0xae */	"bufs_cb_4",
	/* 0xaf */	"bios_a0_af",
	/* 0xb0 */	"bios_a0_b0",
	/* 0xb1 */	"bios_a0_b1",
	/* 0xb2 */	"do_a_long_jmp",
	/* 0xb3 */	"bios_a0_b3",
	/* 0xb4 */	"bios_a0_b4",
};

char *biosB0n[256] = {
	/* 0x00 */	"SysMalloc",
	/* 0x01 */	"bios_b0_01",
	/* 0x02 */	"bios_b0_02",
	/* 0x03 */	"bios_b0_03",
	/* 0x04 */	"bios_b0_04",
	/* 0x05 */	"bios_b0_05",
	/* 0x06 */	"bios_b0_06",
	/* 0x07 */	"DeliverEvent",
	/* 0x08 */	"OpenEvent",
	/* 0x09 */	"CloseEvent",
	/* 0x0a */	"WaitEvent",
	/* 0x0b */	"TestEvent",
	/* 0x0c */	"EnableEvent",
	/* 0x0d */	"DisableEvent",
	/* 0x0e */	"OpenTh",
	/* 0x0f */	"CloseTh",
	/* 0x10 */	"ChangeTh",
	/* 0x11 */	"bios_b0_11",
	/* 0x12 */	"InitPAD",
	/* 0x13 */	"StartPAD",
	/* 0x14 */	"StopPAD",
	/* 0x15 */	"PAD_init",
	/* 0x16 */	"PAD_dr",
	/* 0x17 */	"ReturnFromException",
	/* 0x18 */	"ResetEntryInt",
	/* 0x19 */	"HookEntryInt",
	/* 0x1a */	"bios_b0_1a",
	/* 0x1b */	"bios_b0_1b",
	/* 0x1c */	"bios_b0_1c",
	/* 0x1d */	"bios_b0_1d",
	/* 0x1e */	"bios_b0_1e",
	/* 0x1f */	"bios_b0_1f",
	/* 0x20 */	"UnDeliverEvent",
	/* 0x21 */	"bios_b0_21",
	/* 0x22 */	"bios_b0_22",
	/* 0x23 */	"bios_b0_23",
	/* 0x24 */	"bios_b0_24",
	/* 0x25 */	"bios_b0_25",
	/* 0x26 */	"bios_b0_26",
	/* 0x27 */	"bios_b0_27",
	/* 0x28 */	"bios_b0_28",
	/* 0x29 */	"bios_b0_29",
	/* 0x2a */	"bios_b0_2a",
	/* 0x2b */	"bios_b0_2b",
	/* 0x2c */	"bios_b0_2c",
	/* 0x2d */	"bios_b0_2d",
	/* 0x2e */	"bios_b0_2e",
	/* 0x2f */	"bios_b0_2f",
	/* 0x30 */	"bios_b0_30",
	/* 0x31 */	"bios_b0_31",
	/* 0x32 */	"open(b0)",
	/* 0x33 */	"lseek(b0)",
	/* 0x34 */	"read(b0)",
	/* 0x35 */	"write(b0)",
	/* 0x36 */	"close(b0)",
	/* 0x37 */	"ioctl(b0)",
	/* 0x38 */	"exit(b0)",
	/* 0x39 */	"bios_b0_39",
	/* 0x3a */	"getc(b0)",
	/* 0x3b */	"putc(b0)",
	/* 0x3c */	"getchar(b0)",
	/* 0x3d */	"putchar(b0)",
	/* 0x3e */	"gets(b0)",
	/* 0x3f */	"puts(b0)",
	/* 0x40 */	"cd",
	/* 0x41 */	"format",
	/* 0x42 */	"firstfile",
	/* 0x43 */	"nextfile",
	/* 0x44 */	"rename",
	/* 0x45 */	"delete",
	/* 0x46 */	"undelete",
	/* 0x47 */	"AddDevice",
	/* 0x48 */	"RemoteDevice",
	/* 0x49 */	"PrintInstalledDevices",
	/* 0x4a */	"InitCARD",
	/* 0x4b */	"StartCARD",
	/* 0x4c */	"StopCARD",
	/* 0x4d */	"bios_b0_4d",
	/* 0x4e */	"_card_write",
	/* 0x4f */	"_card_read",
	/* 0x50 */	"_new_card",
	/* 0x51 */	"Krom2RawAdd",
	/* 0x52 */	"bios_b0_52",
	/* 0x53 */	"bios_b0_53",
	/* 0x54 */	"_get_errno",
	/* 0x55 */	"_get_error",
	/* 0x56 */	"GetC0Table",
	/* 0x57 */	"GetB0Table",
	/* 0x58 */	"_card_chan",
	/* 0x59 */	"bios_b0_59",
	/* 0x5a */	"bios_b0_5a",
	/* 0x5b */	"ChangeClearPAD",
	/* 0x5c */	"_card_status",
	/* 0x5d */	"_card_wait",
};

char *biosC0n[256] = {
	/* 0x00 */	"InitRCnt",
	/* 0x01 */	"InitException",
	/* 0x02 */	"SysEnqIntRP",
	/* 0x03 */	"SysDeqIntRP",
	/* 0x04 */	"get_free_EvCB_slot",
	/* 0x05 */	"get_free_TCB_slot",
	/* 0x06 */	"ExceptionHandler",
	/* 0x07 */	"InstallExceptionHandler",
	/* 0x08 */	"SysInitMemory",
	/* 0x09 */	"SysInitKMem",
	/* 0x0a */	"ChangeClearRCnt",
	/* 0x0b */	"SystemError(c0)",
	/* 0x0c */	"InitDefInt",
	/* 0x0d */	"bios_c0_0d",
	/* 0x0e */	"bios_c0_0e",
	/* 0x0f */	"bios_c0_0f",
	/* 0x10 */	"bios_c0_10",
	/* 0x11 */	"bios_c0_11",
	/* 0x12 */	"InstallDevices",
	/* 0x13 */	"FlushStdInOutPut",
	/* 0x14 */	"bios_c0_14",
	/* 0x15 */	"_cdevinput",
	/* 0x16 */	"_cdevscan",
	/* 0x17 */	"_circgetc",
	/* 0x18 */	"_circputc",
	/* 0x19 */	"ioabort",
	/* 0x1a */	"bios_c0_1a",
	/* 0x1b */	"KernelRedirect",
	/* 0x1c */	"PatchAOTable",
};

/* *INDENT-ON* */

typedef struct _malloc_chunk
{
    u32 stat;
    u32 size;
    u32 fd;
    u32 bk;
} upse_packed_t malloc_chunk;

#define INUSE 0x1

typedef struct
{
    u32 desc;
    s32 status;
    s32 mode;
    u32 fhandler;
} upse_packed_t EvCB[32];

#define EvStUNUSED	0x0000
#define EvStWAIT	0x1000
#define EvStACTIVE	0x2000
#define EvStALREADY 0x4000

#define EvMdINTR	0x1000
#define EvMdNOINTR	0x2000

typedef struct
{
    s32 status;
    s32 mode;
    u32 reg[32];
    u32 func;
} upse_packed_t TCB;

typedef struct
{
    u32 *jmp_int;
    u32 regs[35];
    EvCB *Event;
    EvCB *RcEV;		// 0xf2

    u32 heap_addr;
    u32 SysIntRP[8];
    TCB Thread[8];
    int CurThread;
} upse_ps1_bios_state_t;

static INLINE void softCall(upse_module_instance_t *ins, u32 pc)
{
    pc0 = pc;
    ra = 0x80001000;
    while (pc0 != 0x80001000)
	upse_r3000_cpu_execute_block(ins);
}

static INLINE void softCall2(upse_module_instance_t *ins, u32 pc)
{
    u32 sra = ra;
    pc0 = pc;
    ra = 0x80001000;
    while (pc0 != 0x80001000)
	upse_r3000_cpu_execute_block(ins);
    ra = sra;
}

static INLINE void DeliverEvent(upse_module_instance_t *ins, u32 ev, u32 spec)
{
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    if (biosstate->Event[ev][spec].status != BFLIP32S(EvStACTIVE))
	return;

//      biosstate->Event[ev][spec].status = BFLIP32S(EvStALREADY);
    if (biosstate->Event[ev][spec].mode == BFLIP32S(EvMdINTR))
    {
	softCall2(ins, BFLIP32S(biosstate->Event[ev][spec].fhandler));
    }
    else
	biosstate->Event[ev][spec].status = BFLIP32S(EvStALREADY);
}

/*                                           *
//                                           *
//                                           *
//               System calls A0             */

/* Abs and labs do the same thing? */

static void bios_abs(upse_module_instance_t *ins)
{				// 0x0e
    if ((s32) a0 < 0)
	v0 = 0 - (s32) a0;
    else
	v0 = a0;
    //v0 = abs(a0);
    pc0 = ra;
}

static void bios_labs(upse_module_instance_t *ins)
{				// 0x0f
    if ((s32) a0 < 0)
	v0 = 0 - (s32) a0;
    else
	v0 = a0;
    //v0 = labs(a0);
    pc0 = ra;
}

static void bios_atoi(upse_module_instance_t *ins)
{				// 0x10
    char *arg = Ra0;
    v0 = atoi(arg);
    pc0 = ra;
}

static void bios_atol(upse_module_instance_t *ins)
{				// 0x11
    char *arg = Ra0;
    v0 = atol(arg);
    pc0 = ra;
}

static void bios_setjmp(upse_module_instance_t *ins)
{				// 13
    u32 *jmp_buf = (u32 *) Ra0;
    int i;

    jmp_buf[0] = BFLIP32(ra);
    jmp_buf[1] = BFLIP32(sp);
    jmp_buf[2] = BFLIP32(fp);
    for (i = 0; i < 8; i++)	// s0-s7
	jmp_buf[3 + i] = BFLIP32(ins->cpustate.GPR.r[16 + i]);
    jmp_buf[11] = BFLIP32(gp);

    v0 = 0;
    pc0 = ra;
}

static void bios_longjmp(upse_module_instance_t *ins)
{				//14
    u32 *jmp_buf = (u32 *) Ra0;
    int i;

    ra = BFLIP32(jmp_buf[0]);	/* ra */
    sp = BFLIP32(jmp_buf[1]);	/* sp */
    fp = BFLIP32(jmp_buf[2]);	/* fp */
    for (i = 0; i < 8; i++)	// s0-s7
	ins->cpustate.GPR.r[16 + i] = BFLIP32(jmp_buf[3 + i]);
    gp = BFLIP32(jmp_buf[11]);	/* gp */

    v0 = a1;
    pc0 = ra;
}

static void bios_strcat(upse_module_instance_t *ins)
{				// 0x15
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    if (pcA0 == NULL || pcA1 == NULL)
	return;

    strcat(pcA0, pcA1 != NULL ? pcA1 : "");
    v0 = a0;
    pc0 = ra;
}

/*0x16*/
static void bios_strncat(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    if (pcA0 == NULL || pcA1 == NULL)
	return;

    strncat(pcA0, pcA1, a2);
    v0 = a0;
    pc0 = ra;
}

static void bios_strcmp(upse_module_instance_t *ins)
{				// 0x17
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    v0 = strcmp(pcA0, pcA1);
    pc0 = ra;
}

static void bios_strncmp(upse_module_instance_t *ins)
{				// 0x18
    u32 max = a2;
    u32 string1 = a0;
    u32 string2 = a1;
    s8 tmpv = 0;

    while (max > 0)
    {
	u8 tmp1 = PSXMuR8(ins, string1);
	u8 tmp2 = PSXMuR8(ins, string2);

	if (!tmp1 || !tmp2)
	    break;

	tmpv = tmp1 - tmp2;
	if (tmpv)
	    break;
	if (!tmp1 || !tmp2)
	    break;
	if (!PSXM(ins, string1) || !PSXM(ins, string2))
	    break;
	max--;
	string1++;
	string2++;
    }
    if (tmpv > 0)
	v0 = 1;
    else if (tmpv < 0)
	v0 = -1;
    else
	v0 = 0;
    //printf("%s:%s, %d, %d\n",Ra0,Ra1,a2,v0);
    //v0 = strncmp(Ra0, Ra1, a2);
    pc0 = ra;
}

/*0x19*/
static void bios_strcpy(upse_module_instance_t *ins)
{
    u32 src = a1, dest = a0;
    u8 val;

    do
    {
	val = PSXMu8(ins, src);
	PSXMu8(ins, dest) = val;
	src++;
	dest++;
    } while (val);
    //strcpy(Ra0, Ra1); 
    v0 = a0;
    pc0 = ra;
}

/*0x1a*/
static void bios_strncpy(upse_module_instance_t *ins)
{
    u32 src = a1, dest = a0, max = a2;
    u8 val;

    do
    {
	val = PSXMu8(ins, src);
	PSXMu8(ins, dest) = val;
	src++;
	dest++;
	max--;
    } while (val && max);

    //strncpy(Ra0, Ra1, a2);  
    v0 = a0;
    pc0 = ra;
}

/*0x1b*/
static void bios_strlen(upse_module_instance_t *ins)
{
    u32 src = a0;

    while (PSXMu8(ins, src))
	src++;

    v0 = src - a0;
    pc0 = ra;
}

static void bios_index(upse_module_instance_t *ins)
{				// 0x1c
    char *pcA0 = (char *) Ra0;
    char *pcRet = strchr(pcA0, a1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_rindex(upse_module_instance_t *ins)
{				// 0x1d
    char *pcA0 = (char *) Ra0;
    char *pcRet = strrchr(pcA0, a1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_strchr(upse_module_instance_t *ins)
{				// 0x1e
    char *pcA0 = (char *) Ra0;
    char *pcRet = strchr(pcA0, a1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_strrchr(upse_module_instance_t *ins)
{				// 0x1f
    char *pcA0 = (char *) Ra0;
    char *pcRet = strrchr(pcA0, a1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_strpbrk(upse_module_instance_t *ins)
{				// 0x20
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;
    char *pcRet = strpbrk(pcA0, pcA1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_strspn(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;
    v0 = strspn(pcA0, pcA1);
    pc0 = ra;
}				/*21 */
static void bios_strcspn(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;
    v0 = strcspn(pcA0, pcA1);
    pc0 = ra;
}				/*22 */

static void bios_strtok(upse_module_instance_t *ins)
{				// 0x23
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;
    char *pcRet = strtok(pcA0, pcA1);
    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_strstr(upse_module_instance_t *ins)
{				// 0x24
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    char *pcRet = strstr(pcA0, pcA1);

    if (pcRet)
	v0 = a0 + pcRet - pcA0;
    else
	v0 = 0;
    pc0 = ra;
}

/*0x25*/
static void bios_toupper(upse_module_instance_t *ins)
{
    v0 = toupper(a0);
    pc0 = ra;
}

/*0x26*/
static void bios_tolower(upse_module_instance_t *ins)
{
    v0 = tolower(a0);
    pc0 = ra;
}

/*0x27*/
static void bios_bcopy(upse_module_instance_t *ins)
{
    u32 dest = a1, src = a0, len = a2;

    while (len--)
    {
	PSXMu8(ins, dest) = PSXMu8(ins, src);
	dest++;
	src++;
    }
    //memcpy(Ra1,Ra0,a2); 
    pc0 = ra;
}

/*0x28*/
static void bios_bzero(upse_module_instance_t *ins)
{
    u32 dest = a0, len = a1;

    while (len--)
    {
	PSXMu8(ins, dest) = 0;
	dest++;
    }

    //memset(Ra0,0,a1); 
    pc0 = ra;
}

/*0x29*/
static void bios_bcmp(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    v0 = memcmp(pcA0, pcA1, a2);
    pc0 = ra;
}

/*0x2a*/
static void bios_memcpy(upse_module_instance_t *ins)
{
    u32 dest = a0, src = a1, len = a2;

    while (len--)
    {
	PSXMu8(ins, dest) = PSXMu8(ins, src);
	dest++;
	src++;
    }
    //memcpy(Ra0, Ra1, a2); 
    v0 = a0;
    pc0 = ra;
}

static void bios_memset(upse_module_instance_t *ins)	/*0x2b */
{
    u32 len = a2;
    u32 dest = a0;

    while (len--)
    {
	if (PSXM(ins, dest))
	    PSXMu8(ins, dest) = a1;
	dest++;
    }
    //memset(Ra0, a1, a2);
    v0 = a0;
    pc0 = ra;
}

static void bios_memmove(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    memmove(pcA0, pcA1, a2);
    v0 = a0;
    pc0 = ra;
}

/*0x2d*/
static void bios_memcmp(upse_module_instance_t *ins)
{
    char *pcA0 = (char *) Ra0;
    char *pcA1 = (char *) Ra1;

    v0 = memcmp(pcA0, pcA1, a2);
    pc0 = ra;
}

static void bios_memchr(upse_module_instance_t *ins)
{				// 2e
    char *pcA0 = (char *) Ra0;

    void *ret = memchr(pcA0, a1, a2);
    if (ret != NULL)
	v0 = (u32) ((char *) ret - Ra0) + a0;
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_rand(upse_module_instance_t *ins)
{				// 2f
    v0 = 1 + (int) (32767.0 * rand() / (RAND_MAX + 1.0));
    pc0 = ra;
}

static void bios_srand(upse_module_instance_t *ins)
{				// 30
    srand(a0);
    pc0 = ra;
}

static void bios_malloc(upse_module_instance_t *ins)
{				// 33
    u32 chunk;
    u32 fd;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    /* a0:  Number of bytes to allocate. */

    chunk = biosstate->heap_addr;

    /* Search for first chunk that's large enough and not currently
       being used.
     */
    while ((a0 > BFLIP32(((malloc_chunk *) PSXM(ins, chunk))->size)) || (BFLIP32(((malloc_chunk *) PSXM(ins, chunk))->stat) == INUSE))
	chunk = ((malloc_chunk *) PSXM(ins, chunk))->fd;
    //printf("%08x\n",chunk);

    /* split free chunk */
    fd = chunk + sizeof(malloc_chunk) + a0;
    ((malloc_chunk *) PSXM(ins, fd))->stat = ((malloc_chunk *) PSXM(ins, chunk))->stat;
    ((malloc_chunk *) PSXM(ins, fd))->size = BFLIP32(BFLIP32(((malloc_chunk *) PSXM(ins, chunk))->size) - a0);
    ((malloc_chunk *) PSXM(ins, fd))->fd = ((malloc_chunk *) PSXM(ins, chunk))->fd;
    ((malloc_chunk *) PSXM(ins, fd))->bk = chunk;

    /* set new chunk */
    ((malloc_chunk *) PSXM(ins, chunk))->stat = BFLIP32(INUSE);
    ((malloc_chunk *) PSXM(ins, chunk))->size = BFLIP32(a0);
    ((malloc_chunk *) PSXM(ins, chunk))->fd = fd;

    v0 = chunk + sizeof(malloc_chunk);
    v0 |= 0x80000000;
    //      printf ("malloc %lx,%lx\n", v0, a0);
    pc0 = ra;
}

static void bios_InitHeap(upse_module_instance_t *ins)
{				// 39
    malloc_chunk *chunk;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    biosstate->heap_addr = a0;		// Ra0

    chunk = (malloc_chunk *) PSXM(ins, biosstate->heap_addr);
    chunk->stat = 0;
    if (((a0 & 0x1fffff) + a1) >= 0x200000)
	chunk->size = BFLIP32(0x1ffffc - (a0 & 0x1fffff));
    else
	chunk->size = BFLIP32(a1);
    chunk->fd = 0;
    chunk->bk = 0;

    pc0 = ra;
}

static void bios_printf(upse_module_instance_t *ins)
{
    pc0 = ra;
}

static void bios_puts(upse_module_instance_t *ins)
{
    pc0 = ra;

    _DEBUG("%s", Ra0);
}

static void bios_FlushCache(upse_module_instance_t *ins)
{				// 44

    pc0 = ra;
}

static void bios__bu_init(upse_module_instance_t *ins)
{				// 70

    DeliverEvent(ins, 0x11, 0x2);	// 0xf0000011, 0x0004
    DeliverEvent(ins, 0x81, 0x2);	// 0xf4000001, 0x0004

    pc0 = ra;
}

static void bios__96_init(upse_module_instance_t *ins)
{				// 71

    pc0 = ra;
}

static void bios__96_remove(upse_module_instance_t *ins)
{				// 72

    pc0 = ra;
}

/* System calls B0 */

static void bios_SetRCnt(upse_module_instance_t *ins)
{				// 02

    a0 &= 0x3;
    if (a0 != 3)
    {
	u32 mode = 0;

	upse_ps1_counter_set_target(ins, a0, a1);
	if (a2 & 0x1000)
	    mode |= 0x050;	// Interrupt Mode
	if (a2 & 0x0100)
	    mode |= 0x008;	// Count to 0xffff
	if (a2 & 0x0010)
	    mode |= 0x001;	// Timer stop mode
	if (a0 == 2)
	{
	    if (a2 & 0x0001)
		mode |= 0x200;
	}			// System Clock mode
	else
	{
	    if (a2 & 0x0001)
		mode |= 0x100;
	}			// System Clock mode

	upse_ps1_counter_set_mode(ins, a0, mode);
    }
    pc0 = ra;
}

static void bios_GetRCnt(upse_module_instance_t *ins)
{				// 03

    a0 &= 0x3;
    if (a0 != 3)
	v0 = upse_ps1_counter_get_count(ins, a0);
    else
	v0 = 0;
    pc0 = ra;
}

static void bios_StartRCnt(upse_module_instance_t *ins)
{				// 04

    a0 &= 0x3;
    if (a0 != 3)
	psxHu32(ins, 0x1074) |= BFLIP32(1 << (a0 + 4));
    else
	psxHu32(ins, 0x1074) |= BFLIP32(0x1);
    v0 = 1;
    pc0 = ra;
}

static void bios_StopRCnt(upse_module_instance_t *ins)
{				// 05

    a0 &= 0x3;
    if (a0 != 3)
	psxHu32(ins, 0x1074) &= BFLIP32(~(1 << (a0 + 4)));
    else
	psxHu32(ins, 0x1074) &= BFLIP32(~0x1);
    pc0 = ra;
}

static void bios_ResetRCnt(upse_module_instance_t *ins)
{				// 06

    a0 &= 0x3;
    if (a0 != 3)
    {
	upse_ps1_counter_set_mode(ins, a0, 0);
	upse_ps1_counter_set_target(ins, a0, 0);
	upse_ps1_counter_set_count(ins, a0, 0);
    }
    pc0 = ra;
}


/* gets ev for use with Event */
#define GetEv() \
	ev = (a0 >> 24) & 0xf; \
	if (ev == 0xf) ev = 0x5; \
	ev*= 32; \
	ev+= a0&0x1f;

/* gets spec for use with Event */
#define GetSpec() \
	spec = 0; \
	switch (a1) { \
		case 0x0301: spec = 16; break; \
		case 0x0302: spec = 17; break; \
		default: \
			for (i=0; i<16; i++) if (a1 & (1 << i)) { spec = i; break; } \
			break; \
	}

static void bios_DeliverEvent(upse_module_instance_t *ins)
{				// 07
    int ev, spec;
    int i;

    GetEv();
    GetSpec();

    DeliverEvent(ins, ev, spec);

    pc0 = ra;
}

static void bios_OpenEvent(upse_module_instance_t *ins)
{				// 08
    int ev, spec;
    int i;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    GetEv();
    GetSpec();

    biosstate->Event[ev][spec].status = BFLIP32S(EvStWAIT);
    biosstate->Event[ev][spec].mode = BFLIP32(a2);
    biosstate->Event[ev][spec].fhandler = BFLIP32(a3);

    v0 = ev | (spec << 8);
    pc0 = ra;
}

static void bios_CloseEvent(upse_module_instance_t *ins)
{				// 09
    int ev, spec;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    ev = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;

    biosstate->Event[ev][spec].status = BFLIP32S(EvStUNUSED);

    v0 = 1;
    pc0 = ra;
}

static void bios_WaitEvent(upse_module_instance_t *ins)
{				// 0a
    int ev, spec;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    ev = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;

    biosstate->Event[ev][spec].status = BFLIP32S(EvStACTIVE);

    v0 = 1;
    pc0 = ra;
}

static void bios_TestEvent(upse_module_instance_t *ins)
{				// 0b
    int ev, spec;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    ev = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;

    if (biosstate->Event[ev][spec].status == BFLIP32S(EvStALREADY))
    {
	biosstate->Event[ev][spec].status = BFLIP32S(EvStACTIVE);
	v0 = 1;
    }
    else
	v0 = 0;

    pc0 = ra;
}

static void bios_EnableEvent(upse_module_instance_t *ins)
{				// 0c
    int ev, spec;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    ev = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;

    biosstate->Event[ev][spec].status = BFLIP32S(EvStACTIVE);

    v0 = 1;
    pc0 = ra;
}

static void bios_DisableEvent(upse_module_instance_t *ins)
{				// 0d
    int ev, spec;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    ev = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;

    biosstate->Event[ev][spec].status = BFLIP32S(EvStWAIT);

    v0 = 1;
    pc0 = ra;
}

/*
 *	long OpenTh(long (*func)(), unsigned long sp, unsigned long gp);
 */

static void bios_OpenTh(upse_module_instance_t *ins)
{				// 0e
    int th;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    for (th = 1; th < 8; th++)
	if (biosstate->Thread[th].status == 0)
	    break;

    biosstate->Thread[th].status = BFLIP32(1);
    biosstate->Thread[th].func = BFLIP32(a0);
    biosstate->Thread[th].reg[29] = BFLIP32(a1);
    biosstate->Thread[th].reg[28] = BFLIP32(a2);

    v0 = th;
    pc0 = ra;
}

/*
 *	int CloseTh(long thread);
 */

static void bios_CloseTh(upse_module_instance_t *ins)
{				// 0f
    int th = a0 & 0xff;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    if (biosstate->Thread[th].status == 0)
    {
	v0 = 0;
    }
    else
    {
	biosstate->Thread[th].status = 0;
	v0 = 1;
    }

    pc0 = ra;
}

/*
 *	int ChangeTh(long thread);
 */

static void bios_ChangeTh(upse_module_instance_t *ins)
{				// 10
    int th = a0 & 0xff;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    if (biosstate->Thread[th].status == 0 || biosstate->CurThread == th)
    {
	v0 = 0;

	pc0 = ra;
    }
    else
    {
	v0 = 1;

	if (biosstate->Thread[biosstate->CurThread].status == BFLIP32S(2))
	{
	    biosstate->Thread[biosstate->CurThread].status = BFLIP32S(1);
	    biosstate->Thread[biosstate->CurThread].func = BFLIP32(ra);
	    memcpy(biosstate->Thread[biosstate->CurThread].reg, ins->cpustate.GPR.r, 32 * 4);
	}

	memcpy(ins->cpustate.GPR.r, biosstate->Thread[th].reg, 32 * 4);
	pc0 = BFLIP32(biosstate->Thread[th].func);
	biosstate->Thread[th].status = BFLIP32(2);
	biosstate->CurThread = th;
    }
}

static void bios_ReturnFromException(upse_module_instance_t *ins)
{				// 17
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    memcpy(ins->cpustate.GPR.r, biosstate->regs, 32 * 4);
    ins->cpustate.GPR.n.lo = biosstate->regs[32];
    ins->cpustate.GPR.n.hi = biosstate->regs[33];

    pc0 = ins->cpustate.CP0.n.EPC;
    if (ins->cpustate.CP0.n.Cause & 0x80000000)
	pc0 += 4;

    ins->cpustate.CP0.n.Status = (ins->cpustate.CP0.n.Status & 0xfffffff0) | ((ins->cpustate.CP0.n.Status & 0x3c) >> 2);
}

static void bios_ResetEntryInt(upse_module_instance_t *ins)
{				// 18
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    biosstate->jmp_int = NULL;
    pc0 = ra;
}

static void bios_HookEntryInt(upse_module_instance_t *ins)
{				// 19
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    biosstate->jmp_int = (u32 *) Ra0;
    pc0 = ra;
}

static void bios_UnDeliverEvent(upse_module_instance_t *ins)
{				// 0x20
    int ev, spec;
    int i;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    GetEv();
    GetSpec();

    if (biosstate->Event[ev][spec].status == BFLIP32S(EvStALREADY) && biosstate->Event[ev][spec].mode == BFLIP32S(EvMdNOINTR))
	biosstate->Event[ev][spec].status = BFLIP32S(EvStACTIVE);

    pc0 = ra;
}

static void bios_GetC0Table(upse_module_instance_t *ins)
{				// 56

    v0 = 0x674;
    pc0 = ra;
}

static void bios_GetB0Table(upse_module_instance_t *ins)
{				// 57

    v0 = 0x874;
    pc0 = ra;
}

/* System calls C0 */

/*
 * int SysEnqIntRP(int index , long *queue);
 */

static void bios_SysEnqIntRP(upse_module_instance_t *ins)
{				// 02
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    biosstate->SysIntRP[a0] = a1;

    v0 = 0;
    pc0 = ra;
}

/*
 * int SysDeqIntRP(int index , long *queue);
 */

static void bios_SysDeqIntRP(upse_module_instance_t *ins)
{				// 03
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    biosstate->SysIntRP[a0] = 0;

    v0 = 0;
    pc0 = ra;
}

static void bios_ChangeClearRCnt(upse_module_instance_t *ins)
{				// 0a
    u32 *ptr;

    ptr = (u32 *) PSXM(ins, (a0 << 2) + 0x8600);
    v0 = BFLIP32(*ptr);
    *ptr = BFLIP32(a1);

    ins->cpustate.CP0.n.Status|= 0x404;
    pc0 = ra;
}

static void bios_dummy(upse_module_instance_t *ins)
{
    u32 call = t1 & 0xff;

    pc0 = ra;
    v0 = a1;

    _DEBUG("WTF: unknown bios call: %x", call);
}

void (*biosA0[256]) (upse_module_instance_t *ins);
void (*biosB0[256]) (upse_module_instance_t *ins);
void (*biosC0[256]) (upse_module_instance_t *ins);

void upse_ps1_bios_init(upse_module_instance_t *ins)
{
    u32 base, size;
    u32 *ptr;
    int i;
    upse_ps1_bios_state_t *biosstate;

    if (upse_has_custom_bios())
        return;

    biosstate = calloc(sizeof(upse_ps1_bios_state_t), 1);
    biosstate->heap_addr = 0;
    biosstate->CurThread = 0;
    biosstate->jmp_int = NULL;

    for (i = 0; i < 256; i++)
    {
	biosA0[i] = NULL;
	biosB0[i] = NULL;
	biosC0[i] = NULL;
    }

    for (i = 0; i < 256; i++)
    {
	if (biosA0[i] == NULL)
	    biosA0[i] = bios_dummy;
	if (biosB0[i] == NULL)
	    biosB0[i] = bios_dummy;
	if (biosC0[i] == NULL)
	    biosC0[i] = bios_dummy;
    }

    biosA0[0x0e] = bios_abs;
    biosA0[0x0f] = bios_labs;
    biosA0[0x10] = bios_atoi;
    biosA0[0x11] = bios_atol;
    //biosA0[0x12] = bios_atob;
    biosA0[0x13] = bios_setjmp;
    biosA0[0x14] = bios_longjmp;

    biosA0[0x15] = bios_strcat;
    biosA0[0x16] = bios_strncat;
    biosA0[0x17] = bios_strcmp;
    biosA0[0x18] = bios_strncmp;
    biosA0[0x19] = bios_strcpy;
    biosA0[0x1a] = bios_strncpy;
    biosA0[0x1b] = bios_strlen;
    biosA0[0x1c] = bios_index;
    biosA0[0x1d] = bios_rindex;
    biosA0[0x1e] = bios_strchr;
    biosA0[0x1f] = bios_strrchr;
    biosA0[0x20] = bios_strpbrk;
    biosA0[0x21] = bios_strspn;
    biosA0[0x22] = bios_strcspn;
    biosA0[0x23] = bios_strtok;
    biosA0[0x24] = bios_strstr;
    biosA0[0x25] = bios_toupper;
    biosA0[0x26] = bios_tolower;
    biosA0[0x27] = bios_bcopy;
    biosA0[0x28] = bios_bzero;
    biosA0[0x29] = bios_bcmp;
    biosA0[0x2a] = bios_memcpy;
    biosA0[0x2b] = bios_memset;
    biosA0[0x2c] = bios_memmove;
    biosA0[0x2c] = bios_memcpy;	/* Our code should be compatible
				   with both memcpy and memmove 
				   semantics. */
    biosA0[0x2d] = bios_memcmp;
    biosA0[0x2e] = bios_memchr;

    biosA0[0x2f] = bios_rand;
    biosA0[0x30] = bios_srand;

    //biosA0[0x31] = bios_qsort;
    //biosA0[0x32] = bios_strtod;
    biosA0[0x33] = bios_malloc;
    //biosA0[0x34] = bios_free;
    //biosA0[0x35] = bios_lsearch;
    //biosA0[0x36] = bios_bsearch;
    //biosA0[0x37] = bios_calloc;
    //biosA0[0x38] = bios_realloc;
    biosA0[0x39] = bios_InitHeap;
    //biosA0[0x3a] = bios__exit;
    biosA0[0x3e] = bios_puts;
    biosA0[0x3f] = bios_printf;
    biosA0[0x44] = bios_FlushCache;
    //biosA0[0x45] = bios_InstallInterruptHandler;
    //biosA0[0x4f] = bios_sys_a0_4f;
    //biosA0[0x50] = bios_sys_a0_50;                
    biosA0[0x70] = bios__bu_init;
    biosA0[0x71] = bios__96_init;
    biosA0[0x72] = bios__96_remove;
    //biosA0[0x73] = bios_sys_a0_73;
    //biosA0[0x74] = bios_sys_a0_74;
    //biosA0[0x75] = bios_sys_a0_75;
    //biosA0[0x76] = bios_sys_a0_76;
    //biosA0[0x77] = bios_sys_a0_77;
    //biosA0[0x78] = bios__96_CdSeekL;
    //biosA0[0x79] = bios_sys_a0_79;
    //biosA0[0x7a] = bios_sys_a0_7a;
    //biosA0[0x7b] = bios_sys_a0_7b;
    //biosA0[0x7c] = bios__96_CdGetStatus;
    //biosA0[0x7d] = bios_sys_a0_7d;
    //biosA0[0x7e] = bios__96_CdRead;
    //biosA0[0x7f] = bios_sys_a0_7f;
    //biosA0[0x80] = bios_sys_a0_80;
    //biosA0[0x81] = bios_sys_a0_81;
    //biosA0[0x82] = bios_sys_a0_82;                
    //biosA0[0x83] = bios_sys_a0_83;
    //biosA0[0x84] = bios_sys_a0_84;
    //biosA0[0x85] = bios__96_CdStop;       
    //biosA0[0x86] = bios_sys_a0_86;
    //biosA0[0x87] = bios_sys_a0_87;
    //biosA0[0x88] = bios_sys_a0_88;
    //biosA0[0x89] = bios_sys_a0_89;
    //biosA0[0x8a] = bios_sys_a0_8a;
    //biosA0[0x8b] = bios_sys_a0_8b;
    //biosA0[0x8c] = bios_sys_a0_8c;
    //biosA0[0x8d] = bios_sys_a0_8d;
    //biosA0[0x8e] = bios_sys_a0_8e;                
    //biosA0[0x8f] = bios_sys_a0_8f;
    //biosA0[0x90] = bios_sys_a0_90;
    //biosA0[0x91] = bios_sys_a0_91;
    //biosA0[0x92] = bios_sys_a0_92;
    //biosA0[0x93] = bios_sys_a0_93;
    //biosA0[0x94] = bios_sys_a0_94;
    //biosA0[0x95] = bios_sys_a0_95;
    //biosA0[0x96] = bios_AddCDROMDevice;
    //biosA0[0x97] = bios_AddMemCardDevide;
    //biosA0[0x98] = bios_DisableKernelIORedirection;
    //biosA0[0x99] = bios_EnableKernelIORedirection;
    //biosA0[0x9a] = bios_sys_a0_9a;
    //biosA0[0x9b] = bios_sys_a0_9b;
    //biosA0[0x9c] = bios_SetConf;
    //biosA0[0x9d] = bios_GetConf;
    //biosA0[0x9e] = bios_sys_a0_9e;
    //biosA0[0x9f] = bios_SetMem;
    //biosA0[0xa0] = bios__boot;
    //biosA0[0xa1] = bios_SystemError;
    //biosA0[0xa2] = bios_EnqueueCdIntr;
    //biosA0[0xa3] = bios_DequeueCdIntr;
    //biosA0[0xa4] = bios_sys_a0_a4;
    //biosA0[0xa5] = bios_ReadSector;
    //biosA0[0xa6] = bios_get_cd_status;
    //biosA0[0xa7] = bios_bufs_cb_0;
    //biosA0[0xa8] = bios_bufs_cb_1;
    //biosA0[0xa9] = bios_bufs_cb_2;
    //biosA0[0xaa] = bios_bufs_cb_3;
    //biosA0[0xab] = bios__card_info;
    //biosA0[0xac] = bios__card_load;
    //biosA0[0axd] = bios__card_auto;
    //biosA0[0xae] = bios_bufs_cd_4;
    //biosA0[0xaf] = bios_sys_a0_af;
    //biosA0[0xb0] = bios_sys_a0_b0;
    //biosA0[0xb1] = bios_sys_a0_b1;
    //biosA0[0xb2] = bios_do_a_long_jmp
    //biosA0[0xb3] = bios_sys_a0_b3;
    //biosA0[0xb4] = bios_sub_function;
//*******************B0 CALLS****************************
    //biosB0[0x00] = bios_SysMalloc;
    //biosB0[0x01] = bios_sys_b0_01;
    biosB0[0x02] = bios_SetRCnt;
    biosB0[0x03] = bios_GetRCnt;
    biosB0[0x04] = bios_StartRCnt;
    biosB0[0x05] = bios_StopRCnt;
    biosB0[0x06] = bios_ResetRCnt;
    biosB0[0x07] = bios_DeliverEvent;
    biosB0[0x08] = bios_OpenEvent;
    biosB0[0x09] = bios_CloseEvent;
    biosB0[0x0a] = bios_WaitEvent;
    biosB0[0x0b] = bios_TestEvent;
    biosB0[0x0c] = bios_EnableEvent;
    biosB0[0x0d] = bios_DisableEvent;
    biosB0[0x0e] = bios_OpenTh;
    biosB0[0x0f] = bios_CloseTh;
    biosB0[0x10] = bios_ChangeTh;
    //biosB0[0x11] = bios_bios_b0_11;
    biosB0[0x17] = bios_ReturnFromException;
    biosB0[0x18] = bios_ResetEntryInt;
    biosB0[0x19] = bios_HookEntryInt;
    //biosB0[0x1a] = bios_sys_b0_1a;
    //biosB0[0x1b] = bios_sys_b0_1b;
    //biosB0[0x1c] = bios_sys_b0_1c;
    //biosB0[0x1d] = bios_sys_b0_1d;
    //biosB0[0x1e] = bios_sys_b0_1e;
    //biosB0[0x1f] = bios_sys_b0_1f;
    biosB0[0x20] = bios_UnDeliverEvent;
    //biosB0[0x21] = bios_sys_b0_21;
    //biosB0[0x22] = bios_sys_b0_22;
    //biosB0[0x23] = bios_sys_b0_23;
    //biosB0[0x24] = bios_sys_b0_24;
    //biosB0[0x25] = bios_sys_b0_25;
    //biosB0[0x26] = bios_sys_b0_26;
    //biosB0[0x27] = bios_sys_b0_27;
    //biosB0[0x28] = bios_sys_b0_28;
    //biosB0[0x29] = bios_sys_b0_29;
    //biosB0[0x2a] = bios_sys_b0_2a;
    //biosB0[0x2b] = bios_sys_b0_2b;
    //biosB0[0x2c] = bios_sys_b0_2c;
    //biosB0[0x2d] = bios_sys_b0_2d;
    //biosB0[0x2e] = bios_sys_b0_2e;
    //biosB0[0x2f] = bios_sys_b0_2f;
    //biosB0[0x30] = bios_sys_b0_30;
    //biosB0[0x31] = bios_sys_b0_31;
    biosB0[0x3f] = bios_puts;
    biosB0[0x56] = bios_GetC0Table;
    biosB0[0x57] = bios_GetB0Table;
    //biosB0[0x58] = bios__card_chan;
    //biosB0[0x59] = bios_sys_b0_59;
    //biosB0[0x5a] = bios_sys_b0_5a;
    //biosB0[0x5c] = bios__card_status;
    //biosB0[0x5d] = bios__card_wait;
//*******************C0 CALLS****************************
    //biosC0[0x00] = bios_InitRCnt;
    //biosC0[0x01] = bios_InitException;
    biosC0[0x02] = bios_SysEnqIntRP;
    biosC0[0x03] = bios_SysDeqIntRP;
    //biosC0[0x04] = bios_get_free_EvCB_slot;
    //biosC0[0x05] = bios_get_free_TCB_slot;
    //biosC0[0x06] = bios_ExceptionHandler;
    //biosC0[0x07] = bios_InstallExeptionHandler;
    //biosC0[0x08] = bios_SysInitMemory;
    //biosC0[0x09] = bios_SysInitKMem;
    biosC0[0x0a] = bios_ChangeClearRCnt;
    //biosC0[0x0b] = bios_SystemError;
    //biosC0[0x0c] = bios_InitDefInt;
    //biosC0[0x0d] = bios_sys_c0_0d;
    //biosC0[0x0e] = bios_sys_c0_0e;
    //biosC0[0x0f] = bios_sys_c0_0f;
    //biosC0[0x10] = bios_sys_c0_10;
    //biosC0[0x11] = bios_sys_c0_11;
    //biosC0[0x12] = bios_InstallDevices;
    //biosC0[0x13] = bios_FlushStfInOutPut;
    //biosC0[0x14] = bios_sys_c0_14;
    //biosC0[0x15] = bios__cdevinput;
    //biosC0[0x16] = bios__cdevscan;
    //biosC0[0x17] = bios__circgetc;
    //biosC0[0x18] = bios__circputc;                  
    //biosC0[0x19] = bios_ioabort;
    //biosC0[0x1a] = bios_sys_c0_1a
    //biosC0[0x1b] = bios_KernelRedirect;
    //biosC0[0x1c] = bios_PatchAOTable;
    biosC0[0x3f] = bios_printf;
//************** THE END ***************************************

    base = 0x1000;
    size = sizeof(EvCB) * 32;
    biosstate->Event = (void *) &ins->psxR[base];
    base += size * 6;
    memset(biosstate->Event, 0, size * 6);
    biosstate->RcEV = biosstate->Event + 32 * 2;

    ptr = (u32 *) & ins->psxM[0x0874];	// b0 table
    ptr[0] = BFLIP32(0x4c54 - 0x884);

    ptr = (u32 *) & ins->psxM[0x0674];	// c0 table
    ptr[6] = BFLIP32(0xc80);

    memset(biosstate->SysIntRP, 0, sizeof(biosstate->SysIntRP));
    memset(biosstate->Thread, 0, sizeof(biosstate->Thread));
    biosstate->Thread[0].status = BFLIP32(2);	// main thread

    PSXMu32(ins, 0x0150) = BFLIP32(0x160);
    PSXMu32(ins, 0x0154) = BFLIP32(0x320);
    PSXMu32(ins, 0x0160) = BFLIP32(0x248);
    strcpy(&ins->psxM[0x248], "bu");

	PSXMu32(ins, 0x0ca8) = BFLIP32(0x1f410004);
	PSXMu32(ins, 0x0cf0) = BFLIP32(0x3c020000);
	PSXMu32(ins, 0x0cf4) = BFLIP32(0x2442641c);
	PSXMu32(ins, 0x09e0) = BFLIP32(0x43d0);
	PSXMu32(ins, 0x4d98) = BFLIP32(0x946f000a);

    // opcode HLE
    psxRu32(ins, 0x0000) = BFLIP32((0x3b << 26) | 4);
    PSXMu32(ins, 0x0000) = BFLIP32((0x3b << 26) | 0);
    PSXMu32(ins, 0x00a0) = BFLIP32((0x3b << 26) | 1);
    PSXMu32(ins, 0x00b0) = BFLIP32((0x3b << 26) | 2);
    PSXMu32(ins, 0x00c0) = BFLIP32((0x3b << 26) | 3);
    PSXMu32(ins, 0x4c54) = BFLIP32((0x3b << 26) | 0);
    PSXMu32(ins, 0x8000) = BFLIP32((0x3b << 26) | 5);
    PSXMu32(ins, 0x07a0) = BFLIP32((0x3b << 26) | 0);
    PSXMu32(ins, 0x0884) = BFLIP32((0x3b << 26) | 0);
    PSXMu32(ins, 0x0894) = BFLIP32((0x3b << 26) | 0);

    ins->biosstate = biosstate;
}

void upse_ps1_bios_shutdown(upse_module_instance_t *ins)
{
    free(ins->biosstate);
}

void biosInterrupt(upse_module_instance_t *ins)
{
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    if (BFLIP32(psxHu32(ins, 0x1070)) & 0x1)
    {				// Vsync
	if (biosstate->RcEV[3][1].status == BFLIP32S(EvStACTIVE))
	{
	    softCall(ins, BFLIP32(biosstate->RcEV[3][1].fhandler));
//                        hwwrite_32(0x1f801070, ~(1));
	}
    }

    if (BFLIP32(psxHu32(ins, 0x1070)) & 0x70)
    {				// Rcnt 0,1,2
	int i;

	for (i = 0; i < 3; i++)
	{
	    if (BFLIP32(psxHu32(ins, 0x1070)) & (1 << (i + 4)))
	    {
		if (biosstate->RcEV[i][1].status == BFLIP32S(EvStACTIVE))
		{
		    softCall(ins, BFLIP32(biosstate->RcEV[i][1].fhandler));
		    upse_ps1_hal_write_32(ins, 0x1f801070, ~(1 << (i + 4)));
		}
	    }
	}
    }
}

static INLINE void SaveRegs(upse_module_instance_t *ins)
{
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    memcpy(biosstate->regs, ins->cpustate.GPR.r, 32 * 4);
    biosstate->regs[32] = ins->cpustate.GPR.n.lo;
    biosstate->regs[33] = ins->cpustate.GPR.n.hi;
    biosstate->regs[34] = ins->cpustate.pc;
}

void upse_ps1_bios_exception(upse_module_instance_t *ins)
{
    int i;
    upse_ps1_bios_state_t *biosstate = ins->biosstate;

    switch (ins->cpustate.CP0.n.Cause & 0x3c)
    {
      case 0x00:		// Interrupt
#ifdef PSXCPU_LOG
//                      PSXCPU_LOG("interrupt\n");
#endif
	  SaveRegs(ins);

	  biosInterrupt(ins);

	  for (i = 0; i < 8; i++)
	  {
	      if (biosstate->SysIntRP[i])
	      {
		  u32 *queue = (u32 *) PSXM(ins, biosstate->SysIntRP[i]);

		  s0 = BFLIP32(queue[2]);
		  softCall(ins, BFLIP32(queue[1]));
	      }
	  }

	  if (biosstate->jmp_int != NULL)
	  {
	      int i;

	      upse_ps1_hal_write_32(ins, 0x1f801070, 0xffffffff);

	      ra = BFLIP32(biosstate->jmp_int[0]);
	      sp = BFLIP32(biosstate->jmp_int[1]);
	      fp = BFLIP32(biosstate->jmp_int[2]);
	      for (i = 0; i < 8; i++)	// s0-s7
		  ins->cpustate.GPR.r[16 + i] = BFLIP32(biosstate->jmp_int[3 + i]);
	      gp = BFLIP32(biosstate->jmp_int[11]);

	      v0 = 1;
	      pc0 = ra;
	      return;
	  }
	  upse_ps1_hal_write_16(ins, 0x1f801070, 0);
	  break;
      case 0x20:		// Syscall
#ifdef PSXCPU_LOG
//                      PSXCPU_LOG("syscall exp %x\n", a0);
#endif
	  switch (a0)
	  {
	    case 1:		// EnterCritical - disable irq's
		ins->cpustate.CP0.n.Status &= ~0x404;
		break;
	    case 2:		// ExitCritical - enable irq's
		ins->cpustate.CP0.n.Status |= 0x404;
		break;
	  }
	  pc0 = ins->cpustate.CP0.n.EPC + 4;

	  ins->cpustate.CP0.n.Status = (ins->cpustate.CP0.n.Status & 0xfffffff0) | ((ins->cpustate.CP0.n.Status & 0x3c) >> 2);
	  return;
      default:
          _DEBUG("WTF: unknown exception <%x>", ins->cpustate.CP0.n.Cause & 0x3c);
	  break;
    }

    pc0 = ins->cpustate.CP0.n.EPC;
    if (ins->cpustate.CP0.n.Cause & 0x80000000)
	pc0 += 4;

    ins->cpustate.CP0.n.Status = (ins->cpustate.CP0.n.Status & 0xfffffff0) | ((ins->cpustate.CP0.n.Status & 0x3c) >> 2);
}
