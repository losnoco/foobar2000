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

#include "cart.h"
#include "memory.h"

/* 16 are (sort of) reserved for UNIF/iNES and 16 to map other stuff. */

static INLINE void setpageptr(NESCART *ca, int s, uint32 A, uint8 *p, int ram)
{
 uint32 AB=A>>11;
 int x;

 if(p)
  for(x=(s>>1)-1;x>=0;x--)
  {
   ca->PRGIsRAM[AB+x]=ram;
   ca->Page[AB+x]=p-A;
  }
 else
  for(x=(s>>1)-1;x>=0;x--)
  {
   ca->PRGIsRAM[AB+x]=0;
   ca->Page[AB+x]=0;
  }
}

static uint8 nothing[8192];

void FESTAC_Kill(NESCART *ca)
{
 free(ca);
}

NESCART *FESTAC_Init(void)
{
 int x;
 NESCART *ca;

 if(!(ca=malloc(sizeof(NESCART)))) return(0);

 memset(ca,0,sizeof(NESCART));

 for(x=0;x<32;x++)
 {
  ca->Page[x]=nothing-x*2048;
  ca->PRGptr[x]=0;
  ca->PRGsize[x]=0;
 }
 return(ca);
}

void FESTAC_SetupPRG(NESCART *ca, int chip, uint8 *p, uint32 size, int ram)
{
 ca->PRGptr[chip]=p;
 ca->PRGsize[chip]=size;

 ca->PRGmask2[chip]=(size>>11)-1;
 ca->PRGmask4[chip]=(size>>12)-1;
 ca->PRGmask8[chip]=(size>>13)-1;
 ca->PRGmask16[chip]=(size>>14)-1;
 ca->PRGmask32[chip]=(size>>15)-1; 

 ca->PRGram[chip]=ram?1:0;
}

DECLFR(CartBR)
{
 NESCART *ca=private;

 return ca->Page[A>>11][A];
}

DECLFW(CartBW)
{
 NESCART *ca=private;

 if(ca->PRGIsRAM[A>>11] && ca->Page[A>>11])
  ca->Page[A>>11][A]=V;
}

DECLFR(CartBROB)
{
 NESCART *ca=private;

 if(!ca->Page[A>>11]) return(DB);
 return ca->Page[A>>11][A];
}

void FASTAPASS(3)  setprg2r(NESCART *ca, int r, unsigned int A, unsigned int V)
{
  V&=ca->PRGmask2[r];

  setpageptr(ca,2,A,ca->PRGptr[r]?(&ca->PRGptr[r][V<<11]):0,ca->PRGram[r]);
}

void FASTAPASS(2) setprg2(NESCART *ca, uint32 A, uint32 V)
{
 setprg2r(ca,0,A,V);
}

void FASTAPASS(3) setprg4r(NESCART *ca, int r, unsigned int A, unsigned int V)
{
  V&=ca->PRGmask4[r];
  setpageptr(ca,4,A,ca->PRGptr[r]?(&ca->PRGptr[r][V<<12]):0,ca->PRGram[r]);
}

void FASTAPASS(2) setprg4(NESCART *ca, uint32 A, uint32 V)
{
 setprg4r(ca,0,A,V);
}

void FASTAPASS(3) setprg8r(NESCART *ca, int r, unsigned int A, unsigned int V)
{
  if(ca->PRGsize[r]>=8192)
  {
   V&=ca->PRGmask8[r];
   setpageptr(ca,8,A,ca->PRGptr[r]?(&ca->PRGptr[r][V<<13]):0,ca->PRGram[r]);
  }
  else
  {
   uint32 VA=V<<2;
   int x;
   for(x=0;x<4;x++)
    setpageptr(ca,2,A+(x<<11),ca->PRGptr[r]?(&ca->PRGptr[r][((VA+x)&ca->PRGmask2[r])<<11]):0,ca->PRGram[r]);
  }
}

void FASTAPASS(2) setprg8(NESCART *ca, uint32 A, uint32 V)
{
 setprg8r(ca,0,A,V);
}

void FASTAPASS(3) setprg16r(NESCART *ca, int r, unsigned int A, unsigned int V)
{
  if(ca->PRGsize[r]>=16384)
  {
   V&=ca->PRGmask16[r];
   setpageptr(ca,16,A,ca->PRGptr[r]?(&ca->PRGptr[r][V<<14]):0,ca->PRGram[r]);
  }
  else
  {
   uint32 VA=V<<3;
   int x;

   for(x=0;x<8;x++)
    setpageptr(ca,2,A+(x<<11),ca->PRGptr[r]?(&ca->PRGptr[r][((VA+x)&ca->PRGmask2[r])<<11]):0,ca->PRGram[r]);
  }
}

void FASTAPASS(2) setprg16(NESCART *ca, uint32 A, uint32 V)
{
 setprg16r(ca,0,A,V);
}

void FASTAPASS(3) setprg32r(NESCART *ca, int r,unsigned int A, unsigned int V)
{
  if(ca->PRGsize[r]>=32768)
  {
   V&=ca->PRGmask32[r];
   setpageptr(ca,32,A,ca->PRGptr[r]?(&ca->PRGptr[r][V<<15]):0,ca->PRGram[r]);
  }
  else
  {
   uint32 VA=V<<4;
   int x;

   for(x=0;x<16;x++)
    setpageptr(ca,2,A+(x<<11),ca->PRGptr[r]?(&ca->PRGptr[r][((VA+x)&ca->PRGmask2[r])<<11]):0,ca->PRGram[r]);
  }
}

void FASTAPASS(2) setprg32(NESCART *ca, uint32 A, uint32 V)
{
 setprg32r(ca,0,A,V);
}
