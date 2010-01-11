
#include "CommonFunc.h"

#include "../../asiosdk2/common/asiosys.h"
#include "../../asiosdk2/common/asio.h"
#include "../../asiosdk2/host/asiodrivers.h"

#define	BUFF_SIZE		4
#define	MAX_BPS			64

#define	ISCALER8			0x7f
#define	ISCALER16			0x7fff
#define	ISCALER24			0x7fffff
#define	ISCALER32			0x7fffffff

#define	FSCALER8			static_cast<double>(ISCALER8)
#define	FSCALER16			static_cast<double>(ISCALER16)
#define	FSCALER24			static_cast<double>(ISCALER24)
#define	FSCALER32			static_cast<double>(ISCALER32)

void	bufferSwitch(long index, ASIOBool directProcess);
void	sampleRateChanged(ASIOSampleRate sRate);
long	asioMessages(long selector, long value, void* message, double* opt);
ASIOTime*	bufferSwitchTimeInfo(ASIOTime* timeInfo, long index,
													ASIOBool directProcess);

inline void	ResetOutBuff(int index, int WriteSample_);
inline void	ToAsioBuff(int index, int CopySize);
inline void	ToAsioBuffOverRun(int index, int CopyMax);

struct
FormatInfo_
{
	UINT	Sr;
	int		Format;
	UINT	Bps;
	UINT	Bps_b;
	UINT	Nch;
	int		Bps_b_Nch;
};

struct
ChannelInfo_
{
	ASIOSampleType	Type;
	int		Bps_b;
	unsigned char*	Buff;
	TO_BUFF_ZERO_FUNC	ToBuffZeroFunc;
	TO_BUFF_FUNC	ToBuffFunc;
};

class
PcmAsio
{
public:
	PcmAsio(void);
	~PcmAsio(void);

	void	CloseDriver(void);
	int		MsgOpen(UINT sr, int format, UINT bps, UINT nch);
	int		MsgCanWrite(void);
	int		MsgWrite(int Size, unsigned char* Data);
	int		MsgGetLatency(void);
	void	MsgPlay(void);
	void	MsgPause(int state);
	int		MsgFlush(void);

private:
	enum {
		DATA_FORMAT_LINEAR_PCM,
		DATA_FORMAT_IEEE_FLOAT,
	};
	bool	InitCom;
	bool	LoadDriver;
	bool	InitDriver;
	bool	InitBuff;
	bool	ChangeThreadPriority;
	bool	First;
	bool	NowPause;
	UINT	ShiftChannels;
	UINT	BeforeShiftChannels;
	unsigned char*	CutBuff;
	int		DefaultThreadPriority;
	int		CutBuffSize;
	UINT	DeviceNch;

	inline bool	OpenDriver(void);
	inline void	Setup(void);
	void	SetFlush(void);
	void	Close(void);
	inline int	GetCanWriteSize(void);
	void	Stop(void);

	inline TO_BUFF_ZERO_FUNC	SetToBuffZeroFuc(ASIOSampleType Type);
	inline TO_BUFF_FUNC	SetToBuffFuc(ASIOSampleType Type, int Format, UINT Bps);
	inline void	ToBuff(unsigned char* Data, int& NowReadSample);
	inline void	ToBuff_Zero(int Idx);
};

