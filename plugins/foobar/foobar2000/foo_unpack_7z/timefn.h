#ifndef _TIMEFN_H
#define _TIMEFN_H

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
#endif

uint64_t dostime_to_timestamp(unsigned long ts);

#endif