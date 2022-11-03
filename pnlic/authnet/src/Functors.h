// 람다식을 쓸 수 있다면 얼마나 좋을까냐마는, VS2003도 지원해야 하기 때문에
// 이곳 저곳에서 재사용되는 functor class들의 집합.

#pragma once 

#include "LowContextSwitchingLoop.h"

namespace Proud 
{
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

		inline CriticalSection* GetCriticalSection(CSuperSocket* object)
		{
			return &object->m_cs;
		}

		// try-lock or lock을 성공하게 되면 여기가 실행된다. 여기서 issue send or non-block send를 한다.
		// solock: per-socket lock. 이미 lock한 상태이다.
		inline bool DoElementAndUnlock(CSuperSocket* object, CriticalSectionLock& solock)
		{
			assert(solock.IsLocked());
			
			bool finished = true;
#if defined (_WIN32)
			// try-lock이 성공되면 여기에 온다. 따라서 두번째 인자가 true이어야.
			object->IssueSendOnNeedAndUnlock(false, solock);
#else
			CIoEventStatus comp;
			finished = object->NonBlockSendAndUnlock(false, solock, comp);
#endif
			assert(!solock.IsLocked());

			if (finished)
			{
				// 완료 성공했으니까 참조 해제를 해주어야.
				object->DecreaseUseCount();
			}
			return finished;
		}
	};

	// DoForLongInterval에 대한 low-context switch loop용.
	// NOTE: REMOTE는 pointer type이어야 한다.
	template<typename REMOTE>
	struct DoForLongIntervalFunctor
	{
		// lambda expression이 지원되면 람다 캡처로 간단히 해결될 것들이, 
		// 이렇게 멤버 변수 선언과 생성자 함수 만들기 노가다를 떠야 한다... 끙.
		int64_t m_currTime;

		inline DoForLongIntervalFunctor(/*MAIN* main, */int64_t currTime)
		{
			//m_main = main;
			m_currTime = currTime;
		}

		inline CriticalSection* GetCriticalSection(REMOTE& object)
		{
			return &object->GetCriticalSection();
		}

		inline bool DoElementAndUnlock(REMOTE& object, CriticalSectionLock& solock)
		{
			uint32_t queueLength;

			object->DoForLongInterval(m_currTime, queueLength);
			solock.Unlock();
			// 반드시 decrease use count는 unlock 이후에 해야 함.
			object->DecreaseUseCount();
			
			return true;
		}
	};

	// for AddToSendQueueWithSplitterAndSignal_Copy.
	template<typename REMOTE>
	struct AddToSendQueueWithSplitterAndSignal_Copy_Functor
	{
		// lambda expression이 지원되면 람다 캡처로 간단히 해결될 것들이, 
		// 이렇게 멤버 변수 선언과 생성자 함수 만들기 노가다를 떠야 한다... 끙.
		CSendFragRefs *m_sd;
		int64_t m_currTime;

		AddToSendQueueWithSplitterAndSignal_Copy_Functor(CSendFragRefs *sd)
		{
			m_sd = sd;
		}

		inline AddToSendQueueWithSplitterAndSignal_Copy_Functor(/*MAIN* main, */int64_t currTime)
		{
			//m_main = main;
			m_currTime = currTime;
		}

		// LowContextSwitchingLoop가 non-block or block lock을 하기 위해서.
		inline CriticalSection* GetCriticalSection(CSuperSocket* object)
		{
			return &(object->GetSendQueueCriticalSection());
		}

		inline bool DoElementAndUnlock(CSuperSocket* object, CriticalSectionLock& solock)
		{
			object->AddToSendQueueWithSplitterAndSignal_Copy(*m_sd,SendOpt());
			solock.Unlock();
			object->DecreaseUseCount();
			return true;
		}
	};


}
