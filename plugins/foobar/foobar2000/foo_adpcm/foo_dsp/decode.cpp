//================ C R A Z Y N A T I O N ===================================
//==========================================================================
// ADPCM decoder for Nintendo GAMECUBE dsp format, w/ modifications by hcs
//==========================================================================
#include <windows.h>
#include "dsp.h"

// decode 8 bytes of input (1 frame, 14 samples)
int decodebuffer
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