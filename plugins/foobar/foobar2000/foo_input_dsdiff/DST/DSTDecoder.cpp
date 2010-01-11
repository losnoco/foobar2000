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

Source file: DSTDecoder.c (Initialize decoder environment)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include "dst_fram.h"
#include "dst_init.h"
#include "DSTDecoder.h"


/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/

//static ebunch           D;

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/

/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name                   : Init
 * Description            : Initialises the encoder component.
 * Input                  : NrOfChannels: 2,5,6
 * Output                 :
 * Pre-condition          :
 * Post-condition         :
 * Returns:               : True if Init ok, else false
 * Global parameter usage :
 * 
 *****************************************************************************/
BOOL Init(ebunch * D, int NrChannels, int Fs44)
{
  BOOL  result = TRUE;

  D->FrameHdr.NrOfChannels = (char)NrChannels;

  if (result==TRUE)
  {
    D->FrameHdr.FrameNr = 0;
    D->StrFilter.TableType = FILTER;
    D->StrPtable.TableType = PTABLE;
  }

  /*  64FS =>  4704 */
  /* 128FS =>  9408 */
  /* 256FS => 18816 */
  D->FrameHdr.MaxFrameLen = (588 * Fs44 / 8); 

  D->FrameHdr.ByteStreamLen   = D->FrameHdr.MaxFrameLen   * D->FrameHdr.NrOfChannels;
  D->FrameHdr.BitStreamLen    = D->FrameHdr.ByteStreamLen * RESOL;
  D->FrameHdr.NrOfBitsPerCh   = D->FrameHdr.MaxFrameLen   * RESOL;

  D->FrameHdr.MaxNrOfFilters  = 2 * D->FrameHdr.NrOfChannels;
  D->FrameHdr.MaxNrOfPtables  = 2 * D->FrameHdr.NrOfChannels;

  DST_InitDecoder(D);

  return result;
}


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name                   : Close
 * Description            : 
 * Input                  : 
 * Output                 :
 * Pre-condition          :
 * Post-condition         :
 * Returns:               :
 * Global parameter usage :
 * 
 *****************************************************************************/
BOOL Close( ebunch * D )
{
  BOOL  result = TRUE;

  DST_CloseDecoder(D);

  return (result);
}

 
 /*************************GLOBAL FUNCTION**************************************
 * 
 * Name                   : Decode
 * Description            : 
 * Input                  : 
 * Output                 :
 * Pre-condition          :
 * Post-condition         :
 * Returns:               :
 * Global parameter usage :
 * 
 *****************************************************************************/
BOOL Decode(  ebunch* D,
              BYTE*   DSTFrame, 
              BYTE*   DSDMuxedChannelData, 
              int     FrameCnt,
              ULONG*  FrameSize )
{
  BOOL  result = TRUE;

  DST_FramDSTDecode(DSTFrame,
                    DSDMuxedChannelData,
                    *FrameSize,
                    FrameCnt,   
                    D);

  return (result);
}


