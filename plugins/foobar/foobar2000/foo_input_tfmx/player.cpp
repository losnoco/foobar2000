#include <stdafx.h>


#ifdef _DEBUG
#define DEBUG(x) OutputDebugString(_T(x))
#define NOTSUPPORTED OutputDebugString(_T("Unsupported\n"))
#else
#define DEBUG(x)
#define NOTSUPPORTED
#endif


//fprintf(stderr,"Found code %08x at step %04x in macro %02x",x.l,c->MacroStep-1,c->MacroNum)

int notevals[] = {					0x6AE,0x64E,0x5F4,0x59E,0x54D,0x501,
0x4B9,0x475,0x435,0x3F9,0x3C0,0x38C,0x358,0x32A,0x2FC,0x2D0,0x2A8,0x282,
0x25E,0x23B,0x21B,0x1FD,0x1E0,0x1C6,0x1AC,0x194,0x17D,0x168,0x154,0x140,
0x12F,0x11E,0x10E,0x0FE,0x0F0,0x0E3,0x0D6,0x0CA,0x0BF,0x0B4,0x0AA,0x0A0,
0x097,0x08F,0x087,0x07F,0x078,0x071,0x0D6,0x0CA,0x0BF,0x0B4,0x0AA,0x0A0,
0x097,0x08F,0x087,0x07F,0x078,0x071,0x0D6,0x0CA,0x0BF,0x0B4 };

#define NOTE_MAX (sizeof(notevals)/sizeof(notevals[0])-1)

int CTFMXSource::NotePort(U32 i)
{
	int rv=0;
	UNI x;
	struct Cdb *c;
	x.l=i;
#ifdef _DEBUG
	{
		TCHAR tmp[256];
		wsprintf(tmp,_T("%02x:%02x:%02x:%02x\n"),x.b.b0,x.b.b1,x.b.b2,x.b.b3);
		OutputDebugString(tmp);
	}
#endif
	c=&cdb[x.b.b2&(multimode?7:3)];
	if (x.b.b0==0xFC) { /* lock */
		c->SfxFlag=x.b.b1;
		c->SfxLockTime=x.b.b3;
		return rv;
	}
	if (c->SfxFlag) return rv;

	if (x.b.b0<0xC0) {
		if (seeking) return rv;
		c->Finetune=(int)x.b.b3;
		c->Velocity=(x.b.b2>>4)&0xF;
		c->PrevNote=c->CurrNote;
		c->CurrNote=(BYTE)x.b.b0;
#ifdef _DEBUG
//		if ((x.b.b2&3)==3)
		{
			TCHAR foo[64];
			wsprintf(foo,_T("NotePort%u %u : %u\n"),x.b.b2&3,samples_written,x.b.b0);
			OutputDebugString(foo);
		}
#endif
		rv=1;
		c->ReallyWait=1;
		c->NewStyleMacro=1;
		c->MacroPtr=macros[c->MacroNum=x.b.b1];
		c->MacroStep=0;
		c->EfxRun=0;
		c->MacroWait=0;
		c->KeyUp=1;
		c->Loop=-1;
		c->MacroRun=-1;
		c->WaitDMACount=0;
	} else if (x.b.b0<0xF0) {
//		t1w1_2 da:01:01:0f   dd:01:02:11
//	    tfc: e1 01 01 01  e4 02 00 03 t1l51: ed:01:02:03

		c->PortaReset=x.b.b1;

 		c->PortaTime=1;
		if (!c->PortaRate) c->PortaPer=c->DestPeriod;
		c->PortaRate=x.b.b3;
		c->DestPeriod=notevals[c->CurrNote=(x.b.b0&0x3F)]*(0x100+c->Finetune)>>8;
	} else switch (x.b.b0) {
	case 0xF7: /* enve */	//0f 18 81 0c
		c->EnvRate=x.b.b1;
		if ((x.b.b2&0xF0)>=0x80) //Turrican intro hack
		{
			c->EnvReset=c->EnvTime=1;
		}
		else c->EnvReset=c->EnvTime=(x.b.b2>>4)+1;
		c->EnvEndvol=x.b.b3;
		//c->NewVol=0;
		break;
	case 0xF6: /* vibr */
		c->VibTime=(c->VibReset=(x.b.b1&0xFE))>>1;
		c->VibWidth=x.b.b3;
		c->VibFlag=1; /* ?! */
		c->VibOffset=0;
		break;
	case 0xF5: /* kup^ */
		c->KeyUp=0;
		break;
	}
	return rv;
}

#define MAYBEWAIT if (!c->NewStyleMacro) {\
		c->NewStyleMacro=1;\
		break;\
	} else {\
		return;\
	}

int /*CTFMXSource::*/LoopOff(CTFMXSource * diz,struct Hdb *hw)
{
	return 1;
}


int /*CTFMXSource::*/LoopOn(CTFMXSource * diz,struct Hdb *hw)
{
	if (!hw->c) return 1;
	if (hw->c->WaitDMACount)
		if (--hw->c->WaitDMACount) return 1;

	hw->loop=::LoopOff;
	hw->c->dma_almost=0;

	return 1;
}

static void check_newvol(Cdb * c)
{
	if (c->NewVol)
	{
		c->CurVol=c->NewVol-1;//hack for 0 volume (wouldn't trigger this)
		c->NewVol=0;
	}
}

void CTFMXSource::RunMacro(Cdb * c)
{
	bool looped=0;
	UNI x;
	int a;

	c->MacroWait=0;
	loop:
	x.l=rev32(editbuf[c->MacroPtr+(c->MacroStep++)]);
	a=x.b.b0;
	x.b.b0=0;

#ifdef _DEBUG
	{
		TCHAR tmp[256];
		wsprintf(tmp,_T("RunMacro time: %u, channel: %u - %02x:%02x:%02x:%02x\n"),samples_written,c-cdb,a,x.b.b1,x.b.b2,x.b.b3);
		OutputDebugString(tmp);
	}
#endif
	switch (a) 
	{
	case 0: /* dmaoff+reset */
		DEBUG("DMAOff+Reset\n");
		c->EnvReset=0;
		c->VibReset=0;
		c->PortaRate=0;
		c->AddBeginTime=0;
		//c->ArpRun=c->SIDSize=0

		if (x.b.b2)
		{
			c->CurVol=x.b.b3;
		}
	case 0x13: /* dmaoff */

		DEBUG("DMAOff\n");
		c->hw->loop=LoopOff;
		if (!x.b.b1)
		{
			c->hw->mode=0;
			flush_hdb(c->hw,0);
			break;
		} else {
			//c->hw->mode|=4;
			c->StopNext=1;
			c->NewStyleMacro=0;
			return;
		}
	case 0x1: /* dma on */
		DEBUG("DMAOn\n");
		c->StopNext=0;
		c->dma_almost=0;
		c->EfxRun=x.b.b1;
		c->hw->mode=1;
		if (!c->NewStyleMacro) {  
			c->hw->SampleStart=(char*)&smplbuf[c->SaveAddr];
			c->hw->SampleLength=(c->SaveLen)?c->SaveLen<<1:131072;
			c->hw->sbeg=c->hw->SampleStart;
			c->hw->slen=c->hw->SampleLength;
			c->hw->pos=0;
			c->hw->mode|=2;
		}
		check_newvol(c);
		break;
	case 0x2: /* setbegin */
		DEBUG("SetBegin\n");
		c->AddBeginTime=0;
		c->SaveAddr=c->CurAddr=x.l;
		break;
	case 0x11: /* addbegin */
		DEBUG("AddBegin\n");
//			printf("AddBegin - time=%02x, delta=%04x",x.b.b1,(int)x.w.w1);
		c->AddBeginTime=c->AddBeginReset=x.b.b1;
		a=c->CurAddr+(c->AddBegin=(S16)x.w.w1);
/*		if (c->SIDSize)
			c->SIDSrcSample=c->CurAddr=a;
		else
*/			c->SaveAddr=c->CurAddr=a;
		break;
	case 0x3: /* setlen */
		DEBUG("SetLen\n");
		c->SaveLen=c->CurrLength=(U16)x.l;
		break;
	case 0x12: /* addlen */
		DEBUG("AddLen\n");
		c->CurrLength+=x.w.w1;
		a=c->CurrLength;
/*		if (c->SIDSize)
			c->SIDSrcLength=a;
		else*/
			c->SaveLen=a;
		break;
	case 0x4:
		DEBUG("Wait\n");
		c->MacroWait=x.w.w1;
		if (x.b.b1&0x01) {
			if (c->ReallyWait++)
				return;
		}
		MAYBEWAIT;
	case 0x1A:
		DEBUG("Wait on DMA\n");
		c->hw->loop=LoopOn;
		c->hw->c=c;
		c->WaitDMACount=x.w.w1+1;
		c->dma_almost=0;
//		c->MacroRun=0;
//		return;
		MAYBEWAIT;
	case 0x1C: /* note split */
		DEBUG("Splitnote\n");
		if (c->CurrNote>x.b.b1)
			c->MacroStep=x.w.w1;
		break;
	case 0x1D: /* vol split */
		DEBUG("Splitvol\n");
		if (c->CurVol>x.b.b1)
			c->MacroStep=x.w.w1;
		break;
/*
	case 0x1B: // random play
	case 0x1E: // random limit
*/
	case 0x10: /* loop key up */
		DEBUG("Loop key up\n");
		if (!c->KeyUp)
			break;
	case 0x5: /* loop */
		DEBUG("Loop\n");
		if (c->Loop--)
		{
			if (c->Loop<0) c->Loop=x.b.b1-1;// ? x.b.b1-1 : 0;
			c->MacroStep=x.w.w1;
			if (looped) return;
			looped=1;
		}
		break;
	//	MAYBEWAIT;
	case 0x7: /* stop */
		DEBUG("STOP\n");
		check_newvol(c);
		c->MacroRun=0;
		return;
	case 0xD: /* add volume */
		DEBUG("Addvolume\n");
		if (x.b.b2!=0xFE) {
			//c->CurVol=(c->Velocity*3)+(S8)x.b.b3;
			c->NewVol=(c->Velocity*3)+(S8)x.b.b3+1;
			
			break;
		}
		NOTSUPPORTED;
		break;
	case 0xE: /* set volume */
		DEBUG("Setvolume\n");
		if (x.b.b2!=0xFE) {
			c->NewVol=x.b.b3+1;
			break;
		}
		NOTSUPPORTED;
		break;
	case 0x21: /* start macro */
		DEBUG("Play macro\n");
		x.b.b0=c->CurrNote;
		x.b.b2|=c->Velocity<<4;
		NotePort(x.l);
		break;
	case 0x1F: /* set prev note */
		DEBUG("AddPrevNote\n");
		a=c->PrevNote;
		if (a<0)
		{
			a=c->CurrNote;//hack
			if (a<0) a=0;
		}
		goto SetNote;
	case 0x8:
		DEBUG("AddNote\n");
		a=c->CurrNote;
		if (a<0) a=0;
		goto SetNote;
	case 0x9:
		a=0;
		SetNote:
		a+=(char)x.b.b1;
		c->DestPeriod=(notevals[(UINT)a&0x3F]*(0x100+(int)c->Finetune+(int)(short)x.w.w1))>>8;
		if (!c->PortaRate || !c->CurPeriod) c->CurPeriod=c->DestPeriod;
		MAYBEWAIT;
	case 0x17: /* setperiod */
		DEBUG("Setperiod\n");
		c->DestPeriod=x.w.w1;
		if (!c->PortaRate || !c->CurPeriod) c->CurPeriod=x.w.w1;
		break;
	case 0xB: /* portamento */
		DEBUG("Portamento\n");
		c->PortaReset=x.b.b1;
		c->PortaTime=1;
		if (!c->PortaRate) c->PortaPer=c->DestPeriod;
		c->PortaRate=x.w.w1;
		break;
	case 0xC: /* vibrato */
		DEBUG("Vibrato\n");
		c->VibTime=(c->VibReset=x.b.b1)>>1;
		c->VibWidth=x.b.b3;
		c->VibFlag=1;
		if (!c->PortaRate) {
			c->CurPeriod=c->DestPeriod;
			c->VibOffset=0;
		}
		break;
	case 0xF: /* envelope */
		DEBUG("Envelope\n");
		if (c->NewVol && x.b.b3)//hack
		{
			c->CurVol=x.b.b3;
			c->EnvEndvol=c->NewVol-1;
			c->EnvReset=c->EnvTime=x.b.b2;
			c->EnvRate=abs((int)(c->CurVol-c->EnvEndvol));
			c->NewVol=0;
		}
		else
		{
			c->EnvReset=c->EnvTime=x.b.b2;
			c->EnvRate=x.b.b1;
			c->EnvEndvol=x.b.b3;
		}
		break;
	case 0xA: /* reset */
		DEBUG("Reset efx\n");
		c->EnvReset=0;
		c->VibReset=0;
		/*c->ArpRun=c->SIDSize=0;*/
		c->PortaRate=0;
		c->AddBeginTime=0;
		break;
	case 0x14: /* wait key up */
		DEBUG("Wait key up\n");
		if (!c->KeyUp) c->Loop=0;
		if (!c->Loop) {
			c->Loop=-1;
			break;
		}
		if (c->Loop==-1)
			c->Loop=x.b.b3-1;
		else
			c->Loop--;
		c->MacroStep--;
		return;
	case 0x15: /* go sub */
		DEBUG("Gosub patt\n");
		c->ReturnPtr=(U16)c->MacroPtr;
		c->ReturnStep=c->MacroStep;
	case 0x6: /* cont */
		DEBUG("Continue\n");
		c->MacroPtr=(c->MacroNum=(U16)macros[x.b.b1]);
		c->MacroStep=x.w.w1;
		c->Loop=-1;
		break;
	case 0x16: /* return sub */
		DEBUG("Returnpatt\n");
		c->MacroPtr=c->ReturnPtr;
		c->MacroStep=c->ReturnStep;
		break;
	case 0x18: /* sampleloop */
		DEBUG("Sampleloop\n");
		c->SaveAddr+=(x.w.w1&0xFFFE);
		c->SaveLen-=x.w.w1>>1;
		c->CurrLength=c->SaveLen;
		c->CurAddr=c->SaveAddr;
		break;
	case 0x19: /* oneshot */
		DEBUG("One-shot\n");
		//c->hw->loop=LoopKill;
		c->AddBeginTime=0;
		c->SaveAddr=c->CurAddr=0;
		c->SaveLen=c->CurrLength=1;
		break;
	case 0x20: /* cue */
		DEBUG("Cue\n");
		idb.Cue[x.b.b1&0x03]=x.w.w1;
		break;
	case 0x31: /* turrican 3 title - we can safely ignore */
		break;
	default:
		NOTSUPPORTED;
		break;
/*		c->MacroRun=0;
		return;*/
	}
	goto loop;
}

void CTFMXSource::DoEffects(struct Cdb *c)
{
	int a;
	if (c->EfxRun<0) return;
	if (!c->EfxRun) {
		c->EfxRun=1;
		return;
	}
	if (c->AddBeginTime) {
		c->CurAddr+=c->AddBegin;
/*		if (c->SIDSize)
			c->SIDSrcSample=c->CurAddr;
		else
*/		
		c->AddBeginTime--;
		if (!c->AddBeginTime) {
			c->AddBegin=-c->AddBegin;
			c->AddBeginTime=c->AddBeginReset;
		}
		c->SaveAddr=c->CurAddr;


	}
/*
	if (c->SIDSize) {
		fputs("SID not supported\n",stderr);
		c->SIDSize=0;
	}
*/
	if (c->VibReset) {
		a=(c->VibOffset+=c->VibWidth);
		a=(c->DestPeriod*(0x800+a))>>11;
		if (!c->PortaRate) c->CurPeriod=a;
		if (!(--c->VibTime)) {
			c->VibTime=c->VibReset;
			c->VibWidth=-c->VibWidth;
		}
	}
	if ((c->PortaRate)&&((--c->PortaTime)==0)) {
		c->PortaTime=c->PortaReset;
		if (c->PortaPer>c->DestPeriod) {
			if (compat == G_COMPAT_OLD)
				a=c->PortaPer-c->PortaRate;
			else
				a=(c->PortaPer*(256-c->PortaRate))>>8;

			if (a<=c->DestPeriod)
				c->PortaRate=0;
		} else if (c->PortaPer<c->DestPeriod) {
			if (compat == G_COMPAT_OLD)
				a=c->PortaPer+c->PortaRate;
			else a=(c->PortaPer*(256+c->PortaRate))>>8;
			if (a>=c->DestPeriod)
				c->PortaRate=0;
		} else c->PortaRate=0;
		if (!c->PortaRate)
			a=c->DestPeriod;
		c->PortaPer=c->CurPeriod=a;
	}




	if ((c->EnvReset)&&(!(c->EnvTime--)))
	{
		c->EnvTime=c->EnvReset;
		if (c->CurVol > c->EnvEndvol) {
			if (c->CurVol<c->EnvRate) c->EnvReset=0; else
			c->CurVol -= c->EnvRate;
			if (c->EnvEndvol > c->CurVol)
				c->EnvReset=0;
		} else if (c->CurVol < c->EnvEndvol) {
			c->CurVol += c->EnvRate;
			if (c->EnvEndvol < c->CurVol)
				c->EnvReset=0;
		}
		else c->EnvReset=0;

		if (!c->EnvReset) {
				c->EnvReset=c->EnvTime=0;
				c->CurVol=c->EnvEndvol;
		}

	}
/*	if (c->ArpRun) {
		fputs("Arpeggio/randomplay not supported\n",stderr);
		c->ArpRun=0;
	}
*/
	if ((mdb.FadeSlope)&&((--mdb.FadeTime)==0)) {
		mdb.FadeTime=mdb.FadeReset;
		mdb.MasterVol+=mdb.FadeSlope;
		if (mdb.FadeDest==mdb.MasterVol) mdb.FadeSlope=0;
	}
}

void CTFMXSource::DoMacro(int cc)
{
	if (mute_mask & (1<<cc)) return;
	struct Cdb *c=cdb+cc;

	int a;
/* locking */
	if (c->SfxLockTime>=0)
		c->SfxLockTime--;
	else
		c->SfxFlag=0;//c->SfxPriority=0;
	if (a=c->SfxCode) {
		c->SfxFlag=0;
		c->SfxCode=0;
		NotePort(a);
		c->SfxFlag=0;//c->SfxPriority;
	}
//	DEBUG(3)
//	printf("%01x:\t",cc);

	check_newvol(c);

	if (c->StopNext && c->hw->mode)
	{
		c->hw->mode=0;
		c->StopNext=0;
		flush_hdb(c->hw,0);
		//c->NewStyleMacro=1;
	}

	if (c->MacroWait)
	{
		c->MacroWait--;
	}
	else if (c->MacroRun)
	{
		if (c->WaitDMACount==1 && c->dma_almost)
		{
			c->WaitDMACount=0;
			c->dma_almost=0;
		}


		if (!c->WaitDMACount)
			RunMacro(c);
	}

	DoEffects(c);

	if (compat == G_COMPAT_EMOO) check_newvol(c);
	
	
	/* has to be here because of if(efxrun=1) */
	if (c->CurPeriod)
	{
		c->hw->sdur=MulDiv(c->CurPeriod*outRate,1<<16,3579545);
		//c->hw->delta=(3579545<<9)/(c->CurPeriod*outRate>>5);
	}
	c->hw->SampleLength=(c->SaveLen)?c->SaveLen<<1:131072;
	c->hw->SampleStart=(char*)&smplbuf[c->SaveAddr];

	c->hw->oldvol=c->hw->vol;
	c->hw->vol=MulDiv(c->CurVol,mdb.MasterVol*64,1<</*11*/3);
	
	if ((c->hw->mode&3)==1)
	{
		c->hw->sbeg=c->hw->SampleStart;
		c->hw->slen=c->hw->SampleLength;
		flush_hdb(c->hw,0);
		c->hw->oldvol=c->hw->vol;
	}
	
}

void CTFMXSource::DoAllMacros()
{
#if 1//debug hack
	DoMacro(0);
	DoMacro(1);
	DoMacro(2);
	if (multimode) {
		DoMacro(4);
		DoMacro(5);
		DoMacro(6);
		DoMacro(7);
	} /* else -- DoMacro(3) should always run so fade speed is right */
		DoMacro(3);
#else
		DoMacro(0);
#endif

}

void CTFMXSource::ChannelOff(int i)
{
	struct Cdb *c;
	c=&cdb[i&0x7];
	if (!c->SfxFlag) {
		c->hw->mode=0;
		c->AddBeginTime=c->AddBeginReset=c->MacroRun=
		/*c->SIDSize=c->ArpRun=*/0;
		c->NewStyleMacro=1;
		c->SaveAddr=0;
		c->CurVol=0;
		c->SaveLen=c->CurrLength=1;
		c->hw->loop=LoopOff;
		c->hw->c=c;
	}
}

static int sign(int x)
{
	if (x<0) return -1;
	else if (x>0) return 1;
	else return 0;
}

void CTFMXSource::DoFade(int sp,int dv)
{
	mdb.FadeDest=dv;
	if (!(mdb.FadeTime=mdb.FadeReset=sp)||(mdb.MasterVol==sp)) {
		//mdb.MasterVol=dv;
		mdb.MasterVol=dv;
		mdb.FadeSlope=0;
		return;
	}
	mdb.FadeSlope=sign(mdb.FadeDest - mdb.MasterVol);
}

void CTFMXSource::GetTrackStep()
{
	U16 *l;
	int x,y;
	loop:
/*	if (loops) {
		if ((pdb.CurrPos==pdb.FirstPos)&&(!(--loops))) {
			mdb.PlayerEnable=0;
			return;
		}
	}*/
	l=(U16 *)&editbuf[hdr.trackstart+(pdb.CurrPos*4)];
//	printf("%04x:",pdb.CurrPos);
//	for(x=0;x<8;x++) printf("%04x ",l[x]);
//	DEBUG(2) {
//		printf("tempo=%d pre=%d jif=%d",0x1B51F8/mdb.CIASave,
//		       pdb.Prescale,jiffies);
//	}
//	puts("");
	jiffies=0;
	if ((l[0])==0xEFFE) {
		switch (l[1]) {
		case 0: /* stop */
			mdb.PlayerEnable=0;
			return;
		case 1: /* loop */
/*			if (loops) {
				if (!(--loops)) {
					mdb.PlayerEnable=0;
					return;
				}
			}*/
			if (!(mdb.TrackLoop--)) {
				mdb.TrackLoop=-1;
				pdb.CurrPos++;
				goto loop;
			}
			else if (mdb.TrackLoop<0)
				mdb.TrackLoop=l[3];
			pdb.CurrPos=l[2];
			loop_cnt++;
			goto loop;
		case 2: /* speed */
			mdb.SpeedCnt=pdb.Prescale=l[2];
			if (!(l[3]&0xF200)&&(x=(l[3]&0x1FF)>0xF))
			{
				eClocks=0x1B51F8/x;
				mdb.CIASave=(U16)eClocks;
			}
			pdb.CurrPos++;
			goto loop;
		case 3: /* timeshare */
			if (!((x=l[3])&0x8000)) {
				x=((char)x)<-0x20?-0x20:(char)x;
				eClocks=(14318*(x+100))/100;
				mdb.CIASave=(U16)eClocks;
				multimode=1;
			} /* else multimode=0;*/
			pdb.CurrPos++;
			goto loop;
		case 4: /* fade */
			DoFade(l[2]&0xFF,l[3]&0xFF);
			pdb.CurrPos++;
			goto loop;
		default:
//			fprintf(stderr,"EFFE %04x in trackstep\n",
//				l[1]);
			pdb.CurrPos++;
			goto loop;
		}
	}
	else 
	{
		for (x=0;x<8;x++) 
		{
			pdb.p[x].PXpose=(int)(l[x]&0xff);
			if ((y=pdb.p[x].PNum=(l[x]>>8))<0x80)
			{
				pdb.p[x].PStep=0;
				pdb.p[x].PWait=0;
				pdb.p[x].PLoop=0xFFFF;
				pdb.p[x].PAddr=patterns[y];
			}
		}
	}
}

int CTFMXSource::DoTrack(struct Pdb *p,int pp)
{
	UNI x;
	int t;
	if (p->PNum==0xFE) {
		p->PNum++;
		ChannelOff(p->PXpose);
		return(0);
	}
	if (!p->PAddr) return(0);
	if (p->PNum>=0x90) return(0);
	if (p->PWait--) return(0);
	while(1) 
	{
		x.l=rev32(editbuf[p->PAddr+p->PStep++]);
		t=x.b.b0;
/*		DEBUG(3)
		{
			char tmp[256];
			wsprintf(tmp,"%x: %02x:%02x:%02x:%02x (%04x)\n",pp,t,x.b.b1,x.b.b2,x.b.b3,jiffies);
			OutputDebugString(tmp);
		}*/
		if (t<0xF0) {
			if ((t&0xC0)==0x80) {
				p->PWait=x.b.b3;
				x.b.b3=0;
			}
			x.b.b0=((t+p->PXpose)&0x3F);
			if ((t&0xC0)==0xC0)
				x.b.b0|=0xC0;
			NotePort(x.l);
			if ((t&0xC0)==0x80)
				return(0);
			continue;
		}
#ifdef _DEBUG
		{
			TCHAR foo[256];
			wsprintf(foo,_T("DoTrack time : %u, track: %u (%u/%x), data: %02x:%02x:%02x:%02x\n"),samples_written,pp,p->PNum,p->PAddr,x.b.b0,x.b.b1,x.b.b2,x.b.b3);
			OutputDebugString(foo);
		}
#endif
		switch (t&0xF) {
		case 15: /* NOP */
			break;
		case 0:	/* End */
			p->PNum=0xFF;
			pdb.CurrPos=(pdb.CurrPos==pdb.LastPos)?
				    pdb.FirstPos:pdb.CurrPos+1;
			GetTrackStep();
			return(1);
		case 1:
			if (!(p->PLoop)) {
				p->PLoop=0xFFFF;
				break;
			} else if (p->PLoop==0xFFFF) /* FF --'ed */
				p->PLoop=x.b.b1;
			p->PLoop--;
			p->PStep=x.w.w1;
			break;
		case 8: /* GsPt */
			p->PRoAddr=(U16)p->PAddr;
			p->PRoStep=p->PStep;
			/* fall through to... */
		case 2: /* Cont */
			p->PAddr=patterns[x.b.b1];
			p->PStep=x.w.w1;
			break;
		case 3: /* Wait */
			p->PWait=x.b.b1;
			return(0);
		case 14: /* StCu */
			mdb.PlayPattFlag=0;
		case 4: /* Stop */
			p->PNum=0xFF;
			return(0);
		case 5: /* Kup^ */
		case 6: /* Vibr */
		case 7: /* Enve */
		case 12: /* Lock */
			NotePort(x.l);
			break;
		case 9: /* RoPt */
			p->PAddr=p->PRoAddr;
			p->PStep=p->PRoStep;
			break;
		case 10: /* Fade */
			DoFade(x.b.b1,x.b.b3);
			break;
		case 13: /* Cue */
			idb.Cue[x.b.b1&0x03]=x.w.w1;
			break;

		case 11: /* PPat */
			{
				t=x.b.b2&0x07;
				BYTE num=x.b.b1;
				if (compat == G_COMPAT_OLD && num!=0xFF)
				{
					//this is crazy.... (T1 intro)
					num--;
				}
				if (num<0x80) pdb.p[t].PAddr=patterns[num];

				
				pdb.p[t].PNum=num;
				pdb.p[t].PXpose=x.b.b3;
				pdb.p[t].PStep=0;
				pdb.p[t].PWait=0;
				pdb.p[t].PLoop=0xFFFF;
			}
			break;

		}
	}
}

void CTFMXSource::DoTracks()
{
	int x;

	jiffies++;
	if (!mdb.SpeedCnt--) {
		mdb.SpeedCnt=pdb.Prescale;
		for (x=0;x<8;x++) {
			if (DoTrack(&pdb.p[x],x)) {
				x=-1;
				continue;
			}
		}
	}
}

void CTFMXSource::tfmxIrqIn()
{
	if (!mdb.PlayerEnable) return;
	if (!seeking) DoAllMacros();
	if (mdb.CurrSong>=0) DoTracks();
}

void CTFMXSource::AllOff() 
{
	int x;
	struct Cdb *c;
	mdb.PlayerEnable=0;
	for (x=0;x<8;x++) {
		c=&cdb[x];
		c->hw=&hdb[x];
		c->hw->c=c;	/* wait on dma */
		hdb[x].mode=0;
		c->MacroWait=
		c->MacroRun=c->SfxFlag=/*c->SIDSize=c->ArpRun=*/c->CurVol=
		c->SfxFlag=0;
		c->SfxCode=0;
		c->SaveAddr=0;
		c->Loop=-1;
		c->NewStyleMacro=1;
		c->SfxLockTime=-1;
		c->hw->sbeg=c->hw->SampleStart=(char*)smplbuf;
		c->hw->SampleLength=c->hw->slen=c->SaveLen=2;
		c->hw->loop=LoopOff;
	}
}

void CTFMXSource::TfmxInit()
{
	ZeroMemory(&hdb,sizeof(hdb));
	ZeroMemory(&cdb,sizeof(cdb));
	int x;
	AllOff();
	for (x=0;x<8;x++) {
		act[x]=!(mute_mask&(1<<x));
		hdb[x].c=&cdb[x];
		pdb.p[x].PNum=0xFF;
		pdb.p[x].PAddr=0;
		ChannelOff(x);
		cdb[x].CurrNote=cdb[x].PrevNote=-1;
		cdb[x].CurVol=0x40;
	}

}

void CTFMXSource::StartSong(int song, int mode)
{
	int x;
	mdb.PlayerEnable=0; /* sort of locking mechanism */
	mdb.MasterVol=0x40;
	mdb.FadeSlope=0;
	mdb.TrackLoop=-1;
	mdb.PlayPattFlag=0;
	eClocks=14318; /* assume 125bpm, NTSC timing */
	mdb.CIASave=14318;
	if (mode!=2) {
		pdb.CurrPos=pdb.FirstPos=hdr.start[song];
		pdb.LastPos=hdr.end[song];
		if ((x=hdr.tempo[song])>=0x10) {
			eClocks=0x1B51F8/x;
			mdb.CIASave=(U16)eClocks;
			pdb.Prescale=0;
			}
		else
			pdb.Prescale=x;
	}
	for (x=0;x<8;x++) {
		pdb.p[x].PAddr=0;
		pdb.p[x].PNum=0xFF;
		pdb.p[x].PXpose=0;
		pdb.p[x].PStep=0;
	}
	if (mode!=2) GetTrackStep();
	if (startPat!=-1) {
		pdb.CurrPos=pdb.FirstPos=startPat;
		GetTrackStep();
		startPat=-1;
	}
	mdb.SpeedCnt=mdb.EndFlag=0;
	mdb.PlayerEnable=1;
}

void CTFMXSource::seek_start()
{
	UINT n;
	for(n=0;n<8;n++)
	{
		cdb[n].MacroRun=0;
		hdb[n].mode=0;
		flush_hdb(&hdb[n],0);
	}
	for ( unsigned i = 0; i < 8; ++i )
	{
		blip_clear( blip_buf[i] );
	}
	seeking=1;
}