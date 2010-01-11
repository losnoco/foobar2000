#ifndef IMA_ADPCM_H
#define IMA_ADPCM_H

//#define STRICT_IMA

#define ISSTMAX 88

extern const int imaStepSizeTable[ISSTMAX + 1];
extern unsigned char imaStateAdjustTable[ISSTMAX+1][8];

#endif