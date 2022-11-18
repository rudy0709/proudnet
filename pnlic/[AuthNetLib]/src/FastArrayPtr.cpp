/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/ByteArrayPtr.h"
#include "../include/CriticalSect.h"
#include "../include/FastArrayPtr.h"
#include "../include/MiniDumper.h"
#include "../include/sysutil.h"
#include "FastArrayPtrImpl.h"
#include "FreeList.h"
#include "PooledObjectAsLocalVar.h"
#include "PooledObjects.h"

namespace Proud
{
	const char* ArrayPtrIsNullError = "error: ArrayPtr is null!";
	const char* ArrayPtrIsNotNullError = "error: ArrayPtr is not null!";
	const char* MustInternalBufferError = "exception due to not using Internal buffer";
	const char* MustNotExternalBufferError = "exception due to using external buffer";
	const char* MustUnsafeInternalBufferError = "exception due to not using Unsafe Internal buffer";
	const char* AlreadyHasInternalBufferError = "Cannot use external buffer because the other typed buffer already exists. Consider using UninitBuffer().";
	const char* AlreadyHasExternalBufferError = "Cannot use internal buffer because the other typed buffer already exists. Consider using UninitBuffer().";
	const char* AlreadyHasUnsafeInternalBufferError = "Cannot use unsafe internal buffer because the other typed buffer already exists. Consider using UninitBuffer().";

	void ThrowArrayIsNullError() 
	{		
#if defined(_WIN32)
		String fn = GetProcessName();
		fn += _PNT(".dmp");

		CMiniDumper::WriteDumpFromHere(fn);
#endif
		throw Exception(ArrayPtrIsNullError);
	}

	ByteArrayPtr::Tombstone* ByteArrayPtr::NewTombstone()
	{
		ByteArrayPtr::Tombstone* ret = CClassObjectPool<ByteArrayPtr::Tombstone>::Instance().NewOrRecycle();
		ret->m_objByteArray.SetCount(0);
		return ret;
	}

	void ByteArrayPtr::FreeTombstone( ByteArrayPtr::Tombstone* tombstone )
	{
		CClassObjectPool<ByteArrayPtr::Tombstone>::Instance().Drop(tombstone);
	}

	void ByteArrayPtr::UninitInternalBuffer()
	{
		if(m_tombstone)
		{
            if(AtomicDecrement32(&m_tombstone->m_refCount) == 0)
			{
				FreeTombstone(m_tombstone);
			}
			m_tombstone = NULL;
		}
	}

}
