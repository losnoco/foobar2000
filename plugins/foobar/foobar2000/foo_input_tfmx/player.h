struct Hdb {
	UINT pos;
	UINT sdur;
	UINT slen,SampleLength;
	long prev,next;
	UINT cur_pos,next_pos;//interpolation
	UINT vol;
	UINT oldvol;
	S8 *sbeg,*SampleStart;
	UINT mode;
	BOOL mixing;
	int (*loop)(class CTFMXSource* diz,struct Hdb*);
	int loopcnt;
	struct Cdb *c;
};

void flush_hdb(Hdb*,UINT inter_mode);

struct Idb {
	U16 Cue[4];
};

struct Mdb {
	char	PlayerEnable;
	char	EndFlag;
	char	CurrSong;
	U16	SpeedCnt;
	U16	CIASave;
	char	SongCont,SongNum;
	U16	PlayPattFlag;
	int MasterVol,FadeDest,FadeTime,FadeReset,FadeSlope;
	S16	TrackLoop;
	U16	DMAon,DMAoff,DMAstate,DMAAllow;
};

struct Pdb {
	U32 PAddr;
	U8 PNum;
	S8 PXpose;
	U16 PLoop,PStep;
	U8 PWait;
	U16 PRoAddr,PRoStep;
};

struct Pdblk {
	U16 FirstPos,LastPos,CurrPos;
	U16 Prescale;
	struct Pdb p[8];
};
	
struct Cdb {
	S8 MacroRun,EfxRun;
	U8 NewStyleMacro;
	S8 PrevNote,CurrNote;
	U8 Velocity,Finetune;
	U8 KeyUp,ReallyWait;
	U32 MacroPtr;
	U16 MacroStep,MacroWait,MacroNum;
	S16 Loop;

	U32 CurAddr,SaveAddr;
	U16 CurrLength,SaveLen;

	U16 WaitDMACount;
	bool dma_almost,StopNext;


	U8 EnvReset,EnvTime,EnvRate;
	UINT EnvEndvol,CurVol;
	UINT NewVol;

	S16 VibOffset;
	S8 VibWidth;
	U8 VibFlag,VibReset,VibTime;

	U8 PortaReset,PortaTime;
	U16 CurPeriod,DestPeriod,PortaPer;
	S16 PortaRate;

	U8 AddBeginTime,AddBeginReset;
	U16 ReturnPtr,ReturnStep;
	S32 AddBegin;

	U8 SfxFlag;//,SfxPriority;
	S16 SfxLockTime;
	U32 SfxCode;

	struct Hdb *hw;
};
