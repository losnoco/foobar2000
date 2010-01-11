
#define	STRICT

#include <windows.h>
#include <commctrl.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "UpDown.h"

SUpDown::SUpDown(SWindow* parent, int id) : SControl(parent, UPDOWN_CLASS, id)
{
}

SUpDown::~SUpDown(void)
{
}

void
SUpDown::SetRange(short int nLower, short int nUpper)
{
	SendMessage(UDM_SETRANGE, 0, MAKELONG(nUpper, nLower));
}

short int
SUpDown::SetPos(short nPos)
{
	return static_cast<short int>(SendMessage(UDM_SETPOS, 0, MAKELONG(nPos, 0)));
}

short int
SUpDown::GetPos(void)
{
	return LOWORD(SendMessage(UDM_GETPOS));
}

