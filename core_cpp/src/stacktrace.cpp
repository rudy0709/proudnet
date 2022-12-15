#include "stdafx.h"
#include "../include/stacktrace.h"
#ifdef _WIN32
#include "stacktrace-win32.h"
#else 
#include "stacktrace-unix.h"
#endif

namespace Proud
{
	// 현재 실행 지점의 call stack을 output에 채운다.
	void GetStackTrace(CFastArray<String>& output)
	{
#ifdef _WIN32
		GetStackTrace_Win32(output);
#elif !defined(__ANDROID__)
		GetStackTrace_Unix(output);
#endif
	}
}

