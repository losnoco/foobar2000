#ifndef _RESAMPLER_H_
#define _RESAMPLER_H_

#include <foobar2000.h>

class upsampler
{
	void * handle;
	unsigned remain, min_sample_rate, max_sample_rate;
	pfc::array_t<float> buffer;

public:
	upsampler();
	~upsampler();

	void recreate( unsigned min, unsigned max );
	int process( double factor, const float * in, int in_count, float * out, int out_count );

	inline unsigned buffered() const { return remain; }
};

class downsampler
{
	void * handle;
	unsigned remain, min_sample_rate, max_sample_rate;
	pfc::array_t<float> buffer;

public:
	downsampler();
	~downsampler();

	void recreate( unsigned min, unsigned max );
	int process( double factor, const float * in, int in_count, float * out, int out_count );

	inline unsigned buffered() const { return remain; }
};

#endif
