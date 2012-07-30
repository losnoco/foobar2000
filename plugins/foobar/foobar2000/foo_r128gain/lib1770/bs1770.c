/*
 * bs1770.c
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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "bs1770.h"

#define IS_DEN(x) \
    (fabs(den_tmp=(x))<1.0e-15)
#define DEN(x) \
    (IS_DEN(x)?0.0:den_tmp)

#define GET(buf,offs,i) \
    ((buf)[(get_tmp=(offs)+(i))<0?BS1770_BUF_SIZE+get_tmp:get_tmp])

#define GETX(buf,offs,i)  GET(buf,offs,i)
#define GETY(buf,offs,i)  GET(buf,(offs)-6,i)
#define GETZ(buf,offs,i)  GET(buf,(offs)-3,i)

#define MIN(x,y) \
  ((x)<(y)?x:y)

double BS1770_G[BS1770_MAX_CHANNELS]={
  1.0,
  1.0,
  1.0,
  1.41,
  1.41
};

static biquad_t pre48000={
#ifdef _MSC_VER
  48000,
  -1.69065929318241,
  0.73248077421585,
  1.53512485958697,
  -2.69169618940638,
  1.19839281085285
#else
  .fs=48000,
  .a1=-1.69065929318241,
  .a2=0.73248077421585,
  .b0=1.53512485958697,
  .b1=-2.69169618940638,
  .b2=1.19839281085285
#endif
};

static biquad_t rlb48000={
#ifdef _MSC_VER
  48000,
  -1.99004745483398,
  0.99007225036621,
  1.0,
  -2.0,
  1.0
#else
  .fs=48000,
  .a1=-1.99004745483398,
  .a2=0.99007225036621,
  .b0=1.0,
  .b1=-2.0,
  .b2=1.0
#endif
};

void bs1770_close(bs1770_t *bs1770)
{
  free(bs1770_cleanup(bs1770));
}

bs1770_t *bs1770_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album)
{
  memset(bs1770, 0, sizeof *bs1770);
  bs1770->fs=0.0;
  bs1770->pre.fs=0.0;
  bs1770->rlb.fs=0.0;

  bs1770->track=track;
  bs1770->album=album;

  return bs1770;
error:
  return NULL;
}

bs1770_t *bs1770_s_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album, double gate, double ms, int partition,
 double reference)
{
  if (NULL==bs1770_stats_s_init(track,gate,ms,partition,reference))
    return NULL;
  else if (NULL==bs1770_stats_s_init(album,gate,ms,partition,reference))
    return NULL;

  return bs1770_init(bs1770,track,album);
}

bs1770_t *bs1770_h_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album, double gate, double ms, int partition,
 double reference)
{
  if (NULL==bs1770_stats_h_init(track,gate,ms,partition,reference))
    return NULL;
  else if (NULL==bs1770_stats_h_init(album,gate,ms,partition,reference))
    return NULL;

  return bs1770_init(bs1770,track,album);
}

bs1770_t *bs1770_cleanup(bs1770_t *bs1770)
{
  return bs1770;
}

void bs1770_set_fs(bs1770_t *bs1770, double fs, int channels)
{
  int i;

  bs1770->fs=fs;

  bs1770->pre.fs=fs;
  biquad_requantize(&pre48000, &bs1770->pre);

  bs1770->rlb.fs=fs;
  biquad_requantize(&rlb48000, &bs1770->rlb);

  for (i=0;i<MIN(channels,BS1770_MAX_CHANNELS);++i) {
    double *buf=bs1770->ring.buf[i];
    int get_tmp;

    GETX(buf,0,0)=0.0;
  }

  bs1770->ring.size=bs1770->ring.offs=1;
}

void bs1770_add_sample(bs1770_t *bs1770, double fs, int channels,
    bs1770_sample_t sample)
{
  biquad_t *pre=&bs1770->pre;
  biquad_t *rlb=&bs1770->rlb;
  double wssqs=0.0;
  double *g=BS1770_G;
  int offs, size, i;
  int get_tmp;
  double den_tmp;

  if (bs1770->fs!=fs)
    bs1770_set_fs(bs1770, fs, channels);

  offs=bs1770->ring.offs;
  size=bs1770->ring.size;

  for (i=0;i<MIN(channels,BS1770_MAX_CHANNELS);++i) {
    double *buf=bs1770->ring.buf[i];
    double x=GETX(buf,offs,0)=DEN(sample[i]);

    if (1<size) {
      double y=GETY(buf,offs,0)=DEN(pre->b0*x
        +pre->b1*GETX(buf,offs,-1)+pre->b2*GETX(buf,offs,-2)
        -pre->a1*GETY(buf,offs,-1)-pre->a2*GETY(buf,offs,-2))
        ;
      double z=GETZ(buf,offs,0)=DEN(rlb->b0*y
        +rlb->b1*GETY(buf,offs,-1)+rlb->b2*GETY(buf,offs,-2)
        -rlb->a1*GETZ(buf,offs,-1)-rlb->a2*GETZ(buf,offs,-2))
        ;
      wssqs+=(*g++)*z*z;
      ++buf;
    }
  }

  bs1770_stats_add_sqs(bs1770->track, fs, wssqs);
  bs1770_stats_add_sqs(bs1770->album, fs, wssqs);

  if (size<2)
    ++bs1770->ring.size;

  if (++bs1770->ring.offs==BS1770_BUF_SIZE)
    bs1770->ring.offs=0;
}

void bs1770_flush(bs1770_t *bs1770, double fs, int channels)
{
  channels=MIN(channels,BS1770_MAX_CHANNELS);

  if (1<bs1770->ring.size) {
    bs1770_sample_t sample;
    int i;

    for (i=0;i<channels;++i)
      sample[i]=0.0;

    bs1770_add_sample(bs1770, fs, channels, sample);
  }

  bs1770->ring.size=bs1770->ring.offs=0;
  bs1770->fs=0.0;
}
