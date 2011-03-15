#include <stdafx.h>


int LoopOff(CTFMXSource*,struct Hdb *hw);


U32 rev32(U32 p)
{
	U32 ret = 0;
	ret |= (p & 0xFF) << 24;
	ret |= (p & 0xFF00) << 8;
	ret |= (p & 0xFF0000) >> 8;
	ret |= (p & 0xFF000000) >> 24;
	return ret;
}

U16 rev16(U16 p)
{
	U16 ret=0;
	ret |= (p & 0xFF) << 8;
	ret |= (p & 0xFF00) >> 8;
	return ret;
}

static char id_string[8]={'T','F','M','X','-','M','O','D'};

static const GUID guid_cfg_parent_tfmx = { 0xc5acdbaf, 0x352c, 0x45d2, { 0xaf, 0xbc, 0xb6, 0xb6, 0x5c, 0x9c, 0xe2, 0x98 } };

static const GUID guid_cfg_infinite = { 0x95171f76, 0x51b4, 0x4da6, { 0x93, 0xd8, 0xba, 0x98, 0x8d, 0xc, 0x4b, 0xcc } };
static const GUID guid_cfg_sample_rate = { 0x7f507203, 0x3fa4, 0x4c85, { 0xa2, 0xfc, 0x78, 0xe7, 0xa9, 0xb3, 0x71, 0x7b } };
static const GUID guid_cfg_stereo_separation = { 0x56bc36a5, 0x9559, 0x4fb4, { 0x98, 0x21, 0xc6, 0x70, 0x8f, 0xc8, 0x24, 0xfa } };
static const GUID guid_cfg_default_length = { 0x9f5f0285, 0x26d1, 0x4edf, { 0xa5, 0xa6, 0xdf, 0x8c, 0xc3, 0xbd, 0xdd, 0xd1 } };
static const GUID guid_cfg_default_fade = { 0xf594de6d, 0x8fcd, 0x4b55, { 0x83, 0x2d, 0x36, 0xbd, 0xf, 0x85, 0x66, 0xae } };

advconfig_branch_factory cfg_tfmx_parent("TFMX decoder", guid_cfg_parent_tfmx, advconfig_branch::guid_branch_playback, 0);

advconfig_checkbox_factory cfg_infinite("Loop indefinitely", guid_cfg_infinite, guid_cfg_parent_tfmx, 0, false);

advconfig_integer_factory cfg_sample_rate("Sample rate", guid_cfg_sample_rate, guid_cfg_parent_tfmx, 1, 44100, 8000, 192000 );

advconfig_integer_factory cfg_stereo_separation( "Stereo separation (percent)", guid_cfg_stereo_separation, guid_cfg_parent_tfmx, 2, 83, 0, 100 );

advconfig_integer_factory cfg_default_length( "Default length (seconds)", guid_cfg_default_length, guid_cfg_parent_tfmx, 3, 170, 1, 9999 );
advconfig_integer_factory cfg_default_fade( "Default fade time (seconds)", guid_cfg_default_fade, guid_cfg_parent_tfmx, 4, 10, 0, 99 );

BYTE get_compat(UINT mdat,UINT smpl);

static TAG* tag_create(service_ptr_t<file> & r, abort_callback & p_abort);

int CTFMXSource::load_tfmx(service_ptr_t<file> & r,const char * mfn, abort_callback & p_abort)
{
	UINT x,y,z=0;
	U16 *sh,*lg;

	UINT mdat_s = (UINT)r->get_size_ex( p_abort );

	if ( is_tfm )
	{
		char foo[8];
		r->read_object_t( foo, p_abort );
		if ( memcmp( foo, id_string, 8 ) )
		{
			return 1;
		}
		DWORD smpl_offs,tag_offs,res;
		r->read_lendian_t( smpl_offs, p_abort );
		r->read_lendian_t( tag_offs, p_abort );
		r->read_lendian_t( res, p_abort );
		read_header( &hdr, r, p_abort );

		UINT mdat_size=smpl_offs-20-sizeof(hdr);
		if (mdat_size>0x10000)
		{
			r->read_object( editbuf, 0x10000, p_abort );
			r->seek( smpl_offs, p_abort );
			x = 0x10000;
		}
		else
		{
			r->read_object( editbuf, mdat_size, p_abort );
			x = mdat_size;
		}
		x >>= 2;
		smplbuf = (BYTE*)malloc( smplbuf_len = tag_offs - smpl_offs );
		r->read_object( smplbuf, tag_offs-smpl_offs, p_abort );

		tag=tag_create( r, p_abort );
		if (tag) compat = tag->compat;
	}
	else
	{
		read_header( &hdr, r, p_abort );

		if (strncmp("TFMX-SONG",hdr.magic,9)&&
			strncmp("TFMX_SONG",hdr.magic,9)&&
			strncmp("TFMXSONG",hdr.magic,8)&&
			strncmp("tfmxsong",hdr.magic,8)) 
		{
			return 2;
		}
		x = r->read( editbuf, 0x10000, p_abort ) >> 2;
		cur_song=0;
	}

	mlen=x;

	editbuf[x]=-1;
	if (!hdr.trackstart) hdr.trackstart=0x180; else
	hdr.trackstart=(hdr.trackstart-0x200)>>2;
	if (!hdr.pattstart) hdr.pattstart=0x80; else
	hdr.pattstart=(hdr.pattstart-0x200)>>2;
	if (!hdr.macrostart) hdr.macrostart=0x100; else
	hdr.macrostart=(hdr.macrostart-0x200)>>2;
/*	if (x<136) {
		return(2);
	}*/

/*	Now that we have pointers to most everything, this would be a good time to
	fix everything we can... rev16 tracksteps, convert pointers to array
	indices, rev32 patterns and macros.  We fix the macros first, then the
	patterns, and then the tracksteps (because we have to know when the
	patterns begin to know when the tracksteps end...) */
	z=hdr.macrostart;
	macros = &editbuf[z];
	for (x=0;x<128;x++) {
		y=(rev32(editbuf[z])-0x200);
		if ((y&3)||((y>>2)>mlen)) // probably not strictly right
			break;
		editbuf[z++]=y>>2;
	}
	num_mac=x;

	z=hdr.pattstart;
	patterns = &editbuf[z];
	for (x=0;x<128;x++) {
		y=(rev32(editbuf[z])-0x200);
		if ((y&3)||((y>>2)>mlen)) 
			break;
		editbuf[z++]=y>>2;
	}
	num_pat=x;

	lg=(U16*)&editbuf[patterns[0]];
	sh=(U16*)&editbuf[hdr.trackstart];
	num_ts=(patterns[0]-hdr.trackstart)>>2;
	y=0;
	while (sh<lg)
	{
		x=rev16(*sh);
		*sh++=x;
	}	

	if (!is_tfm)
	{
		filesystem::g_open( r, sfn, filesystem::open_mode_read, p_abort );
		UINT len=(UINT)r->get_size_ex( p_abort );
		if (len==-1 || len==0)
		{
			return 1;
		}
		smplbuf=(U8*)malloc(smplbuf_len=len);
		if (!smplbuf)
		{
			return 1;
		}
		r->read_object( smplbuf, len, p_abort );
		compat=get_compat(mdat_s,len);
	}

	return 0;
}

void CTFMXSource::read_header( Hdr * hdr, service_ptr_t<file> & r, abort_callback & p_abort )
{
	r->read_object_t( hdr->magic, p_abort );
	r->read_object_t( hdr->pad, p_abort );
	r->read_object_t( hdr->text, p_abort );
	for ( unsigned i = 0; i < 32; ++i )
	{
		r->read_bendian_t( hdr->start[ i ], p_abort );
	}
	for ( unsigned i = 0; i < 32; ++i )
	{
		r->read_bendian_t( hdr->end[ i ], p_abort );
	}
	for ( unsigned i = 0; i < 32; ++i )
	{
		r->read_bendian_t( hdr->tempo[ i ], p_abort );
	}
	r->read_object_t( hdr->mute, p_abort );
	r->read_bendian_t( hdr->trackstart, p_abort );
	r->read_bendian_t( hdr->pattstart, p_abort );
	r->read_bendian_t( hdr->macrostart, p_abort );
	r->read_object_t( hdr->pad2, p_abort );
}

static bool GetSampleFile( const char * mfn, pfc::string8 & sfn, abort_callback & p_abort )
{
	const char * _p=strrchr(mfn,'.');

	char * tmp = (char*)alloca(strlen(mfn)+1);
	strcpy(tmp,mfn);

	int p=_p-mfn;
	
	if (!_stricmp(mfn+p+1,"tfx"))
	{
		strcpy(&tmp[p+1],"sam");
		if (filesystem::g_exists(tmp, p_abort))
		{
			sfn = tmp;
			return true;
		}
		else return true;
	}
	else if (!_stricmp(mfn+p+1,"mdat"))
	{
		strcpy(&tmp[p+1],"smpl");
		if (filesystem::g_exists(tmp, p_abort))
		{
			sfn = tmp;
			return true;
		}
		else return false;
	}
	else if (!memcmp(mfn+p-4,"mdat",4) || !memcmp(mfn+p-4,"MDAT",4))
	{
		memcpy(tmp+p-4,"smpl",4);
		if (filesystem::g_exists( tmp, p_abort ))
		{
			sfn = tmp;
			return true;
		}
		else return false;
	}
	else return false;
}

void CTFMXSource::init_song()
{
	has_len = false;

	if (tag)
	{
		song_len=fade_len=0;
		SONG * p_song = tag->find_song(cur_song);
		if (p_song)
		{
			song_len=MulDiv(p_song->len,outRate,1000);
			fade_len=MulDiv(p_song->fade,outRate,1000);
			has_len = song_len || fade_len;
		}
	}

	if (!has_len) {song_len = cfg_default_length.get() * outRate; fade_len = cfg_default_fade.get() * outRate;}

	has_len = !cfg_infinite.get();

	startPat=-1;
	
	eClocks=14318;

	TfmxInit();

	StartSong(cur_song,0);

	n_samples=0;
	samples_done=0;

	samples_written=0;

	lp_l=lp_r=0;

}

int CTFMXSource::Open( service_ptr_t<file> & r, const char *url, int track, abort_callback & p_abort )
{
#if defined(_DEBUG) && defined(FILELOG)
	if (!hLog)
	{
		hLog=CreateFile("c:\\tfmx.log",GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);//OPEN_ALWAYS
		//SetFilePointer(hLog,0,0,FILE_END);
	}
#endif

	outRate=cfg_sample_rate.get();
	separation = cfg_stereo_separation.get();

	mfn = url;

	if ( !stricmp_utf8( pfc::string_extension( url ), "TFM" ) )
	{
		is_tfm = true;
	}
	else
	{
		if ( !GetSampleFile( mfn, sfn, p_abort ) ) return 1;
	}

	if ( load_tfmx( r, url, p_abort ) ) return 1;

	cur_song = track;

	init_song();

	return 0;
}

void CTFMXSource::conv_float( float * out, int samples )
{
	float separation_alpha = 0.5 + (float)separation * .005;
	float inv_separation_alpha = 1.0 - separation_alpha;

	audio_math::convert_from_int32( tbuf,               samples, tbuf_f,               1 << 8 );
	audio_math::convert_from_int32( tbuf + HALFBUFSIZE, samples, tbuf_f + HALFBUFSIZE, 1 << 8 );

	memset( tbuf, 0, samples * sizeof( *tbuf ) );
	memset( tbuf + HALFBUFSIZE, 0, samples * sizeof( *tbuf ) );

	float * lc = tbuf_f;
	float * rc = tbuf_f + HALFBUFSIZE;

	for ( int i = 0; i < samples; ++i )
	{
		*out++ = *lc * separation_alpha + *rc * inv_separation_alpha;
		*out++ = *rc * separation_alpha + *lc * inv_separation_alpha;
		lc++;
		rc++;
	}
}

int CTFMXSource::GetSamples( float *sample_buffer, int samples, int *srate )
{
	if (srate) *srate=outRate;
	int wr=0;
	if (!sample_buffer)
	{
		if (!seeking)
		{
			seek_start();
		}
	}
	else seeking=0;
	while(wr<samples)
	{
		if (!n_samples || samples_done>=n_samples)
		{
			tfmxIrqIn();
			n_samples=(eClocks*(outRate>>1));
			eRem+=(n_samples%357955);
			n_samples/=357955;
			if (eRem>357955) {n_samples++;eRem-=357955;}
			samples_done=0;
		}
		int d=samples-wr;
		if (d>n_samples-samples_done) d=n_samples-samples_done;
		if (sample_buffer && d>HALFBUFSIZE) d=HALFBUFSIZE;
		if (has_len && samples_written+d>song_len+fade_len)
		{
			d=song_len+fade_len-samples_written;
			if (!d || (int)d<0)
			{
				break;
			}
		}
		if (sample_buffer)
		{
			mixmem(d);
			if (has_len && samples_written+d>song_len)//fade
			{
				int n;
				int *lc=tbuf,*rc=tbuf+HALFBUFSIZE;
				for(n=0;n<d;n++)
				{
					int ofs=samples_written+n;
					if (ofs>song_len)
					{
						int fac=fade_len-(ofs-song_len);
						*lc=MulDiv(*lc,fac,fade_len);
						*rc=MulDiv(*rc,fac,fade_len);
					}
					lc++;
					rc++;
				}
			}
			conv_float( sample_buffer + wr * 2, d );
		}

		samples_done+=d;
		wr+=d;
		samples_written+=d;
	}
	return wr;
}


int CTFMXSource::SetPosition(double pos)
{
	int new_offs = (int)(pos * (double)outRate);
	if (new_offs>samples_written)
	{
		GetSamples( 0, new_offs-samples_written, 0 );
	}
	else
	{
		init_song();
		GetSamples( 0, new_offs, 0 );
	}
	return 0;
}


double CTFMXSource::GetLength(void)
{
	return (double)(song_len + fade_len)/(double)outRate;
}

CTFMXSource::~CTFMXSource()
{
	for ( unsigned i = 0; i < 8; ++i )
	{
		blip_delete( blip_buf[i] );
	}

#if defined(_DEBUG) && defined(FILELOG)
	if (hLog && hLog!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLog);
	}
	hLog=0;
#endif

	if (tag) delete tag;
	if (smplbuf) free(smplbuf);
}

CTFMXSource::CTFMXSource()
{
	memset(&hdb,0,sizeof(hdb));
	memset(&mdb,0,sizeof(mdb));
	memset(&cdb,0,sizeof(cdb));
	memset(&pdb,0,sizeof(pdb));
	memset(&idb,0,sizeof(idb));

	memset(&hdr,0,sizeof(hdr));

	jiffies=0;
	lp_l=lp_r=0;

	multimode=0;


	mlen=0;
	memset(editbuf,0,sizeof(editbuf));
	smplbuf=0;
	macros=0;
	patterns=0;

	smplbuf_len=0;

	num_ts=num_pat=num_mac=0;

	songnum=0;
	startPat=0;

	blend=0;

	eClocks=0;

	memset(act,0,sizeof(act));

	memset(tbuf,0,sizeof(tbuf));

	n_samples=0;
	eRem=0;
	samples_done=0;
	samples_written=0;
	is_tfm=has_len=false;
	seeking=false;
	song_len=fade_len=0;
	outRate=0;
	cur_song=0;
	tag=0;
	mute_mask=0;
	compat=0;
	loop_cnt=0;

	for ( unsigned i = 0; i < 8; ++i )
	{
		blip_buf[ i ] = blip_new( HALFBUFSIZE );
		blip_set_rates( blip_buf[ i ], 65536, 1 );
	}
}




static void readstring(service_ptr_t<file> & r, pfc::string_base & out, abort_callback & p_abort )
{
	WORD w;
	r->read_lendian_t( w, p_abort );
	pfc::array_t<char> temp;
	temp.set_count( w + 1 );
	r->read_object( temp.get_ptr(), w, p_abort );
	temp[w]=0;
	out = temp.get_ptr();
}

static TAG* tag_create( service_ptr_t<file> & r, abort_callback & p_abort )
{
	TAG * tag=new TAG;

	try
	{
		WORD w=0;
		char id;

		for(;;)
		{
			r->read_object_t( id, p_abort );
			if (!id) break;
			pfc::string8 ts;
			readstring( r, ts, p_abort );

			switch( id )
			{
			case 1:
				tag->artist = ts;
				break;
			case 2:
				tag->album = ts;
				break;
			case 3:
				tag->date = ts;
				break;
			case 4:
				tag->comment = ts;
				break;
			case 5:
				tag->compat = ts[0];
				break;
			case G_ID_TITLE:
				tag->title = ts;
				break;
			default:
				break;
			}
		}

		int n_songs = 0;
		t_uint16 blah;
		r->read_lendian_t( blah, p_abort ); n_songs = blah;
		r->read_lendian_t( blah, p_abort ); tag->default_song = blah;

		if (n_songs==0)
		{
			tag->songs.add_item(new SONG);
		}
		else
		{
			int n;
			for(n=0;n<n_songs;n++)
			{
				SONG * s = new SONG;
				tag->songs.add_item(s);
				t_uint16 blah16;
				t_uint32 blah32;
				r->read_lendian_t( blah16, p_abort ); s->id = blah16;
				r->read_lendian_t( blah32, p_abort ); s->len = blah32;
				r->read_lendian_t( blah32, p_abort ); s->fade = blah32;
				char id;
				while(1)
				{
					r->read_object_t( id, p_abort );
					if (!id) break;
					pfc::string8 ts;
					readstring( r, ts, p_abort );
					switch(id)
					{
					case 1:
						s->title = ts;
						break;
					case 2:
						//					tag->songs[n].mute_mask=*(BYTE*)ts;
						break;
					default:
						break;
					}
				}
			}
		}
		if (tag->default_song>=tag->songs.get_count()) tag->default_song=0;
		return tag;
	}
	catch (...)
	{
		delete tag;
		throw;
	}
}
