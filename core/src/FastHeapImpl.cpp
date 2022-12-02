/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "FastHeapImpl.h"
#include "../include/Exception.h"
#include "../include/BasicTypes.h"
#include "../include/sysutil.h"
#include "ReportError.h"

namespace Proud
{
//#define THROW_MALLOC_FAIL throw Exception("CRT malloc returned NULL at %s!",__FUNCTION__)

	CFastHeapImpl::CFastHeapImpl( size_t AccelBlockSizeLimit, const CFastHeapSettings& settings)
	{
		m_firstHeap = CFirstHeapImpl::GetSharedPtr();
	
		if (AccelBlockSizeLimit < BucketCount)
        {
			throw Exception("bad AccelBlockSizeLimit value");
        }

		if(m_settings.m_pHeap == NULL)
		{
			m_settings.m_pHeap = CFirstHeap::GetHeap();
		}

		mAccelBlockSizeLimit = AccelBlockSizeLimit;

		// 적절한 bucket 갯수를 지정한다.
		// bucket들을 준비한다.
		m_buckets = (CLookasideAllocator**)::malloc(sizeof(CLookasideAllocator*) * BucketCount);
		if (m_buckets == NULL)
			throw std::bad_alloc();

		for (int i = 0;i < BucketCount;i++)
		{
			m_buckets[i] = CLookasideAllocator::New(settings);
		}

		// AccelBlockSizeLimit를 재조정한다.
		AccelBlockSizeLimit /= BucketCount;
		AccelBlockSizeLimit *= BucketCount;

		m_state = AfterCreation;

	}

	CFastHeapImpl::~CFastHeapImpl(void)
	{
		for(int i=0;i<BucketCount;i++)
		{
			delete m_buckets[i];
		}
		::free(m_buckets);

		m_state = AfterDestruction;
	}

	void* CFastHeapImpl::Alloc( size_t size )
	{
#ifndef DISABLE_FAST_HEAP
		if (size == 0)
			ThrowInvalidArgumentException();

		if (m_state != AfterCreation)
		{
			// 예외를 발생시키지 말자. 전역 변수 파괴 단계에서는 예외를 catch할 곳도 없으니까.
			ShowUserMisuseError(_PNT("CFastHeapImpl.Alloc() is called after the allocator is already disposed!"));
			return NULL;
		}
		else
		{
			size_t groundSize = GetGroundSize(size);
			CLookasideAllocator* alloc = GetAllocByGroundSize(groundSize);

			// 풀링 가능한 크기를 넘으면 표준 힙에서 할당한다. 그 외에는 풀에서 할당한다.
			BlockHeader* block;
			if (alloc != NULL)
			{
				block = (BlockHeader*)alloc->Alloc(sizeof(BlockHeader) + groundSize);
				if (block == NULL)
					return NULL;

				block->mBlockLength = groundSize;
			}
			else
			{
				block = (BlockHeader*)m_settings.m_pHeap->Alloc(sizeof(BlockHeader) + groundSize);
				if (block == NULL)
					return NULL;

				block->mBlockLength = groundSize;
			}
			block->mSplitter = (SplitterType)SplitterValue;

			CheckValidation(alloc, block);
			return GetBlockContent(block);
		}
#else // ENABLE_FAST_HEAP
		return CProcHeap::Alloc(size);
#endif
	}

	void* CFastHeapImpl::Realloc( void* ptr, size_t size )
	{
#ifndef DISABLE_FAST_HEAP
		if (m_state != AfterCreation)
		{
			// 예외를 발생시키지 말자. 전역 변수 파괴 단계에서는 예외를 catch할 곳도 없으니까.
			ShowUserMisuseError(_PNT("CFastHeapImpl::Realloc() is called after the allocator is already disposed!"));
			return NULL;
		}
		else
		{
			if (size == 0)
			{
				// std realloc도 size = 0이면 free를 한다. 그 규약을 맞추도록 한다.
				Free(ptr);
				return NULL;
			}
			else
			{
				// realloc을 하되, 포인터 위치가 바뀌어야 하면 내용물 복사도 한다.
				BlockHeader* oldBlock = GetValidatedBlockHeader(ptr);
				if (oldBlock == NULL)
					ThrowInvalidArgumentException();

				// 풀->풀, 풀->힙,힙->풀,힙->힙 케이스를 모두 따지자.
				// 선정된 lookaside allocator이 같냐 여부로 체크하면 된다.
				size_t groundSize = GetGroundSize(size);
				size_t oldGroundSize = oldBlock->mBlockLength;

				CLookasideAllocator* oldAlloc = GetAllocByGroundSize(oldGroundSize);
				CLookasideAllocator* alloc = GetAllocByGroundSize(groundSize);
				BlockHeader* newBlock;

				// 힙->힙
				if (oldAlloc == NULL && alloc == NULL)
				{
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 재할당. 구 내용물 복사도 같이 일어남.
					newBlock = (BlockHeader*)m_settings.m_pHeap->Realloc(oldBlock, sizeof(BlockHeader) + groundSize);
					if (newBlock == NULL)
						return NULL;

					// 헤더 세팅
					assert(newBlock->mSplitter == (SplitterType)SplitterValue);
					newBlock->mBlockLength = groundSize;
				}
				else if (oldAlloc == NULL && alloc != NULL) // 힙->풀
				{
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 할당
					newBlock = (BlockHeader*)alloc->Alloc(sizeof(BlockHeader) + groundSize);
					if (newBlock == NULL)
						return NULL;

					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 구 내용물을 복사
					// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
					//UnsafeFastMemcpy(newBlock, oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, groundSize));
					memcpy_s(newBlock, sizeof(BlockHeader) + groundSize, 
						oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, groundSize));

					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 헤더 세팅
					assert(newBlock->mSplitter == (SplitterType)SplitterValue);
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					newBlock->mBlockLength = groundSize;
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 구 블럭을 제거
					m_settings.m_pHeap->Free(oldBlock);
				}
				else if (oldAlloc != NULL && alloc == NULL) // 풀->힙
				{
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 할당
					// ikpil.choi 2016-11-10 : 리팩토링, 모든 할당은 블럭사이즈(groundSize)로 통합
					//newBlock = (BlockHeader*)m_settings.m_pHeap->Alloc(sizeof(BlockHeader) + size);
					newBlock = (BlockHeader*)m_settings.m_pHeap->Alloc(sizeof(BlockHeader) + groundSize);
					if (newBlock == NULL)
						return NULL;

					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 구 내용물을 복사
					// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
					//UnsafeFastMemcpy(newBlock, oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, size));
					memcpy_s(newBlock, sizeof(BlockHeader) + groundSize, 
						oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, groundSize));

					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 헤더 세팅
					assert(newBlock->mSplitter == (SplitterType)SplitterValue);
					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					newBlock->mBlockLength = groundSize; // ikpil.choi 2016-11-07 : groundSize 가 논리적으로 맞는가?

					assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

					// 구 블럭을 제거
					oldAlloc->Free(oldBlock);
				}
				else // 풀->풀
				{
					if (oldAlloc == alloc) // 풀이 서로 같은 경우. 즉 ground size가 같은 경우다.
					{
						// 특별히 할 것은 없다. 그냥 끝낸다.
						newBlock = oldBlock;
					}
					else
					{
						// 할당
						newBlock = (BlockHeader*)alloc->Alloc(sizeof(BlockHeader) + groundSize);
						if (newBlock == NULL)
							return NULL;

						assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

						// 구 내용물을 복사
						// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
						//UnsafeFastMemcpy(newBlock, oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, groundSize));
						memcpy_s(newBlock, sizeof(BlockHeader) + groundSize, 
							oldBlock, sizeof(BlockHeader) + PNMIN(oldBlock->mBlockLength, groundSize));

						assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

						// 헤더 세팅
						assert(newBlock->mSplitter == (SplitterType)SplitterValue);

						assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

						newBlock->mBlockLength = groundSize;

						assert(oldBlock->mSplitter==(SplitterType)SplitterValue);

						// 구 블럭을 제거
						oldAlloc->Free(oldBlock);
					}
				}

				assert(newBlock->mBlockLength > 0);
				CheckValidation(alloc, newBlock);
				return GetBlockContent(newBlock);
			}
		}
#else // ENABLE_FAST_HEAP
		return CProcHeap::Realloc(ptr,size);
#endif
	}

	void CFastHeapImpl::Free( void* ptr )
	{
#ifndef DISABLE_FAST_HEAP
		if (m_state != AfterCreation)
		{
			// 예외를 발생시키지 말자. 전역 변수 파괴 단계에서는 예외를 catch할 곳도 없으니까.
			ShowUserMisuseError(_PNT("CFastHeap.Free(): Attempt Free after Heap destruction! Doing Free before WinMain returns in highly recommended. Refer the manual for the resolution."));
		}
		else
		{
			// 블럭 헤더를 얻는다.
			BlockHeader* header = GetValidatedBlockHeader(ptr);
			if (header == NULL)
			{
				throw Exception("Not CFastHeap-allocated block! Refer to ProudNet help 'ProudNet Technical Note' for more help.");
			}

			header->mSplitter = (SplitterType) SplitterValue_Freed; // 못쓰게함

			// 해당하는 allocator를 찾아 제거 시행
			CLookasideAllocator* alloc = GetAllocByGroundSize(header->mBlockLength);
			if (alloc == NULL)
			{
				m_settings.m_pHeap->Free(header);
			}
			else
			{
				alloc->Free(header);
			}
		}
#else // ENABLE_FAST_HEAP
		CProcHeap::Free(ptr);
#endif
	}

	inline CLookasideAllocator* CFastHeapImpl::GetAllocByGroundSize( size_t groundSize )
	{
		if (groundSize == 0)
			return NULL;

		size_t index = (groundSize - 1) / GetGranularity();
#if defined(_WIN32)	
		BoundCheckUInt32(index, __FUNCTION__);
#endif

		if (index < (size_t)BucketCount)
			return m_buckets[(int)index];
		else
			return NULL;
	}

	inline CFastHeapImpl::BlockHeader* CFastHeapImpl::GetValidatedBlockHeader( void* ptr )
	{
		if (ptr == NULL)
			return NULL;

		char* ptr2 = (char*)ptr;
		ptr2 -= sizeof(BlockHeader);
		BlockHeader* header = (BlockHeader*)ptr2;
		if (header->mSplitter != (SplitterType)SplitterValue)
			return NULL;

		return header;
	}

	inline void* CFastHeapImpl::GetBlockContent( BlockHeader* header )
	{
		return (void*)(header + 1);
	}

	inline size_t CFastHeapImpl::GetGroundSize( size_t blockSize )
	{
		if (blockSize == 0)
			return 0;
		else
		{
			size_t gran = GetGranularity();
			// ikpil.choi 2016-11-10 : FastHeap 리팩토링 중 버킷인덱스로 명시적으로 선언
			size_t bucketIndex = (blockSize - 1) / gran;
			return (bucketIndex + 1) * gran;
		}
	}

	inline void CFastHeapImpl::CheckValidation( CLookasideAllocator* alloc, BlockHeader* block )
	{
		_pn_unused(block);
		_pn_unused(alloc);
#ifdef _DEBUG
		size_t groundSize = GetGroundSize(block->mBlockLength);
		assert(GetAllocByGroundSize(groundSize) == alloc);
		assert(block->mSplitter==(SplitterType)SplitterValue);
#endif
	}

	inline size_t CFastHeapImpl::GetGranularity()
	{
		return mAccelBlockSizeLimit / BucketCount;
	}

	int CFastHeapImpl::DebugCheckConsistency()
	{
		for(int i=0;i<BucketCount;i++)
		{
			int d = m_buckets[i]->DebugCheckConsistency();
			if(d != _HEAPOK)
				return d;
		}
		return _HEAPOK;
	}

#if !defined(_WIN32)
    CMemoryHeap* CMemoryHeap::NewFromHeapHandle()
	{
		return CMemoryHeapImpl::NewFromHeapHandle();
	}
    
	CMemoryHeap* CMemoryHeap::NewHeap()
	{
		return CMemoryHeapImpl::NewHeap();
	}
    
    
	CMemoryHeapImpl* CMemoryHeapImpl::NewFromHeapHandle()
	{
		CMemoryHeapImpl* ret = new CMemoryHeapImpl;
		return ret;
	}
    
	CMemoryHeapImpl* CMemoryHeapImpl::NewHeap()
	{
		CMemoryHeapImpl* ret = new CMemoryHeapImpl;
		return ret;
	}
#else
	CMemoryHeap* CMemoryHeap::NewFromHeapHandle(HANDLE hHeap, bool autoDestroy)
	{
		return CMemoryHeapImpl::NewFromHeapHandle(hHeap,autoDestroy);
	}

	CMemoryHeap* CMemoryHeap::NewHeap(bool autoDestroy)
	{
		return CMemoryHeapImpl::NewHeap(autoDestroy);
	}


	CMemoryHeapImpl* CMemoryHeapImpl::NewFromHeapHandle( HANDLE hHeap, bool autoDestroy )
	{
		CMemoryHeapImpl* ret = new CMemoryHeapImpl;
		ret->m_hHeap = hHeap;
		ret->m_autoDestroy = autoDestroy;

		return ret;
	}

	CMemoryHeapImpl* CMemoryHeapImpl::NewHeap( bool autoDestroy )
	{
		CMemoryHeapImpl* ret = new CMemoryHeapImpl;

		// process heap은 CRT debug mode가 뒤섞여 있으므로 비신뢰. 따로 만들어 쓰자. 어차피 이렇게 해두면 메모리 긁는 상황에서 엔진은 자유로울 수 있다.
		ret->m_hHeap = ::HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);

		if(ret->m_hHeap == NULL)
		{
			CErrorReporter_Indeed::Report(_PNT("FATAL: HeapCreate failed!")); // nothrow
		}

		uint32_t HeapFragValue = 2;

		if(CKernel32Api::Instance().HeapSetInformation)
		{
			CKernel32Api::Instance().HeapSetInformation(ret->m_hHeap,
				HeapCompatibilityInformation,
				&HeapFragValue,
				sizeof(HeapFragValue));
		}

		ret->m_autoDestroy = autoDestroy;

		return ret;
	}
#endif

	void* CFirstHeap::Alloc( size_t size )
	{
		return CFirstHeapImpl::GetUnsafeRef().Alloc(size);
	}

	void CFirstHeap::Free( void* ptr )
	{
		CFirstHeapImpl::GetUnsafeRef().Free(ptr);

	}

	void* CFirstHeap::Realloc( void* ptr,size_t size )
	{
		return CFirstHeapImpl::GetUnsafeRef().Realloc(ptr,size);

	}

	CMemoryHeap* CFirstHeap::GetHeap()
	{
		return CFirstHeapImpl::GetUnsafeRef().m_pHeap;
	}

	void CFastHeap::AssureValidBlock(void* block)
	{
#ifndef DISABLE_FAST_HEAP
		CFastHeapImpl::BlockHeader *hdr = (CFastHeapImpl::BlockHeader*)block;
		hdr--; // 바로 앞으로 옮기기
		if(hdr->mSplitter!=(CFastHeapImpl::SplitterType)CFastHeapImpl::SplitterValue)
        {
			throw Exception("Not a fastheap allocated block!");
        }
#endif // ENABLE_FAST_HEAP
	}

	const char* CannotUseFastHeapForFilledCollectionErrorText = "Collection object can use fast heap only before filled.";
}