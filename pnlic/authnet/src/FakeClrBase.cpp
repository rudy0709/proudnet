/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/sysutil.h"
#include <new>

#if defined(_WIN32)
    #include <process.h>
#else
    #include <errno.h>
#endif

#include "../include/FakeClrBase.h"
#include "../include/FakeCLRRandom.h"
#include "GlobalTimerThread.h"

namespace Proud
{
	CriticalSection g_guid_random_CS;
	Random g_guid_random;
    
	PROUD_API  Guid Guid::RandomGuid()
	{
		CriticalSectionLock lock(g_guid_random_CS,true);
        
		return g_guid_random.NextGuid();
	}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

#ifdef _WIN32
	void Timer::EventSetter(void* ctx)
	{
		Timer* t = (Timer*)ctx;
		::SetEvent(t->m_eventHandle);
	}

	Timer::Timer( HANDLE eventHandle, uint32_t interval, DWORD_PTR dwUser)
	{
		assert(interval > 0);
		m_eventHandle = eventHandle;
		m_timerID = CGlobalTimerThread::Instance().TimerMiniTask_Add(interval, EventSetter, this);
	}

	Timer::~Timer()
	{
		CGlobalTimerThread::Instance().TimerMiniTask_Remove(m_timerID);
	}
#endif
}