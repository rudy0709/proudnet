#pragma once

#include "../include/Ptr.h"
#include "../include/Singleton.h"

namespace Proud
{
	class CLookasideAllocator;
	// 예전에는 위 주석처리한 소스를 썼으나 간헐 크래쉬가 발견되어 아래와 같이 대체함.
	class CRefCountHeap:public CSingleton<CRefCountHeap>
	{
	public:
		CLookasideAllocator* m_heap;

		CRefCountHeap();
		~CRefCountHeap();
	};
}
