/***********************************************************************
MPEG-4 Audio RM Module
Lossless coding of 1-bit oversampled audio - DST (Direct Stream Transfer)

This software was originally developed by:

* Aad Rijnberg 
  Philips Digital Systems Laboratories Eindhoven 
  <aad.rijnberg@philips.com>

* Fons Bruekers
  Philips Research Laboratories Eindhoven
  <fons.bruekers@philips.com>
   
* Eric Knapen
  Philips Digital Systems Laboratories Eindhoven
  <h.w.m.knapen@philips.com> 

And edited by:

* Richard Theelen
  Philips Digital Systems Laboratories Eindhoven
  <r.h.m.theelen@philips.com>

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright  2004.

Source file: DSTData.h (DSTData object)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


#if !defined(__DSTDATA_H_INCLUDED)
#define __DSTDATA_H_INCLUDED

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include "types.h"

/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/

int GetDSTDataPointer      (dstdata * data, BYTE** pBuffer);
int ResetReadingIndex      (dstdata * data);
int ReadNextBitFromBuffer  (dstdata * data, BYTE*  pBit);
int ReadNextNBitsFromBuffer(dstdata * data, LONG*  pBits, LONG NrBits);
int ReadNextByteFromBuffer (dstdata * data, BYTE*  pByte);

int FillBuffer(dstdata * data, BYTE* pBuf, LONG Size);

int FIO_BitGetChrUnsigned(dstdata * data, int Len, unsigned char *x);
int FIO_BitGetIntUnsigned(dstdata * data, int Len, int *x);
int FIO_BitGetIntSigned(dstdata * data, int Len, int *x);
int get_in_bitcount(dstdata * data);

int CreateBuffer(dstdata * data, LONG Size);
int DeleteBuffer(dstdata * data);


#endif /* !defined(__DSTDATA_H_INCLUDED) */

