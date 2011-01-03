.386P

EXTRN _kdmasm1 : dword
EXTRN _kdmasm2 : dword
EXTRN _kdmasm3 : dword
EXTRN _kdmasm4 : dword

EXTRN _qualookup : dword

CODE SEGMENT PUBLIC USE32 'CODE'
ASSUME cs:CODE,ds:CODE

PUBLIC _k_asm_dep_unlock ;Data Execution Prevention unlock (works under XP2 SP2)
_k_asm_dep_unlock:
	EXTRN __imp__VirtualProtect@16:NEAR
	sub esp, 4
	push esp
	push 40h ;PAGE_EXECUTE_READWRITE
	push offset _dep_protect_end - offset _k_asm_dep_unlock
	push offset _k_asm_dep_unlock
	call dword ptr __imp__VirtualProtect@16
	add esp, 4
	ret

PUBLIC _monohicomb    ;Visual C entry point (passes parameters by stack)
_monohicomb:
	push ebx
	push ecx
	push edx
	push esi
	push edi
	mov eax, [esp+4+20]
	mov ebx, [esp+8+20]
	mov ecx, [esp+12+20]
	mov edx, [esp+16+20]
	mov esi, [esp+20+20]
	mov edi, [esp+24+20]
	call monohicomb_
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret

PUBLIC monohicomb_    ;Watcom C entry point (passes parameters by register)
monohicomb_:
	;eax=temp2,ebx=temp1,ecx=cnt,edx=dat,esi=dasplc,edi=stemp,ebp=dasoff
	push ebp
	mov ebp, ecx

	mov dword ptr ds:[mach1hm+3], ebx   ;voloffs1

	mov ebx, _kdmasm4
	mov dword ptr ds:[mach5hm+3], ebx

	mov ebx, edx
	shl ebx, 32-12
	sar edx, 12
	mov dword ptr ds:[mach4hm+2], edx   ;dasinc
	;mov edx, ebx       ;Fake 4-bit counter to save shift
	;shr edx, 20   ;
	;mov bx, dx    ;
	mov dword ptr ds:[mach3hm+2], ebx   ;dasinc

	mov ecx, esi
	shl ecx, 32-12
	sar esi, 12
	;mov edx, ecx       ;Fake 4-bit counter to save shift
	;shr edx, 20   ;
	;mov cx, dx    ;

	xor edx, edx
	sub edi, 4
	jmp begithm

prebegithm:
	cmp _kdmasm1, 0
	jz endithm
	mov ebx, _kdmasm2
	mov dword ptr ds:[mach5hm+3], ebx
	mov _kdmasm4, ebx
	mov ebx, _kdmasm3
	mov eax, ebx
	shl ebx, 32-12
	sar eax, 12
	sub ecx, ebx
	sbb esi, eax
	test ebp, ebp
	jz endithm

begithm:
	;edx = _qualookup[((ebp>>19)&0x00001e00)+(bh-bl)]+bl
mach5hm: mov bx, word ptr [esi+88888888h]
	mov eax, ecx
	shr eax, 20
	add edi, 4
	mov dl, bl
	mov al, bl
	sub al, bh
	adc ah, ah
	add dl, byte ptr _qualookup[eax]

	mov ebx, dword ptr [edi]
mach1hm: add ebx, dword ptr [edx*4+88888888h]  ;voloffs1
mach3hm: add ecx, 88888888h     ;dasinc
mach4hm: adc esi, 88888888h     ;dasinc
	dec ebp

	mov dword ptr [edi], ebx

	ja begithm     ;jump if (no carry (add)) and (not zero (dec))!
	jc prebegithm
endithm:

	shl esi, 12
	shr ecx, 32-12
	lea eax, [esi+ecx]

	pop ebp
	ret

PUBLIC _stereohicomb    ;Visual C entry point (passes parameters by stack)
_stereohicomb:
	push ebx
	push ecx
	push edx
	push esi
	push edi
	mov eax, [esp+4+20]
	mov ebx, [esp+8+20]
	mov ecx, [esp+12+20]
	mov edx, [esp+16+20]
	mov esi, [esp+20+20]
	mov edi, [esp+24+20]
	call stereohicomb_
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret

PUBLIC stereohicomb_    ;Watcom C entry point (passes parameters by register)
stereohicomb_:
	;eax=temp2,ebx=temp1,ecx=cnt,edx=dat,esi=dasplc,edi=stemp,ebp=dasoff
	push ebp
	mov ebp, ecx

	mov dword ptr ds:[mach1hs+3], ebx   ;voloffs1
	add ebx, 4
	mov dword ptr ds:[mach2hs+3], ebx   ;voloffs2

	mov ebx, _kdmasm4
	mov dword ptr ds:[mach5hs+3], ebx

	mov ebx, edx
	shl ebx, 32-12
	sar edx, 12
	mov dword ptr ds:[mach4hs+2], edx   ;dasinc
	;mov edx, ebx       ;Fake 4-bit counter to save shift
	;shr edx, 20   ;
	;mov bx, dx    ;
	mov dword ptr ds:[mach3hs+2], ebx   ;dasinc

	mov ecx, esi
	shl ecx, 32-12
	sar esi, 12
	;mov edx, ecx       ;Fake 4-bit counter to save shift
	;shr edx, 20   ;
	;mov cx, dx    ;

	xor edx, edx
	sub edi, 8
	jmp begiths

prebegiths:
	cmp _kdmasm1, 0
	jz endiths
	mov ebx, _kdmasm2
	mov dword ptr ds:[mach5hs+3], ebx
	mov _kdmasm4, ebx
	mov ebx, _kdmasm3
	mov eax, ebx
	shl ebx, 32-12
	sar eax, 12
	sub ecx, ebx
	sbb esi, eax
	test ebp, ebp
	jz endiths

begiths:
	;edx = _qualookup[((ebp>>19)&0x00001e00)+(bh-bl)]+bl
mach5hs: mov bx, word ptr [esi+88888888h]
	mov eax, ecx
	shr eax, 20
	add edi, 8
	mov dl, bl
	mov al, bl
	sub al, bh
	adc ah, ah
	add dl, byte ptr _qualookup[eax]

	mov eax, dword ptr [edi]
mach1hs: mov ebx, dword ptr [edx*8+88888888h]  ;voloffs1
	add ebx, eax
	mov eax, dword ptr [edi+4]
	mov dword ptr [edi], ebx
mach2hs: mov ebx, dword ptr [edx*8+88888888h]  ;voloffs2
	add eax, ebx

mach3hs: add ecx, 88888888h     ;dasinc
mach4hs: adc esi, 88888888h     ;dasinc

	dec ebp

	mov dword ptr [edi+4], eax

	ja begiths     ;jump if (no carry (add)) and (not zero (dec))!
	jc prebegiths
endiths:

	shl esi, 12
	shr ecx, 32-12
	lea eax, [esi+ecx]

	pop ebp
	ret


	;for(i=0;i<bytespertic;i++)
	;{
	;   j = (stemp[i]>>8);
	;   if (j < 0) j = 0;
	;   if (j > 255) j = 255;
	;   *charptr++ = (char)j;
	;}
PUBLIC _bound2char    ;Visual C entry point (passes parameters by stack)
_bound2char:
	push ebx
	push ecx
	push edx
	push esi
	push edi
	mov ecx, [esp+4+20]
	mov esi, [esp+8+20]
	mov edi, [esp+12+20]
	call bound2char_
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret

PUBLIC bound2char_    ;Watcom C entry point (passes parameters by register)
bound2char_:
	add ecx, ecx
	lea eax, [ecx+edi]
	mov dword ptr ds:[machchar+3], eax
	xor edi, edi
	sub edi, ecx
	xor ecx, ecx

	mov edx, 0ffff0000h
startbound2char:
	mov ebx, dword ptr [esi]
	mov eax, dword ptr [esi+4]
	test ebx, edx
	jnz short boundchar1
boundchar1back:
	mov dword ptr [esi], 32768
	test eax, edx
	jnz short boundchar2
boundchar2back:
	mov dword ptr [esi+4], 32768
	mov al, bh
	add esi, 8
machchar: mov word ptr [edi+88888888h], ax
	add edi, 2
	jnc short startbound2char
	ret
boundchar1:
	cmp ebx, 80000000h
	sbb bh, bh
	jmp short boundchar1back
boundchar2:
	cmp eax, 80000000h
	sbb ah, ah
	jmp short boundchar2back

	;for(i=0;i<bytespertic;i++)
	;{
	;   j = stemp[i];
	;   if (j < 0) j = 0;
	;   if (j > 65535) j = 65535;
	;   *shortptr++ = (short)(j^8000h);
	;}
PUBLIC _bound2short   ;Visual C entry point (passes parameters by stack)
_bound2short:
	push ebx
	push ecx
	push edx
	push esi
	push edi
	mov ecx, [esp+4+20]
	mov esi, [esp+8+20]
	mov edi, [esp+12+20]
	call bound2short_
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret

PUBLIC bound2short_    ;Watcom C entry point (passes parameters by register)
bound2short_:
	shl ecx, 2
	lea eax, [ecx+edi]
	mov dword ptr ds:[machshort+2], eax
	xor edi, edi
	sub edi, ecx
	xor ecx, ecx

	mov edx, 0ffff0000h
startbound2short:
	mov ebx, dword ptr [esi]
	mov eax, dword ptr [esi+4]
	test ebx, edx
	jnz short boundshort1
boundshort1back:
	mov dword ptr [esi], 32768
	test eax, edx
	jnz short boundshort2
boundshort2back:
	shl eax, 16
	mov dword ptr [esi+4], 32768
	mov ax, bx
	add esi, 8
	xor eax, 80008000h
machshort: mov dword ptr [edi+88888888h], eax
	add edi, 4
	jnc short startbound2short
	ret
boundshort1:
	cmp ebx, 80000000h
	sbb ebx, ebx
	jmp short boundshort1back
boundshort2:
	cmp eax, 80000000h
	sbb eax, eax
	jmp short boundshort2back

_dep_protect_end:
CODE ENDS
END
