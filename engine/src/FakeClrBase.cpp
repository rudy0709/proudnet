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

	/* 사용자가 잘못된 버전의 라이브러리,헤더파일을 뒤섞어 쓰는 경우를 막기 위해 추가되었다.

	가령, Linux Debug 라이브러리를 갖다 쓰는 앱 A가 있다.
	A에서는 실수로 _DEBUG를 안 선언하고, _DEBUG를 선언하고 빌드됐던 라이브러리 즉 디버그 전용 라이브러리를 갖다 썼다.
	이때 다발 에러가 발생한다. 가령 CMessageType의 Test splitter 변수 자체가 없는데 CMessage의 inline 생성자 함수가 거기다 건드리면서 메모리를 그어버린다.
	따라서 이렇게 해서, 잘못된 버전을 건드리게 되면 link error가 나게 만들었다.
	*/
	volatile int PROUDNET_H_LIB_SIGNATURE = 0;

	Guid Guid::RandomGuid()
	{
		CriticalSectionLock lock(g_guid_random_CS,true);

		return g_guid_random.NextGuid();
	}


#ifdef _WIN32
	void Timer::EventSetter(void* ctx)
	{
		Timer* t = (Timer*)ctx;
		::SetEvent(t->m_eventHandle);
	}

	Timer::Timer( HANDLE eventHandle, uint32_t interval, DWORD_PTR /*dwUser*/)
	{
		assert(interval > 0);
		m_eventHandle = eventHandle;
		// 이 객체는 이제 사용자들의 사용을 권장하지 않는다. 즉 내부 용도로만 쓰인다.
		// 따라서 GetUnsafePtr을 써도 OK.
		m_timerID = CGlobalTimerThread::GetUnsafeRef().TimerMiniTask_Add(interval, EventSetter, this);
	}

	Timer::~Timer()
	{
		CGlobalTimerThread::GetUnsafeRef().TimerMiniTask_Remove(m_timerID);
	}
#endif
}
