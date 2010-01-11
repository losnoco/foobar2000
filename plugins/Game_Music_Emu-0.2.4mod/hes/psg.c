#include "shared.h"
#include <math.h>
#include <string.h>

#include "synth_wrapper.h"

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
      dbtable[y][x] = (flub * (y - 0x10) * 12);
     //printf("%f\n",flub);
    }
    memset(&hes->psg, 0, sizeof(hes->psg));

    return (0);
}

void psg_reset(FESTALON_HES *hes)
{
    //memset(&hes->psg.channel, 0, sizeof(hes->psg.channel));
	unsigned i;
    for ( i = 0; i < 6; ++i )
    {
     memset( &hes->psg.channel[i], 0, &((psg_channel*)0)->blip_buffer );
    }
	memset( &hes->psg.channel[6], 0, sizeof( psg_channel ) * 2 );
}

void psg_shutdown(FESTALON_HES *hes)
{
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

static _inline void psg_update_ch(FESTALON_HES *hes, psg_channel *ch, int32 timestamp)
{
 int delta = ch->dda_cache[0] - ch->last_amp[0];
 if (delta)
 {
  ch->last_amp[0] = ch->dda_cache[0];
  synth_offset( hes->psg.blip_synth, timestamp, delta, ch->blip_buffer[0] );
 }
 delta = ch->dda_cache[1] - ch->last_amp[1];
 if (delta)
 {
  ch->last_amp[1] = ch->dda_cache[1];
  synth_offset( hes->psg.blip_synth, timestamp, delta, ch->blip_buffer[1] );
 }
}

void psg_update(FESTALON_HES *hes)
{
 int chc;
 int32 V, step;
 int32 timestamp;
 int disabled[6];

 timestamp = ((h6280_Regs *)hes->h6280)->timestamp & ~3; //(((h6280_Regs *)hes->h6280)->timestamp >> 1) &~1;

 for(V=0;V<6;V++)
 {
  disabled[V] = ((hes->psg.channel[V].control & 0x80)^0x80) >> 7;
  
  disabled[V] |= ! ( hes->psg.channel[V].blip_buffer[0] && hes->psg.channel[V].blip_buffer[1] );
 }

 for(chc = 5; chc >= 0; chc--)
 {
  psg_channel *ch = &hes->psg.channel[chc];
   
  if(disabled[chc]) continue;

  if ( timestamp >= hes->psg.lastts ) psg_update_ch( hes, ch, hes->psg.lastts );

  if ( ! ( ch->control & 0x40 ) )
  {
   if (ch->noisectrl & 0x80)
   {
    for(V = hes->psg.lastts + ( ( ch->noisecount - 1 ) & 0x1F ) * 2, step = ( ch->noisectrl & 0x1F ) ? ( ch->noisectrl & 0x1F ) * 2 : 0x20 * 2; V < timestamp; V += step)
    {
     ch->lfsr = (ch->lfsr << 1) | ((~((ch->lfsr >> 15) ^ (ch->lfsr >> 14) ^ (ch->lfsr >> 13) ^ (ch->lfsr >> 3)))&1);
     ch->dda = (ch->lfsr&1)?0x1F:0;
     redo_ddacache(hes, ch);
     psg_update_ch( hes, ch, V );
    }
    ch->noisecount = ( V - timestamp + 2 ) >> 1;
   }
   else
   {
    for(V = hes->psg.lastts + ( ( ch->counter - 1 ) & 0xFFF ) * 2, step = ch->frequency ? ch->frequency * 2 : 0x1000 * 2; V < timestamp; V += step)
    {
     ch->waveform_index = (ch->waveform_index + 1) & 0x1F;
     ch->dda = ch->waveform[ch->waveform_index];
     redo_ddacache(hes, ch);
     psg_update_ch( hes, ch, V );
    }
    ch->counter = ( V - timestamp + 2 ) >> 1;
   }
  }
 }
 hes->psg.lastts = ( ((h6280_Regs *)hes->h6280)->timestamp + 3 ) & ~3;
}


uint32 psg_flush(FESTALON_HES *hes)
{
 int32 timestamp;

 h6280_Regs *h6280 = hes->h6280;

 psg_update(hes);

 timestamp = h6280->timestamp; // >> 2;

 h6280->timestamp = 0;
 hes->psg.lastts -= timestamp;

 return(timestamp);
}

