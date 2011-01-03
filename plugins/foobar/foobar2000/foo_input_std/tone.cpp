#include "stdafx.h"
#include <math.h>

#if 0
// {E3EC4EDD-EF9E-4442-8AF9-A66C239AA76C}
static const GUID guid_cfg_tone_rate = 
{ 0xe3ec4edd, 0xef9e, 0x4442, { 0x8a, 0xf9, 0xa6, 0x6c, 0x23, 0x9a, 0xa7, 0x6c } };

// {5CE02B53-6560-4889-B25C-A3F17F6A127D}
static const GUID guid_cfg_tone_oversample = 
{ 0x5ce02b53, 0x6560, 0x4889, { 0xb2, 0x5c, 0xa3, 0xf1, 0x7f, 0x6a, 0x12, 0x7d } };


cfg_int cfg_tone_rate(guid_cfg_tone_rate,44100),cfg_tone_oversample(guid_cfg_tone_oversample,0);

#else

enum {
	cfg_tone_rate = 44100,
	cfg_tone_oversample = 0
};

#endif

#ifdef _M_IX86

__declspec(naked) static double generate_sine(audio_sample * out,unsigned count,double offset,double delta)
{
	__asm
	{
		mov eax,dword ptr [esp+4]//out
		mov ecx,dword ptr [esp+8]//count
		fld qword ptr [esp+20]//delta
		fld qword ptr [esp+12]//offset
		shr ecx,1
l1:
		fld st(0)
		fsin
		fstp audio_sample_asm ptr [eax]
		lea eax,[eax+audio_sample_bytes]
		fadd st(0),st(1)

		fld st(0)
		fsin
		fstp audio_sample_asm ptr [eax]
		lea eax,[eax+audio_sample_bytes]
		fadd st(0),st(1)

		loop l1

		fstp st(1)
		ret
	}
}

#else

static float generate_sine(audio_sample * out,unsigned count,double offset,double delta)
{
	while(count)
	{
		*(out++) = sin(offset);
		offset += delta;
		count--;
	}
	return offset;
}

#endif

class input_tone : public input
{
private:
	double offset;
	double freq;
	double length;
	unsigned rate;
	unsigned oversample;
	enum 
	{
		SAMPLES = 1024,
		OVERSAMPLE = 32,
	};
	audio_sample temp[SAMPLES];
public:

	virtual t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort) {return io_result_error_data;}

	inline static bool g_test_filename(const char * fn,const char * ext) 
	{
		return !stricmp_utf8_partial(fn,"tone://");
	}

	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		if (p_file.is_valid()) return io_result_error_data;
		const char * fn = p_location.get_path();
		if (!g_test_filename(fn,"")) return io_result_error_data;
		
		oversample = cfg_tone_oversample ? OVERSAMPLE : 1;
		rate = cfg_tone_rate;
		if (rate==0) return io_result_error_data;

		offset = 0;

		fn+=7;
		freq = strtod(fn,0);
		if (freq<=0) return io_result_error_data;
		while(*fn && *fn!=',') fn++;
		if (*fn==',')
		{
			fn++;
			length = strtod(fn,0);
		}
		else length = -1.0;

		if (freq>rate/2)
		{
			console::error("Selected sample rate is too low to properly generate this tone.");
			return io_result_error_data;
		}

		char title[128];
		sprintf(title,"Tone: %u Hz",(int)freq);
		p_info.meta_set("title",title);
		if (length>0.0) p_info.set_length(length);

		
		return io_result_success;
	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		double delta = 2*3.1415926535897932384626433832795 * freq / (double)(rate*oversample);
		double oversample_mul = 1.0 / (double)oversample;
	
		if (length==0.0) return io_result_eof;
	
		audio_sample * ptr = temp;
		unsigned n;
		if (oversample>1)
		{
			for(n=0;n<SAMPLES;n++)
			{
				double out = 0;
				unsigned s;
				for(s=0;s<oversample;s++)
				{
					out += sin(offset);
					offset += delta;
				}
				ptr[n] = (audio_sample)(out * oversample_mul);
			}
		}
		else
		{
			offset = generate_sine(ptr,SAMPLES,offset,delta);
		}

		unsigned samples = SAMPLES;
		if (length>0.0)
		{
			double length_delta = (double)SAMPLES / (double)rate;
			if (length>length_delta) length -= length_delta;
			else
			{
				samples = (int)(length * (double)rate);
				length = 0.0;
			}
		}
		
		if (samples==0) return io_result_eof;

		chunk->set_data(ptr,samples,1,rate);

		return io_result_success;
	}

	virtual t_io_result seek(double s,abort_callback & p_abort) {return io_result_error_generic;}
	virtual bool can_seek() {return false;}

	inline static bool g_needs_reader() {return false;}
	inline static bool g_is_our_content_type(const char*,const char*) {return false;}

	static GUID g_get_guid()
	{
		// {2B172814-B0CA-4f0b-8FCD-CB4F4E851D37}
		static const GUID guid = 
		{ 0x2b172814, 0xb0ca, 0x4f0b, { 0x8f, 0xcd, 0xcb, 0x4f, 0x4e, 0x85, 0x1d, 0x37 } };
		return guid;
	}

	static const char * g_get_name() {return "Tone generator";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

};

class input_silence : public input
{
private:
	double length;
	enum 
	{
		SAMPLES = 2048,
	};

	unsigned samplerate,channels;

	audio_sample temp[SAMPLES];
public:

	input_silence()
	{
		samplerate = 44100;
		channels = 2;
		memset(temp,0,sizeof(temp));
	}

	virtual t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort) {return io_result_error_data;}

	inline static bool g_test_filename(const char * fn,const char * ext) 
	{
		return !stricmp_utf8_partial(fn,"silence://");
	}

	
	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		if (p_file.is_valid()) return io_result_error_data;
		const char * fn = p_location.get_path();
		if (!g_test_filename(fn,"")) return io_result_error_data;
		
		fn+=10;
		length = strtod(fn,0);
		if (length<=0) return io_result_error_data;
		
		{
			const char * ptr = strchr(fn,',');
			if (ptr)
			{
				samplerate = atoi(ptr+1);
				
				ptr = strchr(ptr+1,',');
				if (ptr)
				{
					channels = atoi(ptr+1);
				}
			}
		}

		if (samplerate<1000 || samplerate>1000000 || channels<1 || channels>256) return io_result_error_data;

		p_info.meta_set("title","Silence");
		p_info.set_length(length);
		
		return io_result_success;

	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		if (length==0.0) return io_result_eof;
	
		audio_sample * ptr = temp;
		unsigned samples = tabsize(temp)/channels;
	
		double length_delta = (double)samples / (double)samplerate;
		if (length>length_delta) length -= length_delta;
		else
		{
			samples = (unsigned) (length * (double)samplerate);
			length = 0;
		}
		if (samples==0) return io_result_eof;
		chunk->set_data(ptr,samples,channels,samplerate);
		return io_result_success;
	}

	virtual t_io_result seek(double s,abort_callback & p_abort) {return io_result_error_generic;}
	virtual bool can_seek() {return false;}

	inline static bool g_needs_reader() {return false;}
	inline static bool g_is_our_content_type(const char*,const char*) {return false;}

	static GUID g_get_guid()
	{
		// {3DA14121-E93A-4fcf-8943-2D01637E9A60}
		static const GUID guid = 
		{ 0x3da14121, 0xe93a, 0x4fcf, { 0x89, 0x43, 0x2d, 0x1, 0x63, 0x7e, 0x9a, 0x60 } };
		return guid;
	}

	static const char * g_get_name() {return "Silence generator";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

static double log2(double x) {return log(x)/log(2.0);}

class input_sweep : public input
{
private:
	double offset;
	double startfreq,stopfreq;
	double length,length_div;
	double oi;
	unsigned position;
	unsigned length_samples;
	unsigned rate;
	unsigned oversample;
	unsigned last_freq;
	enum 
	{
		SAMPLES = 1024,
		OVERSAMPLE = 32,
	};
	audio_sample temp[SAMPLES];
public:

	virtual t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort) {return io_result_error_data;}

	inline static bool g_test_filename(const char * fn,const char * ext) 
	{
		return !stricmp_utf8_partial(fn,"sweep://");
	}

	
	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		if (p_file.is_valid()) return io_result_error_data;
		const char * fn = p_location.get_path();
		if (!g_test_filename(fn,"")) return io_result_error_data;
		
		oversample = cfg_tone_oversample ? OVERSAMPLE : 1;
		rate = cfg_tone_rate;
		if (rate==0) return io_result_error_data;

		offset = 0;
		position = 0;

		fn+=8;
		startfreq = strtod(fn,0);
		if (startfreq<=0) return io_result_error_data;
		while(*fn && *fn!='-') fn++;
		if (*fn==0) return io_result_error_data;
		fn++;
		stopfreq = strtod(fn,0);
		if (stopfreq<=0) return io_result_error_data;
		while(*fn && *fn!=',') fn++;
		if (*fn==0) return io_result_error_data;
		fn++;
		length = strtod(fn,0);
		if (length<=0) return io_result_error_data;

		oi = log2(stopfreq/startfreq);
		
		length_samples = (unsigned)(length * rate);
		length_div = 1.0 / (double)length_samples;
		last_freq = 0;

		if (startfreq>rate/2.0 || stopfreq>rate/2.0)
		{
			console::error("Selected sample rate is too low to properly generate this sweep.");
			return io_result_error_data;
		}

		char title[128];
		sprintf(title,"Sweep: %u-%u Hz",(unsigned)startfreq,(unsigned)stopfreq);
		p_info.meta_set("title",title);
		p_info.set_length(length);
		
		return io_result_success;

	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		double deltamul = 2*3.1415926535897932384626433832795 / (double)(rate*oversample);
		double oversample_div = 1.0 / (double)oversample;
	
		last_freq = (unsigned)((double)startfreq * pow(2.0,((double)position) * length_div * oi));

		audio_sample * ptr = temp;
		unsigned n;
		unsigned samples = length_samples - position;
		if (samples>SAMPLES) samples = SAMPLES;
		if (samples==0) return io_result_eof;
		for(n=0;n<samples;n++)
		{
			double out = 0;
			unsigned s;
			for(s=0;s<oversample;s++)
			{
				out += sin(offset);
				offset += (double)startfreq * pow(2.0,((double)position + (double)s * oversample_div) * length_div * oi) * deltamul;
			}
			ptr[n] = (audio_sample)(out * oversample_div);
			position++;
		}
		
		chunk->set_data(ptr,samples,1,rate);

		return io_result_success;
	}

	virtual t_io_result seek(double s,abort_callback & p_abort) {return io_result_error_generic;}
	virtual bool can_seek() {return false;}

	inline static bool g_needs_reader() {return false;}

	bool get_dynamic_info(file_info & out,double * timestamp_delta,bool * b_track_change)
	{
		*timestamp_delta = 0;
		*b_track_change = false;
		out.info_set_int("sweep_frequency",last_freq);
		return true;
	}

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}

	static GUID g_get_guid()
	{
		// {792F3967-B3D8-4360-8187-D9A4C6387E7F}
		static const GUID guid = 
		{ 0x792f3967, 0xb3d8, 0x4360, { 0x81, 0x87, 0xd9, 0xa4, 0xc6, 0x38, 0x7e, 0x7f } };
		return guid;
	}

	static const char * g_get_name() {return "Sweep generator";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

static input_factory_t<input_tone> g_input_tone_factory;
static input_factory_t<input_silence> g_input_silence_factory;
static input_factory_t<input_sweep> g_input_sweep_factory;