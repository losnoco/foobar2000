/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-eventloop.h
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

#ifndef __UPSE__LIBUPSE__UPSE_EVENTLOOP_H__GUARD
#define __UPSE__LIBUPSE__UPSE_EVENTLOOP_H__GUARD

#include "upse-module.h"

void upse_eventloop_run(upse_module_t *mod);
void upse_eventloop_stop(upse_module_t *mod);
int upse_eventloop_render(upse_module_t *mod, s16 **samples);
void upse_eventloop_set_audio_callback(upse_module_t *mod, upse_audio_callback_func_t func, const void *user_data);
int upse_eventloop_seek(upse_module_t *mod, u32 time);

#endif
