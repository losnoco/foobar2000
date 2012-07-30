/*
 * bs1770_ctx_m.c
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

bs1770_ctx_m_t *bs1770_ctx_m_open(size_t count, int mode, double gate,
    double ms, int partition, double reference)
{
  bs1770_ctx_m_t *ctx=NULL;

  if (NULL==(ctx=malloc(sizeof *ctx)))
    goto error;

  if (NULL==bs1770_ctx_m_init(ctx,count,mode,gate,ms,partition,reference))
    goto error;

  return ctx;
error:
  if (NULL!=ctx)
    free(ctx);

  return NULL;
}

void bs1770_ctx_m_close(bs1770_ctx_m_t *ctx)
{
  free(bs1770_ctx_m_cleanup(ctx));
}

bs1770_ctx_m_t *bs1770_ctx_m_init(bs1770_ctx_m_t *ctx, size_t count, int mode,
    double gate, double ms, int partition, double reference)
{
  size_t i;
  bs1770_stats_init_t bs1770_stats_init
      =BS1770_MODE_H==mode?bs1770_stats_h_init:bs1770_stats_s_init;

  memset(ctx, 0, sizeof *ctx);

  ctx->count = count;

  if (NULL==(ctx->track = calloc(count, sizeof(*ctx->track))))
    goto error;

  if (NULL==(ctx->album = calloc(count, sizeof(*ctx->album))))
    goto error;

  if (NULL==(ctx->bs1770 = calloc(count, sizeof(*ctx->bs1770))))
    goto error;

  for (i = 0; i < count; i++)
  {
    if (NULL==bs1770_stats_init(&ctx->track[i],gate,ms,partition,reference))
      goto error;

    if (NULL==bs1770_stats_init(&ctx->album[i],gate,ms,partition,reference))
      goto error;

    if (NULL==bs1770_init(&ctx->bs1770[i], &ctx->track[i], &ctx->album[i]))
      goto error;
  }

  return ctx;
error:
  bs1770_ctx_m_cleanup(ctx);
  
  return NULL;
}

bs1770_ctx_m_t *bs1770_ctx_m_cleanup(bs1770_ctx_m_t *ctx)
{
  size_t i;

  if (ctx->bs1770)
  {
    for (i = 0; i < ctx->count; i++)
      bs1770_cleanup(&ctx->bs1770[i]);
    free(ctx->bs1770);
    ctx->bs1770 = NULL;
  }

  if (ctx->album)
  {
    for (i = 0; i < ctx->count; i++)
      bs1770_stats_cleanup(&ctx->album[i]);
    free(ctx->album);
    ctx->album = NULL;
  }

  if (ctx->track)
  {
    for (i = 0; i < ctx->count; i++)
      bs1770_stats_cleanup(&ctx->track[i]);
    free(ctx->track);
    ctx->track = NULL;
  }

  return ctx;
}

void bs1770_ctx_m_add_sample(bs1770_ctx_m_t *ctx, size_t index, double fs,
    int channels, bs1770_sample_t sample)
{
  bs1770_add_sample(&ctx->bs1770[index], fs, channels, sample);
}

double bs1770_ctx_m_track_lufs(bs1770_ctx_m_t *ctx, size_t index, double fs,
    int channels)
{
  double lufs;

  bs1770_flush(&ctx->bs1770[index], fs, channels);
  lufs=bs1770_stats_get_lufs(&ctx->track[index]);
  bs1770_stats_reset(&ctx->track[index]);

  return lufs;
}

double bs1770_ctx_m_track_lra(bs1770_ctx_m_t *ctx, size_t index, double lower,
    double upper, double fs, int channels)
{
  double lra;

  bs1770_flush(&ctx->bs1770[index], fs, channels);
  lra=bs1770_stats_get_lra(&ctx->track[index],lower,upper);
  bs1770_stats_reset(&ctx->track[index]);

  return lra;
}

double bs1770_ctx_m_album_lufs(bs1770_ctx_m_t *ctx)
{
  double lufs;
  size_t i;

  lufs=bs1770_stats_get_lufs_multiple(ctx->album, ctx->count);

  for (i = 0; i < ctx->count; i++)
    bs1770_stats_reset(&ctx->album[i]);

  return lufs;
}

double bs1770_ctx_m_album_lra(bs1770_ctx_m_t *ctx, double lower, double upper)
{
  double lra;
  size_t i;

  lra=bs1770_stats_get_lra_multiple(ctx->album, ctx->count, lower, upper);

  for (i = 0; i < ctx->count; i++)
    bs1770_stats_reset(&ctx->album[i]);

  return lra;
}
