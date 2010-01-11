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
	int32 CVBC[3];
	int32 vcount[3];
	int32 dcount[2];
	uint8 b3;
	int32 phaseacc;

	uint8 VPSG[8];
	uint8 VPSG2[4];
	int disabled;
	NESAPU *gapu;
} VRC6APU;

static void DoSQV1HQ(VRC6APU *VRC6Sound);
static void DoSQV2HQ(VRC6APU *VRC6Sound);
static void DoSawVHQ(VRC6APU *VRC6Sound);

static DECLFW(VRC6SW)
{
	VRC6APU *VRC6Sound=private;

        A&=0xF003;
        if(A>=0x9000 && A<=0x9002)
        {
	 DoSQV1HQ(VRC6Sound);
         VRC6Sound->VPSG[A&3]=V;
        }
        else if(A>=0xa000 && A<=0xa002)
        {
         DoSQV2HQ(VRC6Sound);
         VRC6Sound->VPSG[4|(A&3)]=V;
        }
        else if(A>=0xb000 && A<=0xb002)
        {
         DoSawVHQ(VRC6Sound);
         VRC6Sound->VPSG2[A&3]=V;
	}
}

static void DoSQVHQ(VRC6APU *VRC6Sound, int x)
{
 int32 V;
 int32 amp=((VRC6Sound->VPSG[x<<2]&15)<<8)*6/8;
     
 if(VRC6Sound->VPSG[(x<<2)|0x2]&0x80 && !(VRC6Sound->disabled&(0x1<<x)))  
 {
  if(VRC6Sound->VPSG[x<<2]&0x80)
  {
   for(V=VRC6Sound->CVBC[x];V<VRC6Sound->gapu->X->timestamp;V++)
    VRC6Sound->gapu->WaveHi[V]+=amp;
  }
  else   
  {
   int32 thresh=(VRC6Sound->VPSG[x<<2]>>4)&7;
   int32 curout;

   curout = 0;
   if(VRC6Sound->dcount[x]>thresh)        /* Greater than, not >=.  Important. */
    curout = amp;

   for(V=VRC6Sound->CVBC[x];V<VRC6Sound->gapu->X->timestamp;V++)
   {
    VRC6Sound->gapu->WaveHi[V]+=curout;
    VRC6Sound->vcount[x]--;
    if(VRC6Sound->vcount[x]<=0)            /* Should only be <0 in a few circumstances. */
    {
     VRC6Sound->vcount[x]=(VRC6Sound->VPSG[(x<<2)|0x1]|((VRC6Sound->VPSG[(x<<2)|0x2]&15)<<8))+1;
     VRC6Sound->dcount[x]=(VRC6Sound->dcount[x]+1)&15;
     curout = 0;
     if(VRC6Sound->dcount[x]>thresh)        /* Greater than, not >=.  Important. */
      curout = amp;
    }
   }
  }
 }
 VRC6Sound->CVBC[x]=VRC6Sound->gapu->X->timestamp;
}
   
static void DoSQV1HQ(VRC6APU *VRC6Sound)
{
 DoSQVHQ(VRC6Sound,0);
}

static void DoSQV2HQ(VRC6APU *VRC6Sound)
{ 
 DoSQVHQ(VRC6Sound, 1);
}
   
static void DoSawVHQ(VRC6APU *VRC6Sound)
{
 int32 V;
 int32 curout = (((VRC6Sound->phaseacc>>3)&0x1f)<<8)*6/8;
 if(VRC6Sound->VPSG2[2]&0x80 && !(VRC6Sound->disabled&0x4))
 {
  for(V=VRC6Sound->CVBC[2];V<VRC6Sound->gapu->X->timestamp;V++)
  {
   VRC6Sound->gapu->WaveHi[V]+= curout;
   VRC6Sound->vcount[2]--;
   if(VRC6Sound->vcount[2]<=0)
   {
    VRC6Sound->vcount[2]=(VRC6Sound->VPSG2[1]+((VRC6Sound->VPSG2[2]&15)<<8)+1)<<1;
    VRC6Sound->phaseacc+=VRC6Sound->VPSG2[0]&0x3f;
    VRC6Sound->b3++;
    if(VRC6Sound->b3==7)
    {
     VRC6Sound->b3=0;
     VRC6Sound->phaseacc=0;
    }
    curout = (((VRC6Sound->phaseacc>>3)&0x1f)<<8)*6/8;
   }
  }
 }
 VRC6Sound->CVBC[2]=VRC6Sound->gapu->X->timestamp;
}


void VRC6SoundHQ(void *private)
{
    DoSQV1HQ(private);
    DoSQV2HQ(private);
    DoSawVHQ(private);
}

void VRC6SyncHQ(void *private, int32 ts)
{
 int x;
 VRC6APU *VRC6Sound=private;

 for(x=0;x<3;x++) VRC6Sound->CVBC[x]=ts;
}

static void Kill(void *private)
{
 free(private);
}

static void Disable(void *private, int mask)
{
 VRC6APU *VRC6Sound=private;

 VRC6Sound->disabled=mask;
}


int NSFVRC6_Init(EXPSOUND *ep, NESAPU *apu)
{
        ep->HiFill=VRC6SoundHQ;
        ep->HiSync=VRC6SyncHQ;
	ep->private=malloc(sizeof(VRC6APU));
	ep->Kill=Kill;
	ep->Disable=Disable;
	ep->Channels = 3;

	memset(ep->private,0,sizeof(VRC6APU));

	((VRC6APU*)ep->private)->gapu=apu;
	SetWriteHandler(apu->X,0x8000,0xbfff,VRC6SW,ep->private);
	return(1);
}
