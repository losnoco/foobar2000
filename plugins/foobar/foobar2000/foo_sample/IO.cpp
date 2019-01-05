#include "stdafx.h"

void RunIOTest() {
	try {
		abort_callback_dummy noAbort;
		auto request = http_client::get()->create_request("GET");
		request->run("https://www.foobar2000.org", noAbort);
	} catch (std::exception const & e) {
		popup_message::g_show( PFC_string_formatter() << "Network test failure:\n" << e, "Information");
		return;
	}
	popup_message::g_show(PFC_string_formatter() << "Network test OK", "Information");
}

