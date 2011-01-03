section .text use32

global __alloca
__alloca:
	push	ecx
	mov		ecx, esp
	add		ecx, 8
repeat:
	cmp		eax, 1000h
	jb		OK
	sub		ecx, 1000h
	or		dword [ecx], 0
	sub		eax, 1000h
	jmp		repeat
OK:
	sub		ecx, eax
	or		dword [ecx], 0
	mov		eax, esp
	mov		esp, ecx
	mov		ecx, dword [eax]
	mov		eax, dword [eax + 4]
	jmp		eax
