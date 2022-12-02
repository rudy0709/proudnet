#pragma once 

#include "../include/atomic.h"
#include "../include/SysTime.h"
#include "sysutil-private.h"

namespace Proud
{
	// 매우 간단한, spin lock 객체.
	// 주의: 이것을 쓸 때는 다음 조건을 만족시켜야 한다.
	// CPU time만 해야 하고, 매우 contention이 짧아야 하고(1/1000 이하), O(1) 수준의 짧은 연산량이어야 하고, no reentrant 즉 재귀적 잠금을 안 해야 한다.
	// Proud.CriticalSection에 준한다.
	// code profile 결과, critsec보다 5배 가량 빠르다!
	// 주의: non-recursive이다. 데드락 주의할 것!
	class SpinMutex
	{
		// 0: unlocked, 1:locked
		volatile int m_lockState;
		// 통계용. 값이 좀 틀려도 된다. 대신, 10배나 느린 volatile access가 아니다.
		int64_t m_acquireSuccessCount;
		int64_t m_acquireFailCount;
	public:
		SpinMutex()
		{
			m_acquireSuccessCount = 0;
			m_acquireFailCount = 0;
			m_lockState = 0;
		}

		inline bool IsLocked() const
		{
			return m_lockState == 1;
		}

		inline bool TryLock()
		{
			int r = AtomicCompareAndSwap32(0, 1, &m_lockState);
			if (r == 0)
			{
				m_acquireSuccessCount++;
				// acquired!
				return true;
			}
			m_acquireFailCount++;
			return false;
		}
		
		inline void Lock()
		{
			// acquire까지 반복한다.
			int count = 0;
			while (true)
			{
				bool r = TryLock();
				if(r)
				{
					// acquired!
					return;
				}

				count++;
				if (count > 1000)
				{
					// context switch를 유도한다. 더 이상 오래 돌아봤자 CPU만 태운다.
					// 반드시 있어야 한다. 없으면 OS time slice가 끝날 때까지 계속 CPU를 장시간 소모해 버리고
					// 이로 인해 총체적 시간 낭비가 일어난다.
					Proud::YieldThread();
					count = 0;
				}
				else 
				{
					// 자세한 내용은 megayuchi.com의 SRWLock 관련 글 참고.
					__UTIL_LOCK_SPIN_LOCK_PAUSE();
				}
			}
		}


		// 주의: 사전에 이미 lock 여부를 검사 안한다.
		inline void Unlock()
		{
			AtomicCompareAndSwap32(1, 0, &m_lockState);
		}
	};

	// CriticalSectionLock에 준한다.
	// 주의: non-recursive이다. 데드락 주의할 것!
	class SpinLock
	{
		SpinMutex* m_mutex;
		int m_lockCount;
	public:
		inline SpinLock(SpinMutex& mutex, bool initialLock)
		{
			m_lockCount = 0;
			m_mutex = &mutex;
			if (initialLock)
			{
				Lock();
			}
		}

		inline void Lock()
		{
			if (m_lockCount == 0)
			{
				m_mutex->Lock();
			}
			m_lockCount++;
		}

		inline bool TryLock()
		{
			if (m_lockCount == 0)
			{
				bool r = m_mutex->TryLock();
				if (r)
				{
					m_lockCount++;
				}
				return r;
			}
			else
				return false;
		}

		inline void Unlock()
		{
			m_lockCount--;
			if (m_lockCount == 0)
			{
				m_mutex->Unlock();
			}
		}

		// mutex가 lock 걸렸는지 검사하는게 아니라, 이 SpinLock 객체에 대한 lock이 시행중인지만 검사한다.
		inline bool IsLocked()
		{
			return m_lockCount > 0;
		}

		inline ~SpinLock()
		{
			Unlock();
		}
	};
	
}