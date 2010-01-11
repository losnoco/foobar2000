/* Cpu detection code, extracted from mmx.h ((c)1997-99 by H. Dietz
   and R. Fisher). Converted to C and improved by Fabrice Bellard */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cputest.h"

#ifndef _MSC_VER

/* ebx saving is necessary for PIC. gcc seems unable to see it alone */
#define cpuid(index,eax,ebx,ecx,edx)\
    __asm __volatile\
	("movl %%ebx, %%esi\n\t"\
         "cpuid\n\t"\
         "xchgl %%ebx, %%esi"\
         : "=a" (eax), "=S" (ebx),\
           "=c" (ecx), "=d" (edx)\
         : "a" (index));

#endif

#define CPUID_STD_MMX          0x00800000
#define CPUID_STD_SSE          0x02000000
#define CPUID_STD_SSE2         0x04000000
#define CPUID_STD_SSE3         0x00000001  // ECX!
#define CPUID_EXT_AMD_3DNOW    0x80000000
#define CPUID_EXT_AMD_3DNOWEXT 0x40000000
#define CPUID_EXT_AMD_MMXEXT   0x00400000
#define CPUID_EXT_CYR_MMX      0x00800000
#define CPUID_EXT_CYR_MMXEXT   0x01000000

/* Function to test if multimedia instructions are supported...  */
static int mm_support(void)
{
#ifdef ARCH_X86
    int rval;

    int a, b, c, d;
    char vendor[13] = "UnknownVndr";

#ifdef _MSC_VER
	_asm {
		pushf
		pop eax

		mov ecx, eax

		xor eax, 0x200000
		push eax
		popf

		pushf
		pop eax
		push ecx
		popf

		mov c, ecx
		mov a, eax
	}
#else
    __asm__ __volatile__ (
                          /* See if CPUID instruction is supported ... */
                          /* ... Get copies of EFLAGS into eax and ecx */
                          "pushf\n\t"
#ifdef __x86_64__
                          "pop %0\n\t"
#else
                          "popl %0\n\t"
#endif
                          "movl %0, %1\n\t"
                          
                          /* ... Toggle the ID bit in one copy and store */
                          /*     to the EFLAGS reg */
                          "xorl $0x200000, %0\n\t"
                          "push %0\n\t"
                          "popf\n\t"
                          
                          /* ... Get the (hopefully modified) EFLAGS */
                          "pushf\n\t"
#ifdef __x86_64__
                          "pop %0\n\t"
#else
                          "popl %0\n\t"
#endif
                          : "=a" (eax), "=c" (ecx)
                          :
                          : "cc" 
                          );
#endif

    if (a == c)
        return 0; /* CPUID not supported */
    
#ifdef _MSC_VER
	_asm {
		mov eax, 0
		cpuid
		mov a, eax
		mov b, ebx
		mov c, ecx
		mov d, edx
	}
#else
    cpuid(0, a, b, c, d);
#endif

    /* save the vendor string */
    *(int *)vendor = b;
    *(int *)&vendor[4] = d;
    *(int *)&vendor[8] = c;

    rval = 0;

    /* highest cpuid is 0, no standard features */
    if (a == 0)
        return rval;

    /* get standard features */
#ifdef _MSC_VER
	_asm {
		mov eax, 1
		cpuid
		mov a, eax
		mov b, ebx
		mov c, ecx
		mov d, edx
	}
#else
    cpuid(1, a, b, c, d);
#endif
    //printf("CPUID 1 eax=0x%8.8x ebx=0x%8.8x ecx=0x%8.8x edx=0x%8.8x\n", eax, ebx, ecx, edx);
    if (d & CPUID_STD_MMX)
        rval |= MM_MMX;
    if (d & CPUID_STD_SSE)
        rval |= MM_MMXEXT | MM_SSE;
    if (d & CPUID_STD_SSE2)
        rval |= MM_SSE2;
    //if (ecx & CPUID_STD_SSE3)
    //    rval |= MM_SSE3;

    /* check for extended feature flags support */
#ifdef _MSC_VER
	_asm {
		mov eax, 0x80000000
		cpuid
		mov a, eax
	}
#else
    cpuid(0x80000000, a, b, c, d);
#endif
    //printf("CPUID 8-0 eax=0x%8.8x ebx=0x%8.8x ecx=0x%8.8x edx=0x%8.8x\n", eax, ebx, ecx, edx);
    if (a < 0x80000001)
        return rval;

    /* get extended feature flags */
#ifdef _MSC_VER
	_asm {
		mov eax, 0x80000001
		cpuid
		mov a, eax
		mov b, ebx
		mov c, ecx
		mov d, edx
	}
#else
    cpuid(0x80000001, a, b, c, d);
#endif
    //printf("CPUID 8-1 eax=0x%8.8x ebx=0x%8.8x ecx=0x%8.8x edx=0x%8.8x\n", eax, ebx, ecx, edx);

    /* AMD-specific extensions */
    if (strcmp(vendor, "AuthenticAMD") == 0) {
        if (d & CPUID_EXT_AMD_3DNOW)
            rval |= MM_3DNOW;
        if (d & CPUID_EXT_AMD_3DNOWEXT)
            rval |= MM_3DNOWEXT;
        /* if no MMXEXT already, check AMD-specific flag */
        if (((rval & MM_MMXEXT) == 0) && (d & CPUID_EXT_AMD_MMXEXT))
            rval |= MM_MMXEXT;
    }

    /* Cyrix-specific extensions */
    else if (strcmp(vendor, "CyrixInstead") == 0) {
        /* The Cyrix CPU Detection Guide says that CPUID level 1 should 
           return the proper MMX feature flag, but just in case */
        if (((rval & MM_MMX) == 0) && (d & CPUID_EXT_CYR_MMX))
            rval |= MM_MMX;
        /* if no MMXEXT already, check Cyrix-specific flag */
        if (((rval & MM_MMXEXT) == 0) && (d & CPUID_EXT_CYR_MMXEXT))
            rval |= MM_MMXEXT;
    }

    return rval;
#else // not X86
    return 0;
#endif
}

int ac_mmflag(void) 
{
 int mm_flag;

 mm_flag = mm_support();
#ifdef ARCH_X86
 mm_flag |= MM_IA32ASM;
#endif

#ifdef ARCH_POWERPC
 mm_flag |= MM_ALTIVEC; // FIXME for actual detection!
#endif
  return(mm_flag);
}
