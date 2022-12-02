#pragma once 

#include "../include/atomic.h"

namespace Proud
{
	class ScopeExit_AtomicCompareAndSwap32
	{
		volatile int32_t *m_target;
		int m_v1;
		int m_v2;
	public:
		ScopeExit_AtomicCompareAndSwap32(int v1, int v2, volatile int32_t* target)
			: m_target(target), m_v1(v1), m_v2(v2) {}
		
		~ScopeExit_AtomicCompareAndSwap32()
		{
			AtomicCompareAndSwap32(m_v1, m_v2, m_target);
		}
	};

}