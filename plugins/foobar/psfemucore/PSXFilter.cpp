// PSXFilter.cpp: implementation of the CPSXFilter class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "in_psf.h"
#include "PSXFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPSXFilter::CPSXFilter()
{
}

CPSXFilter::~CPSXFilter()
{
}

void CPSXFilter::Reset()
{
  lx1=0.0f;
  lx2=0.0f;
  ly1=0.0f;
  ly2=0.0f;
  hx1[0]=0.0f;
  hx2[0]=0.0f;
  hy1[0]=0.0f;
  hy2[0]=0.0f;
  hx1[1]=0.0f;
  hx2[1]=0.0f;
  hy1[1]=0.0f;
  hy2[1]=0.0f;
}

void CPSXFilter::Redesign(int nSampleRate)
{
// 150 and 4 is about spot-on for low

if(nSampleRate==48000) {
// LOW SHELF sample=48000.000000 freq=150.000000 db=4.000000
la0=1.00320890889339290000;
la1=-1.97516434134506300000;
la2=0.97243484967313087000;
lb1=-1.97525280404731810000;
lb2=0.97555529586426892000;
// HIGH SHELF sample=48000.000000 freq=6000.000000 db=-5.000000
ha0=1.52690772687271160000;
ha1=-1.62653918974914990000;
ha2=0.57997976029249387000;
hb1=-0.80955590379048203000;
hb2=0.28990420120653748000;
} else {
// LOW SHELF sample=44100.000000 freq=150.000000 db=4.000000
la0=1.00349314378906680000;
la1=-1.97295980267143170000;
la2=0.97003400595243994000;
lb1=-1.97306449030610280000;
lb2=0.97342246210683581000;
// HIGH SHELF sample=44100.000000 freq=6000.000000 db=-5.000000
ha0=1.50796284998687450000;
ha1=-1.48628361940858910000;
ha2=0.52606706092412581000;
hb1=-0.71593574211151134000;
hb2=0.26368203361392234000;
}

  Reset();
}

void CPSXFilter::Process(short *stereobuffer, int nSamples)
{
#define OVERALL_SCALE (0.87f)

  int i;
  /* initialise the filter */
  float out, in;

  for (i = 0; i < nSamples; i++) {
    float l, r;
    l = stereobuffer[0];
    r = stereobuffer[1];

    float mid  = l+r;
    float side = l-r;
    //
    // Low shelf
    //
    in = mid;
    out = la0 * in + la1 * lx1 + la2 * lx2 - lb1 * ly1 - lb2 * ly2;
    lx2 = lx1; lx1 = in;
    ly2 = ly1; ly1 = out;
    mid = out;

    l = ((0.5f)*(OVERALL_SCALE))*(mid + side);
    r = ((0.5f)*(OVERALL_SCALE))*(mid - side);

    //
    // High shelf
    //
    in = l;
    out = ha0 * in + ha1 * hx1[0] + ha2 * hx2[0] - hb1 * hy1[0] - hb2 * hy2[0];
    hx2[0] = hx1[0]; hx1[0] = in;
    hy2[0] = hy1[0]; hy1[0] = out;
    l = out;

    in = r;
    out = ha0 * in + ha1 * hx1[1] + ha2 * hx2[1] - hb1 * hy1[1] - hb2 * hy2[1];
    hx2[1] = hx1[1]; hx1[1] = in;
    hy2[1] = hy1[1]; hy1[1] = out;
    r = out;

    int i_l = l;
    int i_r = r;
    if(i_l > ( 32767)) i_l = ( 32767);
    if(i_r > ( 32767)) i_r = ( 32767);
    if(i_l < (-32767)) i_l = (-32768);
    if(i_r < (-32767)) i_r = (-32768);
    stereobuffer[0] = i_l;
    stereobuffer[1] = i_r;
    stereobuffer += 2;
  }

}
