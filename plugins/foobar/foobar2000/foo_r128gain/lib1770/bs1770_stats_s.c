/*
 * bs1770_stats_s.c
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
 * Many thanks to the folks at HA forum (http://www.hydrogenaudio.org/)
 * for valuable discussion, especially C.R.Helmrich, googlebot, and jdoering
 * (cf. http://www.hydrogenaudio.org/forums/index.php?showtopic=85978)
 */
#ifdef NDEBUG /* N.B. assert used with active statements so enable always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#ifdef _MSC_VER
#include <string.h>
#else
#include <strings.h>
#endif
#include <assert.h>
#include <math.h>
#include "bs1770.h"

static bs1770_stats_vmt_t bs1770_stats_s_vmt_t;

bs1770_stats_t *bs1770_stats_s_init(bs1770_stats_t *stats, double gate,
    double ms, int partition, double reference)
{
  stats->vmt=&bs1770_stats_s_vmt_t;

  if (NULL==bs1770_stats_alloc(stats, 4096, gate, ms, partition, reference))
    goto error;

  bs1770_stats_reset(stats);

  return stats;
error:
  bs1770_stats_cleanup(stats);

  return NULL;
}

static void bs1770_stats_s_set_fs(bs1770_stats_t *stats, double fs)
{
  int partition=stats->partition;
  int i;

  bs1770_stats_set_fs(stats, fs);

  for (i=0;;) {
    stats->blocks.wmsq[stats->blocks.offs]=0.0;

    if (++i<partition&&0<stats->blocks.offs)
      --stats->blocks.offs;
    else
      break;
  }
}

static void bs1770_stats_s_add_sqs(bs1770_stats_t *stats, double fs,
    double wssqs)
{
  int x=stats->partition-1;
  double *wmsq;

  if (stats->fs!=fs)
    bs1770_stats_s_set_fs(stats, fs);

  wmsq=stats->blocks.wmsq+stats->blocks.offs;

  if (1.0e-15<=wssqs) {
    double *wp=wmsq-(stats->blocks.offs<x?stats->blocks.offs:x);

    wssqs*=stats->scale;

    while (wp<=wmsq)
      (*wp++)+=wssqs;
  }

  if (++stats->blocks.count==stats->overlap_size) {
    if (x<=stats->blocks.offs) {
      double *prev_wmsq=wmsq-x;

      if (stats->pass1.gate<*prev_wmsq) {
        // cumulative moving average.
        stats->pass1.wmsq+=(*prev_wmsq-stats->pass1.wmsq)
            /(double)(++stats->pass1.count);
      }
      else {
        // remove the silent block.
        memmove(prev_wmsq,prev_wmsq+1,x*sizeof(*wmsq));
        --stats->blocks.offs;
      }
    }

    if (++stats->blocks.offs==stats->blocks.size)
      assert(NULL!=bs1770_stats_realloc(stats));

    stats->blocks.wmsq[stats->blocks.offs]=0.0;
    stats->blocks.count=0;
  }
}

static double bs1770_stats_s_get_lufs(bs1770_stats_t *stats)
{
  double gate=stats->pass1.wmsq*pow(10,0.1*stats->gate);
  double *rp=stats->blocks.wmsq;
  double *mp=rp+(stats->blocks.offs-(stats->partition-1));
  double wmsq=0.0;
  size_t count=0;

  while (rp<mp) {
    if (gate<*rp) {
      wmsq+=*rp;
      ++count;
    }

    ++rp;
  }

  return BS1770_LKFS(count,wmsq,stats->reference);
}

static double bs1770_stats_s_get_lufs_multiple(bs1770_stats_t *stats, size_t stats_count)
{
  double wmsq=0.0;
  double gate=0.0;
  size_t count=0;
  size_t i;
  for (i = 0; i < stats_count; i++)
    gate+=stats[i].pass1.wmsq*pow(10,0.1*stats[i].gate);
  gate/=(double)stats_count;
  for (i = 0; i < stats_count; i++)
  {
    double *rp=stats[i].blocks.wmsq;
    double *mp=rp+(stats[i].blocks.offs-(stats[i].partition-1));

    while (rp<mp) {
      if (gate<*rp) {
        wmsq+=*rp;
        ++count;
      }

      ++rp;
    }
  }

  return BS1770_LKFS(count,wmsq,stats[0].reference);
}

static double bs1770_stats_s_get_lra(bs1770_stats_t *stats, double lower,
    double upper)
{
#ifdef SORTED_SEQUENCE
  double gate=stats->pass1.wmsq*pow(10,0.1*stats->gate);
  double *rp=stats->blocks.wmsq;
  double *mp=rp+(stats->blocks.offs-(stats->partition-1));
  double wmsq=0.0;
  size_t count=0;

  while (rp<mp) {
    if (gate<*rp)
      ++count;

    ++rp;
  }

  if (lower>upper) {
    double tmp=lower;

    lower=upper;
    upper=tmp;
  }

  if (lower<0.0)
    lower=0.0;

  if (1.0<upper)
    upper=1.0;

  if (0<count) {
    size_t lower_count=count*lower;
    size_t upper_count=count*upper;
    size_t prev_count=-1;
    double min=0.0;
    double max=0.0;

    rp=stats->blocks.wmsq;
    count=0;

    while (rp<mp) {
      if (gate<*rp) {
        ++count;

        if (prev_count<lower_count&&lower_count<=count)
          min=10.0*log10(*rp)/*-0.691*/;

        if (prev_count<upper_count&&upper_count<=count) {
          max=10.0*log10(*rp)/*-0.691*/;
          break;
        }

        prev_count=count;
      }

      ++rp;
    }

    return max-min;
  }
  else
    return 0.0;
#else // ! SORTED_SEQUENCE
  return 0.0;
#endif // ! SORTED_SEQUENCE
}

static double bs1770_stats_s_get_lra_multiple(bs1770_stats_t *stats, size_t stats_count,
    double lower, double upper)
{
#ifdef SORTED_SEQUENCE
  size_t i;
  double gate=0.0;
  double total_min=0.0;
  double total_max=0.0;

  if (lower>upper) {
    double tmp=lower;

    lower=upper;
    upper=tmp;
  }

  if (lower<0.0)
    lower=0.0;

  if (1.0<upper)
    upper=1.0;

  for (i = 0; i < stats_count; i++)
    gate+=stats[i].pass1.wmsq*pow(10,0.1*stats[i].gate);
  gate/=(double)stats_count;

  for (i = 0; i < stats_count; i++)
  {
    double *rp=stats[i].blocks.wmsq;
    double *mp=rp+(stats[i].blocks.offs-(stats[i].partition-1));
    double wmsq=0.0;
    size_t count=0;

    while (rp<mp) {
      if (gate<*rp)
        ++count;

      ++rp;
    }

    if (0<count) {
      size_t lower_count=count*lower;
      size_t upper_count=count*upper;
      size_t prev_count=-1;
      double min=0.0;
      double max=0.0;

      rp=stats[i].blocks.wmsq;
      count=0;

      while (rp<mp) {
        if (gate<*rp) {
          ++count;

          if (prev_count<lower_count&&lower_count<=count)
            min=10.0*log10(*rp)/*-0.691*/;

          if (prev_count<upper_count&&upper_count<=count) {
            max=10.0*log10(*rp)/*-0.691*/;
            break;
          }

          prev_count=count;
        }

        ++rp;
      }

      if (i == 0)
      {
        total_min = min;
        total_max = max;
      }
      else
      {
        if (total_min > min) total_min = min;
        if (total_max < max) total_max = max;
      }
    }
  }

  return total_max-total_min;
#else // ! SORTED_SEQUENCE
  return 0.0;
#endif // ! SORTED_SEQUENCE
}

static bs1770_stats_vmt_t bs1770_stats_s_vmt_t={
#ifdef _MSC_VER
  bs1770_stats_free,
  bs1770_stats_clear,
  bs1770_stats_s_add_sqs,
  bs1770_stats_s_get_lufs,
  bs1770_stats_s_get_lufs_multiple,
  bs1770_stats_s_get_lra,
  bs1770_stats_s_get_lra_multiple
#else
  .cleanup=bs1770_stats_free,
  .reset=bs1770_stats_clear,
  .add_sqs=bs1770_stats_s_add_sqs,
  .get_lufs=bs1770_stats_s_get_lufs,
  .get_lufs_multiple=bs1770_stats_s_get_lufs_multiple,
  .get_lra=bs1770_stats_s_get_lra,
  .get_lra_multiple=bs1770_stats_s_get_lra_multiple
#endif
};
