
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "Control.h"
#include "Edit.h"

SEdit::SEdit(SWindow* parent, int id) : SControl(parent, "edit", id)
{
}

SEdit::SEdit(SWindow* parent, HWND hwnd) : SControl(parent, hwnd)
{
}

SEdit::~SEdit(void)
{
}

void
SEdit::SetSel(int nStart, int nEnd)
{
	SendMessage(EM_SETSEL, nStart, nEnd);
}

bool
SEdit::ScrollCaret(void)
{
	return SendMessage(EM_SCROLLCARET) != 0;
}

