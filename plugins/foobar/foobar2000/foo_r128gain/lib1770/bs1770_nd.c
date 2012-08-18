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

bs1770_nd_t *bs1770_nd_init(bs1770_nd_t *node, bs1770_ctx_t *ctx,
	const bs1770_ps_t *lufs, const bs1770_ps_t *lra)
{
  memset(node,0,sizeof *node);

  if (NULL==bs1770_stats_init(&node->lufs,&ctx->lufs,lufs))
	goto error;
  else if (NULL!=lra&&NULL==bs1770_stats_init(&node->lra,&ctx->lra,lra))
	goto error;
  else if (NULL==bs1770_init(&node->bs1770,&node->lufs.aggr,
      NULL!=lra?&node->lra.aggr:NULL))
	goto error;

  return node;
error:
  bs1770_nd_cleanup(node);

  return NULL;
}

bs1770_nd_t *bs1770_nd_cleanup(bs1770_nd_t *node)
{
  bs1770_cleanup(&node->bs1770);
  bs1770_stats_cleanup(&node->lra);
  bs1770_stats_cleanup(&node->lufs);

  return node;
}

void bs1770_nd_add_sample(bs1770_nd_t *node, double fs, int channels,
    bs1770_sample_t sample)
{
  bs1770_add_sample(&node->bs1770,fs,channels,sample);
}

double bs1770_nd_track_lufs(bs1770_nd_t *node)
{
  return bs1770_track_lufs(&node->bs1770);
}

double bs1770_nd_track_lra(bs1770_nd_t *node, double lower, double upper)
{
  return bs1770_track_lra(&node->bs1770,lower,upper);
}
