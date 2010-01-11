#ifndef _MODELESS_DIALOG_H_
#define _MODELESS_DIALOG_H_

#include "service.h"

//use this to hook your modeless dialogs to main message loop and get IsDialogMessage() stuff
//not needed for config pages

class NOVTABLE modeless_dialog_manager : public service_base
{
public:
	virtual void add(HWND p_wnd) = 0;
	virtual void remove(HWND p_wnd) = 0;

	static void g_add(HWND p_wnd);
	static void g_remove(HWND p_wnd);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	modeless_dialog_manager() {}
	~modeless_dialog_manager() {}
};

#endif //_MODELESS_DIALOG_H_