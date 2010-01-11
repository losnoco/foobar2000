/* Project headers */
#include "common/cso.h"

#ifdef _WIN32
/* WIN32 standard headers */
#include "win32l.h"
#include <windows.h>
/* critical section object (WIN32) */
/* critical section object (ANSI-C) */
unsigned CriticalSectionEnter(void *a)
{
	return (1 == InterlockedExchange((LPLONG)a, 0));
}
void CriticalSectionLeave(void *a)
{
	InterlockedExchange((LPLONG)a, 1);
}
void CriticalSectionInitialize(void *a)
{
	*(LPLONG)a = 1;
}
#endif
