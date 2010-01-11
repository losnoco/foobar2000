#include "shared.h"
#include <math.h>
#include <string.h>
#include "../filter.h"

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown routines                                           */
/*--------------------------------------------------------------------------*/


static int32 dbtable[32][32];

static inline void redo_ddacache(FESTALON_HES *hes, psg_channel *ch)
{
 static const int scale_tab[] = {
	0x00, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
	0x10, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F
	};

 int al,lal,ral,lmal,rmal;
 int vll,vlr;

 al = ch->control & 0x1F;
 
 lal = scale_tab[(ch->balance >> 4) & 0xF];
 ral = scale_tab[(ch->balance >> 0) & 0xF];

 lmal = scale_tab[(hes->psg.globalbalance >> 4) & 0xF];
 rmal = scale_tab[(hes->psg.globalbalance >> 0) & 0xF];

 vll = (0x1F - lal) + (0x1F - al) + (0x1F - lmal);
 if(vll > 0x1F) vll = 0x1F;

 vlr = (0x1F - ral) + (0x1F - al) + (0x1F - rmal);
 if(vlr > 0x1F) vlr = 0x1F;

 ch->dda_cache[0] = dbtable[ch->dda][vll];
 ch->dda_cache[1] = dbtable[ch->dda][vlr];
}


int psg_init(FESTALON_HES *hes)
{
    int x;

    for(x=0;x<32;x++)
    {
     int y;
     double flub;

     flub = 1;

     if(x)
      flub /= pow(2, (double)1/4*x);		// ~1.5dB reduction per increment of x
     for(y=0;y<32;y++)
      dbtable[y][x] = (flub * (y - 0x10) * 128);
     //printf("%f\n",flub);
    }
    memset(&hes->psg, 0, sizeof(hes->psg));

    return (0);
}

void psg_reset(FESTALON_HES *hes)
{
    memset(&hes->psg.channel, 0, sizeof(hes->psg.channel));
}

void psg_shutdown(FESTALON_HES *hes)
{
	int x;

	for(x=0;x<2;x++)
	{
	 if(hes->psg.ff[x])
	 {
	  FESTAFILT_Kill(hes->psg.ff[x]);
	  hes->psg.ff[x] = 0;
	 }
	 if(hes->psg.WaveFinal[x])
	  FESTA_free(hes->psg.WaveFinal[x]);
	 hes->psg.WaveFinal[x] = 0;
	}
	if(hes->psg.WaveIL)
	 FESTA_free(hes->psg.WaveIL);
	hes->psg.WaveIL = 0;
}

/*--------------------------------------------------------------------------*/
/* PSG emulation                                                            */
/*--------------------------------------------------------------------------*/

/* Macro to access currently selected PSG channel */
#define PSGCH   hes->psg.channel[hes->psg.select]

void psg_w(FESTALON_HES *hes, uint16 address, uint8 data)
{
	int x;
    psg_update(hes);

    switch(address)
    {
        case 0x0800: /* Channel select */
            hes->psg.select = (data & 7);
            break;

        case 0x0801: /* Global sound balance */
	    //printf("Global: %02x\n\n",data);
            hes->psg.globalbalance = data;
	    for(x=0;x<6;x++)
	     redo_ddacache(hes, &hes->psg.channel[x]);
            break;

        case 0x0802: /* Channel frequency (LSB) */
            PSGCH.frequency = (PSGCH.frequency & 0x0F00) | (data);
            break;

        case 0x0803: /* Channel frequency (MSB) */
            PSGCH.frequency = (PSGCH.frequency & 0x00FF) | ((data & 0x0F) << 8);
            break;

        case 0x0804: /* Channel enable, DDA, volume */
	    //if(hes->psg.select == 5) printf("Ch: %02x\n",data&0x1F);
            PSGCH.control = data;
            if((data & 0xC0) == 0x40) PSGCH.waveform_index = 0;
            redo_ddacache(hes, &PSGCH);
            break;

        case 0x0805: /* Channel balance */
            PSGCH.balance = data;
            redo_ddacache(hes, &PSGCH);
            break;

        case 0x0806: /* Channel waveform data */
            data &= 0x1F;

            if((PSGCH.control & 0xC0) == 0x00)
            {
                PSGCH.waveform[PSGCH.waveform_index] = data;
                PSGCH.waveform_index = ((PSGCH.waveform_index + 1) & 0x1F);
            }

            /* DDA mode - data goes to speaker */
            if((PSGCH.control & 0xC0) == 0xC0)
            {
                PSGCH.dda = data;
            }
	    redo_ddacache(hes, &PSGCH);
            break;

        case 0x0807: /* Noise enable and frequency */
            if(hes->psg.select >= 4) PSGCH.noisectrl = data;
            break;

        case 0x0808: /* LFO frequency */
            hes->psg.lfofreq = data;
            break;

        case 0x0809: /* LFO trigger and control */
            hes->psg.lfoctrl = data;
            break;
    }
}

void psg_update(FESTALON_HES *hes)
{
 int chc;
 int32 V;
 int32 timestamp;
 int disabled[6];

 timestamp = (((h6280_Regs *)hes->h6280)->timestamp >> 1) &~1;

 for(V=0;V<6;V++)
 {
  disabled[V] = ((hes->psg.channel[V].control & 0x80)^0x80) >> 7;

  disabled[V] |= (hes->psg.disabled >> V)&1;
 }

 if(((FESTAFILT *)hes->psg.ff[0])->input_format == FFI_INT16)
 {
  int16 *WaveHiLeft=hes->psg.WaveHi16[0];
  int16 *WaveHiRight = hes->psg.WaveHi16[1];
  #include "psg-loop.h"
 }
 else
 {
  int32 *WaveHiLeft=hes->psg.WaveHi[0];
  int32 *WaveHiRight = hes->psg.WaveHi[1];
  #include "psg-loop.h"
 }


 hes->psg.lastts = timestamp;
}


uint32 psg_flush(FESTALON_HES *hes)
{
 int32 timestamp;
 int32 end, left;
 int32 x;
 int cb;

 h6280_Regs *h6280 = hes->h6280;

 psg_update(hes);

 timestamp = h6280->timestamp >> 2;

 for(cb=0;cb<2;cb++)
 {
  //printf("%d: %d\n",cb, hes->psg.ff[cb]);
  if(((FESTAFILT *)hes->psg.ff[cb])->input_format == FFI_INT16)
  {

  }
  else 
  {
   float *tmpo=(float *)&hes->psg.WaveHi[cb][hes->psg.lastpoo];
   int32 *intmpo = &hes->psg.WaveHi[cb][hes->psg.lastpoo];

   for(x=timestamp- hes->psg.lastpoo;x;x--)
   {
    *tmpo=*intmpo;
    tmpo++;
    intmpo++;
   }
  }
 
  if(((FESTAFILT *)hes->psg.ff[cb])->input_format == FFI_INT16)
  {
   end = FESTAFILT_Do(hes->psg.ff[cb],(float *)hes->psg.WaveHi16[cb],hes->psg.WaveFinal[cb], hes->psg.WaveFinalLen, timestamp, &left, 1);
   memmove(hes->psg.WaveHi16[cb],(int16 *)hes->psg.WaveHi16[cb]+timestamp-left,left*sizeof(uint16));
   memset((int16 *)hes->psg.WaveHi16[cb]+left,0,sizeof(hes->psg.WaveHi16[cb])-left*sizeof(uint16));
  }
  else
  {
   end = FESTAFILT_Do(hes->psg.ff[cb], (float *)hes->psg.WaveHi[cb],hes->psg.WaveFinal[cb], hes->psg.WaveFinalLen, timestamp, &left, 1);
   memmove(hes->psg.WaveHi[cb],hes->psg.WaveHi[cb]+timestamp-left,left*sizeof(uint32));
   memset(hes->psg.WaveHi[cb]+left,0,sizeof(hes->psg.WaveHi[cb])-left*sizeof(uint32));
  }
 }

 h6280->timestamp = left;
 hes->psg.lastpoo = h6280->timestamp;
 hes->psg.lastts = left << 1;

 h6280->timestamp <<= 2;

 for(x=0;x<end;x++)
 {
  hes->psg.WaveIL[x*2] = hes->psg.WaveFinal[0][x];
  hes->psg.WaveIL[x*2+1] = hes->psg.WaveFinal[1][x];
 }
 return(end);
}


void FESTAHES_SetVolume(FESTALON_HES *hes, uint32 volume)
{
 ((FESTAFILT *)hes->psg.ff[0])->SoundVolume=volume;
 ((FESTAFILT *)hes->psg.ff[1])->SoundVolume=volume;
}


int FESTAHES_SetLowpass(FESTALON_HES *hes, int on, uint32 corner, uint32 order)
{
 if(!FESTAFILT_SetLowpass(hes->psg.ff[0], on, corner, order)) return(0);
 if(!FESTAFILT_SetLowpass(hes->psg.ff[1], on, corner, order)) return(0);
 return(1);
}

int FESTAHES_SetSound(FESTALON_HES *hes, uint32 rate, int quality)
{
 int x;

 hes->psg.WaveFinalLen = rate / 60 * 2;      // *2 for extra room

 for(x=0;x<2;x++)
 {
  FESTAFILT **ff = (FESTAFILT **)hes->psg.ff;
  if(ff[x])
  {
   FESTAFILT_Kill(ff[x]);
   ff[x] = 0;
  }
  ff[x] = FESTAFILT_Init(rate,1789772.7272, 0, quality);

  if(hes->psg.WaveFinal[x]) 
  {
   FESTA_free(hes->psg.WaveFinal[x]);
   hes->psg.WaveFinal[x] = 0;
  }
  hes->psg.WaveFinal[x] = FESTA_malloc(16, sizeof(float) * hes->psg.WaveFinalLen);
 }

 if(hes->psg.WaveIL) FESTA_free(hes->psg.WaveIL);
 hes->psg.WaveIL = FESTA_malloc(16, sizeof(float) * hes->psg.WaveFinalLen * 2);

 return(1);
}

