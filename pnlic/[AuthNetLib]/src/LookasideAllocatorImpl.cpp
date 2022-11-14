/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "LookasideAllocatorImpl.h"
#include "../include/sysutil.h"
#include "../include/FastHeap.h"
#include "../include/NetConfig.h"
#include "STLUtil.h"
#include "FastHeapImpl.h"
#include "../include/ThreadUtil.h"
#include "CriticalSectImpl.h"

//#define USE_LookasideAllocator_FirstAllocCount

namespace Proud
{
	uint32_t CLookasideAllocatorImpl::m_cpuCount = 0;// GetNoofProcessors();여기서 이걸 호출할 경우 CLookasideAllocator를 static member로 쓰는 곳 때문에 CLookasideAllocatorImpl 생성이 먼저 일어나는 문제가 있다. 따라서 여기서는 함수 호출을 하지 않고 초기값만 넣어주도록 한다.
	
	/* 생성만 하고 free는 하지 않는다. 
	비록 메모리 누수겠지만, 프로세스 종료시 전까지만 쓰이므로 무시하자. 
	여기서 이걸 호출할 경우 CLookasideAllocator를 static member로 쓰는 곳 때문에 
	CLookasideAllocatorImpl 생성이 먼저 일어나는 문제가 있다. 
	따라서 여기서는 함수 호출을 하지 않고 초기값만 넣어주도록 한다. */
	intptr_t CLookasideAllocatorImpl::m_cpuIndexTlsIndex;

	/* ptherad_key_t는 invalid 값이라는 것이 명시된 바가 없다. 
	따라서 이 변수로 key 여부를 검사해야 한다. */
	bool CLookasideAllocatorImpl::m_cpuIndexTlsIndexValid = false; 

	void* CLookasideAllocatorImpl::CPerCpu::Alloc( size_t size )
	{
#ifdef _DEBUG
		if(m_state != AfterCreation)
		{
			// 예외를 발생시키지 말자. 전역 변수 파괴 단계에서는 예외를 catch할 곳도 없으니까.
			ShowUserMisuseError(_PNT("CLookasideAllocator.Alloc() is called after the allocator is already disposed!"));
		}
#endif
		CriticalSectionLock lock(m_cs, m_owner->m_settings.m_accessMode != FastHeapAccessMode_UnsafeSingleThread);

		m_inUse  =true;

		m_owner->CheckCritSecLockageOnUnsafeModeCase();

		if (size == 0)
		{
			m_inUse = false;
			return NULL;
		}

		if (m_fixedBlockSize == 0)
			m_fixedBlockSize = size;

		else if (m_fixedBlockSize != size)
		{
			m_inUse = false;
			ThrowInvalidArgumentException();
		}

		// free list에 있으면 그걸 재활용, 없으면 새 할당
		if(m_freeHead == NULL)
		{
			uint8_t* block = (uint8_t*)m_owner->m_settings.m_pHeap->Alloc(sizeof(BlockHeader) + size);
			if (block == NULL)
				return NULL;

			BlockHeader* header = (BlockHeader*)block;
			CallConstructor(header);
			header->m_payloadLength = m_fixedBlockSize;
			header->m_allocCpuIndex = m_cpuIndex;

			m_initialAllocCount++;

			m_inUse = false;
			return GetNetBlock(header);
		}
		else
		{
			BlockHeader* rnode = m_freeHead;
			// head node를 꺼낸다.  FIFO보다는 FILO가 CPU cache에 객체가 잔존할 확률이 크기 때문에 더 효율적이다.
			m_freeHead = rnode->m_nextFreeNode;
			rnode->m_nextFreeNode = NULL;
			rnode->m_allocCpuIndex = m_cpuIndex;
			m_freeCount--;
			m_inUse = false;

			return GetNetBlock(rnode);
		}

		return NULL;
	}

	void CLookasideAllocatorImpl::CPerCpu::Free( BlockHeader* block )
	{
#ifdef _DEBUG
		// 이미 gFastRefCountHeap이 파괴된 상태이면 에러를 발생시킨다.
		if (m_state != AfterCreation)
		{
			// 예외를 발생시키지 말자. 전역 변수 파괴 단계에서는 예외를 catch할 곳도 없으니까.
			ShowUserMisuseError(_PNT("CLookasideAllocator.Free() is called after the allocator is already disposed! Doing Free before WinMain returns in highly recommended."));
		}
#endif
		CriticalSectionLock lock(m_cs, m_owner->m_settings.m_accessMode != FastHeapAccessMode_UnsafeSingleThread);
		m_inUse = true;
		m_owner->CheckCritSecLockageOnUnsafeModeCase();			

		// 제거하려는 항목을 free list에 반환한다.
		if (block->m_nextFreeNode != NULL)
		{
			ShowUserMisuseError(_PNT("Not a CLookasideAllocator or owner CFastHeap allocated block! Refer to technical notes for more help."));
			m_inUse = false;
		}
		else
		{
			// head로서 push한다.  FIFO보다는 FILO가 CPU cache에 객체가 잔존할 확률이 크기 때문에 더 효율적이다.
			block->m_nextFreeNode = m_freeHead;
			m_freeHead = block;
			m_freeCount++;
		}
		m_inUse = false;
	}

	CLookasideAllocatorImpl::CLookasideAllocatorImpl(const CFastHeapSettings& settings)
	{
		InitStaticVars();

		m_currentTurn = 0;
		m_settings = settings;
		if(CNetConfig::ForceUnsafeHeapToSafeHeap)
			m_settings.m_accessMode = FastHeapAccessMode_MultiThreaded;

		if(m_settings.m_pHeap== NULL)
			m_settings.m_pHeap = CFirstHeap::GetHeap();

		if(m_settings.m_accessMode == FastHeapAccessMode_UnsafeSingleThread)
		{
			m_perCpus = (CPerCpu**)::malloc(sizeof(CPerCpu*));
			if (m_perCpus == NULL)
				throw std::bad_alloc();

			m_perCpus[0] = new CPerCpu(this,0);
		}
		else
		{
			m_perCpus = (CPerCpu**)::malloc(sizeof(CPerCpu*) * m_cpuCount);
			if(m_perCpus == NULL)
				throw std::bad_alloc();

			for(uint16_t i=0;i<m_cpuCount;i++)
			{
				m_perCpus[i] = new CPerCpu(this,i);
			}
		}
	}

	CLookasideAllocatorImpl::CPerCpu::~CPerCpu()
	{
		CriticalSectionLock lock(m_cs,false);
		lock.UnsafeLock();
#ifdef _DEBUG
		m_state = AfterDestruction;
#endif

		// 모든 free list를 제거한다.
		while(m_freeHead != NULL)
		{
			BlockHeader* node = m_freeHead;
			m_freeHead = node->m_nextFreeNode;
			node->m_nextFreeNode = NULL;
			CallDestructor(node); // 유지보수하다보면 혹시모르니
			m_owner->m_settings.m_pHeap->Free(node);
		}
	}

	CLookasideAllocatorImpl::CPerCpu::CPerCpu(CLookasideAllocatorImpl* owner,uint16_t cpuIndex)
	{
		m_owner = owner;
		m_fixedBlockSize = 0;
#ifdef _DEBUG
		m_state = AfterCreation;
#endif

		m_freeHead = NULL;
		m_freeCount = 0;
		m_initialAllocCount = 0;
		m_cpuIndex = cpuIndex;
		m_inUse = false;
	}

	CLookasideAllocatorImpl::BlockHeader* CLookasideAllocatorImpl::CPerCpu::GetBlockHeader(void* ptr)
	{
		if (m_fixedBlockSize <= 0)
		{
			throw Exception("Cannot call GetBlockHeader before allocing anyone!");
		}

		CLookasideAllocatorImpl::BlockHeader* block = GetBlockHeaderNoSizeCheck(ptr);
		if (block == NULL)
			return NULL;
		if (block->m_payloadLength != m_fixedBlockSize)
			return NULL;

		return block;
	}
	void CLookasideAllocatorImpl::CPerCpu::AssureValidBlock(CLookasideAllocatorImpl::BlockHeader* block)
	{
		if (block->m_splitter != BlockSplitter || block->m_nextFreeNode == (CLookasideAllocatorImpl::BlockHeader*)0xfdfdfdfd || block->m_payloadLength != m_fixedBlockSize)
		{
			throw Exception("Invalid Lookaside block is detected!");
		}
	}

	void CLookasideAllocatorImpl::CheckCritSecLockageOnUnsafeModeCase()
	{ 
#if defined(_WIN32)
		/* TODO: 이상한 콜스택.txt 상황떄문에, 이를 릴리즈 빌드에서는 무효하게 해야 하나? */
		if(m_settings.m_accessMode == FastHeapAccessMode_UnsafeSingleThread 
           && m_settings.m_debugSafetyCheckCritSec != NULL
		   && !Proud::IsCriticalSectionLockedByCurrentThread(m_settings.m_debugSafetyCheckCritSec->m_standard->m_cs))
		{
			ShowUserMisuseError(_PNT("Unsafe heap accessor with thread unsafety is detected!"));
		}
#endif
	}

	int CLookasideAllocatorImpl::DebugCheckConsistency()
	{
		return m_settings.m_pHeap->DebugCheckConsistency();
	}

	CLookasideAllocatorImpl::~CLookasideAllocatorImpl()
	{
		if(m_settings.m_accessMode == FastHeapAccessMode_UnsafeSingleThread)
		{
			delete m_perCpus[0];
		}
		else
		{
			for(uint16_t i=0;i<m_cpuCount;i++)
			{
				delete m_perCpus[i];
			}
		}

		::free(m_perCpus);
	}

	void* CLookasideAllocatorImpl::Alloc( size_t size )
	{
		// get anyone and use it
		int turn = NextTurn();
		return m_perCpus[turn]->Alloc(size);
	}

	void CLookasideAllocatorImpl::Free( void* ptr )
	{
		BlockHeader* block = GetBlockHeader(ptr);
		if(!block)
		{
			ShowUserMisuseError(_PNT("Not a CLookasideAllocator or owner CFastHeap allocated block! Refer to technical notes for more help."));
			return;
		}

		uint16_t cpuIndex = block->m_allocCpuIndex;
		CPerCpu* perCpu = m_perCpus[cpuIndex];
		if(cpuIndex< 0 || cpuIndex>=m_cpuCount)
		{
			ShowUserMisuseError(_PNT("Not a CLookasideAllocator or owner CFastHeap allocated block! Refer to technical notes for more help."));
			return;
		}

		if(perCpu->m_fixedBlockSize != block->m_payloadLength)
		{
			ShowUserMisuseError(_PNT("Not a CLookasideAllocator or owner CFastHeap allocated block! Refer to technical notes for more help."));
			return;
		}

		perCpu->Free(block);
	}

	int CLookasideAllocatorImpl::NextTurn()
	{
		if(m_settings.m_accessMode == FastHeapAccessMode_UnsafeSingleThread)
			return 0; 

		intptr_t cpuIndex  = 0;
		if (CNetConfig::AllowUseTls)
		{
			if(!m_cpuIndexTlsIndexValid)
				throw Exception("m_cpuIndexTlsIndexValid is false!");

			cpuIndex = (intptr_t)Proud::TlsGetValue((uint32_t)m_cpuIndexTlsIndex);
		if(cpuIndex == 0)
		{
			int cpuCount = m_cpuCount;
				cpuIndex = AtomicIncrement32(&m_currentTurn) % cpuCount;
				Proud::TlsSetValue((uint32_t)m_cpuIndexTlsIndex, (void*)(cpuIndex + 1)); // 미설정이 0이므로
			}
			else
			{
				cpuIndex--; // 이게 정상 값
			}
		}
		else
		{
			int cpuCount = m_cpuCount;
			cpuIndex = AtomicIncrement32(&m_currentTurn) % cpuCount;	// InterlockedIncrement returns the incremented value.
		}

		return (int)cpuIndex;
	}

	void CLookasideAllocatorImpl::InitStaticVars()
	{
		static CriticalSection initStaticVarCritSec;
		CriticalSectionLock initializerLock(initStaticVarCritSec,true);
		if(m_cpuCount == 0)
			m_cpuCount = GetNoofProcessors();

		if (CNetConfig::AllowUseTls)
		{
			if(!m_cpuIndexTlsIndexValid)
			{
				m_cpuIndexTlsIndex = Proud::TlsAlloc(); // TlsFree()를 하는 곳이 없어서 메모리 누수겠지만, 프로세스 종료시 전까지만 쓰이므로 무시하자. 
				m_cpuIndexTlsIndexValid = true;
			}
		}
	}
}