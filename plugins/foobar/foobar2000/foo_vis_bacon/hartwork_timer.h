// Written by Sebastian Pipping <webmaster@hartwork.org>
// 2004-02-21

#ifndef _HARTWORK_TIMER_H_
#define _HARTWORK_TIMER_H_


#include <windows.h>


class HARTWORK_TIMER
{
private:
	static bool		m_bInited;				// Shared by ALL instances
	static bool		m_bUseHardwareTimer;	// Shared by ALL instances
	static double	m_fFactor;				// Shared by ALL instances

	LONGLONG		m_iStart;

	LONGLONG current_time();
	static void choose_timer_once();

public:
	HARTWORK_TIMER();
	bool has_passed( double fMS, bool bUpdateStart );
};


#endif // _HARTWORK_TIMER_H_