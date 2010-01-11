;
;  Assembler routines to detect CPU features for Intel x86 compatible CPUs
;  (Intel, AMD, Rise, Cyrix). CPU must be at least a 386SX, otherwise the
;  detection routines fail with an "Illegal Instruction Trap".
;
;  But note:
;    - you currently need at least a high end 486 CPU to run in realtime
;    - this is only important if you want to built a 16 bit executable
;      running on 80286 and on modern CPUs with Katmai/3DNow! support
;

%include "tools.inc"

        globaldef       Has_MMX
        globaldef       Has_3DNow
        globaldef       Has_SIMD
        globaldef       Has_SIMD2


        segment_code

testCPUID:
        pushfd
        pop     eax
        mov     ecx,eax
        xor     eax,0x200000
        push    eax
        popfd
        pushfd
        pop     eax
        cmp     eax,ecx
        mov     eax,1
        ret

;-------------------------------------;
;    bool_t  Has_MMX (void)           ;
;-------------------------------------;

proc    Has_MMX
        pushad
        call    testCPUID
        jz      return0         ; no CPUID command, so no MMX

        CPUID
        test    edx,0x800000
        jz      return0         ; no MMX support
        jmp     short return1   ; MMX support
endproc

;-------------------------------------;
;    bool_t  Has_SIMD (void)          ;
;-------------------------------------;

proc    Has_SIMD
        pushad
        call    testCPUID
        jz      return0         ; no CPUID command, so no SIMD

        CPUID
        test    edx,0x02000000
        jz      return0         ; no SIMD support
        jmp     short return1   ; SIMD support
endproc

;-------------------------------------;
;    bool_t  Has_SIMD2 (void)         ;
;-------------------------------------;

proc    Has_SIMD2
        pushad
        call    testCPUID
        jz      return0         ; no CPUID command, so no SIMD2

        CPUID
        test    edx,0x04000000
        jz      return0         ; no SIMD2 support
                                ; SIMD2 support
return1:
        popad
        xor     eax,eax
        inc     eax
        ret

return0:
        popad
        xor     eax,eax
        ret

endproc

;-------------------------------------;
;    bool_t  Has_3DNow (void)         ;
;-------------------------------------;

proc    Has_3DNow
        pushad
        call    testCPUID
        jz      return0         ; no CPUID command, so no 3DNow!

        mov     eax,0x80000000
        CPUID
        cmp     eax,0x80000000
        jbe     return0         ; no extended MSR(1), so no 3DNow!

        mov     eax,0x80000001
        CPUID
        test    edx,0x80000000
        jz      return0         ; no 3DNow! support
        jmp     short return1   ; 3DNow! support
endproc

;-------------------------------------;
;    void  Init_FPU2 (void)           ;
;-------------------------------------;


proc    Init_FPU2
        push    eax
        fstcw   [esp]
        and     byte [esp+1], 0FCh
        fldcw   [esp]
        pop     eax
endproc

        end
