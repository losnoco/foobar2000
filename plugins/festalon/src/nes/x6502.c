/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>

#include "../types.h"
#include "x6502.h"
#include "sound.h"
#include "nsf.h"

static DECLFW(BNull)
{

}

static DECLFR(ANull)    
{
 return(DB);
}

void SetReadHandler(X6502 *X, int32 start, int32 end, readfunc func, void *private)
{
  int32 x;

  if(!func)
   func=ANull;
  for(x=end;x>=start;x--)
  {
   X->ARead[x]=func;
   X->AReadPrivate[x]=private;
  }
}

void SetWriteHandler(X6502 *X, int32 start, int32 end, writefunc func, void *private)
{
  int32 x;

  if(!func)
   func=BNull;

  for(x=end;x>=start;x--)
  {
   if(X->BWrite[x].func && X->BWrite[x].func != BNull)
   {
    WriteMap *wmtmp = malloc(sizeof(WriteMap));
    memcpy(wmtmp, &X->BWrite[x], sizeof(WriteMap));

    X->BWrite[x].func = func;
    X->BWrite[x].private = private;
    X->BWrite[x].next = wmtmp;
   }
   else
   {
    X->BWrite[x].func = func;
    X->BWrite[x].private = private;
    X->BWrite[x].next = 0;
   }
  }
} 

#define _PC              X->PC
#define _A               X->A
#define _X               X->X
#define _Y               X->Y
#define _S               X->S
#define _P               X->P
#define _PI              X->mooPI
#define _DB              X->DB
#define _count           X->count
#define _tcount          X->tcount
#define _IRQlow          X->IRQlow
#define _jammed          X->jammed
#define _timestamp	 X->timestamp

#define ADDCYC(x)       \
{       \
 int __x=x;     \
 _tcount+=__x;  \
 _count-=__x*48;	\
}

static INLINE uint8 RdMem(X6502 *X, unsigned int A)
{
 return(_DB=X->ARead[A](X->AReadPrivate[A],A,_DB));
}

static void WrMem(X6502 *X, unsigned int A, uint8 V)
{
 WriteMap *wm = &X->BWrite[A];

 do
 {
  wm->func(wm->private, A, V);
  wm = wm->next;
 } while(wm);
 //X->BWrite[A].func(X->BWrite[A].private,A,V);
}

static INLINE uint8 RdRAM(X6502 *X, unsigned int A)
{
 return(_DB=X->RAM[A]);
}

static INLINE void WrRAM(X6502 *X, unsigned int A, uint8 V)
{
 X->RAM[A]=V;
}

uint8 FASTAPASS(1) X6502_DMR(X6502 *X, uint32 A)
{
 ADDCYC(1);
 return(_DB=X->ARead[A](X->AReadPrivate[A],A,_DB));
}

void FASTAPASS(2) X6502_DMW(X6502 *X, uint32 A, uint8 V)
{
 WriteMap *wm;
 ADDCYC(1);

 wm = &X->BWrite[A];

 do
 {
  wm->func(wm->private, A, V);
  wm = wm->next;
 } while(wm);
 //X->BWrite[A].func(X->BWrite[A].private,A,V);
}

#define PUSH(V) \
{       \
 uint8 VTMP=V;  \
 WrRAM(X,0x100+_S,VTMP);  \
 _S--;  \
}       

#define POP() RdRAM(X,0x100+(++_S))

static const uint8 ZNTable[256]=
{
 Z_FLAG, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
 N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG
};

/* Some of these operations will only make sense if you know what the flag
   constants are. */

#define X_ZN(zort)      _P&=~(Z_FLAG|N_FLAG);_P|=ZNTable[zort]
#define X_ZNT(zort)	_P|=ZNTable[zort]

#define JR(cond);	\
{		\
 if(cond)	\
 {	\
  uint32 tmp;	\
  int8 disp;	\
  disp=RdMem(X,_PC);	\
  _PC++;	\
  ADDCYC(1);	\
  tmp=_PC;	\
  _PC+=disp;	\
  if((tmp^_PC)&0x100)	\
  ADDCYC(1);	\
 }	\
 else _PC++;	\
}

#define LDA	   _A=x;X_ZN(_A)
#define LDX	   _X=x;X_ZN(_X)
#define LDY        _Y=x;X_ZN(_Y)

/*  All of the freaky arithmetic operations. */
#define AND        _A&=x;X_ZN(_A)
#define BIT        _P&=~(Z_FLAG|V_FLAG|N_FLAG);_P|=ZNTable[x&_A]&Z_FLAG;_P|=x&(V_FLAG|N_FLAG)
#define EOR        _A^=x;X_ZN(_A)
#define ORA        _A|=x;X_ZN(_A)

#define ADC  {	\
	      uint32 l=_A+x+(_P&1);	\
	      _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);	\
              _P|=((((_A^x)&0x80)^0x80) & ((_A^l)&0x80))>>1;	\
              _P|=(l>>8)&C_FLAG;	\
	      _A=l;	\
	      X_ZNT(_A);	\
	     }

#define SBC  {	\
	      uint32 l=_A-x-((_P&1)^1);	\
	      _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);	\
	      _P|=((_A^l)&(_A^x)&0x80)>>1;	\
	      _P|=((l>>8)&C_FLAG)^C_FLAG;	\
	      _A=l;	\
	      X_ZNT(_A);	\
	     }

#define CMPL(a1,a2) {	\
		     uint32 t=a1-a2;	\
		     X_ZN(t&0xFF);	\
		     _P&=~C_FLAG;	\
		     _P|=((t>>8)&C_FLAG)^C_FLAG;	\
		    }

/* Special undocumented operation.  Very similar to CMP. */
#define AXS	    {	\
                     uint32 t=(_A&_X)-x;    \
                     X_ZN(t&0xFF);      \
                     _P&=~C_FLAG;       \
                     _P|=((t>>8)&C_FLAG)^C_FLAG;        \
		     _X=t;	\
                    }

#define CMP		CMPL(_A,x)
#define CPX		CMPL(_X,x)
#define CPY	      	CMPL(_Y,x)

/* The following operations modify the byte being worked on. */
#define DEC       	x--;X_ZN(x)
#define INC		x++;X_ZN(x)

#define ASL        _P&=~C_FLAG;_P|=x>>7;x<<=1;X_ZN(x)
#define LSR	_P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=x&1;x>>=1;X_ZNT(x)

/* For undocumented instructions, maybe for other things later... */
#define LSRA	_P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=_A&1;_A>>=1;X_ZNT(_A)

#define ROL	{	\
		 uint8 l=x>>7;	\
		 x<<=1;	\
		 x|=_P&C_FLAG;	\
		 _P&=~(Z_FLAG|N_FLAG|C_FLAG);	\
		 _P|=l;	\
		 X_ZNT(x);	\
		}
#define ROR	{	\
		 uint8 l=x&1;	\
		 x>>=1;	\
		 x|=(_P&C_FLAG)<<7;	\
		 _P&=~(Z_FLAG|N_FLAG|C_FLAG);	\
		 _P|=l;	\
		 X_ZNT(x);	\
		}
		 
/* Icky icky thing for some undocumented instructions.  Can easily be
   broken if names of local variables are changed.
*/

/* Absolute */
#define GetAB(target) 	\
{	\
 target=RdMem(X,_PC);	\
 _PC++;	\
 target|=RdMem(X, _PC)<<8;	\
 _PC++;	\
}

/* Absolute Indexed(for reads) */
#define GetABIRD(target, i)	\
{	\
 unsigned int tmp;	\
 GetAB(tmp);	\
 target=tmp;	\
 target+=i;	\
 if((target^tmp)&0x100)	\
 {	\
  target&=0xFFFF;	\
  RdMem(X,target^0x100);	\
  ADDCYC(1);	\
 }	\
}

/* Absolute Indexed(for writes and rmws) */
#define GetABIWR(target, i)	\
{	\
 unsigned int rt;	\
 GetAB(rt);	\
 target=rt;	\
 target+=i;	\
 target&=0xFFFF;	\
 RdMem(X,(target&0x00FF)|(rt&0xFF00));	\
}

/* Zero Page */
#define GetZP(target)	\
{	\
 target=RdMem(X,_PC); 	\
 _PC++;	\
}

/* Zero Page Indexed */
#define GetZPI(target,i)	\
{	\
 target=i+RdMem(X,_PC);	\
 _PC++;	\
}

/* Indexed Indirect */
#define GetIX(target)	\
{	\
 uint8 tmp;	\
 tmp=RdMem(X,_PC);	\
 _PC++;	\
 tmp+=_X;	\
 target=RdRAM(X,tmp);	\
 tmp++;		\
 target|=RdRAM(X,tmp)<<8;	\
}

/* Indirect Indexed(for reads) */
#define GetIYRD(target)	\
{	\
 unsigned int rt;	\
 uint8 tmp;	\
 tmp=RdMem(X,_PC);	\
 _PC++;	\
 rt=RdRAM(X,tmp);	\
 tmp++;	\
 rt|=RdRAM(X,tmp)<<8;	\
 target=rt;	\
 target+=_Y;	\
 if((target^rt)&0x100)	\
 {	\
  target&=0xFFFF;	\
  RdMem(X,target^0x100);	\
  ADDCYC(1);	\
 }	\
}

/* Indirect Indexed(for writes and rmws) */
#define GetIYWR(target)	\
{	\
 unsigned int rt;	\
 uint8 tmp;	\
 tmp=RdMem(X,_PC);	\
 _PC++;	\
 rt=RdRAM(X,tmp);	\
 tmp++;	\
 rt|=RdRAM(X,tmp)<<8;	\
 target=rt;	\
 target+=_Y;	\
 target&=0xFFFF; \
 RdMem(X,(target&0x00FF)|(rt&0xFF00));	\
}

/* Now come the macros to wrap up all of the above stuff addressing mode functions
   and operation macros.  Note that operation macros will always operate(redundant
   redundant) on the variable "x".
*/

#define RMW_A(op) {uint8 x=_A; op; _A=x; break; } /* Meh... */
#define RMW_AB(op) {unsigned int A; uint8 x; GetAB(A); x=RdMem(X,A); WrMem(X,A,x); op; WrMem(X,A,x); break; }
#define RMW_ABI(reg,op) {unsigned int A; uint8 x; GetABIWR(A,reg); x=RdMem(X,A); WrMem(X,A,x); op; WrMem(X,A,x); break; }
#define RMW_ABX(op)	RMW_ABI(_X,op)
#define RMW_ABY(op)	RMW_ABI(_Y,op)
#define RMW_IX(op)  {unsigned int A; uint8 x; GetIX(A); x=RdMem(X,A); WrMem(X,A,x); op; WrMem(X,A,x); break; }
#define RMW_IY(op)  {unsigned int A; uint8 x; GetIYWR(A); x=RdMem(X,A); WrMem(X,A,x); op; WrMem(X,A,x); break; }
#define RMW_ZP(op)  {uint8 A; uint8 x; GetZP(A); x=RdRAM(X,A); op; WrRAM(X,A,x); break; }
#define RMW_ZPX(op) {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(X,A); op; WrRAM(X,A,x); break;}

#define LD_IM(op)	{uint8 x; x=RdMem(X,_PC); _PC++; op; break;}
#define LD_ZP(op)	{uint8 A; uint8 x; GetZP(A); x=RdRAM(X,A); op; break;}
#define LD_ZPX(op)  {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(X,A); op; break;}
#define LD_ZPY(op)  {uint8 A; uint8 x; GetZPI(A,_Y); x=RdRAM(X,A); op; break;}
#define LD_AB(op)	{unsigned int A; uint8 x; GetAB(A); x=RdMem(X,A); op; break; }
#define LD_ABI(reg,op)  {unsigned int A; uint8 x; GetABIRD(A,reg); x=RdMem(X,A); op; break;}
#define LD_ABX(op)	LD_ABI(_X,op)
#define LD_ABY(op)	LD_ABI(_Y,op)
#define LD_IX(op)	{unsigned int A; uint8 x; GetIX(A); x=RdMem(X,A); op; break;}
#define LD_IY(op)	{unsigned int A; uint8 x; GetIYRD(A); x=RdMem(X,A); op; break;}

#define ST_ZP(r)	{uint8 A; GetZP(A); WrRAM(X,A,r); break;}
#define ST_ZPX(r)	{uint8 A; GetZPI(A,_X); WrRAM(X,A,r); break;}
#define ST_ZPY(r)	{uint8 A; GetZPI(A,_Y); WrRAM(X,A,r); break;}
#define ST_AB(r)	{unsigned int A; GetAB(A); WrMem(X,A,r); break;}
#define ST_ABI(reg,r)	{unsigned int A; GetABIWR(A,reg); WrMem(X,A,r); break; }
#define ST_ABX(r)	ST_ABI(_X,r)
#define ST_ABY(r)	ST_ABI(_Y,r)
#define ST_IX(r)	{unsigned int A; GetIX(A); WrMem(X,A,r); break; }
#define ST_IY(r)	{unsigned int A; GetIYWR(A); WrMem(X,A,r); break; }

static uint8 CycTable[256] =
{                             
/*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
/*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
/*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
/*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
/*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
/*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
/*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

void FASTAPASS(1) X6502_IRQBegin(X6502 *X, int w)
{
 _IRQlow|=w;
}

void FASTAPASS(1) X6502_IRQEnd(X6502 *X, int w)
{
 _IRQlow&=~w;
}

void X6502_Reset(X6502 *X)
{
 _IRQlow=FCEU_IQRESET;
}
  
void X6502_Free(X6502 *X)
{
 int x;
 
 for(x=0;x<65536;x++)
 {
  WriteMap *wm = &X->BWrite[x];
  wm = wm->next;

  while(wm)
  {
   WriteMap *next = wm->next;

   free(wm);

   wm = next;
  }

 }
 free(X);
}

X6502 *X6502_Init(uint8 *RAM, int PAL, void *private)
{
	X6502 *X;

	if(!(X=malloc(sizeof(X6502)))) return(0);

	memset(X,0,sizeof(X6502));

        X->RAM=RAM;
	X->PAL=PAL;
	X->private=private;
	return(X);
}

void X6502_Power(X6502 *X)
{
 _count=_tcount=_IRQlow=_PC=_A=_X=_Y=_S=_P=_PI=_DB=_jammed=0;
 _timestamp=0;
 SetReadHandler(X,0x0000,0xFFFF,0,0);
 SetWriteHandler(X,0x0000,0xFFFF,0,0);
 X6502_Reset(X);
}

void X6502_Run(X6502 *X, int32 cycles)
{
	if(X->PAL)
	 cycles*=15;          // 15*4=60
	else
	 cycles*=16;          // 16*4=64

	_count+=cycles;
	
	while(_count>0)
	{
	 int32 temp;
	 uint8 b1;

	 if(_IRQlow)
	 {
	  if(_IRQlow&FCEU_IQRESET)
	  {
	   _jammed=0;
	   _PI=_P=I_FLAG;
	   _IRQlow&=~FCEU_IQRESET;
	  }
	  else 
	  { 
	   if(!(_PI&I_FLAG) && !_jammed)
	   {
	    ADDCYC(7);
	    PUSH(_PC>>8);
	    PUSH(_PC);
	    PUSH((_P&~B_FLAG)|(U_FLAG));
	    _P|=I_FLAG;
	    _PC=RdMem(X,0xFFFE);
	    _PC|=RdMem(X,0xFFFF)<<8;
	   }
	  }
	  if(_count<=0) 
	  {
	   _PI=_P;
	   return;
	   } /* Should increase accuracy without a */
 	                                   /* major speed hit. */
	 }

	 _PI=_P;
	 b1=RdMem(X,_PC);
	 //printf("%04x:%02x\n",_PC,b1);
	 ADDCYC(CycTable[b1]);

	 temp=_tcount;
	 _tcount=0;

	 _timestamp += temp;
         FCEU_SoundCPUHook(X->private, temp);

	 //printf("%04x:$%02x\n",_PC,b1);
	 _PC++;
         switch(b1)
         {
          #include "ops.h"
         }  
	 if(_PC == 0x3800) break;
	}
}

void X6502_SpeedHack(X6502 *X, NESAPU *apu)
{
 int32 howmuch;

 _PI = _P;

 if(_IRQlow && !(_PI&I_FLAG))
  X6502_Run(X, 0);

 if((!apu->IRQFrameMode || (apu->DMCFormat&0xC0) == 0x80) && !(_PI&I_FLAG))
 {
  puts("abnormal skip");
  while(_count > 0)
  {
   _count -= 7 * 48;
   _timestamp += 7;
   FCEU_SoundCPUHook(X->private, 7);
   if(_IRQlow)
    X6502_Run(X, 0);
  }
 }
 else
 {
  howmuch = _count / 48;
  if(howmuch > 0)
  {
   _count -= howmuch * 48;
   _timestamp += howmuch;
   FCEU_SoundCPUHook(X->private, howmuch);
  }
 }
}
