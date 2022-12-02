// 람다식을 쓸 수 있다면 얼마나 좋을까냐마는, VS2003도 지원해야 하기 때문에
// 이곳 저곳에서 재사용되는 functor class들의 집합.

#pragma once

#include <memory>

#include "LowContextSwitchingLoop.h"

#include "SuperSocket.h"

namespace Proud 
{
	using namespace std;
	
	/* 다음을 low context swith loop안에서 수행한다.
	(try)lock -> issue send -> unlock -> dec use count */
	template<typename MAIN>
	struct IssueSendFunctor
	{
		// lambda expression이 지원되면 람다 캡처로 간단히 해결될 것들이, 
		// 이렇게 멤버 변수 선언과 생성자 함수 만들기 노가다를 떠야 한다... 끙.
		MAIN* m_main;

		inline IssueSendFunctor(MAIN* main)
		{
			m_main = main;
		}

		inline CriticalSection* GetCriticalSection(const shared_ptr<CSuperSocket>& object)
		{
			return &object->m_cs;
		}

		// try-lock or lock을 성공하게 되면 여기가 실행된다. 여기서 issue send or non-block send를 한다.
		// solock: per-socket lock. 이미 lock한 상태이다.
		inline bool DoElementAndUnlock(const shared_ptr<CSuperSocket>& object, CriticalSectionLock& solock)
		{
			assert(solock.IsLocked());
			
			bool finished = true;
#if defined (_WIN32)
			// try-lock이 성공되면 여기에 온다. 따라서 두번째 인자가 true이어야.
			object->IssueSendOnNeedAndUnlock(
				object,
				false, solock);
#else
			CIoEventStatus comp;
			finished = object->NonBlockSendAndUnlock(
				object,
				false, solock, comp);
#endif
			assert(!solock.IsLocked());

			return finished;
		}
	};

}
