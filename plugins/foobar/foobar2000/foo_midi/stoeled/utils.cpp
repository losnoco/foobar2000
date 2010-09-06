#include "main.h"

DWORD _fastcall rev32(DWORD d) {return _rv(d);}

DWORD read_16_le(const void * d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [0] + ( d_ [1] * 256 ) );
}

DWORD read_16_be(const void *d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [1] + ( d_ [0] * 256 ) );
}

DWORD read_24_le(const void *d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [0] + ( d_ [1] * 256 ) + ( d_ [2] * 65536 ) );
}

DWORD read_24_be(const void *d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [2] + ( d_ [1] * 256 ) + ( d_ [0] * 65536 ) );
}

DWORD read_32_le(const void *d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [0] + ( d_ [1] * 256 ) + ( d_ [2] * 65536 ) + ( d_ [3] * 16777216 ) );
}

DWORD read_32_be(const void *d)
{
	const unsigned char * d_ = (const unsigned char *) d;
	return ( d_ [3] + ( d_ [2] * 256 ) + ( d_ [1] * 65536 ) + ( d_ [0] * 16777216 ) );
}

#ifdef USE_LOG
static HANDLE hLog;
void log_start()
{
	hLog=CreateFile("c:\\in_midi.log",GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
	SetFilePointer(hLog,0,0,FILE_END);
	log_write("opening log");
}

void log_quit() {log_write("closing log");log_write("");log_write("");CloseHandle(hLog);}

void log_write(char* t)
{
	DWORD bw;
	WriteFile(hLog,t,strlen(t),&bw,0);
	char _t[2]={13,10};
	WriteFile(hLog,_t,2,&bw,0);
	FlushFileBuffers(hLog);
}
#endif


void * grow_buf::set_size(UINT new_used)
{
	if (new_used==0)
	{
		if (mem_logic != ALLOC_FAST_DONTGODOWN)
		{
			if (data!=0) {free(data);data=0;}
			size = 0;
		}
	}
	else
	{
		UINT new_size;
		if (mem_logic == ALLOC_FAST || mem_logic == ALLOC_FAST_DONTGODOWN)
		{
			new_size = size;
			if (new_size < 1) new_size = 1;
			while(new_size < new_used) new_size <<= 1; 
			if (mem_logic!=ALLOC_FAST_DONTGODOWN) while(new_size>>1 > new_used) new_size >>= 1;
		}
		else
		{
			new_size = new_used;
		}

		if (new_size!=size)
		{
			if (data==0)
			{
				data = malloc(new_size);
			}
			else
			{
				void * new_data;
#if defined(_DEBUG) && 0
				new_data = malloc(new_size);
				if (new_data) memcpy(new_data,data,new_size>size ? size : new_size);
				if (size >= 4) *(DWORD*)data = 0xDEADBEEF;
				free(data);
				data = new_data;				
#else
				new_data = realloc(data,new_size);
				if (new_data==0) free(data);
				data = new_data;
#endif
			}
			size = new_size;
		}
	}
	used = new_used;
	return data;
}

void grow_buf::prealloc(UINT num)
{
	if (size<num && mem_logic==ALLOC_FAST_DONTGODOWN)
	{
		int old_used = used;
		set_size(num);
		used = old_used;
	}
}

void* grow_buf::write_ptr(const void *ptr, unsigned bytes,unsigned start)
{
	check_size(bytes+start);

	if (ptr) 
		memcpy((char*)get_ptr()+start,ptr,bytes);
	else 
		memset((char*)get_ptr()+start,0,bytes);
	
	return (char*)get_ptr()+start;
}

static unsigned tick_to_ms( const CTempoMap * tmap, unsigned d, unsigned dtx )
{
	unsigned pos_ms = 0;
	DWORD pos = 0;
	DWORD t_pos=0;
	DWORD cur_temp=0;
	UINT ntm=tmap->pos;

	while(t_pos<ntm && pos+d>=(UINT)tmap->data[t_pos].pos)
	{
		DWORD d1=tmap->data[t_pos].pos-pos;
		pos_ms+=MulDiv(cur_temp,d1<<8,dtx);
		cur_temp=tmap->data[t_pos].tm;
		t_pos++;
		pos+=d1;
		d-=d1;
	}

	pos_ms+=MulDiv(cur_temp,d<<8,dtx);

	return pos_ms;
}

//tempo map object

CTempoMap* tmap_create()
{
	CTempoMap* m=new CTempoMap;
	if (m)
	{
		m->pos=0;
		m->size=0x100;
		m->data=(TMAP_ENTRY*)malloc(m->size*sizeof(TMAP_ENTRY));
	}
	return m;
}

void CTempoMap::AddEntry(int _p,int tm)
{
	if (!data) {pos=size=0;return;}
	if (pos && _p<=data[pos-1].pos) {data[pos-1].tm=tm;return;}
	if (pos==size)
	{
		size*=2;
		data=(TMAP_ENTRY*)realloc(data,size*sizeof(TMAP_ENTRY));
		if (!data) {pos=0;return;}
	}
	data[pos].pos=_p;
	data[pos].tm=tm;
	pos++;
}

int ReadSysex(const BYTE* src,int ml)
{
	int r=1;
	while(r<ml)
	{
		r++;
		if (src[r]==0xF7) return r+1;
	}
	int d;
	r=1+DecodeDelta(src+1,&d,ml-1);
	r+=d;
	return r;
}

int DecodeDelta(const BYTE* src,int* _d,int sz)
{
	if (!sz) return 0;
	int l=0;
	int d=0;
	BYTE b;
	do {
		b=src[l++];
		d=(d<<7)|(b&0x7F);
	} while(b&0x80 && l < sz);
	*_d=d;
	return l;
}

int EncodeDelta(BYTE* dst,int d)
{
	if (d==0)
	{
		dst[0]=0;
		return 1;
	}
	else
	{
		int r=0;
		int n=1;
		while(d>>(7*n)) n++;
		do {
			n--;
			BYTE b=(BYTE)((d>>(7*n))&0x7F);
			if (n) b|=0x80;
			dst[r++]=b;
		} while(n);
		return r;
	}
}

int CTempoMap::BuildTrack(grow_buf & out)
{
	if (!pos) return 0;
	int start=out.get_size();
	//BYTE* trk=(BYTE*)malloc(8+4+pos*10);
	//if (!trk) return 0;
	out.write_dword(_rv('MTrk'));
	out.write_dword(0);//track size
	DWORD ct=0;
	int n;
	BYTE t_event[6]={0xFF,0x51,0x03,0,0,0};
	for(n=0;n<pos;n++)
	{
		DWORD t=data[n].pos;
		gb_write_delta(out,t-ct);
		ct=t;
		t=data[n].tm;
		t_event[3]=(BYTE)(t>>16);
		t_event[4]=(BYTE)(t>>8);
		t_event[5]=(BYTE)(t);
		out.write(t_event,6);
	}
	out.write_dword(0x002FFF00);
	out.write_dword_ptr(rev32(out.get_size()-(start+8)),start+4);
	return 1;
}

//sysex map management

void CSysexMap::AddEvent(const BYTE* e,DWORD s,DWORD t)
{
	if (!data || !events) return;
	DWORD np=pos+1;
	if (np>=e_size)
	{
		do {
			e_size<<=1;
		} while(np>=e_size);
		events=(SYSEX_ENTRY*)realloc(events,e_size*sizeof(SYSEX_ENTRY));
		if (!events) return;
	}
	DWORD nd=d_pos+s;
	if (nd>=d_size)
	{
		do {
			d_size<<=1;
		} while(nd>=d_size);
		data=(BYTE*)realloc(data,d_size);
		if (!data) return;
	}
	data[d_pos]=0xF0;
	int x;
	int sp=DecodeDelta(e+1,&x,s-1);
	memcpy(data+d_pos+1,e+1+sp,s-1-sp);
	events[pos].pos=t;
	events[pos].ofs=d_pos;
	events[pos].len=s-sp;
	d_pos=nd-sp;
	pos++;
}

CSysexMap* smap_create()
{
	CSysexMap* s=new CSysexMap;
	if (s)
	{
		s->e_size=0x10;
		s->d_size=0x40;
		s->events=(SYSEX_ENTRY*)malloc(sizeof(SYSEX_ENTRY)*s->e_size);
		s->data=(BYTE*)malloc(s->d_size);
		s->d_pos=s->pos=0;
	}
	return s;
}


CSysexMap::~CSysexMap()
{
	if (data) free(data);
	if (events) free(events);
}

BYTE d_GMReset[6]={0xF0,0x7E,0x7F,0x09,0x01,0xF7};
BYTE d_XGReset[9]={0xf0,0x43,0x10,0x4c,0x00,0x00,0x7e,0x00,0xf7};
BYTE d_GSReset[11]={0xF0,0x41,0x10,0x42,0x12,0x40,0x00,0x7F,0x00,0x41,0xF7};

CSysexMap* CSysexMap::Translate(MIDI_file * mf)
{
	CTempoMap* tmap=mf->tmap;
	if (!events || !data || !tmap) return 0;
	CSysexMap* nm=smap_create();
	if (!nm) return 0;
	nm->d_size=d_size;
	nm->d_pos=d_pos;
	nm->data=(BYTE*)realloc(nm->data,nm->d_size);
	if (!nm->data) {delete nm;return 0;}
	memcpy(nm->data,data,d_pos);
	nm->e_size=e_size;
	nm->pos=pos;
	nm->events=(SYSEX_ENTRY*)realloc(nm->events,sizeof(SYSEX_ENTRY)*nm->e_size);
	if (!nm->events) {delete nm;return 0;}
	
	int pos_ms=0;
	int n=0;
	int cur_temp=0;
	int ntm=tmap->pos,t_pos=0;
	int p_t=0;
	int dtx = rev16(*(WORD*)(mf->data+12))*1000;
	int pos_tx=0;



	while(n<pos)
	{
		pos_tx=events[n].pos;
		/*
		int d=pos_tx-p_t;
		p_t=pos_tx;
		while(t_pos<ntm && pos_tx+d>=tmap->data[t_pos].pos)
		{
			DWORD d1=tmap->data[t_pos].pos-pos_tx;
			pos_ms+=MulDiv(cur_temp,d1<<8,dtx);
			cur_temp=tmap->data[t_pos].tm;
			t_pos++;
			pos_tx+=d1;
			d-=d1;
		}
		pos_ms+=MulDiv(cur_temp,d<<8,dtx);
		pos_tx+=d;
		*/
		pos_ms = tick_to_ms( tmap, pos_tx, dtx );

		nm->events[n].pos=(pos_ms+128)>>8;
		nm->events[n].ofs=events[n].ofs;
		nm->events[n].len=events[n].len;
		n++;
	}
	return nm;
}

int CSysexMap::BuildTrack(grow_buf & out)
{
	if (!pos) return 0;
		
	int start=out.get_size();
	out.write_dword(_rv('MTrk'));
	out.write_dword(0);
	
	int ct=0;
	int n;
	for(n=0;n<pos;n++)
	{
		DWORD t=events[n].pos;
		gb_write_delta(out,t-ct);
		ct=t;
		out.write_byte(0xF0);
		gb_write_delta(out,events[n].len-1);
		out.write(data+events[n].ofs+1,events[n].len-1);
	}
	out.write_dword(0x002FFF00);
	out.write_dword_ptr(rev32(out.get_size()-(start+8)),start+4);
	return 1;
}



const char* CSysexMap::GetType()
{
	int ret=0; int ret2=0;
	int n;
	for(n=0;n<pos;n++)
	{
		int len = events[n].len;
		int ofs = events[n].ofs;
		if (len>1) ret=data[ofs+1];
		if (len>3) ret2=data[ofs+3];
		if (ret!=0x7E) break;
	}

	switch(ret)
	{
	case 0x7E:
		return "GM";
	case 0x43:
		return "XG";
	case 0x42:
		return "X5";
	case 0x41:
		if ( ret2 == 0x42 ) return "GS";
		else if ( ret2 == 0x16 ) return "MT-32";
		else if ( ret2 == 0x14 ) return "D-50";
		break;
	}
	return 0;
}

void CSysexMap::CleanUp()
{
	if (!pos) return;
	int n,m;
	for(n=0;n<pos-1;n++)
	{
		for(m=n+1;m<pos;m++)
		{
			if (events[n].pos>events[m].pos)
			{
				SYSEX_ENTRY t=events[n];
				events[n]=events[m];
				events[m]=t;
			}
		}
	}
}

/*BOOL DoOpenFile(HWND w,char* fn,char* filt,char* ext,BOOL save)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=w;
	ofn.lpstrFilter=filt;
	ofn.lpstrFile=fn;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrDefExt=ext;
	if (save)
	{
		ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
		return GetSaveFileName(&ofn);		
	}
	else
	{
		ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
		return GetOpenFileName(&ofn);
	}
}

BOOL DoSaveFile(HWND w,char* fn,char* filt,char* ext)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=w;
	ofn.lpstrFilter=filt;
	ofn.lpstrFile=fn;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrDefExt=ext;
	ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
	return GetOpenFileName(&ofn);
}*/

typedef void (*SYSEXFUNC)(void*,BYTE*,UINT);

#define rsysex(X) f(i,X,sizeof(X))
#define _sysex(X,Y) f(i,X,Y)

MIDI_EVENT* do_table(MIDI_file * mf,UINT prec,UINT * size,/*UINT* _lstart,UINT* _lend,UINT _ltype,*/DWORD cflags)
{
	BYTE * data_ptr = 0;
	int data_size = 0;
	if (!DoCleanUp(mf,CLEAN_1TRACK|CLEAN_NOSYSEX|CLEAN_NOTEMPO|cflags,(void**)&data_ptr,&data_size)) return 0;
	if (data_size<=0x0e) {free(data_ptr);return 0;}

	UINT ts;
	BYTE* track;
	track=data_ptr+8+6+8;
	ts=read_32_be(track - 4);//rev32(*(DWORD*)(track-4));
	CTempoMap* tmap=mf->tmap;
	UINT n=0;
	UINT pt=0;
	CSysexMap* smap;

	if (mf->smap && mf->smap->pos)
	{
		smap=mf->smap;
	}
	else smap=0;

	n=0;
	DWORD pos=0;
	DWORD pos_ms=0;
	UINT dtx=(UINT)rev16(*(WORD*)(data_ptr+8+4))*1000/prec;
	grow_buf boo;

	int ns=0;
	UINT track_pos=0,smap_pos=0;
	/*UINT loop_start=-1;
	UINT loop_end=-1;*/

	{
		int _d;
		n+=DecodeDelta(track+n,&_d,ts-n);
		track_pos+=_d;
	}

	if (smap)
	{
		smap_pos=smap->events[0].pos;
	}
	else smap_pos=-1;

	while(1)
	{
		DWORD ev=0;
		DWORD d=0;
		{
			if (track_pos<smap_pos)
			{
				d=track_pos-pos;
				ev=track[n];//(*(DWORD*)(track+n))&0xFFFFFF;
				if ((ev&0xF0)==0xF0)
				{
					track_pos=-1;
					continue;
				}
				if ((ev&0xF0)==0xC0 || (ev&0xF0)==0xD0)
				{
					ev |= track[n+1] * 256;
					n+=2;
				}
				else
				{
					ev |= track[n+1] * 256 + track[n+2] * 65536;
					n+=3;
				}
				if ((ev&0xFF00F0)==0x90)
				{
					ev=(ev&0xFF0F)|0x7F0080;
				}
				int _d;
				n+=DecodeDelta(track+n,&_d,ts-n);
				track_pos+=_d;
			}
			else if (smap_pos!=-1)
			{
				d=smap_pos-pos;
				ev=0x80000000|ns;
				ns++;
				if (ns==smap->pos) smap_pos=-1;
				else smap_pos=smap->events[ns].pos;
			}
		}
		if (!ev) break;
		pos+=d;
		pos_ms = tick_to_ms( tmap, pos, dtx );
		{
			MIDI_EVENT me={(pos_ms+128)>>8,ev};
			boo.write(&me,sizeof(me));
		}
	}
	
	free(data_ptr);

	UINT sz=boo.get_size();
	MIDI_EVENT* ret=(MIDI_EVENT*)boo.finish();
	if (ret)
	{
		*size=sz>>3;//sz/sizeof(MIDI_EVENT);
		/*if (cfg_loop_type==2 && loop_start==-1) loop_start=0;
		else if (cfg_loop_type==0) loop_start=-1;
		if (_lstart) *_lstart=loop_start;*/
	}
	return ret;
}

void gb_write_delta(grow_buf & gb,DWORD d)
{
	BYTE tmp[8];
	gb.write(tmp,EncodeDelta(tmp,d));
}

CTempoMap* tmap_merge(CTempoMap* m1,CTempoMap* m2)
{
	int p1=0,p2=0;
	CTempoMap * ret=0;
	if (m1 && m2 && m1->data && m2->data)
	{
		ret=tmap_create();
		if (ret)
		{
			while(p1<m1->pos && p2<m2->pos)
			{
				if (m1->data[p1].pos<=m2->data[p2].pos)
				{
					ret->AddEntry(m1->data[p1].pos,m1->data[p1].tm);
					p1++;
				}
				else
				{
					ret->AddEntry(m2->data[p2].pos,m2->data[p2].tm);
					p2++;
				}
			}
			while(p1<m1->pos)
			{
				ret->AddEntry(m1->data[p1].pos,m1->data[p1].tm);
				p1++;
			}
			while(p2<m2->pos)
			{
				ret->AddEntry(m2->data[p2].pos,m2->data[p2].tm);
				p2++;
			}
		}
	}
	if (m1) delete m1;
	if (m2) delete m2;
	return ret;

}

KAR_ENTRY * kmap_create(MIDI_file* mf,UINT prec,UINT * num,char** text)
{
	if (!mf->kar_track) return 0;
	grow_buf b_data,b_map;
	KAR_ENTRY te;
	BYTE *track=(BYTE*)mf->data+mf->kar_track+8;
	BYTE *track_end = track+read_32_be(mf->data + mf->kar_track + 4);//rev32(*(DWORD*)(mf->data+mf->kar_track+4));
	int time=0;
	int ptr=0;
	BYTE lc=0;
	while(track<track_end)
	{
		int d;
		track+=DecodeDelta(track,&d,track_end-track);
		time+=d;
		if (*track==0xFF)	//meta
		{
			BYTE type=track[1];
			track+=2;
			track+=DecodeDelta(track,&d,track_end-track);
			char * ptr=(char*)track;
			track+=d;
			if ((type==0x5 || type==0x1) && d && *ptr!='@')	//lyrics
			{
				te.time=time;
				te.foo=1;
				int n;
				te.start=b_data.get_size();
				for(n=0;n<d;n++)
				{
					switch(ptr[n])
					{
//					case '@':
					case '\\':
					case '/':
					case 0x0D:
						b_data.write("\x0d\x0a",2);
						break;
					case 0x0A:
						break;
					default:
						te.foo=0;
						b_data.write_byte(ptr[n]);
						break;
					}
				}
				te.end=b_data.get_size();
				if (te.start<te.end) b_map.write(&te,sizeof(te));
			}
		}
		else if (*track==0xF0)
		{
			track++;
			track+=DecodeDelta(track,&d,track_end-track);
			track+=d;
		}
		else if ((*track&0xF0)==0xF0)
		{
			track++;//hack
		}
		else
		{
			if (*track&0x80) lc=*(track++)&0xF0;
			if (lc==0 || lc==0xC0 || lc==0xD0) track++;
			else track+=2;
		}
	}
	int map_siz = b_map.get_size();
	KAR_ENTRY * map=(KAR_ENTRY*)b_map.finish();
	map_siz/=sizeof(KAR_ENTRY);

	if (num) *num=map_siz;
	
	if (text)
	{
		b_data.write_byte(0);
		*text=(char*)b_data.finish();
	}
	else b_data.reset();

	if (map)
	{
		int n;

		time=0;
		
		CTempoMap* tmap=mf->tmap;

		int pos_ms=0;
		int t_pos=0;
		int cur_temp=0;
		int dtx=(UINT)rev16(*(WORD*)(mf->data+8+4))*1000/prec;

		for(n=0;n<map_siz;n++)
		{
			int d=0;
			d=map[n].time-time;
			
			while(t_pos<tmap->pos && time+d>=tmap->data[t_pos].pos)
			{
				DWORD d1=tmap->data[t_pos].pos-time;
				pos_ms+=MulDiv(cur_temp,d1,dtx);
				cur_temp=tmap->data[t_pos].tm;
				t_pos++;
				time+=d1;
				d-=d1;
			}
			pos_ms+=MulDiv(cur_temp,d,dtx);
			time+=d;
			map[n].time=pos_ms;
		}
	}

	return map;
}

unsigned meta_table::num_entries() const
{
	unsigned rval = 0;
	entry * feh = entries;
	while (feh)
	{
		feh = feh->next;
		rval++;
	}
	return rval;
}

const void * meta_table::enum_entry(unsigned idx) const
{
	const entry * feh = entries;
	while (feh && idx)
	{
		feh = feh->next;
		idx--;
	}
	return feh;
}

const void * meta_table::enum_next_entry(const void * ent)
{
	const entry * feh = (const entry *) ent;
	if (feh)
		return feh->next;
	return NULL;
}

const char * meta_table::enum_name(const void * ent)
{
	const entry * feh = (const entry *) ent;
	if (feh)
	{
		switch (feh->id)
		{
		case _rv('IALB'):
			return "album";
		case _rv('IARL'):
			return "archival_location";
		case _rv('IART'):
			return "artist";
		case _rv('ITRK'):
			return "tracknumber";
		case _rv('ICMS'):
			return "commissioned";
		case _rv('ICMP'):
			return "composer";
		case _rv('ICMT'):
			return "comment";
		case _rv('ICOP'):
			return "copyright";
		case _rv('ICRD'):
			return "creation_date";
		case _rv('IENG'):
			return "engineer";
		case _rv('IGNR'):
			return "genre";
		case _rv('IKEY'):
			return "keywords";
		case _rv('IMED'):
			return "medium";
		case _rv('INAM'):
			return "title";
		case _rv('IPRD'):
			return "product";
		case _rv('ISBJ'):
			return "subject";
		case _rv('ISFT'):
			return "software";
		case _rv('ISRC'):
			return "source";
		case _rv('ISRF'):
			return "source_form";
		case _rv('ITCH'):
			return "technician";
		default:
			return "unknown";
		}
	}
	return NULL;
}

const char * meta_table::enum_data(const void * ent)
{
	const entry * feh = (const entry *) ent;
	if (feh)
	{
		return feh->data;
	}
	return NULL;
}

void meta_table::insert_entry(unsigned idx, int id, const char * data, unsigned size)
{
	entry * feh = entries;
	entry * prev = NULL;
	entry * moo;
	while (feh && idx)
	{
		prev = feh;
		feh = feh->next;
		idx--;
	}
	moo = new entry;
	moo->id = id;
	moo->data = new char[size + 1];
	memcpy(moo->data, data, size);
	moo->data[size] = 0;
	moo->next = feh;
	if (prev)
	{
		prev->next = moo;
	}
	else
	{
		entries = moo;
	}
}

bool meta_table::remove_entry(unsigned idx)
{
	entry * feh = entries;
	entry * prev = NULL;
	while (feh && idx)
	{
		prev = feh;
		feh = feh->next;
		idx--;
	}
	if (feh)
	{
		entry * next = feh->next;
		delete [] feh->data;
		delete feh;
		if (prev)
		{
			prev->next = next;
		}
		else
		{
			entries = next;
		}
		return true;
	}
	return false;
}

void CMarkerMap::AddEvent(const BYTE* e,DWORD s,DWORD t)
{
	if (!data || !events) return;
	DWORD np=pos+1;
	if (np>=e_size)
	{
		do {
			e_size<<=1;
		} while(np>=e_size);
		events=(SYSEX_ENTRY*)realloc(events,e_size*sizeof(SYSEX_ENTRY));
		if (!events) return;
	}
	DWORD nd=d_pos+s;
	if (nd>=d_size)
	{
		do {
			d_size<<=1;
		} while(nd>=d_size);
		data=(BYTE*)realloc(data,d_size);
		if (!data) return;
	}
	memcpy(data+d_pos,e,s);
	events[pos].pos=t;
	events[pos].ofs=d_pos;
	events[pos].len=s;
	d_pos=nd;
	pos++;
}

CMarkerMap* mmap_create()
{
	CMarkerMap* s=new CMarkerMap;
	if (s)
	{
		s->e_size=0x10;
		s->d_size=0x40;
		s->events=(SYSEX_ENTRY*)malloc(sizeof(SYSEX_ENTRY)*s->e_size);
		s->data=(BYTE*)malloc(s->d_size);
		s->d_pos=s->pos=0;
	}
	return s;
}


CMarkerMap::~CMarkerMap()
{
	if (data) free(data);
	if (events) free(events);
}

CMarkerMap* CMarkerMap::Translate(MIDI_file * mf)
{
	CTempoMap* tmap=mf->tmap;
	if (!events || !data || !tmap) return 0;
	CMarkerMap* nm=mmap_create();
	if (!nm) return 0;
	nm->d_size=d_size;
	nm->d_pos=d_pos;
	nm->data=(BYTE*)realloc(nm->data,nm->d_size);
	if (!nm->data) {delete nm;return 0;}
	memcpy(nm->data,data,d_pos);
	nm->e_size=e_size;
	nm->pos=pos;
	nm->events=(SYSEX_ENTRY*)realloc(nm->events,sizeof(SYSEX_ENTRY)*nm->e_size);
	if (!nm->events) {delete nm;return 0;}

	int pos_ms=0;
	int n=0;
	int cur_temp=0;
	int ntm=tmap->pos,t_pos=0;
	int p_t=0;
	int dtx = rev16(*(WORD*)(mf->data+12))*1000;
	int pos_tx=0;



	while(n<pos)
	{
		int d=events[n].pos-p_t;
		p_t=events[n].pos;
		pos_tx+=d;
		pos_ms = tick_to_ms( tmap, pos_tx, dtx );

		nm->events[n].pos=(pos_ms+128)>>8;
		nm->events[n].ofs=events[n].ofs;
		nm->events[n].len=events[n].len;
		n++;
	}
	return nm;
}

