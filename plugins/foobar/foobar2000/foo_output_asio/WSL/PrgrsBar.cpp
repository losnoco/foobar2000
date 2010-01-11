
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "PrgrsBar.h"

SProgressBar::SProgressBar(SWindow* parent, int id) :
											SControl(parent, PROGRESS_CLASS, id)
{
}

SProgressBar::~SProgressBar(void)
{
}

UINT
SProgressBar::SetRange(UINT nMinRange, UINT nMaxRange)
{
	return SendMessage(PBM_SETRANGE, 0, MAKELONG(nMinRange, nMaxRange));
}

UINT
SProgressBar::SetPos(UINT nNewPos)
{
	return SendMessage(PBM_SETPOS, nNewPos);
}

