/*
 * DLL Manager class v1.1 - 2005-05-24 10:24 UTC 
 *
 * "Clean" multi-instance DLL loader for libraries which only support one static
 * global instance internally.
 * 
 * Without the unique flag, it acts as little more than a synchronized DLL loader.
 *
 * When unique is set, modules which are already imported into the current address
 * space will be copied to a new file so the import will get a fresh instance of
 * the module. When the handle is passed to free(), or when the dll_manager is
 * destroyed, all temporary modules will be unloaded and destroyed.
 *
 *   Copyright (C) 2003-2005, Chris Moeller,
 *   All rights reserved.                          
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *     3. The names of its contributors may not be used to endorse or promote 
 *        products derived from this software without specific prior written 
 *        permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "dll_manager.h"

dll_manager::~dll_manager()
{
	unsigned n = dlls.get_count();
	if (n)
	{
		unsigned i;
		DLLINFO * dll;
		for (i = 0; i < n; i++)
		{
			dll = dlls[i];
			//uOutputDebugString(string_printf("DLL file leaked: %s - refcount: %d", dll->name, dll->refcount));
			FreeLibrary(dll->handle);
			uDeleteFile(dll->path);
		}
		dlls.delete_all();
	}
}

HMODULE dll_manager::load(const char * name, bool unique)
{
	insync(sync);

	if (unique)
	{
		HMODULE hModule = uGetModuleHandle(name);

		if (hModule)
		{
			string8 path, temp;
			if (!uGetModuleFileName(hModule, path)) return NULL;

			uGetTempPath(temp);
			uGetTempFileName(temp, string_filename(path), 0, temp);

			BOOL rval;

			if (IsUnicode())
				rval = CopyFileW(string_utf16_from_utf8(path), string_utf16_from_utf8(temp), FALSE);
			else
				rval = CopyFileA(string_ansi_from_utf8(path), string_ansi_from_utf8(temp), FALSE);

			if (!rval) return NULL;

			hModule = uLoadLibrary(temp);

			if (!hModule)
			{
				uDeleteFile(temp);
				return NULL;
			}

			dlls.add_item(new DLLINFO(hModule, temp));

			return hModule;
		}
	}

	return uLoadLibrary(name);
}

BOOL dll_manager::free(HMODULE hModule)
{
	insync(sync);

	//uOutputDebugString(string_printf("free(%x)", hModule));

	BOOL rval = FreeLibrary(hModule);

	unsigned n = dlls.get_count();
	if (n)
	{
		unsigned i;
		DLLINFO * dll;
		for (i = 0; i < n; i++)
		{
			dll = dlls[i];
			if (hModule == dll->handle)
			{
				uDeleteFile(dll->path);
				dlls.delete_by_idx(i);
				break;
			}
		}
	}

	return rval;
}
