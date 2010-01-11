// Windows/System.h

#pragma once

#ifndef __WINDOWS_SYSTEM_H
#define __WINDOWS_SYSTEM_H

#include "../Common/String.h"

namespace NWindows {
namespace NSystem {

bool MyGetWindowsDirectory(CSysString &path);
bool MyGetSystemDirectory(CSysString &path);

}}

#endif
