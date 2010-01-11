#include "foobar2000.h"

bool ui_drop_item_callback::g_on_drop(interface IDataObject * pDataObject)
{
	service_enum_t<ui_drop_item_callback> e;
	service_ptr_t<ui_drop_item_callback> ptr;
	if (e.first(ptr)) do {
		if (ptr->on_drop(pDataObject)) return true;
	} while(e.next(ptr));
	return false;
}

bool ui_drop_item_callback::g_is_accepted_type(interface IDataObject * pDataObject)
{
	service_enum_t<ui_drop_item_callback> e;
	service_ptr_t<ui_drop_item_callback> ptr;
	if (e.first(ptr)) do {
		if (ptr->is_accepted_type(pDataObject)) return true;
	} while(e.next(ptr));
	return false;
}

bool user_interface::g_find(service_ptr_t<user_interface> & p_out,const GUID & p_guid)
{
	service_enum_t<user_interface> e;
	service_ptr_t<user_interface> ptr;
	if (e.first(ptr)) do {
		if (ptr->get_guid() == p_guid)
		{
			p_out = ptr;
			return true;
		}
	} while(e.next(ptr));
	return false;
}


bool ui_control::g_get(service_ptr_t<ui_control> & p_out) {return service_enum_create_t(p_out,0);}

bool ui_control_v2::g_get(service_ptr_t<ui_control_v2> & p_out)
{
	service_ptr_t<ui_control> api;
	if (!service_enum_create_t(api,0)) return false;
	return api->service_query_t(p_out);
}