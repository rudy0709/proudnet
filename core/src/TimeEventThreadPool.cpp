#include "stdafx.h"
#include "TimeEventThreadPool.h"
//#include "ThreadPoolImpl.h"


namespace Proud
{
	// 등록되었던 주기적 이벤트 트리거의 시간 간격을 변경한다.
	void CThreadPoolPeriodicPoster::SetPostInterval(const int64_t &newInterval)
	{
		m_globalTimerThread->TimerMiniTask_SetInterval(m_timerEventHandle, newInterval);
	}

	// 일정 시간마다 IThreadPoolReferrer를 위한 custom event가 
	// 발생하게 한다.
	CThreadPoolPeriodicPoster::CThreadPoolPeriodicPoster(
		IThreadReferrer* threadPoolReferrer,
		CustomValueEvent postType,
		CThreadPoolImpl* threadpool,
		int64_t postTime)
	{
		m_globalTimerThread = CGlobalTimerThread::GetSharedPtr();

		m_threadPoolReferrer = threadPoolReferrer;
		m_postType = postType;
		m_threadPool = threadpool;

		m_timerEventHandle = m_globalTimerThread->TimerMiniTask_Add(
			(postTime), PosterProc, this);

	}

	// 이벤트를 post하는 역할을 하는 함수
	void CThreadPoolPeriodicPoster::PosterProc(void* context)
	{
		CThreadPoolPeriodicPoster* main = (CThreadPoolPeriodicPoster*)context;
		main->PosterProc2();

	}

	// 이벤트를 post하는 역할을 하는 함수
	void CThreadPoolPeriodicPoster::PosterProc2()
	{
		m_threadPool->PostCustomValueEvent(m_threadPoolReferrer, m_postType);
	}

	// 파괴자에서는 생성자에서 했던 등록을 해제한다.
	CThreadPoolPeriodicPoster::~CThreadPoolPeriodicPoster()
	{
		m_globalTimerThread->TimerMiniTask_Remove(m_timerEventHandle);
	}

}