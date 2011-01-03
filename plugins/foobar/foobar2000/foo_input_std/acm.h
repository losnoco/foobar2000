class acm_converter
{
private:
	enum
	{
		INBUF=0x20000
	};
	HACMSTREAM hacm;
	BYTE* inbuf,*outbuf;
	UINT inbuf_s,outbuf_s,inbuf_b,outbuf_b,outbuf_start;
	ACMSTREAMHEADER ahd;
	bool initialized;
protected:
	virtual UINT acm_read_callback(void * buf,UINT size)=0;

	void acm_cleanup()
	{
		if (hacm)
		{
			if (ahd.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
			{
				ahd.cbSrcLength=inbuf_s;
				acmStreamUnprepareHeader(hacm,&ahd,0);
			}
			acmStreamClose(hacm,0);
			hacm=0;
		}
		if (inbuf) {free(inbuf);inbuf=0;}
		if (outbuf) {free(outbuf);outbuf=0;}
	}

	acm_converter()
	{
		hacm=0;
		inbuf=outbuf=0;
		inbuf_s=outbuf_s=inbuf_b=outbuf_b=outbuf_start=0;
		memset(&ahd,0,sizeof(ahd));
		initialized=0;
	}
	~acm_converter() {acm_cleanup();}

	UINT acm_read(void* buf,UINT buf_size)
	{
		if (outbuf_b>=buf_size)
		{
			if (buf) memcpy(buf,outbuf+outbuf_start,buf_size);
			outbuf_b-=buf_size;
			outbuf_start+=buf_size;
			return buf_size;
		}
		UINT ret_d=0;
		if (outbuf_b)
		{
			if (buf)
			{
				memcpy(buf,outbuf+outbuf_start,outbuf_b);
				buf=(BYTE*)buf+outbuf_b;
			}
			buf_size-=outbuf_b;
			ret_d=outbuf_b;
			outbuf_b=0;
		}
		outbuf_start=0;
		int rd=inbuf_s-inbuf_b;
		rd=acm_read_callback(inbuf+inbuf_b,rd);
		if (rd>0)
		{
			inbuf_b+=rd;
		}
		ahd.cbSrcLength=inbuf_b;
		DWORD flags=0;
		if (rd<=0) flags|=ACM_STREAMCONVERTF_END;
		if (!initialized)
		{
			initialized=1;
			flags|=ACM_STREAMCONVERTF_START;
		}
		else flags|=ACM_STREAMCONVERTF_BLOCKALIGN;
		acmStreamConvert(hacm,&ahd,flags);
		inbuf_b-=ahd.cbSrcLengthUsed;
		if (inbuf_b)
		{
			memcpy(inbuf,inbuf+ahd.cbSrcLengthUsed,inbuf_b);
		}
		outbuf_b=ahd.cbDstLengthUsed;
		outbuf_start=0;
		if (rd<=0 && buf_size>outbuf_b) buf_size=outbuf_b;
		return ret_d+acm_read(buf,buf_size);
	}

	int acm_init(WAVEFORMATEX* src,WAVEFORMATEX* dst)
	{
		if (acmStreamOpen(&hacm,0,src,dst,0,0,0,0)) goto fail;

		inbuf_s=INBUF - INBUF%src->nBlockAlign;
		outbuf_s = MulDiv(inbuf_s,dst->nAvgBytesPerSec,(src->wFormatTag==2) ? (src->nAvgBytesPerSec>>2) : src->nAvgBytesPerSec);//wFormatTag==2 => gayass ms adpcm driver, borked avgbytespersec
		if (!inbuf_s || !outbuf_s) goto fail;
		inbuf=(BYTE*)malloc(inbuf_s);
		if (!inbuf) goto fail;
		outbuf=(BYTE*)malloc(outbuf_s);
		if (!outbuf) goto fail;
		ahd.cbStruct=sizeof(ahd);
		ahd.pbSrc=inbuf;
		ahd.cbSrcLength=inbuf_s;
		ahd.pbDst=outbuf;
		ahd.cbDstLength=outbuf_s;
		if (acmStreamPrepareHeader(hacm,&ahd,0)) goto fail;
		return 1;
	fail:
		acm_cleanup();
		return 0;
	};

	void acm_flush()
	{
		outbuf_b=inbuf_b=outbuf_start=0;initialized=0;
	}
};
