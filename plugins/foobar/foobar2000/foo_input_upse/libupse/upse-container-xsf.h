/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse-container-xsf.h
 * Purpose: libupse: Unpacks lowlevel xSF file format.
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

/*
 * The following component of UPSE is licensed under the BSD license, due
 * to including third party code. Any modifications to this component are also
 * under the BSD license. The above boilerplate refers to UPSE as a whole work.
 *
 * Copyright (c) 2007-2008, R. Belmont and Richard Bannister.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the names of R. Belmont and Richard Bannister nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UPSE_LIBUPSE_UPSE_CONTAINER_XSF_H__GUARD
#define __UPSE_LIBUPSE_UPSE_CONTAINER_XSF_H__GUARD

#define MAX_UNKNOWN_TAGS			32

typedef struct {
	char lib[256];
	char libaux[8][256];
	
	char inf_title[256];
	char inf_copy[256];
	char inf_artist[256];
	char inf_game[256];
	char inf_year[256];
	char inf_length[256];
	char inf_fade[256];
	char inf_refresh[256];
        char inf_volume[256];
	
	char tag_name[MAX_UNKNOWN_TAGS][256];
	char tag_data[MAX_UNKNOWN_TAGS][256];

	u32 *res_section;
	u32 res_size;
} upse_xsf_t;

upse_xsf_t *upse_xsf_decode(u8 *input, u32 input_len, u8 **output, u64 *size);
long upse_time_to_ms(const char *str);

#endif
