/*
 * bs1770_hist.c
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
 * Based on an idea found at
 * http://kokkinizita.linuxaudio.org/papers/loudness-meter-pres.pdf
 */
#include <math.h>
#include "bs1770.h"

#define BS1770_HIST_MIN    (-70)
#define BS1770_HIST_MAX    (+5)
#define BS1770_HIST_GRAIN  (100)
#define BS1770_HIST_NBINS \
    (BS1770_HIST_GRAIN*(BS1770_HIST_MAX-BS1770_HIST_MIN)+1)

static int bs1770_hist_bin_compare(const void *key, const void *bin)
{
  if (*(const double *)key<((const bs1770_hist_bin_t *)bin)->x)
    return -1;
  else if (0==((const bs1770_hist_bin_t *)bin)->y)
    return 0;
  else if (((const bs1770_hist_bin_t *)bin)->y<=*(const double *)key)
    return 1;
  else
    return 0;
}

void bs1770_hist_reset(bs1770_hist_t *hist)
{
  double step=1.0/BS1770_HIST_GRAIN;
  bs1770_hist_bin_t *wp=hist->bin;
  bs1770_hist_bin_t *mp=wp+BS1770_HIST_NBINS;

  hist->pass1.wmsq=0.0;
  hist->pass1.count=0;

  while (wp<mp) {
    size_t i=wp-hist->bin;
    double db=step*i+BS1770_HIST_MIN;
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

bs1770_hist_t *bs1770_hist_cleanup(bs1770_hist_t *hist)
{
  if (NULL!=hist->bin)
    free(hist->bin);

  return hist;
}

bs1770_hist_t *bs1770_hist_init(bs1770_hist_t *hist, const bs1770_ps_t *ps)
{
  hist->active=0;
  hist->bin=NULL;

  hist->gate=ps->gate;
  hist->reference=ps->reference;

  if (NULL==(hist->bin
      =malloc(BS1770_HIST_NBINS*sizeof(bs1770_hist_bin_t))))
    goto error;

  bs1770_hist_reset(hist);
  hist->active=1;

  return hist;
error:
  bs1770_hist_cleanup(hist);

  return NULL;
}

void bs1770_hist_inc_bin(bs1770_hist_t *hist, double wmsq)
{
  bs1770_hist_bin_t *bin=bsearch(&wmsq,hist->bin,BS1770_HIST_NBINS,
      sizeof hist->bin[0],bs1770_hist_bin_compare);

  if (NULL!=bin) {
    // cumulative moving average.
    hist->pass1.wmsq+=(wmsq-hist->pass1.wmsq)
        /(double)(++hist->pass1.count);
    ++bin->count;
  }
}

void bs1770_hist_add(bs1770_hist_t *album, bs1770_hist_t *track)
{
  bs1770_count_t next_count=album->pass1.count+track->pass1.count;
  bs1770_hist_bin_t *wp=album->bin;
  const bs1770_hist_bin_t *rp=track->bin;
  const bs1770_hist_bin_t *mp=rp+BS1770_HIST_NBINS;

  // cumulative moving average.
  album->pass1.wmsq=(double)album->pass1.count/next_count*album->pass1.wmsq
      +(double)track->pass1.count/next_count*track->pass1.wmsq;
  album->pass1.count=next_count;

  while (rp<mp)
	(wp++)->count+=(rp++)->count;
}

double bs1770_hist_get_lufs(bs1770_hist_t *hist)
{
  double gate=hist->pass1.wmsq*pow(10,0.1*hist->gate);
  const bs1770_hist_bin_t *rp=hist->bin;
  const bs1770_hist_bin_t *mp=rp+BS1770_HIST_NBINS;
  double wmsq=0.0;
  unsigned long long count=0;

  while (rp<mp) {
    if (0ull<rp->count&&gate<rp->x) {
      wmsq+=(double)rp->count*rp->x;
      count+=rp->count;
    }

    ++rp;
  }

  return BS1770_LKFS(count,wmsq,hist->reference);
}

double bs1770_hist_get_lra(bs1770_hist_t *hist, double lower,
   double upper)
{
  double gate=hist->pass1.wmsq*pow(10,0.1*hist->gate);
  struct bs1770_hist_bin *rp=hist->bin;
  struct bs1770_hist_bin *mp=rp+BS1770_HIST_NBINS;
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

    rp=hist->bin;
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
