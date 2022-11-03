#pragma once

#include "../include/FastHeap.h"
#include "../include/LookasideAllocator.h"
#include "../include/FastArrayPtr.h"
#include "../include/sysutil.h"
#if defined(_WIN32) || defined(__MARMALADE__)
#include <malloc.h>
#endif

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	// fast heap은 자체는 critical section을 가지지 않는다. 내부의 bucket들만이 가진다.
	// ** 이 클래스는 컴파일러의 최적화 옵션에 따라 성능이 두배까지 차이가 난다. **
	class CFastHeapImpl: public CFastHeap
	{
	private:
		// 만약 이미 파괴된 상태라면 0xfe 등 non allocated heap 공간에서의 값이 리턴될 것이다.
		enum State
		{
			BeforeCreation = 0x01,
			AfterCreation,
			AfterDestruction,
		};

		PROUDNET_VOLATILE_ALIGN int m_state;
		CFastHeapSettings m_settings;

	public:

// #ifdef _DEBUG
// 		typedef CLookasideAllocatorInternal* SplitterType;
// #else
		typedef uint16_t SplitterType; // 릴리즈 빌드에서는 사이즈를 줄이자.
//#endif

		struct BlockHeader
		{
			SplitterType mSplitter;
			/* 할당된 메모리 블럭 크기
			ground size이다.
			self size 비포함 */
			size_t mBlockLength;
		};
		enum { BucketCount = 128, SplitterValue = 1818, SplitterValue_Freed = 2828 };

		CFastHeapImpl( size_t AccelBlockSizeLimit, const CFastHeapSettings& settings);
		~CFastHeapImpl(void);

		void* Alloc( size_t size );
		void* Realloc( void* ptr, size_t size );
		void Free( void* ptr );
	private:
		size_t GetGroundSize(size_t blockSize);

		size_t GetGranularity();
		CLookasideAllocator* GetAllocByGroundSize(size_t groundSize);
		void CheckValidation(CLookasideAllocator* alloc, BlockHeader* block);
		BlockHeader* GetValidatedBlockHeader(void* ptr);
		void* GetBlockContent(BlockHeader* header);

		size_t mAccelBlockSizeLimit;

		CLookasideAllocator** m_buckets;
	public:
#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif

		virtual int DebugCheckConsistency();

	};

	class CMemoryHeapImpl:public CMemoryHeap
	{
#if defined(_WIN32)
		HANDLE m_hHeap;
		bool m_autoDestroy;
#endif

		CMemoryHeapImpl() {}
	public:
		void* Alloc(size_t size)
		{
#if !defined(_WIN32)
			return malloc(size ? size : 1);
#else
			return ::HeapAlloc(m_hHeap, 0, size ? size : 1);
#endif
		}

		void* Realloc(void* ptr,size_t size)
		{
#if !defined(_WIN32)
			return realloc(ptr, size ? size : 1);
#else
			return ::HeapReAlloc(m_hHeap, 0, ptr, size ? size : 1);
#endif
		}
		void Free(void* ptr)
		{
#if !defined(_WIN32)
			free(ptr);
#else
			if(m_hHeap == NULL)
			{
				ShowUserMisuseError(_PNT("CMemoryHeap.Free(): Attempt Free after Heap destruction! Doing Free before WinMain returns in highly recommended. Refer the manual for the resolution."));
			}
			else
				::HeapFree(m_hHeap, 0, ptr);
#endif
		}
		int DebugCheckConsistency() 
		{
#if !defined(_WIN32)
			return _HEAPOK;
#else
			::HeapValidate(m_hHeap, 0, NULL);
			return _HEAPOK;
#endif
		}

		~CMemoryHeapImpl() 
		{
#if defined(_WIN32)
			if(m_autoDestroy)
			{
				::HeapDestroy(m_hHeap);
				m_hHeap = NULL;
			}
#endif
		}

#if !defined(_WIN32)
		static CMemoryHeapImpl* NewFromHeapHandle();
		static CMemoryHeapImpl* NewHeap();

#else
		static CMemoryHeapImpl* NewFromHeapHandle(HANDLE hHeap, bool autoDestroy);
		static CMemoryHeapImpl* NewHeap(bool autoDestroy);
#endif
	};


	class CFirstHeapImpl:public CSingleton<CFirstHeapImpl>
	{
		CMemoryHeap* m_pHeap;
		friend class CFirstHeap;
	public:
		CFirstHeapImpl()
		{
#if !defined(_WIN32)
			m_pHeap = CMemoryHeap::NewHeap();
#else
			m_pHeap = CMemoryHeap::NewHeap(true);
#endif

		}

		~CFirstHeapImpl()
		{
			// 이게 없으니 Memory leak이 감지된다.
			delete m_pHeap;
			m_pHeap = NULL;
		}

		void* Alloc(size_t size)
		{
			return m_pHeap->Alloc(size);
		}

		void* Realloc(void* ptr,size_t size)
		{
			return m_pHeap->Realloc(ptr,size);

		}
		void Free(void* ptr)
		{
			if(m_pHeap == NULL)
			{
				ShowUserMisuseError(_PNT("CFirstHeap.Free(): Attempt Free after Heap destruction! Doing Free before WinMain returns in highly recommended. Refer the manual for the resolution."));
			}
			else
				m_pHeap->Free(ptr);
		}
	};



#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}