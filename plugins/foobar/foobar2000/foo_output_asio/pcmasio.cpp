
/*
** foo_output_asio - ASIO Output plug-in for foobar2000
** 2003/03/04 Written by Otachan
** http://www.aikis.or.jp/~otachan/
*/

#define	STRICT

#define _WIN32_WINNT 0x400

#include <windows.h>
#include <float.h>
#include <objbase.h>

#include "WSL\WSL.h"
#include "WSL\Window.h"
#include "WSL\Control.h"
#include "WSL\Button.h"
#include "WSL\Static.h"
#include "WSL\Edit.h"
#include "WSL\UpDown.h"
#include "WSL\ComboBox.h"
#include "WSL\Dialog.h"

#include "foo_output_asio.h"
#include "ToBuff.h"
#include "pcmasio.h"

extern AsioDrivers*	asioDrivers;

extern cfg_int	cfg_device;
extern cfg_int	cfg_thread_priority;
extern cfg_int	cfg_buf_size;
extern cfg_int	cfg_shift_channels;
extern cfg_int	cfg_input_monitor;

CRITICAL_SECTION	CriticalSection;

volatile bool	EndThread;
bool	AsioPostOutput;

int		PreferredSize;
int		BuffPreferredSize;
int		BuffStart;
int		BuffEnd;
int		WriteSample;

ASIOCallbacks	AsioCallbacks;
FormatInfo_		FormatInfo;
ChannelInfo_*	ChannelInfos;
ASIOBufferInfo*	BufferInfos;

void
bufferSwitch(long index, ASIOBool directProcess)
{
	if(EndThread) return;

	::EnterCriticalSection(&CriticalSection);

	const int	WriteSample_ = WriteSample;
	int		CopySize;

	if(WriteSample_ < PreferredSize) {
		ResetOutBuff(index, WriteSample_);
		CopySize = WriteSample_;
	} else {
		CopySize = PreferredSize;
	}

	if(CopySize) {
		const int	CopyMax = BuffStart + CopySize;

		if(CopyMax <= BuffPreferredSize) {
			ToAsioBuff(index, CopySize);
		} else {
			ToAsioBuffOverRun(index, CopyMax);
		}

		BuffStart = (CopyMax >= BuffPreferredSize) ?
									CopyMax - BuffPreferredSize : CopyMax;
		WriteSample -= CopySize;
	}

	::LeaveCriticalSection(&CriticalSection);

	if(AsioPostOutput) ASIOOutputReady();
}

inline void
ResetOutBuff(int index, int WriteSample_)
{
	int		SetSize = PreferredSize - WriteSample_;

	for(UINT Idx = 0; Idx < FormatInfo.Nch; Idx++) {
		const int	Bps_b = ChannelInfos[Idx].Bps_b;

		memset(reinterpret_cast<unsigned char*>(BufferInfos[Idx].buffers[index]) +
											WriteSample_ * Bps_b, 0, SetSize * Bps_b);
	}
}

inline void
ToAsioBuff(int index, int CopySize)
{
	for(UINT Idx = 0; Idx < FormatInfo.Nch; Idx++) {
		const int	Bps_b = ChannelInfos[Idx].Bps_b;

		memcpy(	BufferInfos[Idx].buffers[index],
				ChannelInfos[Idx].Buff + BuffStart * Bps_b,
				CopySize * Bps_b);
	}
}

inline void
ToAsioBuffOverRun(int index, int CopyMax)
{
	const int	CopySize1 = BuffPreferredSize - BuffStart;
	const int	CopySize2 = CopyMax - BuffPreferredSize;

	for(UINT Idx = 0; Idx < FormatInfo.Nch; Idx++) {
		const int	Bps_b = ChannelInfos[Idx].Bps_b;
		const int	CopySize1_Bps_b = CopySize1 * Bps_b;
		unsigned char*	Buff = ChannelInfos[Idx].Buff;
		unsigned char*	OutBuff = reinterpret_cast<unsigned char*>
												(BufferInfos[Idx].buffers[index]);

		memcpy(OutBuff, Buff + BuffStart * Bps_b, CopySize1_Bps_b);
		memcpy(OutBuff + CopySize1_Bps_b, Buff, CopySize2 * Bps_b);
	}
}

void
sampleRateChanged(ASIOSampleRate sRate)
{
}

long
asioMessages(long selector, long value, void* message, double* opt)
{
	long	RetCode;

	switch(selector) {
	case kAsioSelectorSupported:
		if((value == kAsioEngineVersion) ||
				(value == kAsioResetRequest) ||
				(value == kAsioBufferSizeChange) ||
				(value == kAsioResyncRequest) ||
				(value == kAsioLatenciesChanged) ||
				(value == kAsioSupportsTimeInfo) ||
				(value == kAsioSupportsTimeCode) ||
				(value == kAsioSupportsInputMonitor)) {
			RetCode = 1;
		} else {
			RetCode = 0;
		}
		break;
	case kAsioEngineVersion:
		RetCode = 2;
		break;
	case kAsioResetRequest:
		play_control::command(play_control::PLAYBACK_STOP, 0);
		RetCode = 1;
		break;
	case kAsioBufferSizeChange:
		RetCode = 0;
		break;
	case kAsioResyncRequest:
		RetCode = 0;
		break;
	case kAsioLatenciesChanged:
		RetCode = 0;
		break;
	case kAsioSupportsTimeInfo:
		RetCode = 0;
		break;
	case kAsioSupportsTimeCode:
		RetCode = 0;
		break;
	case kAsioSupportsInputMonitor:
		RetCode = 1;
		break;
	default:
		RetCode = 0;
		break;
	}

	return RetCode;
}

ASIOTime*
bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool directProcess)
{
	return NULL;
}

PcmAsio::PcmAsio(void)
{
	InitCom = false;
	LoadDriver = false;
	InitDriver = false;
}

PcmAsio::~PcmAsio(void)
{
	CloseDriver();
}

inline bool
PcmAsio::OpenDriver(void)
{
	const int	NumDrv = asioDrivers->asioGetNumDev();

	if(NumDrv && (cfg_device < NumDrv)) {
		const int	DriverNameLen = 64;
		char	DriverName[DriverNameLen];

		if(asioDrivers->asioGetDriverName(cfg_device,
												DriverName, DriverNameLen) == 0) {
			::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			InitCom = true;

			if(asioDrivers->loadDriver(DriverName)) {
				ASIODriverInfo	DriverInfo;

				DriverInfo.asioVersion = 2;
				DriverInfo.sysRef = NULL;

				if(ASIOInit(&DriverInfo) == ASE_OK) {
					Setup();
					InitDriver = true;
				}

				LoadDriver = true;
			}
		}
	}

	return InitDriver;
}

void
PcmAsio::CloseDriver(void)
{
	if(InitDriver) {
		Stop();

		if(ChangeThreadPriority) {
			::SetThreadPriority(::GetCurrentThread(), DefaultThreadPriority);
		}

		Close();
		ASIOExit();

		delete[] CutBuff;
		delete[] ChannelInfos;
		delete[] BufferInfos;

		InitDriver = false;
	}

	if(LoadDriver) {
		asioDrivers->removeCurrentDriver();
		LoadDriver = false;
	}

	if(InitCom) {
		::CoUninitialize();
		InitCom = false;
	}
}

inline void
PcmAsio::Setup(void)
{
	First = true;

	FormatInfo.Sr = 0;
	FormatInfo.Format = -1;
	FormatInfo.Bps = 0;
	FormatInfo.Nch = 0;

	ShiftChannels = cfg_shift_channels;
	BeforeShiftChannels = 0;

	long	DeviceInputNch;
	long	DeviceOutputNch;

	ASIOGetChannels(&DeviceInputNch, &DeviceOutputNch);

	DeviceNch = DeviceOutputNch;

	CutBuff = new unsigned char[(MAX_BPS >> 3) * DeviceNch];

	long	MinSize;
	long	MaxSize;
	long	PreferredSize_;
	long	Granularity;

	ASIOGetBufferSize(&MinSize, &MaxSize, &PreferredSize_, &Granularity);

	PreferredSize = PreferredSize_;
	BuffPreferredSize = PreferredSize * BUFF_SIZE * (cfg_buf_size + 1);

	AsioPostOutput = ASIOOutputReady() == ASE_OK;

	ASIOInputMonitor	InputMonitor;

	InputMonitor.input = -1;
	InputMonitor.output = 0;
	InputMonitor.gain = 0x20000000;
	InputMonitor.state = cfg_input_monitor ? ASIOTrue : ASIOFalse;
	InputMonitor.pan = 0x3fffffff;

	ASIOFuture(kAsioSetInputMonitor, &InputMonitor);

	ChannelInfos = new ChannelInfo_[DeviceNch];

	for(UINT Idx = 0; Idx < DeviceNch; Idx++) {
		ChannelInfos[Idx].Buff = NULL;
	}

	BufferInfos = new ASIOBufferInfo[DeviceNch];
	InitBuff = false;

	HANDLE	CurrentThread = ::GetCurrentThread();

	DefaultThreadPriority = ::GetThreadPriority(CurrentThread);

	int		nPriority;

	switch(cfg_thread_priority) {
	case 1:
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case 2:
		nPriority = THREAD_PRIORITY_HIGHEST;
		break;
	case 3:
		nPriority = THREAD_PRIORITY_TIME_CRITICAL;
		break;
	default:
		nPriority = THREAD_PRIORITY_NORMAL;
		break;
	}

	if(ChangeThreadPriority = nPriority != DefaultThreadPriority) {
		::SetThreadPriority(CurrentThread, nPriority);
	}

	_controlfp(_RC_NEAR, _MCW_RC);
}

int
PcmAsio::MsgOpen(UINT sr, int format, UINT bps, UINT nch)
{
	if(InitDriver == false) {
		if(OpenDriver() == false) {
			CloseDriver();
			return 0;
		}
	}

	const bool	ChangeSr = sr != FormatInfo.Sr;
	const bool	ChangeFormat = format != FormatInfo.Format;
	const bool	ChangeBps = bps != FormatInfo.Bps;
	const UINT	org_nch = nch;

	nch += ShiftChannels;

	const bool	ChangeNch = nch != FormatInfo.Nch;

	if((ChangeSr == false) && (ChangeFormat == false) && (ChangeBps == false) &&
								(ChangeNch == false) &&
								(ShiftChannels == BeforeShiftChannels)) {
		return 1;
	}

	if(ChangeSr || ChangeNch) Stop();

	if(ChangeSr) {
		if(ASIOCanSampleRate(sr) == ASE_OK) {
			ASIOSetSampleRate(sr);
		} else {
			CloseDriver();
			return 0;
		}

		FormatInfo.Sr = sr;
	}

	if(ChangeFormat || ChangeBps) {
		if((	((format == DATA_FORMAT_LINEAR_PCM) &&
					((bps == 8) || (bps == 16) || (bps == 24) || (bps == 32))) ||
				((format == DATA_FORMAT_IEEE_FLOAT) &&
					((bps == 32) || (bps == 64)))) == false) {
			CloseDriver();
			return 0;
		}

		FormatInfo.Format = format;
		FormatInfo.Bps = bps;
		FormatInfo.Bps_b = bps >> 3;
	}

	FormatInfo.Bps_b_Nch = FormatInfo.Bps_b * org_nch;

	if(ChangeNch) {
		if(nch > DeviceNch) {
			CloseDriver();
			return 0;
		}

		Close();

		for(UINT Idx = 0; Idx < nch; Idx++) {
			ASIOChannelInfo	ChannelInfo;

			ChannelInfo.channel = Idx;
			ChannelInfo.isInput = ASIOFalse;

			ASIOGetChannelInfo(&ChannelInfo);

			int		ChannelBps_b_;

			switch(ChannelInfo.type) {
			case ASIOSTInt16LSB:
				ChannelBps_b_ = 2;
				break;
			case ASIOSTInt24LSB:
				ChannelBps_b_ = 3;
				break;
			case ASIOSTInt32LSB:
			case ASIOSTFloat32LSB:
			case ASIOSTInt32LSB16:
			case ASIOSTInt32LSB24:
				ChannelBps_b_ = 4;
				break;
			case ASIOSTFloat64LSB:
				ChannelBps_b_ = 8;
				break;
			default:
				CloseDriver();
				return 0;
			}

			ChannelInfos[Idx].Type = ChannelInfo.type;
			ChannelInfos[Idx].Bps_b = ChannelBps_b_;
			ChannelInfos[Idx].Buff =
							new unsigned char[BuffPreferredSize * ChannelBps_b_];
			ChannelInfos[Idx].ToBuffZeroFunc = SetToBuffZeroFuc(ChannelInfo.type);

			BufferInfos[Idx].isInput = ASIOFalse;
			BufferInfos[Idx].channelNum = Idx;
			BufferInfos[Idx].buffers[0] = NULL;
			BufferInfos[Idx].buffers[1] = NULL;
		}

		ASIOCreateBuffers(BufferInfos, nch, PreferredSize, &AsioCallbacks);
		InitBuff = true;

		FormatInfo.Nch = nch;
	}

	if(ChangeFormat || ChangeBps || ChangeNch) {
		for(UINT Idx = 0; Idx < nch; Idx++) {
			ChannelInfos[Idx].ToBuffFunc =
									SetToBuffFuc(ChannelInfos[Idx].Type, format, bps);
		}
	}

	BeforeShiftChannels = ShiftChannels;

	if(ChangeSr || ChangeNch) SetFlush();

	return 1;
}

void
PcmAsio::SetFlush(void)
{
	EndThread = false;
	BuffStart = 0;
	BuffEnd = 0;
	WriteSample = 0;
	CutBuffSize = 0;
	NowPause = false;
}

void
PcmAsio::Close(void)
{
	if(InitBuff) {
		ASIODisposeBuffers();
		InitBuff = false;
	}

	for(UINT Idx = 0; Idx < DeviceNch; Idx++) {
		if(ChannelInfos[Idx].Buff) {
			delete[] ChannelInfos[Idx].Buff;
			ChannelInfos[Idx].Buff = NULL;
		}
	}
}

int
PcmAsio::MsgCanWrite(void)
{
	return NowPause ? 0 : GetCanWriteSize();
}

inline int
PcmAsio::GetCanWriteSize(void)
{
	int		CanWriteSize =
					(BuffPreferredSize - WriteSample) * FormatInfo.Bps_b_Nch;

	return (CanWriteSize > CutBuffSize) ? CanWriteSize - CutBuffSize : 0;
}

int
PcmAsio::MsgWrite(int Size, unsigned char* Data)
{
	unsigned char*	AddData;

	if(CutBuffSize) {
		const int	NewSize = CutBuffSize + Size;

		AddData = new unsigned char[NewSize];

		memcpy(AddData, CutBuff, CutBuffSize);
		memcpy(AddData + CutBuffSize, Data, Size);

		Data = AddData;
		Size = NewSize;
	} else {
		AddData = NULL;
	}

	int		CutSize = Size / FormatInfo.Bps_b_Nch * FormatInfo.Bps_b_Nch;

	if((CutBuffSize = Size - CutSize) != 0) {
		memcpy(CutBuff, Data + CutSize, CutBuffSize);
	}

	int		NowReadSample = 0;

	while(EndThread == false) {
		::EnterCriticalSection(&CriticalSection);

		const int	WriteDataSize = Min(CutSize,
						(BuffPreferredSize - WriteSample) * FormatInfo.Bps_b_Nch);

		for(int Idx = 0; Idx < WriteDataSize; Idx += FormatInfo.Bps_b_Nch) {
			ToBuff(Data, NowReadSample);

			BuffEnd = (++BuffEnd == BuffPreferredSize) ? 0 : BuffEnd;
			WriteSample++;
		}

		::LeaveCriticalSection(&CriticalSection);

		if((CutSize -= WriteDataSize) == 0) break;

		Sleep(1);
	}

	if(AddData) delete[] AddData;

	return 1;
}

int
PcmAsio::MsgGetLatency(void)
{
	return WriteSample * FormatInfo.Bps_b_Nch;
}

void
PcmAsio::MsgPlay(void)
{
	if(First) {
		ASIOStart();
		First = false;
	}
}

void
PcmAsio::Stop(void)
{
	if(First == false) {
		ASIOStop();
		EndThread = true;
		First = true;
	}
}

void
PcmAsio::MsgPause(int state)
{
	if(state) {
		if(First == false) ASIOStop();
	} else {
		ASIOStart();
	}

	NowPause = state != 0;
}

int
PcmAsio::MsgFlush(void)
{
	Stop();
	SetFlush();

	return 0;
}

inline TO_BUFF_ZERO_FUNC
PcmAsio::SetToBuffZeroFuc(ASIOSampleType Type)
{
	TO_BUFF_ZERO_FUNC	ToBuffZeroFunc;

	switch(Type) {
	case ASIOSTInt16LSB:
		ToBuffZeroFunc = ToBuff_Zero_Int16LSB;
		break;
	case ASIOSTInt24LSB:
		ToBuffZeroFunc = ToBuff_Zero_Int24LSB;
		break;
	case ASIOSTInt32LSB:
		ToBuffZeroFunc = ToBuff_Zero_Int32LSB;
		break;
	case ASIOSTFloat32LSB:
		ToBuffZeroFunc = ToBuff_Zero_Float32LSB;
		break;
	case ASIOSTFloat64LSB:
		ToBuffZeroFunc = ToBuff_Zero_Float64LSB;
		break;
	case ASIOSTInt32LSB16:
		ToBuffZeroFunc = ToBuff_Zero_Int32LSB16;
		break;
	case ASIOSTInt32LSB24:
		ToBuffZeroFunc = ToBuff_Zero_Int32LSB24;
		break;
	}

	return ToBuffZeroFunc;
}

inline TO_BUFF_FUNC
PcmAsio::SetToBuffFuc(ASIOSampleType Type, int Format, UINT Bps)
{
	TO_BUFF_FUNC	ToBuffFunc;

	switch(Bps) {
	case 8:
		switch(Type) {
		case ASIOSTInt16LSB:
			ToBuffFunc = ToBuff_Int8_Int16LSB;
			break;
		case ASIOSTInt24LSB:
			ToBuffFunc = ToBuff_Int8_Int24LSB;
			break;
		case ASIOSTInt32LSB:
			ToBuffFunc = ToBuff_Int8_Int32LSB;
			break;
		case ASIOSTFloat32LSB:
			ToBuffFunc = ToBuff_Int8_Float32LSB;
			break;
		case ASIOSTFloat64LSB:
			ToBuffFunc = ToBuff_Int8_Float64LSB;
			break;
		case ASIOSTInt32LSB16:
			ToBuffFunc = ToBuff_Int8_Int32LSB16;
			break;
		case ASIOSTInt32LSB24:
			ToBuffFunc = ToBuff_Int8_Int32LSB24;
			break;
		}
		break;
	case 16:
		switch(Type) {
		case ASIOSTInt16LSB:
			ToBuffFunc = ToBuff_Int16_Int16LSB;
			break;
		case ASIOSTInt24LSB:
			ToBuffFunc = ToBuff_Int16_Int24LSB;
			break;
		case ASIOSTInt32LSB:
			ToBuffFunc = ToBuff_Int16_Int32LSB;
			break;
		case ASIOSTFloat32LSB:
			ToBuffFunc = ToBuff_Int16_Float32LSB;
			break;
		case ASIOSTFloat64LSB:
			ToBuffFunc = ToBuff_Int16_Float64LSB;
			break;
		case ASIOSTInt32LSB16:
			ToBuffFunc = ToBuff_Int16_Int32LSB16;
			break;
		case ASIOSTInt32LSB24:
			ToBuffFunc = ToBuff_Int16_Int32LSB24;
			break;
		}
		break;
	case 24:
		switch(Type) {
		case ASIOSTInt16LSB:
			ToBuffFunc = ToBuff_Int24_Int16LSB;
			break;
		case ASIOSTInt24LSB:
			ToBuffFunc = ToBuff_Int24_Int24LSB;
			break;
		case ASIOSTInt32LSB:
			ToBuffFunc = ToBuff_Int24_Int32LSB;
			break;
		case ASIOSTFloat32LSB:
			ToBuffFunc = ToBuff_Int24_Float32LSB;
			break;
		case ASIOSTFloat64LSB:
			ToBuffFunc = ToBuff_Int24_Float64LSB;
			break;
		case ASIOSTInt32LSB16:
			ToBuffFunc = ToBuff_Int24_Int32LSB16;
			break;
		case ASIOSTInt32LSB24:
			ToBuffFunc = ToBuff_Int24_Int32LSB24;
			break;
		}
		break;
	case 32:
		switch(Format) {
		case DATA_FORMAT_LINEAR_PCM:
			switch(Type) {
			case ASIOSTInt16LSB:
				ToBuffFunc = ToBuff_Int32_Int16LSB;
				break;
			case ASIOSTInt24LSB:
				ToBuffFunc = ToBuff_Int32_Int24LSB;
				break;
			case ASIOSTInt32LSB:
				ToBuffFunc = ToBuff_Int32_Int32LSB;
				break;
			case ASIOSTFloat32LSB:
				ToBuffFunc = ToBuff_Int32_Float32LSB;
				break;
			case ASIOSTFloat64LSB:
				ToBuffFunc = ToBuff_Int32_Float64LSB;
				break;
			case ASIOSTInt32LSB16:
				ToBuffFunc = ToBuff_Int32_Int32LSB16;
				break;
			case ASIOSTInt32LSB24:
				ToBuffFunc = ToBuff_Int32_Int32LSB24;
				break;
			}
			break;
		case DATA_FORMAT_IEEE_FLOAT:
			switch(Type) {
			case ASIOSTInt16LSB:
				ToBuffFunc = ToBuff_Float32_Int16LSB;
				break;
			case ASIOSTInt24LSB:
				ToBuffFunc = ToBuff_Float32_Int24LSB;
				break;
			case ASIOSTInt32LSB:
				ToBuffFunc = ToBuff_Float32_Int32LSB;
				break;
			case ASIOSTFloat32LSB:
				ToBuffFunc = ToBuff_Float32_Float32LSB;
				break;
			case ASIOSTFloat64LSB:
				ToBuffFunc = ToBuff_Float32_Float64LSB;
				break;
			case ASIOSTInt32LSB16:
				ToBuffFunc = ToBuff_Float32_Int32LSB16;
				break;
			case ASIOSTInt32LSB24:
				ToBuffFunc = ToBuff_Float32_Int32LSB24;
				break;
			}
			break;
		}
		break;
	case 64:
		switch(Type) {
		case ASIOSTInt16LSB:
			ToBuffFunc = ToBuff_Float64_Int16LSB;
			break;
		case ASIOSTInt24LSB:
			ToBuffFunc = ToBuff_Float64_Int24LSB;
			break;
		case ASIOSTInt32LSB:
			ToBuffFunc = ToBuff_Float64_Int32LSB;
			break;
		case ASIOSTFloat32LSB:
			ToBuffFunc = ToBuff_Float64_Float32LSB;
			break;
		case ASIOSTFloat64LSB:
			ToBuffFunc = ToBuff_Float64_Float64LSB;
			break;
		case ASIOSTInt32LSB16:
			ToBuffFunc = ToBuff_Float64_Int32LSB16;
			break;
		case ASIOSTInt32LSB24:
			ToBuffFunc = ToBuff_Float64_Int32LSB24;
			break;
		}
		break;
	}

	return ToBuffFunc;
}

inline void
PcmAsio::ToBuff(unsigned char* Data, int& NowReadSample)
{
	for(UINT Idx = 0; Idx < FormatInfo.Nch; Idx++) {
		if(Idx < ShiftChannels) {
			ChannelInfos[Idx].ToBuffZeroFunc(Idx);
		} else {
			ChannelInfos[Idx].ToBuffFunc(Idx, Data + NowReadSample);
			NowReadSample += FormatInfo.Bps_b;
		}
	}
}

#include "ToBuff.cpp"

