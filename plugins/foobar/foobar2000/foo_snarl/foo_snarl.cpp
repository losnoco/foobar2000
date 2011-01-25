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
#include "SnarlInterface.h"
#include "config.h"
#include <map>
#include <strsafe.h>

#pragma comment(lib, "../shared/shared.lib")

using namespace Snarl::V41;
using namespace pfc;

DECLARE_COMPONENT_VERSION(
		"foo_snarl",
		"1.1.0", 
		"Snarl notification interface for Foobar2000\n"
		"Developed by: Skyler Kehren (Pyrodogg)\n"
		"Contributions by:Max Battcher\n"
		"foosnarl at pyrodogg.com\n"
		"Copyright (C) 2008-2011 Skyler Kehren\n"
		"Released under BSD License");

string8 foobarIcon;

SnarlInterface sn;
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

void FSRegisterClass(int intClass){
	if(sn.AddClass(FSMsgClassDecode[intClass],FSMsgClassDecode[intClass],true) == 0){
		/*LPSTR error = "Unable to register ";
		LPSTR szClass = FSMsgClassDecode[intClass];
		error = strcat(error,szClass);
		error = strcat(error," class");
		popup_message::g_complain("FooSnarl",error);*/
		if (sn.GetLastError() != SnarlEnums::ErrorClassAlreadyExists)
		{
			popup_message::g_complain("FooSnarl",string_formatter() << "Unable to register class " << intClass);
		}
	}
}

void try_register()
{
	//Register Foobar2000 with Snarl
	if(sn.IsSnarlRunning()){
		//Display appropriate message depending on registration state with Snarl
		if(sn.RegisterApp("Foobar2000","Foobar2000",foobarIcon,hwndFooSnarlMsg,WM_USER,SnarlEnums::AppHasAbout)!=0){
			FSRegisterClass(Play);
			FSRegisterClass(Pause);
			FSRegisterClass(Stop);
			FSRegisterClass(Seek);
		}else{
			if (sn.GetLastError() != SnarlEnums::ErrorNotRunning)
				popup_message::g_show("FooSnarl:Unable to register with Snarl","",popup_message::icon_error);
		}

		if(hwndFooSnarlMsg == NULL){
			popup_message::g_show("FooSnarl:Unable to create message window","",popup_message::icon_error);
		}
	} else { 
		/*Snarl not running */
		//popup_message::g_show("Popup Test","Test",popup_message::icon_information);
	}
}

void try_unregister()
{
	//Unregister foosnarl
	if(sn.RemoveAllClasses(false)==0){
		//Failed to remove registered classes
		if (sn.GetLastError() != SnarlEnums::ErrorNotRunning)
			sn.EZNotify("","Error","FooSnarl failed to remove registered classes",10,foobarIcon,0,0,0);
	}

	if(sn.UnregisterApp()==0){
		//FooSnarl failed to unregister
		if (sn.GetLastError() != SnarlEnums::ErrorNotRunning)
			sn.EZNotify("","Error","FooSnarl failed to unregister with Snarl",10,foobarIcon,0,0,0);
	};
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
			case SnarlEnums::SnarlLaunched:
			case SnarlEnums::SnarlStarted:
				try_register();
				return 0;

			case SnarlEnums::SnarlQuit:
			case SnarlEnums::SnarlStopped:
				try_unregister();
				return 0;
			}
		}
		break;
	case WM_USER:
		switch(LOWORD(wParam))
		{
		case SnarlEnums::SnarlQuit:
			{
				try_unregister();
				return 0;
			}
		case SnarlEnums::NotificationClicked:
		case SnarlEnums::NewNotificationClick:
			{
				static_api_ptr_t<ui_control>()->activate();
				return 0;
			}
		case SnarlEnums::NotificationAck:
		case SnarlEnums::NewNotificationInvoked:
			{
				return 0;
			}
		case SnarlEnums::NotificationTimedOut:
		case SnarlEnums::NewNotificationExpired:
			{
				return 0;
			}
		case SnarlEnums::NotificationAction:
		case SnarlEnums::NewNotificationAction:
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
	string8 image;
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

	::SNARL_GLOBAL_MSG = sn.Broadcast();

	hwndFooSnarlMsg = CreateWindowEx(0,_T("FooSnarlMsg"),_T("FooSnarl Msg"),0,0,0,0,0,GetDesktopWindow(),0,0,0);

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
string8 temp_file;

void on_playback_event(int alertClass){
	static_api_ptr_t<playback_control> pc;
	metadb_handle_ptr handle;
	string8 format;
	service_ptr_t<titleformat_object> script;
	string_formatter text;
	string snarl_title;
	string snarl_msg;
	string snarl_icon;
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
	list_t<GUID> guid_list;
	handle_list.add_item(handle);
	guid_list.add_item( album_art_ids::cover_front );
	try
	{
		abort_callback_impl moo;
		album_art_extractor_instance_v2::ptr art_instance = static_api_ptr_t<album_art_manager_v2>()->open( handle_list, guid_list, moo );
		album_art_data_ptr art = art_instance->query( album_art_ids::cover_front, moo );
		if ( art->get_size() )
		{
			string8 temp_path;
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
	catch (...)
	{
		//Process title format string for message icon location
		g_advconfig_icon.get_static_instance().get_state(format);
		static_api_ptr_t<titleformat_compiler>()->compile_safe(script, format);
		pc->playback_format_title_ex(handle, NULL, text, script, NULL, play_control::display_level_titles);
		snarl_icon = text.toString();
	}

	//Test for existance of folder.jpg picture file
	DWORD attrib = GetFileAttributes(stringcvt::string_os_from_utf8(snarl_icon.get_ptr()));
	if((snarl_icon.get_ptr()=="" )||(0xFFFFFFFF == attrib)){
		snarl_icon = foobarIcon;
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
		sn.Hide(sn.GetLastMsgToken());
	}
	FSLastMsgClass = alertClass;

	if(sn.IsVisible(lastClassMsg[alertClass]) == -1){
		//Update existing message from same class
		sn.EZUpdate(lastClassMsg[alertClass],snarl_title.get_ptr(),snarl_msg.get_ptr(),snarl_time,snarl_icon.get_ptr());
	} else {
		//Create new message
		sn.EZNotify(FSClass(alertClass),snarl_title.get_ptr(),snarl_msg.get_ptr(),snarl_time,snarl_icon.get_ptr(),0,0,0);
		if(sn.GetLastMsgToken() != 0){
			FSAddActions();
		}
		lastClassMsg[alertClass] = sn.GetLastMsgToken();
	}	

}

LONG32 FSAddActions(){
	sn.AddAction(sn.GetLastMsgToken(),"Back","@1");
	sn.AddAction(sn.GetLastMsgToken(),"Next","@2");
	sn.AddAction(sn.GetLastMsgToken(),"Stop","@3");
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