#include "stdafx.h"

static bool g_is_enabled()
{
	return config_object::g_get_data_bool_simple(standard_config_objects::bool_remember_window_positions,false);
}

//introduced in win98/win2k
typedef HANDLE (WINAPI * t_MonitorFromRect)(const RECT*, DWORD);

#ifndef MONITOR_DEFAULTTONULL
#define MONITOR_DEFAULTTONULL       0x00000000
#endif

static bool test_rect(const RECT * rc)
{
	t_MonitorFromRect p_MonitorFromRect = (t_MonitorFromRect)GetProcAddress(GetModuleHandle(TEXT("user32")),"MonitorFromRect");
	if (!p_MonitorFromRect)
	{
		//if (!SystemParametersInfo(SPI_GETWORKAREA,0,&workarea,0)) return true;
		int max_x = GetSystemMetrics(SM_CXSCREEN), max_y = GetSystemMetrics(SM_CYSCREEN);
		return rc->left < max_x && rc->right > 0 && rc->top < max_y && rc->bottom > 0;
	}
	return !!p_MonitorFromRect(rc,MONITOR_DEFAULTTONULL);
}


bool cfg_window_placement::read_from_window(HWND window)
{
	WINDOWPLACEMENT wp;
	memset(&wp,0,sizeof(wp));
	if (g_is_enabled())
	{
		wp.length = sizeof(wp);
		if (!GetWindowPlacement(window,&wp))
			memset(&wp,0,sizeof(wp));
		else
		{
			if (!IsWindowVisible(window)) wp.showCmd = SW_HIDE;
		}
	}
	m_data = wp;
	return m_data.length == sizeof(m_data);
}

bool cfg_window_placement::on_window_creation(HWND window)
{
	bool ret = false;
	assert(!m_windows.have_item(window));
	m_windows.add_item(window);

	if (g_is_enabled())
	{
		if (m_data.length==sizeof(m_data) && test_rect(&m_data.rcNormalPosition))
		{
			if (SetWindowPlacement(window,&m_data))
			{
				ret = true;
			}
		}
	}

	return ret;
}

void cfg_window_placement::on_window_destruction(HWND window)
{
	if (m_windows.have_item(window))
	{
		read_from_window(window);
		m_windows.remove_item(window);
	}
}

bool cfg_window_placement::get_raw_data(write_config_callback * out)
{
	if (!g_is_enabled()) return false;

	{
		unsigned n, m = m_windows.get_count();
		for(n=0;n<m;n++)
		{
			HWND window = m_windows[n];
			assert(IsWindow(window));
			if (IsWindow(window) && read_from_window(window)) break;
		}
	}

	if (m_data.length == sizeof(m_data))
	{
		out->write(&m_data,sizeof(m_data));
		return true;
	}
	else return false;
}

void cfg_window_placement::set_raw_data(const void * data,int size)
{
	if (size == sizeof(m_data))
	{
		WINDOWPLACEMENT temp = *(WINDOWPLACEMENT*)data;
		if (temp.length == sizeof(temp)) m_data = temp;
	}
}

cfg_window_placement::cfg_window_placement(const GUID & p_guid) : cfg_var(p_guid)
{
	memset(&m_data,0,sizeof(m_data));
}
