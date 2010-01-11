
#define	STRICT

#include <windows.h>

#include "WSL.h"
#include "Window.h"
#include "WinAdmin.h"

SWindowAdmin::SWindowAdmin(void)
{
	List = NULL;
}

SWindowAdmin::~SWindowAdmin(void)
{
	SWindow_List*	DeleteList = List;

	while(DeleteList) {
		SWindow_List*	NextList = DeleteList->Next;

		delete DeleteList;

		DeleteList = NextList;
	}
}

void
SWindowAdmin::Add(SWindow* Window)
{
	SWindow_List*	AddList = new SWindow_List;
	SWindow_List*	LastList = GetLast();

	if(LastList) {
		LastList->Next = AddList;
		AddList->First = false;
	} else {
		AddList->First = true;
		List = AddList;
	}

	AddList->Before = LastList;
	AddList->Next = NULL;
	AddList->Window = Window;
}

void
SWindowAdmin::Delete(SWindow* Window)
{
	SWindow_List*	DeleteList = GetList(Window);

	if(DeleteList == NULL) return;

	SWindow_List*	Before = DeleteList->Before;
	SWindow_List*	Next = DeleteList->Next;

	delete DeleteList;

	if(Before) Before->Next = Next;
	if(Next) Next->Before = Before;

	if(Before == NULL) List = Next ? Next : NULL;
}

bool
SWindowAdmin::IsWindow(void)
{
	return List != NULL;
}

bool
SWindowAdmin::IsFirstList(SWindow* Window)
{
	return List && (List->Window == Window);
}

bool
SWindowAdmin::IsFirst(SWindow* Window)
{
	return List && GetList(Window)->First;
}

SWindow_List*
SWindowAdmin::GetLast(void)
{
	SWindow_List*	EditList = List;

	while(EditList && EditList->Next) {
		EditList = EditList->Next;
	}

	return EditList;
}

SWindow_List*
SWindowAdmin::GetList(SWindow* Window)
{
	if(List == NULL) return NULL;

	SWindow_List*	EditList = List;

	do {
		if(EditList->Window == Window) break;
		EditList = EditList->Next;
	} while(EditList);

	return EditList;
}

