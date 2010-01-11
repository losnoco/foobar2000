#include "foobar2000.h"

void modeless_dialog_manager::add(HWND wnd)
{
	service_enum_t<modeless_dialog_manager> e;
	service_ptr_t<modeless_dialog_manager> ptr;
	if (e.first(ptr)) do {
		ptr->_add(wnd);
	} while(e.next(ptr));
}

void modeless_dialog_manager::remove(HWND wnd)
{
	service_enum_t<modeless_dialog_manager> e;
	service_ptr_t<modeless_dialog_manager> ptr;
	if (e.first(ptr)) do {
		ptr->_remove(wnd);
	} while(e.next(ptr));
}