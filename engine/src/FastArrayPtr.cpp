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
#include "FastArayPtrImpl.h"
#include "FreeList.h"
#include "PooledObjectAsLocalVar.h"
#include "FavoriteLV.h"
#include "PooledObjects_C.h"

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

	// Unreal Engine에서는 throw 키워드 사용 불가. 따라서 유저에게 공개되는 API에서는 throw 키워드를 쓰지 않는다.
	void ThrowArrayIsNullError()
	{
#if defined(_WIN32)
		String fn = GetProcessName();
		fn += _PNT(".dmp");

		CMiniDumper::WriteDumpFromHere(fn.GetString());
#endif
		throw Exception(ArrayPtrIsNullError);
	}


	// #UE4_PATCH BiasManagedPointer_IMPLEMENT_TOMBSTONE에서 복붙함.
	void* ByteArrayPtr_AllocTombstone()
	{
		typedef  BiasManagedPointer<ByteArray, true>::Tombstone TombstoneType;
		TombstoneType* ret = CClassObjectPool<TombstoneType>::GetUnsafeRef().NewOrRecycle();
		return ret;
	}

	void ByteArrayPtr_FreeTombstone(void* tombstone)
	{
		typedef  BiasManagedPointer<ByteArray, true>::Tombstone TombstoneType;
		CClassObjectPool<TombstoneType>::GetUnsafeRef().Drop((TombstoneType*)tombstone);
	}
}
