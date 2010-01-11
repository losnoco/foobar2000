/*
	VRC7 instruments loader
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

extern char *GetDLLArgv0(void);
static void LoadVRC7ToneSub(char *fname, unsigned type)
{
	extern void VRC7SetTone(unsigned char *p, unsigned type);
	unsigned char tone[16 * 19];
	char tonename[MAX_PATH];
	LPTSTR fnoff;
	HANDLE hFile;
	DWORD size;

	GetFullPathName(GetDLLArgv0(), MAX_PATH, tonename, &fnoff);
	lstrcpy(fnoff, fname);
	hFile = CreateFile(
		tonename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	if (ReadFile(hFile, tone, 16 * 19, &size, NULL))
	{
		if (size == 8 * 15 || size == 16 * 19) VRC7SetTone(tone, type);
	}
	CloseHandle(hFile);
}

void LoadVRC7Tone(void)
{
	LoadVRC7ToneSub("vrc7tone.bin", 0);
	LoadVRC7ToneSub("vrc7.ill", 1);
	LoadVRC7ToneSub("fmpac.ill", 2);
	LoadVRC7ToneSub("fmunit.ill", 3);
}
