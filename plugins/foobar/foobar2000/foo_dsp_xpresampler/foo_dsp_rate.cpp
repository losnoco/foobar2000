/* Copyright (c) 2008-11 lvqcl.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */ 

#include "stdafx.h"

const int buf_samples = 32768;

void dsp_rate::init()
{
	t_dsp_rate_params params;
	params.get_rateconfig(cfg_);

	sample_rate_ = 0;
	out_rate_ = 0;
	channel_count_ = 0;
	channel_map_ = 0;
	
	buf_channels_ = 0;
	buffer_ = NULL;

	frequency_accumulator_ = 0.0f;

	out_buffer_ = NULL;
}

dsp_rate::dsp_rate()
{
	init();
}

dsp_rate::dsp_rate(const t_dsp_rate_params& params)
{
	init();
	params.get_rateconfig(cfg_);
}

/*dsp_rate::dsp_rate(const dsp_preset& p_data)
{
	init();
	set_data(p_data);
}*/

bool dsp_rate::set_data(const dsp_preset & p_data)
{
	t_dsp_rate_params params;
	if (!params.set_data(p_data)) return false;
	params.get_rateconfig(cfg_);
	return true;
}

dsp_rate::~dsp_rate()
{
	delete[] out_buffer_;
	delete[] buffer_;
	buffer_ = NULL;
}

void dsp_rate::on_endoftrack(abort_callback & p_abort) { flushwrite(false); }

void dsp_rate::on_endofplayback(abort_callback & p_abort) { flushwrite(true); }

void dsp_rate::reinit(unsigned sample_rate, unsigned channel_count, unsigned channel_map)
{
	int rate_error;

	if (buf_channels_ < channel_count)
	{
		delete[] buffer_;
		buf_channels_ = channel_count;
		buffer_ = new audio_sample[buf_samples*buf_channels_];

		delete[] out_buffer_;
		out_buffer_ = new audio_sample[1024*buf_channels_];
	}

	out_rate_ = cfg_.realrate(sample_rate);

	ratio_ = (float)sample_rate / (float)out_rate_;

	memset( buffer_, 0, buf_samples * buf_channels_ * sizeof(audio_sample) );

	buffer_read_offset_ = 0;
	buffer_write_offset_ = buffer_filled_ = fir_latency( ratio_ );

	frequency_accumulator_ = 0.0f;

	channel_count_ = channel_count; channel_map_ = channel_map; sample_rate_ = sample_rate;
}

float dsp_rate::get_sample( void * context, size_t channel, size_t offset )
{
	dsp_rate * ctx = ( dsp_rate * ) context;
	return ctx->buffer_ [( (ctx->buffer_read_offset_ + offset ) % buf_samples ) * ctx->channel_count_ + channel];
}

void dsp_rate::put_sample( void * context, size_t channel, size_t offset, float sample )
{
	dsp_rate * ctx = ( dsp_rate * ) context;
	ctx->out_buffer_ [offset * ctx->channel_count_ + channel] = sample;
}

bool dsp_rate::on_chunk(audio_chunk * chunk, abort_callback & p_abort)
{
	size_t in_samples_used, out_samples_gen;
	int rate_error;

	unsigned channel_count = chunk->get_channels();
	unsigned channel_map = chunk->get_channel_config();
	unsigned sample_rate = chunk->get_sample_rate();
	t_size sample_count = chunk->get_sample_count();
	audio_sample * current = chunk->get_data();

	if ((channel_count_ != channel_count) || (channel_map_ != channel_map) || (sample_rate_ != sample_rate))
	{	// number of channels or samplerate has changed - reinitialize
		flushwrite(true); //here channel_count_, channel_map_ and sample_rate_ must have old values
		//rate_state_ == NULL here
		if (cfg_.is_no_resample(sample_rate)) return true;
		reinit(sample_rate, channel_count, channel_map);
	}

	while( sample_count || fir_required_input( 1024, ratio_, frequency_accumulator_ ) <= buffer_filled_ )
	{
		if ( sample_count )
		{
			int samples_to_buffer = buf_samples - buffer_filled_;
			if ( samples_to_buffer > sample_count ) samples_to_buffer = sample_count;
			audio_sample * write1 = buffer_ + buffer_write_offset_ * channel_count_;
			unsigned to_write1 = samples_to_buffer;
			audio_sample * write2 = NULL;
			unsigned to_write2 = 0;
			if ( samples_to_buffer + buffer_write_offset_ > buf_samples )
			{
				write2 = buffer_;
				to_write2 = samples_to_buffer + buffer_write_offset_ - buf_samples;
				to_write1 = buf_samples - buffer_write_offset_;
			}
			memcpy( write1, current, to_write1 * channel_count_ * sizeof(audio_sample) );
			if ( write2 ) memcpy( write2, current + channel_count_ * to_write1, to_write2 * channel_count_ * sizeof(audio_sample) );
			buffer_filled_ += samples_to_buffer;
			buffer_write_offset_ = ( buffer_write_offset_ + samples_to_buffer ) % buf_samples;
			sample_count -= samples_to_buffer;
			current += channel_count_ * samples_to_buffer;
			if ( fir_required_input( 1024, ratio_, frequency_accumulator_ ) > buffer_filled_ ) break;
		}
		int samples_read = fir_resample( get_sample, put_sample, this, channel_count_, 1024, ratio_, &frequency_accumulator_ );
		buffer_read_offset_ = ( buffer_read_offset_ + samples_read ) % buf_samples;
		buffer_filled_ -= samples_read;
		audio_chunk * out = insert_chunk(1024*channel_count_);
		out->set_data(out_buffer_, 1024, channel_count_, out_rate_, channel_map_);
	}

	return false;
}

void dsp_rate::flush()
{
	return;
}

void dsp_rate::flushwrite(bool close)
{
	if ( close && channel_count_ && sample_rate_ )
	{
		unsigned latency_samples = fir_latency( ratio_ );
		audio_chunk_impl temp_chunk;
		temp_chunk.pad_with_silence_ex( latency_samples, channel_count_, sample_rate_);
		temp_chunk.set_channels( channel_count_, channel_map_ );
		on_chunk( &temp_chunk, abort_callback_dummy() );
	}
	return;
}

double dsp_rate::get_latency()
{
	if (sample_rate_ && out_rate_)
		return double(buffer_filled_ - fir_latency( ratio_ ) ) / double(sample_rate_);
	else return 0;
	// warning: sample_rate_ and out_rate_ can be from previous track (?),
	// but in_samples_accum_ == out_samples_gen_accum_ == 0,  so it's ok.
}
