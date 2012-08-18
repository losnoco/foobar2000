/*
 * bs1770_aggr.c
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
/*
 * Partly based on discussions at HA forum (http://www.hydrogenaudio.org/)
 * especially with C.R.Helmrich, googlebot, and jdoering
 * (cf. http://www.hydrogenaudio.org/forums/index.php?showtopic=85978)
 */
#include <math.h>
#include "bs1770.h"

#if defined (_MSC_VER)
#  define round(x) floor((x)+0.5)
#endif

#define SILENCE \
  (-70.0)
#define SILENCE_GATE \
  pow(10.0,0.1*(0.691+SILENCE))
#define BLOCK_SIZE(size) \
  ((size)*sizeof(((bs1770_aggr_t *)NULL)->blocks.wmsq[0]))

void bs1770_aggr_reset(bs1770_aggr_t *aggr)
{
  aggr->fs=0.0;
  aggr->overlap_size=0;
  aggr->block_size=0;
  aggr->scale=0.0;

  aggr->blocks.wmsq[aggr->blocks.offs=0]=0.0;
  aggr->blocks.count=0;
  aggr->blocks.used=1;
}

static void bs1770_aggr_set_fs(bs1770_aggr_t *aggr, double fs)
{
  int partition=aggr->partition;

  aggr->fs=fs;
  aggr->overlap_size=round(aggr->length*fs/partition);  // 400/partition ms.
  aggr->block_size=partition*aggr->overlap_size;        // 400 ms.
  aggr->scale=1.0/(double)aggr->block_size;

  aggr->blocks.wmsq[aggr->blocks.offs=0]=0.0;
  aggr->blocks.count=0;
  aggr->blocks.used=1;
}

void bs1770_aggr_add_sqs(bs1770_aggr_t *aggr, double fs, double wssqs)
{
  double *wmsq=aggr->blocks.wmsq;

  if (aggr->fs!=fs)
    bs1770_aggr_set_fs(aggr, fs);

  if (1.0e-15<=wssqs) {
    double *wp=wmsq;
    double *mp=wp+aggr->blocks.used;

    wssqs*=aggr->scale;

    while (wp<mp)
      (*wp++)+=wssqs;
  }

  if (++aggr->blocks.count==aggr->overlap_size) {
    int next_offs=aggr->blocks.offs+1;

    if (next_offs==aggr->blocks.size)
      next_offs=0;

    if (aggr->blocks.used==aggr->blocks.size) {
      double prev_wmsq=wmsq[next_offs];

      if (aggr->gate<prev_wmsq)
        bs1770_hist_inc_bin(aggr->track,prev_wmsq);
    }

    aggr->blocks.wmsq[next_offs]=0.0;
    aggr->blocks.count=0;
    aggr->blocks.offs=next_offs;

    if (aggr->blocks.used<aggr->blocks.size)
      ++aggr->blocks.used;
  }
}

bs1770_aggr_t *bs1770_aggr_cleanup(bs1770_aggr_t *aggr)
{
  if (NULL!=aggr->blocks.wmsq)
    free(aggr->blocks.wmsq);

  return aggr;
}

bs1770_aggr_t *bs1770_aggr_init(bs1770_aggr_t *aggr, const bs1770_ps_t *ps,
    bs1770_hist_t *track, bs1770_hist_t *album)
{

  aggr->gate=SILENCE_GATE;
  aggr->length=0.001*ps->ms;
  aggr->partition=ps->partition;

  aggr->blocks.size=ps->partition;
  aggr->blocks.wmsq=NULL;
  
  if (NULL==(aggr->blocks.wmsq=malloc(BLOCK_SIZE(aggr->blocks.size))))
    goto error;

  bs1770_aggr_reset(aggr);
  aggr->track=track;
  aggr->album=album;

  return aggr;
error:
  bs1770_aggr_cleanup(aggr);

  return NULL;
}
