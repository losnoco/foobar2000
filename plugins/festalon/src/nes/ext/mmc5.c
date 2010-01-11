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
        uint16 wl[2];
        uint8 env[2];
        uint8 enable;
        uint8 running; 
        uint8 raw;
        uint8 rawcontrol;
        uint8 mul[2];

	uint8 ExRAM[1024];
        
        int32 dcount[2];
        
        int32 BC[3];
        int32 vcount[2];

	int disabled;
	NESAPU *gapu;
} MMC5APU;

static void Do5SQHQ(MMC5APU *, int P);
static void MMC5RunSoundHQ(void *);

static DECLFW(Mapper5_write)
{
 MMC5APU *MMC5Sound=private;
 switch(A)
 {
  case 0x5205:MMC5Sound->mul[0]=V;break;
  case 0x5206:MMC5Sound->mul[1]=V;break;
 }
}

static DECLFW(MMC5_ExRAMWr)
{
 MMC5APU *MMC5Sound=private;

 MMC5Sound->ExRAM[A&0x3ff]=V;
}

static DECLFR(MMC5_ExRAMRd)
{
 MMC5APU *MMC5Sound=private;

 return MMC5Sound->ExRAM[A&0x3ff];
}

static DECLFR(MMC5_read)
{
 MMC5APU *MMC5Sound=private;
 switch(A)
 {
  case 0x5205:return (MMC5Sound->mul[0]*MMC5Sound->mul[1]);
  case 0x5206:return ((MMC5Sound->mul[0]*MMC5Sound->mul[1])>>8);
 }
 return(DB);
}

static void Do5PCMHQ(MMC5APU *MMC5Sound)
{
   int32 V;
   if(!(MMC5Sound->rawcontrol&0x40) && MMC5Sound->raw && !(MMC5Sound->disabled&0x4))
    for(V=MMC5Sound->BC[2];V<MMC5Sound->gapu->X->timestamp;V++)
     MMC5Sound->gapu->WaveHi[V]+=MMC5Sound->raw<<5;
   MMC5Sound->BC[2]=MMC5Sound->gapu->X->timestamp;
} 


static DECLFW(Mapper5_SW)
{
 MMC5APU *MMC5Sound=private;

 A&=0x1F;

 switch(A)
 {
  case 0x10:Do5PCMHQ(MMC5Sound);MMC5Sound->rawcontrol=V;break;
  case 0x11:Do5PCMHQ(MMC5Sound);MMC5Sound->raw=V;break;

  case 0x0:
  case 0x4:
	   Do5SQHQ(MMC5Sound,A>>2);
           MMC5Sound->env[A>>2]=V;
           break;
  case 0x2:
  case 0x6:Do5SQHQ(MMC5Sound,A>>2);
           MMC5Sound->wl[A>>2]&=~0x00FF;
           MMC5Sound->wl[A>>2]|=V&0xFF;
           break;
  case 0x3:
  case 0x7://printf("%04x:$%02x\n",A,V>>3);
	   MMC5Sound->wl[A>>2]&=~0x0700;
           MMC5Sound->wl[A>>2]|=(V&0x07)<<8;           
	   MMC5Sound->running|=1<<(A>>2);
	   break;
  case 0x15:
            Do5SQHQ(MMC5Sound,0);
            Do5SQHQ(MMC5Sound,1);
	    MMC5Sound->running&=V;
            MMC5Sound->enable=V;
            break;
 }
}

static void Do5SQHQ(MMC5APU *MMC5Sound, int P)
{
 static const int tal[4]={1,2,4,6};
 int32 V,amp,rthresh,wl;
  
 wl=MMC5Sound->wl[P]+1;
 amp=((MMC5Sound->env[P]&0xF)<<8);
 rthresh=tal[(MMC5Sound->env[P]&0xC0)>>6];
     
 if(wl>=8 && (MMC5Sound->running&(P+1)) && !(MMC5Sound->disabled&(0x1<<P)))
 {
  int dc,vc;
  int32 curout;  
  wl<<=1;

  dc=MMC5Sound->dcount[P];   
  vc=MMC5Sound->vcount[P];

  curout = 0;
  if(dc<rthresh) curout = amp;

  for(V=MMC5Sound->BC[P];V<MMC5Sound->gapu->X->timestamp;V++)
  {
    MMC5Sound->gapu->WaveHi[V]+=curout;
    //if(dc<rthresh)
    // MMC5Sound->gapu->WaveHi[V]+=amp;
    vc--;
    if(vc<=0)   /* Less than zero when first started. */
    {
     vc=wl;
     dc=(dc+1)&7;
     curout = 0;
     if(dc<rthresh) curout = amp;
    }
  }  
  MMC5Sound->dcount[P]=dc;
  MMC5Sound->vcount[P]=vc;
 }
 MMC5Sound->BC[P]=MMC5Sound->gapu->X->timestamp;
}

static void MMC5RunSoundHQ(void *private)
{  
  Do5SQHQ(private, 0);
  Do5SQHQ(private, 1);
  Do5PCMHQ(private);
}

static void MMC5HiSync(void *private, int32 ts)
{
 MMC5APU *MMC5Sound=private;
 int x;
 for(x=0;x<3;x++) MMC5Sound->BC[x]=ts;
}

static void Kill(void *private)
{
 free(private);
}

static void Disable(void *private, int mask)
{
 MMC5APU *MMC5Sound=private;

 MMC5Sound->disabled=mask;
}

int NSFMMC5_Init(EXPSOUND *ep, NESAPU *apu)
{
  MMC5APU *MMC5Sound;

  MMC5Sound=malloc(sizeof(MMC5APU));
  memset(MMC5Sound,0,sizeof(MMC5APU));
  MMC5Sound->gapu=apu;

  ep->private=MMC5Sound;

  ep->HiSync=MMC5HiSync;
  ep->HiFill=MMC5RunSoundHQ;
  ep->Kill=Kill;
  ep->Disable=Disable;
  ep->Channels = 3;

  SetWriteHandler(apu->X,0x5c00,0x5fef,MMC5_ExRAMWr,MMC5Sound);
  SetReadHandler(apu->X,0x5c00,0x5fef,MMC5_ExRAMRd,MMC5Sound); 
  SetWriteHandler(apu->X,0x5000,0x5015,Mapper5_SW,MMC5Sound);
  SetWriteHandler(apu->X,0x5205,0x5206,Mapper5_write,MMC5Sound);
  SetReadHandler(apu->X,0x5205,0x5206,MMC5_read,MMC5Sound);

  return(1);
}
