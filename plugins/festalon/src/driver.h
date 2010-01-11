#include "types.h"
#include "festalon.h"

/*	Load an NSF from "buf" of size "size".
	Returns NULL on failure(memory allocation failed).
*/
FESTALON *FESTAI_Load(uint8 *buf, uint32 size);


/*	Intialize emulation to use a sound playback rate of "rate" and a quality of "quality"(0 or 1).
	Quality "1" has a better frequency response in frequencies near the filters' cutoff frequencies.

	This function should be called after FESTAI_Load(), and at least once before any other FESTAI_*() 
	functions are called.
*/
int FESTAI_SetSound(FESTALON *fe, uint32 rate, int quality);

#define FESTAGFI_TAGS		0x1
#define FESTAGFI_TAGS_DATA	0x2

FESTALON *FESTAI_GetFileInfo(uint8 *buf, uint32 size, int type);
void FESTAI_FreeFileInfo(FESTALON *fe);

/* Emulate a segment of sound(approximately equal to the length of sound
   of one video frame on the real system).  Returns a pointer to the sound
   data.  Pass a pointer to an int variable to receive the amount(in samples)
   of sound data generated.
*/
float *FESTAI_Emulate(FESTALON *, int *Count);

/* Sets the sound volume, in percentage.  100 is the default, and should not
   be exceeded by default, as it can cause clipping with a few expansion
   sound chips(this will probably be fixed in the future by reducing the
   total volume of all output when some expansion chips are used).

   Returns the current volume(interal limits are set on how high the volume can be).
*/
int32 FESTAI_SetVolume(FESTALON *, int32 volume);

/* Changes the current song.  The current song is numbered starting from 1.
   If o is zero, adjust the song number by the value specified by z.  Otherwise,
   z will specify the song number.  Returns the adjusted/active song number.
*/
int FESTAI_SongControl(FESTALON *, int z, int o);

/* Free all memory associated with the FESTALON "object" pointer passed */
void FESTAI_Close(FESTALON *);

/* Specify a channel disable mask.  The lower 5 bits(least-bit first) correspond 
   to the internal sound channel, and the adjacent higher bits correspond
   to the expansion sound channel(s), if present.
*/
void FESTAI_Disable(FESTALON *, int);

int FESTAI_SetLowpass(FESTALON *, int on, uint32 corner, uint32 order);


/* Creates an NSF in memory, returning a pointer to it, and setting "totalsize" to
   the size of the buffer/NSF.
   Returns 0 on failure.
*/
uint8 *FESTAI_CreateNSF(FESTALON *fe, uint32 *totalsize);

uint8 *FESTAI_CreateNSFE(FESTALON *fe, uint32 *totalsize);
