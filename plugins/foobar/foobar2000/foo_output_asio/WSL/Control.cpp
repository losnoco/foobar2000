
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"

SControl::SControl(SWindow* parent, char* classname, int id) :
						SWindow(parent, NULL, classname, id, ControlMode_Control)
{
	Attr.Style |= WS_CHILD;
}

SControl::SControl(SWindow* parent, HWND hwnd) :
						SWindow(parent, NULL, NULL, 0, ControlMode_Control)
{
	HWindow = hwnd;
}

SControl::~SControl(void)
{
}

