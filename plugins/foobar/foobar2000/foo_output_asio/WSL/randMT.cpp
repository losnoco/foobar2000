#include <stdio.h>
#include <stdlib.h>

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

/* Static member */
int mti;                   /* index number */
unsigned long mt[N + 1];   /* the array for the state vector */
unsigned long mtr[N];      /* the array for the random number */
unsigned long bInitialized = 0;
unsigned long bMMX = 0;

/* Prototype */
void srandMT(unsigned long seed);
inline unsigned long CheckMMX(void);
void MMX_generateMT(void);
inline void generateMT(void);
unsigned long randMT(void);


void srandMT(unsigned long seed)
{
    int i;

    for(i = 0; i < N; i++){
         mt[i] = seed & 0xffff0000;
         seed = 69069 * seed + 1;
         mt[i] |= (seed & 0xffff0000) >> 16;
         seed = 69069 * seed + 1;
    }

    bMMX = CheckMMX();
    bInitialized = 1;
    MMX_generateMT();
}


#define cpuid __asm __emit 0fh __asm __emit 0a2h

inline unsigned long CheckMMX(void)
{
    unsigned long flag = 0;

    _asm{
        push    edx
        push    ecx
        pushfd
        pop     eax
        xor     eax, 00200000h
        push    eax
        popfd
        pushfd
        pop     ebx
        cmp     eax, ebx
        jnz     non_mmx
        mov     eax, 0
        cpuid
        cmp     eax, 0
        jz      non_mmx
        mov     eax, 1
        cpuid
        and     edx, 00800000h
        jz      non_mmx
        mov     flag, 1
        non_mmx:
        pop     ecx
        pop     edx
    }
    return(flag);
}

/* MMX version */
void MMX_generateMT(void)
{
    if(bMMX == 0){
        generateMT();
        return;
    }

    _asm{
        mov         eax, MATRIX_A
        movd        mm4, eax
        punpckldq   mm4, mm4
        mov         eax, 1
        movd        mm5, eax
        punpckldq   mm5, mm5
        movq        mm6, mm5
        psllq       mm6, 31     /* UPPER_MASK */
        movq        mm7, mm6
        psubd       mm7, mm5    /* LOWER_MASK */

        lea         esi, mt
        add         esi, ((N-M)/2)*8
        mov         edi, esi
        add         edi, M*4
        
        mov         ecx, -((N-M)/2)
        /* 8clocks */
        movq        mm0, [esi+ecx*8]
        pand        mm0, mm6
        movq        mm1, [esi+ecx*8+4]
        movq        mm2, [edi+ecx*8]
        pand        mm1, mm7
    loop0:
        por         mm0, mm1
        movq        mm3, mm1
        psrld       mm0, 1
        pand        mm3, mm5
        pxor        mm2, mm0
        psubd       mm3, mm5
        movq        mm0, [esi+ecx*8+8]
        pandn       mm3, mm4
        movq        mm1, [esi+ecx*8+12]
        pxor        mm3, mm2
        movq        mm2, [edi+ecx*8+8]
        pand        mm0, mm6
        movq        [esi+ecx*8], mm3
        pand        mm1, mm7
        inc         ecx
        jnz         loop0
        /**/
        mov         eax, [esi]
        mov         ebx, [esi+4]
        and         eax, UPPER_MASK
        and         ebx, LOWER_MASK
        mov         edx, [edi]
        or          eax, ebx
        shr         eax, 1
        or          ebx, 0xfffffffe
        xor         eax, edx
        add         ebx, 1
        xor         eax, MATRIX_A
        and         ebx, MATRIX_A
        xor         ebx, eax
        mov         [esi], ebx
        /**/

        lea         esi, mt
        mov         eax, [esi] /* mt[N] = mt[0]; */
        add         esi, N*4
        mov         [esi], eax /* mt[N] = mt[0]; */

        mov         edi, esi
        sub         edi, (N-M)*4
        add         esi, 4
        add         edi, 4
        
        mov         ecx, -((M+1)/2) /* overrun */
        /* 8clocks */
        movq        mm0, [esi+ecx*8]
        pand        mm0, mm6
        movq        mm1, [esi+ecx*8+4]
        movq        mm2, [edi+ecx*8]
        pand        mm1, mm7
    loop1:
        por         mm0, mm1
        movq        mm3, mm1
        psrld       mm0, 1
        pand        mm3, mm5
        pxor        mm2, mm0
        psubd       mm3, mm5
        movq        mm0, [esi+ecx*8+8]
        pandn       mm3, mm4
        movq        mm1, [esi+ecx*8+12]
        pxor        mm3, mm2
        movq        mm2, [edi+ecx*8+8]
        pand        mm0, mm6
        movq        [esi+ecx*8], mm3
        pand        mm1, mm7
        inc         ecx
        jnz         loop1
        /**/

        push        ebp

        mov         eax, TEMPERING_MASK_B
        movd        mm6, eax
        punpckldq   mm6, mm6
        mov         eax, TEMPERING_MASK_C
        movd        mm7, eax
        punpckldq   mm7, mm7

        lea         esi, mt
        lea         edi, mtr
        add         esi, N*4
        add         edi, N*4
        mov         ecx, -N/2
        /* 17clocks */
        movq        mm0, [esi+ecx*8]
        movq        mm1, mm0
        psrld       mm0, 11
        pxor        mm0, mm1
        movq        mm2, [esi+ecx*8+8]
    loop2:
        movq        mm1, mm0
        pslld       mm0, 7
        movq        mm3, mm2
        psrld       mm2, 11
        pand        mm0, mm6
        pxor        mm2, mm3
        pxor        mm0, mm1
        movq        mm3, mm2
        pslld       mm2, 7
        movq        mm1, mm0
        pslld       mm0, 15
        pand        mm2, mm6
        pand        mm0, mm7
        pxor        mm2, mm3
        pxor        mm0, mm1
        movq        mm3, mm2
        pslld       mm2, 15
        movq        mm1, mm0
        pand        mm2, mm7
        psrld       mm0, 18
        pxor        mm2, mm3
        pxor        mm1, mm0
        movq        mm0, [esi+ecx*8+16]
        movq        mm3, mm2
        movq        [edi+ecx*8], mm1
        psrld       mm2, 18
        movq        mm1, mm0
        pxor        mm3, mm2
        movq        mm2, [esi+ecx*8+24]
        psrld       mm0, 11
        movq        [edi+ecx*8+8], mm3
        pxor        mm0, mm1
        add         ecx, 2
        jnz         loop2
        /**/

        pop         ebp
        emms
    }
    mti = 0;
}

/* Non MMX version */
inline void generateMT(void)
{
//    if(bMMX){
//        MMX_generateMT();
//        return;
//    }

    _asm{
        lea     esi, mt
        add     esi, (N-M)*4
        mov     edi, esi
        add     edi, M*4
        
        mov     ecx, -(N-M)
        /* 8clocks */
        mov     eax, [esi+ecx*4]
        mov     ebx, [esi+ecx*4+4]
    loop0:
        and     eax, UPPER_MASK
        and     ebx, LOWER_MASK
        mov     edx, [edi+ecx*4]
        or      eax, ebx
        shr     eax, 1
        or      ebx, 0xfffffffe
        xor     eax, edx
        add     ebx, 1
        xor     eax, MATRIX_A
        and     ebx, MATRIX_A
        xor     ebx, eax
        mov     eax, [esi+ecx*4+4]
        mov     [esi+ecx*4], ebx
        mov     ebx, [esi+ecx*4+8]
        inc     ecx
        jnz     loop0
        /**/

        lea     esi, mt
        mov     eax, [esi] /* mt[N] = mt[0]; */
        add     esi, N*4
        mov     [esi], eax /* mt[N] = mt[0]; */
        mov     edi, esi
        sub     edi, (N-M)*4
        
        mov     ecx, -M
        /* 8clocks */
        mov     eax, [esi+ecx*4]
        mov     ebx, [esi+ecx*4+4]
    loop1:
        and     eax, UPPER_MASK
        and     ebx, LOWER_MASK
        mov     edx, [edi+ecx*4]
        or      eax, ebx
        shr     eax, 1
        or      ebx, 0xfffffffe
        xor     eax, edx
        add     ebx, 1
        xor     eax, MATRIX_A
        and     ebx, MATRIX_A
        xor     ebx, eax
        mov     eax, [esi+ecx*4+4]
        mov     [esi+ecx*4], ebx
        mov     ebx, [esi+ecx*4+8]
        inc     ecx
        jnz     loop1
        /**/

        push    ebp

        lea     esi, mt
        lea     edi, mtr
        add     esi, N*4
        add     edi, N*4
        mov     ecx, -N/2
        /* 17clocks */
        mov     eax, [esi+ecx*8]
    loop2:
        mov     edx, [esi+ecx*8+4]
        mov     ebx, eax
        shr     eax, 11
        mov     ebp, edx
        shr     edx, 11
        xor     eax, ebx
        xor     edx, ebp
        mov     ebx, eax
        shl     eax, 7
        mov     ebp, edx
        shl     edx, 7
        and     eax, TEMPERING_MASK_B
        and     edx, TEMPERING_MASK_B
        xor     eax, ebx
        xor     edx, ebp
        mov     ebx, eax
        shl     eax, 15
        mov     ebp, edx
        shl     edx, 15
        and     eax, TEMPERING_MASK_C
        and     edx, TEMPERING_MASK_C
        xor     eax, ebx
        xor     edx, ebp
        mov     ebx, eax
        shr     eax, 18
        mov     ebp, edx
        shr     edx, 18
        xor     eax, ebx
        xor     edx, ebp
        mov     [edi+ecx*8],   eax
        mov     [edi+ecx*8+4], edx
        mov     eax, [esi+ecx*8+8]
        inc     ecx
        jnz     loop2
        /**/

        pop     ebp
    }
    mti = 0;
}

/* C version */
/*
void generateMT_C(void)
{
    int kk;
    unsigned long y;
    static unsigned long mag01[2] = {0x0, MATRIX_A};
    
    for(kk = 0; kk < N - M; kk++){
        y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
        mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
    }

    mt[N] = mt[0];

    for(; kk < N; kk++){
        y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
        mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }

    for(kk = 0; kk < N; kk++){
        y = mt[kk];
        y ^= TEMPERING_SHIFT_U(y);
        y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
        y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
        y ^= TEMPERING_SHIFT_L(y);
        mtr[kk] = y;
    }
    mti = 0;
}
*/

//#ifdef inline
//inline unsigned long randMT(void)
//#else
unsigned long randMT(void)
//#endif
{
    if(mti >= N){
        if(!bInitialized) srandMT(4357);
        MMX_generateMT();
    }
    return mtr[mti++];
}


/* This main() outputs first 1000 generated numbers.  */
/*
int main(int argc, char *argv[])
{ 
    int i;

    srandMT(4357);
    for (i=0; i<1000; i++) {
        printf("%10lu ", randMT());
        if (i%5==4) printf("\n");
    }
    return EXIT_SUCCESS;
}
*/
