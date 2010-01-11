#include "foobar2000.h"

void abort_callback::check_e()
{
	if (is_aborting()) throw io_result_aborted;
}
