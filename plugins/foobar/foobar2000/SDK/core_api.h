#ifndef _CORE_API_H_
#define _CORE_API_H_

namespace core_api
{
	HINSTANCE get_my_instance();
	const char * get_my_file_name();// eg. "foo_asdf" for foo_asdf.dll
	const char * get_my_full_path();//eg. file://c:\blah\foobar2000\foo_asdf.dll
	HWND get_main_window();
	bool are_services_available();
	bool assert_main_thread();
	bool is_main_thread();
	bool is_shutting_down();
	bool is_initializing();
	const char * get_profile_path();//eg. "file://c:\documents_and_settings\username\blah\foobar2000"
};

#endif