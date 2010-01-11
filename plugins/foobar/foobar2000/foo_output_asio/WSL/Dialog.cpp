
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "Dialog.h"

SDialog::SDialog(SWindow* parent, LPCSTR id) : SWindow(parent, NULL, NULL, 0,
															ControlMode_Dialog)
{
	Attr.DlgId = id;
}

SDialog::SDialog(SWindow* parent, int id) : SWindow(parent, NULL, NULL, 0,
															ControlMode_Dialog)
{
	Attr.DlgId = MAKEINTRESOURCE(id);
}

SDialog::SDialog(HWND parent, LPCSTR id) : SWindow(parent, NULL, NULL, 0,
															ControlMode_Dialog)
{
	Attr.DlgId = id;
}

SDialog::SDialog(HWND parent, int id) : SWindow(parent, NULL, NULL, 0,
															ControlMode_Dialog)
{
	Attr.DlgId = MAKEINTRESOURCE(id);
}

SDialog::~SDialog(void)
{
}

void
SDialog::CmOk(void)
{
	CloseWindow(IDOK);
}

void
SDialog::CmCancel(void)
{
	CloseWindow(IDCANCEL);
}

void
SDialog::WmCommand(Org_Mes* OrgMes, int wNotifyCode, int wID, HWND hwndCtl)
{
	OrgMes->ExecMessage = true;
	SWindow::WmCommand(OrgMes, wNotifyCode, wID, hwndCtl);

	if(wNotifyCode == BN_CLICKED) {
		switch(wID) {
		case IDOK:
			CmOk();
			break;
		case IDCANCEL:
			CmCancel();
			break;
		}
	}
}

