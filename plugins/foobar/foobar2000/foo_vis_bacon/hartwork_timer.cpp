// Written by Sebastian Pipping <webmaster@hartwork.org>
// 2004-02-21

#include "hartwork_timer.h"


bool	HARTWORK_TIMER::m_bInited			= false;
bool	HARTWORK_TIMER::m_bUseHardwareTimer	= false;
double	HARTWORK_TIMER::m_fFactor			= 1.0;


HARTWORK_TIMER::HARTWORK_TIMER()
{
	if( !m_bInited )
	{
		choose_timer_once();
	}
	m_iStart = current_time();
}

void HARTWORK_TIMER::choose_timer_once()
{
	LONGLONG iFrequency;
	m_bUseHardwareTimer = !!QueryPerformanceFrequency( ( LARGE_INTEGER * )&iFrequency );
	if( m_bUseHardwareTimer )
	{
		m_fFactor = 1.0 / iFrequency;
	}
	else
	{
		m_fFactor = 0.001;
	}
}

LONGLONG HARTWORK_TIMER::current_time()
{
	LONGLONG iRet = 0;
	if( m_bUseHardwareTimer )
	{
		QueryPerformanceCounter( ( LARGE_INTEGER * )&iRet );
	}
	else
	{
	    iRet = GetTickCount(); //timeGetTime();
	}
	return iRet;
}


bool HARTWORK_TIMER::has_passed( double fMS, bool bUpdateStart )
{
	LONGLONG iNow = current_time();
	double fTimePassed = ( iNow - m_iStart ) * ( m_fFactor * 1000 );
	bool bRet = ( fTimePassed >= fMS );
	if( bRet && bUpdateStart ) m_iStart = iNow;
	return bRet;
}
