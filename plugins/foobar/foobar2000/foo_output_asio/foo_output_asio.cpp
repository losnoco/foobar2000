
/*
** foo_output_asio - ASIO Output plug-in for foobar2000
** 2003/03/04 Written by Otachan
** http://www.aikis.or.jp/~otachan/
*/

#define	STRICT

#define _WIN32_WINNT 0x400

#include <windows.h>
#include <process.h>

#include "WSL\WSL.h"
#include "WSL\Window.h"
#include "WSL\Control.h"
#include "WSL\Button.h"
#include "WSL\Static.h"
#include "WSL\Edit.h"
#include "WSL\UpDown.h"
#include "WSL\ComboBox.h"
#include "WSL\Dialog.h"

#include "resource.h"
#include "foo_output_asio.h"
#include "ToBuff.h"
#include "pcmasio.h"

extern HINSTANCE	WSLhInstance;

extern AsioDrivers*	asioDrivers;

extern CRITICAL_SECTION	CriticalSection;

extern ASIOCallbacks	AsioCallbacks;

service_factory_single_t<initquit, initquit_output_asio>	foo;
service_factory_t<output, output_asio>	foo2;
service_factory_single_t<config, config_output_asio>	foo3;

cfg_int	cfg_device("asio_device", 0);
cfg_int	cfg_thread_priority("asio_thread_priority", 2);
cfg_int	cfg_buf_size("asio_buffer_size", 7);
cfg_int	cfg_shift_channels("asio_shift_output_channels", 0);
cfg_int	cfg_input_monitor("asio_use_direct_input_monitor", 0);

PcmAsio*	pPcmAsio;

HANDLE	hThread;
HANDLE	EventReadyThread;
HANDLE	EventDestroyThread;

void
initquit_output_asio::on_init(void)
{
	::InitializeCriticalSection(&CriticalSection);

	WSLhInstance = service_factory_base::get_my_instance();
	hThread = NULL;

	AsioCallbacks.bufferSwitch = &bufferSwitch;
	AsioCallbacks.sampleRateDidChange = &sampleRateChanged;
	AsioCallbacks.asioMessage = &asioMessages;
	AsioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

	asioDrivers = new AsioDrivers;
}

void
initquit_output_asio::on_quit(void)
{
	if(hThread) {
		::SetEvent(EventDestroyThread);

		if(::WaitForSingleObject(hThread, 5000) != WAIT_OBJECT_0) {
			if(::TerminateThread(hThread, 0)) {
				::WaitForSingleObject(hThread, 3000);
			}
		}

		::CloseHandle(hThread);
		::CloseHandle(EventDestroyThread);
	}

	delete asioDrivers;

	::DeleteCriticalSection(&CriticalSection);
}

output_asio::output_asio(void)
{
	OpenDriver = false;
}

output_asio::~output_asio(void)
{
	if(OpenDriver) ParamMsg(MSG_CLOSE_DRIVER).Call();
}

GUID
output_asio::get_guid(void)
{
	// {3A5EDE8E-840D-497c-9774-156A12FC4275}
	static const GUID guid = 
		{0x3a5ede8e, 0x840d, 0x497c, {0x97, 0x74, 0x15, 0x6a, 0x12, 0xfc, 0x42, 0x75}};

	return guid;
}

const char*
output_asio::get_name(void)
{
	return "ASIO";
}

int
output_asio::open_ex(int srate, int bps, int nch, int format_code)
{
	if(hThread == NULL) {
		EventDestroyThread = ::CreateEvent(NULL, false, false, NULL);
		EventReadyThread = ::CreateEvent(NULL, false, false, NULL);

		unsigned int	dwThread;

		hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadProc,
															NULL, 0, &dwThread));
		::WaitForSingleObject(EventReadyThread, INFINITE);

		::CloseHandle(EventReadyThread);
	}

	int		RetCode = ParamMsg(MSG_OPEN, srate, format_code, bps, nch).Call();

	OpenDriver = RetCode != 0;

	return RetCode;
}

int
output_asio::can_write(void)
{
	return ParamMsg(MSG_CAN_WRITE).Call();
}

int
output_asio::write(const char* data, int bytes)
{
	return ParamMsg(MSG_WRITE, bytes,
					reinterpret_cast<unsigned char*>(const_cast<char*>(data))).Call();
}

int
output_asio::get_latency_bytes(void)
{
	return pPcmAsio->MsgGetLatency();
}

void
output_asio::force_play(void)
{
	ParamMsg(MSG_PLAY).Call();
}

void
output_asio::pause(int state)
{
	ParamMsg(MSG_PAUSE, state).Call();
}

int
output_asio::do_flush(void)
{
	return ParamMsg(MSG_FLUSH).Call();
}

int
output_asio::is_playing(void)
{
	return pPcmAsio->MsgGetLatency() ? 1 : 0;
}

const char*
config_output_asio::get_name(void)
{
	return "ASIO";
}

const char*
config_output_asio::get_parent_name(void)
{
	return "Output";
}

unsigned int __stdcall
ThreadProc(void* /*Param*/)
{
	pPcmAsio = new PcmAsio;

	::SetEvent(EventReadyThread);

	while(::WaitForSingleObjectEx(EventDestroyThread, INFINITE, true) !=
																	WAIT_OBJECT_0);

	delete pPcmAsio;

	_endthreadex(0);

	return 0;
}

void CALLBACK
ApcProc(ULONG_PTR dwParam)
{
	ParamMsg*	Param = reinterpret_cast<ParamMsg*>(dwParam);

	switch(Param->Msg) {
	case MSG_CLOSE_DRIVER:
		pPcmAsio->CloseDriver();
		break;
	case MSG_OPEN:
		Param->RetCode = pPcmAsio->MsgOpen(Param->Param1, Param->Param2,
													Param->Param3, Param->Param4);
		break;
	case MSG_CAN_WRITE:
		Param->RetCode = pPcmAsio->MsgCanWrite();
		break;
	case MSG_WRITE:
		Param->RetCode = pPcmAsio->MsgWrite(Param->Param1, Param->Buff);
		break;
	case MSG_PLAY:
		pPcmAsio->MsgPlay();
		break;
	case MSG_PAUSE:
		pPcmAsio->MsgPause(Param->Param1);
		break;
	case MSG_FLUSH:
		Param->RetCode = pPcmAsio->MsgFlush();
		break;
	}

	Param->UnPause();
}

ParamMsg::ParamMsg(int _Msg)
{
	Msg = _Msg;
}

ParamMsg::ParamMsg(int _Msg, int _Param1)
{
	Msg = _Msg;
	Param1 = _Param1;
}

ParamMsg::ParamMsg(int _Msg, int _Param1, int _Param2, int _Param3, int _Param4)
{
	Msg = _Msg;
	Param1 = _Param1;
	Param2 = _Param2;
	Param3 = _Param3;
	Param4 = _Param4;
}

ParamMsg::ParamMsg(int _Msg, int _Param1, unsigned char* _Buff)
{
	Msg = _Msg;
	Param1 = _Param1;
	Buff = _Buff;
}

int
ParamMsg::Call(void)
{
	EventWaitThread = ::CreateEvent(NULL, false, false, NULL);

	::QueueUserAPC(&ApcProc, hThread, reinterpret_cast<ULONG_PTR>(this));
	::WaitForSingleObject(EventWaitThread, INFINITE);

	::CloseHandle(EventWaitThread);

	return RetCode;
}

void
ParamMsg::UnPause(void)
{
	::SetEvent(EventWaitThread);
}

HWND
config_output_asio::create(HWND parent)
{
	OptionDialog*	Dialog = new OptionDialog(parent);

	Dialog->Create();

	return Dialog->HWindow;
}

OptionDialog::OptionDialog(HWND hParentWnd) : SDialog(hParentWnd, IDD_CONFIG_ASIO)
{
	CloseOnDestroy = true;
}

OptionDialog::~OptionDialog(void)
{
	delete Device;
	delete ThreadPriority;
	delete BufferSizeUpDown;
	delete ShiftChannelsUpDown;
	delete InputMonitor;
}

bool
OptionDialog::WmInitDialog(Org_Mes* OrgMes, HWND hwnd, LONG lInitParam)
{
	OrgMes->ExecMessage = true;
	SDialog::WmInitDialog(OrgMes, hwnd, lInitParam);

	HWND	hParentParentWindow = ::GetParent(HParentWindow);

	MoveWindowCenter(hParentParentWindow ? hParentParentWindow : HParentWindow);

	Device = new SComboBox(this, IDC_DEVICE);
	ThreadPriority = new SComboBox(this, IDC_THREAD_PRIORITY);
	BufferSizeUpDown = new SUpDown(this, IDC_BUFFER_SIZE_UPDOWN);
	ShiftChannelsUpDown = new SUpDown(this, IDC_SHIFT_CHANNELS_UPDOWN);
	InputMonitor = new SButton(this, IDC_INPUT_MONITOR, BS_CHECKBOX);

	const int	NumDrv = asioDrivers->asioGetNumDev();

	for(int Idx = 0; Idx < NumDrv; Idx++) {
		const int	DriverNameLen = 64;
		char	DriverName[DriverNameLen];

		asioDrivers->asioGetDriverName(Idx, DriverName, DriverNameLen);
		Device->AddString(DriverName);
	}

	Device->SetCurSel(cfg_device);

	ThreadPriority->AddString("Normal");
	ThreadPriority->AddString("Above normal");
	ThreadPriority->AddString("Highest");
	ThreadPriority->AddString("Time critical");

	ThreadPriority->SetCurSel(cfg_thread_priority);

	BufferSizeUpDown->SetRange(0, 63);
	BufferSizeUpDown->SetPos(cfg_buf_size);

	ShiftChannelsUpDown->SetRange(0, 99);
	ShiftChannelsUpDown->SetPos(cfg_shift_channels);

	if(cfg_input_monitor) {
		InputMonitor->Check();
	}

	return true;
}

void
OptionDialog::WmCommand(Org_Mes* OrgMes, int wNotifyCode, int wID, HWND hwndCtl)
{
	OrgMes->ExecMessage = true;
	SDialog::WmCommand(OrgMes, wNotifyCode, wID, hwndCtl);

	switch(wID) {
	case IDC_DEVICE:
		if(wNotifyCode == CBN_SELCHANGE) {
			cfg_device = Device->GetCurSel();
		}
		break;
	case IDC_THREAD_PRIORITY:
		if(wNotifyCode == CBN_SELCHANGE) {
			cfg_thread_priority = ThreadPriority->GetCurSel();
		}
		break;
	case IDC_BUFFER_SIZE:
		if(wNotifyCode == EN_CHANGE) {
			cfg_buf_size = BufferSizeUpDown->GetPos();
		}
		break;
	case IDC_SHIFT_CHANNELS:
		if(wNotifyCode == EN_CHANGE) {
			cfg_shift_channels = ShiftChannelsUpDown->GetPos();
		}
		break;
	case IDC_INPUT_MONITOR:
		if(wNotifyCode == BN_CLICKED) {
			cfg_input_monitor = InputMonitor->GetCheck() == BF_CHECKED;
		}
		break;
	}
}

