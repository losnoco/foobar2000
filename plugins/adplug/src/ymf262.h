#ifndef YMF262_H
#define YMF262_H


#define logerror(A, B, C)
#define BUILD_YMF262 1

#define INLINE static

/* select number of output bits: 8 or 16 */
#define OPL3_SAMPLE_BITS 16

/* compiler dependence */
#ifndef OSD_CPU_H
#define OSD_CPU_H
typedef unsigned char   UINT8;   /* unsigned  8bit */
typedef unsigned short  UINT16;  /* unsigned 16bit */
typedef unsigned int    UINT32;  /* unsigned 32bit */
typedef signed char     INT8;    /* signed  8bit   */
typedef signed short    INT16;   /* signed 16bit   */
typedef signed int      INT32;   /* signed 32bit   */
#endif

#if (OPL3_SAMPLE_BITS==16)
typedef INT16 OPL3SAMPLE;
#endif
#if (OPL3_SAMPLE_BITS==8)
typedef INT8 OPL3SAMPLE;
#endif


typedef void (*OPL3_TIMERHANDLER)(void * param, int channel, double interval_Sec);
typedef void (*OPL3_IRQHANDLER)(void * param, int irq);
typedef void (*OPL3_UPDATEHANDLER)(void * param, int min_interval_us);



#if BUILD_YMF262

void * YMF262Init(int clock, int rate);
void YMF262Shutdown(void *);
void YMF262ResetChip(void *);
int  YMF262Write(void *, int a, int v);
unsigned char YMF262Read(void *, int a);
int  YMF262TimerOver(void *, int c);
void YMF262UpdateOne(void *, INT16 *buffer, INT16 *buffers_chan[], int length);

void YMF262SetTimerHandler(void *, OPL3_TIMERHANDLER TimerHandler, void * param, int channelOffset);
void YMF262SetIRQHandler(void *, OPL3_IRQHANDLER IRQHandler, void * param);
void YMF262SetUpdateHandler(void *, OPL3_UPDATEHANDLER UpdateHandler, void * param);

#endif


#endif /* YMF262_H */
