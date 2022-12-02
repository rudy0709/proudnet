#pragma once 
#include "LowContextSwitchingLoop.h"

namespace Proud
{
	template <typename T>
	void CClassObjectPool<T>::ShrinkOnNeed()
	{		
		assert(m_subPoolCount <= MaxCpuCoreCount);

		ShrinkOnNeed_Functor f(this);
		// SubPool 각각에 대해 lock을 걸면 잦은 대기가 발생한다.
		// 따라서 low context switch loop를 한다.
		// SpinMutex를 쓰기 때문에 모든 sub pool이 contention이면 어쩔 수 없이 긴 대기 시간이 발생하지만,
		// 이정도 부하를 댓가로 critsec보다 2배 빠른 atomic op의 효과를 볼 수 있으니, 감수하자.
		LowContextSwitchingLoop(f.m_subPools, f.m_subPoolCount, f);
	}

}