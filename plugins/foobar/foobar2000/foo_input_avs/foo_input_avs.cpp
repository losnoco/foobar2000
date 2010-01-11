#include <foobar2000.h>

//#pragma pack
//#include "internal.h"
#include "avisynth.h"
//#include <windows.h>

/*
enum {
	raw_bits_per_sample = 16,
	raw_channels = 2,
	raw_sample_rate = 44100,

	raw_bytes_per_sample = raw_bits_per_sample / 8,
	raw_total_sample_width = raw_bytes_per_sample * raw_channels,
};
*/

// No inheritance. Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
class input_avs {

private:
	PClip m_clip;
	AVSValue* m_res;
	IScriptEnvironment* m_env;
	HMODULE m_dll;
	VideoInfo m_inf;
	long m_pos;
	char * m_script;


	void cleanUp()
	{
		m_clip = NULL;
		m_pos = 0;

		memset(&m_inf, 0, sizeof(m_inf));

		if(m_script)
		{
			free(m_script);
			m_script = NULL;
		}

		if(m_res)
		{
			delete m_res;
			m_res = NULL;
		}

		if(m_env)
		{
			delete m_env;
			m_env = NULL;
		}

		if(m_dll)
		{
			FreeLibrary(m_dll);
			m_dll = NULL;
		}
	}



public:

	input_avs()
	{
		m_script =NULL;
		m_res = NULL;
		m_clip = NULL;
		m_env = NULL;
		m_dll = NULL;
	}



	~input_avs()
	{
		cleanUp();
	}

	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) 
	{
		cleanUp();

		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.

		m_file = p_filehint;//p_filehint may be null, hence next line
		input_open_file_helper(m_file,p_path,p_reason,p_abort);//if m_file is null, opens file with appropriate privelages for our operation (read/write for writing tags, read-only otherwise).
		t_filesize scriptSize = m_file->get_size(p_abort);
		m_script = (char *)malloc(scriptSize + 16);
		memset(m_script, 0, scriptSize + 16);
		m_file->read(m_script, scriptSize, p_abort);
		//*(m_buffer.get_ptr()+scriptSize+1)=NULL;

		m_dll = LoadLibrary(L"avisynth.dll");
		if(!m_dll)  throw exception_io_data("Cannot load avisynth.dll");

		IScriptEnvironment* (__stdcall * CreateScriptEnvironment)(int version) = (IScriptEnvironment*(__stdcall *)(int)) GetProcAddress(m_dll, "CreateScriptEnvironment");
		if(!CreateScriptEnvironment) throw exception_io_data("Cannot load CreateScriptEnvironment");

		m_env = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION);
		if (m_env == NULL) throw exception_io_data("Required Avisynth 2.5");

		try
		{
			char * scr = m_script;
			AVSValue arg(scr);
			AVSValue res = m_env->Invoke("Eval", AVSValue(&arg, 1));
			m_clip = res.AsClip();
			m_inf = m_clip->GetVideoInfo();
			if(!m_inf.HasAudio())
				throw exception_io_unsupported_format("Avisynth script does not return any audio");

			if(m_inf.sample_type == SAMPLE_FLOAT)
			{
				res = m_env->Invoke("ConvertAudioTo24Bit", AVSValue(&res, 1));
				m_clip = res.AsClip();
				m_inf = m_clip->GetVideoInfo();
			}
		}
		catch(AvisynthError err) 
		{
			throw exception_io_data(err.msg);
		}

		m_pos = 0;

	}

	void get_info(file_info & p_info,abort_callback & p_abort) 
	{
		p_info.set_length(audio_math::samples_to_time(m_inf.num_audio_samples, m_inf.audio_samples_per_second));
		//note that the values below should be based on contents of the file itself, NOT on user-configurable variables for example. To report info that changes independently from file contents, use get_dynamic_info/get_dynamic_info_track instead.
		p_info.info_set_int("samplerate",m_inf.audio_samples_per_second);
		p_info.info_set_int("channels",m_inf.nchannels);
		p_info.info_set_int("bitspersample",m_inf.BytesPerChannelSample()*8);
		p_info.info_set("encoding","lossless");
		p_info.meta_add("AviSynth script", m_script);
		int bitrate = ( 8*m_inf.BytesPerAudioSample()*m_inf.audio_samples_per_second + 500);
		bitrate /= 1000;
		p_info.info_set_bitrate(  bitrate);
	}

	t_filestats get_file_stats(abort_callback & p_abort) {return m_file->get_stats(p_abort);}

	void decode_initialize(unsigned p_flags,abort_callback & p_abort) 
	{
		m_pos = 0;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) 
	{
		int	deltaread = max(min( m_inf.num_audio_samples - m_pos , (long)1024),0);
		
		if(0 == deltaread) return false;

		int bytes = deltaread * m_inf.BytesPerAudioSample();
		
		try 
		{
			m_buffer.set_size(bytes);
			m_clip->GetAudio(m_buffer.get_ptr(),m_pos, deltaread, m_env);
			m_pos += deltaread;
		}
		catch(AvisynthError err) 
		{
			throw exception_io_data(err.msg);
		}

		p_chunk.set_data_fixedpoint(m_buffer.get_ptr(), bytes ,m_inf.audio_samples_per_second,m_inf.nchannels,m_inf.BytesPerChannelSample()*8,audio_chunk::g_guess_channel_config(m_inf.nchannels));
		
		return true;
	}
	void decode_seek(double p_seconds,abort_callback & p_abort) 
	{
		// IMPORTANT: convert time to sample offset with proper rounding! audio_math::time_to_samples does this properly for you.
		m_pos = min( audio_math::time_to_samples(p_seconds,m_inf.audio_samples_per_second), m_inf.num_audio_samples);
	}
	bool decode_can_seek() {return true;}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {return false;}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {return false;}
	void decode_on_idle(abort_callback & p_abort) {m_file->on_idle(p_abort);}

	void retag(const file_info & p_info,abort_callback & p_abort) {throw exception_io_unsupported_format();}
	
	static bool g_is_our_content_type(const char * p_content_type) {return stricmp_utf8(p_content_type,"text/avisynth") == 0;}
	static bool g_is_our_path(const char * p_path,const char * p_extension) {return stricmp_utf8(p_extension,"avs") == 0;}
public:
	service_ptr_t<file> m_file;
	pfc::array_t<t_uint8> m_buffer;
};

static input_singletrack_factory_t<input_avs> g_input_avs_factory;


DECLARE_COMPONENT_VERSION("AVS input","0.1","written by Dmitry Alexandrov aka dimzon\ndimzon541@gmail.com");
DECLARE_FILE_TYPE("AviSynth files","*.AVS");