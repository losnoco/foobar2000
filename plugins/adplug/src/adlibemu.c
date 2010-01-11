/*
 * ADLIBEMU.C
 * Copyright (C) 1998-2001 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
This file is a digital Adlib emulator for OPL2 and possibly OPL3

Features that could be added in a future version:
- Amplitude and Frequency Vibrato Bits (not hard, but a big speed hit)
- Global Keyboard Split Number Bit (need to research this one some more)
- 2nd Adlib chip for OPL3 (simply need to make my cell array bigger)
- Advanced connection modes of OPL3 (Just need to add more "docell" cases)
- L/R Stereo bits of OPL3 (Need adlibgetsample to return stereo)

Features that aren't worth supporting:
- Anything related to adlib timers&interrupts (Sorry - I always used IRQ0)
- Composite sine wave mode (CSM) (Supported only on ancient cards)

I'm not sure about a few things in my code:
- Attack curve.  What function is this anyway?  I chose to use an order-3
  polynomial to approximate but this doesn't seem right.
- Attack/Decay/Release constants - my constants may not be exact
- What should ADJUSTSPEED be?
- Haven't verified that Global Keyboard Split Number Bit works yet
- Some of the drums don't always sound right.  It's pretty hard to guess
  the exact waveform of drums when you look at random data which is
  slightly randomized due to digital ADC recording.
- Adlib seems to have a lot more treble than my emulator does.  I'm not
  sure if this is simply unfixable due to the sound blaster's different
  filtering on FM and digital playback or if it's a serious bug in my
  code.
*/

#include <math.h>
#include <string.h>
#include <stdlib.h>

#if !defined(max) && !defined(__cplusplus)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min) && !defined(__cplusplus)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define PI 3.141592653589793
#define MAXCELLS 18
#define WAVPREC 2048

#define FRQSCALE (49716/512.0)

//Constants for Ken's Awe32, on a PII-266 (Ken says: Use these for KSM's!)
#define MODFACTOR 4.0      //How much of modulator cell goes into carrier
#define MFBFACTOR 1.0      //How much feedback goes back into modulator
#define ADJUSTSPEED 0.75   //0<=x<=1  Simulate finite rate of change of state

//Constants for Ken's Awe64G, on a P-133
//#define MODFACTOR 4.25   //How much of modulator cell goes into carrier
//#define MFBFACTOR 0.5    //How much feedback goes back into modulator
//#define ADJUSTSPEED 0.85 //0<=x<=1  Simulate finite rate of change of state

typedef struct
{
    float val, t, tinc, vol, sustain, amp, mfb;
    float a0, a1, a2, a3, decaymul, releasemul;
    short *waveform;
    long wavemask;
    void (*cellfunc)(void *, float);
    unsigned char flags, dum0, dum1, dum2;
} celltype;


static const float kslmul[4] = {0.0,0.5,0.25,1.0};
static const float frqmul[16] = {.5,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15};
static const unsigned char modulatorbase[9] = {0,1,2,8,9,10,16,17,18};
static const unsigned char base2cell[22] = {0,1,2,0,1,2,0,0,3,4,5,3,4,5,0,0,6,7,8,6,7,8};

#define FIFOSIZ 256

typedef struct
{
	long numspeakers, bytespersample;
	float recipsamp;
	celltype cell[MAXCELLS];
	signed short wavtable[WAVPREC*3];
	float nfrqmul[16];
	unsigned char adlibreg[256], ksl[8][16];
	unsigned char odrumstat; // = 0;

	float AMPSCALE; //=(8192.0);

	float lvol[9]; // = {1,1,1,1,1,1,1,1,1};  //Volume multiplier on left speaker
	float rvol[9]; // = {1,1,1,1,1,1,1,1,1};  //Volume multiplier on right speaker
	long lplc[9]; // = {0,0,0,0,0,0,0,0,0};   //Samples to delay on left speaker
	long rplc[9]; // = {0,0,0,0,0,0,0,0,0};   //Samples to delay on right speaker

	long nlvol[9], nrvol[9];
	long nlplc[9], nrplc[9];
	long rend; // = 0;

	float *rptr[9], *nrptr[9];
	float rbuf[9][FIFOSIZ*2];
	float snd[FIFOSIZ*2];
} adlib_state;

#ifndef USING_ASM
#define _inline
#endif

#ifdef USING_ASM
static _inline void ftol (float f, long *a)
{
    _asm
	{
	    mov eax, a
		fld f
		fistp dword ptr [eax]
		}
}
#else
static void ftol(float f, long *a) {
    *a=f;
}
#endif

#define ctc ((celltype *)c)      //A rare attempt to make code easier to read!
void docell4 (void *c, float modulator) { }
void docell3 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);
    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell2 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    if (*(long *)&ctc->amp <= 0x37800000)
    {
	ctc->amp = 0;
	ctc->cellfunc = docell4;
    }
    ctc->amp *= ctc->releasemul;

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell1 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    if ((*(long *)&ctc->amp) <= (*(long *)&ctc->sustain))
    {
	if (ctc->flags&32)
	{
	    ctc->amp = ctc->sustain;
	    ctc->cellfunc = docell3;
	}
	else
	    ctc->cellfunc = docell2;
    }
    else
	ctc->amp *= ctc->decaymul;

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell0 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    ctc->amp = ((ctc->a3*ctc->amp + ctc->a2)*ctc->amp + ctc->a1)*ctc->amp + ctc->a0;
    if ((*(long *)&ctc->amp) > 0x3f800000)
    {
	ctc->amp = 1;
	ctc->cellfunc = docell1;
    }

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}


static const long waveform[8] = {WAVPREC,WAVPREC>>1,WAVPREC,(WAVPREC*3)>>2,0,0,(WAVPREC*5)>>2,WAVPREC<<1};
static const long wavemask[8] = {WAVPREC-1,WAVPREC-1,(WAVPREC>>1)-1,(WAVPREC>>1)-1,WAVPREC-1,((WAVPREC*3)>>2)-1,WAVPREC>>1,WAVPREC-1};
static const long wavestart[8] = {0,WAVPREC>>1,0,WAVPREC>>2,0,0,0,WAVPREC>>3};
static const float attackconst[4] = {1/2.82624,1/2.25280,1/1.88416,1/1.59744};
static const float decrelconst[4] = {1/39.28064,1/31.41608,1/26.17344,1/22.44608};
void cellon (adlib_state * state, long i, long j, celltype *c, unsigned char iscarrier)
{
    long frn, oct, toff;
    float f;

    frn = ((((long)state->adlibreg[i+0xb0])&3)<<8) + (long)state->adlibreg[i+0xa0];
    oct = ((((long)state->adlibreg[i+0xb0])>>2)&7);
    toff = (oct<<1) + ((frn>>9)&((frn>>8)|(((state->adlibreg[8]>>6)&1)^1)));
    if (!(state->adlibreg[j+0x20]&16)) toff >>= 2;

    f = pow(2.0,(state->adlibreg[j+0x60]>>4)+(toff>>2)-1)*attackconst[toff&3]*state->recipsamp;
    c->a0 = .0377*f; c->a1 = 10.73*f+1; c->a2 = -17.57*f; c->a3 = 7.42*f;
    f = -7.4493*decrelconst[toff&3]*state->recipsamp;
    c->decaymul = pow(2.0,f*pow(2.0,(state->adlibreg[j+0x60]&15)+(toff>>2)));
    c->releasemul = pow(2.0,f*pow(2.0,(state->adlibreg[j+0x80]&15)+(toff>>2)));
    c->wavemask = wavemask[state->adlibreg[j+0xe0]&7];
    c->waveform = &state->wavtable[waveform[state->adlibreg[j+0xe0]&7]];
    if (!(state->adlibreg[1]&0x20)) c->waveform = &state->wavtable[WAVPREC];
    c->t = wavestart[state->adlibreg[j+0xe0]&7];
    c->flags = state->adlibreg[j+0x20];
    c->cellfunc = docell0;
    c->tinc = (float)(frn<<oct)*state->nfrqmul[state->adlibreg[j+0x20]&15];
    c->vol = pow(2.0,((float)(state->adlibreg[j+0x40]&63) +
		      (float)kslmul[state->adlibreg[j+0x40]>>6]*state->ksl[oct][frn>>6]) * -.125 - 14);
    c->sustain = pow(2.0,(float)(state->adlibreg[j+0x80]>>4) * -.5);
    if (!iscarrier) c->amp = 0;
    c->mfb = pow(2.0,((state->adlibreg[i+0xc0]>>1)&7)+5)*(WAVPREC/2048.0)*MFBFACTOR;
    if (!(state->adlibreg[i+0xc0]&14)) c->mfb = 0;
    c->val = 0;
}

//This function (and bug fix) written by Chris Moeller
void cellfreq (adlib_state * state, signed long i, signed long j, celltype *c)
{
    long frn, oct;

    frn = ((((long)state->adlibreg[i+0xb0])&3)<<8) + (long)state->adlibreg[i+0xa0];
    oct = ((((long)state->adlibreg[i+0xb0])>>2)&7);

    c->tinc = (float)(frn<<oct)*state->nfrqmul[state->adlibreg[j+0x20]&15];
    c->vol = pow(2.0,((float)(state->adlibreg[j+0x40]&63) +
		      (float)kslmul[state->adlibreg[j+0x40]>>6]*state->ksl[oct][frn>>6]) * -.125 - 14);
}

void * adlibinit (long dasamplerate, long danumspeakers, long dabytespersample)
{
    long i, j, frn, oct;

	adlib_state * state = malloc( sizeof( adlib_state ) );
	if ( !state ) return 0;

    memset((void *)(state->adlibreg),0,sizeof(state->adlibreg));
    memset((void *)(state->cell),0,sizeof(celltype)*MAXCELLS);
    memset((void *)(state->rbuf),0,sizeof(state->rbuf));
    state->rend = 0; state->odrumstat = 0;

	state->AMPSCALE = 8192.0;

	for(i=0;i<10;i++)
	{
		state->lvol [i] = 1;
		state->rvol [i] = 1;
		state->lplc [i] = 0;
		state->rplc [i] = 0;
	}

    for(i=0;i<MAXCELLS;i++)
    {
	state->cell[i].cellfunc = docell4;
	state->cell[i].amp = 0;
	state->cell[i].vol = 0;
	state->cell[i].t = 0;
	state->cell[i].tinc = 0;
	state->cell[i].wavemask = 0;
	state->cell[i].waveform = &state->wavtable[WAVPREC];
    }

    state->numspeakers = danumspeakers;
    state->bytespersample = dabytespersample;

    state->recipsamp = 1.0 / (float)dasamplerate;
    for(i=15;i>=0;i--) state->nfrqmul[i] = frqmul[i]*state->recipsamp*FRQSCALE*(WAVPREC/2048.0);

	for(i=0;i<(WAVPREC>>1);i++)
	{
	    state->wavtable[i] =
		state->wavtable[(i<<1)  +WAVPREC] = (signed short)(16384*sin((float)((i<<1)  )*PI*2/WAVPREC));
	    state->wavtable[(i<<1)+1+WAVPREC] = (signed short)(16384*sin((float)((i<<1)+1)*PI*2/WAVPREC));
	}
	for(i=0;i<(WAVPREC>>3);i++)
	{
	    state->wavtable[i+(WAVPREC<<1)] = state->wavtable[i+(WAVPREC>>3)]-16384;
	    state->wavtable[i+((WAVPREC*17)>>3)] = state->wavtable[i+(WAVPREC>>2)]+16384;
	}

	//[table in book]*8/3
	state->ksl[7][0] = 0; state->ksl[7][1] = 24; state->ksl[7][2] = 32; state->ksl[7][3] = 37;
	state->ksl[7][4] = 40; state->ksl[7][5] = 43; state->ksl[7][6] = 45; state->ksl[7][7] = 47;
	state->ksl[7][8] = 48; for(i=9;i<16;i++) state->ksl[7][i] = i+41;
	for(j=6;j>=0;j--)
	    for(i=0;i<16;i++)
	    {
		oct = (long)state->ksl[j+1][i]-8; if (oct < 0) oct = 0;
		state->ksl[j][i] = (unsigned char)oct;
	    }
	return state;
}

void adlibfree (void * state_v)
{
	if ( state_v ) free( state_v );
}

void adlib0 (void * state_v, long i, long v)
{
	adlib_state * state = (adlib_state *) state_v;
    unsigned char tmp = state->adlibreg[i];
    state->adlibreg[i] = v;

    if (i == 0xbd)
    {
	if ((v&16) > (state->odrumstat&16)) //BassDrum
	{
	    cellon(state,6,16,&state->cell[6],0);
	    cellon(state,6,19,&state->cell[15],1);
	    state->cell[15].vol *= 2;
	}
	if ((v&8) > (state->odrumstat&8)) //Snare
	{
	    cellon(state,16,20,&state->cell[16],0);
	    state->cell[16].tinc *= 2*(state->nfrqmul[state->adlibreg[17+0x20]&15] / state->nfrqmul[state->adlibreg[20+0x20]&15]);
	    if (((state->adlibreg[20+0xe0]&7) >= 3) && ((state->adlibreg[20+0xe0]&7) <= 5)) state->cell[16].vol = 0;
	    state->cell[16].vol *= 2;
	}
	if ((v&4) > (state->odrumstat&4)) //TomTom
	{
	    cellon(state,8,18,&state->cell[8],0);
	    state->cell[8].vol *= 2;
	}
	if ((v&2) > (state->odrumstat&2)) //Cymbal
	{
	    cellon(state,17,21,&state->cell[17],0);

	    state->cell[17].wavemask = wavemask[5];
	    state->cell[17].waveform = &state->wavtable[waveform[5]];
	    state->cell[17].tinc *= 16; state->cell[17].vol *= 2;

	    //cell[17].waveform = &wavtable[WAVPREC]; cell[17].wavemask = 0;
	    //if (((adlibreg[21+0xe0]&7) == 0) || ((adlibreg[21+0xe0]&7) == 6))
	    //   cell[17].waveform = &wavtable[(WAVPREC*7)>>2];
	    //if (((adlibreg[21+0xe0]&7) == 2) || ((adlibreg[21+0xe0]&7) == 3))
	    //   cell[17].waveform = &wavtable[(WAVPREC*5)>>2];
	}
	if ((v&1) > (state->odrumstat&1)) //Hihat
	{
	    cellon(state,7,17,&state->cell[7],0);
	    if (((state->adlibreg[17+0xe0]&7) == 1) || ((state->adlibreg[17+0xe0]&7) == 4) ||
		((state->adlibreg[17+0xe0]&7) == 5) || ((state->adlibreg[17+0xe0]&7) == 7)) state->cell[7].vol = 0;
	    if ((state->adlibreg[17+0xe0]&7) == 6) { state->cell[7].wavemask = 0; state->cell[7].waveform = &state->wavtable[(WAVPREC*7)>>2]; }
	}

	state->odrumstat = v;
    }
    else if (((unsigned)(i-0x40) < (unsigned)22) && ((i&7) < 6))
    {
	if ((i&7) < 3) // Modulator
	    cellfreq(state,base2cell[i-0x40],i-0x40,&state->cell[base2cell[i-0x40]]);
	else          // Carrier
	    cellfreq(state,base2cell[i-0x40],i-0x40,&state->cell[base2cell[i-0x40]+9]);
    }
    else if ((unsigned)(i-0xa0) < (unsigned)9)
    {
	cellfreq(state,i-0xa0,modulatorbase[i-0xa0],&state->cell[i-0xa0]);
	cellfreq(state,i-0xa0,modulatorbase[i-0xa0]+3,&state->cell[i-0xa0+9]);
    }
    else if ((unsigned)(i-0xb0) < (unsigned)9)
    {
	if ((v&32) > (tmp&32))
	{
	    cellon(state,i-0xb0,modulatorbase[i-0xb0],&state->cell[i-0xb0],0);
	    cellon(state,i-0xb0,modulatorbase[i-0xb0]+3,&state->cell[i-0xb0+9],1);
	}
	else if ((v&32) < (tmp&32))
	    state->cell[i-0xb0].cellfunc = state->cell[i-0xb0+9].cellfunc = docell2;
	cellfreq(state,i-0xb0,modulatorbase[i-0xb0],&state->cell[i-0xb0]);
	cellfreq(state,i-0xb0,modulatorbase[i-0xb0]+3,&state->cell[i-0xb0+9]);
    }

    //outdata(i,v);
}

#ifdef USING_ASM
static const long fpuasm;
static const float fakeadd = 8388608.0+128.0;
static _inline void clipit8 (float f, long a)
{
    _asm
	{
	    mov edi, a
		fld dword ptr f
		fadd dword ptr fakeadd
		fstp dword ptr fpuasm
		mov eax, fpuasm
		test eax, 0x007fff00
		jz short skipit
		shr eax, 16
		xor eax, -1
		skipit: mov byte ptr [edi], al
		}
}

static _inline void clipit16 (float f, long a)
{
    _asm
	{
	    mov eax, a
		fld dword ptr f
		fist word ptr [eax]
		cmp word ptr [eax], 0x8000
		jne short skipit2
		fst dword ptr [fpuasm]
		cmp fpuasm, 0x80000000
		sbb word ptr [eax], 0
		skipit2: fstp st
		}
}
#else
static void clipit8(float f,unsigned char *a) {
    f/=256.0;
    f+=128.0;
    if (f>254.5) *a=255;
    else if (f<0.5) *a=0;
    else *a=f;
}

static void clipit16(float f,short *a) {
    if (f>32766.5) *a=32767;
    else if (f<-32767.5) *a=-32768;
    else *a=f;
}
#endif

void adlibsetvolume(void * state_v, int i) {
	adlib_state * state = (adlib_state *) state_v;
    state->AMPSCALE=i;
}

void adlibgetsample (void * state_v, unsigned char *sndptr, long numbytes)
{
	adlib_state * state = (adlib_state *) state_v;
    long i, j, k=0, ns, endsamples, rptrs, numsamples;
    celltype *cptr;
    float f;
    short *sndptr2=(short *)sndptr;

    numsamples = (numbytes>>(state->numspeakers+state->bytespersample-2));

    if (state->bytespersample == 1) f = state->AMPSCALE/256.0; else f = state->AMPSCALE;
    if (state->numspeakers == 1)
    {
	state->nlvol[0] = state->lvol[0]*f;
	for(i=0;i<9;i++) state->rptr[i] = &state->rbuf[0][0];
	rptrs = 1;
    }
    else
    {
	rptrs = 0;
	for(i=0;i<9;i++)
	{
	    if ((!i) || (state->lvol[i] != state->lvol[i-1]) || (state->rvol[i] != state->rvol[i-1]) ||
		(state->lplc[i] != state->lplc[i-1]) || (state->rplc[i] != state->rplc[i-1]))
	    {
		state->nlvol[rptrs] = state->lvol[i]*f;
		state->nrvol[rptrs] = state->rvol[i]*f;
		state->nlplc[rptrs] = state->rend-min(max(state->lplc[i],0),FIFOSIZ);
		state->nrplc[rptrs] = state->rend-min(max(state->rplc[i],0),FIFOSIZ);
		rptrs++;
	    }
	    state->rptr[i] = &state->rbuf[rptrs-1][0];
	}
    }


    //CPU time used to be somewhat less when emulator was only mono!
    //   Because of no delay fifos!

    for(ns=0;ns<numsamples;ns+=endsamples)
    {
	endsamples = min(FIFOSIZ*2-state->rend,FIFOSIZ);
	endsamples = min(endsamples,numsamples-ns);

	for(i=0;i<9;i++)
	    state->nrptr[i] = &state->rptr[i][state->rend];
	for(i=0;i<rptrs;i++)
	    memset((void *)&state->rbuf[i][state->rend],0,endsamples*sizeof(float));

	if (state->adlibreg[0xbd]&0x20)
	{
				//BassDrum (j=6)
	    if (state->cell[15].cellfunc != docell4)
	    {
		if (state->adlibreg[0xc6]&1)
		{
		    for(i=0;i<endsamples;i++)
		    {
			(state->cell[15].cellfunc)((void *)&state->cell[15],0.0);
			state->nrptr[6][i] += state->cell[15].val;
		    }
		}
		else
		{
		    for(i=0;i<endsamples;i++)
		    {
			(state->cell[6].cellfunc)((void *)&state->cell[6],state->cell[6].val*state->cell[6].mfb);
			(state->cell[15].cellfunc)((void *)&state->cell[15],state->cell[6].val*WAVPREC*MODFACTOR);
			state->nrptr[6][i] += state->cell[15].val;
		    }
		}
	    }

				//Snare/Hihat (j=7), Cymbal/TomTom (j=8)
	    if ((state->cell[7].cellfunc != docell4) || (state->cell[8].cellfunc != docell4) || (state->cell[16].cellfunc != docell4) || (state->cell[17].cellfunc != docell4))
	    {
		for(i=0;i<endsamples;i++)
		{
		    k = k*1664525+1013904223;
		    (state->cell[16].cellfunc)((void *)&state->cell[16],k&((WAVPREC>>1)-1)); //Snare
		    (state->cell[7].cellfunc)((void *)&state->cell[7],k&(WAVPREC-1));       //Hihat
		    (state->cell[17].cellfunc)((void *)&state->cell[17],k&((WAVPREC>>3)-1)); //Cymbal
		    (state->cell[8].cellfunc)((void *)&state->cell[8],0.0);                 //TomTom
		    state->nrptr[7][i] += state->cell[7].val + state->cell[16].val;
		    state->nrptr[8][i] += state->cell[8].val + state->cell[17].val;
		}
	    }
	}
	for(j=9-1;j>=0;j--)
	{
	    if ((state->adlibreg[0xbd]&0x20) && (j >= 6) && (j < 9)) continue;

	    cptr = &state->cell[j]; k = j;
	    if (state->adlibreg[0xc0+k]&1)
	    {
		if ((cptr[9].cellfunc == docell4) && (cptr->cellfunc == docell4)) continue;
		for(i=0;i<endsamples;i++)
		{
		    (cptr->cellfunc)((void *)cptr,cptr->val*cptr->mfb);
		    (cptr->cellfunc)((void *)&cptr[9],0);
		    state->nrptr[j][i] += cptr[9].val + cptr->val;
		}
	    }
	    else
	    {
		if (cptr[9].cellfunc == docell4) continue;
		for(i=0;i<endsamples;i++)
		{
		    (cptr->cellfunc)((void *)cptr,cptr->val*cptr->mfb);
		    (cptr[9].cellfunc)((void *)&cptr[9],cptr->val*WAVPREC*MODFACTOR);
		    state->nrptr[j][i] += cptr[9].val;
		}
	    }
	}

	if (state->numspeakers == 1)
	{
	    if (state->bytespersample == 1)
	    {
		for(i=endsamples-1;i>=0;i--)
		    clipit8(state->nrptr[0][i]*state->nlvol[0],sndptr+1);
	    }
	    else
	    {
		for(i=endsamples-1;i>=0;i--)
		    clipit16(state->nrptr[0][i]*state->nlvol[0],sndptr2+i);
	    }
	}
	else
	{
	    memset((void *)(state->snd),0,endsamples*sizeof(float)*2);
	    for(j=0;j<rptrs;j++)
	    {
		for(i=0;i<endsamples;i++)
		{
		    state->snd[(i<<1)  ] += state->rbuf[j][(state->nlplc[j]+i)&(FIFOSIZ*2-1)]*state->nlvol[j];
		    state->snd[(i<<1)+1] += state->rbuf[j][(state->nrplc[j]+i)&(FIFOSIZ*2-1)]*state->nrvol[j];
		}
		state->nlplc[j] += endsamples;
		state->nrplc[j] += endsamples;
	    }

	    if (state->bytespersample == 1)
	    {
		for(i=(endsamples<<1)-1;i>=0;i--)
		    clipit8(state->snd[i],sndptr+i);
	    }
	    else
	    {
		for(i=(endsamples<<1)-1;i>=0;i--)
		    clipit16(state->snd[i],sndptr2+i);
	    }
	}

	sndptr = sndptr+(state->numspeakers*endsamples);
	sndptr2 = sndptr2+(state->numspeakers*endsamples);
	state->rend = ((state->rend+endsamples)&(FIFOSIZ*2-1));
    }
}
