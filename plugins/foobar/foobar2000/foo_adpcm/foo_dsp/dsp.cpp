/*
** Destop's player plugin
** Copyright (c) 2003 Destop - CZN
** w/ modifications by hcs
*/

#include <foobar2000.h>
#include "cube.h"

int get16bit(unsigned char* p)
{
	return (p[0] << 8) | p[1];
}

int get32bit(unsigned char* p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
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

long mp2round(long addr) {
	return (addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

long mp2roundup(long addr) {
	return (0x8f00+addr%0x8f00)+(addr/0x8f00*2*0x8f00);
}

// return 1 on failure, 0 on success
void InitDSPFILE(CUBEFILE * dsp, abort_callback & p_abort, headertype type) {
	unsigned char readbuf[0x100];
	dsp->ch[0].infile->seek_e( 0, p_abort );

	unsigned read = dsp->ch[0].infile->read_e( &readbuf, 0x100, p_abort );
	
	if ( type == type_spt )
	{
		if ( read < 0x4E ) throw io_result_error_data;

		dsp->ch[0].infile = dsp->ch[1].infile;
		dsp->ch[0].infile->seek_e( 0, p_abort );
		get_dspheaderspt( &dsp->ch[0].header, readbuf );
	}

	t_filesize size = dsp->ch[0].infile->get_size_e( p_abort );

	bool idsp = false;

	if ( type == type_spt ) {
		// SPT+SPD
		
		// only play single archives.
		if (get32bit(readbuf)!=1) {
			throw io_result_error_data;
		}

		dsp->ch[0].header.num_adpcm_nibbles = dsp->ch[1].header.num_adpcm_nibbles = size * 2;
		dsp->ch[0].header.num_samples = dsp->ch[1].header.num_samples = size * 14 / 8;

		dsp->NCH = 1;
		dsp->ch[0].chanstart = 0;
		dsp->ch[0].type = type_spt;
		dsp->ch[0].bps = 4;
	} else if (readbuf[0]=='R' && readbuf[1]=='S' && readbuf[2]==0x00 && readbuf[3]==0x03) {
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
	} else if (readbuf[0]=='C' && readbuf[1]=='s' && readbuf[2]=='t' && readbuf[3]=='r') {
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
	} else if (readbuf[0]==0x02 && readbuf[1]==0 && readbuf[2]==0x7d && readbuf[3]==0) {
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
	} else if (readbuf[0]==' ' && readbuf[1]=='H' && readbuf[2]=='A' && readbuf[3]=='L' && readbuf[4]=='P' &&
		readbuf[5]=='S' && readbuf[6]=='T') {
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
			};
			class offset_list : public mem_block_t<offset_info>
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
				offsets.append( offset_info( c, sample_count ) );
				dsp->ch[0].infile->seek_e( c + 4, p_abort );
				dsp->ch[0].infile->read_object_e( &sc, 8, p_abort );
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

			idsp = readbuf[0]=='I' && readbuf[1]=='D' && readbuf[2]=='S' && readbuf[3]=='P';

			if ( idsp )
			{
				get_dspheaderstd(&dsp->ch[0].header,readbuf+0xC);
				get_dspheaderstd(&dsp->ch[1].header,readbuf+0x6C);
			}
			else
			{
				get_dspheaderstd(&dsp->ch[0].header,readbuf);
				get_dspheaderstd(&dsp->ch[1].header,readbuf+0x60);
			}

			// if a valid second header (agrees with first)
			if (dsp->ch[0].header.num_adpcm_nibbles==dsp->ch[1].header.num_adpcm_nibbles &&
				dsp->ch[0].header.num_samples==dsp->ch[1].header.num_samples) {

				// stereo
				dsp->NCH=2;
				if (idsp) {
					dsp->ch[0].interleave=dsp->ch[1].interleave=0x6b40;
					dsp->ch[0].chanstart=0xcc;
					dsp->ch[1].chanstart=0x6b40+0xcc;
					dsp->ch[0].type = dsp->ch[1].type = type_idsp;
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

					//dsp->ch[0].interleave=dsp->ch[1].interleave=0x4000;
					//dsp->ch[0].chanstart=0xc0;
					//dsp->ch[1].chanstart=0xc0+0x7ea0/2;
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

					dsp->ch[1].infile->seek_e( 0, p_abort );

					dsp->ch[1].infile->read_object_e( &readbuf, 0x100, p_abort );
					
					get_dspheaderstd(&dsp->ch[1].header,readbuf);

					dsp->NCH=2;
					dsp->ch[1].interleave=0;

					dsp->ch[1].type=type_std;

					dsp->ch[1].bps=4;

					dsp->ch[1].chanstart=0x60;

				} else dsp->NCH=1; // if dual-file stereo
			} // if valid second header (standard)
		//} // if MP2 demo
	}

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

		dsp->ch[0].infile->seek_e( 0, p_abort );

		if (!memcmp(data,readbuf,28) && !memcmp(data2,readbuf+0x50,19)) {
			dsp->ch[0].offs+=0x38;
			dsp->ch[1].offs+=0x38;
		}
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
		throw io_result_error_data;
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
void fillbufferDSP(CUBESTREAM * stream, abort_callback & p_abort) {
	int i,l;
	short decodebuf[14];
	char ADPCMbuf[8];

	stream->infile->seek_e( stream->offs, p_abort );

	i=0;
	do {
		if (i==0) {
			l = stream->infile->read_e( ADPCMbuf, 8, p_abort );
			if ( ! l ) throw io_result_eof;
			if ( l < 8 ) memset( ADPCMbuf + l, 0, 8 - l );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf,stream->header.coef,&stream->hist1,&stream->hist2);
			i=14;
			stream->offs+=8;
						
			if (stream->header.loop_flag && (stream->offs-stream->chanstart+8)>=((stream->header.ea*stream->bps/8)&(~7))) {
				stream->offs=stream->chanstart+((stream->header.sa*stream->bps/8)&(~7));
				//DisplayError("loop from %08x to %08x",stream->ea,stream->offs);
				stream->infile->seek_e( stream->offs, p_abort );
			}
		}
		stream->chanbuf[stream->writeloc++]=decodebuf[14-i];
		stream->filled++;
		i--;
		if (stream->writeloc>=0x8000/8*14) stream->writeloc=0;
	} while (stream->writeloc != stream->readloc);
}

// each HALP block contains the address of the next one and the size of the current one
void fillbufferHALP(CUBEFILE * dsp, abort_callback & p_abort) {
	int c,i;
	short decodebuf1[28];
	short decodebuf2[28];
	char ADPCMbuf[16];

	i=0;
	do {
		if (i==0) {
			
			// handle HALPST headers
			if (dsp->halpsize==0) {
				if ((long)dsp->nexthalp < 0) {
					throw io_result_eof;
				}
				dsp->ch[0].offs=dsp->nexthalp+0x20;
				dsp->ch[0].infile->seek_e( dsp->nexthalp, p_abort );
				dsp->ch[0].infile->read_object_e( ADPCMbuf, 16, p_abort );
				dsp->halpsize=get32bit((unsigned char *)&ADPCMbuf+4)+1; // size to read?
				
				dsp->ch[1].offs=dsp->nexthalp+0x20+get32bit((unsigned char *)&ADPCMbuf)/2;
				dsp->nexthalp=get32bit((unsigned char *)&ADPCMbuf+8);
			}

			dsp->ch[0].infile->seek_e( dsp->ch[0].offs, p_abort );
			dsp->ch[0].infile->read_object_e( ADPCMbuf, 16, p_abort );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf1,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);
			DSPdecodebuffer((unsigned char *)&ADPCMbuf+8,decodebuf1+14,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			dsp->ch[1].infile->seek_e( dsp->ch[1].offs, p_abort );
			dsp->ch[1].infile->read_object_e( ADPCMbuf, 16, p_abort );
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
}

// interleaved files requires streams with knowledge of each other (for proper looping)
void fillbufferDSPinterleave(CUBEFILE * dsp, abort_callback & p_abort) {
	int i;
	short decodebuf1[14];
	short decodebuf2[14];
	char ADPCMbuf[8];

	i=0;
	do {
		if (i==0) {

			dsp->ch[0].infile->seek_e( dsp->ch[0].offs, p_abort );
			dsp->ch[0].infile->read_object_e( ADPCMbuf, 8, p_abort );
			DSPdecodebuffer((unsigned char *)&ADPCMbuf,decodebuf1,dsp->ch[0].header.coef,&dsp->ch[0].hist1,&dsp->ch[0].hist2);

			dsp->ch[1].infile->seek_e( dsp->ch[1].offs, p_abort );
			dsp->ch[1].infile->read_object_e( ADPCMbuf, 8, p_abort );
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
}

void fillbuffers(CUBEFILE * dsp, abort_callback & p_abort) {
	if (dsp->ch[0].type==type_adp) {
		fillbufferADP(dsp, p_abort);
	} else if (dsp->ch[0].type==type_halp) {
		fillbufferHALP(dsp, p_abort);
	} else if (dsp->ch[0].interleave) {
		fillbufferDSPinterleave(dsp, p_abort);
	} else {
		fillbufferDSP(&dsp->ch[0], p_abort);
		if (dsp->NCH==2)
			fillbufferDSP(&dsp->ch[1], p_abort);
	}
}
