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
//#include "NetCore.h"

namespace Proud
{
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

		return malloc(size ? size : 1);
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

		free(ptr);
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

		return realloc(ptr, size ? size : 1);
	}
}