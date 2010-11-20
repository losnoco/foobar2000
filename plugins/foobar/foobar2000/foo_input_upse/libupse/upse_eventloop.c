/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_eventloop.c
 * Purpose: libupse: System-specific event loop management.
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

void
upse_eventloop_run(upse_module_t *mod)
{
    if (mod->evloop_run)
        mod->evloop_run(&mod->instance);
}

void
upse_eventloop_stop(upse_module_t *mod)
{
    if (mod->evloop_stop)
        mod->evloop_stop(&mod->instance);
}

int
upse_eventloop_render(upse_module_t *mod, s16 **samples)
{
    if (mod->evloop_render)
        return mod->evloop_render(&mod->instance, samples);

    return 0;
}

void
upse_eventloop_set_audio_callback(upse_module_t *mod, upse_audio_callback_func_t func, const void *user_data)
{
    if (mod->evloop_setcb)
        mod->evloop_setcb(&mod->instance, func, user_data);
}

int
upse_eventloop_seek(upse_module_t *mod, u32 time)
{
    if (mod->evloop_seek)
        return mod->evloop_seek(&mod->instance, time);

    return 0;
}
