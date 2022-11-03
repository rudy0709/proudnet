#pragma once 

#include "../include/atomic.h"
#include "../include/SysTime.h"

namespace Proud
{
	// 매우 간단한, spin lock 객체.
	// 주의: 이것을 쓸 때는 다음 조건을 만족시켜야 한다.
	// CPU time만 해야 하고, 매우 contention이 짧아야 하고(1/1000 이하), O(1) 수준의 짧은 연산량이어야 하고, no reentrant 즉 재귀적 잠금을 안 해야 한다.
	// Proud.CriticalSection에 준한다.
	class SpinMutex
	{
		// 0: unlocked, 1:locked
		volatile int m_lockState;
		int64_t m_acquireSuccessCount;
		int64_t m_acquireFailCount;
	public:
		SpinMutex()
		{
			m_acquireSuccessCount = 0;
			m_acquireFailCount = 0;
			m_lockState = 0;
		}

		void Lock()
		{
			// acquire까지 반복한다.
			int count = 0;
			while (1)
			{
				int r = AtomicCompareAndSwap32(0, 1, &m_lockState);
				if (r == 0)
				{
					m_acquireSuccessCount++;
					// acquired!
					return;
				}
				count++;
				m_acquireFailCount++;
				if (count > 1000)
				{
					Proud::Sleep(0); // context switch를 유도한다. 더 이상 오래 돌아봤자 CPU만 태운다.
				}
			}
		}

		// 주의: 사전에 이미 lock 여부를 검사 안한다.
		void Unlock()
		{
			AtomicCompareAndSwap32(1, 0, &m_lockState);
		}
	};

	// CriticalSectionLock에 준한다.
	class SpinLock
	{
		SpinMutex* m_mutex;
		int m_lockCount;
	public:
		SpinLock(SpinMutex& mutex, bool initialLock)
		{
			m_lockCount = 0;
			m_mutex = &mutex;
			if (initialLock)
			{
				Lock();
			}
		}

		void Lock()
		{
			if (m_lockCount == 0)
			{
				m_mutex->Lock();
			}
			m_lockCount++;
		}

		void Unlock()
		{
			m_lockCount--;
			if (m_lockCount == 0)
			{
				m_mutex->Unlock();
			}
		}

		~SpinLock()
		{
			Unlock();
		}
	};
	
}