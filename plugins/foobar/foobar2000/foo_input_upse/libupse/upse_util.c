/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_util.c
 * Purpose: libupse: utility functions
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

#define _ISOC99_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "upse-internal.h"
#include "upse.h"
#include "upse-string.h"

float
upse_strtof(const char *value)
{
    float val;

    _ENTER;

    val = atof(value);

    _LEAVE;

    return val;
}

u8 *
upse_get_buffer(void *fp, const upse_iofuncs_t *funcs, u32 *len_)
{
    int len, pos;
    u8 *buf;

    _ENTER;

    pos = funcs->tell_impl(fp);

    funcs->seek_impl(fp, 0, SEEK_END);
    len = funcs->tell_impl(fp);
    funcs->seek_impl(fp, 0, SEEK_SET);

    buf = calloc(sizeof(u8), len);
    funcs->read_impl(buf, len, 1, fp);

    funcs->seek_impl(fp, pos, SEEK_SET);

    if (len_ != NULL)
        *len_ = len;

    _LEAVE;

    return buf;
}
