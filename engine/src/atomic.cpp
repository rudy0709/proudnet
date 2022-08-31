/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/atomic.h"
#include "../include/CriticalSect.h"

namespace Proud
{
	CriticalSection g_atomicOpCritSec;

	int32_t CSlowAtomic::Increment32(volatile int32_t *target)
	{
		g_atomicOpCritSec.m_neverCallDtor = true;
		CriticalSectionLock lock(g_atomicOpCritSec, true);

		int32_t a = *target;
		a++;
		*target = a;
		return a;
	}

	int32_t CSlowAtomic::Decrement32(volatile int32_t *target)
	{
		g_atomicOpCritSec.m_neverCallDtor = true;
		CriticalSectionLock lock(g_atomicOpCritSec, true);

		int32_t a = *target;
		a--;
		*target = a;
		return a;
	}

	int32_t CSlowAtomic::Exchange32(volatile int32_t *target, int32_t newValue)
	{
		g_atomicOpCritSec.m_neverCallDtor = true;
		CriticalSectionLock lock(g_atomicOpCritSec, true);

		int32_t a = *target;	// 리턴값 저장용, 나중에 이값을 리턴한다. 이전값을 리턴하기 때문에
		*target = newValue;

		return a;
	}

	int32_t CSlowAtomic::CompareAndSwap(int32_t oldValue, int32_t newValue, volatile int32_t *target)
	{
		g_atomicOpCritSec.m_neverCallDtor = true;
		CriticalSectionLock lock(g_atomicOpCritSec, true);

		int32_t a = *target;
		if(a == oldValue)
		{
			*target = newValue;
		}

		return a;
	}

	int32_t CSlowAtomic::Swap(int32_t newValue, volatile int32_t *target)
	{
		g_atomicOpCritSec.m_neverCallDtor = true;
		CriticalSectionLock lock(g_atomicOpCritSec, true);

		int32_t a = *target;
		*target = newValue;

		return a;
	}
}
