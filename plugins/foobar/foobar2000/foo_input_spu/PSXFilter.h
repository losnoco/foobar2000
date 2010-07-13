#pragma once

class CPSXFilter  
{
public:
	void Process(float *stereobuffer, int nSamples);
	void Redesign(int nSampleRate);
	void Reset();
	CPSXFilter();
	virtual ~CPSXFilter();

private:
	float lx1,lx2,ly1,ly2;
	float la0,la1,la2,lb1,lb2;

	float hx1[2],hx2[2],hy1[2],hy2[2];
	float ha0,ha1,ha2,hb1,hb2;
};
