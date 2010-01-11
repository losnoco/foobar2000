#include "shared.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#if 1
#define PF_FLOATING_POINT_PRECISION_ERRATA  0   
#define PF_FLOATING_POINT_EMULATED          1   
#define PF_COMPARE_EXCHANGE_DOUBLE          2   
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3   
#define PF_PPC_MOVEMEM_64BIT_OK             4   
#define PF_ALPHA_BYTE_INSTRUCTIONS          5   
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6   
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7   
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8   
#define PF_PAE_ENABLED                      9   
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10   


static BOOL wrap_IsProcessorFeaturePresent(DWORD code)
{
	typedef BOOL (WINAPI * t_IsProcessorFeaturePresent)(DWORD);
	t_IsProcessorFeaturePresent p_IsProcessorFeaturePresent = (t_IsProcessorFeaturePresent)GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsProcessorFeaturePresent");
	if (p_IsProcessorFeaturePresent==0) return FALSE;
	return p_IsProcessorFeaturePresent(code);
}

static bool detect_3dnow()
{
	return !!wrap_IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE);	
}
#endif

static const BOOL g_have_sse2 = wrap_IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);
static const BOOL g_have_sse = wrap_IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE);
static const BOOL g_have_3dnow = wrap_IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE);

static const float g_const_0 = 0.0;
static const t_uint32 g_const_fabs_andmask = 0x7FFFFFFF;

static audio_sample __calculate_peak(const audio_sample * p_src,unsigned p_num)
{
	audio_sample peak = 0;
	unsigned num = p_num;
	for(;num;num--)
	{
		audio_sample temp = (audio_sample)fabs(*(p_src++));
		if (temp>peak) peak = temp;
	}
	return peak;
}


__declspec(naked) static audio_sample __fastcall __calculate_peak_3dnow_4word(const audio_sample * p_src,unsigned p_num)
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

static audio_sample __calculate_peak_sse_8word(const audio_sample * p_src,unsigned p_num)
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

		_mm_storeu_ps(resultbuf,peak1);

		_mm_empty();
	}

	return pfc::max_t(pfc::max_t(resultbuf[0],resultbuf[1]),pfc::max_t(resultbuf[2],resultbuf[3]));
}

__declspec(naked) static void __fastcall __convert_to_16bit_sse_8word(const audio_sample * p_src,unsigned num,t_int16 * p_dst,float p_mul)
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

__declspec(naked) static void __fastcall __convert_to_32bit_3dnow_4word(const audio_sample * p_src,unsigned num,t_int32 * p_dst,float p_mul)
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

inline static t_int64 rint64(audio_sample val)
{
	t_int64 rv;
	_asm
	{
		fld val;
		fistp rv;
	}
	return rv;
}

static void __convert_to_32bit(const audio_sample * p_source,unsigned p_count,t_int32 * p_output,float p_scale)
{
	unsigned num = p_count;
	for(;num;--num)
	{
		t_int64 val = rint64( *(p_source++) * p_scale );
		if (val < -(t_int64)0x80000000) val = -(t_int64)0x80000000;
		else if (val > 0x7FFFFFFF) val = 0x7FFFFFFF;
		*(p_output++) = (t_int32) val;
	}
}


static const float g_const_32bit_max = (float) 0x7FFFFF80;

__declspec(naked) static void __fastcall __convert_to_32bit_sse_8word(const audio_sample * p_src,unsigned num,t_int32 * p_dst,float p_mul)
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

__declspec(naked) static void __fastcall __convert_to_32bit_sse2_8word(const audio_sample * p_src,unsigned num,t_int32 * p_dst,float p_mul)
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




__declspec(naked) static void __fastcall __convert_to_16bit_x87(const audio_sample * src,unsigned num,t_int16 * out,float p_mul)
{
	__asm
	{//ecx : src , edx : num,
		push edi
		push eax//for temporary storage space on stack
		test edx,edx
		mov edi,dword ptr [esp+8+4+0]//out
		jz l5
l1:
		fld audio_sample_asm ptr [ecx]
		fmul dword ptr [esp+8+4+4]
		add ecx,audio_sample_bytes
		fistp dword ptr [esp]
		mov eax,dword ptr [esp]
		cmp eax,0xFFFF8000
		jge l2
		mov eax,0xFFFF8000
		jmp l3
l2:
		cmp eax,0x00007FFF
		jle l4
		mov eax,0x00007FFF
l3:
//		clip detection was here
l4:
		dec edx
		mov word ptr [edi],ax
		lea edi,[edi+2]
		jnz l1
l5:
		pop eax
		pop edi
		ret 8
	}
}

inline static void __convert_from_int16(const t_int16 * p_source,unsigned p_count,audio_sample * p_output,float p_scale)
{
	unsigned num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

static void __convert_from_int16_sse2_4word(const t_int16 * p_source,unsigned p_count,audio_sample * p_output,float p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);

	for(;p_count;--p_count)
	{
		__m128 temp128;
		temp128 = _mm_cvtpi16_ps(*(const __m64*) p_source);
		temp128 = _mm_mul_ps(temp128, mul);
		_mm_storeu_ps(p_output,temp128);
		p_source += 4;
		p_output += 4;
	}

	_mm_empty();
}

static void __convert_from_int32_sse_4word(const t_int32 * p_source,unsigned p_count,audio_sample * p_output,float p_scale)
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

inline static void __convert_from_int32(const t_int32 * p_source,unsigned p_count,audio_sample * p_output,float p_scale)
{
	unsigned num = p_count;
	for(;num;num--)
		*(p_output++) = (audio_sample)*(p_source++) * p_scale;
}

inline static void __scale(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
{
	for(unsigned n=0;n<p_count;n++)
		p_output[n] = p_source[n] * p_scale;
}

inline static void __scale_sse_4word(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
{
	__m128 mul = _mm_load1_ps(&p_scale);
	for(;p_count;--p_count)
	{
		__m128 temp;
		temp = _mm_loadu_ps(p_source);
		temp = _mm_mul_ps(temp,mul);
		_mm_storeu_ps(p_output,temp);
		p_source += 4;
		p_output += 4;
	}
}

__declspec(naked) static void __fastcall __scale_3dnow_8word(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
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

inline static void __scale_sse_8word(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
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

	void SHARED_EXPORT scale(const audio_sample * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
	{
		if (g_have_3dnow)
		{
			__scale_3dnow_8word(p_source,p_count >> 3,p_output,p_scale);
			__scale(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),p_scale);
		}
		else if (g_have_sse)
		{
			__scale_sse_8word(p_source,p_count >> 3,p_output,p_scale);
			__scale(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),p_scale);
		}
		else
			__scale(p_source,p_count,p_output,p_scale);
	}
	
	void SHARED_EXPORT convert_to_int16(const audio_sample * p_source,unsigned p_count,t_int16 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x8000);
		if (g_have_sse)
		{
			__convert_to_16bit_sse_8word(p_source,p_count >> 3,p_output,scale);
			__convert_to_16bit_x87(p_source + (p_count & ~7),p_count & 7,p_output + (p_count & ~7),scale);
		}
		else
			__convert_to_16bit_x87(p_source,p_count,p_output,scale);
	}

	audio_sample SHARED_EXPORT convert_to_int16_calculate_peak(const audio_sample * p_source,unsigned p_count,t_int16 * p_output,audio_sample p_scale)
	{
		//todo?
		convert_to_int16(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int16(const t_int16 * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x8000 );
		if (g_have_sse2)
		{
			__convert_from_int16_sse2_4word(p_source,p_count >> 2,p_output,scale);
			__convert_from_int16(p_source + (p_count & ~3),p_count & 3,p_output + (p_count & ~3),scale);
		}
		else
			__convert_from_int16(p_source,p_count,p_output,scale);
	}

	void SHARED_EXPORT convert_to_int32(const audio_sample * p_source,unsigned p_count,t_int32 * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample)(p_scale * 0x80000000);
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
		{
			__convert_to_32bit(p_source,p_count,p_output,scale);
		}
	}

	audio_sample SHARED_EXPORT convert_to_int32_calculate_peak(const audio_sample * p_source,unsigned p_count,t_int32 * p_output,audio_sample p_scale)
	{
		convert_to_int32(p_source,p_count,p_output,p_scale);
		return p_scale * calculate_peak(p_source,p_count);
	}

	void SHARED_EXPORT convert_from_int32(const t_int32 * p_source,unsigned p_count,audio_sample * p_output,audio_sample p_scale)
	{
		audio_sample scale = (audio_sample) ( p_scale / (double) 0x80000000 );
		if (g_have_sse)
		{
			__convert_from_int32_sse_4word(p_source,p_count>>2,p_output,scale);
			__convert_from_int32(p_source + (p_count & ~3),p_count & 3,p_output + (p_count & ~3),scale);
		}
		else
			__convert_from_int32(p_source,p_count,p_output,scale);
	}


	audio_sample SHARED_EXPORT calculate_peak(const audio_sample * p_source,unsigned p_count)
	{
		if (g_have_3dnow)
		{
			return pfc::max_t(
				__calculate_peak_3dnow_4word(p_source,p_count >> 2),
				__calculate_peak(p_source + (p_count & ~3), p_count & 3)
				);
		}
		else if (g_have_sse)
		{
			return pfc::max_t(
				__calculate_peak_sse_8word(p_source,p_count >> 3),
				__calculate_peak(p_source + (p_count & ~7), p_count & 7)
			);
		}
		else
		{
			return __calculate_peak(p_source,p_count);
		}
	}

	void SHARED_EXPORT kill_denormal(audio_sample * p_buffer,unsigned p_count) {
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
	void SHARED_EXPORT add_offset(audio_sample * p_buffer,audio_sample p_delta,unsigned p_count) {
		for(unsigned n=0;n<p_count;n++) {
			p_buffer[n] += p_delta;
		}
	}

}
