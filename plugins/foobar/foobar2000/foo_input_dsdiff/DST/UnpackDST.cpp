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

Source file: UnpackDST.c (Unpacking DST Frame Data)

Required libraries: <none>

Authors:
RT:  Richard Theelen, PDSL-labs Eindhoven <r.h.m.theelen@philips.com>

Changes:
08-Mar-2004 RT  Initial version

************************************************************************/


/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/

#include <stdlib.h>
#include "UnpackDST.h"
#include <exception>


/*============================================================================*/
/*       Forward declaration function prototypes                              */
/*============================================================================*/

void ReadDSDframe(dstdata       * DSTData,
				  long          MaxFrameLen, 
                  int           NrOfChannels, 
                  unsigned char * DSDFrame);

int RiceDecode(dstdata * DSTData, int m);
int Log2RoundUp(long x);

void ReadTableSegmentData(int     NrOfChannels, 
                          int     FrameLen,
                          int     MaxNrOfSegs, 
                          int     MinSegLen, 
                          Segment *S,
                          int     *SameSegAllCh);
void CopySegmentData(FrameHeader *FH);
void ReadSegmentData(FrameHeader *FH);
void ReadTableMappingData(int     NrOfChannels, 
                          int     MaxNrOfTables,
                          Segment *S, 
                          int     *NrOfTables, 
                          int     *SameMapAllCh);
void CopyMappingData(FrameHeader *FH);
void ReadMappingData(FrameHeader *FH);
void ReadFilterCoefSets(int NrOfChannels, FrameHeader *FH, CodedTable *CF);
void ReadProbabilityTables(FrameHeader *FH, CodedTable *CP, int **P_one);
void ReadArithmeticCodedData(int ADataLen, unsigned char *AData);



/***************************************************************************/
/*                                                                         */
/* name     : ReadDSDframe                                                 */
/*                                                                         */
/* function : Read DSD signal of this frame from the DST input file.       */
/*                                                                         */
/* pre      : a file must be opened by using getbits_init(),               */
/*            MaxFrameLen, NrOfChannels                                    */
/*                                                                         */
/* post     : BS11[][]                                                     */
/*                                                                         */
/* uses     : fio_bit.h                                                    */
/*                                                                         */
/***************************************************************************/

void ReadDSDframe(dstdata       *DSTData,
				  long          MaxFrameLen, 
                  int           NrOfChannels, 
                  unsigned char *DSDFrame)
{
  int             ByteNr;
  int             max = (MaxFrameLen*NrOfChannels);
  
  for (ByteNr = 0; ByteNr < max; ByteNr++) 
  {
    FIO_BitGetChrUnsigned(DSTData,8,&DSDFrame[ByteNr]);
  }
}

/***************************************************************************/
/*                                                                         */
/* name     : RiceDecode                                                   */
/*                                                                         */
/* function : Read a Rice code from the DST file                           */
/*                                                                         */
/* pre      : a file must be opened by using putbits_init(), m             */
/*                                                                         */
/* post     : Returns the Rice decoded number                              */
/*                                                                         */
/* uses     : fio_bit.h                                                    */
/*                                                                         */
/***************************************************************************/

int RiceDecode(dstdata * DSTData, int m)
{
  int LSBs;
  int Nr;
  int RLBit;
  int RunLength;
  int Sign;
  
  /* Retrieve run length code */
  RunLength = 0;
  do
  {
    FIO_BitGetIntUnsigned(DSTData, 1, &RLBit);
    if (RLBit == 0)
    {
      RunLength++;
    }
  } while (RLBit == 0);
  
  /* Retrieve least significant bits */
  FIO_BitGetIntUnsigned(DSTData, m, &LSBs);
  
  Nr = (RunLength << m) + LSBs;

  /* Retrieve optional sign bit */
  if (Nr != 0)
  {
    FIO_BitGetIntUnsigned(DSTData, 1, &Sign);
    if (Sign == 1)
    {
      Nr = -Nr;
    }
  }
  
  return Nr;
}

/***************************************************************************/
/*                                                                         */
/* name     : Log2RoundUp                                                  */
/*                                                                         */
/* function : Calculate the log2 of an integer and round the result up,    */
/*            by using integer arithmetic.                                 */
/*                                                                         */
/* pre      : x                                                            */
/*                                                                         */
/* post     : Returns the rounded up log2 of x.                            */
/*                                                                         */
/* uses     : None.                                                        */
/*                                                                         */
/***************************************************************************/

int Log2RoundUp(long x)
{
  int y = 0;
  
  while (x >= (1 << y))
  {
    y++;
  }
  
  return y;
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadTableSegmentData                                         */
/*                                                                         */
/* function : Read segmentation data for filters or Ptables.               */
/*                                                                         */
/* pre      : NrOfChannels, FrameLen, MaxNrOfSegs, MinSegLen               */
/*                                                                         */
/* post     : S->Resolution, S->SegmentLen[][], S->NrOfSegments[]          */
/*                                                                         */
/* uses     : types.h, fio_bit.h, stdio.h, stdlib.h                        */
/*                                                                         */
/***************************************************************************/

void ReadTableSegmentData(dstdata *DSTData,
						  int     NrOfChannels, 
                          int     FrameLen,
                          int     MaxNrOfSegs, 
                          int     MinSegLen, 
                          Segment *S,
                          int     *SameSegAllCh)
{
  int ChNr         = 0;
  int DefinedBits  = 0;
  int ResolRead    = 0;
  int SegNr        = 0;
  int MaxSegSize;
  int NrOfBits;
  int EndOfChannel;
  
  MaxSegSize = FrameLen - MinSegLen/8;
  
  FIO_BitGetIntUnsigned(DSTData, 1, SameSegAllCh);
  if (*SameSegAllCh == 1)
  {
    FIO_BitGetIntUnsigned(DSTData, 1, &EndOfChannel);
    while (EndOfChannel == 0)
    {
      if (SegNr >= MaxNrOfSegs)
      {
        /*fprintf(stderr, "ERROR: Too many segments for this channel!\n");
        exit(1);*/
        throw std::exception( "Too many segments for this channel!" );
      }
      if (ResolRead == 0)
      {
        NrOfBits = Log2RoundUp(FrameLen - MinSegLen/8);
        FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->Resolution);
        if ((S->Resolution == 0) || (S->Resolution > FrameLen - MinSegLen/8))
        {
          /*fprintf(stderr, "ERROR: Invalid segment resolution!\n");
          exit(1);*/
          throw std::exception( "Invalid segment resolution!" );
        }
        ResolRead = 1;
      }
      NrOfBits = Log2RoundUp(MaxSegSize / S->Resolution);
      FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->SegmentLen[0][SegNr]);
      
      if ((S->Resolution * 8 * S->SegmentLen[0][SegNr] < MinSegLen) ||
          (S->Resolution * 8 * S->SegmentLen[0][SegNr] >
           FrameLen * 8 - DefinedBits - MinSegLen))
      {
        /*fprintf(stderr, "ERROR: Invalid segment length!\n");
        exit(1);*/
        throw std::exception( "Invalid segment length!" );
      }
      DefinedBits += S->Resolution * 8 * S->SegmentLen[0][SegNr];
      MaxSegSize  -= S->Resolution * S->SegmentLen[0][SegNr];
      SegNr++;
      FIO_BitGetIntUnsigned(DSTData, 1, &EndOfChannel);
    }
    S->NrOfSegments[0]      = SegNr + 1;
    S->SegmentLen[0][SegNr] = 0;
    
    for (ChNr = 1; ChNr < NrOfChannels; ChNr++)
    {
      S->NrOfSegments[ChNr] = S->NrOfSegments[0];
      for (SegNr = 0; SegNr < S->NrOfSegments[0]; SegNr++)
      {
        S->SegmentLen[ChNr][SegNr] = S->SegmentLen[0][SegNr];
      }
    }
  }
  else
  {
    while (ChNr < NrOfChannels)
    {
      if (SegNr >= MaxNrOfSegs)
      {
        /*fprintf(stderr, "ERROR: Too many segments for this channel!\n");
        exit(1);*/
        throw std::exception( "Too many segments for this channel!" );
      }
      FIO_BitGetIntUnsigned(DSTData, 1, &EndOfChannel);
      if (EndOfChannel == 0)
      {
        if (ResolRead == 0)
        {
          NrOfBits = Log2RoundUp(FrameLen - MinSegLen/8);
          FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->Resolution);
          if ((S->Resolution == 0) || (S->Resolution > FrameLen - MinSegLen/8))
          {
            /*fprintf(stderr, "ERROR: Invalid segment resolution!\n");
            exit(1);*/
            throw std::exception( "Invalid segment resolution!" );
          }
          ResolRead = 1;
        }
        NrOfBits = Log2RoundUp(MaxSegSize / S->Resolution);
        FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->SegmentLen[ChNr][SegNr]);
        
        if ((S->Resolution * 8 * S->SegmentLen[ChNr][SegNr] < MinSegLen) ||
            (S->Resolution * 8 * S->SegmentLen[ChNr][SegNr] >
             FrameLen * 8 - DefinedBits - MinSegLen))
        {
          /*fprintf(stderr, "ERROR: Invalid segment length!\n");
          exit(1);*/
          throw std::exception( "Invalid segment length!" );
        }
        DefinedBits += S->Resolution * 8 * S->SegmentLen[ChNr][SegNr];
        MaxSegSize  -= S->Resolution * S->SegmentLen[ChNr][SegNr];
        SegNr++;
      }
      else
      {
        S->NrOfSegments[ChNr]      = SegNr + 1;
        S->SegmentLen[ChNr][SegNr] = 0;
        SegNr                      = 0;
        DefinedBits                = 0;
        MaxSegSize                 = FrameLen - MinSegLen/8;
        ChNr++;
      }
    }
  }
  if (ResolRead == 0)
  {
    S->Resolution = 1;
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : CopySegmentData                                              */
/*                                                                         */
/* function : Read segmentation data for filters and Ptables.              */
/*                                                                         */
/* pre      : FH->NrOfChannels, FH->FSeg.Resolution,                       */
/*            FH->FSeg.NrOfSegments[], FH->FSeg.SegmentLen[][]             */
/*                                                                         */
/* post     : FH-> : PSeg : .Resolution, .NrOfSegments[], .SegmentLen[][], */
/*                   PSameSegAllCh                                         */
/*                                                                         */
/* uses     : types.h, conststr.h                                          */
/*                                                                         */
/***************************************************************************/

void CopySegmentData(FrameHeader *FH)
{
  int ChNr;
  int SegNr;
  
  FH->PSeg.Resolution = FH->FSeg.Resolution;
  FH->PSameSegAllCh   = 1;
  for (ChNr = 0; ChNr < FH->NrOfChannels; ChNr++)
  {
    FH->PSeg.NrOfSegments[ChNr] = FH->FSeg.NrOfSegments[ChNr];
    if (FH->PSeg.NrOfSegments[ChNr] > MAXNROF_PSEGS)
    {
      /*fprintf(stderr, "ERROR: Too many segments!\n");
      exit(1);*/
      throw std::exception( "Too many segments!" );
    }
    if (FH->PSeg.NrOfSegments[ChNr] != FH->PSeg.NrOfSegments[0])
    {
      FH->PSameSegAllCh = 0;
    }
    for (SegNr = 0; SegNr < FH->FSeg.NrOfSegments[ChNr]; SegNr++)
    {
      FH->PSeg.SegmentLen[ChNr][SegNr] = FH->FSeg.SegmentLen[ChNr][SegNr];
      if ((FH->PSeg.SegmentLen[ChNr][SegNr] != 0) &&
        (FH->PSeg.Resolution*8*FH->PSeg.SegmentLen[ChNr][SegNr]<MIN_PSEG_LEN))
      {
        /*fprintf(stderr, "ERROR: Invalid segment length!\n");
        exit(1);*/
        throw std::exception( "Invalid segment length!" );
      }
      if (FH->PSeg.SegmentLen[ChNr][SegNr] != FH->PSeg.SegmentLen[0][SegNr])
      {
        FH->PSameSegAllCh = 0;
      }
    }
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadSegmentData                                              */
/*                                                                         */
/* function : Read segmentation data for filters and Ptables.              */
/*                                                                         */
/* pre      : FH->NrOfChannels, CO->MaxFrameLen                            */
/*                                                                         */
/* post     : FH-> : FSeg : .Resolution, .SegmentLen[][], .NrOfSegments[], */
/*                   PSeg : .Resolution, .SegmentLen[][], .NrOfSegments[], */
/*                   PSameSegAsF, FSameSegAllCh, PSameSegAllCh             */
/*                                                                         */
/* uses     : types.h, conststr.h, fio_bit.h                               */
/*                                                                         */
/***************************************************************************/

void ReadSegmentData(dstdata * DSTData, FrameHeader *FH)
{
  FIO_BitGetIntUnsigned(DSTData, 1, &FH->PSameSegAsF);
  ReadTableSegmentData( DSTData,
                        FH->NrOfChannels, 
                        FH->MaxFrameLen, 
                        MAXNROF_FSEGS,
                        MIN_FSEG_LEN, 
                        &FH->FSeg, 
                        &FH->FSameSegAllCh);

  if (FH->PSameSegAsF == 1)
  {
    CopySegmentData(FH);
  }
  else
  {
    ReadTableSegmentData( DSTData,
                          FH->NrOfChannels, 
                          FH->MaxFrameLen,
                          MAXNROF_PSEGS, 
                          MIN_PSEG_LEN, 
                          &FH->PSeg, 
                          &FH->PSameSegAllCh);
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadTableMappingData                                         */
/*                                                                         */
/* function : Read mapping data for filters or Ptables.                    */
/*                                                                         */
/* pre      : NrOfChannels, MaxNrOfTables, S->NrOfSegments[]               */
/*                                                                         */
/* post     : S->Table4Segment[][], NrOfTables, SameMapAllCh               */
/*                                                                         */
/* uses     : types.h, fio_bit.h, stdio.h, stdlib.h                        */
/*                                                                         */
/***************************************************************************/

void ReadTableMappingData(dstdata *DSTData,
						  int     NrOfChannels, 
                          int     MaxNrOfTables,
                          Segment *S, 
                          int     *NrOfTables, 
                          int     *SameMapAllCh)
{
  int ChNr;
  int CountTables = 1;
  int NrOfBits    = 1;
  int SegNr;
  
  S->Table4Segment[0][0] = 0;

  FIO_BitGetIntUnsigned(DSTData, 1, SameMapAllCh);
  if (*SameMapAllCh == 1)
  {
    for (SegNr = 1; SegNr < S->NrOfSegments[0]; SegNr++)
    {
      NrOfBits = Log2RoundUp(CountTables);
      FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->Table4Segment[0][SegNr]);
      
      if (S->Table4Segment[0][SegNr] == CountTables)
      {
        CountTables++;
      }
      else if (S->Table4Segment[0][SegNr] > CountTables)
      {
        /*fprintf(stderr, "ERROR: Invalid table number for segment!\n");
        exit(1);*/
        throw std::exception( "Invalid table number for segment!" );
      }
    }
    for(ChNr = 1; ChNr < NrOfChannels; ChNr++)
    {
      if (S->NrOfSegments[ChNr] != S->NrOfSegments[0])
      {
        /*fprintf(stderr, "ERROR: Mapping can't be the same for all channels!\n");
        exit(1);*/
        throw std::exception( "Mapping can't be the same for all channels!" );
      }
      for (SegNr = 0; SegNr < S->NrOfSegments[0]; SegNr++)
      {
        S->Table4Segment[ChNr][SegNr] = S->Table4Segment[0][SegNr];
      }
    }
  }
  else
  {
    for(ChNr = 0; ChNr < NrOfChannels; ChNr++)
    {
      for (SegNr = 0; SegNr < S->NrOfSegments[ChNr]; SegNr++)
      {
        if ((ChNr != 0) || (SegNr != 0))
        {
          NrOfBits = Log2RoundUp(CountTables);
          FIO_BitGetIntUnsigned(DSTData, NrOfBits, &S->Table4Segment[ChNr][SegNr]);
          
          if (S->Table4Segment[ChNr][SegNr] == CountTables)
          {
            CountTables++;
          }
          else if (S->Table4Segment[ChNr][SegNr] > CountTables)
          {
            /*fprintf(stderr, "ERROR: Invalid table number for segment!\n");
            exit(1);*/
            throw std::exception( "Invalid table number for segment!" );
          }
        }
      }
    }
  }
  if (CountTables > MaxNrOfTables)
  {
    /*fprintf(stderr, "ERROR: Too many tables for this frame!\n");
    exit(1);*/
    throw std::exception( "Too many tables for this frame!" );
  }
  *NrOfTables = CountTables;
}


/***************************************************************************/
/*                                                                         */
/* name     : CopyMappingData                                              */
/*                                                                         */
/* function : Copy mapping data for Ptables from the filter mapping.       */
/*                                                                         */
/* pre      : CO-> : NrOfChannels, MaxNrOfPtables                          */
/*            FH-> : FSeg.NrOfSegments[], FSeg.Table4Segment[][],          */
/*                   NrOfFilters, PSeg.NrOfSegments[]                      */
/*                                                                         */
/* post     : FH-> : PSeg.Table4Segment[][], NrOfPtables, PSameMapAllCh    */
/*                                                                         */
/* uses     : types.h, stdio.h, stdlib.h, conststr.h                       */
/*                                                                         */
/***************************************************************************/

void CopyMappingData(FrameHeader *FH)
{
  int ChNr;
  int SegNr;
  
  FH->PSameMapAllCh = 1;
  for (ChNr = 0; ChNr < FH->NrOfChannels; ChNr++)
  {
    if (FH->PSeg.NrOfSegments[ChNr] == FH->FSeg.NrOfSegments[ChNr])
    {
      for (SegNr = 0; SegNr < FH->FSeg.NrOfSegments[ChNr]; SegNr++)
      {
        FH->PSeg.Table4Segment[ChNr][SegNr]=FH->FSeg.Table4Segment[ChNr][SegNr];
        if (FH->PSeg.Table4Segment[ChNr][SegNr] != 
            FH->PSeg.Table4Segment[0][SegNr])
        {
          FH->PSameMapAllCh = 0;
        }
      }
    }
    else
    {
      /*fprintf(stderr, "ERROR: Not same number of segments for filters and Ptables!\n");
      exit(1);*/
      throw std::exception( "Not same number of segments for filters and Ptables!" );
    }
  }
  FH->NrOfPtables = FH->NrOfFilters;
  if (FH->NrOfPtables > FH->MaxNrOfPtables)
  {
    /*fprintf(stderr, "ERROR: Too many tables for this frame!\n");
    exit(1);*/
    throw std::exception( "Too many tables for this frame!" );
  }
}

/***************************************************************************/
/*                                                                         */
/* name     : ReadMappingData                                              */
/*                                                                         */
/* function : Read mapping data (which channel uses which filter/Ptable).  */
/*                                                                         */
/* pre      : CO-> : NrOfChannels, MaxNrOfFilters, MaxNrOfPtables          */
/*            FH-> : FSeg.NrOfSegments[], PSeg.NrOfSegments[]              */
/*                                                                         */
/* post     : FH-> : FSeg.Table4Segment[][], .NrOfFilters,                 */
/*                   PSeg.Table4Segment[][], .NrOfPtables,                 */
/*                   PSameMapAsF, FSameMapAllCh, PSameMapAllCh, HalfProb[] */
/*                                                                         */
/* uses     : types.h, conststr.h, fio_bit.h                               */
/*                                                                         */
/***************************************************************************/

void ReadMappingData(dstdata * DSTData, FrameHeader *FH)
{
  int j;
  
  FIO_BitGetIntUnsigned(DSTData, 1, &FH->PSameMapAsF);
  ReadTableMappingData(DSTData, FH->NrOfChannels, FH->MaxNrOfFilters, &FH->FSeg,
    &FH->NrOfFilters, &FH->FSameMapAllCh);
  if (FH->PSameMapAsF == 1)
  {
    CopyMappingData(FH);
  }
  else
  {
    ReadTableMappingData(DSTData, FH->NrOfChannels, FH->MaxNrOfPtables, &FH->PSeg, 
      &FH->NrOfPtables, &FH->PSameMapAllCh);
  }
  
  for (j = 0; j < FH->NrOfChannels; j++)
  {
    FIO_BitGetIntUnsigned(DSTData, 1, &FH->HalfProb[j]);
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadFilterCoefSets                                           */
/*                                                                         */
/* function : Read all filter data from the DST file, which contains:      */
/*            - which channel uses which filter                            */
/*            - for each filter:                                           */
/*              ~ prediction order                                         */
/*              ~ all coefficients                                         */
/*                                                                         */
/* pre      : a file must be opened by using getbits_init(), NrOfChannels  */
/*            FH->NrOfFilters, CF->CPredOrder[], CF->CPredCoef[][],        */
/*            FH->FSeg.Table4Segment[][0]                                  */
/*                                                                         */
/* post     : FH->PredOrder[], FH->ICoefA[][], FH->NrOfHalfBits[],         */
/*            CF->Coded[], CF->BestMethod[], CF->m[][],                    */
/*                                                                         */
/* uses     : types.h, fio_bit.h, conststr.h, stdio.h, stdlib.h, dst_ac.h  */
/*                                                                         */
/***************************************************************************/

void ReadFilterCoefSets(dstdata     *DSTData,
						int          NrOfChannels,
                        FrameHeader *FH,
                        CodedTable  *CF)
{
  int c;
  int ChNr;
  int CoefNr;
  int FilterNr;
  int TapNr;
  int x;
  
  /* Read the filter parameters */
  for(FilterNr = 0; FilterNr < FH->NrOfFilters; FilterNr++)
  {
    FIO_BitGetIntUnsigned(DSTData, SIZE_CODEDPREDORDER, &FH->PredOrder[FilterNr]);
    FH->PredOrder[FilterNr]++;
    
    FIO_BitGetIntUnsigned(DSTData, 1, &CF->Coded[FilterNr]);
    if (CF->Coded[FilterNr] == 0)
    {
      CF->BestMethod[FilterNr] = -1;
      for(CoefNr = 0; CoefNr < FH->PredOrder[FilterNr]; CoefNr++)
      {
        FIO_BitGetIntSigned(DSTData, SIZE_PREDCOEF, &FH->ICoefA[FilterNr][CoefNr]);
      }
    }
    else
    {
      FIO_BitGetIntUnsigned(DSTData, SIZE_RICEMETHOD, &CF->BestMethod[FilterNr]);
      if (CF->CPredOrder[CF->BestMethod[FilterNr]] >= FH->PredOrder[FilterNr])
      {
        /*printf("ERROR: Invalid coefficient coding method!\n");
        exit(1);*/
        throw std::exception( "Invalid coefficient coding method!" );
      }
      
      for(CoefNr = 0; CoefNr < CF->CPredOrder[CF->BestMethod[FilterNr]];
          CoefNr++)
      {
        FIO_BitGetIntSigned(DSTData, SIZE_PREDCOEF, &FH->ICoefA[FilterNr][CoefNr]);
      }
      
      FIO_BitGetIntUnsigned(DSTData, SIZE_RICEM,
                            &CF->m[FilterNr][CF->BestMethod[FilterNr]]);
      
      for(CoefNr = CF->CPredOrder[CF->BestMethod[FilterNr]];
          CoefNr < FH->PredOrder[FilterNr]; CoefNr++)
      {
        for (TapNr = 0, x = 0; TapNr < CF->CPredOrder[CF->BestMethod[FilterNr]];
             TapNr++)
        {
          x += CF->CPredCoef[CF->BestMethod[FilterNr]][TapNr]
               * FH->ICoefA[FilterNr][CoefNr - TapNr - 1];
        }
        
        if (x >= 0)
        {
          c = RiceDecode(DSTData, CF->m[FilterNr][CF->BestMethod[FilterNr]]) - (x+4)/8;
        }
        else
        {
          c = RiceDecode(DSTData, CF->m[FilterNr][CF->BestMethod[FilterNr]]) + (-x+3)/8;
        }
        
        if ((c < -(1<<(SIZE_PREDCOEF-1))) || (c >= (1<<(SIZE_PREDCOEF-1))))
        {
          /*printf("ERROR: filter coefficient out of range!\n");
          exit(1);*/
          throw std::exception( "filter coefficient out of range!" );
        }
        else
        {
          FH->ICoefA[FilterNr][CoefNr] = c;
        }
      }
    }
  }
  
  for (ChNr = 0; ChNr < NrOfChannels; ChNr++)
  {
    FH->NrOfHalfBits[ChNr] = FH->PredOrder[FH->FSeg.Table4Segment[ChNr][0]];
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadProbabilityTables                                        */
/*                                                                         */
/* function : Read all Ptable data from the DST file, which contains:      */
/*            - which channel uses which Ptable                            */
/*            - for each Ptable all entries                                */
/*                                                                         */
/* pre      : a file must be opened by using getbits_init(),               */
/*            FH->NrOfPtables, CP->CPredOrder[], CP->CPredCoef[][]         */
/*                                                                         */
/* post     : FH->PtableLen[], CP->Coded[], CP->BestMethod[], CP->m[][],   */
/*            P_one[][]                                                    */
/*                                                                         */
/* uses     : types.h, fio_bit.h, conststr.h, stdio.h, stdlib.h            */
/*                                                                         */
/***************************************************************************/

void ReadProbabilityTables(dstdata      *DSTData,
						   FrameHeader  *FH,
                           CodedTable   *CP,
                           int          **P_one)
{
  int c;
  int EntryNr;
  int PtableNr;
  int TapNr;
  int x;
  
  /* Read the data of all probability tables (table entries) */
  for(PtableNr = 0; PtableNr < FH->NrOfPtables; PtableNr++)
  {
    FIO_BitGetIntUnsigned(DSTData, AC_HISBITS, &FH->PtableLen[PtableNr]);
    FH->PtableLen[PtableNr]++;
    
    if (FH->PtableLen[PtableNr] > 1)
    {
      FIO_BitGetIntUnsigned(DSTData, 1, &CP->Coded[PtableNr]);
      
      if (CP->Coded[PtableNr] == 0)
      {
        CP->BestMethod[PtableNr] = -1;
        for(EntryNr = 0; EntryNr < FH->PtableLen[PtableNr]; EntryNr++)
        {
          FIO_BitGetIntUnsigned(DSTData, AC_BITS - 1, &P_one[PtableNr][EntryNr]);
          P_one[PtableNr][EntryNr]++;
        }
      }
      else
      {
        FIO_BitGetIntUnsigned(DSTData, SIZE_RICEMETHOD, &CP->BestMethod[PtableNr]);
        if (CP->CPredOrder[CP->BestMethod[PtableNr]] >= FH->PtableLen[PtableNr])
        {
          /*fprintf(stderr,"ERROR: Invalid Ptable coding method!\n");
          exit(1);*/
          throw std::exception( "Invalid Ptable coding method!" );
        }
        
        for(EntryNr = 0; EntryNr < CP->CPredOrder[CP->BestMethod[PtableNr]];
            EntryNr++)
        {
          FIO_BitGetIntUnsigned(DSTData, AC_BITS - 1, &P_one[PtableNr][EntryNr]);
          P_one[PtableNr][EntryNr]++;
        }
        
        FIO_BitGetIntUnsigned(DSTData, SIZE_RICEM,
                              &CP->m[PtableNr][CP->BestMethod[PtableNr]]);
        
        for(EntryNr = CP->CPredOrder[CP->BestMethod[PtableNr]];
            EntryNr < FH->PtableLen[PtableNr]; EntryNr++)
        {
          for (TapNr = 0, x = 0;
               TapNr < CP->CPredOrder[CP->BestMethod[PtableNr]]; TapNr++)
          {
            x += CP->CPredCoef[CP->BestMethod[PtableNr]][TapNr]
                 * P_one[PtableNr][EntryNr - TapNr - 1];
          }
          
          if (x >= 0)
          {
            c = RiceDecode(DSTData, CP->m[PtableNr][CP->BestMethod[PtableNr]]) - (x+4)/8;
          }
          else
          {
            c = RiceDecode(DSTData, CP->m[PtableNr][CP->BestMethod[PtableNr]])+ (-x+3)/8;
          }
          
          if ((c < 1) || (c > (1 << (AC_BITS - 1))))
          {
            /*fprintf(stderr,"ERROR: Ptable entry out of range!\n");
            exit(1);*/
            throw std::exception( "Ptable entry out of range!" );
          }
          else
          {
            P_one[PtableNr][EntryNr] = c;
          }
        }
      }
    }
    else
    {
      P_one[PtableNr][0]       = 128;
      CP->BestMethod[PtableNr] = -1;
    }
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : ReadArithmeticCodeData                                       */
/*                                                                         */
/* function : Read arithmetic coded data from the DST file, which contains:*/
/*            - length of the arithmetic code                              */
/*            - all bits of the arithmetic code                            */
/*                                                                         */
/* pre      : a file must be opened by using getbits_init(), ADataLen      */
/*                                                                         */
/* post     : AData[]                                                      */
/*                                                                         */
/* uses     : fio_bit.h                                                    */
/*                                                                         */
/***************************************************************************/

void ReadArithmeticCodedData(dstdata        *DSTData,
							 int            ADataLen, 
                             unsigned char  *AData)
{
  int j;

  for(j = 0; j < ADataLen; j++)
  {
    FIO_BitGetChrUnsigned(DSTData, 1, &AData[j]);
  }
}


/***************************************************************************/
/*                                                                         */
/* name     : UnpackDSTframe                                               */
/*                                                                         */
/* function : Read a complete frame from the DST input file                */
/*                                                                         */
/* pre      : a file must be opened by using getbits_init()                */
/*                                                                         */
/* post     : Complete D-structure                                         */
/*                                                                         */
/* uses     : types.h, fio_bit.h, stdio.h, stdlib.h, constopt.h,           */
/*                                                                         */
/***************************************************************************/

int UnpackDSTframe(ebunch*    D, 
                   BYTE*      DSTdataframe, 
                   BYTE*      DSDdataframe)
{
  int   Dummy;
  int   Ready = 0;
  
  /* fill internal buffer with DSTframe */
  FillBuffer(&D->DSTData, DSTdataframe, D->FrameHdr.CalcNrOfBytes);
  
  /* interpret DST header byte */
  FIO_BitGetIntUnsigned(&D->DSTData, 1, &D->FrameHdr.DSTCoded);

  if (D->FrameHdr.DSTCoded == 0)
  {
    FIO_BitGetChrUnsigned(&D->DSTData, 1, &D->DstXbits.Bit);
    FIO_BitGetIntUnsigned(&D->DSTData, 6, &Dummy);
    if (Dummy != 0)
    {
      /*fprintf(stderr, "ERROR: Illegal stuffing pattern in frame %d!\n", D->FrameHdr.FrameNr);
      exit(1);*/
      throw std::exception( "Illegal stuffing pattern in frame!" );
    }

    /* Read DSD data and put in output stream */
    ReadDSDframe( &D->DSTData, 
                  D->FrameHdr.MaxFrameLen, 
                  D->FrameHdr.NrOfChannels, 
                  DSDdataframe);
  }
  else
  {
    ReadSegmentData(&D->DSTData, &D->FrameHdr);
    
    ReadMappingData(&D->DSTData, &D->FrameHdr);
    
	ReadFilterCoefSets(&D->DSTData, D->FrameHdr.NrOfChannels, &D->FrameHdr, &D->StrFilter);
    
	ReadProbabilityTables(&D->DSTData, &D->FrameHdr, &D->StrPtable, D->P_one);
    
	D->ADataLen = D->FrameHdr.CalcNrOfBits - get_in_bitcount(&D->DSTData);

    if ( D->ADataLen > D->FrameHdr.BitStreamLen )
    {
      throw std::exception( "Illegal arithmetic data length!" );
    }

	ReadArithmeticCodedData(&D->DSTData, D->ADataLen, D->AData);

    if ((D->ADataLen > 0) && (D->AData[0] != 0))
    {
      /*fprintf(stderr, "ERROR: Illegal arithmetic code in frame %d!\n", D->FrameHdr.FrameNr);
      exit(1);*/
      throw std::exception( "Illegal arithmetic code in frame!" );
    }
  }
  
  return Ready;
}

