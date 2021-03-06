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

Copyright � 2004.

Source file: types.h (Type definitions)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


#ifndef __TYPES_H_INCLUDED
#define __TYPES_H_INCLUDED


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdio.h>
#include "conststr.h"

/*============================================================================*/
/*       TYPEDEFINITIONS                                                      */
/*============================================================================*/

#define TRUE        (1==1)
#define FALSE       (!TRUE)

typedef long            LONG;
typedef unsigned long   ULONG;
typedef unsigned short  USHORT;
typedef int             BOOL;
typedef unsigned char   BYTE;

enum TTable    { FILTER, PTABLE };

typedef struct CoderOptionsInfo 
{
  int           NrOfChannels;     /* Number of channels in the recording   */
  int           Fsample44;        /* Sample frequency input file           */
  
  ULONG         NrOfFramesInFile; /* Number of frames in the recording     */
  char          OutFileName[512]; /* Filename for decoder DSD output file  */
  char          InFileName[512];  /* Filename for decoder DST instream     */
} CoderOptions;

typedef struct 
{
  int           Resolution;       /* Resolution for segments                 */
  int           **SegmentLen;     /* SegmentLen[ChNr][SegmentNr]             */
  int           *NrOfSegments;    /* NrOfSegments[ChNr]                      */
  int           **Table4Segment;  /* Table4Segment[ChNr][SegmentNr]          */
} Segment;

typedef struct FrameHeaderInfo 
{
  int           FrameNr;          /* Nr of frame that is currently processed    */
  char          NrOfChannels;     /* Number of channels in the recording        */
  int           NrOfFilters;      /* Number of filters used for this frame      */
  int           NrOfPtables;      /* Number of Ptables used for this frame      */
  int           Fsample44;        /* Sample frequency 64, 128, 256              */
  int           *PredOrder;       /* Prediction order used for this frame       */
  int           *PtableLen;       /* Nr of Ptable entries used for this frame   */
  float         **FCoefA;         /* Floating point FIR coefficients            */
  int           **ICoefA;         /* Integer coefs for actual coding            */
  int           DSTCoded;          /* 1=DST coded is put in DST stream,          */
                                  /* 0=DSD is put in DST stream                 */
  long          CalcNrOfBytes;    /* Contains number of bytes of the complete   */
  long          CalcNrOfBits;     /* Contains number of bits of the complete    */
                                  /* channel stream after arithmetic encoding   */
                                  /* (also containing bytestuff-,               */
                                  /* ICoefA-bits, etc.)                         */
  int           *HalfProb;        /* Defines per channel which probability is   */
                                  /* applied for the first PredOrder[] bits of  */
                                  /* a frame (0 = use Ptable entry, 1 = 128)    */
  int           *NrOfHalfBits;    /* Defines per channel how many bits at the   */
                                  /* start of each frame are optionally coded   */
                                  /* with p=0.5                                 */
  Segment       FSeg;             /* Contains segmentation data for filters     */
  int           **Filter4Bit;     /* Filter4Bit[ChNr][BitNr]                    */
  Segment       PSeg;             /* Contains segmentation data for Ptables     */
  int           **Ptable4Bit;     /* Ptable4Bit[ChNr][BitNr]                    */
  int           PSameSegAsF;      /* 1 if segmentation is equal for F and P     */
  int           PSameMapAsF;      /* 1 if mapping is equal for F and P          */
  int           FSameSegAllCh;    /* 1 if all channels have same Filtersegm.    */
  int           FSameMapAllCh;    /* 1 if all channels have same Filtermap      */
  int           PSameSegAllCh;    /* 1 if all channels have same Ptablesegm.    */
  int           PSameMapAllCh;    /* 1 if all channels have same Ptablemap      */
  int           SegAndMapBits;    /* Number of bits in the stream for Seg&Map   */

  int           MaxNrOfFilters;   /* Max. nr. of filters allowed per frame      */
  int           MaxNrOfPtables;   /* Max. nr. of Ptables allowed per frame      */
  long          MaxFrameLen;      /* Max frame length of this file              */
  long          ByteStreamLen;    /* MaxFrameLen * NrOfChannels                 */
  long          BitStreamLen;     /* ByteStreamLen * RESOL                      */
  long          NrOfBitsPerCh;    /* MaxFrameLen * RESOL                        */

} FrameHeader;


typedef struct CodedTablesInfo 
{
  int           ***C;             /* Coded_Coef[Fir/PtabNr][Method][CoefNr]     */
  int           *CPredOrder;      /* Code_PredOrder[Method]                     */
  int           **CPredCoef;      /* Code_PredCoef[Method][CoefNr]              */
  int           *Coded;            /* DST encode coefs/entries of Fir/PtabNr     */
  int           *BestMethod;      /* BestMethod[Fir/PtabNr]                     */
  int           **m;              /* m[Fir/PtabNr][Method]                      */
  int           **L1;             /* L1_Norm[Fir/PtabNr][Method]                */
  int           **Data;           /* Fir/PtabData[Fir/PtabNr][Index]            */
  int           *DataLen;         /* Fir/PtabDataLength[Fir/PtabNr]             */
  int           StreamBits;       /* nr of bits all filters use in the stream   */
  int           TableType;        /* FILTER or PTABLE: indicates contents       */
} CodedTable;


typedef struct DSTXBITSData 
{
  int            PBit;
  unsigned char  Bit;
  unsigned char  Data[MAX_DSTXBITS_SIZE];
  int            DataLen;
} DSTXBITS_STR;


typedef struct FirPtrData
{
  int            *Pnt;
  int            **Status;
} FirPtr;


typedef struct dstdatastruct
{
	BYTE*   m_pDSTdata;
	LONG    m_TotalBytes;
	LONG    m_ByteCounter;
	LONG    m_BitCounter;
	int     m_BitPosition;
	//long    m_mask[32];
	BYTE    m_DataByte;
} dstdata;


typedef struct acdatastruct
{
  unsigned int  Init;
  unsigned int  C;
  unsigned int  A;
  int           cbptr;
} acdata;

typedef struct ebunchdata 
{
  FirPtr         FirPtrs;        /* Contains pointers to the two arrays used    */

  CodedTable     StrFilter;      /* Contains FIR-coef. compression data         */
  CodedTable     StrPtable;      /* Contains Ptable-entry compression data      */
  FrameHeader    FrameHdr;       /* Contains frame based header information     */
  DSTXBITS_STR   DstXbits;       /* Contains DST_X_Bits                         */
                                 /* input stream.                               */
  signed char    **BitStream11;  /* Contains the bitstream of a complete        */
                                 /* frame. This array contains "bits" with      */
                                 /* value -1 or 1                               */
  unsigned char  **BitResidual;  /* Contains the residual signal to be          */
                                 /* applied to the arithmetic encoder           */
  long           **PredicVal;    /* Contains the output value of the FIR        */
                                 /* filter for each bit of a complete frame     */
  int            **P_one;        /* Probability table for arithmetic coder      */

  unsigned char  *AData;         /* Contains the arithmetic coded bit stream    */
                                 /* of a complete frame                         */
  int            ADataLen;       /* Number of code bits contained in AData[]    */

  dstdata        DSTData;

  acdata         ACData;
} ebunch;


#endif  /* __TYPES_H_INCLUDED */

