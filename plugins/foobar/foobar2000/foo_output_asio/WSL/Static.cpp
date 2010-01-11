
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "Static.h"

SStatic::SStatic(SWindow* parent, int id) : SControl(parent, "static", id)
{
}

SStatic::~SStatic(void)
{
}

