#pragma once

#include "../include/Singleton.h"
#include "../include/ByteArrayPtr.h"
#include "FreeList.h"

namespace Proud {

/*	class CByteArrayHolderAlloc:public CSingleton<CByteArrayHolderAlloc>
	{
	public:
		CLookasideAllocator* m_allocator;

		CByteArrayHolderAlloc()
		{
			m_allocator=CLookasideAllocator::New();
		}

		~CByteArrayHolderAlloc()
		{
			delete m_allocator;
		}
	};*/

//	// 넷클섭과 랜클섭이 전역으로 사용되거나 CNetClientManager에서 ByteArrayPtr의 싱글톤을 사용한다.
//	// 그런데 싱글톤이 먼저 파괴되면 크래시. 따라서 이를 막기 위해 그러한 모듈들은 파괴 순서를 보장해야 한다.
//	// 이것은 그 역할을 한다. ShrinkOnNeed를 콜 하는 모듈들은 이것의 RefCount를 멤버로 갖고 있어야 한다.
//	class CByteArrayPtrManager:public CSingleton<CByteArrayPtrManager>
//	{
//	public:
//		// 할당,해제 가속화를 위해, 그리고 fast heap과 unsafe heap으로의 독립을 위해.
//		// 주의!! CriticalSection 과 g_byteArrayPtrTombstonePool 선언순서를 바꾸면 안된다.
//		// g_byteArrayPtrTombstonePool 이 삭제되기 전까지 CriticalSection이 지워지면 안되기 때문.
//		CriticalSection g_byteArrayPtrTombstonePoolCritSec;
//		CObjectPool<ByteArrayPtr::Tombstone> g_byteArrayPtrTombstonePool;
//	};
}
