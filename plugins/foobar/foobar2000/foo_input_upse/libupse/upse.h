/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse.h
 * Purpose: libupse development header
 *
 * Copyright (c) 2007 William Pitcock <nenolod@sacredspiral.co.uk>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#ifndef __UPSE_LIBUPSE_UPSE_H_GUARD
#define __UPSE_LIBUPSE_UPSE_H_GUARD

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#else
#include <unistd.h>
#endif

#include "upse-types.h"
#include "upse-container-xsf.h"

typedef struct
{
    u16 rate;
    u32 length;
    u32 stop;
    u32 fade;
    u32 volume;
    char *title;
    char *artist;
    char *game;
    char *year;
    char *genre;
    char *psfby;
    char *comment;
    char *copyright;
    upse_xsf_t *xsf;
} upse_psf_t;

typedef struct
{
    void *(*open_impl) (const char *path, const char *mode);
    size_t(*read_impl) (void *ptr, size_t size, size_t nmemb, void *file);
    int (*seek_impl) (void *file, long offset, int whence);
    int (*close_impl) (void *file);
    long (*tell_impl) (void *file);
} upse_iofuncs_t;

upse_psf_t *upse_get_psf_metadata(const char *path, const upse_iofuncs_t * iofuncs);
void upse_free_psf_metadata(upse_psf_t * info);

typedef void (*upse_audio_callback_func_t) (unsigned char *, long, const void *);

/*
 * Set the interpolation mode.
 *
 *   1: interpolation (like a real PSX/PS2)
 *   0: interpolation only on reverb
 *      (PeOPS/Neill Corlett's model says interpolation always but doesn't do it)
 */
void upse_set_interpolation_mode(u32 setting);

/*
 * Set a custom interpolation coefficient.
 * Implies that interpolation_mode is non-zero.
 */
void upse_set_interpolation_coefficient(double setting);

/*
 * Enable or disable reverb.
 */
void upse_set_reverb_mode(u32 setting);

/*
 * Enable or disable non-downsampled reverb. [EXPERIMENTAL]
 */
void upse_set_reverb_no_downsample(u32 setting);

/*
 * Custom BIOS stuff.
 */
int upse_has_custom_bios(void);
const char *upse_get_custom_bios(void);
void upse_set_custom_bios(const char *bios);

/*
 * Utility functions.
 */
float upse_strtof(const char *value);

#include "upse-eventloop.h"
#include "upse-module.h"

#endif
