#include "pfc.h"


__declspec(naked) __int64 profiler_local::get_timestamp()
{
	__asm
	{
		rdtsc
		ret
	}
}

profiler_static::profiler_static(const char * p_name)
{
	name = p_name;
	total_time = 0;
	num_called = 0;
}

profiler_static::~profiler_static()
{
	char blah[512];
	char total_time_text[128];
	char num_text[128];
	char average_text[128];
	char name_truncated[48];
	char total_time_truncated[16];
	_i64toa(total_time,total_time_text,10);
	{
		const unsigned max = tabsize(name_truncated) - 1;
		unsigned namelen = strlen(name);
		if (namelen > max) namelen = max;
		memcpy(name_truncated,name,max);
		memset(name_truncated + namelen, ' ',max-namelen);
		name_truncated[max] = 0;
	}
	{
		unsigned timelen = strlen(total_time_text);
		const unsigned max = tabsize(total_time_truncated) - 1;
		if (timelen > max)
		{
			strcpy(total_time_truncated,"<overflow>");
		}
		else
		{
			unsigned pad = max - timelen;
			memset(total_time_truncated,' ',pad);
			memcpy(total_time_truncated + pad, total_time_text, timelen);
			total_time_truncated[max] = 0;
		}
	}
	
	_i64toa(num_called,num_text,10);
	if (num_called > 0)
		_i64toa(total_time / num_called, average_text,10);
	else strcpy(average_text,"N/A");
	sprintf(blah,"profiler: %s - %s cycles (executed %s times, %s average)\n",name_truncated,total_time_truncated,num_text,average_text);
	OutputDebugStringA(blah);
}
