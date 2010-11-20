/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_filesystem.c
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

#include "upse-filesystem.h"

upse_filesystem_t *
upse_filesystem_new(void)
{
    upse_filesystem_t *out = calloc(sizeof(upse_filesystem_t), 1);

    return out;
}

void
upse_filesystem_attach_path(upse_filesystem_t *fs, const char *path, u8 *data, u32 len)
{
    upse_filesystem_entry_t *entry = calloc(sizeof(upse_filesystem_entry_t), 1);

    entry->prev = fs->tail;
    if (entry->prev != NULL)
    {
        entry->next = entry->prev->next;
        entry->prev->next = entry;
    }

    if (fs->head == NULL)
        fs->head = entry;

    if (fs->tail == NULL)
        fs->tail = entry;

    entry->filename = strdup(path);
    entry->data = malloc(len);
    entry->len = len;

    memcpy(entry->data, data, len);
}

int
upse_filesystem_get_path(upse_filesystem_t *fs, const char *path, u8 **data, u32 *len)
{
    upse_filesystem_entry_t *iter;

    for (iter = fs->head; iter != NULL; iter = iter->next)
    {
        if (!strcasecmp(path, iter->filename))
        {
            *data = iter->data;
            *len = iter->len;

            return 1;
        }
    }

    return 0;
}
