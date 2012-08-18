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
#include <stdlib.h>
#include <string.h>
#include "bs1770.h"

bs1770_ctx_t *bs1770_ctx_open(size_t size, const bs1770_ps_t *lufs,
    const bs1770_ps_t *lra)
{
  bs1770_ctx_t *ctx;

  if (NULL==(ctx=malloc(sizeof *ctx)))
    return NULL;
  else if (NULL==bs1770_ctx_init(ctx,size,lufs,lra))
    { free(ctx); return NULL; }
  else
    return ctx;
}

void bs1770_ctx_close(bs1770_ctx_t *ctx)
{
  free(bs1770_ctx_cleanup(ctx));
}

bs1770_ctx_t *bs1770_ctx_init(bs1770_ctx_t *ctx, size_t size, 
    const bs1770_ps_t *lufs, const bs1770_ps_t *lra)
{
  memset(ctx,0,sizeof *ctx);

  if (NULL==bs1770_hist_init(&ctx->lufs,lufs))
    goto error;
  else if (NULL!=lra&&NULL==bs1770_hist_init(&ctx->lra,lra))
    goto error;
  else if (1<size) {
    ctx->size=0;

    if (NULL==(ctx->nodes=malloc(size*sizeof *ctx->nodes)))
      goto error;
    
    while (ctx->size<size) {
      bs1770_nd_t *node=ctx->nodes+ctx->size;

      if (NULL==bs1770_nd_init(node,ctx,lufs,lra))
        goto error;

      ++ctx->size;
    }
  }
  else {
    if (NULL==bs1770_nd_init(&ctx->node,ctx,lufs,lra))
      goto error;

    ctx->nodes=&ctx->node;
    ctx->size=1;
  }

  return ctx;
error:
  bs1770_ctx_cleanup(ctx);

  return NULL;
}

bs1770_ctx_t *bs1770_ctx_cleanup(bs1770_ctx_t *ctx)
{
  if (NULL!=ctx->nodes) {
    bs1770_nd_t *mp=ctx->nodes;
    bs1770_nd_t *rp=mp+ctx->size;

    while (mp<rp)
      bs1770_nd_cleanup(--rp);

    if (1!=ctx->size)
      free(ctx->nodes);
  }

  bs1770_hist_cleanup(&ctx->lra);
  bs1770_hist_cleanup(&ctx->lufs);

  return ctx;
}

void bs1770_ctx_add_sample(bs1770_ctx_t *ctx, size_t i, double fs,
    int channels, bs1770_sample_t sample)
{
  bs1770_add_sample(&ctx->nodes[i].bs1770,fs,channels,sample);
}

double bs1770_ctx_track_lufs(bs1770_ctx_t *ctx, size_t i)
{
  return bs1770_nd_track_lufs(ctx->nodes+i);
}

double bs1770_ctx_track_lra(bs1770_ctx_t *ctx, size_t i, double lower,
    double upper)
{
  return bs1770_nd_track_lra(ctx->nodes+i,lower,upper);
}

double bs1770_ctx_album_lufs(bs1770_ctx_t *ctx)
{
  double lufs;

  lufs=bs1770_hist_get_lufs(&ctx->lufs);
  bs1770_hist_reset(&ctx->lufs);

  return lufs;
}

double bs1770_ctx_album_lra(bs1770_ctx_t *ctx, double lower, double upper)
{
  double lra=0.0;

  if (ctx->lra.active) {
    lra=bs1770_hist_get_lra(&ctx->lra,lower,upper);
    bs1770_hist_reset(&ctx->lra);
  }

  return lra;
}
