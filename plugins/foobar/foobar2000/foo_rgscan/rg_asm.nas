BITS 32

segment .text
align 32

global _filter2_x87

_filter2_x87:
		push esi
		push edi
		mov eax, [esp+8+4] ;eax == nsamples
		mov esi, [esp+8+4+4] ;esi == a
		mov edi, [esp+8+4+8] ;edi == b
		fld dword [edi]
		fld dword [edi+4]
		fld dword [edi+8]
		fld dword [esi+4]
		fld dword [esi+8]	;a[2] a[1] b[2] b[1] b[0]
		fld dword [edx-4]	;output[i-1] a[2] a[1] b[2] b[1] b[0]
		test eax,eax
		jz _filter2_x87_l1
_filter2_x87_l0:
		fld dword [ecx]
		fmul st0,st6	;b[0]
		fld dword [ecx-4]
		fmul st0,st6	;b[1]
		faddp st1,st0
		fld dword [ecx-8]
		fmul st0,st5	;b[2]
		faddp st1,st0
		fxch st0,st1
		fmul st0,st3 ;output[i-1] sum a[2] a[1] b[2] b[1] b[0]
		fsubp st1,st0
		fld dword [edx-8]
		fmul st0,st2
		fsubp st1,st0
		fst dword [edx]
		lea ecx,[ecx+4]
		dec eax
		lea edx,[edx+4]
		jnz _filter2_x87_l0
_filter2_x87_l1:
		fstp st0
		fstp st0
		fstp st0
		fstp st0
		fstp st0
		fstp st0

		pop edi
		pop esi
		ret 12


global _filter10_3dnow
_filter10_3dnow:

	;ecx == input, edx == output
		push esi
		push edi
		mov eax, [esp+8+4] ;eax == nsamples
		mov esi, [esp+8+4+4] ;esi == a
		mov edi, [esp+8+4+8] ;edi == b
		test eax,eax
		movd mm7, dword [edi]
		movq mm6, qword [edi+4]
		movq mm5, qword [edi+12]
		movq mm4, qword [edi+20]
		jz _filter10_3dnow_l1
_filter10_3dnow_l0:		
		movd mm0, dword [ecx]
		pfmul mm0, mm7
		
		movq mm2, qword [ecx - (4 * 2)]	;input[i-1] | input[i-2]
		movq mm1, qword [edx - (4 * 2)] ;output[i-1] | output[i-2]
		pfmul mm2, mm6  ;qword [edi + (4 * 1)] ;input[i-1] * b[1] | input[i-2] * b[2]
		pfmul mm1, qword [esi + (4 * 1)]
		pfadd mm0,mm2

		movq mm2, qword [ecx - (4 * 4)]
		movq mm3, qword [edx - (4 * 4)]
		pfmul mm2, mm5  ;qword [edi + (4 * 3)]
		pfmul mm3, qword [esi + (4 * 3)]
		pfadd mm0,mm2
		pfadd mm1,mm3

		movq mm2, qword [ecx - (4 * 6)]
		movq mm3, qword [edx - (4 * 6)]
		pfmul mm2, mm4;qword [edi + (4 * 5)]
		pfmul mm3, qword [esi + (4 * 5)]
		pfadd mm0,mm2
		pfadd mm1,mm3

		movq mm2, qword [ecx - (4 * 8)]
		movq mm3, qword [edx - (4 * 8)]
		pfmul mm2, qword [edi + (4 * 7)]
		pfmul mm3, qword [esi + (4 * 7)]
		pfadd mm0,mm2
		pfadd mm1,mm3

		movq mm2, qword [ecx - (4 * 10)]
		movq mm3, qword [edx - (4 * 10)]
		pfmul mm2, qword [edi + (4 * 9)]
		pfmul mm3, qword [esi + (4 * 9)]
		pfadd mm0,mm2
		pfadd mm1,mm3

		pfsub mm0,mm1
		pfacc mm0,mm0

		lea ecx,[ecx+4]

		movd dword [edx],mm0

		dec eax
		lea edx,[edx+4]
		jnz _filter10_3dnow_l0
_filter10_3dnow_l1:
		pop edi
		pop esi
		femms
		ret 12

global _squared_values_x87
_squared_values_x87:
	;ecx : src, edx : num
		mov eax,edx
		fldz
		shr edx,1
		jz _squared_values_x87_l1
_squared_values_x87_l0:
		fld dword [ecx]
		fmul st0,st0
		faddp st1,st0
		fld dword [ecx+4]
		dec edx
		fmul st0,st0
		lea ecx,[ecx+8]
		faddp st1,st0
		jnz _squared_values_x87_l0
_squared_values_x87_l1:
		and eax,1
		jz _squared_values_x87_l2
		fld dword [ecx]
		fmul st0,st0
		faddp st1,st0
_squared_values_x87_l2:
		ret

global __detect_3dnow_ex
__detect_3dnow_ex:

        pushad

		mov     eax,0x80000000
        CPUID
        cmp     eax,0x80000000
        jbe     __detect_3dnow_ex_return0         ; no extended MSR(1), so no 3DNow!

        mov     eax,0x80000001
        CPUID
        test    edx,0x80000000
        jz      __detect_3dnow_ex_return0
		test	edx,0x40000000
		jz		__detect_3dnow_ex_return0
        jmp     __detect_3dnow_ex_return1

__detect_3dnow_ex_return0:
		popad
		xor eax,eax
		ret
__detect_3dnow_ex_return1:
		popad
		mov eax,1
		ret
