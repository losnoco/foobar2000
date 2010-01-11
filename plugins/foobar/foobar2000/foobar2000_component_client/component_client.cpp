#include "../SDK/foobar2000.h"
#include "../SDK/component.h"

static HINSTANCE g_hIns;

static string_simple g_name,g_full_path;

static bool g_services_available = false, g_initialized = false;

#ifdef _DEBUG
static void selftest()
{
	assert(sizeof(bool)==1);
	assert(sizeof(int)==4);
	assert(sizeof(__int64)==8);
}
#endif



namespace core_api
{

	HINSTANCE FB2KAPI get_my_instance()
	{
		return g_hIns;
	}

	HWND FB2KAPI get_main_window()
	{
		return g_api->get_main_window();
	}
	pcchar FB2KAPI get_my_file_name()
	{
		return g_name;
	}

	pcchar FB2KAPI get_my_full_path()
	{
		return g_full_path;
	}

	bool FB2KAPI are_services_available()
	{
		return g_services_available;
	}
	bool FB2KAPI assert_main_thread()
	{
		return (g_services_available && g_api) ? g_api->assert_main_thread() : true;
	}

	bool FB2KAPI is_main_thread()
	{
		return (g_services_available && g_api) ? g_api->is_main_thread() : true;
	}
	pcchar FB2KAPI get_profile_path()
	{
		return (g_services_available && g_api) ? g_api->get_profile_path() : 0;
	}

	bool FB2KAPI is_shutting_down()
	{
		return (g_services_available && g_api) ? g_api->is_shutting_down() : g_initialized;
	}
	bool FB2KAPI is_initializing()
	{
		return (g_services_available && g_api) ? g_api->is_initializing() : !g_initialized;
	}
}

namespace {
	class foobar2000_client_impl : public foobar2000_client
	{
	public:
		unsigned FB2KAPI get_version() {return FOOBAR2000_CLIENT_VERSION;}
		pservice_factory_base FB2KAPI get_service_list() {return service_factory_base::list_get_pointer();}

		t_io_result FB2KAPI get_config(stream_writer * p_stream,abort_callback & p_abort)
		{
			return cfg_var::config_write_file(p_stream,p_abort);
		}

		t_io_result FB2KAPI set_config(stream_reader * p_stream,abort_callback & p_abort)
		{
			return cfg_var::config_read_file(p_stream,p_abort);
		}

		void FB2KAPI set_library_path(const char * path,const char * name) {
			g_full_path = path;
			g_name = name;
		}

		void FB2KAPI services_init(bool val) {
			if (val) g_initialized = true;
			g_services_available = val;
		}

		bool is_debug() {
#ifdef _DEBUG
			return true;
#else
			return false;
#endif
		}
	};
}

static foobar2000_client_impl g_client;

extern "C"
{
	__declspec(dllexport) foobar2000_client * _cdecl foobar2000_get_interface(foobar2000_api * p_api,HINSTANCE hIns)
	{
#ifdef _DEBUG
		selftest();
#endif
		g_hIns = hIns;
		g_api = p_api;

		return &g_client;
	}
}

#if 0
BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD     nReason,  IN LPVOID  Reserved )
{
    BOOLEAN bSuccess = TRUE;

    switch ( nReason ) {
        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls( hDllHandle );

            break;

        case DLL_PROCESS_DETACH:

            break;
    }
	return TRUE;
}
#endif