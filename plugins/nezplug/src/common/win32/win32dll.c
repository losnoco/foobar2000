/*
	Entry routine - Win32
	Author:   Mamiya (mamiya@proc.org.tohoku.ac.jp)
	Language: ANSI-C + WIN32SDK
	License:  PDS
*/

/* WIN32 standard headers */
#include "win32l.h"
#include <windows.h>
/* ANSI standard headers */
/* Libraries headers */
/* Project headers */

static char dllpath[MAX_PATH] = "";
static HINSTANCE hDLLInstance = 0;
char *GetDLLArgv0(void)
{
	return dllpath;
}

void *GetDLLInstance(void)
{
	return hDLLInstance;
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved
)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hDLLInstance = hinstDLL;
		GetModuleFileName(hinstDLL, dllpath, MAX_PATH);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
	}
	return TRUE;
}
