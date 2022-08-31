#include "stdafx.h"

// POSIX 지원 플랫폼에서 표준 유닉스 헤더파일 포함
#if defined(__linux__) || defined(__FreeBSD__) || defined(__MAMALADE__)
#include <unistd.h>
#endif

#include "../include/SysTime.h"
//#include "GlobalTimerThread.h"
#ifndef _WIN32
	#include <unistd.h>
#endif

namespace Proud
{
	void Sleep(int sleepTimeMs)
	{
#ifdef _WIN32
		::Sleep((DWORD)sleepTimeMs);
#else
		::usleep(sleepTimeMs * 1000);
#endif
	}
}
