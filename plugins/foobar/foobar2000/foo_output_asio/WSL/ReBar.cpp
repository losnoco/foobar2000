
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "ReBar.h"

SReBar::SReBar(SWindow* parent, int id) : SControl(parent, REBARCLASSNAME, id)
{
	Attr.Style |= WS_CLIPCHILDREN;
}

SReBar::~SReBar(void)
{
}

bool
SReBar::InsertBand(UINT index, LPREBARBANDINFO BandInfo)
{
	return SendMessage(RB_INSERTBAND, index,
							reinterpret_cast<LPARAM>(BandInfo)) != 0;
}

bool
SReBar::DeleteBand(UINT index)
{
	return SendMessage(RB_DELETEBAND, index) != 0;
}

bool
SReBar::SetBandInfo(UINT index, LPREBARBANDINFO BandInfo)
{
	return SendMessage(RB_SETBANDINFO, index,
							reinterpret_cast<LPARAM>(BandInfo)) != 0;
}

bool
SReBar::GetBandInfo(UINT index, LPREBARBANDINFO BandInfo)
{
	return SendMessage(RB_GETBANDINFO, index,
							reinterpret_cast<LPARAM>(BandInfo)) != 0;
}

UINT
SReBar::IdToIndex(UINT id)
{
	return SendMessage(RB_IDTOINDEX, id);
}

void
SReBar::ShowBand(UINT index, bool show)
{
	SendMessage(RB_SHOWBAND, index, show);
}

