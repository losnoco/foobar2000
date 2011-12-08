/***************************************************************************

  Capcom System QSound(tm)
  ========================

  Driver by Paul Leaman (paul@vortexcomputing.demon.co.uk)
		and Miguel Angel Horna (mahorna@teleline.es)

  A 16 channel stereo sample player.

  QSpace position is simulated by panning the sound in the stereo space.

  Register
  0	 xxbb	xx = unknown bb = start high address
  1	 ssss	ssss = sample start address
  2	 pitch
  3	 unknown (always 0x8000)
  4	 loop offset from end address
  5	 end
  6	 master channel volume
  7	 not used
  8	 Balance (left=0x0110  centre=0x0120 right=0x0130)
  9	 unknown (most fixed samples use 0 for this register)

  Many thanks to CAB (the author of Amuse), without whom this probably would
  never have been finished.

  If anybody has some information about this hardware, please send it to me
  to mahorna@teleline.es or 432937@cepsz.unizar.es.
  http://teleline.terra.es/personal/mahorna

***************************************************************************/

#include <math.h>
#include "cpuintrf.h"
#include "qsound.h"

/*
Two Q sound drivers:
DRIVER1 Based on the Amuse source
DRIVER2 Miguel Angel Horna (mahorna@teleline.es)
*/
#define QSOUND_DRIVER1	  1

// Alex added :  made own routines, which include down and upsampling
//#define QSOUND_DRIVER1_AIMOD 0
#define QSOUND_DRIVER1_AIMOD 1
#define MEMORY_CHECK 0
// bits of one sample period
#define TIME_BITS (16)
#define TIME_ONE (1 << TIME_BITS)
#pragma warning (disable:4244)
// for what I have seen so far, there are no mono samples (looked only in MegaMan 2)
// but using this macro as a switch, will make the reasons for things, more obvious in the code
// so that will know what to do, if there actually this thing is volatile
//#define STEREO_SAMPLES 0
#define STEREO_SAMPLES 1

/*
I don't know whether this system uses 8 bit or 16 bit samples.
If it uses 16 bit samples then the sample ROM loading macros need
to be modified to work with non-intel machines.
*/
#define QSOUND_8BIT_SAMPLES 1

/*
Debug defines
*/
#define LOG_WAVE	0
#define LOG_QSOUND  0

/* Typedefs & defines */

#define QSOUND_DRIVER2 (! QSOUND_DRIVER1)

#if QSOUND_8BIT_SAMPLES
// 8 bit source ROM samples
typedef signed char QSOUND_SRC_SAMPLE;
#define LENGTH_DIV 1
#else
// 16 bit source ROM samples
typedef signed short QSOUND_SRC_SAMPLE;
#define LENGTH_DIV 2
#endif

#define QSOUND_CLOCKDIV 166			 /* Clock divider */
#define QSOUND_CHANNELS 16
typedef INT16 QSOUND_SAMPLE;

struct QSOUND_CHANNEL
{
	int bank;	   /* bank (x16)	*/
	int address;	/* start address */
	int pitch;	  /* pitch */
	int reg3;	   /* unknown (always 0x8000) */
	int loop;	   /* loop address */
	int end;		/* end address */
	int vol;		/* master volume */
	int pan;		/* Pan value */
	int reg9;	   /* unknown */

	/* Work variables */
	int key;		/* Key on / key off */

#if QSOUND_DRIVER1
	int lvol;	   /* left volume */
	int rvol;	   /* right volume */
#if ! QSOUND_DRIVER1_AIMOD
	int lastdt;	 /* last sample value */
#endif
	int offset;	 /* current offset counter */
#else
	QSOUND_SRC_SAMPLE *buffer;
	int factor;		   /*step factor (fixed point 8-bit)*/
	int mixl,mixr;		/*mixing factor (fixed point)*/
	int cursor;		   /*current sample position (fixed point)*/
	int lpos;			 /*last cursor pos*/
	int lastsaml;		 /*last left sample (to avoid any calculation)*/
	int lastsamr;		 /*last right sample*/
#endif

// Alex added
#if QSOUND_DRIVER1_AIMOD
	int beg;
// whole sample size; it is 'end - beg'
	int len;
// I prefer to use indices; this lets to wrap the sample around, so that it would end on a lower address than on which it has began;
// integer offset; increases after producing a sample
	int ix;
	int ix_beg_loop;
//// *isn't really necessary
	int got_sample;
	int value;
#endif
};
// Alex added
typedef struct QSOUND_CHANNEL qsound_channel_t;


/* Private variables */
static struct QSound_interface *intf;	/* Interface  */
static int qsound_stream;				/* Audio stream */
static qsound_channel_t qsound_channel[QSOUND_CHANNELS];
// Alex :  16 bit
static int qsound_data;				  /* register latch data */
QSOUND_SRC_SAMPLE *qsound_sample_rom;	/* Q sound sample ROM */

#if QSOUND_DRIVER1
static int qsound_pan_table[33];		 /* Pan volume table */
static float qsound_frq_ratio;		   /* Frequency ratio */
#endif

#if LOG_WAVE
static FILE *fpRawDataL;
static FILE *fpRawDataR;
#endif

/* Function prototypes */
void qsound_update( int num, INT16 **buffer, int length );
void qsound_set_command(int data, int value);

#if QSOUND_DRIVER2
void setchannel(int channel,signed short *buffer,int length,int vol,int pan);
void setchloop(int channel,int loops,int loope);
void stopchan(int channel);
void calcula_mix(int channel);
#endif

// Alex added
#if QSOUND_DRIVER1_AIMOD
#define PI 3.14159265358979323846264338327950288
// 4 bits gives hardly noticable difference
//#define RESAMPLE_TABLE_BITS 3
//#define RESAMPLE_TABLE_BITS 4
//#define RESAMPLE_TABLE_BITS 6
#define RESAMPLE_TABLE_BITS 8
//#define RESAMPLE_TABLE_BITS 14
//#define RESAMPLE_TABLE_BITS 16
#define RESAMPLE_TABLE_SIZE (1 << RESAMPLE_TABLE_BITS)
// and this on my ear is good above 8
//#define RESAMPLE_N 1
//#define RESAMPLE_N 2
//#define RESAMPLE_N 3
//#define RESAMPLE_N 4
//#define RESAMPLE_N 8
#define RESAMPLE_N 16
//#define RESAMPLE_N 32
//#define RESAMPLE_N 64
//#define RESAMPLE_N 128
#define RESAMPLE_ROW_SIZE (RESAMPLE_N * 2)
double resample_table[RESAMPLE_TABLE_SIZE][RESAMPLE_ROW_SIZE] = {0};
// heh, managing to keep/rotate old values is an additional algo; what will be slower :  manage old values and add one new, or just get new ones ?
// I don't think it will be any sensitively more slower, if don't bother with old values and fetch (some of) those values again
// correction :  it is not necessary to rotate the buffer, but good to keep the last ones
// note :  this can't be used when there is a decoding being performed; if there is decoding, make the decoding buffer usable for this purpose, instead of try to remake this buffering
struct resample_buf_t
{
#if MEMORY_CHECK
	int debug_0;
#endif
	int samples[RESAMPLE_ROW_SIZE];
// info which is necessary for the case of old values rotation
#if 0
// what has 'samples[0]'; its position
	int beg;
// how much samples are in
	int len;
#endif
// what has 'samples[RESAMPLE_N - 1]'; the position of it
	int pos;
#if MEMORY_CHECK
	int debug_1;
#endif
};
typedef struct resample_buf_t resample_buf_t;
//int resample_sample_buf_beg[QSOUND_CHANNELS][RESAMPLE_ROW_SIZE] = {0};
resample_buf_t resample_buf[QSOUND_CHANNELS] = {0};

#define assert(_condition) do { if (! (_condition)) __asm int 3 } while (0)
#define SA(_expr) typedef char _static_assert_t[!! (_expr)]
#endif


int  qsound_sh_start( struct QSound_interface *qsintf )
{
	int i;

	intf = qsintf;

	qsound_sample_rom = (QSOUND_SRC_SAMPLE *)intf->sample_rom;

	memset(qsound_channel, 0, sizeof(qsound_channel));

#if QSOUND_DRIVER1
	qsound_frq_ratio = ((float)intf->clock / (float)QSOUND_CLOCKDIV) / (float)AUDIO_RATE;
	qsound_frq_ratio *= 16.0;
#if QSOUND_DRIVER1_AIMOD
#if STEREO_SAMPLES
	qsound_frq_ratio /= 2;
#endif
#endif

	/* Create pan table */
	for (i=0; i<33; i++)
	{
		qsound_pan_table[i]=(int)((256/sqrt(32)) * sqrt(i));
	}
#else
	i=0;
#endif

#if LOG_QSOUND
	logerror("Pan table\n");
	for (i=0; i<33; i++)
		logerror("%02x ", qsound_pan_table[i]);
#endif
#if 0
	{
		/* Allocate stream */
#define CHANNELS ( 2 )
		char buf[CHANNELS][40];
		const char *name[CHANNELS];
		int  vol[2];
		name[0] = buf[0];
		name[1] = buf[1];
		sprintf( buf[0], "%s L", sound_name(msound) );
		sprintf( buf[1], "%s R", sound_name(msound) );
		vol[0]=MIXER(intf->mixing_level[0], MIXER_PAN_LEFT);
		vol[1]=MIXER(intf->mixing_level[1], MIXER_PAN_RIGHT);
		qsound_stream = stream_init_multi(
			CHANNELS,
			name,
			vol,
			Machine->sample_rate,
			0,
			qsound_update );
	}
#endif
#if LOG_WAVE
	fpRawDataR=fopen("qsoundr.raw", "w+b");
	fpRawDataL=fopen("qsoundl.raw", "w+b");
	if (!fpRawDataR || !fpRawDataL)
	{
		return 1;
	}
#endif

	return 0;
}

void qsound_sh_stop (void)
{
#if LOG_WAVE
	if (fpRawDataR)
	{
		fclose(fpRawDataR);
	}
	if (fpRawDataL)
	{
		fclose(fpRawDataL);
	}
#endif
}

void qsound_data_h_w(int data)
{
	qsound_data=(qsound_data&0xff)|(data<<8);
}

void qsound_data_l_w(int data)
{
	qsound_data=(qsound_data&0xff00)|data;
}

void qsound_cmd_w(int data)
{
//	printf("QS: cmd %x, data %x\n", data, qsound_data);
	qsound_set_command(data, qsound_data);
}

int qsound_status_r(void)
{
	/* Port ready bit (0x80 if ready) */
	return 0x80;
}

#if QSOUND_DRIVER1_AIMOD
// Alex :  management of 'end' and 'loop' is behaviour of hardware, which needs that hardware, to be found out, so I couldn't say
// more dobtful that it also changes behaviour of 'ch->loop'
#define QSOUND_INCLUSIVE_END 0
//#define QSOUND_INCLUSIVE_END 1
void Qsound_InitChannel (int channel)
{
	int a;
	int len;
	qsound_channel_t* ch;

	ch = &qsound_channel[channel];
	ch->value = 0;

// this is the normal full length
// *by what I see, I would say that 'end' is an inclusive address (values like 2fff)
// 'end' samples :  2fff, dffc, 6c1d, 0065, 00dd
	len = ch->end + QSOUND_INCLUSIVE_END - ch->address;
// case where 'beg == end'
//// *actually, all these abnormalities need not to be supported
	if (! len)
		len = 0x00010000;
// but let the actual length to be bigger
// 'loop' samples (suspecting that it really subtracts from 'end') :  0885, 4534, 569a, 005a, 00d9
	if (len < ch->loop)
		len = ch->loop;
	ch->len = len;

// beginning of sample
	a = ch->end + QSOUND_INCLUSIVE_END - len;
	a &= 0x0000ffff;
	ch->beg = a;

// beginning of loop
// let the loop to begin before the point of first start
//	a = ch->len - ch->loop;
	a = ch->len - QSOUND_INCLUSIVE_END - ch->loop;
// even not this
//	a &= 0x0000ffff;
// but this
	assert(a >= 0);
	ch->ix_beg_loop = a;
// let the sample to start from nonzero position; it is - not from the beginning
	ch->ix = ch->address - ch->beg;

// invalid value
	resample_buf[channel].pos = -1;
#if MEMORY_CHECK
	resample_buf[channel].debug_0 = 0x12345678;
	resample_buf[channel].debug_1 = 0x87654321;
#endif

#define DUMP_SAMPLES 0
//#define DUMP_SAMPLES 1
#if DUMP_SAMPLES
	{
	FILE* f;
	char filename[1024];
	char* wave;
	QSOUND_SRC_SAMPLE* pST;
	int i;

	pST = qsound_sample_rom + ch->bank;
	wave = pST + ch->beg;

// analyze on stereo, because I would like to find out - are there mono samples also or there are none of those
//#define SKIP_STEREO 0
#define SKIP_STEREO 1
#if SKIP_STEREO
	{
	int len_aligned;
	int v;
//	int v_old;
	__int64 difference_1;
	__int64 difference_2;
	__int64 delta_difference;
	int diff_1;
	int diff_2;
	int delta_diff;
	double d;

	difference_1 = 0;
	difference_2 = 0;
// *prevent division by zero
	difference_2 += 1;
	delta_difference = 0;
// the last samples will not be with what to compare
	len_aligned = ch->len - 2;
	len_aligned &= -2;
	for (i = 0; i < len_aligned; i += 2)
	{
		v = wave[i];
// by what I see in diagrams, those two waves mostly differ sensitively
// often are like with a bit displaced phase, or so
		diff_1 = wave[i + 1] - v;
		diff_2 = wave[i + 2] - v;
// this value shows a duplicated mono sounds, when it is low
		difference_1 += abs(diff_1);
// this is a bearing/orienting/reference value
// note, that it will be doubled in the case of a mono sound
		difference_2 += abs(diff_2);
// mono samples will mostly have values near 'diff_1 * 2 == diff_2', except if that sample mostly consists of high-frequency
		delta_diff = diff_2 / 2 - diff_1;
// this value must get high, if there are significant difference between two supposed waves
		delta_difference += abs(delta_diff);
//		v_old = v;
	}
	d = difference_1;
	d /= difference_2;
// this is a mono sound duplicated to stereo, be sure
	if (d < 0.001)
		goto finish_test;
	d = delta_difference;
	d /= difference_2;
// and define the limit for this value is much harder;
// if that sound does not consist of high frequencies, this value will be very low
// but samples here mostly differ very much between stereo channels
	if (d > 0.5)
//	if (d > 1.0)
		goto finish_test;
	}
#endif

	sprintf (filename, "%02x_%04x_%04x.smpl", ch->bank >> 16, ch->beg, ch->len);
	f = fopen (filename, "wb");
	if (! f)
		goto finish_test;
	for (i = 0; i < ch->len; i++)
	{
		fwrite (&wave[i], 1, 1, f);
	}
	fclose (f);
finish_test:
	;
	}
#endif
}
#endif

// Alex added :  keyoff always, if something changes
#define ALWAYS_KEYOFF 0
//#define ALWAYS_KEYOFF 1
// reinit sound on any event
#define ALWAYS_REINIT 0
//#define ALWAYS_REINIT 1

// Alex :  'value' is 16 bit (that of course must be meant in the variable type, but this is not my code)
void qsound_set_command (int data, int value)
{
	int ch=0,reg=0;

	if (data < 0x80)
	{
		ch=data>>3;
		reg=data & 0x07;
	}
	else
	{
		if (data < 0x90)
		{
			ch=data-0x80;
			reg=8;
		}
		else
		{
			if (data >= 0xba && data < 0xca)
			{
				ch=data-0xba;
				reg=9;
			}
			else
			{
				/* Unknown registers */
				ch=99;
				reg=99;
			}
		}
	}

#if 0
#if ALWAYS_REINIT
		Qsound_InitChannel (ch);
#endif
#if ALWAYS_KEYOFF
		qsound_channel[ch].key = 0;
#endif
#endif

	switch (reg)
	{
	case 0: /* Bank */
		ch = (ch + 1) & 0x0f;	/* strange ... */
		qsound_channel[ch].bank = (value & 0x7f) << 16;
		qsound_channel[ch].bank /= LENGTH_DIV;
#if ALWAYS_KEYOFF
		qsound_channel[ch].key = 0;
#endif
#ifdef MAME_DEBUG
		if (! value & 0x8000)
		{
			char baf[40];
			sprintf(baf,"Register3=%04x",value);
			usrintf_showmessage(baf);
		}
#endif

		break;
	case 1: /* start */
//		printf("QS: key on ch %02d\n", ch);
		qsound_channel[ch].address = value;
		qsound_channel[ch].address /= LENGTH_DIV;
#if ALWAYS_REINIT
		Qsound_InitChannel (ch);
#endif
#if ALWAYS_KEYOFF
		qsound_channel[ch].key = 0;
#endif
		break;
	case 2: /* pitch */
#if QSOUND_DRIVER1
		qsound_channel[ch].pitch = (long)((float)value * qsound_frq_ratio);
		qsound_channel[ch].pitch /= LENGTH_DIV;
#else
		qsound_channel[ch].factor = ((float)(value*(6/LENGTH_DIV)) / (float)Machine->sample_rate) * 256.0;
#endif
		if (! value)
		{
			/* Key off */
//			printf("QS: key off ch %02d\n", ch);
			qsound_channel[ch].key = 0;
		}
		break;
	case 3: /* unknown */
		qsound_channel[ch].reg3=value;
#ifdef MAME_DEBUG
		if (value != 0x8000)
		{
			char baf[40];
			sprintf(baf,"Register3=%04x",value);
			usrintf_showmessage(baf);
		}
#endif
		break;
	case 4: /* loop offset */
		qsound_channel[ch].loop = value / LENGTH_DIV;
#if ALWAYS_REINIT
		Qsound_InitChannel (ch);
#endif
#if ALWAYS_KEYOFF
		qsound_channel[ch].key = 0;
#endif
		break;
	case 5: /* end */
		qsound_channel[ch].end = value / LENGTH_DIV;
#if ALWAYS_REINIT
		Qsound_InitChannel (ch);
#endif
#if ALWAYS_KEYOFF
		qsound_channel[ch].key = 0;
#endif
		break;
	case 6: /* master volume */
		if (! value)
		{
			/* Key off */
			qsound_channel[ch].key = 0;
		}
		else if (! qsound_channel[ch].key)
		{
			/* Key on */
			qsound_channel[ch].key = 1;
#if QSOUND_DRIVER1
			qsound_channel[ch].offset = 0;
// Alex added :  key on is the place for values calculation/check
#if QSOUND_DRIVER1_AIMOD
			qsound_channel[ch].value = 0;
			Qsound_InitChannel (ch);
#else
			qsound_channel[ch].lastdt = 0;
#endif
#else
			qsound_channel[ch].cursor=qsound_channel[ch].address <<8 ;
			qsound_channel[ch].buffer=qsound_sample_rom + qsound_channel[ch].bank;
#endif
		}
		qsound_channel[ch].vol=value;
#if QSOUND_DRIVER2
		calcula_mix(ch);
#endif
		break;

	case 7:  /* unused */
#ifdef MAME_DEBUG
		{
			char baf[40];
			sprintf(baf,"UNUSED QSOUND REG 7=%04x",value);
			usrintf_showmessage(baf);
		}
#endif

		break;
	case 8:
		{
#if QSOUND_DRIVER1
		   int pandata=(value-0x10)&0x3f;
		   if (pandata > 32)
		   {
				pandata=32;
		   }
		   qsound_channel[ch].rvol=qsound_pan_table[pandata];
		   qsound_channel[ch].lvol=qsound_pan_table[32-pandata];
#endif
		   qsound_channel[ch].pan = value;
#if QSOUND_DRIVER2
		   calcula_mix(ch);
#endif
		}
		break;
	 case 9:
		qsound_channel[ch].reg9=value;
/*
#ifdef MAME_DEBUG
		{
			char baf[40];
			sprintf(baf,"QSOUND REG 9=%04x",value);
			usrintf_showmessage(baf);
		}
#endif
*/
		break;
	}
#if LOG_QSOUND
	logerror("QSOUND WRITE %02x CH%02d-R%02d =%04x\n", data, ch, reg, value);
#endif
}

#if DISABLE_XSF_TESTS
#else
extern UINT32 dwChannelMute;
#endif

#if QSOUND_DRIVER1

#if ! QSOUND_DRIVER1_AIMOD
/* Driver 1 - based on the Amuse source */

void qsound_update( int num, INT16 **buffer, int length )
{
	int i,j;
	int rvol, lvol, count;
	qsound_channel_t* pC = &qsound_channel[0];
	QSOUND_SRC_SAMPLE* pST;
	QSOUND_SAMPLE* datap[2];

	datap[0] = buffer[0];
	datap[1] = buffer[1];
	memset (datap[0], 0x00, length * sizeof(QSOUND_SAMPLE));
	memset (datap[1], 0x00, length * sizeof(QSOUND_SAMPLE));

	for (i = 0; i < QSOUND_CHANNELS; i++)
	{
		if (pC->key)
		{
			QSOUND_SAMPLE* pOutL = datap[0];
			QSOUND_SAMPLE* pOutR = datap[1];
			pST = qsound_sample_rom + pC->bank;

#define VOL_SHIFT (8 * LENGTH_DIV)
			rvol = pC->rvol * pC->vol >> VOL_SHIFT;
			lvol = pC->lvol * pC->vol >> VOL_SHIFT;

			for (j = length - 1; j >= 0; j--)
			{
// take the integer amount
				count = pC->offset >> 16;
// leave only the fractional part
				pC->offset &= 0xffff;
				if (count)
				{
					pC->address += count;
					if (pC->address >= pC->end)
					{
						if (! pC->loop)
						{
							/* Reached the end of a non-looped sample */
							pC->key=0;
							break;
						}
						/* Reached the end, restart the loop */
						pC->address = (pC->end - pC->loop) & 0xffff;
					}
					pC->lastdt = pST[pC->address];
				}

				if (! (dwChannelMute & (1 << i)))
				{
					*pOutL += (pC->lastdt * lvol) >> 6;
					*pOutR += (pC->lastdt * rvol) >> 6;
				}
				pOutL++;
				pOutR++;
				pC->offset += pC->pitch;
			}
		}
		pC++;
	}

#if LOG_WAVE
	fwrite(datap[0], length*sizeof(QSOUND_SAMPLE), 1, fpRawDataL);
	fwrite(datap[1], length*sizeof(QSOUND_SAMPLE), 1, fpRawDataR);
#endif
}

// Alex version
#else

// ----------------------------------------------------------------

// not used, but while I clearly have idea what is what, I am writting
void Resample_GetValueOnSincKernel (double* val, double offset)
{
	double x;
//	double x1;
	double v;

// http://en.wikipedia.org/wiki/Sinc_function , normalized sinc function
// sinc(x) = sin(pi*x) / pi*x
	x = offset;
// normalize sinc
	x *= PI;
	v = sin (x);
	*val = v / x;
}

void Resample_GetValueOnLanczosKernel (double* val, double offset)
{
	double x;
	double x1;
	double v;

// http://en.wikipedia.org/wiki/Lanczos_resampling , Definition
// sinc(x) * sinc(x/a) = a*sin(pi*x)*sin(pi*x/a) / pi^2*x^2
	x = offset;
// normalize sinc
	x *= PI;
	v = sin (x);
// pi * x / a
	x1 = x / RESAMPLE_N;
	v *= sin (x1);
// pi * pi * x * x / a
	x = x * x1;
	*val = v / x;
}

#define RESAMPLE_DUMP_TABLE 0
//#define RESAMPLE_DUMP_TABLE 1

// row which will be used to generate single output sample, at processing time
void Resample_GenerateRow (double(* row)[RESAMPLE_ROW_SIZE], double offset)
{
	int i;
	double v;

	for (i = 0; i < RESAMPLE_N; i++)
	{
// Lanczos
#if 1
// left side (for previous samples)
		Resample_GetValueOnLanczosKernel (&v, i + offset);
		(*row)[RESAMPLE_N - 1 - i] = v;
// right side (for incoming samples)
		Resample_GetValueOnLanczosKernel (&v, offset - i - 1);
		(*row)[RESAMPLE_N + i] = v;
// truncated Sinc
#else
// left side (for previous samples)
		Resample_GetValueOnSincKernel (&v, i + offset);
		(*row)[RESAMPLE_N - 1 - i] = v;
// right side (for incoming samples)
		Resample_GetValueOnSincKernel (&v, offset - i - 1);
		(*row)[RESAMPLE_N + i] = v;
#endif
	}
}

void Resample_GenerateTable ()
{
	int i;
	double(* row)[RESAMPLE_ROW_SIZE];
	double x;
#if RESAMPLE_DUMP_TABLE
	FILE* f;
	f = fopen ("aoqsf_resample_tab.txt", "wb");
#endif

// division 0.0/0.0 on intel processors will fail (gives -nan on GCC)
// *the first sample must be on exactly this position
	row = resample_table;
	memset (row, 0, sizeof(*row));
	(*row)[RESAMPLE_N - 1] = 1.0;

	for (i = 1; i < RESAMPLE_TABLE_SIZE; i++)
	{
		row = &resample_table[i];
		x = i;
		x /= RESAMPLE_TABLE_SIZE;
		Resample_GenerateRow (row, x);
	}
#if RESAMPLE_DUMP_TABLE
	if (f)
	{
		for (i = 0; i < RESAMPLE_TABLE_SIZE; i++)
		{
			int j;

			row = &resample_table[i];
			fprintf (f, "%04x :  ", i);
			for (j = 0; j < RESAMPLE_ROW_SIZE; j++)
			{
				fprintf (f, "%f, ", (*row)[j]);
			}
			fprintf (f, "\n");
		}
		fclose (f);
	}
#endif
}

void Qsound_GetOneSample (qsound_channel_t* pC, int* ix, int* val)
{
	QSOUND_SRC_SAMPLE* pST;
	int a;
	int i;

//	pC->value = 0;
	*val = 0;
	i = *ix;
// *upsample also needs something before the beginning; return 0 value for it
	if (i < 0)
// go increment the index
		goto finish;
//	pC->address = pC->beg + i;
	if (i >= pC->len)
// *this also is plausible; and don't place it here - it is a wrong place for it
//	if (i >= pC->len - 1)
	{
		if (! pC->loop)
		{
			/* Reached the end of a non-looped sample */
// exit; just don't report that a sample has been fetched; that should be sufficient
			return;
		}
		/* Reached the end, restart the loop */
		i = pC->ix_beg_loop;
	}
// Alex :  banks are of 16 bit; whole area of banks is 7 + 16 bit
	pST = qsound_sample_rom + pC->bank;
	a = pC->beg + i;
// Alex :  stay in the same bank (at least I suspect it has to)
	a &= 0x0000ffff;
//	pC->value = pST[a];
	*val = pST[a];
// we've got something this time
	pC->got_sample = 1;
finish:
	i++;
	*ix = i;
}

void Qsound_GetSample (qsound_channel_t* pC, int* ix)
{
	int val;

//	val = 0;
	Qsound_GetOneSample (pC, ix, &val);
	pC->value = val;
// *will double the volume, what is handled in 'VOL_DIMINISH_SHIFT' macro
#if STEREO_SAMPLES
	Qsound_GetOneSample (pC, ix, &val);
// I didn't find any music which would get any profit when not downmix the second channel, except that some samples after this mix are sounding more quiet (less bright)
// so I assume there are no (and I actually didn't find) samples, on which the interpolator would work badly, like because of some small phase shift it would
// (although I am slightly in doubts that it is possible at all :  downmixed do sound bad, while originals would sound good)
	pC->value += val;
//	pC->value <<= 1;
#endif
}

// *a bit duplicates the code above, except that here the movement is not only by 1.0, but by greater and lessser values also
//void Qsound_UpdatePosition (qsound_channel_t* pC)
void Qsound_IncrementPosition (qsound_channel_t* pC)
{
	int ix;

// extra test
//	assert(pC->ix_beg_loop == pC->len - pC->loop);
	ix = pC->ix;
// address is necessary the one which is on the old position :/
// at least this is what is in the original routine of DRIVER1
// although I cannot promise that; below is one more commented such line of code for the new one
	pC->address = pC->beg + ix;
	pC->offset += pC->pitch;
// increment ix if necessary
#if STEREO_SAMPLES
	ix += pC->offset >> 16 << 1;
#else
	ix += pC->offset >> 16;
#endif
// leave only the fractional part
	pC->offset &= 0xffff;
	if (ix >= pC->len)
	{
		if (! pC->loop)
		{
			/* Reached the end of a non-looped sample */
			pC->key = 0;
// put the outlimited/outlasted value, because otherwise it possibly will try to fetch samples again
			goto finish;
		}
		/* Reached the end, restart the loop */
		assert(pC->len > pC->ix_beg_loop);
		ix -= pC->len;
		ix += pC->ix_beg_loop;
// but this actually is expectable
		assert(ix < pC->len);
	}
finish:
	pC->ix = ix;
// put also 'address', for the case if the value will be requested (not actually supported)
// hmmm, 'address' really is being mysteriously reused somehow, and enough often
//	pC->address = pC->beg + ix;
}

void Qsound_UpdateChannel_Downsample (qsound_channel_t* pC, double* sample)
{
	int period;
	__int64 v;
	int v1;
	int ix;
	int offset;

	*sample = 0;

// take the integer amount
	period = pC->pitch;
// other is not for this subr
	assert(period > TIME_ONE);
// also something wrong would be
	assert(pC->offset < TIME_ONE);
	v = 0;
// test
//	goto finish;
// *note, if take this position as is, it will give incorrect phase, if there will be some transition between downsample and upsample, like vibrato or so
	ix = pC->ix;
	offset = pC->offset;

// transition between downsample and upsample, like vibrato or so, would happen on a wrong phase;
// lets move us to the correct offset; subtract half of period
#if 1
	{
	__int64 pos;
// create complete position
// (sure, this could be a variable in the struct instead, but it is used only here, and here are only three steps to recreate it :  allocate var, put integer part, put fractional part)
#if STEREO_SAMPLES
// *ix will step by two
	pos = ix >> 1 << 16;
#else
	pos = ix << 16;
#endif
	pos += offset;

// subtract half of period
	pos -= period / 2;

// decompose back
	offset = pos & 0x0000ffff;
#if STEREO_SAMPLES
// keep odd position, if ix is on it
	ix = (pos >> 16 << 1) | (ix & 1);
#else
	ix = pos >> 16;
#endif
	}
#endif

// the correct phase gives value of 'sample[0]' for the first sample, thus begins from '0.5 - period / 2' and ends on '0.5 + period / 2'
// if there is a remainder part from the previous sample
	if (offset)
// the old sample was partially used; now have to use its remainder
	{
		int chunk;
		Qsound_GetSample (pC, &ix);
// don't !  the phase has been displaced back
//		if (! pC->got_sample)
//			goto finish;
		chunk = TIME_ONE - offset;
		v1 = pC->value;
		v1 *= chunk;
		v = v1 >> 16;
		period -= chunk;
	}
// add complete samples
	for (; period >= TIME_ONE; period -= TIME_ONE)
	{
		Qsound_GetSample (pC, &ix);
		v += pC->value;
	}
// if there is a last partial sample
	if (period)
	{
		Qsound_GetSample (pC, &ix);
		v1 = pC->value;
		v1 *= period;
		v1 >>= 16;
		v += v1;
	}

//finish:
// divide by amount of samples
	v <<= 16;
	*sample = (double)v / pC->pitch;
}

void Buffer_Fill (resample_buf_t* row_buf, qsound_channel_t* pC, int ix)
{
	int i;

#if STEREO_SAMPLES
	ix -= (RESAMPLE_N - 1) * 2;
// wrong
//	ix -= RESAMPLE_N * 2 - 1;
#else
	ix -= RESAMPLE_N - 1;
#endif
	for (i = 0; i < RESAMPLE_ROW_SIZE; i++)
	{
		Qsound_GetSample (pC, &ix);
		row_buf->samples[i] = pC->value;
	}
}

void Buffer_Check (resample_buf_t* row_buf, qsound_channel_t* pC)
{
	if (row_buf->pos == pC->ix)
		return;
	Buffer_Fill (row_buf, pC, pC->ix);
	row_buf->pos = pC->ix;
// yes, being here it makes some assymmetry, but I had to move it here
	if (! pC->got_sample)
		pC->key = 0;
}

void Qsound_UpdateChannel_Upsample (resample_buf_t* row_buf, qsound_channel_t* pC, double* sample)
{
// update values in the buffer, if there aren't the current
	Buffer_Check (row_buf, pC);

// test of no resample
//	*sample = row_buf->samples[RESAMPLE_N - 1];
// test
//	*sample = row_buf->samples[0];
//	*sample = row_buf->samples[RESAMPLE_ROW_SIZE - 1];
// test of linear resample
#if 0
	{
	int v;
	v = row_buf->samples[RESAMPLE_N - 1] * (TIME_ONE - pC->offset);
	v += row_buf->samples[RESAMPLE_N] * pC->offset;
	*sample = v >> TIME_BITS;
	}
#endif
// test of bendings (need anything what is better than linear)
#if 0
	{
	double v;
	double bending;
	double slope_0;
	double slope_1;

	SA(RESAMPLE_N >= 2);

	slope_0 = row_buf->samples[RESAMPLE_N - 1] - row_buf->samples[RESAMPLE_N - 2];
	slope_1 = row_buf->samples[RESAMPLE_N + 0] - row_buf->samples[RESAMPLE_N - 1];
	bending = slope_0 - slope_1;
	v = bending;
	slope_0 = row_buf->samples[RESAMPLE_N + 0] - row_buf->samples[RESAMPLE_N - 1];
	slope_1 = row_buf->samples[RESAMPLE_N + 1] - row_buf->samples[RESAMPLE_N + 0];
	bending = slope_0 - slope_1;
	v += bending;

// works on half of period; maximum is where 'pC->offset' is 'TIME_ONE / 2'
	v *= sin (PI * pC->offset / TIME_ONE);
// (1.414 + 1.414) bending turns into (1.0 - 0.707) addon (it is for 4-point sine, where points are 0.707, 0.707, -0.707, -0.707)
	v *= (1.0 - 0.707) / (1.414 * 2);
// extra tests
//	v *= 0.0;
	*sample += v;
	}
#endif
// it is time of Lanczos resample
#if 1
	{
	int pos_in_table;
//	double(* row)[RESAMPLE_ROW_SIZE];
	double* row;
	double v;
	int i;

	assert(pC->offset >= 0);
	assert(pC->offset < TIME_ONE);
	pos_in_table = pC->offset >> (TIME_BITS - RESAMPLE_TABLE_BITS);
//	__asm int 3

	assert(pos_in_table < RESAMPLE_TABLE_SIZE);
//// test :  this should make no effect
//	pos_in_table = RESAMPLE_TABLE_SIZE - pos_in_table - 1;
//	row = &resample_table[pos_in_table];
	row = resample_table[pos_in_table];
	v = 0;
	for (i = 0; i < RESAMPLE_ROW_SIZE; i++)
	{
		v += row_buf->samples[i] * row[i];
//		v += row_buf->samples[i] * (*row)[i];
	}
	*sample = v;
	}
#endif
// test of no resample
//	*sample = row_buf->samples[RESAMPLE_N - 1];
}

// Alex :  multiplication ?  maybe meant addition ?
// '8' is the amount of bits in 'lvol'/'rvol' must be
//#define VOL_SHIFT (8 * LENGTH_DIV)
// like this
// although no, 'LENGTH_DIV' must not be used here
//#define VOL_SHIFT (8 + LENGTH_DIV - 1)
#define VOL_SHIFT (8)
// Alex :  hmmm, too quiet
//#define VOL_DIMINISH_SHIFT (6 + STEREO_SAMPLES)
#define VOL_DIMINISH_SHIFT (5 + STEREO_SAMPLES)

void Qsound_UpdateChannel (int ch, QSOUND_SAMPLE* output, int length)
{
	int i;
	int vol;
	double sample;
	resample_buf_t* row_buf;
	qsound_channel_t* pC;

	row_buf = &resample_buf[ch];
	pC = &qsound_channel[ch];
	if (! pC->key)
		return;
#if MEMORY_CHECK
	assert(row_buf->debug_0 == 0x12345678);
	assert(row_buf->debug_1 == 0x87654321);
#endif

// lvol and rvol were simplified into just ariphmetic mean
// (I am not sure if they are used for the modulation of sounds, so had to leave them)
//	vol = pC->vol >> VOL_SHIFT;
	vol = pC->vol * (pC->lvol + pC->rvol) >> (VOL_SHIFT + 1);

// test between upsample and downsample
#define TEST_PITCH 0
//#define TEST_PITCH 1
#if TEST_PITCH
	{
	int p;
	p = pC->pitch;
	pC->pitch <<= 2;
//	pC->pitch <<= 1;
//	pC->pitch <<= 1;
#endif

	pC->got_sample = 0;
	for (i = length - 1; i >= 0; i--)
	{
		if (pC->pitch > TIME_ONE)
		{
			Qsound_UpdateChannel_Downsample (pC, &sample);
			if (! pC->got_sample)
				pC->key = 0;
		}
		else
		{
			Qsound_UpdateChannel_Upsample (row_buf, pC, &sample);
		}
		Qsound_IncrementPosition (pC);
		*output += sample * vol / (1 << VOL_DIMINISH_SHIFT);
		output++;
// test
//		pC->address = pC->beg + pC->ix;
//		pC->address = pC->beg + pC->ix + 1;
	}
// if there is no input; it happens when going out from a nonlooped sample
// ergh
//	if (! pC->got_sample)
//		pC->key = 0;

#if TEST_PITCH
	pC->pitch = p;
	}
#endif
}

// Alex :  'num' isn't used in any version
void qsound_update (int num, INT16** buffer, int length)
{
	int i;
	QSOUND_SAMPLE* datap[2];

	datap[0] = buffer[0];
	datap[1] = buffer[1];
	memset (datap[0], 0x00, length * sizeof(QSOUND_SAMPLE));
	memset (datap[1], 0x00, length * sizeof(QSOUND_SAMPLE));

// lame fast solution
	if (resample_table[0][RESAMPLE_N - 1] != 1.0)
	{
		Resample_GenerateTable ();
		assert(resample_table[0][RESAMPLE_N - 1] == 1.0);
	}

	for (i = 0; i < QSOUND_CHANNELS; i++)
	{
		Qsound_UpdateChannel (i, datap[0], length);
		memcpy (datap[1], datap[0], length * sizeof(QSOUND_SAMPLE));
	}

#if LOG_WAVE
	fwrite(datap[0], length*sizeof(QSOUND_SAMPLE), 1, fpRawDataL);
	fwrite(datap[1], length*sizeof(QSOUND_SAMPLE), 1, fpRawDataR);
#endif
}

// ----------------------------------------------------------------

#endif

#else

/* ----------------------------------------------------------------
		QSound Sample Mixer (Slow)
		Miguel Angel Horna mahorna@teleline.es

 ------------------------------------------------------------------ */


void calcula_mix(int channel)
{
	int factl,factr;
	qsound_channel_t *pC=&qsound_channel[channel];
	int vol=pC->vol>>5;
	int pan=((pC->pan&0xFF)-0x10)<<3;
	pC->mixl=vol;
	pC->mixr=vol;
	factr=pan;
	factl=255-factr;
	pC->mixl=(pC->mixl * factl)>>8;
	pC->mixr=(pC->mixr * factr)>>8;
#if QSOUND_8BIT_SAMPLES
	pC->mixl<<=8;
	pC->mixr<<=8;
#endif
}

void qsound_update(int num,void **buffer,int length)
{
	int i,j;
	QSOUND_SAMPLE *bufL,*bufR, sample;
	qsound_channel_t *pC=qsound_channel;

	memset( buffer[0], 0x00, length * sizeof(QSOUND_SAMPLE) );
	memset( buffer[1], 0x00, length * sizeof(QSOUND_SAMPLE) );

	for(j=0;j<QSOUND_CHANNELS;++j)
	{
		bufL=(QSOUND_SAMPLE *) buffer[0];
		bufR=(QSOUND_SAMPLE *) buffer[1];
		if(pC->key)
		{
			for(i=0;i<length;++i)
			{
				int pos=pC->cursor>>8;
				if(pos!=pC->lpos)	/*next sample*/
				{
					sample=pC->buffer[pos];
					pC->lastsaml=(sample*pC->mixl)>>8;
					pC->lastsamr=(sample*pC->mixr)>>8;
					pC->lpos=pos;
				}

				(*bufL++)+=pC->lastsaml;
				(*bufR++)+=pC->lastsamr;

				pC->cursor+=pC->factor;
				if(pC->loop && (pC->cursor>>8) > pC->end)
				{
					pC->cursor=(pC->end-pC->loop)<<8;
				}
				else if((pC->cursor>>8) > pC->end)
					pC->key=0;
			}
		}
		pC++;
	}
}
#endif


/**************** end of file ****************/
