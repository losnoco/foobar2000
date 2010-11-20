/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-loader.h
 * Purpose: libupse: Loader management.
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

#include "upse-internal.h"

typedef struct _upse_loader_list {
    const char *bytes;
    int length;
    int offset;
    upse_loader_func_t func;
    struct _upse_loader_list *prev, *next;
} upse_loader_list_t;

static upse_loader_list_t *loader_list_ = NULL;

void
upse_loader_add_magic(const char *bytes, int length, int offset, upse_loader_func_t func)
{
    upse_loader_list_t *loader = (upse_loader_list_t *) calloc(sizeof(upse_loader_list_t), 1);

    loader->bytes = bytes;
    loader->length = length;
    loader->offset = offset;
    loader->func = func;

    loader->next = loader_list_;

    if (loader->next)
        loader->next->prev = loader;

    loader_list_ = loader;
}

void
upse_loader_del_magic(const char *bytes, int length, int offset)
{
    upse_loader_list_t *iter, *iter2;

    for (iter = loader_list_, iter2 = iter->next; iter2 != NULL; iter = iter2, iter2 = iter2->next)
    {
         if (iter->length != length)
             continue;

         if (iter->offset != offset)
             continue;

         if (!memcmp(iter->bytes, bytes, iter->length))
         {
             if (iter->prev)
                 iter->prev->next = iter->next;

             if (iter->next)
                 iter->next->prev = iter->prev;

             if (loader_list_ == iter)
                 loader_list_ = iter->next;

             free(iter);
         }
    }
}

/*
 * this compares loader elements by offset, ensuring that the list
 * is ordered sequentially from offset=0 forward.
 *
 * this means that all seeks in the file are seeking forward, which
 * makes things very I/O friendly, and also means that we use
 * filesystem/stdio precaching to our advantage, as well as generally
 * reducing syscalls overall (e.g. comparing everything at offset X at
 * the same time)
 */
static int
upse_loader_compare_offset(const void *a, const void *b)
{
    const upse_loader_t *elem_a = a;
    const upse_loader_t *elem_b = b;

    return (elem_b->offset - elem_a->offset);
}

/*
 * called at end of loader registration, qsorts by offset to ensure seek(2) reduction
 * and efficient ordering, see above for a more detailed explanation.
 */
upse_loader_t *
upse_loader_prepare_table(void)
{
    upse_loader_t *table;
    upse_loader_list_t *iter;
    int count, elems;

    /* count and allocate the amount of loaders in the table. */
    for (elems = 1, iter = loader_list_; iter != NULL; elems++, iter = iter->next);
    table = (upse_loader_t *) calloc(sizeof(upse_loader_t), elems);

    /* ... and populate it. */
    for (iter = loader_list_, count = 0; iter != NULL; count++, iter = iter->next)
    {
        table[count].length = iter->length;
        table[count].offset = iter->offset;
        table[count].bytes  = iter->bytes;
        table[count].func   = iter->func;
    }

    /* optimize table to reduce seek(2) calls by ordering sequentially by offset */
    qsort(table, elems, sizeof(upse_loader_t), upse_loader_compare_offset);
    return table;
}
