#define audio_sample_size 32

#if audio_sample_size == 32
typedef float audio_sample;
#define audio_sample_asm dword
#elif audio_sample_size == 64
typedef double audio_sample;
#define audio_sample_asm qword
#else
#error wrong audio_sample_size
#endif

#define audio_sample_bytes (audio_sample_size/8)

namespace audio_math
{
	//! p_source/p_output can point to same buffer
	void SHARED_EXPORT scale(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_to_int16(const audio_sample * p_source,unsigned p_count,t_int16 * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_to_int32(const audio_sample * p_source,unsigned p_count,t_int32 * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT convert_to_int16_calculate_peak(const audio_sample * p_source,unsigned p_count,t_int16 * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_from_int16(const t_int16 * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale);
	void SHARED_EXPORT convert_from_int32(const t_int32 * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT convert_to_int32_calculate_peak(const audio_sample * p_source,unsigned p_count,t_int32 * p_output,audio_sample p_scale);
	audio_sample SHARED_EXPORT calculate_peak(const audio_sample * p_source,unsigned p_count);
}