#include "dumb.h"

#include <assert.h>
#include <string.h>

#if 0
void dumb_reset_resampler_n(int n, DUMB_RESAMPLER *resampler, void *src, int src_channels, long pos, long start, long end, int quality)
{
	resampler->src = src;
	resampler->pos = pos;
	resampler->subpos = 0;
	resampler->start = start;
	resampler->end = end;
	resampler->dir = 1;
	resampler->pickup = NULL;
	resampler->pickup_data = NULL;
	if (quality < 0)
	{
		resampler->quality = 0;
	}
	else if (quality > DUMB_RQ_N_LEVELS - 1)
	{
		resampler->quality = DUMB_RQ_N_LEVELS - 1;
	}
	else
	{
		resampler->quality = quality;
	}
	memset( resampler->x.x24, 0, sizeof( resampler->x.x24 ) );
	resampler->overshot = -1;
}

DUMB_RESAMPLER *dumb_start_resampler_n(int n, void *src, int src_channels, long pos, long start, long end, int quality)
{
	DUMB_RESAMPLER *resampler = (DUMB_RESAMPLER *) malloc(sizeof(*resampler));
	if (!resampler) return NULL;
	dumb_reset_resampler_n(n, resampler, src, src_channels, pos, start, end, quality);
	return resampler;
}

#define MULSCV(a, b) ((int)((LONG_LONG)(a) * (b) >> 32))
#define MULSC(a, b) ((int)((LONG_LONG)((a) << 4) * ((b) << 12) >> 32))
#define MULSC16(a, b) ((int)((LONG_LONG)((a) << 12) * ((b) << 12) >> 32))

template <typename srctype, int src_chan, int dst_chan>
int process_pickup(DUMB_RESAMPLER *resampler);

template <typename srctype, int src_chan, int dst_chan>
long dumb_resample(DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, float delta)
{
	int dt;
	int lvol, lvolr, lvold, lvolt, lvolm, rvol, rvolr, rvold, rvolt, rvolm;
	long done;
	long todo;
	LONG_LONG todo64;
	int quality;

	if (!resampler || resampler->dir == 0) return 0;
	ASSERT(resampler->dir == -1 || resampler->dir == 1);

	done = 0;
	dt = (int)(delta * 65536.0 + 0.5);
	if (dt == 0) return 0;

	if ( volume_left )
	{
		lvolr = (int)(volume_left->volume * 16777216.0);
		lvold = (int)(volume_left->delta * 16777216.0);
		lvolt = (int)(volume_left->target * 16777216.0);
		lvolm = (int)(volume_left->mix * 65536.0);
		lvol = MULSCV( lvolr, lvolm );
		if ( lvolr == lvolt ) volume_left = NULL;
	}
	else
	{
		lvol = 0;
		lvolt = 0;
	}

	if ( volume_right )
	{
		rvolr = (int)(volume_right->volume * 16777216.0);
		rvold = (int)(volume_right->delta * 16777216.0);
		rvolt = (int)(volume_right->target * 16777216.0);
		rvolm = (int)(volume_right->mix * 65536.0);
		rvol = MULSCV( rvolr, rvolm );
		if ( rvolr == rvolt ) volume_right = NULL;
	}
	else
	{
		rvol = 0;
		rvolt = 0;
	}

	if ( !volume_left && !volume_right ) dst = NULL;

	quality = resampler->quality;

	while ( done < dst_size )
	{
		if ( process_pickup<srctype, src_chan, dst_chan>( resampler ) )
		{
			if ( volume_left ) volume_left->volume = (float)lvolr / 16777216.0f;
			if ( volume_right ) volume_right->volume = (float)rvolr / 16777216.0f;
			return done;
		}

		if ((resampler->dir ^ dt) < 0)
			dt = -dt;

		if (resampler->dir < 0)
			todo64 = ((((LONG_LONG)(resampler->pos - resampler->start) << 16) + resampler->subpos - dt) / -dt);
		else
			todo64 = ((((LONG_LONG)(resampler->end - resampler->pos) << 16) - resampler->subpos - 1 + dt) / dt);

		if (todo64 < 0)
			todo = 0;
		else if (todo64 > dst_size - done)
			todo = dst_size - done;
		else
			todo = (long) todo64;
		done += todo;

		{
			srctype *src = (srctype *) resampler->src;
			long pos = resampler->pos;
			int subpos = resampler->subpos;
			long diff = pos;
			long overshot;
			if (resampler->dir < 0)
			{
				if (!dst)
				{
					/* Silence or simulation */
					LONG_LONG new_subpos = subpos + (LONG_LONG)dt * todo;
					pos += (long)(new_subpos >> 16);
					subpos = (long)new_subpos & 65535;
				}
				else if (quality <= DUMB_RQ_ALIASING)
				{
					/* Aliasing, backwards */
					srctype xbuf[2*src_chan];
					srctype *x = &xbuf[0];
					srctype *xstart;
					srctype *xresampler = (srctype*)(&resampler->x);
					xbuf [0] = xresampler [0];
					xbuf [1] = xresampler [1];
					if ( src_chan == 2 )
					{
						xbuf [2] = xresampler [2];
						xbuf [3] = xresampler [3];
					}
					while (todo && x < &xbuf[2*src_chan])
					{
						// TODO: check what happens when multiple tempo slides occur per row
						assert(pos >= resampler->start);
						MIX_ALIAS(+=, 1, 0);
						subpos += dt;
						pos += subpos >> 16;
						x -= (subpos >> 16) * src_chan;
						subpos &= 65535;
						todo--;
					}
					x = xstart = &src[pos*src_chan];
					LOOP4(todo,
						MIX_ALIAS(+=, 1, 2);
					subpos += dt;
					x += (subpos >> 16) * src_chan;
					subpos &= 65535;
					);
					pos += DIVIDE_BY_SRC_CHANNELS(x - xstart);
				}
				else if (quality <= DUMB_RQ_LINEAR)
				{
					/* Linear interpolation, backwards */
					SRCTYPE xbuf[3*SRC_CHANNELS];
					SRCTYPE *x = &xbuf[1*SRC_CHANNELS];
					COPYSRC(xbuf, 0, resampler->X, 1);
					COPYSRC(xbuf, 1, resampler->X, 2);
					COPYSRC(xbuf, 2, src, pos);
					while (todo && x < &xbuf[3*SRC_CHANNELS])
					{
						HEAVYASSERT(pos >= resampler->start);
						MIX_LINEAR(+=, 1, 0, -1);
						subpos += dt;
						pos += subpos >> 16;
						x -= (subpos >> 16) * SRC_CHANNELS;
						subpos &= 65535;
						todo--;
					}
					// TODO: use xstart for others too
					x = &src[pos*SRC_CHANNELS];
					LOOP4(todo,
						HEAVYASSERT(pos >= resampler->start);
					MIX_LINEAR(+=, 1, 1, 2);
					subpos += dt;
					pos += subpos >> 16;
					x += (subpos >> 16) * SRC_CHANNELS;
					subpos &= 65535;
					);
				}
				else
				{
					/* Cubic interpolation, backwards */
					SRCTYPE xbuf[6*SRC_CHANNELS];
					SRCTYPE *x = &xbuf[3*SRC_CHANNELS];
					COPYSRC(xbuf, 0, resampler->X, 0);
					COPYSRC(xbuf, 1, resampler->X, 1);
					COPYSRC(xbuf, 2, resampler->X, 2);
					COPYSRC(xbuf, 3, src, pos);
					if (pos-1 >= resampler->start) COPYSRC(xbuf, 4, src, pos-1);
					if (pos-2 >= resampler->start) COPYSRC(xbuf, 5, src, pos-2);
					while (todo && x < &xbuf[6*SRC_CHANNELS])
					{
						HEAVYASSERT(pos >= resampler->start);
						MIX_CUBIC(+=, 1, x, x, 0, -1, -2, -3);
						subpos += dt;
						pos += subpos >> 16;
						x -= (subpos >> 16) * SRC_CHANNELS;
						subpos &= 65535;
						todo--;
					}
					x = &src[pos*SRC_CHANNELS];
					LOOP4(todo,
						HEAVYASSERT(pos >= resampler->start);
					MIX_CUBIC(+=, 1, x, x, 0, 1, 2, 3);
					subpos += dt;
					pos += subpos >> 16;
					x += (subpos >> 16) * SRC_CHANNELS;
					subpos &= 65535;
					);
				}
				diff = diff - pos;
				overshot = resampler->start - pos - 1;
				if (diff >= 3)
				{
					COPYSRC2(resampler->X, 0, overshot < 3, src, pos+3);
					COPYSRC2(resampler->X, 1, overshot < 2, src, pos+2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos+1);
				}
				else if (diff >= 2)
				{
					COPYSRC(resampler->X, 0, resampler->X, 2);
					COPYSRC2(resampler->X, 1, overshot < 2, src, pos+2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos+1);
				}
				else if (diff >= 1)
				{
					COPYSRC(resampler->X, 0, resampler->X, 1);
					COPYSRC(resampler->X, 1, resampler->X, 2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos+1);
				}
			}
			else
			{
				if (!dst)
				{
					/* Silence or simulation */
					LONG_LONG new_subpos = subpos + (LONG_LONG)dt * todo;
					pos += (long)(new_subpos >> 16);
					subpos = (long)new_subpos & 65535;
				}
				else if (quality <= DUMB_RQ_ALIASING)
				{
					/* Aliasing, forwards */
					SRCTYPE xbuf[2*SRC_CHANNELS];
					SRCTYPE *x = &xbuf[0];
					SRCTYPE *xstart;
					COPYSRC(xbuf, 0, resampler->X, 1);
					COPYSRC(xbuf, 1, resampler->X, 2);
					while (todo && x < &xbuf[2*SRC_CHANNELS])
					{
						HEAVYASSERT(pos < resampler->end);
						MIX_ALIAS(+=, 1, 0);
						subpos += dt;
						pos += subpos >> 16;
						x += (subpos >> 16) * SRC_CHANNELS;
						subpos &= 65535;
						todo--;
					}
					x = xstart = &src[pos*SRC_CHANNELS];
					LOOP4(todo,
						MIX_ALIAS(+=, 1, -2);
					subpos += dt;
					x += (subpos >> 16) * SRC_CHANNELS;
					subpos &= 65535;
					);
					pos += DIVIDE_BY_SRC_CHANNELS(x - xstart);
				}
				else if (quality <= DUMB_RQ_LINEAR)
				{
					/* Linear interpolation, forwards */
					SRCTYPE xbuf[3*SRC_CHANNELS];
					SRCTYPE *x = &xbuf[1*SRC_CHANNELS];
					COPYSRC(xbuf, 0, resampler->X, 1);
					COPYSRC(xbuf, 1, resampler->X, 2);
					COPYSRC(xbuf, 2, src, pos);
					while (todo && x < &xbuf[3*SRC_CHANNELS])
					{
						HEAVYASSERT(pos < resampler->end);
						MIX_LINEAR(+=, 1, -1, 0);
						subpos += dt;
						pos += subpos >> 16;
						x += (subpos >> 16) * SRC_CHANNELS;
						subpos &= 65535;
						todo--;
					}
					x = &src[pos*SRC_CHANNELS];
					LOOP4(todo,
						HEAVYASSERT(pos < resampler->end);
					MIX_LINEAR(+=, 1, -2, -1);
					subpos += dt;
					pos += subpos >> 16;
					x += (subpos >> 16) * SRC_CHANNELS;
					subpos &= 65535;
					);
				}
				else
				{
					/* Cubic interpolation, forwards */
					SRCTYPE xbuf[6*SRC_CHANNELS];
					SRCTYPE *x = &xbuf[3*SRC_CHANNELS];
					COPYSRC(xbuf, 0, resampler->X, 0);
					COPYSRC(xbuf, 1, resampler->X, 1);
					COPYSRC(xbuf, 2, resampler->X, 2);
					COPYSRC(xbuf, 3, src, pos);
					if (pos+1 < resampler->end) COPYSRC(xbuf, 4, src, pos+1);
					if (pos+2 < resampler->end) COPYSRC(xbuf, 5, src, pos+2);
					while (todo && x < &xbuf[6*SRC_CHANNELS])
					{
						HEAVYASSERT(pos < resampler->end);
						MIX_CUBIC(+=, 1, x, x, -3, -2, -1, 0);
						subpos += dt;
						pos += subpos >> 16;
						x += (subpos >> 16) * SRC_CHANNELS;
						subpos &= 65535;
						todo--;
					}
					x = &src[pos*SRC_CHANNELS];
					LOOP4(todo,
						HEAVYASSERT(pos < resampler->end);
					MIX_CUBIC(+=, 1, x, x, -3, -2, -1, 0);
					subpos += dt;
					pos += subpos >> 16;
					x += (subpos >> 16) * SRC_CHANNELS;
					subpos &= 65535;
					);
				}
				diff = pos - diff;
				overshot = pos - resampler->end;
				if (diff >= 3)
				{
					COPYSRC2(resampler->X, 0, overshot < 3, src, pos-3);
					COPYSRC2(resampler->X, 1, overshot < 2, src, pos-2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos-1);
				}
				else if (diff >= 2)
				{
					COPYSRC(resampler->X, 0, resampler->X, 2);
					COPYSRC2(resampler->X, 1, overshot < 2, src, pos-2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos-1);
				}
				else if (diff >= 1)
				{
					COPYSRC(resampler->X, 0, resampler->X, 1);
					COPYSRC(resampler->X, 1, resampler->X, 2);
					COPYSRC2(resampler->X, 2, overshot < 1, src, pos-1);
				}
			}
			resampler->pos = pos;
			resampler->subpos = subpos;
		}
	}
}

template <typename srctype, int src_chan, int dst_chan>
int process_pickup(DUMB_RESAMPLER *resampler)
{
	if (resampler->overshot < 0) {
		resampler->overshot = 0;
		dumb_resample<srctype, src_chan, dst_chan>(resampler, NULL, 2, 0, 0, 1.0f);
		((srctype*)(&resampler->x))[ 0 ] = ((srctype*)(&resampler->x))[ src_chan ];
		if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 1 ] = ((srctype*)(&resampler->x))[ 3 ];
	}

	for (;;) {
		srctype *src = (srctype *) resampler->src;

		if (resampler->dir < 0) {
			if (resampler->overshot >= 3 && resampler->pos+3 >= resampler->start)
			{
				((srctype*)(&resampler->x))[ 0 * src_chan ] = src[ resampler->pos * src_chan + 3 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 0 * src_chan + 1 ] = src[ resampler->pos * src_chan + 3 * src_chan + 1 ];
			}
			if (resampler->overshot >= 2 && resampler->pos+2 >= resampler->start)
			{
				((srctype*)(&resampler->x))[ 1 * src_chan ] = src[ resampler->pos * src_chan + 2 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 1 * src_chan + 1 ] = src[ resampler->pos * src_chan + 2 * src_chan + 1 ];
			}
			if (resampler->overshot >= 1 && resampler->pos+1 >= resampler->start)
			{
				((srctype*)(&resampler->x))[ 2 * src_chan ] = src[ resampler->pos * src_chan + 1 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 2 * src_chan + 1 ] = src[ resampler->pos * src_chan + 1 * src_chan + 1 ];
			}
			resampler->overshot = resampler->start - resampler->pos - 1;
		} else {
			if (resampler->overshot >= 3 && resampler->pos-3 < resampler->end)
			{
				((srctype*)(&resampler->x))[ 0 * src_chan ] = src[ resampler->pos * src_chan - 3 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 0 * src_chan + 1 ] = src[ resampler->pos * src_chan - 3 * src_chan + 1 ];
			}
			if (resampler->overshot >= 2 && resampler->pos-2 < resampler->end)
			{
				((srctype*)(&resampler->x))[ 1 * src_chan ] = src[ resampler->pos * src_chan - 2 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 1 * src_chan + 1 ] = src[ resampler->pos * src_chan - 2 * src_chan + 1 ];
			}
			if (resampler->overshot >= 1 && resampler->pos-1 < resampler->end)
			{
				((srctype*)(&resampler->x))[ 2 * src_chan ] = src[ resampler->pos * src_chan - 1 * src_chan ];
				if ( src_chan == 2 ) ((srctype*)(&resampler->x))[ 2 * src_chan + 1 ] = src[ resampler->pos * src_chan - 1 * src_chan + 1 ];
			}
			resampler->overshot = resampler->pos - resampler->end;
		}

		if (resampler->overshot < 0) {
			resampler->overshot = 0;
			return 0;
		}

		if (!resampler->pickup) {
			resampler->dir = 0;
			return 1;
		}
		(*resampler->pickup)(resampler, resampler->pickup_data);
		if (resampler->dir == 0) return 1;
		ASSERT(resampler->dir == -1 || resampler->dir == 1);
	}
}

long dumb_resample_n_1_1(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, DUMB_VOLUME_RAMP_INFO * volume, float delta)
{
	if ( n == 8 ) return dumb_resample<signed char, 1, 1> ( resampler, dst, dst_size, volume, 0, delta );
}
long dumb_resample_n_1_2(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, float delta);
long dumb_resample_n_2_1(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, float delta);
long dumb_resample_n_2_2(int n, DUMB_RESAMPLER *resampler, sample_t *dst, long dst_size, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, float delta);
void dumb_resample_get_current_sample_n_1_1(int n, DUMB_RESAMPLER *resampler, DUMB_VOLUME_RAMP_INFO * volume, sample_t *dst);
void dumb_resample_get_current_sample_n_1_2(int n, DUMB_RESAMPLER *resampler, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, sample_t *dst);
void dumb_resample_get_current_sample_n_2_1(int n, DUMB_RESAMPLER *resampler, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, sample_t *dst);
void dumb_resample_get_current_sample_n_2_2(int n, DUMB_RESAMPLER *resampler, DUMB_VOLUME_RAMP_INFO * volume_left, DUMB_VOLUME_RAMP_INFO * volume_right, sample_t *dst);
#endif