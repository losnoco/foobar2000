#ifndef _COLUMNS_UXTHEME_H_
#define _COLUMNS_UXTHEME_H_

#include "../sdk/foobar2000.h"

typedef HRESULT (WINAPI * ENABLETHEMEDIALOGTEXTUREPROC)(HWND, DWORD);

#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

#if 0
class uxtheme_helper
{
private:
	static unsigned ref;
	static HINSTANCE hinst_uxtheme ;
	static ENABLETHEMEDIALOGTEXTUREPROC pETDT ;

public:
	static bool add_ref(); //call release() even if this returns false
	static void release();

	static HRESULT EnableThemeDialogTexture(HWND wnd, DWORD flags);
};
#endif

class uxtheme_handle
{
	HINSTANCE inst_uxtheme;
	ENABLETHEMEDIALOGTEXTUREPROC p_etdt;
protected:
	uxtheme_handle(HINSTANCE inst);
	~uxtheme_handle();
public:
	static uxtheme_handle * create(); //may return null if uxtheme not present.
	void release();
	HRESULT enable_theme_dialogue_texture(HWND wnd, DWORD dw_flags);
};
#endif