/***************************************************************************
                         upse-spu-internal.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef __UPSE_SPU_INTERNAL_H__
#define __UPSE_SPU_INTERNAL_H__

#include "upse-types.h"
#include "upse-ps1-memory-manager.h"

//*************************************************************************//
// History of changes:
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// num of channels
#define MAXCHAN     24

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////

// ADSR INFOS PER CHANNEL
typedef struct
{
    int AttackModeExp;
    s32 AttackTime;
    s32 DecayTime;
    s32 SustainLevel;
    int SustainModeExp;
    s32 SustainModeDec;
    s32 SustainTime;
    int ReleaseModeExp;
    u32 ReleaseVal;
    s32 ReleaseTime;
    s32 ReleaseStartTime;
    s32 ReleaseVol;
    s32 lTime;
    s32 lVolume;
} ADSRInfo;

typedef struct
{
    int State;
    int AttackModeExp;
    int AttackRate;
    int DecayRate;
    int SustainLevel;
    int SustainModeExp;
    int SustainIncrease;
    int SustainRate;
    int ReleaseModeExp;
    int ReleaseRate;
    int EnvelopeVol;
    s32 lVolume;
    s32 EnvelopeDelta;
    s32 EnvelopeMax;
} ADSRInfoEx;

///////////////////////////////////////////////////////////

// Tmp Flags

// used for debug channel muting
#define FLAG_MUTE  1

///////////////////////////////////////////////////////////

// MAIN CHANNEL STRUCT
typedef struct
{
    int bNew;			// start flag

    int iSBPos;			// mixing stuff
    int spos;
    int sinc;
    int SB[32 + 1];
    int sval;

    u8 *pStart;			// start ptr into sound mem
    u8 *pCurr;			// current pos in sound mem
    u8 *pLoop;			// loop ptr in sound mem

    int bOn;			// is channel active (sample playing?)
    int bStop;			// is channel stopped (sample _can_ still be playing, ADSR Release phase)
    int iActFreq;		// current psx pitch
    int iUsedFreq;		// current pc pitch
    int iLeftVolume;		// left volume
    int iLeftVolRaw;		// left psx volume value
    int bIgnoreLoop;		// ignore loop bit, if an external loop address is used
    int iRightVolume;		// right volume
    int iRightVolRaw;		// right psx volume value
    int iRawPitch;		// raw pitch (0...3fff)
    int iIrqDone;		// debug irq done flag
    int s_1;			// last decoding infos
    int s_2;
    int bRVBActive;		// reverb active flag
    int iRVBOffset;		// reverb offset
    int iRVBRepeat;		// reverb repeat
    int bNoise;			// noise active flag
    int bFMod;			// freq mod (0=off, 1=sound channel, 2=freq channel)
    int iOldNoise;		// old noise val for this channel   
    int iNoisePos;		// position in noise PRNG.
    ADSRInfo ADSR;		// active ADSR settings
    ADSRInfoEx ADSRX;		// next ADSR settings (will be moved to active on sample start)
} SPUCHAN;

///////////////////////////////////////////////////////////

typedef struct
{
    int StartAddr;		// reverb area start addr in samples
    int CurrAddr;		// reverb area curr addr in samples

    int Enabled;
    int VolLeft;
    int VolRight;
    int iLastRVBLeft;
    int iLastRVBRight;
    int iRVBLeft;
    int iRVBRight;


    int FB_SRC_A;		// (offset)
    int FB_SRC_B;		// (offset)
    int IIR_ALPHA;		// (coef.)
    int ACC_COEF_A;		// (coef.)
    int ACC_COEF_B;		// (coef.)
    int ACC_COEF_C;		// (coef.)
    int ACC_COEF_D;		// (coef.)
    int IIR_COEF;		// (coef.)
    int FB_ALPHA;		// (coef.)
    int FB_X;			// (coef.)
    int IIR_DEST_A0;		// (offset)
    int IIR_DEST_A1;		// (offset)
    int ACC_SRC_A0;		// (offset)
    int ACC_SRC_A1;		// (offset)
    int ACC_SRC_B0;		// (offset)
    int ACC_SRC_B1;		// (offset)
    int IIR_SRC_A0;		// (offset)
    int IIR_SRC_A1;		// (offset)
    int IIR_DEST_B0;		// (offset)
    int IIR_DEST_B1;		// (offset)
    int ACC_SRC_C0;		// (offset)
    int ACC_SRC_C1;		// (offset)
    int ACC_SRC_D0;		// (offset)
    int ACC_SRC_D1;		// (offset)
    int IIR_SRC_B1;		// (offset)
    int IIR_SRC_B0;		// (offset)
    int MIX_DEST_A0;		// (offset)
    int MIX_DEST_A1;		// (offset)
    int MIX_DEST_B0;		// (offset)
    int MIX_DEST_B1;		// (offset)
    int IN_COEF_L;		// (coef.)
    int IN_COEF_R;		// (coef.)
} REVERBInfo;

typedef struct {
    float lx1;
    float lx2;
    float ly1;
    float ly2;

    float la0;
    float la1;
    float la2;
    float lb1;
    float lb2;

    float hx1[2];
    float hx2[2];
    float hy1[2];
    float hy2[2];

    float ha0;
    float ha1;
    float ha2;
    float hb1;
    float hb2;
} upse_spu_lowpass_info_t;

typedef struct {
    s16 hist[2];
} upse_spu_nyquist_info_t;

typedef struct {
    u16 regArea[0x200];
    u16 spuMem[256 * 1024];
    u8 *spuMemC;
    u8 *pSpuIrq;
    u8 pSpuBuffer[32770];

    u16 spuCtrl;		// some vars to store psx reg infos
    u16 spuStat;
    u16 spuIrq;
    u32 spuAddr;		// address into spu mem
    int bSPUIsOpen;

    SPUCHAN s_chan[MAXCHAN + 1];	// channel + 1 infos (1 is security for fmod handling)
    REVERBInfo rvb;

    upse_audio_callback_func_t cb;
    const void *cb_userdata;

    u32 sampcount;
    u32 decaybegin;
    u32 decayend;

    s16 *pS;

    u32 seektime;
    s32 nextirq;

    upse_spu_lowpass_info_t lowpass;
    upse_spu_nyquist_info_t nyquist;

    upse_module_instance_t *ins;

    s32 downbuf[2][8];
    s32 upbuf[2][8];
    int dbpos, ubpos;

    s32 RateTable[160];
} upse_spu_state_t;

extern void upse_spu_lowpass_filter_reset(upse_spu_state_t *spu);
extern void upse_spu_lowpass_filter_redesign(upse_spu_state_t *spu, int samplerate);
extern void upse_spu_lowpass_filter_process(upse_spu_state_t *spu, s16 *samplebuf, int samplecount);

extern void upse_spu_nyquist_filter_process(upse_spu_state_t *spu, s16 *samplebuf, int samplecount);

#endif
