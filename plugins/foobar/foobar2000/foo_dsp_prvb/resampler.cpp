#include "resampler.h"

#include <libresample.h>

upsampler::upsampler() : handle( 0 ), remain( 0 ), min_sample_rate( 8000 ), max_sample_rate( 192000 ) { }
upsampler::~upsampler() { if ( handle ) resample_close( handle ); }

downsampler::downsampler() : handle( 0 ), remain( 0 ), min_sample_rate( 8000 ), max_sample_rate( 192000 ) { }
downsampler::~downsampler() { if ( handle ) resample_close( handle ); }

void upsampler::recreate( unsigned min, unsigned max )
{
	remain = 0;

	min_sample_rate = min;
	max_sample_rate = max;

	if ( handle ) resample_close( handle );

	double min_factor = ( double ) min_sample_rate / 22050.;
	double max_factor = ( double ) max_sample_rate / 22050.;

	if ( min_factor > max_factor ) { double temp = min_factor; min_factor = max_factor; max_factor = temp; }

	handle = resample_open( 0, min_factor, max_factor );

	if ( !handle ) throw std::bad_alloc();
}

void downsampler::recreate( unsigned min, unsigned max )
{
	remain = 0;

	min_sample_rate = min;
	max_sample_rate = max;

	if ( handle ) resample_close( handle );

	double min_factor = 22050. / ( double ) min_sample_rate;
	double max_factor = 22050. / ( double ) max_sample_rate;

	if ( min_factor > max_factor ) { double temp = min_factor; min_factor = max_factor; max_factor = temp; }

	handle = resample_open( 0, min_factor, max_factor );

	if ( !handle ) throw std::bad_alloc();
}

int upsampler::process( double factor, const float * in, int in_count, float * out, int out_count )
{
	if ( !handle ) recreate( min_sample_rate, max_sample_rate );

	buffer.grow_size( remain + in_count );
	float * bufptr = buffer.get_ptr();
	memcpy( bufptr + remain, in, in_count );

	int input_processed = 0;

	int output_generated = resample_process( handle, factor, bufptr, remain + in_count, 0, &input_processed, out, out_count );

	if ( output_generated >= 0 )
	{
		remain = in_count - input_processed;
		memcpy( bufptr, bufptr + input_processed, remain * sizeof( float ) );
	}

	return output_generated;
}

int downsampler::process( double factor, const float * in, int in_count, float * out, int out_count )
{
	if ( !handle ) recreate( min_sample_rate, max_sample_rate );

	buffer.grow_size( remain + in_count );
	float * bufptr = buffer.get_ptr();
	memcpy( bufptr + remain, in, in_count );

	int input_processed = 0;

	int output_generated = resample_process( handle, factor, bufptr, remain + in_count, 0, &input_processed, out, out_count );

	if ( output_generated >= 0 )
	{
		remain = in_count - input_processed;
		memcpy( bufptr, bufptr + input_processed, remain * sizeof( float ) );
	}

	return output_generated;
}
