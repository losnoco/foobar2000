// Rune Madsen (downey@artemis.dk) - Created 2003

// Christopher Snowhill (kode54@gmail.com) - Updated 2016

// Include foobar2000 SDK
#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"

// Include Resource file
#include "resource.h"

// Include windows library headers
#include <commctrl.h>
#include <assert.h>
#include <memory.h>
#include <process.h>    /* _beginthread, _endthread */

// Include TCP wrapper class
#include "blockingsocket.h"

static const GUID guid_cfg_pass = { 0xf3f51c18, 0xe445, 0x4397,{ 0xbe, 0xc2, 0xfe, 0xcc, 0x8c, 0xd0, 0x53, 0xbf } };
static const GUID guid_cfg_port = { 0x5adcc82f, 0xa391, 0x43d6,{ 0xa9, 0x34, 0xc0, 0x8c, 0x3e, 0xfc, 0x18, 0x2b } };
static const GUID guid_cfg_runoninit = { 0x4c09d4d0, 0x3bef, 0x4c20,{ 0xac, 0xb9, 0xfb, 0x40, 0x60, 0x9d, 0x73, 0x49 } };


// Configuration variables
static cfg_string cfg_pass(guid_cfg_pass, "");
static cfg_int cfg_port(guid_cfg_port, 1337);
static cfg_int cfg_runoninit(guid_cfg_runoninit, 0);

// Server values
HANDLE	server_thread;
DWORD	server_threadid;
int		server_port;
pfc::string8 server_pass;
enum ENUM_state { SHUTDOWN, RUNNING }; ENUM_state server_state;

// Variable used to hide dialog box 
int		gui_hide;

// Handle where the server will wait.
HANDLE	waitHandle;

// play specific variables
// static metadb_handle * now_playing;

// Read in the persons user name, and make sure they are a valid user
bool Login(CTelnetSocket* psocket) {
	char* login="Pass: ";
	char line[1000];
	
	if (!server_pass.is_empty()) {
		psocket->Write(login, strlen(login));
		psocket->ReadLine(line, sizeof line);

		if (strcmp(line, server_pass))
			return false;
	}

	psocket->m_wFlags |= CTelnetSocket::FLAG_VALIDATED;
	return true;		
		
}

enum
{
	INVALID_COMMAND = 0,
	FOOBAR2000_EXIT = 1,
	PLAYBACK_PLAY,
	PLAYBACK_STOP,
	PLAYBACK_PAUSE,
	PLAYBACK_PREV,
	PLAYBACK_NEXT,
	PLAYBACK_SKIP
};

static class callback_process_command : public main_thread_callback {
	unsigned command;
	double skip_seconds;

public:
	virtual void callback_run()
	{
		switch (command)
		{
		case FOOBAR2000_EXIT:
			standard_commands::run_main(standard_commands::guid_main_exit);
			break;

		case PLAYBACK_PLAY:
			standard_commands::run_main(standard_commands::guid_main_play);
			break;

		case PLAYBACK_STOP:
			standard_commands::run_main(standard_commands::guid_main_stop);
			break;

		case PLAYBACK_PAUSE:
			static_api_ptr_t<playback_control>()->play_or_pause();
			break;

		case PLAYBACK_PREV:
			standard_commands::run_main(standard_commands::guid_main_previous);
			break;

		case PLAYBACK_NEXT:
			standard_commands::run_main(standard_commands::guid_main_next);
			break;

		case PLAYBACK_SKIP:
			static_api_ptr_t<playback_control>()->playback_seek_delta(skip_seconds);
			break;

		default: break;
		}
	}

	callback_process_command()
	{
		command = INVALID_COMMAND;
		skip_seconds = 0;
	}

	callback_process_command(const callback_process_command & in)
	{
		command = in.command;
		skip_seconds = in.skip_seconds;
	}

	callback_process_command(unsigned command)
	{
		this->command = command;
		callback_enqueue();
	}

	void cmd(unsigned command, double seconds = 0.0)
	{
		this->command = command;
		skip_seconds = seconds;
		callback_enqueue();
	}
};

static service_ptr_t<callback_process_command> g_cmd;

void process_cmd(unsigned command, double seconds = 0.0)
{
	g_cmd->cmd(command, seconds);
}

// Accept and handle connection
void AcceptProc(void* pParam) {
	CBlockingSocket* psockListen=(CBlockingSocket*)pParam;
	CSockAddr saClient;
	CTelnetSocket sConnect;
	char* buffer;

	try {
		char* welcome="Login Accepted. If in doubt, write \"HELP\"";
		char* nonuser="Login Failed.";

		buffer=(char*)calloc(1,1000);
		assert(buffer);
		if(!psockListen->Accept(sConnect,saClient))
			_endthread();
		_beginthread(AcceptProc,0,pParam);		//get ready for another connection

		
		if(!Login(&sConnect))
		{
			sConnect.Print(nonuser);
			sConnect.Close();
		}
		else
		{
			/* Connection Established and Accepted */
			sConnect.Print(welcome);
		
			{
				char *line,*youWrote;
				line= (char *) malloc(201);
				youWrote= (char *) malloc(250);
				while(sConnect.ReadLine(line,200,60*60*24*365)) {
					char cmd[200];
					if (sscanf(line,"%s ",cmd) == EOF) {
						sscanf(line,"%s",cmd);
					}
					/**********************************************************************/
					/************************ Implemented commands ************************/
					/**********************************************************************/
					if(stricmp(cmd, "EXIT") == 0) { process_cmd(FOOBAR2000_EXIT); }
					if(stricmp(cmd, "PLAY") == 0) { process_cmd(PLAYBACK_PLAY); }
					if(stricmp(cmd, "STOP") == 0) { process_cmd(PLAYBACK_STOP); }
					if(stricmp(cmd, "PREV") == 0) { process_cmd(PLAYBACK_PREV); }
					if(stricmp(cmd, "NEXT") == 0) { process_cmd(PLAYBACK_NEXT); }
					if(stricmp(cmd, "PAUSE") == 0) { process_cmd(PLAYBACK_PAUSE); }
					if(stricmp(cmd, "SKIP") == 0) {
						if (strlen(line) > 5) {
							double pos= atof(line+5);
							process_cmd(PLAYBACK_SKIP, pos);
						}
					}
					if(stricmp(cmd, "HELP") == 0) { 
						sprintf(youWrote, "Following commands available:\r\n  EXIT,PLAY,STOP,PREV,NEXT,PAUSE,SKIP <sec>\n\n"); sConnect.Print(youWrote);
					}
					sprintf(youWrote, "Cmd:'%s'",cmd); sConnect.Print(youWrote);
				}
				free(line);
				free(youWrote);
			}
		} /* No more to do in this connection */
	}
	catch(const char * e) {
		//log(e);
		sConnect.Cleanup();
	}
	_endthread();
}


// Server
static DWORD WINAPI server(int *param) {
	DWORD exitcode;
	exitcode=0;

	/* Starting socket */
	CBlockingSocket sockListen;

	WSADATA wsd;
	if(WSAStartup(0x0101,&wsd)!=0)
	{
		console::error("Unable to start socket");
	}
	else {

		try {
			CSockAddr saServer(INADDR_ANY,(unsigned short) *param);
			sockListen.Create();
			sockListen.Bind(saServer);
			sockListen.Listen();
			_beginthread(AcceptProc,0,&sockListen);
		}
		catch(const char * e) {
			sockListen.Cleanup();
			//cout << e << "\n";
		}

		WaitForSingleObject(waitHandle,INFINITE);

		try {
			sockListen.Close();
			Sleep(300);
			WSACleanup();
		}
		catch(const char * e) {
			//cout << e << "\n";
		}
	}

	return exitcode;
}


class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum { IDD = IDD_CONFIG };
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_PORT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PASS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_AUTOSTART, BN_CLICKED, OnClickAutoStart)
		COMMAND_HANDLER_EX(IDC_TRIGSERVER, BN_CLICKED, OnClickTrigServer)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnClickAutoStart(UINT, int, CWindow);
	void OnClickTrigServer(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SetDlgItemInt(IDC_PORT, cfg_port, 0);
	uSetDlgItemText(m_hWnd, IDC_PASS, cfg_pass);
	SendDlgItemMessage(IDC_AUTOSTART, BM_SETCHECK, cfg_runoninit);
	GetDlgItem(IDC_PORT).EnableWindow(server_state == RUNNING ? 0 : 1);
	GetDlgItem(IDC_PASS).EnableWindow(server_state == RUNNING ? 0 : 1);

	return FALSE;
}

void CMyPreferences::OnClickTrigServer(UINT, int, CWindow) {
	// Start or stop server.
	if (server_state == SHUTDOWN && cfg_port > 0 && cfg_port < 65536) {
		// Start
		ResetEvent(waitHandle);
		server_port = cfg_port;
		server_pass = cfg_pass;
		server_thread = CreateThread(NULL, 0, (unsigned long(__stdcall *)(void *))&server, &server_port, 0, &server_threadid);
		server_state = RUNNING;
		uSetDlgItemText(m_hWnd, IDC_TRIGSERVER, "&Stop");
		GetDlgItem(IDC_PORT).EnableWindow(0);
		GetDlgItem(IDC_PASS).EnableWindow(0);
	}
	else if (server_state == RUNNING) {
		// Stop
		SetEvent(waitHandle);
		server_state = SHUTDOWN;
		uSetDlgItemText(m_hWnd, IDC_TRIGSERVER, "&Start");
		GetDlgItem(IDC_PORT).EnableWindow(1);
		GetDlgItem(IDC_PASS).EnableWindow(1);
	}
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnClickAutoStart(UINT, int, CWindow) {
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt(IDC_PORT, cfg_port, 0);
	uSetDlgItemText(m_hWnd, IDC_PASS, cfg_pass);
	SendDlgItemMessage(IDC_AUTOSTART, BM_SETCHECK, cfg_runoninit);
	GetDlgItem(IDC_PORT).EnableWindow(server_state == RUNNING ? 0 : 1);
	GetDlgItem(IDC_PASS).EnableWindow(server_state == RUNNING ? 0 : 1);

	OnChanged();
}

void CMyPreferences::apply() {
	UINT n;
	n = GetDlgItemInt(IDC_PORT, 0, 0);
	if (n > 0 && n < 65536)
	{
		cfg_port = n;
	}
	cfg_pass = string_utf8_from_window(m_hWnd, IDC_PASS);
	cfg_runoninit = SendDlgItemMessage(IDC_AUTOSTART, BM_GETCHECK);

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if (!changed && SendDlgItemMessage(IDC_AUTOSTART, BM_GETCHECK) != cfg_runoninit) changed = true;
	if (!changed && GetDlgItemInt(IDC_PORT, 0, 0) != cfg_port) changed = true;
	if (!changed && strcmp(string_utf8_from_window(m_hWnd, IDC_PASS), cfg_pass) != 0) changed = true;
	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() { return "Telnet Daemon"; }
	GUID get_guid() {
		// {73018CF6-0F58-4DEC-B7BB-3CAE6D4C1057}
		static const GUID guid =
		{ 0x73018cf6, 0xf58, 0x4dec,{ 0xb7, 0xbb, 0x3c, 0xae, 0x6d, 0x4c, 0x10, 0x57 } };
		return guid;
	}
	GUID get_parent_guid() { return guid_tools; }
};


// Function hooked from the foobar2000 player
class initquit_ui : public initquit
{
	virtual void on_init()
	{
		g_cmd = new service_impl_t<callback_process_command>();
		waitHandle= CreateEvent(NULL,TRUE,FALSE,NULL);
		server_state = SHUTDOWN;
		if (cfg_runoninit)
		{
			if (cfg_port > 0 && cfg_port < 65536)
			{
				ResetEvent(waitHandle);
				server_port = cfg_port;
				server_pass = cfg_pass;
				server_thread= CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))&server, &server_port, 0, &server_threadid);
				server_state= RUNNING;
			}
		}
	}
	virtual void on_quit()
	{
		if (server_state == RUNNING) {
			// Stop
			SetEvent(waitHandle);
			server_state= SHUTDOWN;
		}
		if (waitHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(waitHandle);
		}
	}
};


static preferences_page_factory_t <preferences_page_myimpl> g_config_telnetd_factory;
static initquit_factory_t<initquit_ui> g_initquit_telnetd_factory;

DECLARE_COMPONENT_VERSION("Telnet Daemon", "1.0", "Simple telnet remote control daemon.");
VALIDATE_COMPONENT_FILENAME("foo_telnetd.dll");
