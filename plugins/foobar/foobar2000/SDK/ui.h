#ifndef _FOOBAR2000_UI_H_
#define _FOOBAR2000_UI_H_

#include "service.h"

#ifndef WIN32
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
	virtual void shutdown(bool endsession)=0;//you need to destroy your window here
	virtual void activate()=0;
	virtual void hide()=0;
	virtual bool is_visible() = 0;//for activate/hide command
	virtual GUID get_guid() = 0;//for storing configuration (remembering active UI module)

	static bool g_find(service_ptr_t<user_interface> & p_out,const GUID & p_guid);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
protected:
	inline user_interface() {}
	inline ~user_interface() {}
};

class NOVTABLE user_interface_v2 : public user_interface
{
public:
	virtual void override_statusbar_text(const char * p_text) = 0;
	virtual void revert_statusbar_text() = 0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return user_interface::service_query(guid);
	}
protected:
	inline user_interface_v2() {}
	inline ~user_interface_v2() {}
};

template<class T>
class user_interface_factory : public service_factory_single_t<user_interface,T> {};

class NOVTABLE ui_control : public service_base//implemented in the core, do not override
{
public:
	virtual bool is_visible()=0;
	virtual void activate()=0;
	virtual void hide()=0;
	virtual HICON get_main_icon()=0;//no need to free returned handle
	virtual HICON load_main_icon(unsigned width,unsigned height)=0;//use DestroyIcon() to free it

	static bool g_get(service_ptr_t<ui_control> & p_out);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
protected:
	inline ui_control() {}
	inline ~ui_control() {}
};

class NOVTABLE ui_status_text_override : public service_base
{
public:
	virtual void override_text(const char * p_message) = 0;
	virtual void revert_text() = 0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

protected:
	inline ui_status_text_override() {}
	inline ~ui_status_text_override() {}
};

class NOVTABLE ui_control_v2 : public ui_control
{
public:
	//! Instantiates ui_status_text_override service, that can be used to display status messages.
	//! @param p_out receives new ui_status_text_override instance.
	//! @returns true on success, false on failure (out of memory / no GUI loaded / etc)
	virtual bool override_status_text_create(service_ptr_t<ui_status_text_override> & p_out) = 0;

	static bool g_get(service_ptr_t<ui_control_v2> & p_out);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return ui_control::service_query(guid);
	}
protected:
	inline ui_control_v2() {}
	inline ~ui_control_v2() {}
};

class NOVTABLE ui_drop_item_callback : public service_base //called from UI when some object (ie. files from explorer) is dropped
{
public:
	virtual bool on_drop(interface IDataObject * pDataObject)=0;//return true if you processed the data, false if not
	virtual bool is_accepted_type(interface IDataObject * pDataObject)=0;

	static bool g_on_drop(interface IDataObject * pDataObject);
	static bool g_is_accepted_type(interface IDataObject * pDataObject);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
protected:
	inline ui_drop_item_callback() {}
	inline ~ui_drop_item_callback() {}
};

template<class T>
class ui_drop_item_callback_factory : public service_factory_single_t<ui_drop_item_callback,T> {};

#endif