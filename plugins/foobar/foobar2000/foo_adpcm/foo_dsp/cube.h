#define f32 float
#define u8	unsigned char
#define s16 signed short
#define u16 unsigned short
#define s32 signed long
#define u32 unsigned long

#define BPS 16

#include <foobar2000.h>

// both header and file type
typedef enum {
	type_std,   // devkit standard DSP
	type_sfass, // Star Fox Assault Cstr (DSP)
	type_mp2,   // Metroid Prime 2 RS03 (DSP)
	type_pm2,   // Paper Mario 2 STM (DSP)
	type_halp,  // HALPST (DSP)
	type_mp2d,  // Metroid Prime 2 Demo (DSP)
	type_idsp,  // IDSP (DSP)
	type_spt,   // SPT+SPD (DSP)
	type_mss,   // MSS (DSP)
	type_gcm,   // GCM (DSP)
	type_adp	// ADP
} headertype;

// structure represents header for a channel
typedef struct {
	u32 num_samples;
	u32 num_adpcm_nibbles;
	u32 sample_rate;
	u16 loop_flag;
	u16 format;
	u32 sa,ea,ca;
	s16 coef[16];
	u16 gain; // never used anyway
	u16 ps,yn1,yn2;
	u16 lps,lyn1,lyn2;
} CUBEHEAD;

// structure for a single channel
typedef struct {
	service_ptr_t<file> infile; // processing on a single channel needs this,also this allows for seperate L/R files to be played at once
	DSPHEAD header;
	short chanbuf[0x8000/8*14];
	int readloc,writeloc; // offsets into the chanbuf
	int filled;
	headertype type;
	short bps; // bits per "size", 4 if offsets specified in nibbles, 8 if bytes
	t_filesize chanstart; // offset in file of start of this channel
	t_filesize offs; // current location
	short hist1,hist2; // sample history
	long interleave; // _bytes_ of interleave, 0 if none
} CUBESTREAM;

// structure represents a DSP file
typedef struct {
	CUBESTREAM ch[2];
	int NCH;
	long nrsamples;
	long file_length;
	long nexthalp; // next HALPST header
	long halpsize;
	int lastchunk;
	long startinterleave;
} CUBEFILE;

void InitDSPFILE(DSPFILE * dsp, abort_callback & p_abort, headertype type = type_std);
//void CloseDSPFILE(DSPFILE * dsp);

int DSPdecodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short		coef[16],   // location of adpcm info
	short * histp,
	short * hist2p
);

int ADPdecodebuffer(
	unsigned char *input,
	short *outl,
	short *outr,
	long *histl1,
	long *histl2,
	long *histr1,
	long *histr2
);

void fillbuffers(DSPFILE * dsp, abort_callback & p_abort);

extern DSPFILE dspfile;
