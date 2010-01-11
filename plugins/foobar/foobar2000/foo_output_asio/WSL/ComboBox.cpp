
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "ComboBox.h"

SComboBox::SComboBox(SWindow* parent, int id) : SControl(parent, "combobox", id)
{
}

SComboBox::SComboBox(SWindow* parent, HWND hwnd) : SControl(parent, hwnd)
{
}

SComboBox::~SComboBox(void)
{
}

int
SComboBox::GetCount(void)
{
	return SendMessage(CB_GETCOUNT);
}

int
SComboBox::AddString(LPCSTR lpsz)
{
	return SendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpsz));
}

int
SComboBox::SetEditSel(DWORD ichStart, DWORD ichEnd)
{
	return SendMessage(CB_SETEDITSEL, 0, MAKELONG(ichStart, ichEnd));
}

DWORD
SComboBox::GetEditSel(LPDWORD lpdwStart, LPDWORD lpdwEnd)
{
	return SendMessage(CB_GETEDITSEL,
			reinterpret_cast<WPARAM>(lpdwStart), reinterpret_cast<LPARAM>(lpdwEnd));
}

int
SComboBox::SetCurSel(int index)
{
	return SendMessage(CB_SETCURSEL, index);
}

int
SComboBox::GetCurSel(void)
{
	return SendMessage(CB_GETCURSEL);
}

int
SComboBox::DeleteString(int index)
{
	return SendMessage(CB_DELETESTRING, index);
}

