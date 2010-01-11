
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "Tab.h"

STab::STab(SWindow* parent, int id) : SControl(parent, WC_TABCONTROL, id)
{
	Attr.Style |= WS_CLIPSIBLINGS;
}

STab::~STab(void)
{
}

bool
STab::InsertItem(int iItem, TC_ITEM* pitem)
{
	return SendMessage(TCM_INSERTITEM, iItem, reinterpret_cast<LPARAM>(pitem)) != 0;
}

bool
STab::SetItem(int iItem, TC_ITEM* pitem)
{
	return SendMessage(TCM_SETITEM, iItem, reinterpret_cast<LPARAM>(pitem)) != 0;
}

bool
STab::Add(char* text)
{
	TC_ITEM	Item;

	Item.mask = TCIF_TEXT;
	Item.pszText = text;
	Item.cchTextMax = 0;

	return InsertItem(GetItemCount(), &Item);
}

int
STab::GetItemCount(void)
{
	return SendMessage(TCM_GETITEMCOUNT);
}

int
STab::SetCurSel(int iItem)
{
	return SendMessage(TCM_SETCURSEL, iItem);
}

int
STab::GetCurSel(void)
{
	return SendMessage(TCM_GETCURSEL);
}

