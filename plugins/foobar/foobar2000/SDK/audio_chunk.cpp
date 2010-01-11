#include "foobar2000.h"

bool audio_chunk::set_data(const audio_sample * src,unsigned samples,unsigned nch,unsigned srate,unsigned channel_config)
{
	bool rv = false;
	unsigned size = samples * nch;
	audio_sample * out = check_data_size(size);
	if (out)
	{
		if (src)
			mem_ops<audio_sample>::copy(out,src,size);
		else
			mem_ops<audio_sample>::set(out,0,size);
		set_sample_count(samples);
		set_channels(nch,channel_config);
		set_srate(srate);
		rv = true;
	}
	else reset();
	return rv;
}

static bool check_exclusive(unsigned val, unsigned mask)
{
	return (val&mask)!=0 && (val&mask)!=mask;
}

namespace {

	template<class T,bool b_swap,bool b_signed,bool b_pad> class msvc6_sucks_v2 { public:
		inline static void do_fixedpoint_convert(const void * source,unsigned bps,unsigned count,audio_sample* buffer)
		{
			const char * src = (const char *) source;
			unsigned bytes = bps>>3;
			unsigned n;
			T max = ((T)1)<<(bps-1);
			
			T negmask = - max;

			ASSUME(bytes<=sizeof(T));

			const double div = 1.0 / (double)(1<<(bps-1));
			for(n=0;n<count;n++)
			{
				T temp;
				if (b_pad)
				{
					temp = 0;
					memcpy(&temp,src,bytes);
					if (b_swap) byte_order::swap_order(&temp,bytes);
				}
				else
				{
					temp = * reinterpret_cast<const T*>(src);
					if (b_swap) byte_order::swap_t(temp);
				}

				
				
				if (!b_signed) temp ^= max;

				if (b_pad)
				{
					if (temp & max) temp |= negmask;
				}

				if (b_pad)
					src += bytes;
				else
					src += sizeof(T);


				buffer[n] = (audio_sample) ( (double)temp * div );
			}
		}
	};

	template <class T,bool b_pad> class msvc6_sucks { public:
		inline static void do_fixedpoint_convert(bool b_swap,bool b_signed,const void * source,unsigned bps,unsigned count,audio_sample* buffer)
		{
			if (sizeof(T)==1)
			{
				if (b_signed)
				{
					msvc6_sucks_v2<T,false,true,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
				else
				{
					msvc6_sucks_v2<T,false,false,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
			}
			else if (b_swap)
			{
				if (b_signed)
				{
					msvc6_sucks_v2<T,true,true,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
				else
				{
					msvc6_sucks_v2<T,true,false,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
			}
			else
			{
				if (b_signed)
				{
					msvc6_sucks_v2<T,false,true,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
				else
				{
					msvc6_sucks_v2<T,false,false,b_pad>::do_fixedpoint_convert(source,bps,count,buffer);
				}
			}
		}
	};


};


bool audio_chunk::set_data_fixedpoint_ex(const void * source,unsigned size,unsigned srate,unsigned nch,unsigned bps,unsigned flags,unsigned p_channel_config)
{
	assert( check_exclusive(flags,FLAG_SIGNED|FLAG_UNSIGNED) );
	assert( check_exclusive(flags,FLAG_LITTLE_ENDIAN|FLAG_BIG_ENDIAN) );

	bool need_swap = !!(flags & FLAG_BIG_ENDIAN);
	if (byte_order::machine_is_big_endian()) need_swap = !need_swap;

	unsigned count = size / (bps/8);
	audio_sample * buffer = check_data_size(count);
	if (buffer==0) {reset();return false;}
	bool b_signed = !!(flags & FLAG_SIGNED);

	switch(bps)
	{
	case 8:
		msvc6_sucks<char,false>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;
	case 16:
		msvc6_sucks<short,false>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;
	case 24:
		msvc6_sucks<long,true>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;
	case 32:
		msvc6_sucks<long,false>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;
	case 40:
	case 48:
	case 56:
	case 64:
		msvc6_sucks<__int64,true>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;
/*
		additional template would bump size while i doubt if anyone playing PCM above 32bit would care about performance gain
		msvc6_sucks<__int64,false>::do_fixedpoint_convert(need_swap,b_signed,source,bps,count,buffer);
		break;*/
	default:
		//unknown size, cant convert
		mem_ops<audio_sample>::setval(buffer,0,count);
		break;
	}
	set_sample_count(count/nch);
	set_srate(srate);
	set_channels(nch,p_channel_config);
	return true;
}

template<class t_float>
static void process_float_multi(audio_sample * p_out,const t_float * p_in,const unsigned p_count)
{
	unsigned n;
	for(n=0;n<p_count;n++)
		p_out[n] = (audio_sample)p_in[n];
}

template<class t_float>
static void process_float_multi_swap(audio_sample * p_out,const t_float * p_in,const unsigned p_count)
{
	unsigned n;
	for(n=0;n<p_count;n++)
	{
		t_float temp = p_in[n];
		byte_order::swap_t(temp);
		p_out[n] = (audio_sample) temp;
	}
}

bool audio_chunk::set_data_floatingpoint_ex(const void * ptr,unsigned size,unsigned srate,unsigned nch,unsigned bps,unsigned flags,unsigned p_channel_config)
{
	assert(bps==32 || bps==64);
	assert( check_exclusive(flags,FLAG_LITTLE_ENDIAN|FLAG_BIG_ENDIAN) );
	assert( ! (flags & (FLAG_SIGNED|FLAG_UNSIGNED) ) );

	bool use_swap = byte_order::machine_is_big_endian() ? !!(flags & FLAG_LITTLE_ENDIAN) : !!(flags & FLAG_BIG_ENDIAN);

	const unsigned count = size / (bps/8);
	audio_sample * out = check_data_size(count);
	if (out==0) {reset();return false;}

	if (bps == 32)
	{
		if (use_swap)
			process_float_multi_swap(out,reinterpret_cast<const float*>(ptr),count);
		else
			process_float_multi(out,reinterpret_cast<const float*>(ptr),count);
	}
	else if (bps == 64)
	{
		if (use_swap)
			process_float_multi_swap(out,reinterpret_cast<const double*>(ptr),count);
		else
			process_float_multi(out,reinterpret_cast<const double*>(ptr),count);
	}
	else return false;

	set_sample_count(count/nch);
	set_srate(srate);
	set_channels(nch,p_channel_config);

	return true;
}

#if audio_sample_size == 64
bool audio_chunk::set_data_32(const float * src,UINT samples,UINT nch,UINT srate)
{
	unsigned size = samples * nch;
	audio_sample * out = check_data_size(size);
	if (out==0) {reset();return false;}
	dsp_util::convert_32_to_64(src,out,size);
	set_sample_count(samples);
	set_channels(nch);
	set_srate(srate);
	return true;
}
#else
bool audio_chunk::set_data_64(const double * src,UINT samples,UINT nch,UINT srate)
{
	unsigned size = samples * nch;
	audio_sample * out = check_data_size(size);
	if (out==0) {reset();return false;}
	dsp_util::convert_64_to_32(src,out,size);
	set_sample_count(samples);
	set_channels(nch);
	set_srate(srate);
	return true;
}
#endif

bool audio_chunk::is_valid()
{
	unsigned nch = get_channels();
	if (nch==0 || nch>256) return false;
	unsigned srate = get_srate();
	if (srate<1000 || srate>1000000) return false;
	unsigned samples = get_sample_count();
	if (samples==0 || samples >= 0x80000000 / (sizeof(audio_sample) * nch) ) return false;
	unsigned size = get_data_size();
	if (samples * nch > size) return false;
	if (!get_data()) return false;
	return true;
}


bool audio_chunk::pad_with_silence_ex(unsigned samples,unsigned hint_nch,unsigned hint_srate)
{
	if (is_empty())
	{
		if (hint_srate && hint_nch)
		{
			return set_data(0,samples,hint_nch,hint_srate);
		}
		else return false;
	}
	else
	{
		if (hint_srate && hint_srate != get_srate()) samples = MulDiv(samples,get_srate(),hint_srate);
		if (samples > get_sample_count())
		{
			unsigned old_size = get_sample_count() * get_channels();
			unsigned new_size = samples * get_channels();
			audio_sample * ptr = check_data_size(new_size);
			if (ptr)
			{
				mem_ops<audio_sample>::set(ptr + old_size,0,new_size - old_size);
				set_sample_count(samples);
				return true;
			}
			else return false;			
		}
		else return true;
	}
}

bool audio_chunk::pad_with_silence(unsigned samples)
{
	if (samples > get_sample_count())
	{
		unsigned old_size = get_sample_count() * get_channels();
		unsigned new_size = samples * get_channels();
		audio_sample * ptr = check_data_size(new_size);
		if (ptr)
		{
			mem_ops<audio_sample>::set(ptr + old_size,0,new_size - old_size);
			set_sample_count(samples);
			return true;
		}
		else return false;			
	}
	else return true;
}

bool audio_chunk::insert_silence_fromstart(unsigned samples)
{
	unsigned old_size = get_sample_count() * get_channels();
	unsigned delta = samples * get_channels();
	unsigned new_size = old_size + delta;
	audio_sample * ptr = check_data_size(new_size);
	if (ptr)
	{
		mem_ops<audio_sample>::move(ptr+delta,ptr,old_size);
		mem_ops<audio_sample>::set(ptr,0,delta);
		set_sample_count(get_sample_count() + samples);
		return true;
	}
	else return false;			
}

unsigned audio_chunk::skip_first_samples(unsigned samples_delta)
{
	unsigned samples_old = get_sample_count();
	if (samples_delta >= samples_old)
	{
		set_sample_count(0);
		set_data_size(0);
		return samples_old;
	}
	else
	{
		unsigned samples_new = samples_old - samples_delta;
		unsigned nch = get_channels();
		audio_sample * ptr = get_data();
		mem_ops<audio_sample>::move(ptr,ptr+nch*samples_delta,nch*samples_new);
		set_sample_count(samples_new);
		set_data_size(nch*samples_new);
		return samples_delta;
	}
}

void audio_chunk::set_channels(unsigned val)
{
	set_channels(val,g_guess_channel_config(val));
}

void audio_chunk_i::set_channels(unsigned val)
{
	set_channels(val,g_guess_channel_config(val));
}

audio_sample audio_chunk::get_peak(audio_sample peak) const
{
	unsigned num = get_data_length();
	const audio_sample * data = get_data();
	for(;num;num--)
	{
		audio_sample temp = (audio_sample)fabs(*(data++));
		if (temp>peak) peak = temp;
	}
	return peak;
}