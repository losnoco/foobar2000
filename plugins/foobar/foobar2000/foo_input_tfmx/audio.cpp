#include <stdafx.h>

#define DMA_HACK 97

static void get_sample(Hdb* hw,CTFMXSource * diz,UINT done)
{
	long nsamp;
	UINT ndur;
	Cdb* c=hw->c;

	if (hw->mixing)
	{
		if (c->WaitDMACount==1 && MulDiv(hw->pos,100,hw->slen)>DMA_HACK) c->dma_almost=1;

		//get new sample
		if (hw->pos>=hw->slen)
		{
			c->dma_almost=0;
			hw->pos=0;
			hw->sbeg=hw->SampleStart;
			hw->slen=hw->SampleLength;
			if (hw->slen<=2 || !hw->loop(diz,hw) || (c->StopNext && diz->compat==G_COMPAT_NEW && MulDiv(done,100,diz->n_samples)>30))
			{
				hw->mode=0;
				c->StopNext=0;
				hw->slen=hw->pos=0;
				hw->sbeg=(char*)diz->smplbuf;
				hw->mixing=0;
			}
		}
		if (hw->mixing)
		{
			nsamp=(hw->sbeg[hw->pos++]*(int)hw->vol)>>8;
			ndur=hw->sdur;
		}
	}
	if (!hw->mixing)
	{//silence...
		nsamp=0;
		ndur=0;
	}
	hw->cur_pos=0;
	hw->prev=hw->next;
	hw->next=nsamp;
	hw->next_pos=ndur;
}

void CTFMXSource::mix(struct Hdb *hw,int n,blip_t *b, int out[])
{

	Cdb * c=hw->c;

	UINT done=samples_done;

	UINT written;

	if ( (hw->mode&1)==0 || hw->slen<=2 || !hw->sdur)
	{
		hw->mixing=0;
	}
	else if ((hw->mode&3)==1)
	{
		hw->sbeg=hw->SampleStart;
		hw->slen=hw->SampleLength;
		hw->pos=0;
		hw->mode|=2;
		hw->mixing=1;
	}

	written=0;

	


	for ( int i = 0; i < n && hw->mixing; ++i )
	{
		written=0;

		while(written<0x10000 && hw->mixing)
		{
			UINT delta=0x10000-written;
			if (delta>=hw->next_pos-hw->cur_pos) delta=hw->next_pos-hw->cur_pos;
			written+=delta;
			hw->cur_pos+=delta;
			if (hw->cur_pos==hw->next_pos)
			{
				get_sample(hw,this,done);
				blip_add_delta( b, i * 65536 + written, hw->next - hw->prev );
			}
		}
	}

	if (n)
	{
		blip_end_frame( b, n * 65536 );

		blip_read_samples( b, out, n );

		done = n;
	}
}

void CTFMXSource::mixmem(int n)
{
//	int x;
//	S32 *y;
	int * tb,*tb1;
	tb = tbuf; tb1 = &tbuf[HALFBUFSIZE];
	if (multimode) 
	{
		if (act[4]) mix(&hdb[4],n,blip_buf[4], tb);
		if (act[5]) mix(&hdb[5],n,blip_buf[5], tb);
		if (act[6]) mix(&hdb[6],n,blip_buf[6], tb);
		if (act[7]) mix(&hdb[7],n,blip_buf[7], tb);
/*		if (tb1)
		{
			y=tb1;
			for (x=0;x<n;x++,y++)
				*y=(*y>16383)?16383:
				   (*y<-16383)?-16383:*y;
		}*/
	}
	else if (act[3]) mix(&hdb[3],n,blip_buf[3], tb);
	if (act[0]) mix(&hdb[0],n,blip_buf[0], tb);
	if (act[1]) mix(&hdb[1],n,blip_buf[1], tb1);
	if (act[2]) mix(&hdb[2],n,blip_buf[2], tb1);
}

void flush_hdb(Hdb * hw,UINT mode)
{
	if (hw->cur_pos>=hw->next_pos) return;//happens only if there is really no input after the last flush (or playback start);
	
	hw->next=hw->prev;
	hw->cur_pos=0;
	hw->next_pos=0;

}