/*
 * biquad.h
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
#ifndef __BIQUAD_H__
#define __BIQUAD_H__

typedef struct biquad {
  double fs;
  double a1, a2;
  double b0, b1, b2;
} biquad_t;

typedef struct biquad_ps {
  double k;
  double q;
  double vb;
  double vl;
  double vh;
} biquad_ps_t;

typedef char biquad_sox_args_t[6][64];

void biquad_get_ps(biquad_t *biquad, biquad_ps_t *ps);
biquad_t *biquad_requantize(biquad_t *in, biquad_t *out);

int biquad2sox(biquad_t *biquad,  char **argv, biquad_sox_args_t args);

#endif // __BIQUAD_H__
