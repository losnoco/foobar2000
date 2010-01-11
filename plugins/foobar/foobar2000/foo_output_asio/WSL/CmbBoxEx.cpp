
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "CmbBoxEx.h"

SComboBoxEx::SComboBoxEx(SWindow* parent, int id) :
											SControl(parent, WC_COMBOBOXEX, id)
{
}

SComboBoxEx::~SComboBoxEx(void)
{
}

bool
SComboBoxEx::InsertItem(UINT index, PCOMBOBOXEXITEM ItemInfo)
{
	return SendMessage(CBEM_INSERTITEM, index, reinterpret_cast<LPARAM>(ItemInfo)) != 0;
}

HWND
SComboBoxEx::GetComboControl(void)
{
	return reinterpret_cast<HWND>(SendMessage(CBEM_GETCOMBOCONTROL));
}

