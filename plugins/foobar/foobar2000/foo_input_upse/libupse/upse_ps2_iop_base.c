/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps2_iop_base.c
 * Purpose: libupse: UPSE PS2 IOP base code
 *
 * Copyright (c) 2007, 2008, 2009, 2010 William Pitcock <nenolod@sacredspiral.co.uk>
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
#include "upse-ps2-iop-private.h"

void upse_ps2_iop_call(u32 callnum)
{
	_DEBUG("IOP call at 0x%lx", pc0);
}
