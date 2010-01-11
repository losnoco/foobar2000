#ifndef _HES_H_
#define _HES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* PSG structure */
typedef struct {
	uint32 counter;
        uint16 frequency;       /* Channel frequency */
        uint8 control;          /* Channel enable, DDA, volume */
        uint8 balance;          /* Channel balance */
        uint8 waveform[32];     /* Waveform data */
        uint8 waveform_index;   /* Waveform data index */
        uint8 dda;              /* Recent data written to waveform buffer */
        uint8 noisectrl;        /* Noise enable/ctrl (channels 4,5 only) */
	uint32 noisecount;
	uint32 lfsr;
	int32 dda_cache[2];
	int32 last_amp[2];

	void * blip_buffer[2];
} psg_channel;

typedef struct {
    uint8 select;               /* Selected channel (0-5) */
    uint8 globalbalance;        /* Global sound balance */
    uint8 lfofreq;              /* LFO frequency */
    uint8 lfoctrl;              /* LFO control */
    psg_channel channel[8];	// Really just 6...

    uint32 lastts;
    
    void * blip_synth;
}t_psg;

typedef struct
{
	uint8 IBP[0x2000];		/* Itty-bitty player thing. */

	uint8 ram[0x8000];          /* Work RAM */
	uint8 rom[0x100000];        /* HuCard ROM */


	uint8 dummy_read[0x2000];   /* For unmapped reads */
	uint8 dummy_write[0x2000];  /* For unmapped writes */
	uint8 *read_ptr[8];         /* Read pointers in logical address space */
	uint8 *write_ptr[8];        /* Write pointers in logical address space */

	uint8 mpr_start[8];

	/* I/O port data */
	struct {
	    uint8 sel;              /* Status of SEL line */
	    uint8 clr;              /* Status of CLR line */
	}ctrl;

	struct {
	    uint8 *read;            /* Read pointers in physical address space */
	    uint8 *write;           /* Write pointers in physical address space */
	} bank_map[0x100];


	/* VDC */
	uint16 reg[0x20];       /* VDC registers */
	uint8 latch;            /* VDC register latch */
	uint8 status;           /* Status flag */

	t_psg psg;

	void *h6280;
} FESTALON_HES;


uint32 FESTAHES_Emulate(FESTALON_HES *);
void FESTAHES_SongControl(FESTALON_HES *, int which);
void FESTAHES_Close(FESTALON_HES *);
FESTALON_HES *FESTAHES_Load(uint8 *buf, uint32 size);

#ifdef __cplusplus
}
#endif

#endif
