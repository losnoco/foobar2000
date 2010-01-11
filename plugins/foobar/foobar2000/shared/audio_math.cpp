#include "shared.h"
#include <xmmintrin.h>
#include <emmintrin.h>


#ifdef _M_IX86
enum {
	CPU_HAVE_3DNOW		= 1 << 0,
	CPU_HAVE_3DNOW_EX	= 1 << 1,
	CPU_HAVE_SSE		= 1 << 2,
	CPU_HAVE_SSE2		= 1 << 3,
	CPU_HAVE_SSE3		= 1 << 4,
};

static bool query_cpu_feature_set(unsigned p_value) {
	__try {
		if (p_value & (CPU_HAVE_SSE | CPU_HAVE_SSE2 | CPU_HAVE_SSE2)) {
			int buffer[4];
			__cpuid(buffer,1);
			if (p_value & CPU_HAVE_SSE) {
				if ((buffer[3]&(1<<25)) == 0) return false;
			}
			if (p_value & CPU_HAVE_SSE2) {
				if ((buffer[3]&(1<<26)) == 0) return false;
			}
			if (p_value & CPU_HAVE_SSE3) {
				if ((buffer[2]&(1<<0)) == 0) return false;
			}
		}

#ifdef _M_IX86
		if (p_value & (CPU_HAVE_3DNOW_EX | CPU_HAVE_3DNOW)) {
			int buffer_amd[4];
			__cpuid(buffer_amd,0x80000000);
			if ((unsigned)buffer_amd[0] < 0x80000001) return false;
			__cpuid(buffer_amd,0x80000001);
			
			if (p_value & CPU_HAVE_3DNOW) {
				if ((buffer_amd[3]&(1<<22)) == 0) return false;
			}
			if (p_value & CPU_HAVE_3DNOW_EX) {
				if ((buffer_amd[3]&(1<<30)) == 0) return false;
			}
		}
#endif
		return true;
	} __except(1) {
		return false;
	}
}
static const BOOL g_have_sse2 = query_cpu_feature_set(CPU_HAVE_SSE2);
static const BOOL g_have_sse = query_cpu_feature_set(CPU_HAVE_SSE);
static const BOOL g_have_3dnow = query_cpu_feature_set(CPU_HAVE_3DNOW);

#elif defined(_M_X64)

enum {g_have_sse2 = true, g_have_sse = true, g_have_3dnow = false};

#else

enum {g_have_sse2 = false, g_have_sse = false, g_have_3dnow = false};

#endif


static const float g_const_0 = 0.0;
static const t_uint32 g_const_fabs_andmask = 0x7FFFFFFF;
__declspec(align(16)) static const t_uint32 g_const_null_int32_vector[4] = {0,0,0,0};
__declspec(align(16)) static const t_uint32 g_const_8000_int32_vector[4] = {0x8000,0x8000,0x8000,0x8000};
__declspec(align(16)) static const t_uint16 g_const_8000_int16_vector[8] = {0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000};

static audio_sample __calculate_peak(const audio_sample * p_src,t_size p_num)
{
	audio_sample peak = 0;
	t_size num = p_num;
	for(;num;num--)
	{
		audio_sample temp = (audio_sample)fabs(*(p_src++));
		if (temp>peak) peak = temp;
	}
	return peak;
}


#ifdef _M_IX86
__declspec(naked) static audio_sample __fastcall __calculate_peak_3dnow_4word(const audio_sample * p_src,t_size p_num)
{
	__asm
	{//ecx == ptr, edx == num
		movd mm7, g_const_fabs_andmask
		pxor mm2,mm2
		test edx,edx
		punpckldq mm7,mm7
		pxor mm3,mm3
		jz l2
l1:
		movq mm0, [ecx]
		movq mm1, [ecx+8]
		add ecx,16
		pand mm0,mm7
		pand mm1,mm7		
		dec edx
		pfmax mm2,mm0
		pfmax mm3,mm1		
		jnz l1
l2:
		pfmax mm2,mm3
		movq mm4,mm2
		punpckhdq mm2,mm2
		pfmax mm2,mm4
		push eax
		movd [esp],mm2
		femms
		fld dword ptr [esp]
		pop eax
		ret
	}
}
#endif
static audio_sample __calculate_peak_sse_8word(const audio_sample * p_src,t_size p_num)
{
	audio_sample resultbuf[4];
	{
		__m128 peak1, peak2, andmask;
		
		peak1 = _mm_load1_ps(&g_const_0);
		peak2 = peak1;
		andmask = _mm_load1_ps( (const float*)&g_const_fabs_andmask );

		for(;p_num;--p_num)
		{
			__m128 temp1,temp2;
			temp1 = _mm_loadu_ps(p_src);
			temp2 = _mm_loadu_ps(p_src+4);
			temp1 = _mm_and_ps(temp1,andmask);
			temp2 = _mm_and_ps(temp2,andmask);
			peak1 = _mm_max_ps(peak1, temp1);
			peak2 = _mm_max_ps(peak2, temp2);
			p_src += 8;
		}

		peak1 = _mm_max_ps(peak1, peak2);

		peak2 = _mm_movehl_ps(peak2, peak1);

		peak1 = _mm_max_ps(peak1, peak2);

		_mm_storeu_ps(resultbuf,peak1);
	}

	return pfc::max_t(resultbuf[0],resultbuf[1]);
}

inline static audio_sample __calculate_peak_sse(const audio_sample * p_src,t_size p_num)
{
	float peak = 0;

	while(!pfc::is_ptr_aligned_t<16>(p_src) && p_num) {
		audio_sample temp = (audio_sample)fabs(*(p_src++));
		if (temp>peak) peak = temp;
		p_num--;
	}

	{
		audio_sample resultbuf[4];

		__m128 peak1, peak2, andmask;
		
		peak1 = _mm_load1_ps(&g_const_0);
		peak2 = peak1;
		andmask = _mm_load1_ps( (const float*)&g_const_fabs_andmask );

		for(t_size loop = p_num >> 3;loop;--loop)
		{
			__m128 temp1,temp2;
			temp1 = _mm_and_ps(andmask,*(__m128*)(p_src  ));
			temp2 = _mm_and_ps(andmask,*(__m128*)(p_src+4));
			peak1 = _mm_max_ps(peak1, temp1);
			peak2 = _mm_max_ps(peak2, temp2);
			p_src += 8;
		}

		p_num &= 7;

		peak1 = _mm_max_ps(peak1, peak2);

		peak2 = _mm_movehl_ps(peak2, peak1);

		peak1 = _mm_max_ps(peak1, peak2);

		_mm_storeu_ps(resultbuf,peak1);

		peak = pfc::max_t(peak,pfc::max_t(resultbuf[0],resultbuf[1]));
	}

	

	while(p_num) {
		audio_sample temp = (audio_sample)fabs(*(p_src++));
		if (temp>peak) peak = temp;
		p_num--;
	}
	return peak;
}

#ifdef _M_X64
inline static void __convert_to_16bit_sse2_8word(const audio_sample * p_source,t_size p_count,t_int16 * p_output,float p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	for(;p_count;--p_count)
	{
		__m128 temp1,temp2; __m128i itemp1, itemp2;
		temp1 = _mm_loadu_ps(p_source);
		temp2 = _mm_loadu_ps(p_source+4);
		temp1 = _mm_mul_ps(temp1,mul);
		temp2 = _mm_mul_ps(temp2,mul);
		p_source += 8;
		itemp1 = _mm_cvtps_epi32(temp1);
		itemp2 = _mm_cvtps_epi32(temp2);
		_mm_storeu_si128( (__m128i*)p_output, _mm_packs_epi32(itemp1, itemp2) );
		p_output += 8;
	}
}

#endif

#ifdef _M_IX86
__declspec(naked) static void __fastcall __convert_to_16bit_sse_8word(const audio_sample * p_src,t_size num,t_int16 * p_dst,float p_mul)
{//p_src == ecx, num == edx
	__asm
	{
		movss       xmm2,dword ptr [esp+8] 
		mov         eax,dword ptr [esp+4] 
		shufps      xmm2,xmm2,0 
		test        edx,edx 
		je          label2
label1:					
		movups      xmm0,xmmword ptr [ecx] 
		movups      xmm1,xmmword ptr [ecx+10h] 
		
		mulps       xmm0,xmm2 
		mulps       xmm1,xmm2 
		
		cvtps2pi    mm0,xmm0 
		cvtps2pi    mm2,xmm1

		movhlps     xmm0,xmm0 
		movhlps     xmm1,xmm1

		lea			ecx, [ecx+0x20]

		cvtps2pi    mm1,xmm0 
		cvtps2pi    mm3,xmm1

		packssdw    mm0,mm1
		packssdw    mm2,mm3
		movq        mmword ptr [eax],mm0 
		movq        mmword ptr [eax+8],mm2
		dec			edx
		
		lea			eax, [eax+0x10]
		jne         label1
label2:
		emms
		ret         8    
	}
}

__declspec(naked) static void __fastcall __convert_to_32bit_3dnow_4word(const audio_sample * p_src,t_size num,t_int32 * p_dst,float p_mul)
{//p_src == ecx, num == edx
	__asm
	{
		mov         eax,dword ptr [esp+4] 
		movd		mm7, dword ptr [esp+8]
		test        edx,edx 
		punpckldq	mm7,mm7
		je          label2
label1:					

		movq		mm0, [ecx]
		movq		mm1, [ecx+8]
		pfmul		mm0, mm7
		pfmul		mm1, mm7
		lea			ecx, [ecx+0x10]
		dec			edx
		pf2id		mm0, mm0
		pf2id		mm1, mm1
		movq		[eax], mm0
		movq		[eax+8], mm1
		
		lea			eax, [eax+0x10]
		jne         label1
label2:
		femms
		ret         8
	}
}
#endif

static void __convert_to_32bit(const audio_sample * p_source,t_size p_count,t_int32 * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;--num)
	{
		t_int64 val = audio_math::rint64( *(p_source++) * p_scale );
		if (val < -(t_int64)0x80000000) val = -(t_int64)0x80000000;
		else if (val > 0x7FFFFFFF) val = 0x7FFFFFFF;
		*(p_output++) = (t_int32) val;
	}
}


#ifdef _M_IX86
static const float g_const_32bit_max = (float) 0x7FFFFF80;

__declspec(naked) static void __fastcall __convert_to_32bit_sse_8word(const audio_sample * p_src,t_size num,t_int32 * p_dst,float p_mul)
{//p_src == ecx, num == edx
	__asm
	{
		movss       xmm2,dword ptr [esp+8] 
//		movss		xmm5,g_const_32bit_min
		movss		xmm6,g_const_32bit_max
		mov         eax,dword ptr [esp+4]
		shufps      xmm2,xmm2,0
//		shufps      xmm5,xmm5,0
		shufps      xmm6,xmm6,0
		test        edx,edx
		je          label2
label1:					
		movups      xmm0,xmmword ptr [ecx] 
		movups      xmm1,xmmword ptr [ecx+10h] 
		
		mulps       xmm0,xmm2
		mulps       xmm1,xmm2
//		maxps		xmm0,xmm5
//		maxps		xmm1,xmm5
		minps		xmm0,xmm6
		minps		xmm1,xmm6
		
		lea			ecx, [ecx+0x20]

		cvtps2pi    mm0,xmm0 
		cvtps2pi    mm2,xmm1

		movhlps     xmm0,xmm0 
		movhlps     xmm1,xmm1

		cvtps2pi    mm1,xmm0 
		cvtps2pi    mm3,xmm1

		dec			edx

		movq        mmword ptr [eax],mm0 
		movq        mmword ptr [eax+8],mm1

		movq        mmword ptr [eax+16],mm2
		movq        mmword ptr [eax+24],mm3
		
		
		lea			eax, [eax+0x20]
		jne         label1
label2:
		emms
		ret         8    
	}
}

__declspec(naked) static void __fastcall __convert_to_32bit_sse2_8word(const audio_sample * p_src,t_size num,t_int32 * p_dst,float p_mul)
{//p_src == ecx, num == edx
	__asm
	{
		movss       xmm2,dword ptr [esp+8] 
//		movss		xmm5,g_const_32bit_min
		movss		xmm6,g_const_32bit_max
		mov         eax,dword ptr [esp+4]
		shufps      xmm2,xmm2,0
//		shufps      xmm5,xmm5,0
		shufps      xmm6,xmm6,0
		test        edx,edx
		je          label2
label1:					
		movups      xmm0,xmmword ptr [ecx] 
		movups      xmm1,xmmword ptr [ecx+10h] 
		
		mulps       xmm0,xmm2
		mulps       xmm1,xmm2
//		maxps		xmm0,xmm5
//		maxps		xmm1,xmm5
		minps		xmm0,xmm6
		minps		xmm1,xmm6
		
		lea			ecx, [ecx+0x20]

		cvtps2dq	xmm0,xmm0
		cvtps2dq	xmm1,xmm1

		dec			edx
		movups		[eax],xmm0
		movups		[eax+16],xmm1

		lea			eax, [eax+0x20]
		jne         label1
label2:
		ret         8    
	}
}
#endif

inline static void __convert_to_16bit(const audio_sample * p_source,t_size p_count,t_int16 * p_output,float p_scale) {
	for(t_size n=0;n<p_count;n++) {
		*(p_output++) = (t_int16) pfc::clip_t(audio_math::rint32(*(p_source++)*p_scale),-0x8000,0x7FFF);
	}
}

inline static void __convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

static void __convert_from_int16_sse2_8word(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	__m128i nulls = _mm_load_si128((__m128i*)g_const_null_int32_vector);
	__m128i delta1 = _mm_load_si128((__m128i*)g_const_8000_int16_vector);
	__m128i delta2 = _mm_load_si128((__m128i*)g_const_8000_int32_vector);

	for(;p_count;--p_count) {
		__m128i source, temp1, temp2; __m128 float1, float2;
		source = _mm_loadu_si128((__m128i*)p_source);
		source = _mm_xor_si128(source,delta1);
		temp1 = _mm_unpacklo_epi16(source,nulls);
		temp2 = _mm_unpackhi_epi16(source,nulls);
		temp1 = _mm_sub_epi32(temp1,delta2);
		temp2 = _mm_sub_epi32(temp2,delta2);
		p_source += 8;
		float1 = _mm_cvtepi32_ps(temp1);
		float2 = _mm_cvtepi32_ps(temp2);
		float1 = _mm_mul_ps(float1,mul);
		float2 = _mm_mul_ps(float2,mul);
		_mm_storeu_ps(p_output,float1);
		_mm_storeu_ps(p_output+4,float2);
		p_output += 8;
	}
}

static void __convert_from_int16_sse2(const t_int16 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	while(!pfc::is_ptr_aligned_t<16>(p_output) && p_count) {
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
		p_count--;
	}

	{
		__m128 mul = _mm_load1_ps(&p_scale);
		__m128i nulls = _mm_load_si128((__m128i*)g_const_null_int32_vector);
		__m128i delta1 = _mm_load_si128((__m128i*)g_const_8000_int16_vector);
		__m128i delta2 = _mm_load_si128((__m128i*)g_const_8000_int32_vector);

		for(t_size loop = p_count >> 3;loop;--loop) {
			__m128i source, temp1, temp2; __m128 float1, float2;
			source = _mm_loadu_si128((__m128i*)p_source);
			source = _mm_xor_si128(source,delta1);
			temp1 = _mm_unpacklo_epi16(source,nulls);
			temp2 = _mm_unpackhi_epi16(source,nulls);
			temp1 = _mm_sub_epi32(temp1,delta2);
			temp2 = _mm_sub_epi32(temp2,delta2);
			p_source += 8;
			float1 = _mm_cvtepi32_ps(temp1);
			float2 = _mm_cvtepi32_ps(temp2);
			float1 = _mm_mul_ps(float1,mul);
			float2 = _mm_mul_ps(float2,mul);
			_mm_store_ps(p_output,float1);
			_mm_store_ps(p_output+4,float2);
			p_output += 8;
		}
		
		p_count &= 7;
	}

	while(p_count) {
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
		p_count--;
	}
}

static void __convert_from_int32_sse_4word(const t_int32 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	for(;p_count;--p_count)
	{
		__m128 temp1,temp2;
		temp1 = _mm_cvtpi32_ps(temp1, *(const __m64*)(p_source  ));
		temp2 = _mm_cvtpi32_ps(temp2, *(const __m64*)(p_source+2));
		temp1 = _mm_mul_ps(temp1, mul);
		temp2 = _mm_mul_ps(temp2, mul);
		_mm_storel_pi((__m64*)(p_output  ),temp1);
		_mm_storel_pi((__m64*)(p_output+2),temp2);
		p_source += 4;
		p_output += 4;
	}
	_mm_empty();
}

static void __convert_from_int32_sse2_8word(const t_int32 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	for(;p_count;--p_count)
	{
		__m128i itemp1,itemp2; __m128 temp1, temp2;
		itemp1 = _mm_loadu_si128((__m128i*)p_source    );
		itemp2 = _mm_loadu_si128((__m128i*)p_source + 1);
		temp1 = _mm_cvtepi32_ps(itemp1);
		temp2 = _mm_cvtepi32_ps(itemp2);
		p_source += 8;
		temp1 = _mm_mul_ps(temp1, mul);
		temp2 = _mm_mul_ps(temp2, mul);
		_mm_storeu_ps(p_output  ,temp1);
		_mm_storeu_ps(p_output+4,temp2);
		
		p_output += 8;
	}
}

inline static void __convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,float p_scale)
{
	t_size num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

inline static void __scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
{
	for(t_size n=0;n<p_count;n++)
		p_output[n] = p_source[n] * p_scale;
}

#ifdef _M_IX86
__declspec(naked) static void __fastcall __scale_3dnow_8word(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
{
	__asm
	{
		movd mm7,[esp+8]
		test edx,edx
		mov eax,[esp+4]
		punpckldq	mm7,mm7
		jz label2
label1:
		movq mm0, [ecx]
		movq mm1, mm7
		movq mm2, [ecx+16]
		movq mm3, mm7
		pfmul mm0, mm7
		pfmul mm1, [ecx+8]
		pfmul mm2, mm7
		pfmul mm3, [ecx+24]
		add ecx, 32
		movq [eax], mm0
		movq [eax+8], mm1
		dec edx
		movq [eax+16], mm2
		movq [eax+24], mm3
		lea eax, [eax+32]
		jne label1
label2:
		femms
		ret 8
	}
}
#endif
inline static void __scale_sse_8word(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	for(;p_count;--p_count)
	{
		__m128 temp1,temp2;
		temp1 = _mm_loadu_ps(p_source);
		temp2 = _mm_loadu_ps(p_source+4);
		temp1 = _mm_mul_ps(temp1,mul);
		temp2 = _mm_mul_ps(temp2,mul);
		p_source += 8;
		_mm_storeu_ps(p_output,temp1);
		_mm_storeu_ps(p_output+4,temp2);
		p_output += 8;
	}
}


namespace audio_math {

	void SHARED_EXPORT scale(const audio_sample * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
#ifdef _M_IX86
		if (g_have_3dnow)
		{
			__scale_3dnow_8word(p_source,p_count >> 3,p_output,p_scale);
			__scale(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),p_scale);
		} else if (g_have_sse)
		{
			__scale_sse_8word(p_source,p_count >> 3,p_output,p_scale);
			__scale(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),p_scale);
		}
		else
			__scale(p_source,p_count,p_output,p_scale);
#elif defined(_M_X64)
		__scale_sse_8word(p_source,p_count >> 3,p_output,p_scale);
		__scale(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),p_scale);
#else
		__scale(p_source,p_count,p_output,p_scale);
#endif
	}
	
	void SHARED_EXPORT convert_to_int16(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x8000);
#ifdef _M_IX86
		if (g_have_sse) {
			__convert_to_16bit_sse_8word(p_source,p_count >> 3,p_output,scale);
			__convert_to_16bit(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),scale);
		} else {
			__convert_to_16bit(p_source,p_count,p_output,scale);
		}
#elif defined(_M_X64)
		__convert_to_16bit_sse2_8word(p_source,p_count >> 3,p_output,scale);
		__convert_to_16bit(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),scale);
#else
		__convert_to_16bit(p_source,p_count,p_output,scale);
#endif
	}

	audio_sample SHARED_EXPORT convert_to_int16_calculate_peak(const audio_sample * p_source,t_size p_count,t_int16 * p_output,audio_sample p_scale)
	{
		//todo?
		convert_to_int16(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int16(const t_int16 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x8000 );
#ifdef _M_IX86
		if (g_have_sse2) {
			__convert_from_int16_sse2(p_source,p_count,p_output,scale);
		} else {
			__convert_from_int16(p_source,p_count,p_output,scale);
		}
#elif defined(_M_X64)
		__convert_from_int16_sse2(p_source,p_count,p_output,scale);
#else
		__convert_from_int16(p_source,p_count,p_output,scale);
#endif
	}

	void SHARED_EXPORT convert_to_int32(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x80000000);
#ifdef _M_IX86
		if (g_have_3dnow)
		{
			__convert_to_32bit_3dnow_4word(p_source,p_count >> 2,p_output,scale);
			__convert_to_32bit(p_source + (p_count & ~3), p_count & 3, p_output + (p_count & ~3),scale);
		}
		else if (g_have_sse2)
		{
			__convert_to_32bit_sse2_8word(p_source,p_count >> 3,p_output,scale);
			__convert_to_32bit(p_source + (p_count & ~7), p_count & 7, p_output + (p_count & ~7),scale);
		}
		else if (g_have_sse)
		{
			__convert_to_32bit_sse_8word(p_source,p_count >> 3,p_output,scale);
			__convert_to_32bit(p_source + (p_count & ~7), p_count & 7, p_output + (p_count & ~7),scale);
		}
		else 
#endif
		{
			__convert_to_32bit(p_source,p_count,p_output,scale);
		}
	}

	audio_sample SHARED_EXPORT convert_to_int32_calculate_peak(const audio_sample * p_source,t_size p_count,t_int32 * p_output,audio_sample p_scale)
	{
		convert_to_int32(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int32(const t_int32 * p_source,t_size p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x80000000 );
#ifdef _M_IX86
		if (g_have_sse) {
			__convert_from_int32_sse_4word(p_source,p_count>>2,p_output,scale);
			__convert_from_int32(p_source + (p_count & ~3),p_count & 3,p_output + (p_count & ~3),scale);
		} else {
			__convert_from_int32(p_source,p_count,p_output,scale);
		}
#elif defined(_M_X64)
		__convert_from_int32_sse2_8word(p_source,p_count>>3,p_output,scale);
		__convert_from_int32(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),scale);
#else
		__convert_from_int32(p_source,p_count,p_output,scale);
#endif
	}


	audio_sample SHARED_EXPORT calculate_peak(const audio_sample * p_source,t_size p_count)
	{
#ifdef _M_IX86
		if (g_have_sse) {
			return __calculate_peak_sse(p_source,p_count);
		} else if (g_have_3dnow) {
			return pfc::max_t(
				__calculate_peak_3dnow_4word(p_source,p_count >> 2),
				__calculate_peak(p_source + (p_count & ~3), p_count & 3)
				);
		} else {
			return __calculate_peak(p_source,p_count);
		}
#elif defined(_M_X64)
		return __calculate_peak_sse(p_source,p_count);
#else
		return __calculate_peak(p_source,p_count);
#endif

	}

	void SHARED_EXPORT kill_denormal(audio_sample * p_buffer,t_size p_count) {
#if audio_sample_size == 32
		t_uint32 * ptr = reinterpret_cast<t_uint32*>(p_buffer);
		for(;p_count;p_count--)
		{
			t_uint32 t = *ptr;
			if ((t & 0x007FFFFF) && !(t & 0x7F800000)) *ptr=0;
			ptr++;
		}
#elif audio_sample_size == 64
		t_uint64 * ptr = reinterpret_cast<t_uint64*>(p_buffer);
		for(;p_count;p_count--)
		{
			t_uint64 t = *ptr;
			if ((t & 0x000FFFFFFFFFFFFF) && !(t & 0x7FF0000000000000)) *ptr=0;
			ptr++;
		}
#else
#error unsupported
#endif
	}

	// to be optimized?
	void SHARED_EXPORT add_offset(audio_sample * p_buffer,audio_sample p_delta,t_size p_count) {
		for(t_size n=0;n<p_count;n++) {
			p_buffer[n] += p_delta;
		}
	}

}
