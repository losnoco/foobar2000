#include "foobar2000.h"


bool converter::g_instantiate(service_ptr_t<converter> & p_out,const GUID & guid)
{
	service_enum_t<converter> e;
	service_ptr_t<converter> ptr;
	if (e.first(ptr)) do {
		if (ptr->get_guid() == guid)
		{
			p_out = ptr;
			return true;
		}
	} while(e.next(ptr));
	return false;
}

bool converter::g_instantiate_test(const GUID & guid)
{
	service_enum_t<converter> e;
	service_ptr_t<converter> ptr;
	if (e.first(ptr)) do {
		if (ptr->get_guid() == guid) return true;
	} while(e.next(ptr));
	return false;
}
