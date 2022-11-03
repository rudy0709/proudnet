#pragma once

#if !defined(_WIN32)
	#include <pthread.h>
#else
#endif
#include "../include/Ptr.h"
#include "../include/CriticalSect.h"
#include "../include/Singleton.h"
#include "../include/FastArray.h"
#include "../include/FastArrayPtr.h"
#include "../include/FastHeapSettings.h"
#include "../include/LookasideAllocator.h"
#include "../include/CriticalSect.h"
#include "../include/atomic.h"

namespace Proud
{
	// ** 이 클래스는 컴파일러의 최적화 옵션에 따라 성능이 두배까지 차이가 난다. **
	class CLookasideAllocatorImpl: public CLookasideAllocator
	{
	public:
		// 만약 이미 파괴된 상태라면 0xfe 등 non allocated heap 공간에서의 값이 리턴될 것이다.
		enum State
		{
			BeforeCreation = 0x01,
			AfterCreation,
			AfterDestruction,
		};

		CFastHeapSettings m_settings;

		PROUDNET_VOLATILE_ALIGN int32_t m_currentTurn;

		class BlockHeader;

		enum { BlockSplitter = 4321 };

		typedef uint32_t ChunkSplitterType; 
		typedef uint16_t SplitterType; 
		
		static uint32_t m_cpuCount;
		static intptr_t m_cpuIndexTlsIndex;
		static bool m_cpuIndexTlsIndexValid;
	private:
		class CPerCpu
		{
		public:
			CLookasideAllocatorImpl* m_owner;
			//modify by rekfkno1 - 조금이라도 성능을 더 올려보기 위함.
#ifdef _DEBUG
			PROUDNET_VOLATILE_ALIGN int m_state;
#endif

			// free list의 first node.
			BlockHeader* m_freeHead;
			size_t m_freeCount;
			size_t m_initialAllocCount;
			uint16_t m_cpuIndex;

			// 사용자가 처음 블럭을 할당할 때 입력했던 값
			size_t m_fixedBlockSize;

			CriticalSection m_cs;
			
			volatile bool m_inUse;

			// 주어진 블록에 대한 block header 값을 얻는다.
			inline BlockHeader* GetBlockHeader(void* ptr);

			inline BlockHeader* GetBlockHeaderNoSizeCheck(void* ptr)
			{
				if (ptr == NULL)
					return NULL;

				char* ptr2 = (char*)ptr;
				ptr2 -= sizeof(BlockHeader);
				BlockHeader* header = (BlockHeader*)ptr2;
				if (header->m_splitter != BlockSplitter)
					return NULL;

				return header;
			}

			inline size_t GetBlockAndHeaderLength()
			{
				return ( sizeof(BlockHeader) + m_fixedBlockSize);
			}

			void AssureValidBlock(BlockHeader* block);

			CPerCpu(CLookasideAllocatorImpl* owner,uint16_t cpuIndex);
			~CPerCpu();

			void* Alloc( size_t size );
			void Free(BlockHeader* block);
		};
		CPerCpu** m_perCpus;
	public:
		// 너무 작은 크기의 메모리를 할당하려는 경우 이는 배보다 배꼽이 더 큰 셈이지만, CPU 성능을 아낀다는게 어디냐...
		// CListNode 즐. 12바이트를 기본으로 쓰니까.
		class BlockHeader
		{
		public:
			// 메모리 깨짐을 감지하기 위함
			SplitterType m_splitter;

			// Free list의 항목들은 어떤 lookaside allocator로 귀속되어도 상관없다. 하지만 서로 다른 크기의 블록이 free list에 들어갈 수는 없다. 이 멤버의 존재 이유.
			size_t m_payloadLength;    
			
			// 이 블록이 사용중이면 NULL이다.
			BlockHeader* m_nextFreeNode; 
			uint16_t m_allocCpuIndex;

			inline BlockHeader()
			{
				m_allocCpuIndex = 0;
				m_nextFreeNode = NULL;
				m_splitter = BlockSplitter;
			}
		};

		// 유저에게 노출될 순수 블럭 영역을 리턴한다.
		static inline void* GetNetBlock( BlockHeader* header )
		{
			return (void*)(((char*)header) + sizeof(BlockHeader));
		}

		// 주어진 블록에 대한 block header 값을 얻는다.
		static inline BlockHeader* GetBlockHeader(void* ptr)
		{
			if (ptr == NULL)
				return NULL;

			char* ptr2 = (char*)ptr;
			ptr2 -= sizeof(BlockHeader);
			BlockHeader* header = (BlockHeader*)ptr2;
			if (header->m_splitter != BlockSplitter)
				return NULL;

			return header;
		}

		void CheckCritSecLockageOnUnsafeModeCase();
		int NextTurn();

		void InitStaticVars();
	public:
		CLookasideAllocatorImpl(const CFastHeapSettings& settings);
		~CLookasideAllocatorImpl();

		virtual void* Alloc( size_t size );
		virtual void Free(void* ptr);

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif
		virtual int DebugCheckConsistency();

	};


}