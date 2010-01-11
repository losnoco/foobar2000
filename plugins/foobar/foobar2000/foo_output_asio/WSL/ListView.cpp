
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "ListView.h"

// SlistWindItem

SListWindItem::SListWindItem(int subItem)
{
	Init();
	SetSubItem(subItem);
}

SListWindItem::SListWindItem(const char* text, int subItem, int len)
{
	Init();
	SetText(text, len);
	SetSubItem(subItem);
}

void
SListWindItem::SetIndex(int index)
{
	iItem = index;
}

int
SListWindItem::GetIndex(void) const
{
	return iItem;
}

void
SListWindItem::SetSubItem(int subItem)
{
	iSubItem = subItem;
}

int
SListWindItem::GetSubItem(void) const
{
	return iSubItem;
}

void
SListWindItem::SetText(const char* buffer, int len)
{
	mask      |= LVIF_TEXT;
	pszText    = const_cast<char*>(buffer);
	cchTextMax = len;
}

void
SListWindItem::SetImageIndex(int index)
{
	mask  |= LVIF_IMAGE;
	iImage = index;
}

int
SListWindItem::GetImageIndex(void) const
{
	return (mask & LVIF_IMAGE) ? iImage : 0;
}

void
SListWindItem::SetItemData(UINT data)
{
	mask  |= LVIF_PARAM;
	lParam = data;
}

UINT
SListWindItem::GetItemData(void) const
{
	return (mask & LVIF_PARAM) ? lParam : 0;
}

void
SListWindItem::Init(void)
{
	memset(static_cast<LVITEM*>(this), 0, sizeof LVITEM);
}

// SListWindColumn

SListWindColumn::SListWindColumn(char* text, int width, UINT how, int subItem)
{
	Init();
	SetText(text);
	SetFormat(how);
	SetWidth(width);
	SetSubItem(subItem);
}

void
SListWindColumn::SetFormat(UINT how)
{
	mask |= LVCF_FMT;
	fmt   = how;
}

void
SListWindColumn::SetWidth(int pixels)
{
	mask |= LVCF_WIDTH;
	cx    = pixels;
}

void
SListWindColumn::SetText(const char* text, int size)
{
	mask      |= LVCF_TEXT;
	pszText    = const_cast<char*>(text);
	cchTextMax = size;
}

void
SListWindColumn::SetSubItem(int subItem)
{
	mask    |= LVCF_SUBITEM;
	iSubItem = subItem;
}

void
SListWindColumn::Init(void)
{
	memset(static_cast<LVCOLUMN*>(this), 0, sizeof LVCOLUMN);
}

// SListView

SListView::SListView(SWindow* parent, int id) : SControl(parent, WC_LISTVIEW, id)
{
	Attr.Style |= WS_CLIPCHILDREN;
}

SListView::~SListView(void)
{
}

void
SListView::SetExtendedListViewStyle(UINT Style)
{
	SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, Style);
}

UINT
SListView::GetExtendedListViewStyle(void)
{
	return SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
}

HWND
SListView::GetHeader(void)
{
	return reinterpret_cast<HWND>(SendMessage(LVM_GETHEADER));
}

int
SListView::InsertColumn(int iCol, const SListWindColumn& colItem)
{
	return SendMessage(LVM_INSERTCOLUMN, iCol, reinterpret_cast<LPARAM>(&colItem));
}

bool
SListView::GetColumn(int iCol, LV_COLUMN* column)
{
	return SendMessage(LVM_GETCOLUMN, iCol, reinterpret_cast<LPARAM>(column)) != 0;
}

bool
SListView::SetColumnWidth(int iCol, int cx)
{
	return SendMessage(LVM_SETCOLUMNWIDTH, iCol, MAKELPARAM(cx, 0)) != 0;
}

int
SListView::GetColumnWidth(int iCol) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETCOLUMNWIDTH, iCol);
}

/*
void
SListView::SetSelectedColumn(int iCol)
{
	SendMessage(LVM_SETSELECTEDCOLUMN, iCol);
}
*/

int
SListView::InsertItem(SListWindItem& item, int index)
{
	if(index != -1) item.iItem = index;

	return SendMessage(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
}

bool
SListView::SetItem(SListWindItem& item, int index, int subItem)
{
	if(index != -1) item.iItem = index;
	if(subItem != -1) item.iSubItem = subItem;

	return SendMessage(LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item)) != 0;
}

bool
SListView::GetItem(SListWindItem& item, int index, int subItem) const
{
	if(index != -1) item.iItem = index;
	if(subItem != -1) item.iSubItem = subItem;

	return const_cast<SListView*>(this)->SendMessage(LVM_GETITEM, 0,
										reinterpret_cast<LPARAM>(&item)) != 0;
}

void
SListView::SetItemCount(int cItems)
{
	SendMessage(LVM_SETITEMCOUNT, cItems);
}

int
SListView::GetItemCount(void) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETITEMCOUNT);
}

UINT
SListView::GetSelectedCount(void) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETSELECTEDCOUNT);
}

bool
SListView::GetItemPosition(int index, POINT* p)
{
	return SendMessage(LVM_GETITEMPOSITION, index, reinterpret_cast<LPARAM>(p)) != 0;
}

bool
SListView::SetItemState(int index, UINT state, UINT mask)
{
	LVITEM	item;

	item.state = state;
	item.stateMask = mask;

	return SendMessage(LVM_SETITEMSTATE, index, reinterpret_cast<LPARAM>(&item)) != 0;
}

UINT
SListView::GetItemState(int index, UINT mask) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETITEMSTATE,
															index, mask);
}

int
SListView::GetItemText(int index, int subItem, char* text, int size) const
{
	SListWindItem	item(text, subItem, size);

	return const_cast<SListView*>(this)->SendMessage(LVM_GETITEMTEXT,
										index, reinterpret_cast<LPARAM>(&item));
}

bool
SListView::DeleteAllItems(void)
{
	return SendMessage(LVM_DELETEALLITEMS) != 0;
}

int
SListView::GetTopIndex(void) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETTOPINDEX);
}

int
SListView::HitTest(POINT& Pnt, UINT flags)
{
	LVHITTESTINFO	lvhst;

	lvhst.pt    = Pnt;
	lvhst.flags = flags;

	return SendMessage(LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&lvhst));
}

/*
int
SListView::SubItemHitTest(SLwHitTestInfo& info)
{
	LVHITTESTINFO	lvhst;

	lvhst.pt    = Pnt;
	lvhst.flags = flags;

	return SendMessage(LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&info));
}
*/

bool
SListView::EnsureVisible(int index, bool partialOk)
{
	return SendMessage(LVM_ENSUREVISIBLE, index, partialOk) != 0;
}

int
SListView::GetCountPerPage(void) const
{
	return const_cast<SListView*>(this)->SendMessage(LVM_GETCOUNTPERPAGE);
}

HWND
SListView::EditLabel(int index)
{
	return reinterpret_cast<HWND>(SendMessage(LVM_EDITLABEL, index));
}

bool
SListView::Update(int index)
{
	return SendMessage(LVM_UPDATE, index) != 0;
}

int
SListView::SetHotItem(int index)
{
	return SendMessage(LVM_SETHOTITEM, index);
}

int
SListView::GetHotItem(void)
{
	return SendMessage(LVM_GETHOTITEM);
}

HCURSOR
SListView::SetHotCursor(HCURSOR hcur)
{
	return reinterpret_cast<HCURSOR>
			(SendMessage(LVM_SETHOTCURSOR, 0, reinterpret_cast<LPARAM>(hcur)));
}

