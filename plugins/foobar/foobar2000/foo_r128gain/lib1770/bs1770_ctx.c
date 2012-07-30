/*
 * bs1770_ctx.c
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
#include <string.h>
#include "bs1770.h"

bs1770_ctx_t *bs1770_ctx_open(int mode, double gate, double ms, int partition,
    double reference)
{
  bs1770_ctx_t *ctx=NULL;

  if (NULL==(ctx=malloc(sizeof *ctx)))
    goto error;

  if (NULL==bs1770_ctx_init(ctx,mode,gate,ms,partition,reference))
    goto error;

  return ctx;
error:
  if (NULL!=ctx)
    free(ctx);

  return NULL;
}

void bs1770_ctx_close(bs1770_ctx_t *ctx)
{
  free(bs1770_ctx_cleanup(ctx));
}

bs1770_ctx_t *bs1770_ctx_init(bs1770_ctx_t *ctx, int mode, double gate,
    double ms, int partition, double reference)
{
  bs1770_stats_init_t bs1770_stats_init
      =BS1770_MODE_H==mode?bs1770_stats_h_init:bs1770_stats_s_init;

  memset(ctx, 0, sizeof *ctx);

  if (NULL==bs1770_stats_init(&ctx->track,gate,ms,partition,reference))
    goto error;

  if (NULL==bs1770_stats_init(&ctx->album,gate,ms,partition,reference))
    goto error;

  if (NULL==bs1770_init(&ctx->bs1770, &ctx->track, &ctx->album))
    goto error;

  return ctx;
error:
  bs1770_ctx_cleanup(ctx);
  
  return NULL;
}

bs1770_ctx_t *bs1770_ctx_cleanup(bs1770_ctx_t *ctx)
{
  bs1770_cleanup(&ctx->bs1770);
  bs1770_stats_cleanup(&ctx->album);
  bs1770_stats_cleanup(&ctx->track);

  return ctx;
}

void bs1770_ctx_add_sample(bs1770_ctx_t *ctx, double fs, int channels,
    bs1770_sample_t sample)
{
  bs1770_add_sample(&ctx->bs1770, fs, channels, sample);
}

double bs1770_ctx_track_lufs(bs1770_ctx_t *ctx, double fs, int channels)
{
  double lufs;

  bs1770_flush(&ctx->bs1770, fs, channels);
  lufs=bs1770_stats_get_lufs(&ctx->track);
  bs1770_stats_reset(&ctx->track);

  return lufs;
}

double bs1770_ctx_track_lra(bs1770_ctx_t *ctx, double lower, double upper,
    double fs, int channels)
{
  double lra;

  bs1770_flush(&ctx->bs1770, fs, channels);
  lra=bs1770_stats_get_lra(&ctx->track,lower,upper);
  bs1770_stats_reset(&ctx->track);

  return lra;
}

double bs1770_ctx_album_lufs(bs1770_ctx_t *ctx)
{
  double lufs;

  lufs=bs1770_stats_get_lufs(&ctx->album);
  bs1770_stats_reset(&ctx->album);

  return lufs;
}

double bs1770_ctx_album_lra(bs1770_ctx_t *ctx, double lower, double upper)
{
  double lra;

  lra=bs1770_stats_get_lra(&ctx->album, lower, upper);
  bs1770_stats_reset(&ctx->album);

  return lra;
}
