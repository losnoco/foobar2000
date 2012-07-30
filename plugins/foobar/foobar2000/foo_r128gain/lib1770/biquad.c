/*
 * biquad.c
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
#include <stdio.h>
#include "biquad.h"

#define IS_DEN(x) \
    (fabs(den_tmp=(x))<1.0e-15)
#define DEN(x) \
    (IS_DEN(x)?0.0:den_tmp)

void biquad_get_ps(biquad_t *biquad, biquad_ps_t *ps)
{
  double x11 = biquad->a1 - 2;
  double x12 = biquad->a1;
  double x1 = -biquad->a1 - 2;

  double x21 = biquad->a2 - 1;
  double x22 = biquad->a2 + 1;
  double x2 = -biquad->a2 + 1;

  double dx = x22*x11 - x12*x21;
  double k_sq = (x22*x1 - x12*x2)/dx;
  double k_by_q = (x11*x2 - x21*x1)/dx;
  double a0 = 1.0 + k_by_q + k_sq;

  ps->k = sqrt(k_sq);
  ps->q = ps->k/k_by_q;
  ps->vb = 0.5*a0*(biquad->b0 - biquad->b2)/k_by_q;
  ps->vl = 0.25*a0*(biquad->b0 + biquad->b1 + biquad->b2)/k_sq;
  ps->vh = 0.25*a0*(biquad->b0 - biquad->b1 + biquad->b2);
}

biquad_t *biquad_requantize(biquad_t *in, biquad_t *out)
{
  if (in->fs==out->fs)
    *out=*in;
  else {
    biquad_ps_t ps;
    double k, k_sq, k_by_q, a0;
	double den_tmp;

    biquad_get_ps(in, &ps);
    k=tan((in->fs/out->fs)*atan(ps.k));
    k_sq = k*k;
    k_by_q = k/ps.q;
    a0 = 1.0 + k_by_q + k_sq;

    out->a1 = DEN((2.0*(k_sq - 1.0))/a0);
    out->a2 = DEN((1.0 - k_by_q + k_sq)/a0);
    out->b0 = DEN((ps.vh + ps.vb*k_by_q + ps.vl*k_sq)/a0);
    out->b1 = DEN((2.0 * (ps.vl*k_sq - ps.vh))/a0);
    out->b2 = DEN((ps.vh - ps.vb*k_by_q + ps.vl*k_sq)/a0);
  }

  return out;
}

int biquad2sox(biquad_t *biquad,  char **argv, biquad_sox_args_t args)
{
  int argc=0;

  sprintf(*argv++=args[argc++], "%.32f", biquad->b0);
  sprintf(*argv++=args[argc++], "%.32f", biquad->b1);
  sprintf(*argv++=args[argc++], "%.32f", biquad->b2);
  sprintf(*argv++=args[argc++], "%.32f", 1.0);
  sprintf(*argv++=args[argc++], "%.32f", biquad->a1);
  sprintf(*argv++=args[argc++], "%.32f", biquad->a2);

  return argc;
}
