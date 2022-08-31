#pragma once

#include "../include/BasicTypes.h"
#include "../include/Ptr.h"
#include "../include/Singleton.h"
#include "PooledObjects.h"
#include "FinalUserWorkItem.h"

#define DECLARE_DLL_SINGLETON_ClassObjectPool(DLLSPEC, CLASS) \
DECLARE_DLL_SINGLETON(DLLSPEC, CClassObjectPool<CLASS>); \
DLLSPEC void GetClassObjectPoolInDll(CClassObjectPool<CLASS>** output)

// GetDllSingletonSharedPtr, GetDllSingletonRawPtr는 DllSingleton에서 요구됨
// GetClassObjectPoolInDll는 CClassObjectPool에서 요구됨
#define IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CLASS) \
	JitObjectCreator<CClassObjectPool<CLASS> > g_singleton_pool_##CLASS; \
	void GetDllSingletonSharedPtr(RefCount<CClassObjectPool<CLASS> >* output) \
	{ \
		*output = g_singleton_pool_##CLASS.GetJitCreatedObject(); \
	} \
	void GetDllSingletonRawPtr(CClassObjectPool<CLASS>** output) \
	{ \
		*output = g_singleton_pool_##CLASS.GetJitCreatedObjectPtr(); \
	} \
	void GetClassObjectPoolInDll(CClassObjectPool<CLASS>** output) \
	{ \
		*output = g_singleton_pool_##CLASS.GetJitCreatedObjectPtr(); \
	}

namespace Proud
{
	PROUD_API void PooledObjects_ShrinkOnNeed_ClientDll();

	class WSABUF_Array;
	class CSendFragRefs;
	class DefraggingPacket;
	class FragArray;
	class HostIDArray;
	class CReceivedMessageList;
	class CompressedRelayDestList_C;
	class RelayDestList_C;
	class RelayDestList;
	class CSuperSocketArray;
	class CIoEventStatusList;
	class SendDestInfoArray;
	class SendDestInfoPtrArray;
	class RemoteArray;

	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, WSABUF_Array);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, CSendFragRefs);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, DefraggingPacket);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, FragArray);
	DECLARE_DLL_SINGLETON(PROUD_API, CClassObjectPool<ByteArrayPtr_Tombstone>);
	PROUD_API void GetClassObjectPoolInDll(CClassObjectPool<ByteArrayPtr_Tombstone>** output);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, HostIDArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, CReceivedMessageList);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, CompressedRelayDestList_C);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, RelayDestList_C);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, RelayDestList);
	DECLARE_DLL_SINGLETON(PROUD_API, CClassObjectPool<CFinalUserWorkItem::Tombstone>);
	PROUD_API void GetClassObjectPoolInDll(CClassObjectPool<CFinalUserWorkItem::Tombstone>** output);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, CSuperSocketArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, CIoEventStatusList);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, SendDestInfoArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, SendDestInfoPtrArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUD_API, RemoteArray);
}
