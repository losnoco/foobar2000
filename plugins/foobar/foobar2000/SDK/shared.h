//DEPRECATED

#ifndef _FOOBAR2000_SHARED_H_
#define _FOOBAR2000_SHARED_H_

#include "../shared/shared.h"

HWND uCreateDialog(UINT id,HWND parent,DLGPROC proc,long param=0);
int uDialogBox(UINT id,HWND parent,DLGPROC proc,long param=0);

#endif