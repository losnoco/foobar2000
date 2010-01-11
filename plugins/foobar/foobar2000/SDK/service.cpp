#include "foobar2000.h"
#include "component.h"

foobar2000_client g_client;
foobar2000_api * g_api;

service_class_ref service_factory_base::enum_find_class(const GUID & p_guid)
{
	assert(core_api::are_services_available() && g_api);
	return g_api->service_enum_find_class(p_guid);
}

bool service_factory_base::enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,unsigned p_index)
{
	assert(core_api::are_services_available() && g_api);
	return g_api->service_enum_create(p_out,p_class,p_index);
}

unsigned service_factory_base::enum_get_count(service_class_ref p_class)
{
	assert(core_api::are_services_available() && g_api);
	return g_api->service_enum_get_count(p_class);
}

service_factory_base * service_factory_base::list=0;


#ifdef WIN32

long interlocked_increment(long * var)
{
	assert(!((unsigned)var&3));
	return InterlockedIncrement(var);
}

long interlocked_decrement(long * var)
{
	assert(!((unsigned)var&3));
	return InterlockedDecrement(var);
}

#else

#error portme

#endif