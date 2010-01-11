#ifndef _FOOBAR2000_UI_H_
#define _FOOBAR2000_UI_H_

#include "service.h"

#ifndef _WINDOWS
#error PORTME
#endif

class NOVTABLE user_interface : public service_base
{
public:
	typedef BOOL (WINAPI * HookProc_t)(HWND wnd,UINT msg,WPARAM wp,LPARAM lp,LRESULT * ret);
	//HookProc usage:
	//in your windowproc, call HookProc first, and if it returns true, return LRESULT value it passed to you

	virtual const char * get_name()=0;
	virtual HWND init(HookProc_t hook)=0;//create your window here
	virtual void shutdown()=0;//you need to destroy your window here
	virtual void activate()=0;
	virtual void hide()=0;
	virtual bool is_visible() = 0;//for activate/hide command
	virtual GUID get_guid() = 0;//for storing configuration (remembering active UI module)

	virtual void override_statusbar_text(const char * p_text) = 0;
	virtual void revert_statusbar_text() = 0;

	virtual void show_now_playing() = 0;

	static bool g_find(service_ptr_t<user_interface> & p_out,const GUID & p_guid);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	user_interface() {}
	~user_interface() {}
};

template<class T>
class user_interface_factory : public service_factory_single_t<user_interface,T> {};

class NOVTABLE ui_status_text_override : public service_base
{
public:
	virtual void override_text(const char * p_message) = 0;
	virtual void revert_text() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}

protected:
	ui_status_text_override() {}
	~ui_status_text_override() {}
};

class NOVTABLE ui_control : public service_base//implemented in the core, do not override
{
public:
	virtual bool is_visible()=0;
	virtual void activate()=0;
	virtual void hide()=0;
	virtual HICON get_main_icon()=0;//no need to free returned handle
	virtual HICON load_main_icon(unsigned width,unsigned height) = 0;//use DestroyIcon() to free it

	virtual void show_preferences(const GUID & p_page) = 0;

	//! Instantiates ui_status_text_override service, that can be used to display status messages.
	//! @param p_out receives new ui_status_text_override instance.
	//! @returns true on success, false on failure (out of memory / no GUI loaded / etc)
	virtual bool override_status_text_create(service_ptr_t<ui_status_text_override> & p_out) = 0;

	static bool g_get(service_ptr_t<ui_control> & p_out);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	ui_control() {}
	~ui_control() {}
};

class NOVTABLE ui_drop_item_callback : public service_base //called from UI when some object (ie. files from explorer) is dropped
{
public:
	virtual bool on_drop(interface IDataObject * pDataObject) = 0;//return true if you processed the data, false if not
	virtual bool is_accepted_type(interface IDataObject * pDataObject, DWORD * p_effect)=0;

	static bool g_on_drop(interface IDataObject * pDataObject);
	static bool g_is_accepted_type(interface IDataObject * pDataObject, DWORD * p_effect);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	ui_drop_item_callback() {}
	~ui_drop_item_callback() {}
};

template<class T>
class ui_drop_item_callback_factory : public service_factory_single_t<ui_drop_item_callback,T> {};

#endif