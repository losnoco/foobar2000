#include "common/win32/win32l.h"
#undef NOUSER
#include <windows.h>
#include <stdlib.h>
#include "common/nsfsdk/nsfsdk.h"
#include "common/zlib/nez.h"
#include "kmp_com.h"
#include "../version.h"

struct KMP_CTX_TAG
{
	HNSF hnsf;
	unsigned srate;
	unsigned nch;
	unsigned bits;
	unsigned filter;
	unsigned fdstype;
};

static const char *sectionname = "NEZplug";
static char iniFilePath[MAX_PATH] = "";

static UINT GetPrivateProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName)
{
	UINT ret;
	TCHAR szBuffer[32];
	ret = GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, lpFileName);
	wsprintf(szBuffer, "%d", ret);
	WritePrivateProfileString(lpAppName, lpKeyName, szBuffer, lpFileName);
	return ret;
}

static BOOL GetPrivateProfileBoolEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, BOOL bDefault, LPCTSTR lpFileName)
{
	BOOL ret;
	TCHAR szBuffer[32];
	if (GetPrivateProfileString(lpAppName, lpKeyName, bDefault ? "True" : "False", szBuffer, 32, lpFileName))
	{
		/* 先頭だけ確認 */
		ret = szBuffer[0] == 'T' || szBuffer[0] == 't';
	}
	else
		ret = bDefault;
	WritePrivateProfileString(lpAppName, lpKeyName, ret ? "True" : "False", lpFileName);
	return ret;
}

static int RegGetString(const char *lpszSubKey, const char *lpszName, char *lpszBuffer, unsigned uBufferSize)
{
	int ret = 0;
	HKEY hkey;
	DWORD dispo, dwt, dws;
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, lpszSubKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dispo)) return ret;
	dws = uBufferSize;
	if (ERROR_SUCCESS == RegQueryValueEx(hkey, lpszName,NULL,&dwt,(LPBYTE)lpszBuffer, &dws) && dwt == REG_SZ)
	{
		ret = dws;
	}
	RegCloseKey(hkey);
	return ret;
}

static void Setup(KMP_CTX *pctx)
{
	pctx->hnsf = 0;
	pctx->bits = 16;
	if (pctx->nch != 1) pctx->nch = 2;
	if (pctx->srate == 0) pctx->srate = GetPrivateProfileIntEx(sectionname, "SampleRate", 44100, iniFilePath);

	pctx->filter = GetPrivateProfileIntEx(sectionname, "FilterType", 0, iniFilePath);
	pctx->fdstype = GetPrivateProfileIntEx(sectionname, "NSFFdsType", 2, iniFilePath);
}

void Close(KMP_CTX *pctx)
{
	if (pctx->hnsf)
	{
		NSFSDK_Terminate(pctx->hnsf);
	}
	free(pctx);
}

KMP_CTX *Open(const char *file, unsigned size, unsigned srate, unsigned nch)
{
	KMP_CTX *pctx = 0;
	do
	{
		pctx = malloc(sizeof(KMP_CTX));
		if (!pctx) break;

		pctx->srate = srate;
		pctx->nch = nch;
		Setup(pctx);

		{
			/* 暫定的設定 */
			extern void FDSSelect(unsigned type);
			FDSSelect(pctx->fdstype);
		}
		if (size)
		{
			pctx->hnsf = NSFSDK_Load((void *)file, size);
			break;
		}
		else
		{
			void *fbuf;
			unsigned fsize;
			fsize = NEZ_extract((char *)file, &fbuf);
			if (fsize)
			{
				pctx->hnsf = NSFSDK_Load(fbuf, fsize);
				free(fbuf);
			}
		}
		if (!pctx->hnsf) break;
		pctx->nch = NSFSDK_GetChannel(pctx->hnsf);
		/* 古いバージョンとの互換性のためモノラルが前提となっています。 */
		/* そのためステレオ再生するためには明示的に指定する必要があります。 */
		NSFSDK_SetChannel(pctx->hnsf, pctx->nch);
		NSFSDK_SetFrequency(pctx->hnsf, pctx->srate);
		NSFSDK_SetNosefartFilter(pctx->hnsf, pctx->filter);
		return pctx;
	} while (0);
	if (pctx) Close(pctx);
	return 0;
}

unsigned Write(KMP_CTX *pctx, void *buf, unsigned smp)
{
	NSFSDK_Render(pctx->hnsf, buf, smp >> pctx->nch);
	return smp;
}

unsigned WriteSkip(KMP_CTX *pctx, unsigned smp)
{
	NSFSDK_Render(pctx->hnsf, 0, smp >> pctx->nch);
	return smp;
}

unsigned SetPosition(KMP_CTX *pctx, unsigned ms)
{
	if (ms) NSFSDK_SetSongNo(pctx->hnsf, ms / 1000);
	NSFSDK_Reset(pctx->hnsf);
	return 0;
}

int GetLoopFlag(KMP_CTX *pctx)
{
	return 1;
}

int GetSeekableFlag(KMP_CTX *pctx)
{
	return 1;
}

int GetMultisongFlag(KMP_CTX *pctx)
{
	return 1;
}

unsigned GetFrequency(KMP_CTX *pctx)
{
	return pctx->srate;
}
unsigned GetChannels(KMP_CTX *pctx)
{
	return pctx->nch;
}
unsigned GetSamplebits(KMP_CTX *pctx)
{
	return pctx->bits;
}
unsigned GetLength(KMP_CTX *pctx)
{
	return NSFSDK_GetSongMax(pctx->hnsf) * 1000;
}
unsigned GetUnitSamples(KMP_CTX *pctx)
{
	return 44100 * 60 / 1000;	/* 60msec */
}

void Init(void)
{
}
void Deinit(void)
{
}

const char **GetPluginInfo(void)
{
	static char flags[4] = { KPI_VERSION, 0, 1, 0 };
	static const char *infos[] = {
		flags,
		KPI_NAME,
		" - ",
		".nez",".nsf",".gbr",".gbs",".hes",".ay",".cpc",".nsz",
		".nsd",
		".kss",
		0
	};
	char *extp, *fntop;
	extern LPTSTR GetDLLArgv0(void);

	GetFullPathName(GetDLLArgv0(), MAX_PATH, iniFilePath, &fntop);
	for (extp = 0;*fntop;fntop++) if (*fntop == '.') extp = fntop;
	if (!extp) extp = fntop;
	lstrcpy(extp, ".ini");

	if (!GetPrivateProfileBoolEx(sectionname, "EnableKSS", TRUE, iniFilePath)) infos[3 + 9] = 0;
	return infos;
}
