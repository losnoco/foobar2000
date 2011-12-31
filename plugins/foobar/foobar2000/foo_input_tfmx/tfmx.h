
#ifndef __TFMX_H
#define __TFMX_H

#include <foobar2000.h>

#include "tfm.h"

struct Hdr {
	char magic[10];
	char pad[6];
	char text[6][40];
	unsigned short start[32],end[32],tempo[32];
	short mute[8];
	unsigned int trackstart,pattstart,macrostart;
	char pad2[36];
};

typedef unsigned long U32;
typedef unsigned short U16;
typedef unsigned char U8;
typedef long S32;
typedef short S16;
typedef char S8;

typedef union {
	U32 l;
	struct {U16 w1,w0;} w;
	struct {U8 b3,b2,b1,b0;} b;
} UNI;

#include "player.h"

U32 rev32(U32);
U16 rev16(U16);

struct SONG
{
	pfc::string8 title;
	int id;
	int len,fade;
	int mute_mask;
	SONG() {id=0;len=fade=0;mute_mask=0;}
};

struct TAG
{
	pfc::string8 artist,album,date,comment,title;
	int default_song;
	int compat;
	pfc::ptr_list_t<SONG> songs;
	TAG() {compat = 0,default_song = 0;}
	~TAG() {songs.delete_all();}

	SONG * find_song(int idx)
	{
		unsigned n;

		for(n=0;n<songs.get_count();n++)
			if (songs[n]->id==idx) return songs[n];

		return 0;
	}

};

#define BUFSIZE  2048
#define HALFBUFSIZE (BUFSIZE/2)


class CTFMXSource
{
  public:
	int Open( service_ptr_t<file> & r, const char *url, int track, abort_callback & p_abort );
	int GetSamples( float *sample_buffer, int samples, int *srate );
	int SetPosition( double ); 
	double GetLength(int song = 0);
	int GetSongCount();
	int GetSong(int song);
	~CTFMXSource();
	CTFMXSource();

	void read_header( Hdr * hdr, service_ptr_t<file> & r, abort_callback & p_abort );

	void mixmem( int n );
	int load_tfmx( service_ptr_t<file> & r, const char * mfn, abort_callback & p_abort );

	int NotePort(U32 i);
//	int LoopOff(struct Hdb *hw);
//	int LoopOn(struct Hdb *hw);
	void RunMacro(Cdb * c);
	void DoEffects(struct Cdb *c);
	void DoMacro(int cc);
	void DoAllMacros();
	void ChannelOff(int i);
	void DoFade(int sp,int dv);
	void GetTrackStep();
	int DoTrack(struct Pdb *p,int pp);
	void DoTracks();
	void tfmxIrqIn();
	void AllOff();
	void TfmxInit();
	void StartSong(int song, int mode);

	void conv_float( float * out, int samples );
	void mix(struct Hdb *hw,int n,blip_t *b, int out[]);//mix_add
	void init_song();
	void seek_start();



	Hdb hdb[8];
	Mdb mdb;
	Cdb cdb[16];
	Pdblk pdb;
	Idb idb;

	Hdr hdr;

	int jiffies;
	int lp_l,lp_r;

	U32 multimode;

	unsigned int mlen;
	U32 editbuf[16384];
	U8 *smplbuf;
	U32 *macros;
	U32 *patterns;
//	U16 ts[512][8];

	int smplbuf_len;

	int num_ts,num_pat,num_mac;

	int songnum;
	int startPat;//=-1
//	int loops;

	int blend;

	U32 eClocks;

	char act[8];//={1,1,1,1,1,1,1,1};

	int tbuf[BUFSIZE];
	float tbuf_f[BUFSIZE];

	pfc::string8 mfn,sfn;

	int n_samples,eRem,samples_done;

	int samples_written;

	int song_lengths[16];


	bool is_tfm,has_len;
	bool seeking;
	bool track_ended;
	int song_len,fade_len;
	int outRate;
	int separation;
	int cur_song;
	TAG * tag;
	int mute_mask;
	int compat;
	int loop_cnt;

	blip_t * blip_buf[8];
};



#endif
