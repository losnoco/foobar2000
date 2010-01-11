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

#include <stdlib.h>
#include <string.h>

#include "../types.h"
#include "x6502.h"

#include "sound.h"
#include "nsf.h"
#include "../filter.h"

#define SQ_SHIFT	26
#define TRINPCM_SHIFT	18

static const int RectDuties[4]={1,2,4,6};

static const uint8 lengthtable[0x20]=
{
 0x5*2,0x7f*2,0xA*2,0x1*2,0x14*2,0x2*2,0x28*2,0x3*2,0x50*2,0x4*2,0x1E*2,0x5*2,0x7*2,0x6*2,0x0E*2,0x7*2,
 0x6*2,0x08*2,0xC*2,0x9*2,0x18*2,0xa*2,0x30*2,0xb*2,0x60*2,0xc*2,0x24*2,0xd*2,0x8*2,0xe*2,0x10*2,0xf*2
};

static const uint32 NoiseFreqTable[0x10]=
{
 2,4,8,0x10,0x20,0x30,0x40,0x50,0x65,0x7f,0xbe,0xfe,0x17d,0x1fc,0x3f9,0x7f2
};

static const uint32 NTSCDMCTable[0x10]=
{
 428,380,340,320,286,254,226,214,
 190,160,142,128,106, 84 ,72,54
};

static const uint32 PALDMCTable[0x10]=
{
 397, 353, 315, 297, 265, 235, 209, 198, 
 176, 148, 131, 118, 98, 78, 66, 50, 
};

static void DoPCM(NESAPU *apu);
static void DoSQ1(NESAPU *apu);
static void DoSQ2(NESAPU *apu);
static void DoTriangle(NESAPU *apu);
static void DoNoise(NESAPU *apu);

static void LoadDMCPeriod(NESAPU *apu, uint8 V)
{
 if(apu->X->PAL)
  apu->DMCPeriod=PALDMCTable[V];
 else
  apu->DMCPeriod=NTSCDMCTable[V];
}

static void PrepDPCM(NESAPU *apu)
{
 apu->DMCAddress=0x4000+(apu->DMCAddressLatch<<6);
 apu->DMCSize=(apu->DMCSizeLatch<<4)+1;
}

/* Instantaneous?  Maybe the new freq value is being calculated all of the time... */

static int FASTAPASS(2) CheckFreq(uint32 cf, uint8 sr)
{
 uint32 mod;
 if(!(sr&0x8))
 {
  mod=cf>>(sr&7);
  if((mod+cf)&0x800)
   return(0);
 }
 return(1);
}

static void SQReload(NESAPU *apu, int x, uint8 V)
{
           if(apu->EnabledChannels&(1<<x))
           {           
            if(x)
             DoSQ2(apu);
            else
             DoSQ1(apu);
            apu->lengthcount[x]=lengthtable[(V>>3)&0x1f];
	   }

           apu->sweepon[x]=apu->PSG[(x<<2)|1]&0x80;
           apu->curfreq[x]=apu->PSG[(x<<2)|0x2]|((V&7)<<8);
           apu->SweepCount[x]=((apu->PSG[(x<<2)|0x1]>>4)&7)+1;           

           apu->RectDutyCount[x]=7;
	   apu->EnvUnits[x].reloaddec=1;
}

static DECLFW(Write_PSG)
{
 NESAPU *apu=private;

 A&=0x1F;

 switch(A)
 {
  case 0x0:DoSQ1(apu);
	   apu->EnvUnits[0].Mode=(V&0x30)>>4;
	   apu->EnvUnits[0].Speed=(V&0xF);
           break;
  case 0x1:
           apu->sweepon[0]=V&0x80;
           break;
  case 0x2:
           DoSQ1(apu);
           apu->curfreq[0]&=0xFF00;
           apu->curfreq[0]|=V;
           break;
  case 0x3:
           SQReload(apu,0,V);
           break;
  case 0x4:           
	   DoSQ2(apu);
           apu->EnvUnits[1].Mode=(V&0x30)>>4;
           apu->EnvUnits[1].Speed=(V&0xF);
	   break;
  case 0x5:        
          apu->sweepon[1]=V&0x80;
          break;
  case 0x6:DoSQ2(apu);
          apu->curfreq[1]&=0xFF00;
          apu->curfreq[1]|=V;
          break;
  case 0x7:          
          SQReload(apu,1,V);
          break;
  case 0xa:DoTriangle(apu);
	   break;
  case 0xb:
          DoTriangle(apu);
	  if(apu->EnabledChannels&0x4)
           apu->lengthcount[2]=lengthtable[(V>>3)&0x1f];
	  apu->TriMode=1;	// Load mode
          break;
  case 0xC:DoNoise(apu);
           apu->EnvUnits[2].Mode=(V&0x30)>>4;
           apu->EnvUnits[2].Speed=(V&0xF);
           break;
  case 0xE:DoNoise(apu);
           break;
  case 0xF:
	   DoNoise(apu);
           if(apu->EnabledChannels&0x8)
	    apu->lengthcount[3]=lengthtable[(V>>3)&0x1f];
	   apu->EnvUnits[2].reloaddec=1;
           break;
 case 0x10:DoPCM(apu);
	   LoadDMCPeriod(apu,V&0xF);

	   if(apu->SIRQStat&0x80)
	   {
	    if(!(V&0x80))
	    {
	     X6502_IRQEnd(apu->X,FCEU_IQDPCM);
 	     apu->SIRQStat&=~0x80;
	    }
            else X6502_IRQBegin(apu->X,FCEU_IQDPCM);
	   }
	   break;
 }
 apu->PSG[A]=V;
}

static DECLFW(Write_DMCRegs)
{
 NESAPU *apu=private;
 A&=0xF;

 switch(A)
 {
  case 0x00:DoPCM(apu);
            LoadDMCPeriod(apu,V&0xF);

            if(apu->SIRQStat&0x80)
            {
             if(!(V&0x80))
             { 
              X6502_IRQEnd(apu->X,FCEU_IQDPCM);
              apu->SIRQStat&=~0x80;
             }
             else X6502_IRQBegin(apu->X,FCEU_IQDPCM);
            }
	    apu->DMCFormat=V;
	    break;
  case 0x01:DoPCM(apu);
	    apu->RawDALatch=V&0x7F;
	    break;
  case 0x02:apu->DMCAddressLatch=V;break;
  case 0x03:apu->DMCSizeLatch=V;break;
 }


}

static DECLFW(StatusWrite)
{
	NESAPU *apu=private;
	int x;

        DoSQ1(apu);
        DoSQ2(apu);
        DoTriangle(apu);
        DoNoise(apu);
        DoPCM(apu);
        for(x=0;x<4;x++)
         if(!(V&(1<<x))) apu->lengthcount[x]=0;   /* Force length counters to 0. */

        if(V&0x10)
        {
         if(!apu->DMCSize)
          PrepDPCM(apu);
        }
	else
	{
	 apu->DMCSize=0;
	}
	apu->SIRQStat&=~0x80;
        X6502_IRQEnd(apu->X,FCEU_IQDPCM);
	apu->EnabledChannels=V&0x1F;
}

static DECLFR(StatusRead)
{
   NESAPU *apu=private;
   int x;
   uint8 ret;

   ret=apu->SIRQStat;

   for(x=0;x<4;x++) ret|=apu->lengthcount[x]?(1<<x):0;
   if(apu->DMCSize) ret|=0x10;

   apu->SIRQStat&=~0x40;
   X6502_IRQEnd(apu->X,FCEU_IQFCOUNT);

   return ret;
}

static void FASTAPASS(2) FrameSoundStuff(NESAPU *apu, int V)
{
 int P;

 DoSQ1(apu);
 DoSQ2(apu);
 DoNoise(apu);
 DoTriangle(apu);

 if(!(V&1)) /* Envelope decay, linear counter, length counter, freq sweep */
 {
  if(!(apu->PSG[8]&0x80))
   if(apu->lengthcount[2]>0)
    apu->lengthcount[2]--;

  if(!(apu->PSG[0xC]&0x20))	/* Make sure loop flag is not set. */
   if(apu->lengthcount[3]>0)
    apu->lengthcount[3]--;

  for(P=0;P<2;P++)
  {
   if(!(apu->PSG[P<<2]&0x20))	/* Make sure loop flag is not set. */
    if(apu->lengthcount[P]>0)
     apu->lengthcount[P]--;            

   /* Frequency Sweep Code Here */
   /* xxxx 0000 */
   /* xxxx = hz.  120/(x+1)*/
   if(apu->sweepon[P])
   {
    int32 mod=0;

    if(apu->SweepCount[P]>0) apu->SweepCount[P]--; 
    if(apu->SweepCount[P]<=0)
    {
     apu->SweepCount[P]=((apu->PSG[(P<<2)+0x1]>>4)&7)+1; //+1;
     if(apu->PSG[(P<<2)+0x1]&0x8)
     {
      mod-=(P^1)+((apu->curfreq[P])>>(apu->PSG[(P<<2)+0x1]&7));          
      if(apu->curfreq[P] && (apu->PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
      {
       apu->curfreq[P]+=mod;
      }
     }
     else
     {
      mod=apu->curfreq[P]>>(apu->PSG[(P<<2)+0x1]&7);
      if((mod+apu->curfreq[P])&0x800)
      {
       apu->sweepon[P]=0;
       apu->curfreq[P]=0;
      }
      else
      {
       if(apu->curfreq[P] && (apu->PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
       {
        apu->curfreq[P]+=mod;
       }
      }     
     }
    }
   }
   else	/* Sweeping is disabled: */
   {
    //curfreq[P]&=0xFF00;
    //curfreq[P]|=apu->PSG[(P<<2)|0x2]; //|((apu->PSG[(P<<2)|3]&7)<<8); 
   }
  }
 }

 /* Now do envelope decay + linear counter. */

  if(apu->TriMode) // In load mode?
   apu->TriCount=apu->PSG[0x8]&0x7F;
  else if(apu->TriCount)
   apu->TriCount--;

  if(!(apu->PSG[0x8]&0x80))
   apu->TriMode=0;

  for(P=0;P<3;P++)
  {
   if(apu->EnvUnits[P].reloaddec)
   {
    apu->EnvUnits[P].decvolume=0xF;
    apu->EnvUnits[P].DecCountTo1=apu->EnvUnits[P].Speed+1;
    apu->EnvUnits[P].reloaddec=0;
    continue;
   }

   if(apu->EnvUnits[P].DecCountTo1>0) apu->EnvUnits[P].DecCountTo1--;
   if(apu->EnvUnits[P].DecCountTo1==0)
   {
    apu->EnvUnits[P].DecCountTo1=apu->EnvUnits[P].Speed+1;
    if(apu->EnvUnits[P].decvolume || (apu->EnvUnits[P].Mode&0x2))
    {
     apu->EnvUnits[P].decvolume--;
     apu->EnvUnits[P].decvolume&=0xF;
    }
   }
  }
}

void FrameSoundUpdate(NESAPU *apu)
{
 // Linear counter:  Bit 0-6 of $4008
 // Length counter:  Bit 4-7 of $4003, $4007, $400b, $400f

 if(!apu->fcnt && !(apu->IRQFrameMode&0x3))
 {
         apu->SIRQStat|=0x40;
         X6502_IRQBegin(apu->X,FCEU_IQFCOUNT);
 }

 if(apu->fcnt==3)
 {
	if(apu->IRQFrameMode&0x2)
	 apu->fhcnt+=apu->fhinc;
 }
 FrameSoundStuff(apu, apu->fcnt);
 apu->fcnt=(apu->fcnt+1)&3;
}


static INLINE void tester(NESAPU *apu)
{
 if(apu->DMCBitCount==0)
 {
  if(!apu->DMCHaveDMA)
   apu->DMCHaveSample=0;
  else
  {
   apu->DMCHaveSample=1;
   apu->DMCShift=apu->DMCDMABuf;
   apu->DMCHaveDMA=0;
  }
 }    
}

static INLINE void DMCDMA(NESAPU *apu)
{
  if(apu->DMCSize && !apu->DMCHaveDMA)
  {
   X6502_DMR(apu->X,0x8000+apu->DMCAddress);
   X6502_DMR(apu->X,0x8000+apu->DMCAddress);
   X6502_DMR(apu->X,0x8000+apu->DMCAddress);
   apu->DMCDMABuf=X6502_DMR(apu->X,0x8000+apu->DMCAddress);
   apu->DMCHaveDMA=1;
   apu->DMCAddress=(apu->DMCAddress+1)&0x7fff;
   apu->DMCSize--;
   if(!apu->DMCSize)
   {
    if(apu->DMCFormat&0x40)
     PrepDPCM(apu);
    else
    {
     apu->SIRQStat|=0x80;
     if(apu->DMCFormat&0x80)
      X6502_IRQBegin(apu->X,FCEU_IQDPCM);
    }
   }
 }
}

void FASTAPASS(2) FCEU_SoundCPUHook(void *private, int cycles)
{
 NESAPU *apu=((FESTALON_NSF *)private)->apu;
  
 apu->fhcnt-=cycles*48;
 while(apu->fhcnt<=0)
 {
  int32 rest = apu->fhcnt / 48;
  //printf("%8d:%8d\n",apu->X->timestamp,apu->X->timestamp+rest);
  apu->X->timestamp+=rest;	// Yet another ugly hack. 
  if(apu->X->timestamp < apu->lastpoo) puts("eep");
  FrameSoundUpdate(apu);
  apu->X->timestamp-=rest;
  apu->fhcnt+=apu->fhinc;
 }
 

 DMCDMA(apu);
 apu->DMCacc-=cycles;

 while(apu->DMCacc<=0)
 {
  DMCDMA(apu);
  if(apu->DMCHaveSample)
  {
   uint8 bah=apu->RawDALatch;
   int t=((apu->DMCShift&1)<<2)-2;
   int32 rest = apu->DMCacc;
   /* Unbelievably ugly hack */ 
   apu->X->timestamp+=rest;
   //printf("%8d:%8d\n",apu->X->timestamp,apu->ChannelBC[4]);
   DoPCM(apu);
   apu->X->timestamp-=rest;
   apu->RawDALatch+=t;
   if(apu->RawDALatch&0x80)
    apu->RawDALatch=bah;
  }

  apu->DMCacc+=apu->DMCPeriod;
  apu->DMCBitCount=(apu->DMCBitCount+1)&7;
  apu->DMCShift>>=1;  
  tester(apu);
 }
}

void DoPCM(NESAPU *apu)
{
 if(!(apu->disabled&0x10))
 {
  int32 count;
  int32 *D;
  int32 out;

  count = apu->X->timestamp - apu->ChannelBC[4];
  D = &apu->WaveHi[apu->ChannelBC[4]];
  out = apu->RawDALatch << TRINPCM_SHIFT;

  while(count > 0)
  {
   *D += out;
   D++;
   count--;
  }
  //for(V=apu->ChannelBC[4];V<apu->X->timestamp;V++)
  // apu->WaveHi[V]+=apu->RawDALatch<<TRINPCM_SHIFT;
 }
 apu->ChannelBC[4]=apu->X->timestamp;
}

/* This has the correct phase.  Don't mess with it. */
static INLINE void DoSQ(NESAPU *apu, int x)
{
   int32 V;
   int32 amp;
   int32 rthresh;
   int32 *D;
   int32 currdc;
   int32 cf;
   int32 rc;

   if(apu->disabled&(1<<x)) 
    goto endit;

   if(apu->curfreq[x]<8 || apu->curfreq[x]>0x7ff)
    goto endit;
   if(!CheckFreq(apu->curfreq[x],apu->PSG[(x<<2)|0x1]))
    goto endit;
   if(!apu->lengthcount[x])
    goto endit;

   if(apu->EnvUnits[x].Mode&0x1)
    amp=apu->EnvUnits[x].Speed;
   else
    amp=apu->EnvUnits[x].decvolume;
//   printf("%d\n",amp);
   amp<<=SQ_SHIFT;

   rthresh=RectDuties[(apu->PSG[(x<<2)]&0xC0)>>6];

   D=&apu->WaveHi[apu->ChannelBC[x]];
   V=apu->X->timestamp-apu->ChannelBC[x];
   
   currdc=apu->RectDutyCount[x];
   cf=(apu->curfreq[x]+1)*2;
   rc=apu->wlcount[x];

   while(V>0)
   {
    if(currdc<rthresh)
     *D+=amp;
    rc--;
    if(!rc)
    {
     int32 tmp;
     rc=cf;
     currdc=(currdc+1)&7;

     tmp = rc - 1;
     if((V-1) < rc) tmp = V - 1;

     if(tmp > 0)
     {
      rc -= tmp;
      if(currdc<rthresh)
      {
       do { *D += amp; V--; D++; tmp--; } while(tmp);
      }
      else
      { V -= tmp; D+= tmp; }
     }
    }
    V--;
    D++;
   }   
  
   apu->RectDutyCount[x]=currdc;
   apu->wlcount[x]=rc;

   endit:
   apu->ChannelBC[x]=apu->X->timestamp;
}

static void DoSQ1(NESAPU *apu)
{
 DoSQ(apu, 0);
}

static void DoSQ2(NESAPU *apu)
{
 DoSQ(apu, 1);
}

static void DoTriangle(NESAPU *apu)
{
 int32 V;
 int32 tcout;

 tcout=(apu->tristep&0xF);
 if(!(apu->tristep&0x10)) tcout^=0xF;
 tcout=tcout*3;	//(tcout<<1);
 tcout <<= TRINPCM_SHIFT;

 if(apu->disabled&4)       
 {
  apu->ChannelBC[2]=apu->X->timestamp;
  return;
 }

 if(!apu->lengthcount[2] || !apu->TriCount)
 {   				/* Counter is halted, but we still need to output. */
  for(V=apu->ChannelBC[2];V<apu->X->timestamp;V++)
   apu->WaveHi[V]+=tcout;
 }
 else
 {
  int32 wl = (apu->PSG[0xa]|((apu->PSG[0xb]&7)<<8))+1;
  for(V=apu->ChannelBC[2];V<apu->X->timestamp;V++)
  {
    apu->WaveHi[V]+=tcout;
    apu->wlcount[2]--;
    if(!apu->wlcount[2])
    {
     apu->wlcount[2]=wl;
     apu->tristep++;
     tcout=(apu->tristep&0xF);
     if(!(apu->tristep&0x10)) tcout^=0xF;
     tcout=tcout*3;
     tcout<<=TRINPCM_SHIFT;
    }
  }
 }
 apu->ChannelBC[2]=apu->X->timestamp;
}

static void DoNoise(NESAPU *apu)
{
 int32 V;
 int32 outo;
 uint32 amptab[2];
 int32 wl;

 if(apu->EnvUnits[2].Mode&0x1)
  amptab[0]=apu->EnvUnits[2].Speed;
 else
  amptab[0]=apu->EnvUnits[2].decvolume;

 amptab[0]<<=TRINPCM_SHIFT;
 amptab[1]=0;

 amptab[0]<<=1;

 if(apu->disabled&8) 
 {
  apu->ChannelBC[3]=apu->X->timestamp;
  return;
 }

 outo=amptab[apu->nreg&1]; //(nreg>>0xe)&1];

 if(!apu->lengthcount[3])
 {
  outo=amptab[0]=0;
 }

 wl = NoiseFreqTable[apu->PSG[0xE]&0xF]<<1;
 if(apu->PSG[0xE]&0x80)        // "short" noise
 {
  for(V=apu->ChannelBC[3];V<apu->X->timestamp;V++)
  {
   apu->WaveHi[V]+=outo;
   apu->wlcount[3]--;
   if(!apu->wlcount[3])
   {
    uint8 feedback;
    apu->wlcount[3] = wl;
    feedback=((apu->nreg>>8)&1)^((apu->nreg>>14)&1);
    apu->nreg=(apu->nreg<<1)+feedback;
    apu->nreg&=0x7fff;
    outo=amptab[(apu->nreg>>0xe)&1];
   }
  }
 }
 else
  for(V=apu->ChannelBC[3];V<apu->X->timestamp;V++)
  {
   apu->WaveHi[V]+=outo;
   apu->wlcount[3]--;
   if(!apu->wlcount[3])
   {
    uint8 feedback;
    apu->wlcount[3]=wl;
    feedback=((apu->nreg>>13)&1)^((apu->nreg>>14)&1);
    apu->nreg=(apu->nreg<<1)+feedback;
    apu->nreg&=0x7fff;
    outo=amptab[(apu->nreg>>0xe)&1];
   }
  }
 apu->ChannelBC[3]=apu->X->timestamp;
}

static DECLFW(Write_IRQFM)
{
 NESAPU *apu=private;

 V=(V&0xC0)>>6;
 apu->fcnt=0;
 if(V&0x2)  
  FrameSoundUpdate(apu);
 apu->fcnt=1;
 apu->fhcnt=apu->fhinc;
 X6502_IRQEnd(apu->X,FCEU_IQFCOUNT);
 apu->SIRQStat&=~0x40;
 apu->IRQFrameMode=V;
}

int FCEUSND_FlushEmulate(NESAPU *apu)
{
  int x;
  int32 end,left;

  if(!apu->X->timestamp) return(0);

  DoSQ1(apu);
  DoSQ2(apu);
  DoTriangle(apu);
  DoNoise(apu);
  DoPCM(apu);

  {
   for(x=0;x<apu->expcount;x++)
    if(apu->exp[x]->HiFill) apu->exp[x]->HiFill(apu->exp[x]->private);

   if(apu->ff->input_format == FFI_INT16)
   {
    int16 *tmpo=&((int16 *)apu->WaveHi)[apu->lastpoo];
    uint32 *intmpo = &apu->WaveHi[apu->lastpoo];

    if(apu->expcount)
     for(x=apu->X->timestamp- apu->lastpoo;x;x--)
     {
      uint32 b=*intmpo;
   
      *(int16 *)tmpo=(*intmpo & 0x3FFFF) + apu->wlookup2[(b>>TRINPCM_SHIFT)&255]+apu->wlookup1[b>>SQ_SHIFT] - 32768;
      tmpo++;
      intmpo++;
     }
    else
     for(x=apu->X->timestamp- apu->lastpoo;x;x--)
     {
      uint32 b=*intmpo;
      *(int16 *)tmpo=apu->wlookup2[(b>>TRINPCM_SHIFT)&255]+apu->wlookup1[b>>SQ_SHIFT] - 32768;
      tmpo++;
      intmpo++;
     }
   }
   else
   {
    int32 *tmpo=&apu->WaveHi[apu->lastpoo];
    if(apu->expcount)
     for(x=apu->X->timestamp- apu->lastpoo;x;x--)
     {
      uint32 b=*tmpo;
   
      *(float *)tmpo=(*tmpo & 0x3FFFF) + apu->wlookup2[(b>>TRINPCM_SHIFT)&255]+apu->wlookup1[b>>SQ_SHIFT];
      tmpo++;
     }
    else
     for(x=apu->X->timestamp- apu->lastpoo;x;x--)
     {
      uint32 b=*tmpo;
      *(float *)tmpo=apu->wlookup2[(b>>TRINPCM_SHIFT)&255]+apu->wlookup1[b>>SQ_SHIFT];
      tmpo++;
     }
   }
	#ifdef MOO
   {
    int x;
    static int osc=0;

    if(apu->ff->input_format == FFI_INT16)
     for(x=apu->lastpoo;x<apu->X->timestamp;x++) { ((int16 *)apu->WaveHi)[x] = (osc&64)*64; osc++; }
    else
     for(x=apu->lastpoo;x<apu->X->timestamp;x++) { *(float *)&apu->WaveHi[x] = (osc&64)*64; osc++; }
   }
	#endif
   end=FESTAFILT_Do(apu->ff, (float *)apu->WaveHi,apu->WaveFinal,apu->WaveFinalLen, apu->X->timestamp,&left, 0);
   if(apu->ff->input_format == FFI_INT16)
   {
    memmove(apu->WaveHi,(int16 *)apu->WaveHi+apu->X->timestamp-left,left*sizeof(uint16));
    memset((int16 *)apu->WaveHi+left,0,sizeof(apu->WaveHi)-left*sizeof(uint16));
   }
   else
   {
    memmove(apu->WaveHi,apu->WaveHi+apu->X->timestamp-left,left*sizeof(uint32));
    memset(apu->WaveHi+left,0,sizeof(apu->WaveHi)-left*sizeof(uint32));
   }

   for(x=0;x<apu->expcount;x++)
    if(apu->exp[x]->HiFill) apu->exp[x]->HiSync(apu->exp[x]->private,left); 
   for(x=0;x<5;x++)
    apu->ChannelBC[x]=left;
  }

  apu->X->timestampbase+=apu->X->timestamp;
  apu->X->timestamp=left;
  apu->X->timestampbase-=apu->X->timestamp;
  apu->lastpoo=apu->X->timestamp;
  apu->inbuf=end;

  return(end);
}

/* FIXME:  Find out what sound registers get reset on reset.  I know $4001/$4005 don't,
due to that whole MegaMan 2 Game Genie thing.
*/

void FCEUSND_Reset(NESAPU *apu)
{
        int x;

	apu->IRQFrameMode=0x0;
        apu->fhcnt=apu->fhinc;
        apu->fcnt=0;

        apu->nreg=1;
        for(x=0;x<2;x++)
	{
         apu->wlcount[x]=2048;
	 apu->sweepon[x]=0;
	 apu->curfreq[x]=0;
	}
        apu->wlcount[2]=1;	//2048;
        apu->wlcount[3]=2048;
	apu->DMCHaveDMA=apu->DMCHaveSample=0;
	apu->SIRQStat=0x00;

	apu->RawDALatch=0x00;
	apu->TriCount=0;
	apu->TriMode=0;
	apu->tristep=0;
	apu->EnabledChannels=0;
	for(x=0;x<4;x++)
	 apu->lengthcount[x]=0;

	apu->DMCAddressLatch=0;
	apu->DMCSizeLatch=0;
	apu->DMCFormat=0;
	apu->DMCAddress=0;
	apu->DMCSize=0;
	apu->DMCShift=0;
}

void FCEUSND_Power(NESAPU *apu)
{
        int x;
 
	SetWriteHandler(apu->X,0x4000,0x400F,Write_PSG,apu);
	SetWriteHandler(apu->X,0x4010,0x4013,Write_DMCRegs,apu);
	SetWriteHandler(apu->X,0x4017,0x4017,Write_IRQFM,apu);
        
	SetWriteHandler(apu->X,0x4015,0x4015,StatusWrite,apu);
	SetReadHandler(apu->X,0x4015,0x4015,StatusRead,apu);

        memset(apu->PSG,0x00,sizeof(apu->PSG));
	FCEUSND_Reset(apu);

        memset(apu->WaveHi,0,sizeof(apu->WaveHi));
	memset(&apu->EnvUnits,0,sizeof(apu->EnvUnits));

        for(x=0;x<5;x++)
         apu->ChannelBC[x]=0;
        apu->lastpoo=0;
        LoadDMCPeriod(apu, apu->DMCFormat&0xF);
}


void FCEUSND_Kill(NESAPU *apu)
{
 int x;

 for(x=0;x<apu->expcount;x++)
 {
  if(apu->exp[x]->Kill) apu->exp[x]->Kill(apu->exp[x]->private);
  free(apu->exp[x]);
 }
 if(apu->ff)
  FESTAFILT_Kill(apu->ff);
 if(apu->WaveFinal)
  free(apu->WaveFinal);
 free(apu->realmem);
}


NESAPU *FCEUSND_Init(X6502 *X)
{
  NESAPU *apu;
  long x;
  void *realmem;

  if(!(apu=realmem=malloc(16 + sizeof(NESAPU))))
   return(0);

  apu = (NESAPU *)(unsigned long)(((unsigned long)apu+0xFLL)&~0xFLL);
  memset(apu,0,sizeof(NESAPU));
  apu->realmem = realmem;

  apu->X=X;

  apu->fhinc=X->PAL?16626:14915;	// *2 CPU clock rate
  apu->fhinc*=24;

  apu->wlookup1[0]=0;
  for(x=1;x<32;x++)
  {
   apu->wlookup1[x]=(double)16*16*16*4*95.52/((double)8128/(double)x+100);
  }

  apu->wlookup2[0]=0;
  for(x=1;x<203;x++)
  {
   apu->wlookup2[x]=(double)16*16*16*4*163.67/((double)24329/(double)x+100);
  }

  LoadDMCPeriod(apu, apu->DMCFormat&0xF);	

  return(apu);
}

void FCEUSND_Disable(NESAPU *apu, int d)
{
 int x;

 apu->disabled=d&0x1F;
  
 d >>= 5;

 for(x=0;x<apu->expcount;x++)
 {
  if(apu->exp[x]->Disable) apu->exp[x]->Disable(apu->exp[x]->private, d);
  d >>= apu->exp[x]->Channels;
 }
}

void FCEUSND_AddExp(NESAPU *apu, EXPSOUND *exp)
{
 if(apu->expcount < 16)
  apu->exp[apu->expcount++]=exp;
}

void FESTANSF_SetVolume(FESTALON_NSF *fe, uint32 volume)
{
 fe->apu->ff->SoundVolume=volume;
}


int FESTANSF_SetLowpass(FESTALON_NSF *nfe, int on, uint32 corner, uint32 order)
{
 return(FESTAFILT_SetLowpass(nfe->apu->ff, on, corner, order));
}

int FESTANSF_SetSound(FESTALON_NSF *nfe, uint32 rate, int quality)
{
 if(nfe->apu->ff)
 {
  FESTAFILT_Kill(nfe->apu->ff);
  nfe->apu->ff = 0;
 }
 if(!(nfe->apu->ff = FESTAFILT_Init(rate,nfe->X->PAL?PAL_CPU:NTSC_CPU, nfe->X->PAL, quality)))
  return(0);

 nfe->apu->WaveFinalLen = rate / (nfe->X->PAL?50:60) * 2;  // * 2 for extra room
 if(nfe->apu->WaveFinal) free(nfe->apu->WaveFinal);
 if(!(nfe->apu->WaveFinal = malloc(sizeof(float) * nfe->apu->WaveFinalLen)))
  return(0);

 return(1);
}
