
#include "../SDK/foobar2000.h"
#include "../../pfc/pfc.h"
#include "../SDK/initquit.h"
#include "../SDK/output.h"
#include "../SDK/play_control.h"
#include "../SDK/config.h"

unsigned int __stdcall	ThreadProc(void* Param);
void CALLBACK	ApcProc(ULONG_PTR dwParam);

enum {
	MSG_CLOSE_DRIVER,
	MSG_OPEN,
	MSG_CAN_WRITE,
	MSG_WRITE,
	MSG_PLAY,
	MSG_PAUSE,
	MSG_FLUSH,
};

class	PcmAsio;

class
initquit_output_asio : public initquit
{
public:
	virtual void	on_init(void);
	virtual void	on_quit(void);
};

class
ParamMsg
{
public:
	int		Msg;
	int		RetCode;
	int		Param1;
	int		Param2;
	int		Param3;
	int		Param4;
	unsigned char*	Buff;

	ParamMsg(int _Msg);
	ParamMsg(int _Msg, int _Param1);
	ParamMsg(int _Msg, int _Param1, int _Param2, int _Param3, int _Param4);
	ParamMsg(int _Msg, int _Param1, unsigned char* _Buff);

	int		Call(void);
	void	UnPause(void);

private:
	HANDLE	EventWaitThread;
};

class
output_asio : public output_i_base
{
public:
	output_asio(void);
	~output_asio(void);

	virtual GUID	get_guid(void);
	virtual const char*	get_name(void);
	virtual int		open_ex(int srate, int bps, int nch, int format_code);
	virtual int		can_write(void);
	virtual int		write(const char* data, int bytes);
	virtual int		get_latency_bytes(void);
	virtual void	force_play(void);
	virtual void	pause(int state);
	virtual int		do_flush(void);
	virtual int		is_playing(void);

private:
	bool	OpenDriver;
};

class
config_output_asio : public config
{
public:
	const char* get_name(void);
	const char* get_parent_name(void);
	HWND	create(HWND parent);
};

class
OptionDialog : public SDialog
{
public:
	OptionDialog(HWND hParentWnd);
	~OptionDialog(void);

private:
	SComboBox*	Device;
	SComboBox*	ThreadPriority;
	SUpDown*	BufferSizeUpDown;
	SUpDown*	ShiftChannelsUpDown;
	SButton*	InputMonitor;

protected:
	bool	WmInitDialog(Org_Mes* OrgMes, HWND hwnd, LONG lInitParam);
	void	WmCommand(Org_Mes* OrgMes, int wNotifyCode, int wID, HWND hwndCtl);
};

