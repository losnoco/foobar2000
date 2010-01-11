// Rune Madsen (downey@artemis.dk) - Created 2003

// Include foobar2000 SDK
#include "../SDK/components_menu.h"
#include "../SDK/foobar2000.h"
#include "../SDK/initquit.h"
#include "../SDK/play_callback.h"
#include "../SDK/play_control.h"
#include "../SDK/config.h"
#include "../SDK/console.h"
#include "../helpers/unicode_helper.h"

// Include Resource file
#include "resource.h"

// Include windows library headers
#include <commctrl.h>
#include <assert.h>
#include <memory.h>
#include <process.h>    /* _beginthread, _endthread */

// Include TCP wrapper class
#include "blockingsocket.h"

// Configuration variables
static cfg_string cfg_pass("telnetd_password", "");
static cfg_int cfg_port("telnetd_port", 1337);
static cfg_int cfg_runoninit("telnetd_runoninit", 0);

// Server values
HANDLE	server_thread;
DWORD	server_threadid;
int		server_port;
string8 server_pass;
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
	
	psocket->Write(login,strlen(login));
	psocket->ReadLine(line,sizeof line);

	if(strcmp(line, server_pass))
		return false;

	psocket->m_wFlags |= CTelnetSocket::FLAG_VALIDATED;
	return true;		
		
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
					if(stricmp(cmd, "EXIT") == 0) { play_control::command(play_control::FOOBAR2000_EXIT,0); }
					if(stricmp(cmd, "PLAY") == 0) { play_control::command(play_control::PLAYBACK_PLAY,0); }
					if(stricmp(cmd, "STOP") == 0) { play_control::command(play_control::PLAYBACK_STOP,0); }
					if(stricmp(cmd, "PREV") == 0) { play_control::command(play_control::PLAYBACK_PREV,0); }
					if(stricmp(cmd, "NEXT") == 0) { play_control::command(play_control::PLAYBACK_NEXT,0); }
					if(stricmp(cmd, "PAUSE") == 0) { play_control::command(play_control::PLAYBACK_PAUSE,0); }
					if(stricmp(cmd, "SKIP") == 0) {
						if (strlen(line) > 5) {
							double pos= 0;
							play_control::command(play_control::PLAYBACK_GETPOS,(int)&pos);
							{
								double newpos= pos + atof(line+5);
								play_control::command(play_control::PLAYBACK_SEEK,(int)&newpos);
							}
						}
					}
					/*
					if(strcmp(cmd, "JUMP_TRACK") == 0) {
						int track;
						if (strlen(line) < 20 && sscanf(line,"JUMP_TRACK %d",&track) != EOF) {
								
						}
					}
					
					*/
					if(stricmp(cmd, "HELP") == 0) { 
						sprintf(youWrote, "Following commands avalaible:\r\n  EXIT,PLAY,STOP,PREV,NEXT,PAUSE,SKIP <sec>\n\n"); sConnect.Print(youWrote);
					}
					sprintf(youWrote, "Cmd:'%s'",cmd); sConnect.Print(youWrote);
				}
				free(line);
			}
		} /* No more to do in this connection */
	}
	catch(const char* e) {
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
		catch(const char* e) {
			sockListen.Cleanup();
			//cout << e << "\n";
		}

		WaitForSingleObject(waitHandle,INFINITE);

		try {
			sockListen.Close();
			Sleep(300);
			WSACleanup();
		}
		catch(const char* e) {
			//cout << e << "\n";
		}
	}

	return exitcode;
}



static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			SetDlgItemInt(wnd,IDC_PORT,cfg_port,0);
			SetDlgItemText(wnd,IDC_PASS,string_os_from_utf8(cfg_pass));
			SendDlgItemMessage(wnd,IDC_AUTOSTART,BM_SETCHECK,cfg_runoninit,0);
			EnableWindow(GetDlgItem(wnd,IDC_PORT), server_state == RUNNING ? 0 : 1);
			EnableWindow(GetDlgItem(wnd,IDC_PASS), server_state == RUNNING ? 0 : 1);
		}

		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_TRIGSERVER:

			string8 foo;
			foo = string_utf8_from_window(wnd, IDC_PORT);
			if (foo.is_empty())
			{
				foo.reset();
				foo.add_int(cfg_port);
				SetDlgItemText(wnd, IDC_PORT, foo);
			}
			else
			{
				cfg_port = atoi(foo);
			}
			cfg_pass = string_utf8_from_window(wnd, IDC_PASS);
			cfg_runoninit = SendDlgItemMessage(wnd, IDC_AUTOSTART, BM_GETCHECK, 0, 0);
			
			// Start or stop server.
			if (server_state == SHUTDOWN && cfg_port > 0 && cfg_port < 32768) {
				// Start
				ResetEvent(waitHandle);
				server_port = cfg_port;
				server_pass = cfg_pass;
				server_thread= CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))&server, &server_port, 0, &server_threadid);
				server_state= RUNNING;
				SetDlgItemText(wnd,IDC_TRIGSERVER,"&Stop");
				EnableWindow(GetDlgItem(wnd,IDC_PORT), 0);
				EnableWindow(GetDlgItem(wnd,IDC_PASS), 0);
			} else if (server_state == RUNNING) {
				// Stop
				SetEvent(waitHandle);
				server_state= SHUTDOWN;
				SetDlgItemText(wnd,IDC_TRIGSERVER,"&Start");
				EnableWindow(GetDlgItem(wnd,IDC_PORT), 1);
				EnableWindow(GetDlgItem(wnd,IDC_PASS), 1);
			}
			break;
		}
		break;
	case WM_DESTROY:
		{
			string8 foo;
			foo = string_utf8_from_window(wnd, IDC_PORT);
			if (!foo.is_empty())
			{
				cfg_port = atoi(foo);
			}
			cfg_pass = string_utf8_from_window(wnd, IDC_PASS);
			cfg_runoninit = SendDlgItemMessage(wnd, IDC_AUTOSTART, BM_GETCHECK, 0, 0);
		}
		break;
	}
	return 0;
}

class config_ui : public config
{
public:
	HWND create(HWND parent)
	{
		return CreateDialog(service_factory_base::get_my_instance(),MAKEINTRESOURCE(IDD_CONFIG),parent,ConfigProc);
	}
	const char * get_name() {return "Telnet Daemon";}
	const char * get_parent_name() {return "Components";}
};

// Function hooked from the foobar2000 player
class initquit_ui : public initquit
{
	virtual void on_init()
	{
		waitHandle= CreateEvent(NULL,TRUE,FALSE,NULL);
		server_state = SHUTDOWN;
		if (cfg_runoninit)
		{
			if (cfg_port > 0 && cfg_port < 32768)
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


// Playback function hooked from the foobar2000 player
/*
class play_callback_ui : public play_callback
{
	virtual void on_playback_starting()
	{
	}
	virtual void on_playback_new_track(metadb_handle * track)
	{
		now_playing = track;
	}
	virtual void on_playback_stop()
	{
		
	}
	virtual void on_playback_seek(double time)
	{
		
	}
	virtual void on_playback_pause(int state)
	{
		
	}
};
*/

static service_factory_single_t<config,config_ui> foo;
static service_factory_single_t<initquit,initquit_ui> foo2;
// static service_factory_single_t<play_callback,play_callback_ui> foo3;
