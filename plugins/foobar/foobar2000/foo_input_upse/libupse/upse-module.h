/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-module.h
 * Purpose: libupse: Module loading and probing.
 *
 * Copyright (c) 2008, 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#ifndef __UPSE__LIBUPSE__UPSE_MODULE_H__GUARD
#define __UPSE__LIBUPSE__UPSE_MODULE_H__GUARD

typedef union
{
    struct
    {
     	u32 r0, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0, k1, gp, sp, s8, ra, lo, hi;
    } n upse_packed_t;
    u32 r[34];                  /* Lo, Hi in r[33] and r[34] */
} upse_r3000_gpr_regs_t;

typedef union
{
    struct
    {
     	u32 Index, Random, EntryLo0, EntryLo1,
            Context, PageMask, Wired, Reserved0,
            BadVAddr, Count, EntryHi, Compare,
            Status, Cause, EPC, PRid, Config, LLAddr, WatchLO, WatchHI, XContext, Reserved1, Reserved2, Reserved3, Reserved4, Reserved5, ECC, CacheErr, TagLo, TagHi, ErrorEPC, Reserved6;
    } n upse_packed_t;
    u32 r[32];
} upse_r3000_cp0_regs_t;

typedef struct
{
    upse_r3000_gpr_regs_t GPR;          /* General Purpose Registers */
    upse_r3000_cp0_regs_t CP0;          /* Coprocessor0 Registers */
    u32 pc;                     /* Program counter */
    u32 code;                   /* The instruction */
    u32 cycle;
    u32 interrupt;

    int branch;
    int branch2;
    u32 branchPC;
} upse_r3000_cpu_registers_t;

typedef struct {
    void *spu;
    void *ctrstate;
    void *biosstate;

    char psxM[0x200000];
    char psxP[0x10000];
    char psxR[0x80000];
    char psxH[0x10000];
    char *upse_ps1_memory_LUT[0x10000];
    int writeok;

    upse_r3000_cpu_registers_t cpustate;

    u32 lowest_addr;
    u32 highest_addr;
    u32 highest_addr_size;

    void (*spu_irq_callback)(void);
} upse_module_instance_t;

typedef void (*upse_eventloop_func_t)(upse_module_instance_t *ins);
typedef int (*upse_eventloop_render_func_t)(upse_module_instance_t *ins, s16 **samples);
typedef void (*upse_eventloop_setcb_func_t)(upse_module_instance_t *ins, upse_audio_callback_func_t func, const void *user_data);
typedef int (*upse_eventloop_seek_func_t)(upse_module_instance_t *ins, u32 t);

typedef struct {
    void *opaque;
    upse_psf_t *metadata; /* XXX */
    upse_eventloop_func_t evloop_run;
    upse_eventloop_func_t evloop_stop;
    upse_eventloop_render_func_t evloop_render;
    upse_eventloop_setcb_func_t evloop_setcb;
    upse_eventloop_seek_func_t evloop_seek;
    upse_module_instance_t instance;
} upse_module_t;

typedef upse_module_t *(*upse_loader_func_t)(void *fileptr, const char *path, const upse_iofuncs_t *iofuncs);

upse_loader_func_t upse_module_probe(void *fileptr, const upse_iofuncs_t *funcs);
int upse_module_is_supported(void *fileptr, const upse_iofuncs_t *funcs);
int upse_file_is_supported(char *file, const upse_iofuncs_t *funcs);
upse_module_t *upse_module_open(const char *file, const upse_iofuncs_t *funcs);
void upse_module_close(upse_module_t *mod);
void upse_module_init(void);

#endif
