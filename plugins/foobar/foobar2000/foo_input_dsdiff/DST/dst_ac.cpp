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

Copyright © 2004.

Source file: dst_ac.c (Arithmetic Coding part of the DST Coding)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

/* in encoder: size of C register is (ABITS+1) bits;
   in decoder: size of C register is ABITS bits;
   size of A register is ABITS bits */

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>

#include "dst_ac.h"
#include "conststr.h"
#include "types.h"


/*============================================================================*/
/*       CONSTANTS                                                            */
/*============================================================================*/

#define PBITS   AC_BITS      /* number of bits for Probabilities             */
#define NBITS   4            /* number of overhead bits: must be at least 2! */
                             /* maximum "variable shift length" is (NBITS-1) */
#define PSUM    (1 << (PBITS))
#define ABITS  (PBITS + NBITS)               /* must be at least PBITS+2     */
#define MB      0                            /* if (MB) print max buffer use */
#define ONE     (1 << ABITS)
#define HALF    (1 << (ABITS-1))

/***************************************************************************/
/*                                                                         */
/* name     : DST_ACDecodeBit                                              */
/*                                                                         */
/* function : Arithmetic decode one bit.                                   */
/*                                                                         */
/* pre      : p       : probability for next bit being a "one"             */
/*            cb[]    : filled with arithmetic code bit(s)                 */
/*            fs      : Current length of the arithm. code                 */
/*            Flush   : 0 = Normal operation,                              */
/*                      1 = flush remaider of the decoder                  */
/*                                                                         */
/* post     : *b      : output bit of arithmetic decoder                   */
/*            *MonC   : status of C-register (optionally)                  */
/*            *MonA   : status of A-register (optionally)                  */
/*                                                                         */
/***************************************************************************/

void DST_ACDecodeBit(acdata * ACData, unsigned char *b, int p, unsigned char *cb,
                     int fs, int Flush)
{
  unsigned int         ap;
  unsigned int         h;

  if (ACData->Init == 1)
  {
    ACData->Init = 0;
    ACData->A    = ONE - 1;
    ACData->C    = 0;
    for (ACData->cbptr = 1; ACData->cbptr <= ABITS; ACData->cbptr++)
    {
      ACData->C <<= 1;
      if (ACData->cbptr < fs)
      {
        ACData->C |= cb[ACData->cbptr];
      }
    }
  }
  
  if (Flush == 0)
  {
    /* approximate (A * p) with "partial rounding". */
    ap = ((ACData->A >> PBITS) | ((ACData->A >> (PBITS - 1)) & 1)) * p;
    
    h = ACData->A - ap;
    if (ACData->C >= h)
    {
      *b = 1;
      ACData->C -= h;
      ACData->A  = ap;
    }
    else
    {
      *b = 0;
      ACData->A  = h;
    }
    while (ACData->A < HALF)
    {
      ACData->A <<= 1;
      
      /* Use new flushing technique; insert zero in LSB of C if reading past
         the end of the arithmetic code */
      ACData->C <<= 1;
      if (ACData->cbptr < fs)
      {
        ACData->C |= cb[ACData->cbptr];
      }
      ACData->cbptr++;
    }
  }
  else
  {
    ACData->Init = 1;
    *b   = 0;
    if (ACData->cbptr < fs - 7)
    {
      *b = 1;
    }
    else
    {
      while ((*b == 0) && (ACData->cbptr < fs))
      {
        if (cb[ACData->cbptr] != 0)
        {
          *b = 1;
        }
        ACData->cbptr++;
      }
    }
  }
}

#undef PBITS
#undef NBITS
#undef PSUM
#undef ABITS
#undef HALF
#undef ONE
#undef MB

/***************************************************************************/
/*                                                                         */
/* name     : DST_ACGetPtableIndex                                         */
/*                                                                         */
/* function : Determine the Ptable index belonging to the current value    */
/*            of PredicVal.                                                */
/*                                                                         */
/* pre      : PredicVal and PtableLen                                      */
/*                                                                         */
/* post     : Returns the index of the Ptable belonging to the PredicVal.  */
/*                                                                         */
/***************************************************************************/

int DST_ACGetPtableIndex(long PredicVal, int PtableLen)
{
  int  j;
  
  j = labs(PredicVal) >> AC_QSTEP;
  if (j >= PtableLen)
  {
    j = PtableLen - 1;
  }
  
  return j;
}
