
// Finite impulse response (FIR) resampler with adjustable FIR size

// Game_Music_Emu 0.2.7. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.

#ifndef FIR_RESAMPLER_H
#define FIR_RESAMPLER_H

#include "blargg_common.h"

class Fir_Resampler_ {
public:
	typedef short sample_t;
	
	~Fir_Resampler_();
	
	// Set input/output resampling ratio and optionally low-pass rolloff and gain.
	// Returns actual ratio used (rounded to internal precision).
	double time_ratio( double factor, double rolloff = 0.999, double gain = 1.0 );
	
	// Current input/output ratio
	double ratio() const { return ratio_; }
	
// Input
	
	// Resize and clear input buffer
	blargg_err_t buffer_size( int );
	
	// Clear input buffer. At least two output samples will be available after
	// two input samples are written.
	void clear();
	
	// Number of input samples that can be written
	int max_write() const { return buf_size - (write_pos - buf); }
	
	// Pointer to place to write input samples
	sample_t* buffer() { return write_pos; }
	
	// Notify resampler that 'count' input samples have been written
	void write( long count );
	
	// Number of input samples in buffer
	int written() const { return write_pos - buf - write_offset; }
	
	// Skip 'count' input samples. Returns number of samples actually skipped.
	int skip_input( long count );
	
// Output
	
	// Number of extra input samples needed until 'count' output samples are available
	int input_needed( long count ) const;
	
	// Number of output samples available
	int avail() const { return avail_( write_pos - buf - width_ * stereo ); }
	
protected:
	enum { stereo = 2 };
	enum { max_res = 32 };
	sample_t* buf;
	sample_t* write_pos;
	int buf_size;
	int res;
	int imp;
	int const width_;
	int const write_offset;
	unsigned long skip_bits;
	int step;
	int input_per_cycle;
	double ratio_;
	sample_t* impulses;
	
	Fir_Resampler_( int width, sample_t* );
	int avail_( long input_count ) const;
};

// Width is number of points in FIR. Must be even and 4 or more. More points give
// better quality and rolloff effectiveness, and take longer to calculate.
template<int width>
class Fir_Resampler : public Fir_Resampler_ {
	BOOST_STATIC_ASSERT( width >= 4 );
	BOOST_STATIC_ASSERT( width % 2 == 0 );
	short impulses [max_res] [width];
public:
	Fir_Resampler() : Fir_Resampler_( width, impulses [0] ) { }
	
	// Read at most 'count' samples. Returns number of samples actually read.
	int read( sample_t* out, long count );
};

// End of public interface

inline void Fir_Resampler_::write( long count )
{
	write_pos += count;
	assert( unsigned (write_pos - buf) <= buf_size );
}

template<int width>
int Fir_Resampler<width>::read( sample_t* out_begin, long count )
{
	sample_t* out = out_begin;
	const sample_t* in = buf;
	sample_t* end_pos = write_pos;
	unsigned long skip = skip_bits >> this->imp;
	sample_t const* imp = impulses [this->imp];
	int remain = res - this->imp;
	int const step = this->step;
	
	count >>= 1;
	
	if ( end_pos - in >= width * stereo )
	{
		end_pos -= width * stereo;
		do
		{
			count--;
			
			// accumulate in extended precision
			long l = 0;
			long r = 0;
			
			const sample_t* i = in;
			if ( count < 0 )
				break;
			
			for ( int n = width / 2; n--; )
			{
				int pt0 = imp [0];
				int pt1 = imp [1];
				imp += 2;
				l += (pt0 * i [0]) + (pt1 * i [2]);
				r += (pt0 * i [1]) + (pt1 * i [3]);
				i += 4;
			}
			
			remain--;
			
			l >>= 15;
			r >>= 15;
			
			in += step + ((skip * stereo) & stereo);
			skip >>= 1;
			
			if ( !remain )
			{
				imp = impulses [0];
				skip = skip_bits;
				remain = res;
			}
			
			out [0] = l;
			out [1] = r;
			out += stereo;
		}
		while ( in <= end_pos );
	}
	
	this->imp = res - remain;
	
	int left = write_pos - in;
	write_pos = buf + left;
	assert( unsigned (write_pos - buf) <= buf_size );
	STD::memmove( buf, in, left * sizeof *in );
	
	return out - out_begin;
}

#endif

