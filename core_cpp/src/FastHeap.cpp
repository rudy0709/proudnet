/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/FastHeap.h"
#include "../include/Singleton.h"
#include "../include/sysutil.h"
#include "FastHeapImpl.h"

// NOTE: 메모리를 긁히는 문제가 발생하나, 프라우드넷 내부 문제인지 여부를 걸러내려면, _ISOLATE_PROUDNET_HEAP를 켜세요.
// _ISOLATE_PROUDNET_HEAP를 켜면 프라우드넷 내부에서의 heap은 별도의 win32 heap handle을 갖고 합니다. 따라서 사용자의 메모리 긁힘의 영향을 안 받습니다.
// _ISOLATE_PROUDNET_HEAP는 실험 용도로만 사용하세요.

#if defined(_ISOLATE_PROUDNET_HEAP)
#if !defined(_WIN32)
#error Cannot use ISOLATE_PROUDNET_HEAP in non-Windows.
#endif 
#endif 

#if defined(_ISOLATE_PROUDNET_HEAP)
#include <mutex>
#endif

namespace Proud
{
#if defined(_ISOLATE_PROUDNET_HEAP)
	class ProudNetInternalHeap
	{
	public:
		ProudNetInternalHeap() {}

		HANDLE GetHandle()
		{
			if (0 != m_heap)
				return m_heap;

			static std::mutex mutex;
			std::lock_guard<std::mutex> lock(mutex);

			if (0 != m_heap)
				return m_heap;

			m_heap = ::HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);

			return m_heap;
		}

	private:
		HANDLE m_heap;
	};

	ProudNetInternalHeap g_proudNetInternalHeap;
#endif

	CFastHeap::CFastHeap()
	{
	}

	CFastHeap* CFastHeap::New( size_t AccelBlockSizeLimit, const CFastHeapSettings& settings)
	{
		return new CFastHeapImpl(AccelBlockSizeLimit, settings);
	}

	CFastHeap::~CFastHeap()
	{

	}

	void* CProcHeap::Alloc( size_t size )
	{
		// NOTE: Windows 에서 HeapAlloc deadlock 현상 발생때문에
		//		 Windows 도 C Runtime function 을 사용하도록 변경
// #if !defined(_WIN32)
//         return malloc(size ? size : 1);
// #else
// 		return ::HeapAlloc(GetProcessHeap(), 0,size?size:1 );
// #endif

#if defined(_ISOLATE_PROUDNET_HEAP)
		return ::HeapAlloc(g_proudNetInternalHeap.GetHandle(), 0, size ? size : 1);
#else
		return malloc(size ? size : 1);
#endif
	}

	void CProcHeap::Free( void* ptr )
	{
		// NOTE: Windows 에서 HeapAlloc deadlock 현상 발생때문에
		//		 Windows 도 C Runtime function 을 사용하도록 변경
// #if !defined(_WIN32)
//         free(ptr);
// #else
// 		::HeapFree(GetProcessHeap(), 0, ptr);
// #endif

#if defined(_ISOLATE_PROUDNET_HEAP)
		::HeapFree(g_proudNetInternalHeap.GetHandle(), 0, ptr);
#else
		free(ptr);
#endif
	}

	void* CProcHeap::Realloc( void* ptr,size_t size )
	{
		// NOTE: Windows 에서 HeapAlloc deadlock 현상 발생때문에
		//		 Windows 도 C Runtime function 을 사용하도록 변경
// #if !defined(_WIN32)
//         return realloc(ptr, size ? size : 1);
// #else
// 		return ::HeapReAlloc(GetProcessHeap(), 0, ptr, size ? size : 1 );
// #endif

#if defined(_ISOLATE_PROUDNET_HEAP)
		return ::HeapReAlloc(g_proudNetInternalHeap.GetHandle(), 0, ptr, size ? size : 1);
#else
		return realloc(ptr, size ? size : 1);
#endif
	}
}