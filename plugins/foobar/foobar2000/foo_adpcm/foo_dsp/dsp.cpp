/*
** Destop's player plugin
** Copyright (c) 2003 Destop - CZN
** w/ modifications by hcs
*/

#include <foobar2000.h>
#include "cube.h"

static inline int get16bit( unsigned char * p )
{
	return pfc::byteswap_if_le_t( * ( ( t_uint16 * ) p ) );
}

static inline int get16bitL( unsigned char * p )
{
	return pfc::byteswap_if_be_t( * ( ( t_uint16 * ) p ) );
}

static inline int get32bit( unsigned char * p )
{
	return ( t_int32 ) pfc::byteswap_if_le_t( * ( ( t_uint32 * ) p ) );
}

static inline int get32bitL( unsigned char * p )
{
	return ( t_int32 ) pfc::byteswap_if_be_t( * ( ( t_uint32 * ) p ) );
}

// standard devkit version
void get_dspheaderstd(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf);
	dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get16bit(buf+0xC);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x10);
	dsp->ea=get32bit(buf+0x14);
	dsp->ca=get32bit(buf+0x18);
	/*DisplayError("get_dspheaderstd:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);*/
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1C+i*2);
	dsp->gain=get16bit(buf+0x3C);
	dsp->ps=get16bit(buf+0x3E);
	dsp->yn1=get16bit(buf+0x40);
	dsp->yn2=get16bit(buf+0x42);
	dsp->lps=get16bit(buf+0x44);
	dsp->lyn1=get16bit(buf+0x46);
	dsp->lyn2=get16bit(buf+0x48);
}

// SF Assault version
void get_dspheadersfa(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	buf+=0x20; // for SF Assault
	dsp->num_samples=get32bit(buf);
	dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get16bit(buf+0xC);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x10);
	dsp->ea=get32bit(buf+0x14);
	dsp->ca=get32bit(buf+0x18);
	//DisplayError("get_dspheadersfa:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1C+i*2);
	dsp->gain=get16bit(buf+0x3C);
	dsp->ps=get16bit(buf+0x3E);
	dsp->yn1=get16bit(buf+0x40);
	dsp->yn2=get16bit(buf+0x42);
	dsp->lps=get16bit(buf+0x44);
	dsp->lyn1=get16bit(buf+0x46);
	dsp->lyn2=get16bit(buf+0x48);
}

// Metroid Prime 2 version
void get_dspheadermp2(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x8);
	dsp->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp->sample_rate=get32bit(buf+0x0c);
	dsp->loop_flag=get16bit(buf+0x14);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x18);
	dsp->ea=get32bit(buf+0x1c);
	/*DisplayError("get_dspheadermp2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);*/
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 version (second channel)
void get_dspheadermp22(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x8);
	dsp->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp->sample_rate=get32bit(buf+0x0c);
	dsp->loop_flag=get16bit(buf+0x14);
	dsp->format=get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x18);
	dsp->ea=get32bit(buf+0x1c);
	/*DisplayError("get_dspheadermp22:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);*/
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x40+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// SSB:M HALPST version
void get_dspheaderhalp(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x18)*14/16; // I'm using the same as the loop endpoint...
	dsp->num_adpcm_nibbles=get32bit(buf+0x18)*2; // ditto
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->sa=get32bit(buf+0x14);
	dsp->ea=get32bit(buf+0x18);
	//DisplayError("get_dspheaderhalp:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// SSB:M HALPST version (second channel)
void get_dspheaderhalp2(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	dsp->num_samples=get32bit(buf+0x50)*14/16; // I'm using the same as the loop endpoint...
	dsp->num_adpcm_nibbles=get32bit(buf+0x50)*2; // ditto
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->sa=get32bit(buf+0x4c);
	dsp->ea=get32bit(buf+0x50);
	//DisplayError("get_dspheaderhalp2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x58+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 demo's stereo fmt
void get_dspheadermp2d(CUBEHEAD *dsp,unsigned char *buf) {
	int i;
	dsp->num_samples=get32bit(buf+0xc);
	dsp->num_adpcm_nibbles=get32bit(buf+0xc)*2;
	dsp->sample_rate=get32bit(buf+0x8);
	dsp->loop_flag=get16bit(buf+0x10);
	dsp->format=get16bit(buf+0x12);
	dsp->sa=get32bit(buf+0);
	dsp->ea=get32bit(buf+4);
	//DisplayError("get_dspheadermp2d:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x1c+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// Metroid Prime 2 demo's stereo fmt (chan 2)
void get_dspheadermp2d2(CUBEHEAD *dsp,unsigned char *buf) {
	int i;
	dsp->num_samples=get32bit(buf+0x18);
	dsp->num_adpcm_nibbles=get32bit(buf+0x18)*2;
	dsp->sample_rate=get32bit(buf+0x8);
	dsp->loop_flag=get16bit(buf+0x10);
	dsp->format=get16bit(buf+0x12);
	dsp->sa=get32bit(buf+0);
	dsp->ea=get32bit(buf+4);
	//DisplayError("get_dspheadermp2d2:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
	//	"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
	//	dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x3c+i*2);
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
}

// spt header (seperate file)
void get_dspheaderspt(CUBEHEAD *dsp,unsigned char *buf)
{
	int i;
	//dsp->num_samples=get32bit(buf);
	//dsp->num_adpcm_nibbles=get32bit(buf+4);
	dsp->sample_rate=get32bit(buf+8);
	dsp->loop_flag=get32bit(buf+4); // "type" field, 0=nonlooped ADPCM, 1=looped ADPCM
	dsp->format=0; //get16bit(buf+0xE);
	dsp->sa=get32bit(buf+0x0C);
	dsp->ea=get32bit(buf+0x10);
	//dsp->ea=get32bit(buf+0x14);
	
	dsp->ca=get32bit(buf+0x18);
	/*DisplayError("get_dspheaderspt:\nnum_samples=%li\nnum_adpcm_nibbles=%li\nsample_rate=%li\n"
		"loop_flag=%04x\nformat=%04x\nsa=%08x\nea=%08x\nca=%08x",dsp->num_samples,dsp->num_adpcm_nibbles,
		dsp->sample_rate,dsp->loop_flag,dsp->format,dsp->sa,dsp->ea,dsp->ca);*/
	for (i=0;i<16;i++)
	dsp->coef[i]=get16bit(buf+0x20+i*2);
	dsp->gain=get16bit(buf+0x40);
	dsp->ps=get16bit(buf+0x42);
	dsp->yn1=get16bit(buf+0x44);
	dsp->yn2=get16bit(buf+0x46);
	dsp->lps=get16bit(buf+0x48);
	dsp->lyn1=get16bit(buf+0x4A);
	dsp->lyn2=get16bit(buf+0x4C);
}

// ish (I_SF) header (no point in two fcns...)
void get_dspheaderish(CUBEHEAD *dsp1, CUBEHEAD *dsp2, unsigned char * buf) {
	int i;
	dsp1->sample_rate=get32bit(buf+0x08);
	dsp1->num_samples=get32bit(buf+0x0c);
	dsp1->num_adpcm_nibbles=get32bit(buf+0x10);
	dsp1->ca=get32bit(buf+0x14);
	// 0x00008000 at 0x18 might be interleave
	dsp1->loop_flag=get16bit(buf+0x1E);
	dsp1->sa=get32bit(buf+0x20);
	dsp1->ea=get32bit(buf+0x24);
	//dsp1->ea=get32bit(buf+0x28);
	memcpy(dsp2,dsp1,sizeof(CUBESTREAM));

	for (i=0;i<16;i++)
	dsp1->coef[i]=get16bit(buf+0x40+i*2);
	dsp1->ps=get16bit(buf+0x62);
	dsp1->yn1=get16bit(buf+0x64);
	dsp1->yn2=get16bit(buf+0x66);
	dsp1->lps=get16bit(buf+0x68);
	dsp1->lyn1=get16bit(buf+0x6A);
	dsp1->lyn2=get16bit(buf+0x6C);

	for (i=0;i<16;i++)
	dsp2->coef[i]=get16bit(buf+0x80+i*2);
	dsp2->ps=get16bit(buf+0xA2);
	dsp2->yn1=get16bit(buf+0xA4);
	dsp2->yn2=get16bit(buf+0xA6);
	dsp2->lps=get16bit(buf+0xA8);
	dsp2->lyn1=get16bit(buf+0xAA);
	dsp2->lyn2=get16bit(buf+0xAC);
}

// ymf

void get_dspheaderymf(CUBEHEAD * dsp, unsigned char * buf) {
	int i;
	
	//memset(dsp,0,sizeof(CUBESTREAM));
	//dsp->
	dsp->loop_flag=0;
	dsp->yn1=dsp->yn2=dsp->lyn1=dsp->lyn2=0;
	dsp->sample_rate=get32bit(buf+0x08);
	dsp->num_samples=get32bit(buf+0x3c);
	dsp->num_adpcm_nibbles=get32bit(buf+0x40);
	
	for (i=0;i<16;i++) dsp->coef[i]=get16bit(buf+0x0e+i*2);
}


// rsd (GC ADPCM)
void get_dspheaderrsd(CUBEHEAD * dsp, unsigned char * buf) {
	int i;
	
	dsp->loop_flag=0;

	dsp->sample_rate=get32bitL(buf+0x10);
	
	for (i=0;i<16;i++) dsp->coef[i]=get16bitL(buf+0x1c+2*i);
	
	//gain=get16bitL(buf+0x3c);
	dsp->ps=get16bitL(buf+0x3e);
	dsp->yn1=get16bitL(buf+0x40);
	dsp->yn2=get16bitL(buf+0x42);

	dsp->lps=get16bitL(buf+0x44);
	dsp->lyn1=get16bitL(buf+0x46);
	dsp->lyn2=get16bitL(buf+0x48);
}

// GCub
void get_dspheadergcub(CUBEHEAD * dsp1, CUBEHEAD * dsp2, unsigned char * buf) {
    int i;
    dsp1->loop_flag=dsp2->loop_flag=0;
    dsp1->sample_rate=dsp2->sample_rate=get32bit(buf+8);
    dsp1->num_adpcm_nibbles=dsp2->num_adpcm_nibbles=get32bit(buf+12)/2*2;
    dsp1->num_samples=dsp2->num_samples=get32bit(buf+12)/2*14/8;
    for (i=0;i<16;i++) dsp1->coef[i]=get16bit(buf+0x10+(i*2));
    for (i=0;i<16;i++) dsp2->coef[i]=get16bit(buf+0x30+(i*2));
}

long mp2round(long addr) {
	return (addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

long mp2roundup(long addr) {
	return (0x8f00+addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

// return 1 on failure, 0 on success
void InitDSPFILE(CUBEFILE * dsp, abort_callback & p_abort, headertype type) {
	unsigned char readbuf[0x200];
	dsp->ch[0].infile->seek( 0, p_abort );

	unsigned read = dsp->ch[0].infile->read( &readbuf, 0x200, p_abort );
	
	if ( type == type_spt )
	{
		if ( read < 0x4E ) throw exception_io_data();

		dsp->ch[0].infile = dsp->ch[1].infile;
		dsp->ch[0].infile->seek( 0, p_abort );
		get_dspheaderspt( &dsp->ch[0].header, readbuf );
	}

	t_filesize size = dsp->ch[0].infile->get_size( p_abort );

	bool idsp = false;
	bool idsp2 = false;

	if ( type == type_spt ) {
		// SPT+SPD
		
		// only play single archives.
		if (get32bit(readbuf)!=1) throw exception_io_data();

		dsp->ch[0].header.num_adpcm_nibbles = dsp->ch[1].header.num_adpcm_nibbles = size * 2;
		dsp->ch[0].header.num_samples = dsp->ch[1].header.num_samples = size * 14 / 8;

		dsp->NCH = 1;
		dsp->ch[0].chanstart = 0;
		dsp->ch[0].type = type_spt;
		dsp->ch[0].bps = 4;
	} else if ( type == type_ymf ) {
		// YMF

		get_dspheaderymf(&dsp->ch[0].header,readbuf+get32bit(readbuf+0x34));
		get_dspheaderymf(&dsp->ch[1].header,readbuf+get32bit(readbuf+0x34)+0x60);

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x20000;
		dsp->ch[0].chanstart=get32bit(readbuf+0);
		dsp->ch[1].chanstart=get32bit(readbuf+0)+dsp->ch[0].interleave;
		dsp->ch[0].bps=dsp->ch[1].bps=8;
		dsp->ch[0].type=dsp->ch[1].type=type_ymf;
	} else if ( type == type_wvs && !memcmp( "\0\0\0\x2", readbuf, 4 ) ) {
		// WVS

		int i;
		dsp->ch[1].header.sample_rate=dsp->ch[0].header.sample_rate=32000;
		dsp->NCH=get32bit(readbuf);
		dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+0xc);
		dsp->ch[0].chanstart=0x60;
		dsp->ch[1].chanstart=0x60+dsp->ch[0].interleave;

		dsp->ch[1].header.loop_flag=dsp->ch[0].header.loop_flag=0;

		dsp->ch[1].header.num_adpcm_nibbles=dsp->ch[0].header.num_adpcm_nibbles=get32bit(readbuf+0x14);
		dsp->ch[1].header.num_samples=dsp->ch[0].header.num_samples=dsp->ch[0].header.num_adpcm_nibbles*14/8;

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x18)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+(0x38)+i*2);

		dsp->ch[0].bps=dsp->ch[1].bps=8;
		dsp->ch[0].type=dsp->ch[1].type=type_wvs;
	} else if ( !memcmp( "RS\x00\x03", readbuf, 4 ) ) {
		// Metroid Prime 2 "RS03"
		if (get16bit(readbuf+6)==2) { // channel count
			get_dspheadermp2(&dsp->ch[0].header,readbuf);
			get_dspheadermp22(&dsp->ch[1].header,readbuf);

			dsp->ch[0].header.sa=mp2round(dsp->ch[0].header.sa);
			dsp->ch[1].header.sa=mp2round(dsp->ch[1].header.sa);

			dsp->ch[0].header.ea*=2;
			dsp->ch[1].header.ea*=2;

			if (size - dsp->ch[0].header.ea > 0x8f00/2)
				dsp->ch[0].header.ea=dsp->ch[1].header.ea=mp2roundup(dsp->ch[0].header.ea/2);

			dsp->NCH=2;
			dsp->ch[0].interleave=dsp->ch[1].interleave=0x8f00;

			dsp->ch[0].chanstart=0x60;
			dsp->ch[1].chanstart=0x8f60;

			dsp->ch[0].type=dsp->ch[1].type=type_mp2;
			dsp->ch[0].bps=dsp->ch[1].bps=8;
		} else {
			// mono variant, haven't seen any of these loop
			get_dspheadermp2(&dsp->ch[0].header,readbuf);
			
			dsp->ch[0].header.sa*=2;
			
			dsp->NCH=1;
			dsp->ch[0].interleave=0;

			dsp->ch[0].chanstart=0x60;

			dsp->ch[0].type=type_mp2;
			dsp->ch[0].bps=dsp->ch[1].bps=4;
		}
	} else if ( !memcmp( "Cstr", readbuf, 4 ) ) {
		// Star Fox Assault "Cstr"
		get_dspheadersfa(&dsp->ch[0].header,readbuf);
		get_dspheadersfa(&dsp->ch[1].header,readbuf+0x60);

		// weird but needed
		dsp->ch[0].header.sa*=2;

		// second header has odd values for sa and ea
		dsp->ch[1].header.ea=dsp->ch[0].header.ea;
		dsp->ch[1].header.sa=dsp->ch[0].header.sa;

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x800;

		dsp->ch[0].chanstart=0xe0;
		dsp->ch[1].chanstart=0x8e0;

		dsp->ch[0].type=dsp->ch[1].type=type_sfass;
		dsp->ch[0].bps=dsp->ch[1].bps=8;
	} else if ( !memcmp( "\x02\x00", readbuf, 2 ) && !memcmp( readbuf + 2, readbuf + 0x4a, 2) ) {
		// Paper Mario 2 "STM"
		get_dspheaderstd(&dsp->ch[0].header,readbuf+0x40);
		get_dspheaderstd(&dsp->ch[1].header,readbuf+0xa0);
		
		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0;

		dsp->ch[0].chanstart=0x100;
		dsp->ch[1].chanstart=0x100+get32bit(readbuf+8); // the offset of chan2 is in several places...

		// easy way to detect mono form
		if (dsp->ch[0].header.num_adpcm_nibbles!=dsp->ch[1].header.num_adpcm_nibbles ||
			dsp->ch[0].header.num_samples!=dsp->ch[1].header.num_samples) {
			dsp->NCH=1;
			dsp->ch[0].chanstart=0xa0;
		}

		dsp->ch[0].type=dsp->ch[1].type=type_pm2;
		dsp->ch[0].bps=dsp->ch[1].bps=4;
	} else if ( !memcmp( " HALPST", readbuf, 7) ) {
		// Super Smash Bros. Melee "HALPST"
		get_dspheaderhalp(&dsp->ch[0].header,readbuf);
		get_dspheaderhalp2(&dsp->ch[1].header,readbuf);

		dsp->NCH=2;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;
		
		dsp->ch[0].chanstart=0x80;
		dsp->ch[1].chanstart=0x80;
		
		dsp->ch[0].type=dsp->ch[1].type=type_halp;
		dsp->ch[0].bps=dsp->ch[1].bps=8;

		dsp->nexthalp=0x80;
		dsp->halpsize=0;

		// determine if a HALPST file loops
		{
			struct offset_info
			{
				long offset;
				unsigned sample_count;
				offset_info( long o, unsigned s ) : offset( o ), sample_count( s ) {}
				offset_info() {}
			};
			class offset_list : public pfc::array_t<offset_info>
			{
			public:
				bool contains( long i )
				{
					for ( unsigned n = 0, s = get_size(); n < s; ++n )
					{
						if ( get_ptr()[ n ].offset == i ) return true;
					}

					return false;
				}
			};
			offset_list offsets;
			unsigned char sc[8];
			long c=0x80;
			unsigned sample_count = 0;
			while ( c >= 0 && !offsets.contains( c ) ) {
				offsets.append_single( offset_info( c, sample_count ) );
				dsp->ch[0].infile->seek( c + 4, p_abort );
				dsp->ch[0].infile->read_object( &sc, 8, p_abort );
				sample_count += get32bit( sc ) + 1;
				c = get32bit( sc + 4 );
			}
			dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=((c<0)?0:1);
			if ( c >= 0 )
			{
				dsp->ch[0].header.ea = dsp->ch[1].header.ea = MulDiv( sample_count, 28, 32 );
				for ( unsigned n = 0, s = offsets.get_size(); n < s; ++n )
				{
					offset_info & offset = offsets[ n ];
					if ( offset.offset == c )
					{
						dsp->ch[0].header.sa = dsp->ch[1].header.sa = MulDiv( offset.sample_count, 28, 32 );
						break;
					}
				}
			}
		}
	} else if ( !memcmp("I_SF ",readbuf,4) ) {
		/* moo */
		throw exception_io_data();
	} else if ( !memcmp( "RSD3GADP", readbuf, 8 ) ) {
		// RSD (ADPCM type)

		get_dspheaderrsd(&dsp->ch[0].header,readbuf);

		dsp->ch[0].chanstart=get32bitL(readbuf+0x18);

		dsp->ch[0].header.num_adpcm_nibbles=dsp->ch[1].header.num_adpcm_nibbles=
			(size-dsp->ch[0].chanstart)*2;
		dsp->ch[0].header.num_samples=dsp->ch[1].header.num_samples=
			(size-dsp->ch[0].chanstart)*7/4;

		dsp->ch[0].bps = dsp->ch[1].bps = 4;

		dsp->NCH=1;
		dsp->ch[0].interleave=0;

		dsp->ch[0].type=type_rsddsp;
	} else if ( !memcmp( "GCub", readbuf, 4 ) ) {
		// GCub

        get_dspheadergcub(&dsp->ch[0].header,&dsp->ch[1].header,readbuf);

        dsp->ch[0].chanstart=dsp->ch[1].chanstart=0x60;
        dsp->ch[0].bps=dsp->ch[1].bps=4;
        dsp->NCH=2;
        dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;


        dsp->ch[0].type=dsp->ch[1].type=type_gcub;
	} else if ( !memcmp( "RIFF", readbuf, 4 ) && !memcmp( "WAVEfmt ", readbuf + 8, 8 ) && !memcmp( "\xfe\xff", readbuf + 0x14, 2 ) ) {
		int i,l;

		dsp->NCH =get16bitL(readbuf+0x16);
		dsp->ch[0].header.sample_rate=get32bitL(readbuf+0x18);
		dsp->ch[0].chanstart=0x5c;
		dsp->ch[0].header.num_adpcm_nibbles=get32bitL(readbuf+0x2a)/dsp->NCH-0x2a;
		dsp->ch[0].header.num_samples=get32bitL(readbuf+0x2a)/dsp->NCH*14/8;
		dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=1;
		dsp->ch[1].header.ea=dsp->ch[0].header.ea=dsp->ch[0].header.num_adpcm_nibbles;
		dsp->ch[1].header.sa=dsp->ch[0].header.sa=0;

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x2e)+i*2);

		if (dsp->NCH==2) {
			unsigned char coeffs[0x20];

			dsp->ch[1].header.num_adpcm_nibbles=dsp->ch[0].header.num_adpcm_nibbles;
			dsp->ch[1].header.num_samples=dsp->ch[0].header.num_samples;
			
			dsp->ch[1].header.sample_rate=dsp->ch[0].header.sample_rate;
			dsp->ch[1].chanstart=0x58+get32bitL(readbuf+0x2a)/2+0x32;

			dsp->ch[1].infile->seek( 0x58+get32bitL(readbuf+0x2a)/2+4, p_abort );
			dsp->ch[1].infile->read( coeffs, 0x20, p_abort );

			for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(coeffs+i*2);

		}

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_wam;
	} else if ( !memcmp( "FSB3", readbuf, 4 ) && get16bitL( readbuf + 0x4A ) == 0x0608 ) {
		int i;

		dsp->NCH=2;
		dsp->ch[0].header.sample_rate=get32bitL(readbuf+0x4C);
		dsp->ch[0].chanstart=dsp->ch[1].chanstart=0xc4;
		//dsp->ch[0].num_adpcm_nibbles=get32bitL(readbuf+0x2a)/dsp->NCH-0x2a;
		dsp->ch[0].header.num_samples=get32bitL(readbuf+0x38);
		dsp->ch[1].header.ea=dsp->ch[0].header.ea=get32bitL(readbuf+0x44)/14*8; //*dsp->NCH;
		dsp->ch[1].header.sa=dsp->ch[0].header.sa=get32bitL(readbuf+0x40)/14*8;
		dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=dsp->ch[0].header.ea!=0;
		

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x68)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+(0x96)+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_fsb3wii;
	} else if ( !memcmp( "RWSD", readbuf, 4 ) && !memcmp( "DATA", readbuf + 0x20, 4 ) ) {
		int i;

		long waveOffset=0;
		long waveLength=0;
		long coefOffset=0;

		dsp->ch[0].chanstart=dsp->ch[1].chanstart=get32bit(readbuf+0x08);
		waveOffset=get32bit(readbuf+0x18);
		waveLength=get32bit(readbuf+0x1c);

		// DATA Section is useless for the moment ...
		dsp->ch[0].infile->seek( waveOffset, p_abort );
		dsp->ch[0].infile->read( &readbuf, waveLength, p_abort );
		
		dsp->NCH = get16bit(readbuf+0x1A);
		
		if (get32bit(readbuf+0x08)!=1) {
			throw exception_io_data( "RWSD Multi files not implemented yet !" );
		}

		dsp->ch[0].header.sample_rate=dsp->ch[1].header.sample_rate=get16bit(readbuf+0x14);
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x1C)/dsp->NCH*14/8;
		dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=0; //(dsp->ch[0].sa!=0);

		dsp->ch[1].chanstart+=get32bit(readbuf+0x50);

		coefOffset = get32bit(readbuf+0x38)+0x10;
		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+coefOffset+i*2);
		coefOffset = get32bit(readbuf+0x54)+0x10;
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+coefOffset+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_rwsd_wii;
	} else if ( !memcmp( "RSTM", readbuf, 4 ) ) {

		int coef_pointer,coef_pointer_2,coef_pointer_1;
		int i;

		long headOffset=0;
		long headLength=0;
		long coefOffset=0;
		long numSamples=0;
		long fileLength;
		
		headOffset=get32bit(readbuf+0x10);
		headLength=get32bit(readbuf+0x14);
		fileLength=get32bit(readbuf+0x08);

		dsp->ch[0].infile->seek( headOffset, p_abort );
		dsp->ch[0].infile->read( &readbuf, headLength, p_abort );

		// 2 = adpcm
		if (readbuf[0x20]!=2) {
			throw exception_io_data();
		}

		dsp->ch[0].chanstart=dsp->ch[1].chanstart=get32bit(readbuf+0x30);
		dsp->NCH = readbuf[0x22];

		numSamples=(fileLength-dsp->ch[0].chanstart)*14/8/dsp->NCH;
		
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x2c);
		//DisplayError("numSamples from DATA block size=%d\nnumSamples from 0x2c=%d",numSamples,get32bit(readbuf+0x2c));
		
		dsp->ch[0].header.sample_rate=dsp->ch[1].header.sample_rate=get16bit(readbuf+0x24);
		dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=readbuf[0x21]; 

		if(dsp->NCH==2) {
			dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+0x38);
			dsp->ch[1].chanstart+=dsp->ch[0].interleave;
		}

		dsp->ch[0].header.ea=dsp->ch[1].header.ea=dsp->ch[0].header.num_samples/14*8*dsp->NCH;
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get32bit(readbuf+0x28)/14*8*dsp->NCH;

		coef_pointer_1=get32bit(readbuf+0x1c);
		coef_pointer_2=get32bit(readbuf+0x10+coef_pointer_1);
		coef_pointer=coef_pointer_2+0x10;

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+coef_pointer+i*2);

		if (dsp->NCH==2) {	
			for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+coef_pointer+0x38+i*2);
		}
		
		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		/* This is a hack to get Super Paper Mario tracks playing at (apparently) the
		right speed. */
		if (type == type_spmrstm_wii && dsp->ch[0].header.sample_rate==44100) {
			dsp->ch[0].header.sample_rate=dsp->ch[1].header.sample_rate=22050;
			dsp->ch[0].type=type_spmrstm_wii;
		} else {
			dsp->ch[0].type=type_rstm_wii;
		}
	} else if ( !memcmp( "idsp", readbuf, 4) ) {
		int i;
		dsp->ch[0].chanstart=0x1c0;
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x14)-dsp->ch[0].chanstart;

		dsp->NCH = get32bit(readbuf+0xC4);
		dsp->ch[0].header.num_samples=(dsp->ch[0].header.num_samples/dsp->NCH*14/8);
		dsp->ch[0].header.sample_rate=dsp->ch[1].header.sample_rate=get32bit(readbuf+0xc8);
		
		dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+0xd8);
		dsp->ch[1].chanstart=0x1c0+dsp->ch[0].interleave;
		dsp->ch[0].header.ea=dsp->ch[1].header.ea=get32bit(readbuf+0xd4)/14/2*2*8*dsp->NCH;
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get32bit(readbuf+0xd0)/14/2*2*8*dsp->NCH;
		dsp->ch[0].header.loop_flag=dsp->ch[1].header.loop_flag=dsp->ch[0].header.ea!=0;

		if (dsp->NCH==1)
		{
			for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+0x78+i*2);
		}
		else
		{
			for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+0x118+i*2);
			for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+0x178+i*2);
		}

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_idsp_wii;
	} else if ( !memcmp( "GCA1", readbuf, 4 ) ) {

		int i;

		dsp->NCH=1;
		dsp->ch[0].header.sample_rate=get16bit(readbuf+0x2C);
		dsp->ch[0].chanstart=dsp->ch[1].chanstart=0x40;
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x2E);
		dsp->ch[0].header.ea=dsp->ch[1].header.ea=dsp->ch[0].header.num_samples/14*8;
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get16bit(readbuf+0x32)/14*8;
		dsp->ch[0].header.loop_flag = dsp->ch[1].header.loop_flag = (dsp->ch[0].header.sa!=0);

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x04)+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_gca1;
	} else if ( !memcmp( "IDSP", readbuf, 4 ) && !memcmp( "\x00\x00\x6B\x40", readbuf + 0x04, 4 ) ) {

		int i;

		dsp->NCH=get32bit(readbuf+0x24);
		dsp->ch[0].header.sample_rate=get32bit(readbuf+0x14);
		dsp->ch[0].chanstart=dsp->ch[1].chanstart=0xD4;
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x20);
		dsp->ch[0].header.ea=dsp->ch[1].header.ea=dsp->ch[0].header.num_samples;
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get32bit(readbuf+0x54)/14*8;
		dsp->ch[0].header.loop_flag = dsp->ch[1].header.loop_flag = 0; //(dsp->ch[0].sa!=0);
		dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+0x04);
		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x28)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+(0x88)+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_gca1;
	} else if ( !memcmp("THP\0", readbuf, 4 ) ) {
		throw exception_io_data();
	} else if ( type == type_zwdsp ) {
		int i;

		dsp->NCH=2;
		dsp->ch[0].header.sample_rate=get32bit(readbuf+0x8);
		dsp->ch[0].chanstart=0x90;
		dsp->ch[1].chanstart=(size-0x90)/2+0x90;
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x18)/dsp->NCH/8*14;
		dsp->ch[0].header.ea=dsp->ch[1].header.ea=get32bit(readbuf+0x14)/dsp->NCH;
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get32bit(readbuf+0x10)/dsp->NCH;
		dsp->ch[0].header.loop_flag = dsp->ch[1].header.loop_flag = (dsp->ch[0].header.ea!=0);

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x20)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+(0x60)+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_zwdsp;
	} else if ( !memcmp( "KNON", readbuf, 4 ) && !memcmp( "KAST", readbuf + 0x20, 4 ) ) {
		int i;

		dsp->NCH=get16bit(readbuf+0x64);
		dsp->ch[0].header.sample_rate=get16bit(readbuf+0x42);
		dsp->ch[0].chanstart=0x800;
		dsp->ch[1].chanstart=0x810;
		dsp->ch[0].interleave=dsp->ch[1].interleave=0x10;
		dsp->ch[0].header.num_samples=get32bit(readbuf+0x3c)/dsp->NCH/8*14;;
		dsp->ch[0].header.ea=dsp->ch[1].header.ea=get32bit(readbuf+0x48);
		dsp->ch[0].header.sa=dsp->ch[1].header.sa=get32bit(readbuf+0x44);
		dsp->ch[0].header.loop_flag = dsp->ch[1].header.loop_flag = (dsp->ch[0].header.ea!=0);

		for (i=0;i<16;i++) dsp->ch[0].header.coef[i]=get16bit(readbuf+(0x8c)+i*2);
		for (i=0;i<16;i++) dsp->ch[1].header.coef[i]=get16bit(readbuf+(0xec)+i*2);

		dsp->ch[0].bps=8;
		dsp->ch[1].bps=8;

		dsp->ch[0].type=type_knon_dsp;
	} else {
		// assume standard devkit (or other formats without signature)

		// check for MP2 demo stereo format
		/*get_dspheadermp2d(&dsp->ch[0].header,readbuf);
		get_dspheadermp2d2(&dsp->ch[1].header,readbuf);
		if (dsp->ch[0].header.num_samples==dsp->ch[1].header.num_samples) {
				DisplayError("MP2 demo");
				// stereo
				dsp->NCH=2;
				dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;

				dsp->ch[0].chanstart=0x68;
				dsp->ch[1].chanstart=0x8068;

				dsp->ch[0].type=dsp->ch[1].type=type_mp2d;
				dsp->ch[0].bps=dsp->ch[1].bps=8;
		} else { // if MP2 demo*/

			// IDSP in Mario Smash Football
			if ( !memcmp( "IDSP", readbuf, 4 ) ) idsp = true;
			// IDSP (no relation) in Harvest Moon - Another Wonderful Life
			if ( !memcmp( "\x00\x00\x00\x00\x00\x01\x00\x02", readbuf, 8 ) ) idsp2 = true;

			if ( idsp )
			{
				get_dspheaderstd(&dsp->ch[0].header,readbuf+0xC);
				get_dspheaderstd(&dsp->ch[1].header,readbuf+0x6C);
				if ( dsp->ch[0].header.sample_rate < 1000 || dsp->ch[0].header.sample_rate > 96000 )
				{
					get_dspheaderstd(&dsp->ch[0].header,readbuf+0x20);
					get_dspheaderstd(&dsp->ch[1].header,readbuf+0x80);
					dsp->NCH=2;
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x08;
					dsp->ch[0].chanstart=0xe0;
					dsp->ch[1].chanstart=0xe8;
					dsp->ch[0].type=type_std;
					goto init;
				}
			}
			else if ( idsp2 )
			{
				get_dspheaderstd(&dsp->ch[0].header,readbuf+0x40);
				get_dspheaderstd(&dsp->ch[1].header,readbuf+0xa0);
			}
			else
			{
				get_dspheaderstd(&dsp->ch[0].header,readbuf);
				get_dspheaderstd(&dsp->ch[1].header,readbuf+0x60);
			}

			// if a valid second header (agrees with first)
			if (abs((long)dsp->ch[0].header.num_adpcm_nibbles-(long)dsp->ch[1].header.num_adpcm_nibbles)<=1 &&
				abs((long)dsp->ch[0].header.num_samples-(long)dsp->ch[1].header.num_samples)<=1) {

				// stereo
				dsp->NCH=2;
				if (idsp) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+4); //0x6b40;
					dsp->ch[0].chanstart=0xcc;
					dsp->ch[1].chanstart=0xcc+dsp->ch[1].interleave;
					dsp->ch[0].type = dsp->ch[1].type = type_idsp;
				} else if ( idsp2 ) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=get32bit(readbuf+8);
					dsp->ch[0].chanstart=0x100;
					dsp->ch[1].chanstart=0x100+dsp->ch[0].interleave;
					dsp->ch[0].type = dsp->ch[1].type = type_idsp2;
				} else if (type == type_mss) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x1000;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x10c0;
					dsp->ch[0].type = dsp->ch[1].type = type;
				} else if (type == type_gcm) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x8000;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x80c0;
					dsp->ch[0].type = dsp->ch[1].type = type_std;
				} else {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x14180;
					dsp->ch[0].chanstart=0xc0;
					dsp->ch[1].chanstart=0x14180+0xc0;
					dsp->ch[0].type = dsp->ch[1].type = type_std;
				}					

 				dsp->ch[0].bps = dsp->ch[1].bps = 8;
			} else { // if valid second header (standard)
				// mono			
				dsp->ch[0].interleave=0;

				dsp->ch[0].bps=4;

				if (idsp)
				{
					dsp->ch[0].chanstart=0x6c;
					dsp->ch[0].type=type_idsp;
				}
				else
				{
					dsp->ch[0].chanstart=0x60;
					dsp->ch[0].type=type_std;
				}

				if (dsp->ch[0].infile != dsp->ch[1].infile) { // dual-file stereo
					//DisplayError("stereo");

					dsp->ch[1].infile->seek( 0, p_abort );

					dsp->ch[1].infile->read_object( &readbuf, 0x100, p_abort );
					
					get_dspheaderstd(&dsp->ch[1].header,readbuf);

					dsp->NCH=2;
					dsp->ch[1].interleave=0;

					dsp->ch[1].type=type_std;

					dsp->ch[1].bps=4;

					dsp->ch[1].chanstart=0x60;

				} else dsp->NCH=1; // if dual-file stereo

				// Single header stereo (Monopoly Party)
				if (type == type_mpdsp) {
					//DisplayError("monopoly");
					dsp->NCH=2;

					dsp->ch[0].interleave=0xf000;
					dsp->ch[0].header.num_samples/=2;
					dsp->ch[1] = dsp->ch[0];
					dsp->ch[0].chanstart=0x60;
					dsp->ch[1].chanstart=0x60+0xf000;

					dsp->ch[0].type=dsp->ch[1].type=type_mpdsp;
				}

			} // if valid second header (standard)
	}
init:
	dsp->ch[0].offs=dsp->ch[0].chanstart;
	dsp->ch[1].offs=dsp->ch[1].chanstart;
	dsp->startinterleave=dsp->ch[0].interleave;
	dsp->lastchunk=0;

	// check for Metroid Prime over-world (a special case, to remove the blip at the beginning
	// (which seems to have been an error in the source WAV))
	{
		unsigned char data[28] = {
		0x00, 0x73, 0x09, 0xBD, 0x00, 0x83, 0x78, 0xD9, 0x00, 0x00, 0x7D, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x01, 0x3A, 0x6E, 0x00, 0x83, 0x78, 0xD8, 0x00, 0x00, 0x00, 0x02
		};

		unsigned char data2[19] = {
		0xD8, 0x21, 0x13, 0x00, 0x05, 0x00, 0x00, 0x00, 0x78, 0x6F, 0xE8, 0x77, 0xD8, 0xFE, 0x12,
		0x00, 0x0C, 0x00, 0x63
		};

		dsp->ch[0].infile->seek( 0, p_abort );

		if (!memcmp(data,readbuf,28) && !memcmp(data2,readbuf+0x50,19)) {
			dsp->ch[0].offs+=0x38;
			dsp->ch[1].offs+=0x38;
		}
	}

	// Disney's Magical Mirror loop oddity (samples instead of nibbles)
	if (dsp->ch[0].type==type_std && dsp->NCH==2 && !(dsp->ch[0].header.sa&0xf) && !(dsp->ch[0].header.ea&0xf) && !(dsp->ch[1].header.sa&0xf) && !(dsp->ch[1].header.ea&0xf))
	{
		dsp->ch[0].header.sa=dsp->ch[0].header.sa*16/14;
		dsp->ch[0].header.ea=dsp->ch[0].header.ea*16/14;
		dsp->ch[1].header.sa=dsp->ch[1].header.sa*16/14;
		dsp->ch[1].header.ea=dsp->ch[1].header.ea*16/14;

		//MessageBox(NULL,"disney","yo",MB_OK);
	}

	// in case loop end offset is beyond EOF... (MMX:CM)
	//if (dsp->ch[0].header.ea*dsp->ch[0].bps/8>dsp->file_length-dsp->ch[0].chanstart) dsp->ch[0].header.ea=(dsp->file_length-dsp->ch[0].chanstart)*8/dsp->ch[0].bps;
	//if (dsp->NCH==2 && dsp->ch[1].header.ea*dsp->ch[1].bps/8>dsp->file_length-dsp->ch[1].chanstart) dsp->ch[1].header.ea=(dsp->file_length-dsp->ch[1].chanstart)*8/dsp->ch[1].bps;

	/*if (!dsp->ch[0].header.loop_flag)*/ dsp->nrsamples = dsp->ch[0].header.num_samples;
	/*else if (dsp->ch[0].interleave)
		dsp->nrsamples=(dsp->ch[0].header.sa+looptimes*(dsp->ch[0].header.ea-dsp->ch[0].header.sa))*14/(8*8/dsp->ch[0].bps)/dsp->NCH+masterfadelength*dsp->ch[0].header.sample_rate;
	else
		dsp->nrsamples=(dsp->ch[0].header.sa+looptimes*(dsp->ch[0].header.ea-dsp->ch[0].header.sa))*14/(8*8/dsp->ch[0].bps)+masterfadelength*dsp->ch[0].header.sample_rate;*/

	// I got a threshold for the abuse I'll take.
	if (dsp->ch[0].header.sample_rate<=0 || dsp->ch[0].header.sample_rate>96000) {
		throw exception_io_data();
	}

	dsp->file_length=size; //GetFileSize(dsp->ch[0].infile,NULL);
	if ( ! idsp ) dsp->file_length=(dsp->file_length+0xf)&(~0xf);

	dsp->ch[0].hist1=dsp->ch[0].header.yn1;
	dsp->ch[0].hist2=dsp->ch[0].header.yn2;
	dsp->ch[1].hist1=dsp->ch[1].header.yn1;
	dsp->ch[1].hist2=dsp->ch[1].header.yn2;

	dsp->ch[0].readloc=dsp->ch[1].readloc=dsp->ch[0].writeloc=dsp->ch[1].writeloc=0;
	dsp->ch[0].filled=dsp->ch[1].filled=0;
}

/*void CloseDSPFILE(CUBEFILE * dsp) {
	if (dsp->ch[0].infile != INVALID_HANDLE_VALUE) CloseHandle(dsp->ch[0].infile);
	if (dsp->ch[1].infile != INVALID_HANDLE_VALUE) CloseHandle(dsp->ch[1].infile);
	dsp->ch[0].infile=dsp->ch[1].infile=INVALID_HANDLE_VALUE;
}*/

// Note that for both of these I don't bother loading the loop context. Sounds great the
// way it is, why mess with a good thing?

// for noninterleaved files (mono, STM)
// also for reading two mono Metroid Prime files simultaneously as stereo
bool fillbufferDSP(CUBESTREAM * stream, abort_callback & p_abort) {
	int i,l;
	short decodebuf[14];
	char ADPCMbuf[8];

	stream->infile->seek( stream->offs, p_abort );

	i=0;
	do {
		if (i==0) {
			l = stream->infile->read( ADPCMbuf, 8, p_abort );
			if ( ! l ) return false;
			if ( l < 8 ) memset( ADPCMbuf + l, 0, 8 - l );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf,stream->header.coef,&stream->hist1,&stream->hist2);
			i=14;
			stream->offs+=8;

			if (stream->header.loop_flag && (stream->offs-stream->chanstart+8)>=((stream->header.ea*stream->bps/8)&(~7))) {
				stream->offs=stream->chanstart+((stream->header.sa*stream->bps/8)&(~7));
				//DisplayError("loop from %08x to %08x",stream->ea,stream->offs);
				stream->infile->seek( stream->offs, p_abort );
			}
		}
		stream->chanbuf[stream->writeloc++]=decodebuf[14-i];
		stream->filled++;
		i--;
		if (stream->writeloc>=0x8000/8*14) stream->writeloc=0;
	} while (stream->writeloc != stream->readloc);

	return true;
}

// each HALP block contains the address of the next one and the size of the current one
bool fillbufferHALP(CUBEFILE * dsp, abort_callback & p_abort) {
	int c,i;
	short decodebuf1[28];
	short decodebuf2[28];
	char ADPCMbuf[16];

	i=0;
	do {
		if (i==0) {
			
			// handle HALPST headers
			if (dsp->halpsize==0) {
				if ((long)dsp->nexthalp < 0) return false;
				dsp->ch[0].offs=dsp->nexthalp+0x20;
				dsp->ch[0].infile->seek( dsp->nexthalp, p_abort );
				dsp->ch[0].infile->read_object( ADPCMbuf, 16, p_abort );
				dsp->halpsize=get32bit((unsigned char *)&ADPCMbuf+4)+1; // size to read?

				dsp->ch[1].offs=dsp->nexthalp+0x20+get32bit((unsigned char *)&ADPCMbuf)/2;
				dsp->nexthalp=get32bit((unsigned char *)&ADPCMbuf+8);
			}

			dsp->ch[0].infile->seek( dsp->ch[0].offs, p_abort );
			dsp->ch[0].infile->read_object( ADPCMbuf, 16, p_abort );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf1,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);
			DSPdecodebuffer((unsigned char *)&ADPCMbuf+8,decodebuf1+14,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			dsp->ch[1].infile->seek( dsp->ch[1].offs, p_abort );
			dsp->ch[1].infile->read_object( ADPCMbuf, 16, p_abort );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf2,dsp->ch[1].header.coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);
			DSPdecodebuffer((unsigned char *)&ADPCMbuf+8,decodebuf2+14,dsp->ch[1].header.coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);

			i=28;
			c=0;
			dsp->ch[0].offs+=0x10;
			dsp->ch[1].offs+=0x10;
			
			dsp->halpsize-=0x20;
			if (dsp->halpsize<0x20) dsp->halpsize=0;
		}
		dsp->ch[0].chanbuf[dsp->ch[0].writeloc++]=decodebuf1[c];
		dsp->ch[1].chanbuf[dsp->ch[1].writeloc++]=decodebuf2[c];
		dsp->ch[0].filled++;
		dsp->ch[1].filled++;
		c++; i--;
		if (dsp->ch[0].writeloc>=0x8000/8*14) dsp->ch[0].writeloc=0;
		if (dsp->ch[1].writeloc>=0x8000/8*14) dsp->ch[1].writeloc=0;
	} while (dsp->ch[0].writeloc != dsp->ch[0].readloc);

	return true;
}

// interleaved files requires streams with knowledge of each other (for proper looping)
bool fillbufferDSPinterleave(CUBEFILE * dsp, abort_callback & p_abort) {
	int i,l;
	short decodebuf1[14];
	short decodebuf2[14];
	char ADPCMbuf[8];

	i=0;
	do {
		if (i==0) {

			dsp->ch[0].infile->seek( dsp->ch[0].offs, p_abort );
			l = dsp->ch[0].infile->read( ADPCMbuf, 8, p_abort );
			if ( l < 8 ) return false;
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf1,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			dsp->ch[1].infile->seek( dsp->ch[1].offs, p_abort );
			l = dsp->ch[1].infile->read( ADPCMbuf, 8, p_abort );
			if ( l < 8 ) return false;
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf2,dsp->ch[1].header.coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);

			i=14;
			dsp->ch[0].offs+=8;
			dsp->ch[1].offs+=8;

			// handle interleave
			if (!dsp->lastchunk && (dsp->ch[0].offs-dsp->ch[0].chanstart)%dsp->ch[0].interleave==0) {
				dsp->ch[0].offs+=dsp->ch[0].interleave;
				//if (dsp->lastchunk) DisplayError("chanstart with lastchunk\noffset=%08x",dsp->ch[0].offs);
			}
			if (!dsp->lastchunk && (dsp->ch[1].offs-dsp->ch[1].chanstart)%dsp->ch[1].interleave==0) {
				dsp->ch[1].offs+=dsp->ch[1].interleave;

				// metroid prime 2, IDSP has smaller interleave for last chunk
				if (!dsp->lastchunk &&
					(dsp->ch[0].type==type_mp2 || dsp->ch[0].type==type_idsp) && 
					dsp->ch[1].offs+dsp->ch[1].interleave>dsp->file_length) {
					
					dsp->ch[0].interleave=dsp->ch[1].interleave=
						(dsp->file_length-dsp->ch[0].offs)/2;
					dsp->ch[1].offs=dsp->ch[0].offs+dsp->ch[1].interleave;

					dsp->lastchunk=1;

					//DisplayError("smallchunk, ch[0].offs=%08x ch[0].interleave=%08x",dsp->ch[0].offs,dsp->ch[0].interleave);
				}
			}
						
			if (dsp->ch[0].header.loop_flag && (
				(dsp->ch[0].offs-dsp->ch[0].chanstart)>=dsp->ch[0].header.ea*dsp->ch[0].bps/8 ||
				(dsp->ch[1].offs-dsp->ch[0].chanstart)>=dsp->ch[1].header.ea*dsp->ch[1].bps/8 
				) ) {
			
				if (dsp->ch[0].type==type_sfass && (dsp->ch[0].header.sa/dsp->ch[0].interleave)%2 == 1) {
					dsp->ch[1].offs=dsp->ch[0].chanstart+(dsp->ch[0].header.sa&(~7))*dsp->ch[0].bps/8;
					dsp->ch[0].offs=dsp->ch[1].offs-dsp->ch[0].interleave;
				} else {
					dsp->ch[0].offs=dsp->ch[0].chanstart+(dsp->ch[0].header.sa&(~7))*dsp->ch[0].bps/8;
					dsp->ch[1].offs=dsp->ch[1].chanstart+(dsp->ch[1].header.sa&(~7))*dsp->ch[1].bps/8;
				}

				//DisplayError("loop\nch[1].offs=%08x",dsp->ch[1].offs);

				dsp->ch[0].interleave=dsp->ch[1].interleave=dsp->startinterleave;
				dsp->lastchunk=0;
			}
		}
		dsp->ch[0].chanbuf[dsp->ch[0].writeloc++]=decodebuf1[14-i];
		dsp->ch[1].chanbuf[dsp->ch[1].writeloc++]=decodebuf2[14-i];
		dsp->ch[0].filled++;
		dsp->ch[1].filled++;
		i--;
		if (dsp->ch[0].writeloc>=0x8000/8*14) dsp->ch[0].writeloc=0;
		if (dsp->ch[1].writeloc>=0x8000/8*14) dsp->ch[1].writeloc=0;
	} while (dsp->ch[0].writeloc != dsp->ch[0].readloc);

	return true;
}

bool fillbufferDSPfsb3(CUBEFILE * dsp, abort_callback & p_abort) {
	int i,l,j;
	short decodebuf1[14];
	short decodebuf2[14];
	unsigned char ADPCMbuf[8];
	unsigned char readbuf[16];

	i=0;
	do {
		if (i==0) {

			dsp->ch[0].infile->seek( dsp->ch[0].offs, p_abort );
				
			dsp->ch[0].infile->read( readbuf, 16, p_abort );
				
			for(i=0, j=0;i<8;) {
				ADPCMbuf[i++]=readbuf[j++];
				ADPCMbuf[i++]=readbuf[j++];
				j+=2;
			}
				
			DSPdecodebuffer(ADPCMbuf,decodebuf1,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			for(i=0, j=2;i<8;) {
				ADPCMbuf[i++]=readbuf[j++];
				ADPCMbuf[i++]=readbuf[j++];
				j+=2;
			}
			DSPdecodebuffer(ADPCMbuf,decodebuf2,dsp->ch[1].header.coef,&dsp->ch[1].hist1,&dsp->ch[1].hist2);

			i=14;
			dsp->ch[0].offs+=16;

			if (dsp->ch[0].header.loop_flag && (
				(dsp->ch[0].offs-dsp->ch[0].chanstart)>=dsp->ch[0].header.ea*dsp->ch[0].bps/8*2)) {
			
				dsp->ch[0].offs=dsp->ch[0].chanstart+(dsp->ch[0].header.sa&(~7))*dsp->ch[0].bps/8*2;
				dsp->ch[1].offs=dsp->ch[1].chanstart+(dsp->ch[1].header.sa&(~7))*dsp->ch[1].bps/8*2;
				
				//DisplayError("loop\nch[1].offs=%08x",dsp->ch[1].offs);
			}
		}
		dsp->ch[0].chanbuf[dsp->ch[0].writeloc++]=decodebuf1[14-i];
		dsp->ch[1].chanbuf[dsp->ch[1].writeloc++]=decodebuf2[14-i];
		dsp->ch[0].filled++;
		dsp->ch[1].filled++;
		i--;
		if (dsp->ch[0].writeloc>=0x8000/8*14) dsp->ch[0].writeloc=0;
		if (dsp->ch[1].writeloc>=0x8000/8*14) dsp->ch[1].writeloc=0;
	} while (dsp->ch[0].writeloc != dsp->ch[0].readloc);
	return true;
}

bool fillbuffers(CUBEFILE * dsp, abort_callback & p_abort) {
	switch ( dsp->ch[0].type )
	{
	case type_adp:
		return fillbufferADP(dsp, p_abort);
	case type_halp:
		return fillbufferHALP(dsp, p_abort);
	case type_fsb3wii:
		return fillbufferDSPfsb3(dsp, p_abort);
	default:
		if (dsp->ch[0].interleave) {
			return fillbufferDSPinterleave(dsp, p_abort);
		} else {
			bool ret = fillbufferDSP(&dsp->ch[0], p_abort);
			if (dsp->NCH==2)
				ret |= fillbufferDSP(&dsp->ch[1], p_abort);
			return ret;
		}
	}
}
