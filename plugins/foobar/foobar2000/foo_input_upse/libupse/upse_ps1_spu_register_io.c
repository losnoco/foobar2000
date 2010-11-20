/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_ps1_spu_register_io.c
 * Purpose: libupse: PS1 SPU register I/O operations
 *
 * Copyright (c) 2007 William Pitcock <nenolod@sacredspiral.co.uk>
 * Portions copyright (c) 2002 Neill Corlett
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include "upse-types.h"
#include "upse-internal.h"
#include "upse-ps1-spu-base.h"
#include "upse-spu-internal.h"
#include "upse-ps1-spu-register-io.h"

#include "upse.h"
#include "upse-ps1-memory-manager.h"

////////////////////////////////////////////////////////////////////////
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

void SPUwriteRegister(upse_spu_state_t *spu, u32 reg, u16 val)
{
    const u32 r = reg & 0xfff;
    spu->regArea[(r - 0xc00) >> 1] = val;

    if (r >= 0x0c00 && r < 0x0d80)	// some channel info?
    {
	int ch = (r >> 4) - 0xc0;	// calc channel

	//if(ch==20) printf("%08x: %04x\n",reg,val);

	switch (r & 0x0f)
	{
	      //------------------------------------------------// r volume
	  case 0:
	      SetVolumeLR(spu, 0, (u8) ch, val);
	      break;
	      //------------------------------------------------// l volume
	  case 2:
	      SetVolumeLR(spu, 1, (u8) ch, val);
	      break;
	      //------------------------------------------------// pitch
	  case 4:
	      SetPitch(spu, ch, val);
	      break;
	      //------------------------------------------------// start
	  case 6:
	      spu->s_chan[ch].pStart = spu->spuMemC + ((u32) val << 3);
	      break;
	      //------------------------------------------------// level with pre-calcs
	  case 8:
	  {
	      const u32 lval = val;	// DEBUG CHECK
	      //---------------------------------------------//
	      spu->s_chan[ch].ADSRX.AttackModeExp = (lval & 0x8000) ? 1 : 0;
	      spu->s_chan[ch].ADSRX.AttackRate = (lval >> 8) & 0x007f;
	      spu->s_chan[ch].ADSRX.DecayRate = (lval >> 4) & 0x000f;
	      spu->s_chan[ch].ADSRX.SustainLevel = lval & 0x000f;
	      //---------------------------------------------//
	  }
	      break;
	      //------------------------------------------------// adsr times with pre-calcs
	  case 10:
	  {
	      const u32 lval = val;	// DEBUG CHECK

	      //----------------------------------------------//
	      spu->s_chan[ch].ADSRX.SustainModeExp = (lval & 0x8000) ? 1 : 0;
	      spu->s_chan[ch].ADSRX.SustainIncrease = (lval & 0x4000) ? 0 : 1;
	      spu->s_chan[ch].ADSRX.SustainRate = (lval >> 6) & 0x007f;
	      spu->s_chan[ch].ADSRX.ReleaseModeExp = (lval & 0x0020) ? 1 : 0;
	      spu->s_chan[ch].ADSRX.ReleaseRate = lval & 0x001f;
	      //----------------------------------------------//
	  }
	      break;
	      //------------------------------------------------// adsr volume... mmm have to investigate this
	      //case 0xC:
	      //  break;
	      //------------------------------------------------//
	  case 0xE:		// loop?
	      spu->s_chan[ch].pLoop = spu->spuMemC + ((u32) val << 3);
	      spu->s_chan[ch].bIgnoreLoop = 1;
	      break;
	      //------------------------------------------------//
	}
	return;
    }

    switch (r)
    {
	  //-------------------------------------------------//
      case H_SPUaddr:
	  spu->spuAddr = (u32) val << 3;
	  break;
	  //-------------------------------------------------//
      case H_SPUdata:
	  spu->spuMem[spu->spuAddr >> 1] = BFLIP16(val);
	  spu->spuAddr += 2;
	  if (spu->spuAddr > 0x7ffff)
	      spu->spuAddr = 0;
	  break;
	  //-------------------------------------------------//
      case H_SPUctrl:
	  spu->spuCtrl = val;
	  break;
	  //-------------------------------------------------//
      case H_SPUstat:
	  spu->spuStat = val & 0xf800;
	  break;
	  //-------------------------------------------------//
      case H_SPUReverbAddr:
	  if (val == 0xFFFF || val <= 0x200)
	  {
	      spu->rvb.StartAddr = spu->rvb.CurrAddr = 0;
	  }
	  else
	  {
	      const s32 iv = (u32) val << 2;
	      if (spu->rvb.StartAddr != iv)
	      {
		  spu->rvb.StartAddr = (u32) val << 2;
		  spu->rvb.CurrAddr = spu->rvb.StartAddr;
	      }
	  }
	  break;
	  //-------------------------------------------------//
      case H_SPUirqAddr:
	  spu->spuIrq = val;
	  spu->pSpuIrq = spu->spuMemC + ((u32) val << 3);
	  break;
	  //-------------------------------------------------//
	  /* Volume settings appear to be at least 15-bit unsigned in this case.  
	     Definitely NOT 15-bit signed.  Probably 16-bit signed, so s16 type cast.
	     Check out "Chrono Cross:  Shadow's End Forest"
	   */
      case H_SPUrvolL:
	  spu->rvb.VolLeft = (s16) val;
	  //printf("%d\n",val);
	  break;
	  //-------------------------------------------------//
      case H_SPUrvolR:
	  spu->rvb.VolRight = (s16) val;
	  //printf("%d\n",val);
	  break;
	  //-------------------------------------------------//

/*
    case H_ExtLeft:
     //auxprintf("EL %d\n",val);
      break;
    //-------------------------------------------------//
    case H_ExtRight:
     //auxprintf("ER %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUmvolL:
     //auxprintf("ML %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUmvolR:
     //auxprintf("MR %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUMute1:
     //printf("M0 %04x\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUMute2:
    // printf("M1 %04x\n",val);
      break;
*/
	  //-------------------------------------------------//
      case H_SPUon1:
	  SoundOn(spu, 0, 16, val);
	  break;
	  //-------------------------------------------------//
      case H_SPUon2:
	  // printf("Boop: %08x: %04x\n",reg,val);
	  SoundOn(spu, 16, 24, val);
	  break;
	  //-------------------------------------------------//
      case H_SPUoff1:
	  SoundOff(spu, 0, 16, val);
	  break;
	  //-------------------------------------------------//
      case H_SPUoff2:
	  SoundOff(spu, 16, 24, val);
	  // printf("Boop: %08x: %04x\n",reg,val);
	  break;
	  //-------------------------------------------------//
      case H_FMod1:
	  FModOn(spu, 0, 16, val);
	  break;
	  //-------------------------------------------------//
      case H_FMod2:
	  FModOn(spu, 16, 24, val);
	  break;
	  //-------------------------------------------------//
      case H_Noise1:
	  NoiseOn(spu, 0, 16, val);
	  break;
	  //-------------------------------------------------//
      case H_Noise2:
	  NoiseOn(spu, 16, 24, val);
	  break;
	  //-------------------------------------------------//
      case H_RVBon1:
	  spu->rvb.Enabled &= ~0xFFFF;
	  spu->rvb.Enabled |= val;
	  break;

	  //-------------------------------------------------//
      case H_RVBon2:
	  spu->rvb.Enabled &= 0xFFFF;
	  spu->rvb.Enabled |= val << 16;
	  break;

	  //-------------------------------------------------//
      case H_Reverb + 0:
	  spu->rvb.FB_SRC_A = val;
	  break;

      case H_Reverb + 2:
	  spu->rvb.FB_SRC_B = (s16) val;
	  break;
      case H_Reverb + 4:
	  spu->rvb.IIR_ALPHA = (s16) val;
	  break;
      case H_Reverb + 6:
	  spu->rvb.ACC_COEF_A = (s16) val;
	  break;
      case H_Reverb + 8:
	  spu->rvb.ACC_COEF_B = (s16) val;
	  break;
      case H_Reverb + 10:
	  spu->rvb.ACC_COEF_C = (s16) val;
	  break;
      case H_Reverb + 12:
	  spu->rvb.ACC_COEF_D = (s16) val;
	  break;
      case H_Reverb + 14:
	  spu->rvb.IIR_COEF = (s16) val;
	  break;
      case H_Reverb + 16:
	  spu->rvb.FB_ALPHA = (s16) val;
	  break;
      case H_Reverb + 18:
	  spu->rvb.FB_X = (s16) val;
	  break;
      case H_Reverb + 20:
	  spu->rvb.IIR_DEST_A0 = (s16) val;
	  break;
      case H_Reverb + 22:
	  spu->rvb.IIR_DEST_A1 = (s16) val;
	  break;
      case H_Reverb + 24:
	  spu->rvb.ACC_SRC_A0 = (s16) val;
	  break;
      case H_Reverb + 26:
	  spu->rvb.ACC_SRC_A1 = (s16) val;
	  break;
      case H_Reverb + 28:
	  spu->rvb.ACC_SRC_B0 = (s16) val;
	  break;
      case H_Reverb + 30:
	  spu->rvb.ACC_SRC_B1 = (s16) val;
	  break;
      case H_Reverb + 32:
	  spu->rvb.IIR_SRC_A0 = (s16) val;
	  break;
      case H_Reverb + 34:
	  spu->rvb.IIR_SRC_A1 = (s16) val;
	  break;
      case H_Reverb + 36:
	  spu->rvb.IIR_DEST_B0 = (s16) val;
	  break;
      case H_Reverb + 38:
	  spu->rvb.IIR_DEST_B1 = (s16) val;
	  break;
      case H_Reverb + 40:
	  spu->rvb.ACC_SRC_C0 = (s16) val;
	  break;
      case H_Reverb + 42:
	  spu->rvb.ACC_SRC_C1 = (s16) val;
	  break;
      case H_Reverb + 44:
	  spu->rvb.ACC_SRC_D0 = (s16) val;
	  break;
      case H_Reverb + 46:
	  spu->rvb.ACC_SRC_D1 = (s16) val;
	  break;
      case H_Reverb + 48:
	  spu->rvb.IIR_SRC_B1 = (s16) val;
	  break;
      case H_Reverb + 50:
	  spu->rvb.IIR_SRC_B0 = (s16) val;
	  break;
      case H_Reverb + 52:
	  spu->rvb.MIX_DEST_A0 = (s16) val;
	  break;
      case H_Reverb + 54:
	  spu->rvb.MIX_DEST_A1 = (s16) val;
	  break;
      case H_Reverb + 56:
	  spu->rvb.MIX_DEST_B0 = (s16) val;
	  break;
      case H_Reverb + 58:
	  spu->rvb.MIX_DEST_B1 = (s16) val;
	  break;
      case H_Reverb + 60:
	  spu->rvb.IN_COEF_L = (s16) val;
	  break;
      case H_Reverb + 62:
	  spu->rvb.IN_COEF_R = (s16) val;
	  break;
    }

}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

u16 SPUreadRegister(upse_spu_state_t *spu, u32 reg)
{
    const u32 r = reg & 0xfff;

    if (r >= 0x0c00 && r < 0x0d80)
    {
	switch (r & 0x0f)
	{
	  case 0xC:		// get adsr vol
	  {
	      const int ch = (r >> 4) - 0xc0;
	      if (spu->s_chan[ch].bNew)
		  return 1;	// we are started, but not processed? return 1
	      if (spu->s_chan[ch].ADSRX.lVolume &&	// same here... we haven't decoded one sample yet, so no envelope yet. return 1 as well
		  !spu->s_chan[ch].ADSRX.EnvelopeVol)
		  return 1;
	      return (u16) (spu->s_chan[ch].ADSRX.EnvelopeVol >> 16);
	  }

	  case 0xE:		// get loop address
	  {
	      const int ch = (r >> 4) - 0xc0;
	      if (spu->s_chan[ch].pLoop == NULL)
		  return 0;
	      return (u16) ((spu->s_chan[ch].pLoop - spu->spuMemC) >> 3);
	  }
	}
    }

    switch (r)
    {
      case H_SPUctrl:
	  return spu->spuCtrl;

      case H_SPUstat:
	  return spu->spuStat;

      case H_SPUaddr:
	  return (u16) (spu->spuAddr >> 3);

      case H_SPUdata:
      {
	  u16 s = BFLIP16(spu->spuMem[spu->spuAddr >> 1]);
	  spu->spuAddr += 2;
	  if (spu->spuAddr > 0x7ffff)
	      spu->spuAddr = 0;
	  return s;
      }

      case H_SPUirqAddr:
	  return spu->spuIrq;

	  //case H_SPUIsOn1:
	  // return IsSoundOn(0,16);

	  //case H_SPUIsOn2:
	  // return IsSoundOn(16,24);

    }

    return spu->regArea[(r - 0xc00) >> 1];
}

////////////////////////////////////////////////////////////////////////
// SOUND ON register write
////////////////////////////////////////////////////////////////////////

void SoundOn(upse_spu_state_t *spu, int start, int end, u16 val)	// SOUND ON PSX COMAND
{
    int ch;

    for (ch = start; ch < end; ch++, val >>= 1)	// loop channels
    {
	if ((val & 1) && spu->s_chan[ch].pStart)	// mmm... start has to be set before key on !?!
	{
	    spu->s_chan[ch].bIgnoreLoop = 0;
	    spu->s_chan[ch].bNew = 1;
	}
    }
}

////////////////////////////////////////////////////////////////////////
// SOUND OFF register write
////////////////////////////////////////////////////////////////////////

void SoundOff(upse_spu_state_t *spu, int start, int end, u16 val)	// SOUND OFF PSX COMMAND
{
    int ch;
    for (ch = start; ch < end; ch++, val >>= 1)	// loop channels
    {
	if (val & 1)		// && spu->s_chan[i].bOn)  mmm...
	{
	    spu->s_chan[ch].bStop = 1;
	}
    }
}

////////////////////////////////////////////////////////////////////////
// FMOD register write
////////////////////////////////////////////////////////////////////////

void FModOn(upse_spu_state_t *spu, int start, int end, u16 val)	// FMOD ON PSX COMMAND
{
    int ch;

    for (ch = start; ch < end; ch++, val >>= 1)	// loop channels
    {
	if (val & 1)		// -> fmod on/off
	{
	    if (ch > 0)
	    {
		spu->s_chan[ch].bFMod = 1;	// --> sound channel
		spu->s_chan[ch - 1].bFMod = 2;	// --> freq channel
	    }
	}
	else
	{
	    spu->s_chan[ch].bFMod = 0;	// --> turn off fmod
	}
    }
}

////////////////////////////////////////////////////////////////////////
// NOISE register write
////////////////////////////////////////////////////////////////////////

void NoiseOn(upse_spu_state_t *spu, int start, int end, u16 val)	// NOISE ON PSX COMMAND
{
    int ch;

    for (ch = start; ch < end; ch++, val >>= 1)	// loop channels
    {
	if (val & 1)		// -> noise on/off
	{
	    spu->s_chan[ch].bNoise = 1;
	}
	else
	{
	    spu->s_chan[ch].bNoise = 0;
	}
    }
}

////////////////////////////////////////////////////////////////////////
// LEFT VOLUME register write
////////////////////////////////////////////////////////////////////////

// please note: sweep is wrong.

void SetVolumeLR(upse_spu_state_t *spu, int right, u8 ch, s16 vol)	// LEFT VOLUME
{
    //if(vol&0xc000)
    //printf("%d %08x\n",right,vol);
    if (right)
	spu->s_chan[ch].iRightVolRaw = vol;
    else
	spu->s_chan[ch].iLeftVolRaw = vol;

    if (vol & 0x8000)		// sweep?
    {
	s16 sInc = 1;		// -> sweep up?
	if (vol & 0x2000)
	    sInc = -1;		// -> or down?
	if (vol & 0x1000)
	    vol ^= 0xffff;	// -> mmm... phase inverted? have to investigate this
	vol = ((vol & 0x7f) + 1) / 2;	// -> sweep: 0..127 -> 0..64
	vol += vol / (2 * sInc);	// -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
	vol *= 128;
	vol &= 0x3fff;
	//puts("Sweep");
    }
    else			// no sweep:
    {
	if (vol & 0x4000)
	    vol = (vol & 0x3FFF) - 0x4000;
	else
	    vol &= 0x3FFF;

	//if(vol&0x4000)                                      // -> mmm... phase inverted? have to investigate this
	// vol=0-(0x3fff-(vol&0x3fff));
	//else
	// vol&=0x3fff;
    }
    if (right)
	spu->s_chan[ch].iRightVolume = vol;
    else
	spu->s_chan[ch].iLeftVolume = vol;	// store volume
}

////////////////////////////////////////////////////////////////////////
// PITCH register write
////////////////////////////////////////////////////////////////////////

void SetPitch(upse_spu_state_t *spu, int ch, u16 val)	// SET PITCH
{
    int NP;
    if (val > 0x3fff)
	NP = 0x3fff;		// get pitch val
    else
	NP = val;

    spu->s_chan[ch].iRawPitch = NP;

    NP = (44100L * NP) / 4096L;	// calc frequency
    if (NP < 1)
	NP = 1;			// some security
    spu->s_chan[ch].iActFreq = NP;	// store frequency
}
