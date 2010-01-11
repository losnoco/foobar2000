#include "uxtheme.h"

#if 0
unsigned uxtheme_helper::ref = 0;
HINSTANCE uxtheme_helper::hinst_uxtheme = 0;
ENABLETHEMEDIALOGTEXTUREPROC uxtheme_helper::pETDT = 0;


bool uxtheme_helper::add_ref()
{
	if (ref == 0)
	{
		hinst_uxtheme = uLoadLibrary("uxtheme.dll");
	}

	ref++;

	return hinst_uxtheme !=0;

}

void uxtheme_helper::release() {
	ref--;

	if (!ref)
	{
		
			if (hinst_uxtheme) FreeLibrary(hinst_uxtheme);
			hinst_uxtheme = NULL;
			pETDT = NULL;
		
	}
}

HRESULT uxtheme_helper::EnableThemeDialogTexture(HWND wnd, DWORD flags)
{
	HRESULT rv = E_FAIL;
	if (hinst_uxtheme)
	{
		if (!pETDT)
		{
			pETDT = (ENABLETHEMEDIALOGTEXTUREPROC)GetProcAddress(hinst_uxtheme, "EnableThemeDialogTexture");
		}
		if (pETDT) rv = (*pETDT)(wnd, flags);
	}
	return rv;
}
#endif

uxtheme_handle::uxtheme_handle(HINSTANCE inst) : inst_uxtheme(inst), p_etdt(0)
	{};
uxtheme_handle::~uxtheme_handle() {};
uxtheme_handle * uxtheme_handle::create()
{
	uxtheme_handle * rv = 0;
	HINSTANCE inst = uLoadLibrary("uxtheme.dll");
	if (inst)
	{
		rv = new uxtheme_handle(inst);
	}
	return rv;
	}

void uxtheme_handle::release()
{
	FreeLibrary(inst_uxtheme);
	delete this;
}

HRESULT uxtheme_handle::enable_theme_dialogue_texture(HWND wnd, DWORD dw_flags)
{
	HRESULT rv = E_FAIL;
	if (!p_etdt)
	{
		p_etdt = (ENABLETHEMEDIALOGTEXTUREPROC)GetProcAddress(inst_uxtheme, "EnableThemeDialogTexture");
	}
	if (p_etdt) rv = (*p_etdt)(wnd, dw_flags);
	return rv;
}
