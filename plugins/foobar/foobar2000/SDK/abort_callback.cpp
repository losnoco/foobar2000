#include "foobar2000.h"

void abort_callback::check_e()
{
	if (is_aborting()) throw exception_io(io_result_aborted);
}
