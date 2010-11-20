/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-filesystem.h
 * Purpose: libupse: Filesystem container.
 *
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include "upse-types.h"

#ifndef __UPSE_LIBUPSE_UPSE_FILESYSTEM_H__GUARD
#define __UPSE_LIBUPSE_UPSE_FILESYSTEM_H__GUARD

typedef struct _upse_filesystem_entry {
    struct _upse_filesystem_entry *prev, *next;
    u32 len;
    u8 *data;
    char *filename;
} upse_filesystem_entry_t;

typedef struct {
    upse_filesystem_entry_t *head, *tail;
} upse_filesystem_t;

upse_filesystem_t *upse_filesystem_new(void);
void upse_filesystem_attach_path(upse_filesystem_t *fs, const char *path, u8 *data, u32 len);
int upse_filesystem_get_path(upse_filesystem_t *fs, const char *path, u8 **data, u32 *len);

#endif
