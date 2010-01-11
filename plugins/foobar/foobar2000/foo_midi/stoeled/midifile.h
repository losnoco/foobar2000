#ifndef __midifile_h__
#define __midifile_h__

/*#include <dsound.h>
#include <dmusici.h>
#include <dmusicf.h>*/

typedef struct
{
	UINT fmt,ntrax,tix;
	UINT channels;
	const char* e_type;
	pfc::string8 copyright;
	pfc::string8 markers;
	pfc::string8 * traxnames;
	pfc::string8 * traxtext;
} MIDIINFO;

#define FLAG_INCOMPLETE 1

typedef struct tagINSDESC
{
	tagINSDESC * next;
	UINT bank_hi,bank_lo,patch,count,note_max,note_min,channels,user;
	BOOL drum;
} INSTRUMENT_DESC;

class MIDI_file
{
public:
	pfc::string8 title;
	int flags;
	int format;
	int len,tix;
	int size;
	const BYTE* data;
	MIDIINFO info;
	meta_table rmi_data;//extra RMI crap
	void * bmp_data;//RMI-style bitmap data w/o BITMAPFILEHEADER
	int bmp_size;
	int kar_track;
	CTempoMap * tmap;
	CSysexMap * smap;
	CMarkerMap * mmap;

	void GetTitle(char *buf, int maxlen);
	inline int GetLength(void) {return len;}

	static MIDI_file* Create(const void * data,int size);

	void Free() {if (--refcount==0) delete this;}
	MIDI_file * AddRef() {refcount++;return this;}

	static int HeaderTest(const void * data,int total_size);//test first 256 bytes of file

private:
	int refcount;
	MIDI_file();
	int Load(const void * data,int size);
	~MIDI_file();	
};

#define CLEAN_DM 1
#define CLEAN_1TRACK 2
#define CLEAN_NOSYSEX 4
#define CLEAN_NOTEMPO 8
#define CLEAN_DLS 16
#define CLEAN_REDUCE 32
#define CLEAN_EMIDI 64

int DoCleanUp(MIDI_file*,DWORD,void** out_data,int * out_size);
INSTRUMENT_DESC* GetInstruments(MIDI_file*,BOOL do_lsb);

#endif
