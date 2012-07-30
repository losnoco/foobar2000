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

#include <stdlib.h>
#include "biquad.h"
#include "bs1770_ctx.h"
#include "bs1770_ctx_m.h"

#define BS1770_MAX_CHANNELS     5
#define BS1770_BUF_SIZE         9
#define BS1770_LKFS(count,sum,def) \
  ((count)?-0.691+10.0*log10((sum)/((double)(count))):(double)(def))

extern double BS1770_G[BS1770_MAX_CHANNELS];

typedef struct bs1770_stats_vmt bs1770_stats_vmt_t;

typedef struct bs1770_stats_h_bin {
  double db;
  double x;
  double y;
  unsigned long long count;
} bs1770_stats_h_bin_t;

typedef struct bs1770_stats {
  bs1770_stats_vmt_t *vmt;

  double gate;          // BS1770 gate, e.g. -10.0
  double length;        // BS1170 block length in ms
  int partition;        // BS1770 partition, e.g. 4 (75%)
  double reference;     // reference loudness, e.g. -23.0
  double scale;
  double fs;
  size_t overlap_size;
  size_t block_size;

  struct {
    double gate;
    double wmsq;        // cumulative moving average.
    size_t count;       // number of blocks processed.
  } pass1;

  union {
    struct {
#ifdef _MSC_VER
      char unused;
#endif
    } s;

    struct {
      size_t filled;    // number of blocks filled in ring buffer.
      bs1770_stats_h_bin_t *bin;
    } h;
  };

  struct {
    size_t size;        // size of allocated memory.
    size_t offs;        // offset of front block.
    size_t count;       // number of samples processed in front block.
    double *wmsq;       // allocated blocks.
  } blocks;
} bs1770_stats_t;

typedef bs1770_stats_t *(*bs1770_stats_init_t)(bs1770_stats_t *stats,
     double gate, double ms, int partition, double reference);

bs1770_stats_t *bs1770_stats_s_init(bs1770_stats_t *stats, double gate,
    double ms, int partition, double reference);
bs1770_stats_t *bs1770_stats_h_init(bs1770_stats_t *stats, double gate,
    double ms, int partition, double reference);

bs1770_stats_t *bs1770_stats_alloc(bs1770_stats_t *stats, int size,
    double gain, double ms, int partition, double reference);
bs1770_stats_t *bs1770_stats_realloc(bs1770_stats_t *stats);
bs1770_stats_t *bs1770_stats_free(bs1770_stats_t *stats);
void bs1770_stats_clear(bs1770_stats_t *stats);
void bs1770_stats_set_fs(bs1770_stats_t *stats, double fs);

struct bs1770_stats_vmt {
  bs1770_stats_t *(*cleanup)(bs1770_stats_t *stats);
  void (*reset)(bs1770_stats_t *stats);
  void (*add_sqs)(bs1770_stats_t *stats, double fs, double wssqs);
  double (*get_lufs)(bs1770_stats_t *stats);
  double (*get_lufs_multiple)(bs1770_stats_t *stats, size_t stats_count);
  double (*get_lra)(bs1770_stats_t *stats, double lower, double upper);
  double (*get_lra_multiple)(bs1770_stats_t *stats, size_t stats_count, double lower, double upper);
};

#define bs1770_stats_cleanup(stats) \
  ((stats)->vmt->cleanup(stats))
#define bs1770_stats_reset(stats) \
  ((stats)->vmt->reset(stats))
#define bs1770_stats_add_sqs(stats, fs, wssqs) \
  ((stats)->vmt->add_sqs(stats, fs, wssqs))
#define bs1770_stats_get_lufs(stats) \
  ((stats)->vmt->get_lufs(stats))
#define bs1770_stats_get_lufs_multiple(stats,count) \
  ((stats)->vmt->get_lufs_multiple(stats,count))
#define bs1770_stats_get_lra(stats, lower, upper) \
  ((stats)->vmt->get_lra(stats, lower, upper))
#define bs1770_stats_get_lra_multiple(stats, count, lower, upper) \
  ((stats)->vmt->get_lra_multiple(stats, count, lower, upper))

typedef struct bs1770 {
  double fs;
  biquad_t pre;
  biquad_t rlb;

  struct {
    double buf[BS1770_MAX_CHANNELS][BS1770_BUF_SIZE];
    int offs;
    int size;
  } ring;

  bs1770_stats_t *track;
  bs1770_stats_t *album;
} bs1770_t;

typedef bs1770_t *(*bs1770_init_t)(bs1770_t *bs1770,
    bs1770_stats_t *track, bs1770_stats_t *album,
 double gate, double ms, int partition, double reference);

bs1770_t *bs1770_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album);
bs1770_t *bs1770_s_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album, double gate, double ms, int partition,
 double reference);
bs1770_t *bs1770_h_init(bs1770_t *bs1770, bs1770_stats_t *track,
    bs1770_stats_t *album, double gate, double ms, int partition,
 double reference);
bs1770_t *bs1770_cleanup(bs1770_t *bs1770);

void bs1770_set_fs(bs1770_t *bs1770, double fs, int channels);
void bs1770_add_sample(bs1770_t *bs1770, double fs, int channels,
    bs1770_sample_t sample);
void bs1770_flush(bs1770_t *bs1770, double fs, int channels);

struct bs1770_ctx {
  bs1770_stats_t track;
  bs1770_stats_t album;
  bs1770_t bs1770;
};

struct bs1770_ctx_m {
  size_t count;
  bs1770_stats_t *track;
  bs1770_stats_t *album;
  bs1770_t *bs1770;
};

bs1770_ctx_t *bs1770_ctx_init(bs1770_ctx_t *ctx, int mode, double gate,
    double ms, int partition, double def);
bs1770_ctx_t *bs1770_ctx_cleanup(bs1770_ctx_t *ctx);

bs1770_ctx_m_t *bs1770_ctx_m_init(bs1770_ctx_m_t *ctx, size_t count,
    int mode, double gate, double ms, int partition, double def);
bs1770_ctx_m_t *bs1770_ctx_m_cleanup(bs1770_ctx_m_t *ctx);

#ifdef __cplusplus
}
#endif
#endif // __BS1770_H__
