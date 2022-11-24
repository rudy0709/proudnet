/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/Event.h"
#include "../include/Exception.h"

namespace Proud
{

	bool Event::WaitOne( uint32_t timeOut )
	{
#if defined(_WIN32)
		return ::WaitForSingleObject(m_event, timeOut) == WAIT_OBJECT_0;
#else
		int ret = 0;

		if (!m_isSignaled)
		{
			// cond variable 을 사용하기전에 미리 mutex lock
			ret = pthread_mutex_lock(&m_mutex);
			if(ret != 0)
			{
				throw Exception("WaitOne mutex lock failed");
			}
			
			ret = pthread_cond_wait(&m_condVar, &m_mutex);

			// lock 을 했다면 당연히 unlock
			ret = pthread_mutex_unlock(&m_mutex);
			if(ret != 0)
			{
				throw Exception("WaitOne mutex unlock failed");
			}

			if (!m_manualReset)
			{
				Reset();	// Auto-Reset 이니 깨어나서 reset 해주기
			}
		}
		else  // signal 상태...
		{
			// 작업없이 바로 리턴
		}

		return true;
#endif
	}

	bool Event::WaitOne()
	{
		return WaitOne(PN_INFINITE);
	}

#if defined(_WIN32)
	int Event::WaitAny(Event** events, int count, uint32_t timeOut)
	{
		// TRUE,FALSE 키워드가 UE4 & PS4에서 빌드 에러를 일으킨다. 따라서 cpp로 옮김.
		return WaitForSignal(events, count, timeOut, FALSE);
	}

	int Event::WaitAll(Event** events, int count, uint32_t timeOut)
	{
		// TRUE,FALSE 키워드가 UE4 & PS4에서 빌드 에러를 일으킨다. 따라서 cpp로 옮김.
		return WaitForSignal(events, count, timeOut, TRUE);
	}

	int Event::WaitForSignal( Event** events, int count, uint32_t timeOut , bool waitForAll )
	{
		HANDLE hlist[MAXIMUM_WAIT_OBJECTS];
		if (count > MAXIMUM_WAIT_OBJECTS)
		{

			throw Exception("Maximum Available Numbers Exceeded from WaitForSignal");
		}
		for (int i = 0;i < count;i++)
		{
			hlist[i] = events[i]->m_event;
		}

		uint32_t r =::WaitForMultipleObjects(count, hlist, waitForAll, timeOut);
		if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + count)
			return r -WAIT_OBJECT_0;

		return -1;
	}
#endif

	Event::~Event()
	{
#if defined(_WIN32)
		CloseHandle(m_event);

#elif defined(__linux__)
		int ret = 0;

		ret = pthread_cond_destroy(&m_condVar);
		if (ret != 0)
		{
			throw Exception("failed to destroy cond variable");
		}

		ret = pthread_mutex_destroy(&m_mutex);
		if (ret != 0)
		{
			throw Exception("failed to destroy mutex");
		}
#endif
	}

	// 이 함수는 Event객체의 생성자에서만 실행된다.
	void init(Event* target, bool manualReset, bool initialState)
	{
		// 명시된 플랫폼 외에는 사용되지 않는다.

#if defined(_WIN32)
		target->m_event = CreateEvent(0, manualReset, initialState, NULL);
		if ( target->m_event == NULL )
		{
			uint32_t e = GetLastError();
			throw Exception("failed to init event. %x", e);
		}

#elif defined(__linux__)
		target->m_manualReset = manualReset;
		target->m_isSignaled = false;

		int ret = pthread_mutex_init(&target->m_mutex, NULL);
		if ( ret != 0 )
		{
			throw Exception("failed to init mutex");
		}

		ret = pthread_cond_init(&target->m_condVar, NULL);
		if ( ret != 0 )
		{
			throw Exception("failed to init cond variable");
		}

		if ( initialState )
		{
			target->Set();
		}
#endif
	}

	Event::Event()
	{
		init(this, false, false);
	}

	Event::Event( bool manualReset, bool initialState )
	{
		init(this, manualReset, initialState);
	}
}