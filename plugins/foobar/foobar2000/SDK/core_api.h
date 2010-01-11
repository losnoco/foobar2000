#ifndef _CORE_API_H_
#define _CORE_API_H_

namespace core_api
{
	HINSTANCE FB2KAPI get_my_instance();
	pcchar FB2KAPI get_my_file_name();// eg. "foo_asdf" for foo_asdf.dll
	pcchar FB2KAPI get_my_full_path();//eg. file://c:\blah\foobar2000\foo_asdf.dll
	HWND FB2KAPI get_main_window();
	bool FB2KAPI are_services_available();
	bool FB2KAPI assert_main_thread();
	bool FB2KAPI is_main_thread();
	bool FB2KAPI is_shutting_down();
	bool FB2KAPI is_initializing();
	pcchar FB2KAPI get_profile_path();//eg. "file://c:\documents_and_settings\username\blah\foobar2000"
};

#endif