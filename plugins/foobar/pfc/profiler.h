#ifndef _PFC_PROFILER_H_
#define _PFC_PROFILER_H_

class profiler_static
{
private:
	const char * name;
	__int64 total_time,num_called;

public:
	profiler_static(const char * p_name);
	~profiler_static();
	void add_time(__int64 delta) {total_time+=delta;num_called++;}
};

class profiler_local
{
private:
	static __int64 get_timestamp();
	__int64 start;
	profiler_static * owner;
public:
	profiler_local(profiler_static * p_owner)
	{
		owner = p_owner;
		start = get_timestamp();
	}
	~profiler_local()
	{
		__int64 end = get_timestamp();
		owner->add_time(end-start);
	}

};

#define profiler(name) \
	static profiler_static profiler_static_##name(#name); \
	profiler_local profiler_local_##name(&profiler_static_##name);
	

class hires_timer
{
public:
	void start()
	{
		QueryPerformanceCounter(&m_start);
	}
	double query()
	{
		LARGE_INTEGER current, frequency;
		QueryPerformanceCounter(&current);
		QueryPerformanceFrequency(&frequency);
		return (double)( current.QuadPart - m_start.QuadPart ) / (double) frequency.QuadPart;
	}
private:
	LARGE_INTEGER m_start;
};

class lores_timer
{
public:
	void start()
	{
		m_last_seen = m_start = GetTickCount();
	}

	double query()
	{
		t_uint64 time = GetTickCount();
		if (time < (m_last_seen & 0xFFFFFFFF)) time += 0x100000000;
		m_last_seen = (m_last_seen & 0xFFFFFFFF00000000) + time;

		return (double)(m_last_seen - m_start) / 1000.0;
	}

private:
	t_uint64 m_last_seen, m_start;
};

#endif