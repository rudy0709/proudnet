﻿/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/Ptr.h"
#include "../include/FastHeap.h"
#include "../include/Singleton.h"
#include "../include/Exception.h"
#include "../include/LookasideAllocator.h"
#include "../include/CriticalSect.h"
#include "PtrImpl.h"

using namespace std;
namespace Proud
{
	void* RefCount_LookasideAllocator_Alloc(size_t length)
	{
		return CRefCountHeap::Instance().m_heap->Alloc(length);
	}

	void RefCount_LookasideAllocator_Free(void* ptr)
	{
		CRefCountHeap::Instance().m_heap->Free(ptr);

	}
}