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
	uint8 index;
	uint8 PSG[0x10];
	int32 vcount[3];
	int32 dcount[3]; 
	int CAYBC[3];             
	NESAPU *gapu;
	int disabled;
} FMEAPU;

static void AYSoundHQ(void *private);
static void DoAYSQHQ(FMEAPU *, int x);

static DECLFW(Mapper69_SWL)
{
 FMEAPU *FMESound=private;

 FMESound->index=V&0xF;
}

static DECLFW(Mapper69_SWH)
{
	FMEAPU *FMESound=private;
	int x;

        switch(FMESound->index)
        {
              case 0:
              case 1:
              case 8:DoAYSQHQ(FMESound,0);break;
              case 2:
              case 3:
              case 9:DoAYSQHQ(FMESound,1);break;
              case 4:
              case 5:
              case 10:DoAYSQHQ(FMESound,2);break;
              case 7:
		     for(x=0;x<2;x++)
  		      DoAYSQHQ(FMESound,x);
		     break;
             }
	FMESound->PSG[FMESound->index]=V; 
}

static void DoAYSQHQ(FMEAPU *FMESound, int x) 
{
 int32 V;
 int32 freq=((FMESound->PSG[x<<1]|((FMESound->PSG[(x<<1)+1]&15)<<8))+1)<<4;
 int32 amp=(FMESound->PSG[0x8+x]&15)<<6;
 int32 curout;
 int32 timestamp = FMESound->gapu->X->timestamp;
 int32 *WaveHi;
 amp+=amp>>1;

 if(!(FMESound->PSG[0x7]&(1<<x)) && !(FMESound->disabled&(0x1<<x)))
 {
  int32 vcount = FMESound->vcount[x];
  int dcount = FMESound->dcount[x];
  curout = FMESound->dcount[x] * amp;

  WaveHi = FMESound->gapu->WaveHi;
  for(V=FMESound->CAYBC[x];V<timestamp;V++)
  {
   WaveHi[V]+=curout;
   vcount--;

   if(vcount<=0)
   {
    dcount^=1;
    curout ^= amp;
    vcount=freq;
   }
  } 
  FMESound->vcount[x] = vcount;
  FMESound->dcount[x] = dcount;
 }
 FMESound->CAYBC[x]=FMESound->gapu->X->timestamp;
}

static void AYSoundHQ(void *private)
{
    DoAYSQHQ(private, 0);
    DoAYSQHQ(private, 1);
    DoAYSQHQ(private, 2);
}

static void AYHiSync(void *private, int32 ts)
{
 int x;
 FMEAPU *FMESound=private;

 for(x=0;x<3;x++)
  FMESound->CAYBC[x]=ts;
}

static void Kill(void *private)
{
 free(private);
}

static void Disable(void *private, int mask)
{
 FMEAPU *FMESound=private;
 FMESound->disabled=mask;
}

int NSFAY_Init(EXPSOUND *ep, NESAPU *apu)
{
 FMEAPU *FMESound;

 FMESound=malloc(sizeof(FMEAPU));
 memset(FMESound,0,sizeof(FMEAPU));
 FMESound->gapu=apu;

 ep->private=FMESound;
 ep->HiFill=AYSoundHQ;
 ep->Kill=Kill;
 ep->HiSync=AYHiSync;
 ep->Disable=Disable;
 ep->Channels = 3;

 SetWriteHandler(apu->X,0xc000,0xdfff,Mapper69_SWL,ep->private);
 SetWriteHandler(apu->X,0xe000,0xffff,Mapper69_SWH,ep->private);

 return(1);
}

