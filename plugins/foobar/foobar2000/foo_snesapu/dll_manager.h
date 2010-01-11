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

#include <foobar2000.h>

class dll_manager
{
private:
	critical_section sync;

	typedef struct tagDLLINFO
	{
		HMODULE			handle;
		string_simple	path;
		tagDLLINFO(HMODULE p_handle, const char * p_path) : handle(p_handle), path(p_path) {}
	} DLLINFO;

	ptr_list_t<DLLINFO> dlls;

public:
	dll_manager() {}
	~dll_manager();

	HMODULE load(const char * name, bool unique = false);
	BOOL free(HMODULE hModule);
};
