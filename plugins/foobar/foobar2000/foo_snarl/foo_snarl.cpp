/*
Copyright (c) 2008-2011, Skyler Kehren
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of 
	conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of 
	conditions and the following disclaimer in the documentation and/or other materials provided 
	with the distribution.
    * Neither the name of the author nor the names of its contributors may be used to endorse 
	or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include "V41/SnarlInterface.h"
#include "V42/SnarlInterface.h"
#include "config.h"
#include <map>
#include <strsafe.h>
#include <time.h>

#pragma comment(lib, "../shared/shared.lib")

DECLARE_COMPONENT_VERSION(
		"foo_snarl",
		"1.1.0", 
		"Snarl notification interface for Foobar2000\n"
		"Developed by: Skyler Kehren (Pyrodogg)\n"
		"Contributions by:Max Battcher\n"
		"foosnarl at pyrodogg.com\n"
		"Copyright (C) 2008-2011 Skyler Kehren\n"
		"Released under BSD License");

pfc::string8 foobarIcon;

bool using_v42 = false;
Snarl::V41::SnarlInterface sn41;
Snarl::V42::SnarlInterface sn42;
pfc::string8 snarl_password;
UINT SNARL_GLOBAL_MSG = 0;
HWND hwndFooSnarlMsg;
std::map<int,char *> FSMsgClassDecode;
LONG32 lastClassMsg[4] = {0,0,0,0};
LONG32 lastMsg = 0;
int FSLastMsgClass = 0;
int FSMsgClassCount = 4;
enum FSMsgClass : int {
	Stop = 0,
	Play,
	Pause,
	Seek
};

//enum FSMsgAction : int {
//	Back = 1,
//	Next,
//	Stop,
//};

inline char base64_char(unsigned char in)
{
	static const char base64_chars[64] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
										  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
										  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
										  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};
	return base64_chars[in];
}

void FSRegisterClass(int intClass){
	bool error;
	if(using_v42)
	{
		LONG32 ret = sn42.AddClass(FSMsgClassDecode[intClass], FSMsgClassDecode[intClass]);
		if ( ret < 0 && ret != -Snarl::V42::SnarlEnums::ErrorAlreadyRegistered )
		{
			console::formatter() << "[FooSnarl] Unable to register class " << intClass;
		}
	}
	else
	{
		if (sn41.AddClass(FSMsgClassDecode[intClass], FSMsgClassDecode[intClass], true) == 0)
		{
			if (sn41.GetLastError() != Snarl::V41::SnarlEnums::ErrorClassAlreadyExists)
			{
				console::formatter() << "[FooSnarl] Unable to register class " << intClass;
			}
		}
	}
}

void try_register()
{
	//Register Foobar2000 with Snarl
	using_v42 = sn42.GetVersion() >= 42;

	if (using_v42)
	{
		service_ptr_t<genrand_service> g_rand = genrand_service::g_create();
		g_rand->seed( time( NULL ) );
		pfc::array_t<unsigned> junk;
		junk.set_count( 4 );
		for ( unsigned i = 0; i < 4; i++ ) junk[ i ] = g_rand->genrand( ~0 );
		pfc::base64_encode( snarl_password, junk.get_ptr(), 16 );
		snarl_password.replace_byte( '=', '%', snarl_password.get_length() - 2 );

		LONG32 ret = sn42.Register("Foobar2000", "Foobar2000", foobarIcon, snarl_password.get_ptr(), hwndFooSnarlMsg, WM_USER);
		if (ret > 0)
		{
			FSRegisterClass(Play);
			FSRegisterClass(Pause);
			FSRegisterClass(Stop);
			FSRegisterClass(Seek);
		}
		else
		{
			if (ret != -Snarl::V42::SnarlEnums::ErrorNotRunning)
			{
				console::formatter() << "[FooSnarl] Unable to register with Snarl";
			}
		}
	}
	else
	{
		if(sn41.RegisterApp("Foobar2000", "Foobar2000", foobarIcon, hwndFooSnarlMsg, WM_USER, Snarl::V41::SnarlEnums::AppHasAbout) != 0)
		{
			FSRegisterClass(Play);
			FSRegisterClass(Pause);
			FSRegisterClass(Stop);
			FSRegisterClass(Seek);
		}
		else
		{
			if (sn41.GetLastError() != Snarl::V41::SnarlEnums::ErrorNotRunning)
				console::formatter() << "[FooSnarl] Unable to register with Snarl";
		}
	}
}

void try_unregister()
{
	//Unregister foosnarl
	if (using_v42)
	{
		LONG32 ret = sn42.ClearClasses();
		if (ret < 0 && ret != -Snarl::V42::SnarlEnums::ErrorNotRunning)
		{
			console::formatter() << "[FooSnarl] Failed to remove registered classes";
		}

		ret = sn42.Unregister("Foobar2000");
		if (ret < 0 && ret != -Snarl::V42::SnarlEnums::ErrorNotRunning)
		{
			console::formatter() << "[FooSnarl] Failed to unregister with Snarl";
		}
	}
	else
	{
		if(sn41.RemoveAllClasses(false)==0)
		{
			//Failed to remove registered classes
			if (sn41.GetLastError() != Snarl::V41::SnarlEnums::ErrorNotRunning)
				console::formatter() << "[FooSnarl] Failed to remove registered classes";
		}

		if(sn41.UnregisterApp()==0)
		{
			//FooSnarl failed to unregister
			if (sn41.GetLastError() != Snarl::V41::SnarlEnums::ErrorNotRunning)
				console::formatter() << "[FooSnarl] Failed to unregister with Snarl";
		}
	}
}

LRESULT CALLBACK WndProcFooSnarl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch(message)
	{
	case WM_SHOWWINDOW:
		if(wParam == TRUE){
			standard_commands::main_activate();
		}
	case WM_CLOSE:
		DestroyWindow(hwnd);
	default:
		if (message == ::SNARL_GLOBAL_MSG)
		{
			switch(wParam)
			{
			case Snarl::V41::SnarlEnums::SnarlLaunched:
#if Snarl::V41::SnarlEnums::SnarlLaunched != Snarl::V42::SnarlEnums::SnarlLaunched
			case Snarl::V42::SnarlEnums::SnarlLaunched:
#endif
			case Snarl::V42::SnarlEnums::SnarlStarted:
				try_register();
				return 0;

			case Snarl::V41::SnarlEnums::SnarlQuit:
#if Snarl::V41::SnarlEnums::SnarlQuit != Snarl::V42::SnarlEnums::SnarlQuit
			case Snarl::V42::SnarlEnums::SnarlQuit:
#endif
			case Snarl::V42::SnarlEnums::SnarlStopped:
				try_unregister();
				return 0;
			}
		}
		break;
	case WM_USER:
		switch(LOWORD(wParam))
		{
		case Snarl::V41::SnarlEnums::SnarlQuit:
#if Snarl::V41::SnarlEnums::SnarlQuit != Snarl::V42::SnarlEnums::SnarlQuit
		case Snarl::V42::SnarlEnums::SnarlQuit:
#endif
			{
				try_unregister();
				return 0;
			}
		case Snarl::V41::SnarlEnums::NotificationClicked:
#if Snarl::V41::SnarlEnums::NotificationClicked != Snarl::V42::SnarlEnums::NotificationClicked
		case Snarl::V42::SnarlEnums::NotificationClicked:
#endif
			{
				static_api_ptr_t<ui_control>()->activate();
				return 0;
			}
		case Snarl::V41::SnarlEnums::NotificationAck:
#if Snarl::V41::SnarlEnums::NotificationAck != Snarl::V42::SnarlEnums::NotificationAck
		case Snarl::V42::SnarlEnums::NotificationAck:
#endif
			{
				return 0;
			}
		case Snarl::V41::SnarlEnums::NotificationTimedOut:
#if Snarl::V41::SnarlEnums::NotificationTimedOut != Snarl::V42::SnarlEnums::NotificationTimedOut
		case Snarl::V42::SnarlEnums::NotificationTimedOut:
#endif
			{
				return 0;
			}
		case Snarl::V42::SnarlEnums::NotifyAction:
			int action = HIWORD(wParam);
			switch(action){
			case 1:
				//Back action
				static_api_ptr_t<playback_control>()->start(playback_control::track_command_prev,false);
				return 0;
			case 2:
				//Next action
				static_api_ptr_t<playback_control>()->start(playback_control::track_command_next,false);
				return 0;
			case 3:
				//Stop
				static_api_ptr_t<playback_control>()->stop();
				return 0;
			}
		}
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

class FooSnarl : public initquit, public play_callback{ 
public:

// initquit metods
 void on_init()
{
	static_api_ptr_t<play_callback_manager> playCBM;
	static_api_ptr_t<ui_control> uiMain;
	WNDCLASSEX wcex = {0};
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC) WndProcFooSnarl;
	wcex.hInstance = core_api::get_my_instance();
	wcex.lpszClassName = _T("FooSnarlMsg");

	FSMsgClassDecode[FSMsgClass::Play] = "Play";
	FSMsgClassDecode[FSMsgClass::Pause] = "Pause";
	FSMsgClassDecode[FSMsgClass::Stop] = "Stop";
	FSMsgClassDecode[FSMsgClass::Seek] = "Seek";

	RegisterClassEx(&wcex);
	
	//Snippet for testing Message Window failures
	//LPTSTR lpszFunction = TEXT("RegisterClassEx");
	//	LPVOID lpMsgBuf;
	//	LPVOID lpDisplayBuf;
	//	DWORD dw = GetLastError(); 

	//	FormatMessage(
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	//		FORMAT_MESSAGE_FROM_SYSTEM |
	//		FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL,
	//		dw,
	//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		(LPTSTR) &lpMsgBuf,
	//		0, NULL );

	//	// Display the error message and exit the process

	//	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
	//		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
	//	StringCchPrintf((LPTSTR)lpDisplayBuf, 
	//		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
	//		TEXT("%s failed with error %d: %s"), 
	//		lpszFunction, dw, lpMsgBuf); 
	//	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	//	LocalFree(lpMsgBuf);
	//	LocalFree(lpDisplayBuf);

	uGetModuleFileName(NULL, foobarIcon);
	foobarIcon += ",105";

	::SNARL_GLOBAL_MSG = sn41.Broadcast(); // XXX

	hwndFooSnarlMsg = CreateWindowEx(0,_T("FooSnarlMsg"),_T("FooSnarl Msg"),0,0,0,0,0,GetDesktopWindow(),0,0,0);

	if(hwndFooSnarlMsg == NULL)
	{
		console::formatter() << "[FooSnarl] Unable to create message window (Error 0x" << pfc::format_int(GetLastError(),0,16) << ")";
	}
	//sn.EZNotify("","HWND",(LPCSTR)hwndFooSnarlMsg,5,0,0,0,0);

		//Register Playcallback module
	playCBM->register_callback(this,flag_on_playback_new_track | 
		/*flag_on_playback_dynamic_info | */
		flag_on_playback_dynamic_info_track | 
		flag_on_playback_pause | 
		flag_on_playback_stop,true);

	try_register();	
}

 void on_quit()
{
	//Unregister playcallback
	static_api_ptr_t<play_callback_manager>()->unregister_callback(this);

	try_unregister();

	lastSong = 0;
	DestroyWindow(hwndFooSnarlMsg);
	UnregisterClass(_T("FooSnarlMsg"),core_api::get_my_instance());

	if ( temp_file.get_length() ) uDeleteFile(temp_file);
}

protected:
metadb_handle_ptr lastSong;
pfc::string8 temp_file;

void on_playback_event(int alertClass){
	static_api_ptr_t<playback_control> pc;
	metadb_handle_ptr handle;
	pfc::string8 format;
	service_ptr_t<titleformat_object> script;
	pfc::string_formatter text;
	pfc::string snarl_title;
	pfc::string snarl_msg;
	pfc::string snarl_icon;
	pfc::string8 snarl_icon_data;
	long snarl_time;

	if(pc->get_now_playing(handle)){
		lastSong.copy(handle);
	} else {	
			handle.copy(lastSong);
	}

	if (handle.is_empty()) return;
	
	//Process title format string for message body
	g_advconfig_string_format.get_static_instance().get_state(format);
	static_api_ptr_t<titleformat_compiler>()->compile_safe(script, format);
	pc->playback_format_title_ex(handle, NULL, text, script, NULL, play_control::display_level_titles);
	snarl_msg = text.toString();

	//Process title format string for message title
	g_advconfig_string_title_format.get_static_instance().get_state(format);
	static_api_ptr_t<titleformat_compiler>()->compile_safe(script, format);
	pc->playback_format_title_ex(handle, NULL, text, script, NULL, play_control::display_level_titles);
	snarl_title = text.toString();

	metadb_handle_list handle_list;
	pfc::list_t<GUID> guid_list;
	handle_list.add_item(handle);
	guid_list.add_item( album_art_ids::cover_front );
	try
	{
		abort_callback_impl moo;
		album_art_extractor_instance_v2::ptr art_instance = static_api_ptr_t<album_art_manager_v2>()->open( handle_list, guid_list, moo );
		album_art_data_ptr art = art_instance->query( album_art_ids::cover_front, moo );
		if ( art->get_size() )
		{
			if (using_v42 && art->get_size() <= 22 * 1024)
			{
				pfc::base64_encode( snarl_icon_data, art->get_ptr(), art->get_size() );
				snarl_icon_data.replace_byte( '=', '%', snarl_icon_data.length() - 2 );
			}
			else
			{
				pfc::string8 temp_path;
				if ( temp_file.get_length() ) uDeleteFile( temp_file );
				if ( uGetTempPath( temp_path ) && uGetTempFileName( temp_path, "snl", 0, temp_file ) )
				{
					snarl_icon = temp_file;
					temp_path = "file://";
					temp_path += temp_file;
					file::ptr icon_file;
					filesystem::g_open( icon_file, temp_path, filesystem::open_mode_write_new, moo );
					icon_file->write_object( art->get_ptr(), art->get_size(), moo );
					icon_file.release();
				}
				else throw exception_album_art_not_found();
			}
		}
	}
	catch (...)
	{
		//Process title format string for message icon location
		g_advconfig_icon.get_static_instance().get_state(format);
		static_api_ptr_t<titleformat_compiler>()->compile_safe(script, format);
		pc->playback_format_title_ex(handle, NULL, text, script, NULL, play_control::display_level_titles);
		snarl_icon = text.toString();
	}

	//Test for existance of folder.jpg picture file
	if (!using_v42 || snarl_icon.get_length()){
		DWORD attrib = GetFileAttributes(pfc::stringcvt::string_os_from_utf8(snarl_icon.get_ptr()));
		if((snarl_icon.get_ptr()=="" )||(0xFFFFFFFF == attrib)){
			snarl_icon = foobarIcon;
		}
	}

	//Get display timeout from user settings. If invalid, send error. Shouldn't happen max and min are set. 
	snarl_time = (long) g_advconfig_time.get_static_instance().get_state_int();
	if((snarl_time==NULL)||(snarl_time==0)){
		snarl_time = 5;
		snarl_title = "ERROR";
		snarl_msg = "Set valid display time in settings";
	}


	//Send Snarl Message
	if(FSLastMsgClass != alertClass){
		if (using_v42) sn42.Hide(sn42.GetLastMsgToken());
		else sn41.Hide(sn41.GetLastMsgToken());
	}
	FSLastMsgClass = alertClass;

	if (using_v42)
	{
		if (sn42.IsVisible(lastClassMsg[alertClass]) > 0)
		{
			sn42.Update(lastClassMsg[alertClass], FSClass(alertClass), snarl_title.get_ptr(), snarl_msg.get_ptr(), snarl_time, snarl_icon.get_ptr(), snarl_icon_data.get_ptr());
		}
		else
		{
			LONG32 ret = sn42.Notify(FSClass(alertClass), snarl_title.get_ptr(), snarl_msg.get_ptr(), snarl_time, snarl_icon.get_ptr(), snarl_icon_data.get_ptr());
			if (ret > 0)
			{
				FSAddActions();
				lastClassMsg[alertClass] = ret;
			}
			else lastClassMsg[alertClass] = 0;
		}
	}
	else
	{
		if(sn41.IsVisible(lastClassMsg[alertClass]) == -1)
		{
			//Update existing message from same class
			sn41.EZUpdate(lastClassMsg[alertClass],snarl_title.get_ptr(),snarl_msg.get_ptr(),snarl_time,snarl_icon.get_ptr());
		} else {
			//Create new message
			sn41.EZNotify(FSClass(alertClass),snarl_title.get_ptr(),snarl_msg.get_ptr(),snarl_time,snarl_icon.get_ptr(),0,0,0);
			lastClassMsg[alertClass] = sn41.GetLastMsgToken();
		}
	}	

}

LONG32 FSAddActions(){
	LONG32 token = sn42.GetLastMsgToken();
	sn42.AddAction(token,"Back","@1");
	sn42.AddAction(token,"Next","@2");
	sn42.AddAction(token,"Stop","@3");
	return 0;
}

public:
// play_callback methods
void on_playback_starting(play_control::t_track_command p_command,bool p_paused) {}
void on_playback_new_track(metadb_handle_ptr p_track) {
		on_playback_event(FSMsgClass::Play);
}
void  on_playback_stop(play_control::t_stop_reason p_reason) {
	if(p_reason == play_control::stop_reason_eof || p_reason == play_control::stop_reason_user){
		on_playback_event(Stop);
	}
}
void  on_playback_seek(double p_time) {}
void  on_playback_pause(bool p_state) {
	if(p_state == true){
		on_playback_event(FSMsgClass::Pause);
	} else {
		on_playback_event(FSMsgClass::Play);
	}
}
void  on_playback_edited(metadb_handle_ptr p_track) {}
void  on_playback_dynamic_info(const file_info & p_info) {}
void  on_playback_dynamic_info_track(const file_info &p_info) {
	on_playback_event(FSMsgClass::Play);
}
void  on_playback_time(double p_time) {}
void  on_volume_change(float p_new_val) {}
LPSTR FSClass(int intclass){
	switch(intclass){
		case FSMsgClass::Play:
			return "Play";
			break;
		case FSMsgClass::Pause:
			return "Pause";
			break;
		case FSMsgClass::Stop:
			return "Stop";
			break;
		case FSMsgClass::Seek:
			return "Seek";
			break;
	}
	return "";
}

};

//Register initquit
static initquit_factory_t< FooSnarl > foo0;
