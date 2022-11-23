#pragma once 

#include "../include/BasicTypes.h"
#include "../include/pnstdint.h"
#include "../include/sysutil.h"
#include "../include/ProcHeap.h"
#include "mmtime64.h"

//#pragma pack(push,8)

/* offsetof는 gcc에서 이러한 에러를 내므로, 이렇게 꼼수를 써서 해결해야.
warning: invalid access to non-static data member `main()::C::a' of NULL object
16인 이유는: 64bit computer에서 alignment가 붙어도 영향을 안 받게 하기 위해.
*/
#define pn_offsetof(s,m)  ( ((size_t)&((s*)16)->m) - 16 )


namespace Proud 
{
#if (_MSC_VER>=1400)
#pragma managed(push,off)
#endif

	PROUD_API _Noreturn void ThrowInvalidArgumentException();

	/*
	ProudNet 에서 쓰이는 풀 객체는 CLookasideAllocator, CFastHeap, CObjectPool 이 있다.
	CLookasideAllocator, CFastHeap 의 경우 고객사에서 사용할 수 있도록 노출이 되어 있고 CObjectPool 의 경우 노출이 되어 있지 않다.
	CObjectPool 은 메모리 풀이 아닌 객체 풀이다. 생성자와 소멸자의 처리량이 많은 경우에도 최적화를 해준다.

	이 클래스 및 다른 풀 클래스들 (CPooledObjects) 이 노출되지 않는 이유는 사용법이 예민하지 않아야 한다는 엔진 API 정책에 위배되기 때문이다.
	외부에 노출되지 않은 풀 클래스들은 shrink 기능이 있다. 사용상 주의사항이 따르지만 그 댓가로 더 적은 메모리를 차지하는 장점이 있다.
	*/
	// thread unsafe하다.
	template<typename T> 
	class CObjectPool
	{
		enum { TagValue = 7654 };

		// object pool 상태가 되어 있는 객체
		class CDroppee
		{
		public:
			uint16_t m_tag;
			T m_obj;
			CDroppee* m_next;

			inline CDroppee() 
			{
				m_tag = TagValue; 
				m_next = NULL;
			}

			// object-pool에 의해 다뤄지는 것이고, UE4 FMallocBinned등에서 데드락을 일으키는 경우가 있어, 
			// 외부 요인을 제거하기 위해 이 매크로를 추가했음.
			DECLARE_NEW_AND_DELETE_THROWABLE
		};

		// free list
		CDroppee* m_reuableHead;
		// free list에 있는 항목들
		intptr_t m_freeListCount;

		// free list 항목들의 갯수는 줄었다 늘었다 한다.
		//이때 최저점과 최고점이 어디인지를 여기에 남긴다.
		// 이를 근거로 "장시간 안 쓰이는 free list item"들을 ShrinkOnNeed에서 폐기한다.
		// 그래야 심각한 external memory fragmentation을 일으키지 않으니까.
		intptr_t m_minFreeListCount, m_maxFreeListCount;

		// 가장 마지막에 shrink를 한 시간
		uint32_t m_lastShrinkDoneTime;

		inline CDroppee* GetValidPtr(T* ptr)
		{
			uint8_t* p = (uint8_t*)ptr;
			p -= pn_offsetof(CDroppee,m_obj);
			CDroppee* node = (CDroppee*)p;
			if(node->m_tag != TagValue)
				return NULL;

			return node;
		}
	public:

		CObjectPool()
		{
			m_reuableHead = NULL;
			m_freeListCount = 0;
			m_minFreeListCount = 0;
			m_maxFreeListCount = 0;
			m_lastShrinkDoneTime = 0;
		}

		inline ~CObjectPool()
		{
			while(m_reuableHead != NULL)
			{
				CDroppee* node = m_reuableHead;
				m_reuableHead = node->m_next;
				node->m_next = NULL;
				delete node;
			}
		}

		// 할당. pool에서 꺼낼 경우 ctor가 새로 호출되지 않음. 즉 중고품이라는 뜻.
		inline T* NewOrRecycle()
		{
			if(m_reuableHead == NULL)
			{
				CDroppee* newOne = new CDroppee;
				return &newOne->m_obj;
			}
			else
			{
				// head를 리턴한다. 즉 push 처럼 사용한다. FIFO보다는 FILO가 CPU cache에 객체가 잔존할 확률이 크기 때문에 더 효율적이다.
				CDroppee* rnode = m_reuableHead;
				// pop node
				m_reuableHead = rnode->m_next;
				rnode->m_next = NULL;

				assert(m_freeListCount > 0);
				m_freeListCount--;
				if(m_minFreeListCount > m_freeListCount)
					m_minFreeListCount = m_freeListCount;

				return &rnode->m_obj;
			}
		}

		// 해제. 단, 객체 내 파괴자는 콜 안함.
		inline void Drop(T* instance)
		{
			CDroppee* node = GetValidPtr(instance);
			if(!node || node->m_next != NULL)
				ThrowInvalidArgumentException();
			
			// node를 추가하되 head로서 넣는다.
			node->m_next = m_reuableHead;
			m_reuableHead = node;			
			
			m_freeListCount++;
			if(m_maxFreeListCount < m_freeListCount)
				m_maxFreeListCount = m_freeListCount;
		}

		// free list에서 너무 오래 묵은 것들을 제거한다.
		// object pool.pptx 참고.
		// 너무 자주 콜 하지 말아라. 내부적으로 시간 얻는 함수가 작동하는데 시간 얻는데 오래 걸리니까.
		// 대략 1초에 한번 정도면 적당.
		void ShrinkOnNeed()
		{
			if (m_freeListCount == 0)
				return;

			uint32_t currTime = CFakeWin32::GetTickCount();

			// 아직 따끈따끈하면 skip
			if(currTime - m_lastShrinkDoneTime <= 10*1000)
			{
				return;
			}

			m_lastShrinkDoneTime = currTime;

			// free list의 최소였던 갯수와 최대였던 갯수 구간폭 만큼을 남긴 나머지들을 모두 없애자.
			intptr_t remainCount = m_maxFreeListCount - m_minFreeListCount;
			if(remainCount >= 0)
			{
				intptr_t deleteAmount = m_freeListCount - remainCount;

				if (deleteAmount > 0)
				{
					for (int i = 0; i < deleteAmount; i++)
					{
						CDroppee* node = m_reuableHead;
						m_reuableHead = m_reuableHead->m_next;

						delete node;

						m_freeListCount--;
					}
					//NTTNTRACE("CObjectPool freed unused %d items.\n", deleteAmount);
				}
			}

			// 지운 갯수와 상관 없이 상태 리셋
			m_minFreeListCount = m_maxFreeListCount = m_freeListCount;
		}

	};

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)