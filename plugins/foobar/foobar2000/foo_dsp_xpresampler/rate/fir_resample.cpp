#include "stdafx.h"

#ifndef STDAFX
#include "fir_resample.h"

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef _DEBUG
#include <stdio.h>
#endif
#endif

static int fir_len = 0;
static int fir_step = 120;

static float * fir = NULL;

static const double exp_width = 41.0;
static const int lobes_per_wing = 28;
static const double approx_bandwidth = 0.85;
static double bandwidth;

static __inline double fir_val( double x )
{
	double s, sinc, gauss;
	x *= M_PI * bandwidth;
	s = x / exp_width;
	sinc = x ? (sin(x) / x) : 1.0;
	gauss = exp(-(s * s));
	return sinc * gauss;
}

static __inline double mlinear(double y1, double y2, double mu)
{
	return y1 * (1.0 - mu) + y2 * mu;
}

#ifdef _DEBUG
static __inline double to_db(double x)
{
	return 20.0 * log(fabs(x)) / log(10.0);
}
#endif

void fir_init()
{
#ifdef _DEBUG
	double testpoint;
	double exact_val;
	double lin_approx_val;
	double lin_error_db;

	double sum;
	double test_freqs[] = { 0.5, 0.8, 1.0, 1.08, 1.18, 1.33, 1.38 };
	double dct_coeff;
	double x;
	int j;
#endif

	double amended_bandwidth;
	int i, wing_len = int(lobes_per_wing / approx_bandwidth * fir_step + 1);
	bandwidth = 1.0 * lobes_per_wing / wing_len;

	amended_bandwidth = bandwidth * fir_step;
	fir_len = 2 * wing_len + 1;

	fir = (float*) malloc( fir_len * sizeof(*fir) );

	for (i = 0; i < fir_len; i++)
		fir [i] = fir_val( i - wing_len );

#ifdef _DEBUG
	fprintf( stderr, "size: %u\n", fir_len );

	testpoint = 0.5;
	exact_val = fir_val( testpoint );
	lin_approx_val = mlinear( fir [wing_len], fir [wing_len + 1], testpoint );

	lin_error_db = to_db( exact_val - lin_approx_val );

	fprintf( stderr, "interpolation noise: %1.2f dB\n", lin_error_db );

	sum = 0.0;
	for ( i = 0; i < fir_len; i++ ) sum += fir [i];

	for ( j = 0; j < _countof( test_freqs ); j++ )
	{
		dct_coeff = 0.0;
		for ( i = 0; i < fir_len; i++ )
		{
			x = 1.0 * (i - wing_len) / fir_step;
			dct_coeff += fir [i] * cos( x * test_freqs [j] * M_PI );
		}
		fprintf( stderr, "DCT: %1.2f -> %1.2f dB\n", test_freqs [j], to_db( dct_coeff / sum ) );
	}
#endif
}

void fir_shutdown()
{
	if ( fir ) free( fir );
	fir = NULL;
}

int fir_latency( float ratio )
{
	float freqAdjust = ratio;
	unsigned int dsbfirstep;
	if ( freqAdjust > 1.0f )
	{
		dsbfirstep = ceil( fir_step / freqAdjust );
	}
	else
	{
		dsbfirstep = fir_step;
	}
	return ( fir_len / dsbfirstep + 1 ) / 2;
}

int fir_required_input( size_t output_count, float ratio, float frequency_accumulator )
{
	float freqAdjust = ratio;
	float freqAcc_start = frequency_accumulator;
	unsigned int dsbfirstep;
	unsigned int max_ipos = freqAcc_start + output_count * freqAdjust;
	unsigned int fir_cachesize;
	unsigned int required_input;
	if ( freqAdjust > 1.0f )
	{
		dsbfirstep = ceil( fir_step / freqAdjust );
	}
	else
	{
		dsbfirstep = fir_step;
	}
	fir_cachesize = ( fir_len + dsbfirstep - 2 ) / dsbfirstep;
	required_input = max_ipos + fir_cachesize;
	return required_input;
}

int fir_resample( callback_get_sample getter, callback_put_sample putter, void * context, size_t channels, size_t output_count, float ratio, float * frequency_accumulator )
{
	unsigned int i, channel;

	float freqAdjust = ratio;
	float freqAcc_start = *frequency_accumulator;
	float freqAcc_end = freqAcc_start + output_count * freqAdjust;
	unsigned int dsbfirstep;
	unsigned int max_ipos = freqAcc_start + output_count * freqAdjust;

	unsigned int fir_cachesize;
	unsigned int required_input;

	float * intermediate;
	float * fir_copy;

	float * itmp;

	float fir_gain;

	if ( freqAdjust > 1.0f )
	{
		dsbfirstep = ceil( fir_step / freqAdjust );
	}
	else
	{
		dsbfirstep = fir_step;
	}

	fir_gain = (float)dsbfirstep / fir_step;

	fir_cachesize = ( fir_len + dsbfirstep - 2 ) / dsbfirstep;
	required_input = max_ipos + fir_cachesize;

	intermediate = (float*) _alloca( required_input * channels * sizeof(float) );
	fir_copy = (float*) _alloca( fir_cachesize * sizeof(float) );

	itmp = intermediate;
	for ( channel = 0; channel < channels; channel++ )
		for ( i = 0; i < required_input; i++ )
			*(itmp++) = getter( context, channel, i );

	for ( i = 0; i < output_count; i++ )
	{
		float total_fir_steps = ( freqAcc_start + i * freqAdjust ) * dsbfirstep;
		unsigned int int_fir_steps = total_fir_steps;
		unsigned int ipos = int_fir_steps / dsbfirstep;

		unsigned int idx = ( ipos + 1 ) * dsbfirstep - int_fir_steps - 1;
		float rem = int_fir_steps + 1.0 - total_fir_steps;

		int fir_used = 0;
		while ( idx < fir_len - 1 )
		{
			fir_copy [fir_used++] = mlinear( fir [idx], fir [idx + 1], rem );
			idx += dsbfirstep;
		}

		assert( fir_used <= fir_cachesize );
		assert( ipos + fir_used <= required_input );

		for ( channel = 0; channel < channels; channel++ )
		{
			int j;
			float sum = 0.0f;
			float * cache = &intermediate [channel * required_input + ipos];
			for ( j = 0; j < fir_used; j++ )
				sum += fir_copy [j] * cache [j];
			putter( context, channel, i, sum * fir_gain );
		}
	}

	freqAcc_end -= floor( freqAcc_end );
	*frequency_accumulator = freqAcc_end;

	/*free( fir_copy );
	free( intermediate );*/

	return max_ipos;
}
