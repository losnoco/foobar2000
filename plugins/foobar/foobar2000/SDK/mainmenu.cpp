#include "foobar2000.h"

bool mainmenu_commands::g_execute(const GUID & p_guid,service_ptr_t<service_base> p_callback) {
	service_enum_t<mainmenu_commands> e;
	service_ptr_t<mainmenu_commands> ptr;
	while(e.next(ptr)) {
		const t_uint32 count = ptr->get_command_count();
		for(t_uint32 n=0;n<count;n++) {
			if (ptr->get_command(n) == p_guid) {
				ptr->execute(n,p_callback);
				return true;
			}
		}
	}
	return false;
}