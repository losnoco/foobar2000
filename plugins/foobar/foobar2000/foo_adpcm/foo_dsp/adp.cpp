/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop, bero, El Barto, and Rob

*/

// DLS (a.k.a. ADP)
#include <foobar2000.h>
#include "cube.h"

// inputfile == NULL means file is already opened, just reload
// return 1 if valid ADP not detected, 0 on success
bool InitADPFILE(CUBEFILE * adp, abort_callback & p_abort) {
	char readbuf[4];

	adp->ch[0].infile->seek( 0, p_abort );

	// check for valid first frame
	adp->ch[0].infile->read_object( readbuf, 4, p_abort );
	if (readbuf[0]!=readbuf[2] || readbuf[1]!=readbuf[3]) {
		return false;
	}

	t_filesize size = adp->ch[0].infile->get_size( p_abort );

	adp->ch[0].type=type_adp;

	
	adp->NCH = 2;
	adp->ch[0].header.sample_rate = 48000;
	adp->nrsamples = size*7/8;
	adp->ch[0].header.loop_flag=0;
	
    adp->file_length=size;

	adp->ch[0].lhist1 = 0;
    adp->ch[0].lhist2 = 0;
    adp->ch[1].lhist1 = 0;
    adp->ch[1].lhist2 = 0;

	adp->ch[0].readloc=adp->ch[1].readloc=adp->ch[0].writeloc=adp->ch[1].writeloc=0;
	adp->ch[0].filled=adp->ch[1].filled=0;

	adp->ch[0].infile->seek( 0, p_abort );

	return true;
}

bool fillbufferADP(CUBEFILE * adp, abort_callback & p_abort) {
	unsigned char ADPCMbuf[32];

	do {
		unsigned read = adp->ch[0].infile->read( ADPCMbuf, 32, p_abort );
		if ( read < 32 ) return false;
		ADPdecodebuffer(ADPCMbuf,adp->ch[0].chanbuf+adp->ch[0].writeloc,
								 adp->ch[1].chanbuf+adp->ch[1].writeloc,
					&adp->ch[0].lhist1, &adp->ch[0].lhist2, &adp->ch[1].lhist1, &adp->ch[1].lhist2);

		adp->ch[0].writeloc+=28;
		adp->ch[1].writeloc+=28;
		adp->ch[0].filled+=28;
		adp->ch[1].filled+=28;

		if (adp->ch[0].writeloc>=0x8000/8*14) adp->ch[0].writeloc=0;
		if (adp->ch[1].writeloc>=0x8000/8*14) adp->ch[1].writeloc=0;
	} while (adp->ch[0].writeloc != adp->ch[0].readloc);

	return true;
}

DECLARE_FILE_TYPE("GCN ADP audio files", "*.ADP");
