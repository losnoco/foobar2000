#include "main.h"

#define _RIFF 'FFIR'
#define _MThd 'dhTM'
#define _MTrk 'krTM'
#define _RMID 'DIMR'
#define _data 'atad'



//#define BLAH

class CGetInfo
{
public:
	MIDI_file * mf;
	int nch;
	int got_notes;
	int cur_track_start;
	int c_track,s_track;
	bool is_blah,f2;
	int max_ff_track,max_ff_num;

	CTempoMap *tmap,*ttmap;
	CSysexMap *smap;
	CMarkerMap *mmap;
	void CleanTempo();
	int DoTrack(const BYTE* track,int size,pfc::string8 & name,pfc::string8 & text,int);
	bool Run(MIDI_file* mf);
};

static bool memicmp(char* b1,char* b2,int s)
{
	int n;
	for(n=0;n<s;n++) if (tolower(b1[n])!=tolower(b2[n])) return 1;
	return 0;
}

static bool is_kar(char* ptr,int siz)	//hack
{
	siz-=7;//strlen("karaoke");
	int n;
	for(n=0;n<=siz;n++)
	{
		if (!memicmp(ptr+n,"karaoke",7)) return 1;
	}
	return 0;
}

int CGetInfo::DoTrack(const BYTE* track,int size,pfc::string8 & name,pfc::string8 & text,int cpos)
{
	int res=0,_res=0;
	int n=0;
	BYTE lc1,lastcom=0;
	bool run=0;
	int ff_num=0;
	while(n<size)
	{
		{
			int d;
			int _n=DecodeDelta(track+n,&d,size-n);
			if (_n<4) res+=d;
			n+=_n;
		}
		if (track[n]==0xFF)	//meta-events
		{
			if (f2) _res=res;
			if (track[n+1]==0x51 && track[n+2]==0x03)	//tempo
			{
				if (ttmap) ttmap->AddEntry(cpos+res,((DWORD)track[n+3]<<16)+((DWORD)track[n+4]<<8)+((DWORD)track[n+5]));
				n+=6;
			}
			else if (track[n+1]==0x2F && track[n+2]==0x00)
			{
				if (ff_num>max_ff_num)
				{
					max_ff_num=ff_num;
					max_ff_track=cur_track_start;
				}
				return _res;
			}
			else
			{
				DWORD _l=0,l1;
				UINT n1=0;
				{					
					do 
					{
						_l=(_l<<7)|(track[n+2+n1++]&0x7F);
					}
					while(track[n+1+n1]&0x80);
				}
				if (_l>255) l1=255;
				else l1=_l;
				if (track[n+1]<0x10) ff_num++;

				switch(track[n+1])
				{
				case 6:
//					if (!cpr || *cpr) break;
					if (mf->info.markers.length()) mf->info.markers.add_string("\x0d\x0a");
					mf->info.markers.add_string((char*)(track+n+n1+2),l1);
					mmap->AddEvent(&track[n+n1+2], l1, cpos+res);
					break;
				case 2:
					if (mf->info.copyright.length()) mf->info.copyright.add_string("\x0d\x0a");
					mf->info.copyright.add_string((char*)(track+n+n1+2),l1);
					break;
				case 5:
					is_blah=1;
					break;
				case 1:
					if (is_kar((char*)track+n+n1+2,_l)) is_blah=1;
					if (text.length()) text.add_string("\x0d\x0a");
					text.add_string((char*)(track+n+n1+2),l1);
					break;
				case 3:
					if (is_kar((char*)track+n+n1+2,_l)) is_blah=1;				
				case 4:
					if (name.length()==0)
					{
						name.add_string((char*)(track+n+n1+2),l1);
					}
					break;
				}
				n+=2+n1+_l;
			}
		}
		else if ((track[n]&0xF0)==0xF0)
		{
			if (track[n]==0xF0)
			{
				_res=res;
				UINT s=ReadSysex(&track[n],size-n);
				smap->AddEvent(&track[n],s,cpos+res);
				n+=s;
				if (s_track==-1) s_track=c_track;
				else if (s_track!=c_track) s_track=-2;

			}
			else //hack...
			if (track[n]==0xF7) n++;
			else
			{
#ifdef BLAH
				char tmp[32];
				wsprintf(tmp,"invalid Fx event at %x",n);
				MessageBox(0,tmp,0,0);
#endif
				return -1;
			}
		}
		else
		{
			lc1=track[n];
//			if (lc1 == 0) return -1;
			if ((lc1&0x80)==0)
			{
				if (lastcom==0)
					return -1;
				run=1;
				lc1=lastcom;
				n--;
				
			} else run=0;
			_res=res;
			switch(lc1&0xF0)
			{
			case 0x80:
			case 0x90:
				if (!(got_notes&(1<<(lc1&0xF))))
				{
					nch++;
					got_notes|=1<<(lc1&0xF);
				}
			case 0xB0:
			case 0xA0:
			case 0xE0:
				n+=3;
				lastcom=lc1;
				break;
			case 0xC0:
			case 0xD0:
				n+=2;
				lastcom=lc1;
				break;
			default:
				return -1;
			}
		}
	}
	return _res;
}

bool GetMidiInfo(MIDI_file* mf)
{
	CGetInfo i;
	return i.Run(mf);
}

bool CGetInfo::Run(MIDI_file* _mf)
{
	mf=_mf;
	nch=0;
	s_track=-1;
	is_blah=0;
	max_ff_track=max_ff_num=0;

	MIDIHEADER hd = *(MIDIHEADER*)(mf->data+8);
	tmap=tmap_create();
	if (!tmap) return 0;
	smap=smap_create();
	mmap=mmap_create();
	ttmap=0;

	mf->tmap=tmap;//avoid stupid memleaks
	mf->smap=smap;
	mf->mmap=mmap;

	tmap->AddEntry(0,500000);

	DWORD sz = mf->size-14;
	FixHeader(hd);

	got_notes=0;
	nch=0;
	
	const BYTE* trax=mf->data+14;
	const BYTE* ntrak=trax;
	if (hd.trax>0x100 || hd.fmt>2) return 0;
	f2=hd.fmt==2;

	int n,tmp;
	int size=0;
	mf->info.traxnames = new pfc::string8[hd.trax];
	mf->info.traxtext = new pfc::string8[hd.trax];


	for(c_track=0;c_track<hd.trax;c_track++)
	{
		if (!ttmap) ttmap=tmap_create();
		if ((UINT)(ntrak-trax)>=(UINT)sz || *((DWORD*)ntrak)!='krTM' || (tmp=rev32(*((DWORD*)ntrak+1)))+ntrak>sz+trax) return 0;
		cur_track_start=ntrak-mf->data;
		tmp=DoTrack(ntrak+8,tmp,mf->info.traxnames[c_track],mf->info.traxtext[c_track],f2 ? size : 0);
		if (tmp==-1)
		{
/*			ntrak[8]=0;
			ntrak[9]=0xFF;
			ntrak[10]=0x2F;
			ntrak[11]=0;*/
#ifdef BLAH
			{
				char e[128];
				wsprintf(e,"Bad track #%u",c_track);
				MessageBox(0,e,ERROR,0);
			}
#endif
		}
		else
		{
			if (f2) size+=tmp;
			else if (tmp>size) size=tmp;
			if (ttmap->pos)
			{
				mf->tmap=tmap=tmap_merge(tmap,ttmap);
				ttmap=0;
			}
			
		}
		ntrak+=rev32(*((DWORD*)ntrak+1))+8;
	}
	if (ttmap) delete ttmap;

	if (!tmap) return 0;

	mf->tix=MulDiv(size+50,768,hd.dtx);

	DWORDLONG res=0;
	for(n=0;n<tmap->pos-1 && tmap->data[n].pos<size;n++)
	{
		res+=UInt32x32To64(tmap->data[n].tm,tmap->data[n+1].pos-tmap->data[n].pos);
	}
	if (tmap->data[n].pos<size) res+=UInt32x32To64(tmap->data[n].tm,size-tmap->data[n].pos);
	mf->len=(DWORD)(res/(hd.dtx*1000));
	
	mf->info.channels=nch;
	mf->info.fmt=hd.fmt;
	mf->info.ntrax=hd.trax;
	mf->info.tix=size;
	if (!f2 && smap && s_track==-2) smap->CleanUp(); //todo: optimize this shit...
/*	mf->tmap=tmap;
	mf->smap=smap;*/
	mf->info.e_type=smap->GetType();

	if (is_blah)
	{
		mf->kar_track=max_ff_track;
	}
	return 1;
}