#include "stdafx.h"

// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <stack>

#include "GlobalTimerThread.h"
#include "CollUtil.h"

#ifdef __MARMALADE__
#include "s3eTimer.h"
#endif

#if !defined(_WIN32)
#include <sys/time.h>
#include <time.h>
#endif

namespace Proud
{
	// 시간값을 구한다.
	// 주의: CPU 0 스레드에서만 호출할 것. QPC 때문.
	int64_t GetSystemTime_INTERNAL(void* outTimeVal, void* outTimeSpec)
	{
#if defined(_WIN32)
		{
			// 위에서 CPU 0 affinity를 얻었으므로 안전하다.
			LARGE_INTEGER t, q;
			t.QuadPart = 0;
			q.QuadPart = 0;
			QueryPerformanceCounter(&t);
			QueryPerformanceFrequency(&q);
			if (q.QuadPart == 0)
				throw Exception("FATAL! Cannot use QPF!");
			return t.QuadPart / (q.QuadPart / 1000);
		}
#else
		{
			timeval &time = *(timeval*)outTimeVal;
			int rv = gettimeofday(&time, NULL);
			if(rv == -1)
				throw Exception("FATAL! Cannot use gettimeofday!");

			timespec* ts = (timespec*)outTimeSpec;
			ts->tv_sec = time.tv_sec;
			ts->tv_nsec = (time.tv_usec % 1000) * 1000;

#if defined(__MARMALADE__)
			/*Returns the Uninterrupted System Time (UST) in milliseconds. This is the number of milliseconds that have passed since Marmalade started running*/
			return s3eTimerGetMs();  // 49일 이상 켜는 것이 아닌 이상, 안전하다.
#else
			return ((int64_t)time.tv_sec*1000) + time.tv_usec/1000;
#endif
		}
#endif
	}

	CGlobalTimerThread::CGlobalTimerThread(void):m_thread(ThreadProc, this)
	{
		m_stopThread = false;
		m_baseTimeMs = 0;

		// timer 스레드를 생성한다.
		m_thread.Start();
	}


	CGlobalTimerThread::~CGlobalTimerThread(void)
	{
		// 스레드를 종료시킨다.
		m_stopThread = true;
		m_thread.Join();

		// 클리어
		for(CFastMap2<CTask*, CTask*, int>::iterator i=m_tasks.begin();i!=m_tasks.end();i++)
		{
			CTask* t = i->GetSecond();
			delete t;
		}
		m_tasks.Clear();
	}

	void CGlobalTimerThread::ThreadProc( void* context )
	{
		CGlobalTimerThread* main = (CGlobalTimerThread*)context;
		main->ThreadProc2();
	}

	void CGlobalTimerThread::ThreadProc2()
	{
#ifdef _WIN32
		// BIOS에 버그가 있는 일부 하드웨어는 QPC가 잘못된 값을 주는데 이를 예방하기 위해,
		// timer thread에서는 CPU 0 affinity를 설정해야 한다. (win32 only)
		SetThreadAffinityMask(m_thread.GetHandle(), 0x1); // CPU 0 only
#endif // _WIN32

		while(!m_stopThread)
		{
			int64_t currTime = UpdateCachedTime();

			{
				CriticalSectionLock lock(m_critSec, true);
				// 매우 짧은 시간 동안 쉰다.
				// 등록된 task들을 뒤지면서, 때가 된 것들에 대한 이벤트 콜백을 실행한다.
				for(CFastMap2<CTask*, CTask*, int>::iterator i=m_tasks.begin();i!=m_tasks.end();i++)
				{
					CTask* t = i->GetSecond();
					if(currTime > t->m_timeToDo)
					{
						t->m_callback(t->m_callbackContext);
						t->m_timeToDo += t->m_interval;
					}
				}
			}

			Proud::Sleep(1);
		}
	}

	// 주의: 여기 등록되는 타이머 콜백은 매우 작은 일만 하고 즉시 리턴해야 한다. 안그러면 전체적인 타이머 처리가 오차가 커진다.
	// 예를 들어, 변수 트리거 혹은 이벤트 트리거 정도만 하기
	// 리턴값은 핸들이다. delete해서 파괴하는 것이 아님을 주의할 것
	TimerEventHandle CGlobalTimerThread::TimerMiniTask_Add( const int64_t &interval, TaskFunction callback, void* callbackContext )
	{
		int64_t currTime = GetCachedTime();

		CriticalSectionLock lock(m_critSec, true);

		assert(interval > 0);
		CTask* t = new CTask;
		t->m_interval = interval;
		t->m_callback = callback;
		t->m_callbackContext = callbackContext;
		t->m_timeToDo = currTime; // 이렇게 세팅해야, 초기에 엄청난 오실행 예방
		m_tasks.Add(t, t);
		
		TimerEventHandle r;
		r.m_internal = (intptr_t)t;
		return r;
	}

	// TimerMiniTask_Add의 반대 처리
	// mini task를 실행하는 루프와 동시에 실행되지 않으며, 이 함수가 리턴한 직후에는 등록됐던 task가 실행되지 않음이 보장된다.
	void CGlobalTimerThread::TimerMiniTask_Remove( TimerEventHandle event )
	{
		CriticalSectionLock lock(m_critSec, true);

		CTask* t;
		if(m_tasks.TryGetValue((CTask*)event.m_internal, t))
		{
			m_tasks.RemoveKey((CTask*)event.m_internal);
			delete t;
		}
	}

	int64_t CGlobalTimerThread::GetCachedTime()
	{
		// 		// 이 함수는 코트 프로필링 결과 꽤 잦게 사용된다.
		// 		// 따라서, x64에서는 더 나은 성능을 위해 GetPreciseCurrentTime은 critsec을 쓰지 않고 atomic op을 쓴다. 
		// 		// x86에서 64bit atomic op 함수가 쓸 수 없더라.
		// #if defined(_WIN64) || defined(__LP64__)
		// 		{
		// 			int64_t ret;
		// 			ret = AtomicCompareAndSwap64(0, 0, &m_measuredTimeMs);
		// 			return ret;
		// 		}
		// #else
		// 		{
		CriticalSectionLock lock(m_timeCritSec, true);

		// 아직 타이머 스레드가 시작을 막 하는 순간에 사용자가 GetPreciseCurrentTime을 마구 호출하는 경우 
		// 계속해서 0이 리턴하는 문제가 있었다. 이 문제를 막기 위해
		if(m_baseTimeMs == 0)
			return UpdateCachedTime();

		return m_measuredTimeMs;
		// 		}
		// #endif
	}

#if !defined(_WIN32)
	void CGlobalTimerThread::GetAbsoluteTimeSpec(timespec* outTimeSpec)
	{
		CriticalSectionLock lock(m_critSec, true);
		*outTimeSpec = m_absoluteTimeSpec;
	}
#endif // _WIN32

	// 이미 생성했던 mini task의 주기를 갱신한다.
	void CGlobalTimerThread::TimerMiniTask_SetInterval( TimerEventHandle event, const int64_t &interval )
	{
		assert(interval > 0);
		
		CriticalSectionLock lock(m_critSec, true);

		CTask* t;
		if(m_tasks.TryGetValue((CTask*)event.m_internal, t))
		{
			t->m_interval = interval;
		}

	}

	// 현재 시간을 시스템으로부터 구해서 cached time을 갱신하고 현재 시간 값을 리턴한다.
	int64_t CGlobalTimerThread::UpdateCachedTime()
	{
		// NOTE: QPC로 시간부터 얻고 lock을 걸자. 최소한의 contention을 위해.
		void* pTimeVal = NULL;
		void* pTimeSpec = NULL;
#if !defined(_WIN32)
		pTimeVal = &m_absoluteTimeVal;
		pTimeSpec = &m_absoluteTimeSpec;
#endif

		int64_t t = GetSystemTime_INTERNAL(pTimeVal, pTimeSpec);

		CriticalSectionLock lock(m_timeCritSec, true);

		// 정밀 현재 시간을 구한다. 이는 GetPreciseCurrentTime에 의해 사용된다.
		if(m_baseTimeMs == 0)
		{
			m_baseTimeMs = t;
		}

		int64_t currTime = t - m_baseTimeMs;

		// #if defined(_WIN64) || defined(__LP64__)
		// 				AtomicExchange64(currTime, &m_measuredTimeMs);
		// #else
		m_measuredTimeMs = currTime;
		//#endif

		return currTime;
	}


}
