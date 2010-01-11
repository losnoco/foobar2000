#ifndef _PFC_PROFILER_H_
#define _PFC_PROFILER_H_

#include <intrin.h>
namespace pfc {
class profiler_static {
public:
	profiler_static(const char * p_name);
	~profiler_static();
	void add_time(t_int64 delta) {total_time+=delta;num_called++;}
private:
	const char * name;
	t_uint64 total_time,num_called;
};

class profiler_local {
public:
	profiler_local(profiler_static * p_owner) {
		owner = p_owner;
		start = __rdtsc();
	}
	~profiler_local() {
		t_int64 end = __rdtsc();
		owner->add_time(end-start);
	}
private:
	t_int64 start;
	profiler_static * owner;
};

#define profiler(name) \
	static pfc::profiler_static profiler_static_##name(#name); \
	pfc::profiler_local profiler_local_##name(&profiler_static_##name);
	

class hires_timer {
public:
	void start() {
		QueryPerformanceCounter(&m_start);
	}
	double query() {
		LARGE_INTEGER current, frequency;
		QueryPerformanceCounter(&current);
		QueryPerformanceFrequency(&frequency);
		return (double)( current.QuadPart - m_start.QuadPart ) / (double) frequency.QuadPart;
	}
private:
	LARGE_INTEGER m_start;
};

class lores_timer {
public:
	void start() {
		m_last_seen = m_start = GetTickCount();
	}

	double query() {
		t_uint64 time = GetTickCount();
		if (time < (m_last_seen & 0xFFFFFFFF)) time += 0x100000000;
		m_last_seen = (m_last_seen & 0xFFFFFFFF00000000) + time;
		return (double)(m_last_seen - m_start) / 1000.0;
	}
private:
	t_uint64 m_last_seen, m_start;
};
}
#endif