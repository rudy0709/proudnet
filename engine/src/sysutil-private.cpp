#include "stdafx.h"
#include "sysutil-private.h"

#if !defined(_WIN32)
	#include <sched.h>
#endif

namespace Proud
{
	// 현재 스레드를 컨텍스트 스위칭해버린다.
	void YieldThread()
	{
#if defined(_WIN32)
		SwitchToThread();
#else
		sched_yield();
#endif //defined(_WIN32)
	}
}
