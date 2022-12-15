#include "stdafx.h"
#include "PooledObjects_C.h"
#include "BufferSegment.h"
#include "SendFragRefs.h"
#include "PacketFrag.h"
#include "ReceivedMessageList.h"
#include "ioevent.h"
#include "NetClientManager.h"
#include "SuperSocket.h"
#include "DefaultStringEncoder.h"
#include "../include/MiniDumper.h"
#include "../include/strpool.h"
#include "../include/ByteArrayPtr.h"
#include "../include/sysutil.h"

namespace Proud
{
	void PooledObjects_ShrinkOnNeed_ClientDll()
	{
		// TODO: 여기다 각 멤버의 ShrinkOnNeed를 호출
		CClassObjectPool<WSABUF_Array>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CSendFragRefs>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<DefraggingPacket>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<FragArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<ByteArrayPtr::Tombstone>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<HostIDArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CReceivedMessageList>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CompressedRelayDestList_C>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<RelayDestList_C>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<RelayDestList>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CFinalUserWorkItem_Tombstone>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CSuperSocketArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<CIoEventStatusList>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<SendDestInfoArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<SendDestInfoPtrArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<RemoteArray>::GetSharedPtr()->ShrinkOnNeed();
	}

	//***주의*** 싱글톤 생성&파괴순서를 결정합니다. 변수 선언 순서를 지키십시오.

	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(WSABUF_Array);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CSendFragRefs);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(DefraggingPacket);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(FragArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(ByteArrayPtr_Tombstone);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(HostIDArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CReceivedMessageList);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CompressedRelayDestList_C);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(RelayDestList_C);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(RelayDestList);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CFinalUserWorkItem_Tombstone);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CSuperSocketArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(CIoEventStatusList);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(SendDestInfoArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(SendDestInfoPtrArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(RemoteArray);
	
	IMPLEMENT_DLL_SINGLETON(CStringPool);
	IMPLEMENT_DLL_SINGLETON(CDefaultStringEncoder);

#ifdef _WIN32
	IMPLEMENT_DLL_SINGLETON(CLocale);

	IMPLEMENT_DLL_SINGLETON(CSystemInfo);

	IMPLEMENT_DLL_SINGLETON(CExceptionLogger);

	IMPLEMENT_DLL_SINGLETON(CMiniDumper);

	IMPLEMENT_DLL_SINGLETON(CTimerQueue);

#endif // _WIN32



	IMPLEMENT_DLL_SINGLETON(CNetClientManager);


}