/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-types.h
 * Purpose: libupse: type aliases
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

#ifndef _UPSE__LIBUPSE__UPSE_TYPES_H__GUARD
#define _UPSE__LIBUPSE__UPSE_TYPES_H__GUARD

#ifndef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef WIN32_MSC

#define INLINE inline

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t sptr;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;

#else

#pragma warning (disable: 4996)
#pragma warning (disable: 4224)
#pragma warning (disable: 4113)

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed __int64 s64;
typedef unsigned __int64 u64;

#define INLINE

#define strcasecmp stricmp
#define strncasecmp strnicmp

#endif

#ifdef __GNUC__
# define upse_packed_t	__attribute__ ((packed))
#else
# define upse_packed_t
#endif

#endif
