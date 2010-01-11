#ifndef _UI_EXTENSION_WINDOW_H_
#define _UI_EXTENSION_WINDOW_H_

#include "ui_extension.h"

//Since I use the same implementation everywhere I chose to make these helper to handle registering window class shit
//Basically you can use this instead of resorting to dialogs when using common controls etc. or multiple controls.
//NOT YET PROPERLY TESTED


#define __implement_get_class_data(class_name,want_transparent_background) \
	static container_window::class_data my_class_data= {class_name, 0, false, want_transparent_background};	\
		return my_class_data


class container_window
{
protected:
	HWND wnd_host;
	ui_extension_host * p_host;
	
public:
	struct class_data
	{
		const char * class_name;
		long refcount;
		bool class_registered;
		bool want_transparent_background;
	};

	/*override me e.g
	virtual class_data & get_class_data()const 
	{
		__implement_get_class_data(
		"My Window Class", //window class name
		true); //want transparent background (i.e. for toolbar controls)
	}
	*/

	virtual class_data & get_class_data() const = 0;

	__forceinline const char * get_class_name() const
	{
		return get_class_data().class_name;
	}

	__forceinline const bool get_want_transparent_background() const
	{
		return get_class_data().want_transparent_background;
	}

	__forceinline long & get_refcount() const
	{
		return get_class_data().refcount;
	}

	__forceinline bool & get_class_registered() const
	{
		return get_class_data().class_registered;
	}

	container_window();
	
	HWND create(HWND wnd_parent, unsigned styles = 0, unsigned ex_styles = 0);
	
	bool ensure_class_registered();	
	bool class_release();

	void destroy(); //if destroying someother way, you should make sure you call class_release()  properly
	
	static LRESULT WINAPI window_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	
	HWND get_wnd()const;
	
	//override me
	//you won't get called for WM_ERASEBKGRND if you specify want_transparent_background
	virtual LRESULT WINAPI on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)=0;
};

//you wouldnt really need this, but meh. does it even work.
#define get_container_window_pointer(T,T2,wnd) (static_cast<T*>(static_cast<container_ui_extension<T2>*>(reinterpret_cast<container_window*>(uGetWindowLong(wnd,GWL_USERDATA))))

//for multiple instance hosts only

#define container_ui_extension container_ui_extension_t<ui_extension>
#define container_menu_ui_extension container_ui_extension_t<menu_ui_extension>

template <class T>
class container_ui_extension_t : public container_window, public T
{
	ui_extension_host * p_host;
	
	HWND init_or_take_ownership(HWND parent, ui_extension_host * host, cfg_var::read_config_helper * config)
	{
		if (wnd_host)
		{
			ShowWindow(wnd_host, SW_HIDE);
			SetParent(wnd_host, parent);
			p_host->relinquish_ownership(wnd_host);
			p_host = host;
		}
		else
		{
			p_host = host; //store interface to host
			set_config(config);
			create(parent);
		}
		
		return wnd_host;
	}
	virtual void destroy_window() {destroy();}
	
public:
	virtual bool is_available(ui_extension_host * p)const {return true;}
	ui_extension_host * get_host() const {return p_host;}
	virtual HWND get_wnd()const{return container_window::get_wnd();}

	virtual void set_config(cfg_var::read_config_helper * config){};
	
	//override all the ui_extension crap as well

	container_ui_extension_t() : p_host(0){};
};


#endif