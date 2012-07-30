/*
 * bs1770_stats.c
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
#ifdef _MSC_VER
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include "bs1770.h"

#define BLOCK_SIZE(size) \
  ((size)*sizeof(((bs1770_stats_t *)NULL)->blocks.wmsq[0]))

bs1770_stats_t *bs1770_stats_alloc(bs1770_stats_t *stats, int size,
    double gate, double ms, int partition, double reference)
{
  stats->gate=gate;
  stats->length=0.001*ms;
  stats->partition=partition;
  stats->reference=reference;
  stats->pass1.gate=pow(10.0,0.1*(0.691-70.0));

  stats->blocks.size=size;
  stats->blocks.wmsq=NULL;
  
  if (NULL==(stats->blocks.wmsq=malloc(BLOCK_SIZE(stats->blocks.size))))
    goto error;

  return stats;
error:
  return NULL;
}

bs1770_stats_t *bs1770_stats_realloc(bs1770_stats_t *stats)
{
  size_t size=(stats->blocks.size*=2);

  if (NULL==(stats->blocks.wmsq=realloc(stats->blocks.wmsq,BLOCK_SIZE(size))))
    goto error;

  return stats;
error:
  return NULL;
}

bs1770_stats_t *bs1770_stats_free(bs1770_stats_t *stats)
{
  if (NULL!=stats->blocks.wmsq)
    free(stats->blocks.wmsq);

  return stats;
}

void bs1770_stats_clear(bs1770_stats_t *stats)
{
  stats->scale=0.0;
  stats->fs=0.0;
  stats->overlap_size=0;
  stats->block_size=0;

  stats->pass1.wmsq=0.0;
  stats->pass1.count=0;

  stats->blocks.offs=0;
  stats->blocks.count=0;
  stats->blocks.wmsq[0]=0.0;
}

void bs1770_stats_set_fs(bs1770_stats_t *stats, double fs)
{
  int partition=stats->partition;

  stats->fs=fs;
  stats->overlap_size=round(stats->length*fs/partition);  // 400/partition ms.
  stats->block_size=partition*stats->overlap_size;        // 400 ms.
  stats->scale=1.0/(double)stats->block_size;
  stats->blocks.count=0;
}
