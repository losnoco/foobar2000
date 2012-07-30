/*
 * bs1770_ctx.h
 * Copyright (C) 2011, 2012 Peter Belkner <pbelkner@snafu.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#ifndef __BS1770_CTX_H__
#define __BS1770_CTX_H__
#ifdef __cplusplus
extern "C" {
#endif

#define BS1770_MODE_S 1
#define BS1770_MODE_H 2
#define BS1770_MAX_CHANNELS 5

typedef double bs1770_sample_t[BS1770_MAX_CHANNELS];
typedef struct bs1770_ctx bs1770_ctx_t;

#ifdef _MSC_VER
#include <math.h>
static __inline double round(double m) { return floor(m + 0.5); }
#endif

extern const char *bs1770_version;

bs1770_ctx_t *bs1770_ctx_open(int mode, double gate, double ms, int partition,
    double def);
void bs1770_ctx_close(bs1770_ctx_t *ctx);

void bs1770_ctx_add_sample(bs1770_ctx_t *ctx, double fs, int channels,
    bs1770_sample_t sample);
double bs1770_ctx_track_lufs(bs1770_ctx_t *ctx, double fs, int channels);
double bs1770_ctx_track_lra(bs1770_ctx_t *ctx, double lower, double upper,
    double fs, int channels);
double bs1770_ctx_album_lufs(bs1770_ctx_t *ctx);
double bs1770_ctx_album_lufs_multiple(bs1770_ctx_t **ctx, size_t count);
double bs1770_ctx_album_lra(bs1770_ctx_t *ctx, double lower, double upper);
double bs1770_ctx_album_lra_multiple(bs1770_ctx_t **ctx, size_t count, double lower, double upper);

#ifdef __cplusplus
}
#endif
#endif // __BS1770_CTX_H__
