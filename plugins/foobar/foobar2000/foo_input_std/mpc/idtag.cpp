#include "stdafx.h"

// free tagdata from memory
void StreamInfo::Clear()
{

}

StreamInfo::StreamInfo ()
{
    memset ( &simple, 0, sizeof (simple) );
}

__int64 StreamInfo::GetLenthSamples()
{
    __int64 samples = simple.Frames * FRAMELEN;
    if ( simple.IsTrueGapless ) {
        samples -= (FRAMELEN - simple.LastFrameSamples);
    } else {
        samples -= SYNTH_DELAY;
    }
	return samples;
}

double StreamInfo::GetLength()
{
	return (double)GetLenthSamples() / (double)simple.SampleFreq;
}



// searches for a ID3v2-tag and reads the length (in bytes) of it
// -1 on errors of any kind

long
JumpID3v2 ( MPC_Reader& fp )
{
    unsigned char  tmp [10];
    unsigned int   Unsynchronisation;   // ID3v2.4-flag
    unsigned int   ExtHeaderPresent;    // ID3v2.4-flag
    unsigned int   ExperimentalFlag;    // ID3v2.4-flag
    unsigned int   FooterPresent;       // ID3v2.4-flag
    long           ret;

    fp.read  ( tmp, sizeof(tmp) );

    // check id3-tag
    if ( 0 != memcmp ( tmp, "ID3", 3) )
        return 0;

    // read flags
    Unsynchronisation = tmp[5] & 0x80;
    ExtHeaderPresent  = tmp[5] & 0x40;
    ExperimentalFlag  = tmp[5] & 0x20;
    FooterPresent     = tmp[5] & 0x10;

    if ( tmp[5] & 0x0F )
        return -1;              // not (yet???) allowed
    if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 )
        return -1;              // not allowed

    // read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
    ret  = tmp[6] << 21;
    ret += tmp[7] << 14;
    ret += tmp[8] <<  7;
    ret += tmp[9]      ;
    ret += 10;
    if ( FooterPresent )
        ret += 10;

    return ret;
}
