/**********************************************************************

  resample.h

  Real-time library interface by Dominic Mazzoni

  Based on resample-1.7:
    http://www-ccrma.stanford.edu/~jos/resample/

  License: LGPL - see the file LICENSE.txt for more information

**********************************************************************/

#ifndef LIBRESAMPLE_INCLUDED
#define LIBRESAMPLE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

void * __cdecl resample_open(int      highQuality,
                    double   minFactor,
                    double   maxFactor);

void * __cdecl resample_dup(const void *handle);

int __cdecl resample_get_filter_width(const void *handle);

int __cdecl resample_process(void   *handle,
                     double  factor,
                     float  *inBuffer,
                     int     inBufferLen,
                     int     lastFlag,
                     int    *inBufferUsed,
                     float  *outBuffer,
                     int     outBufferLen);

void __cdecl resample_close(void *handle);

#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#endif /* LIBRESAMPLE_INCLUDED */
