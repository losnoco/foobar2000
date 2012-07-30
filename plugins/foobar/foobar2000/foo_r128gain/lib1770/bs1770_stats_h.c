/*
 * bs1770_stats_h.c
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
 * idea taken from "http://lac.linuxaudio.org/2011/download/lm-pres.pdf".
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

#define BS1770_STATS_H_MIN    (-70)
#define BS1770_STATS_H_MAX    (+5)
#define BS1770_STATS_H_GRAIN  (100)
#define BS1770_STATS_H_NBINS \
    (BS1770_STATS_H_GRAIN*(BS1770_STATS_H_MAX-BS1770_STATS_H_MIN)+1)

static bs1770_stats_vmt_t bs1770_stats_h_vmt_t;

bs1770_stats_t *bs1770_stats_h_init(bs1770_stats_t *stats, double gate,
    double ms, int partition, double reference)
{
  stats->vmt=&bs1770_stats_h_vmt_t;
  stats->h.bin=NULL;

  if (NULL==bs1770_stats_alloc(stats,partition,gate,ms,partition,reference))
    goto error;

  if (NULL==(stats->h.bin
      =malloc(BS1770_STATS_H_NBINS*sizeof(bs1770_stats_h_bin_t))))
    goto error;

  bs1770_stats_reset(stats);

  return stats;
error:
  bs1770_stats_cleanup(stats);

  return NULL;
}

static bs1770_stats_t *bs1770_stats_h_cleanup(bs1770_stats_t *stats)
{
  if (NULL!=stats->h.bin)
    free(stats->h.bin);

  bs1770_stats_free(stats);

  return stats;
}

static void bs1770_stats_h_reset(bs1770_stats_t *stats)
{
  double step=1.0/BS1770_STATS_H_GRAIN;
  struct bs1770_stats_h_bin *wp=stats->h.bin;
  struct bs1770_stats_h_bin *mp=wp+BS1770_STATS_H_NBINS;

  bs1770_stats_clear(stats);
  stats->h.filled=1;

  while (wp<mp) {
    size_t i=wp-stats->h.bin;
    double db=step*i+BS1770_STATS_H_MIN;
    double wsmq=pow(10.0,0.1*(0.691+db));

    wp->db=db;
    wp->x=wsmq;
    wp->y=0.0;
    wp->count=0;

    if (0<i)
      wp[-1].y=wsmq;

    ++wp;
  }
}

static void bs1770_stats_h_set_fs(bs1770_stats_t *stats, double fs)
{
  bs1770_stats_set_fs(stats, fs);

  // reset ring buffer.
  stats->h.filled=0;
  stats->blocks.offs=0;
  stats->blocks.wmsq[stats->blocks.offs]=0.0;
}

static int bin_compare(const void *key, const void *bin)
{
  if (*(const double *)key<((const struct bs1770_stats_h_bin *)bin)->x)
    return -1;
  else if (0==((const struct bs1770_stats_h_bin *)bin)->y)
    return 0;
  else if (((const struct bs1770_stats_h_bin *)bin)->y<=*(const double *)key)
    return 1;
  else
    return 0;
}

static void bs1770_stats_h_add_sqs(bs1770_stats_t *stats, double fs,
    double wssqs)
{
  double *wmsq=stats->blocks.wmsq;

  if (stats->fs!=fs)
    bs1770_stats_h_set_fs(stats, fs);

  if (1.0e-15<=wssqs) {
    double *wp=wmsq;
    double *mp=wp+stats->h.filled;

    wssqs*=stats->scale;

    while (wp<mp)
      (*wp++)+=wssqs;
  }

  if (++stats->blocks.count==stats->overlap_size) {
    int next_offs=stats->blocks.offs+1;

    if (next_offs==stats->blocks.size)
      next_offs=0;

    if (stats->h.filled==stats->blocks.size) {
      double prev_wmsq=wmsq[next_offs];

      if (stats->pass1.gate<prev_wmsq) {
        struct bs1770_stats_h_bin *bin;

        // cumulative moving average.
        stats->pass1.wmsq+=(prev_wmsq-stats->pass1.wmsq)
            /(double)(++stats->pass1.count);

        bin=bsearch(&prev_wmsq,stats->h.bin,BS1770_STATS_H_NBINS,
            sizeof stats->h.bin[0],bin_compare);

        if (NULL!=bin)
          ++bin->count;
      }
    }

    stats->blocks.wmsq[next_offs]=0.0;
    stats->blocks.count=0;
    stats->blocks.offs=next_offs;

    if (stats->h.filled<stats->blocks.size)
      ++stats->h.filled;
  }
}

static double bs1770_stats_h_get_lufs(bs1770_stats_t *stats)
{
  double gate=stats->pass1.wmsq*pow(10,0.1*stats->gate);
  struct bs1770_stats_h_bin *rp=stats->h.bin;
  struct bs1770_stats_h_bin *mp=rp+BS1770_STATS_H_NBINS;
  double wmsq=0.0;
  unsigned long long count=0;

  while (rp<mp) {
    if (0ull<rp->count&&gate<rp->x) {
      wmsq+=(double)rp->count*rp->x;
      count+=rp->count;
    }

    ++rp;
  }

  return BS1770_LKFS(count,wmsq,stats->reference);
}

static double bs1770_stats_h_get_lufs_multiple(bs1770_stats_t *stats, size_t stats_count)
{
  double wmsq=0.0;
  double gate=0.0;
  unsigned long long count=0;
  size_t i;
  for (i = 0; i < stats_count; i++)
    gate+=stats[i].pass1.wmsq*pow(10,0.1*stats[i].gate);
  gate/=(double)stats_count;
  for (i = 0; i < stats_count; i++)
  {
    struct bs1770_stats_h_bin *rp=stats[i].h.bin;
    struct bs1770_stats_h_bin *mp=rp+BS1770_STATS_H_NBINS;

    while (rp<mp) {
      if (0ull<rp->count&&gate<rp->x) {
        wmsq+=(double)rp->count*rp->x;
        count+=rp->count;
      }

      ++rp;
    }
  }

  return BS1770_LKFS(count,wmsq,stats[0].reference);
}


static double bs1770_stats_h_get_lra(bs1770_stats_t *stats, double lower,
   double upper)
{
  double gate=stats->pass1.wmsq*pow(10,0.1*stats->gate);
  struct bs1770_stats_h_bin *rp=stats->h.bin;
  struct bs1770_stats_h_bin *mp=rp+BS1770_STATS_H_NBINS;
  unsigned long long count=0ull;

  while (rp<mp) {
    if (0ull<rp->count&&gate<rp->x)
      count+=rp->count;

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

  if (0ull<count) {
    unsigned long long lower_count=count*lower;
    unsigned long long upper_count=count*upper;
    unsigned long long prev_count=-1;
    double min=0.0;
    double max=0.0;

    rp=stats->h.bin;
    count=0ull;

    while (rp<mp) {
      if (gate<rp->x) {
        count+=rp->count;

        if (prev_count<lower_count&&lower_count<=count)
          min=rp->db;

        if (prev_count<upper_count&&upper_count<=count) {
          max=rp->db;
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
}

static double bs1770_stats_h_get_lra_multiple(bs1770_stats_t *stats, size_t stats_count,
   double lower, double upper)
{
  double gate=0.0;
  double total_min=0.0;
  double total_max=0.0;
  size_t i;

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
    struct bs1770_stats_h_bin *rp=stats[i].h.bin;
    struct bs1770_stats_h_bin *mp=rp+BS1770_STATS_H_NBINS;
    unsigned long long count=0ull;

    while (rp<mp) {
      if (0ull<rp->count&&gate<rp->x)
        count+=rp->count;

      ++rp;
    }

    if (0ull<count) {
      unsigned long long lower_count=count*lower;
      unsigned long long upper_count=count*upper;
      unsigned long long prev_count=-1;
      double min=0.0;
      double max=0.0;

      rp=stats[i].h.bin;
      count=0ull;

      while (rp<mp) {
        if (gate<rp->x) {
          count+=rp->count;

          if (prev_count<lower_count&&lower_count<=count)
          min=rp->db;

          if (prev_count<upper_count&&upper_count<=count) {
            max=rp->db;
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
}

static bs1770_stats_vmt_t bs1770_stats_h_vmt_t={
#ifdef _MSC_VER
  bs1770_stats_h_cleanup,
  bs1770_stats_h_reset,
  bs1770_stats_h_add_sqs,
  bs1770_stats_h_get_lufs,
  bs1770_stats_h_get_lufs_multiple,
  bs1770_stats_h_get_lra,
  bs1770_stats_h_get_lra_multiple
#else
  .cleanup=bs1770_stats_h_cleanup,
  .reset=bs1770_stats_h_reset,
  .add_sqs=bs1770_stats_h_add_sqs,
  .get_lufs=bs1770_stats_h_get_lufs,
  .get_lufs_multiple=bs1770_stats_h_get_lufs_multiple,
  .get_lra=bs1770_stats_h_get_lra,
  .get_lra_multiple=bs1770_stats_h_get_lra_multiple
#endif
};
