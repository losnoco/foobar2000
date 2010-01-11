//================ C R A Z Y N A T I O N ===================================
//==========================================================================
// ADPCM decoder for Nintendo GAMECUBE dsp format, w/ modifications by hcs
//==========================================================================
#include <windows.h>
#include "cube.h"

// decode 8 bytes of input (1 frame, 14 samples)
int DSPdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short		coef[16],   // location of adpcm info
	short * histp,
	short * hist2p
)
{
int sample;
short nibbles[14];
int index1;
int i,j;
char *src,*dst;
short delta;
short hist=*histp;
short hist2=*hist2p;

	dst = (char*)out;

	src= (char*) input;
    i = *src&0xFF;
    src++;
    delta = 1 << (i & 255 & 15);
    index1 = (i & 255) >> 4;

    for(i = 0; i < 14; i = i + 2) {
		j = ( *src & 255) >> 4;
		nibbles[i] = j;
		j = *src & 255 & 15;
		nibbles[i+1] = j;
		src++;
	}

     for(i = 0; i < 14; i = i + 1) {
		if(nibbles[i] >= 8) 
			nibbles[i] = nibbles[i] - 16;       
     }
     
	 for(i = 0; i<14 ; i = i + 1) {

		sample = (delta * nibbles[i])<<11;
		sample += coef[index1*2] * hist;
		sample += coef[index1*2+1] * hist2;
		sample = sample + 1024;
		sample = sample >> 11;
		if(sample > 32767) {
			sample = 32767;
		}
		if(sample < -32768) {
			sample = -32768;
		}

        *(short*)dst = (short)sample;
        dst = dst + 2;

		hist2 = hist;
        hist = (short)sample;
        
    }
	*histp=hist;
	*hist2p=hist2;

    return((int)src);
}

// ADP decoder function by hcs, reversed from dtkmake (trkmake v1.4)

#define ONE_BLOCK_SIZE		32
#define SAMPLES_PER_BLOCK	28

short ADPDecodeSample( int bits, int q, long * hist1p, long * hist2p) {
	long hist,cur;
	long hist1=*hist1p,hist2=*hist2p;
	
	switch( q >> 4 )
	{
	case 0:
		hist = 0;
		break;
	case 1:
		hist = (hist1 * 0x3c);
		break;
	case 2:
		hist = (hist1 * 0x73) - (hist2 * 0x34);
		break;
	case 3:
		hist = (hist1 * 0x62) - (hist2 * 0x37);
		break;
	//default:
	//	hist = (q>>4)*hist1+(q>>4)*hist2; // a bit weird but it's in the code, never used
	}
	hist=(hist+0x20)>>6;
	if (hist >  0x1fffff) hist= 0x1fffff;
	if (hist < -0x200000) hist=-0x200000;

	cur = ( ( (short)(bits << 12) >> (q & 0xf)) << 6) + hist;
	
	*hist2p = *hist1p;
	*hist1p = cur;

	cur>>=6;

	if ( cur < -0x8000 ) return -0x8000;
	if ( cur >  0x7fff ) return  0x7fff;

	return (short)cur;
}

// decode 32 bytes of input (28 samples), assume stereo
int ADPdecodebuffer(unsigned char *input, short *outl, short * outr, long *histl1, long *histl2, long *histr1, long *histr2) {
	int i;
	for( i = 0; i < SAMPLES_PER_BLOCK; i++ )
	{
		outl[i] = ADPDecodeSample( input[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] & 0xf, input[0], histl1, histl2 );
		outr[i] = ADPDecodeSample( input[i + (ONE_BLOCK_SIZE - SAMPLES_PER_BLOCK)] >> 4, input[1], histr1, histr2 );
	}
	return 0;
}