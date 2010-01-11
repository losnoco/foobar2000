#include "foobar2000.h"

void abort_callback::check() {
	if (is_aborting()) throw exception_aborted();
}
