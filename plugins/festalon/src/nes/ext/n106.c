/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"

typedef struct {
	uint8 IRAM[128];
	uint8 dopol;

	uint32 FreqCache[8];
	uint32 EnvCache[8];
	uint32 LengthCache[8];

	uint32 PlayIndex[8];
	int32 vcount[8];
	int32 CVBC;

	int disabled;
	NESAPU *gapu;
} N106APU;

static void DoNamcoSoundHQ(void *);
static void SyncHQ(void *, int32 ts);

static DECLFR(Namco_Read4800)
{
	N106APU *N106Sound=private;

	uint8 ret=N106Sound->IRAM[N106Sound->dopol&0x7f];

	/* Maybe I should call DoNamcoSoundHQ() here? */
	if(N106Sound->dopol&0x80)
	 N106Sound->dopol=(N106Sound->dopol&0x80)|((N106Sound->dopol+1)&0x7f);
	return ret;
}

static void FixCache(N106APU * N106Sound, int a,int V)
{
                     int w=(a>>3)&0x7;
                     switch(a&0x07)
                     {
                      case 0x00:N106Sound->FreqCache[w]&=~0x000000FF;N106Sound->FreqCache[w]|=V;break;
                      case 0x02:N106Sound->FreqCache[w]&=~0x0000FF00;N106Sound->FreqCache[w]|=V<<8;break;
                      case 0x04:N106Sound->FreqCache[w]&=~0x00030000;N106Sound->FreqCache[w]|=(V&3)<<16;
                                N106Sound->LengthCache[w]=(8-((V>>2)&7))<<2;
                                break;
                      case 0x07:N106Sound->EnvCache[w]=(double)(V&0xF)*576716;break;
                     }

}

static DECLFW(Mapper19_write)
{
	N106APU *N106Sound=private;
	A&=0xF800;

        switch(A)
	{
	 case 0x4800:
		   if(N106Sound->dopol&0x40)
                   {
		    DoNamcoSoundHQ(N106Sound);
		    FixCache(N106Sound, N106Sound->dopol,V);
                   }
		   N106Sound->IRAM[N106Sound->dopol&0x7f]=V;
		   
                   if(N106Sound->dopol&0x80)
                    N106Sound->dopol=(N106Sound->dopol&0x80)|((N106Sound->dopol+1)&0x7f);
                   break;

        case 0xf800: N106Sound->dopol=V;break;
        }
}

#define TOINDEX	(16+1)

// 16:15
static void SyncHQ(void *private, int32 ts)
{
 N106APU *N106Sound=private;
 N106Sound->CVBC=ts;
}


/* Things to do:
	1	Read freq low
	2	Read freq mid
	3	Read freq high
	4	Read envelope
	...?
*/

static INLINE uint32 FetchDuff(uint8 *IRAM, uint32 P, uint32 envelope, uint32 PlayIndex)
{
    uint32 duff;
    duff=IRAM[((IRAM[0x46+(P<<3)]+(PlayIndex>>TOINDEX))&0xFF)>>1];
    if((IRAM[0x46+(P<<3)]+(PlayIndex>>TOINDEX))&1)
     duff>>=4;
    duff&=0xF;
    duff=(duff*envelope)>>16;
    return(duff);
}

static void DoNamcoSoundHQ(void *private)
{
 N106APU *N106Sound=private;
 int32 P, V;
 int32 cyclesuck;
 uint8 *IRAM = N106Sound->IRAM;
 int32 timestamp = N106Sound->gapu->X->timestamp;
 cyclesuck=(((IRAM[0x7F]>>4)&7)+1)*15;

 for(P=7;P>=(7-((IRAM[0x7F]>>4)&7));P--)
 {
  int32 *WaveHi = &N106Sound->gapu->WaveHi[N106Sound->CVBC];
  if((IRAM[0x44+(P<<3)]&0xE0) && (IRAM[0x47+(P<<3)]&0xF) && !(N106Sound->disabled&(0x1<<P)))
  {
   uint32 freq;
   int32 vco;
   uint32 duff2,lengo,envelope;
   uint32 PlayIndex;

   vco=N106Sound->vcount[P];
   freq=N106Sound->FreqCache[P];
   envelope=N106Sound->EnvCache[P];
   lengo=N106Sound->LengthCache[P];
   PlayIndex = N106Sound->PlayIndex[P];

   duff2=FetchDuff(IRAM, P,envelope, PlayIndex);

   //V = timestamp - N106Sound->CVBC;
   for(V=N106Sound->CVBC;V<timestamp;V++)
   //for(;V;V--)
   {
    *WaveHi+=duff2;
    if(!vco)
    {
     PlayIndex+=freq;
     while((PlayIndex>>TOINDEX)>=lengo)
      PlayIndex-=lengo<<TOINDEX;
     duff2=FetchDuff(IRAM, P,envelope, PlayIndex);
     vco = cyclesuck;
    }
    vco--;

    *WaveHi+=duff2;
    if(!vco)
    {
     PlayIndex+=freq;
     while((PlayIndex>>TOINDEX)>=lengo)
      PlayIndex-=lengo<<TOINDEX;
     duff2=FetchDuff(IRAM, P,envelope, PlayIndex);
     vco=cyclesuck;
    }
    WaveHi++;
    vco--;
   }
   N106Sound->vcount[P]=vco;
   N106Sound->PlayIndex[P] = PlayIndex;
  }
 }
 N106Sound->CVBC=timestamp;
}

static void Kill(void *private)
{
 free(private);
}


static void Disable(void *private, int mask)
{
 N106APU *N106Sound=private;

 N106Sound->disabled=mask;
}


int NSFN106_Init(EXPSOUND *ep, NESAPU *apu)
{
  N106APU *N106Sound;

  N106Sound=malloc(sizeof(N106APU));
  memset(N106Sound,0,sizeof(N106APU));
  N106Sound->gapu=apu;

  SetWriteHandler(apu->X,0xf800,0xffff,Mapper19_write,N106Sound);
  SetWriteHandler(apu->X,0x4800,0x4fff,Mapper19_write,N106Sound);
  SetReadHandler(apu->X,0x4800,0x4fff,Namco_Read4800,N106Sound);

  ep->private=N106Sound;
  ep->HiFill=DoNamcoSoundHQ;
  ep->HiSync=SyncHQ;
  ep->Kill=Kill;
  ep->Disable=Disable;
  ep->Channels = 8;

  return(1);
}

