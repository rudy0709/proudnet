#pragma once

#include "enumimpl.h"
#include "ThreadPoolImpl.h"
#include "GlobalTimerThread.h"
#include "../include/SysTime.h"

namespace Proud
{
	class CThreadPoolImpl;
	class IThreadReferrer;

	// 일정 시간마다 thread pool에 custom event를 post하는 역할을 한다.
	class CThreadPoolPeriodicPoster
	{
		CGlobalTimerThread::PtrType m_globalTimerThread;

		// custom event의 주체
		IThreadReferrer* m_threadPoolReferrer;
		CustomValueEvent m_postType;
		CThreadPoolImpl* m_threadPool;
		TimerEventHandle m_timerEventHandle;

		static void PosterProc(void* context);
		void PosterProc2();
	public:
		 PROUD_API CThreadPoolPeriodicPoster(
			IThreadReferrer* threadPoolReferrer,
			CustomValueEvent postType,
			CThreadPoolImpl* threadpool,
			int64_t postTime);

		 PROUD_API ~CThreadPoolPeriodicPoster();

		 void SetPostInterval(const int64_t &newInterval);
	};
}
