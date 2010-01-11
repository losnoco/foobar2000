#include "cvt.h"

bool is_cmf(const BYTE* buf,int s)
{
	return s>0x20 && *(DWORD*)buf == _rv('CTMF');
}

static BYTE tempodat[7]={0,0xFF,0x51,0x03,0,0,0};

bool load_cmf(MIDI_file * mf,const BYTE* ptr,int sz)
{
	const BYTE* _t=ptr+*(WORD*)(ptr+8);
	int _ts=ptr+sz-_t;
	mf->size=14+8+7+_ts;
	BYTE* _pt=(BYTE*)malloc(mf->size);
	if (!_pt) return 0;

	mf->data=_pt;
	*(DWORD*)_pt=_rv('MThd');
	_pt+=4;
	*(DWORD*)_pt=_rv(6);
	_pt+=4;
	*(WORD*)_pt=0;
	_pt+=2;
	*(WORD*)_pt=0x0100;
	_pt+=2;
	*(WORD*)_pt=rev16(*(WORD*)(ptr+10));
	_pt+=2;
	*(DWORD*)_pt=_rv('MTrk');
	_pt+=4;
	*(DWORD*)_pt=rev32(_ts+7);
	_pt+=4;
	DWORD tm=(DWORD)(48000000/(*(WORD*)(ptr+12)));
	tempodat[4]=(BYTE)((tm>>16)&0xFF);
	tempodat[5]=(BYTE)((tm>>8)&0xFF);
	tempodat[6]=(BYTE)(tm&0xFF);
	memcpy(_pt,tempodat,7);
	_pt+=7;
	memcpy(_pt,_t,_ts);
	return 1;
}