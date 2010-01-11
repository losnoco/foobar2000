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
	type_std,
	type_sfass,
	type_mp2,
	type_pm2,
	type_halp,
	type_mp2d
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
} DSPHEAD;

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
} DSPSTREAM;

// structure represents a DSP file
typedef struct {
	DSPSTREAM ch[2];
	int NCH;
	long nrsamples;
	long file_length;
	long nexthalp; // next HALPST header
	long halpsize;
	int lastchunk;
	long startinterleave;
} DSPFILE;

void InitDSPFILE(DSPFILE * dsp, abort_callback & p_abort);
//void CloseDSPFILE(DSPFILE * dsp);

int decodebuffer
(
    u8			*input, // location of encoded source samples
    s16         *out,   // location of destination buffer (16 bits / sample)
    short		coef[16],   // location of adpcm info
	short * histp,
	short * hist2p
);

void fillbuffers(DSPFILE * dsp, abort_callback & p_abort);

extern DSPFILE dspfile;
