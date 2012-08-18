/*
 * bs1770.h
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
#ifndef __BS1770_H__
#define __BS1770_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "biquad.h"
#include "bs1770_ctx.h"

#define BS1770_MAX_CHANNELS     5
#define BS1770_BUF_SIZE         9
#define BS1770_LKFS(count,sum,def) \
  ((count)?-0.691+10.0*log10((sum)/((double)(count))):(double)(def))

typedef unsigned long long bs1770_count_t;

extern double BS1770_G[BS1770_MAX_CHANNELS];

/// bs1770_hist ///////////////////////////////////////////////////////////////
typedef struct bs1770_hist_bin {
  double db;
  double x;
  double y;
  bs1770_count_t count;
} bs1770_hist_bin_t;

typedef struct bs1770_hist {
  int active;
  double gate;              // BS1770 gate, e.g. -10.0
  double reference;         // reference loudness, e.g. -23.0

  struct {
    double wmsq;            // cumulative moving average.
    bs1770_count_t count;   // number of blocks processed.
  } pass1;

  bs1770_hist_bin_t *bin;
} bs1770_hist_t;

bs1770_hist_t *bs1770_hist_init(bs1770_hist_t *hist, const bs1770_ps_t *ps);
bs1770_hist_t *bs1770_hist_cleanup(bs1770_hist_t *hist);

void bs1770_hist_reset(bs1770_hist_t *hist);
void bs1770_hist_inc_bin(bs1770_hist_t *hist, double wmsq);

void bs1770_hist_add(bs1770_hist_t *album, bs1770_hist_t *track);
double bs1770_hist_get_lufs(bs1770_hist_t *hist);
double bs1770_hist_get_lra(bs1770_hist_t *hist, double lower,
   double upper);

/// bs1770_aggr ///////////////////////////////////////////////////////////////
typedef struct bs1700_aggr {
  double gate;
  double length;        // BS1170 block length in ms
  int partition;        // BS1770 partition, e.g. 4 (75%)
  double scale;
  double fs;
  size_t overlap_size;
  size_t block_size;

  struct {
    size_t size;        // number of blocks in ring buffer.
    size_t used;        // number of blocks used in ring buffer.
    size_t offs;        // offset of front block.
    size_t count;       // number of samples processed in front block.
    double *wmsq;       // allocated blocks.
  } blocks;

  bs1770_hist_t *track;
  bs1770_hist_t *album;
} bs1770_aggr_t;

bs1770_aggr_t *bs1770_aggr_init(bs1770_aggr_t *aggr, const bs1770_ps_t *ps,
    bs1770_hist_t *track, bs1770_hist_t *album);
bs1770_aggr_t *bs1770_aggr_cleanup(bs1770_aggr_t *aggr);

void bs1770_aggr_reset(bs1770_aggr_t *aggr);
void bs1770_aggr_add_sqs(bs1770_aggr_t *aggr, double fs, double wssqs);

/// bs1770 ////////////////////////////////////////////////////////////////////
typedef struct bs1770 {
  double fs;
  int channels;
  biquad_t pre;
  biquad_t rlb;

  struct {
    double buf[BS1770_MAX_CHANNELS][BS1770_BUF_SIZE];
    int offs;
    int size;
  } ring;

  bs1770_aggr_t *lufs;
  bs1770_aggr_t *lra;
} bs1770_t;

bs1770_t *bs1770_init(bs1770_t *bs1770, bs1770_aggr_t *lufs,
    bs1770_aggr_t *lra);
bs1770_t *bs1770_cleanup(bs1770_t *bs1770);

bs1770_t *bs1770_reset(bs1770_t *bs1770);
void bs1770_add_sample(bs1770_t *bs1770, double fs, int channels,
    bs1770_sample_t sample);
void bs1770_flush(bs1770_t *bs1770);

double bs1770_track_lufs(bs1770_t *bs1770);
double bs1770_track_lra(bs1770_t *bs1770, double lower, double upper);

/// bs1770_stats //////////////////////////////////////////////////////////////
typedef struct bs1770_stats {
  int active;
  bs1770_hist_t *album;
  bs1770_hist_t track;
  bs1770_aggr_t aggr;
} bs1770_stats_t;

bs1770_stats_t *bs1770_stats_init(bs1770_stats_t *stats, bs1770_hist_t *album,
    const bs1770_ps_t *ps);
bs1770_stats_t *bs1770_stats_cleanup(bs1770_stats_t *stats);

/// bs1770_nd /////////////////////////////////////////////////////////////////
typedef struct bs1770_nd {
  bs1770_stats_t lufs;
  bs1770_stats_t lra;
  bs1770_t bs1770;
} bs1770_nd_t;

bs1770_nd_t *bs1770_nd_init(bs1770_nd_t *nd, bs1770_ctx_t *ctx,
    const bs1770_ps_t *lufs, const bs1770_ps_t *lra);
bs1770_nd_t *bs1770_nd_cleanup(bs1770_nd_t *node);

void bs1770_nd_add_sample(bs1770_nd_t *node, double fs, int channels,
    bs1770_sample_t sample);
double bs1770_nd_track_lufs(bs1770_nd_t *node);
double bs1770_nd_track_lra(bs1770_nd_t *node, double lower, double upper);

/// bs1770_ctx ////////////////////////////////////////////////////////////////
struct bs1770_ctx {
  bs1770_hist_t lufs;
  bs1770_hist_t lra;
  size_t size;
  bs1770_nd_t node;
  bs1770_nd_t *nodes;
};

bs1770_ctx_t *bs1770_ctx_init(bs1770_ctx_t *ctx, size_t size,
    const bs1770_ps_t *lufs, const bs1770_ps_t *lra);
bs1770_ctx_t *bs1770_ctx_cleanup(bs1770_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif // __BS1770_H__
