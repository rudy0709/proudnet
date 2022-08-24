#pragma once

#include "ProudNetServer.h"

#if defined(_WIN32)

#include "MiniDumper.h"

#define PN_STDCALL __stdcall				// msvc: stdcall, other: cdecl or something

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")

#else //defined(_WIN32)
	
#define PN_STDCALL
#include "MiniDumpAction.h"

#endif //defined(_WIN32)