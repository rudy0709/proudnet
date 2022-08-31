#include "stdafx.h"
#include "PooledObjects_S.h"
#include "SuperPeer_S.h"
#include "SendDest_S.h"
#include "P2PGroup_S.h"
#ifdef _WIN32
#include "../include/NTService.h"
#endif // _WIN32

namespace Proud
{

	void PooledObjects_ShrinkOnNeed_ServerDll()
	{
		// TODO: 여기다 각 멤버의 ShrinkOnNeed를 호출
		CClassObjectPool<SuperPeerCandidateArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<P2PGroupSubsetList>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<ReliableDestInfoArray>::GetSharedPtr()->ShrinkOnNeed();
		CClassObjectPool<UnreliableDestInfoArray>::GetSharedPtr()->ShrinkOnNeed();
	}

	//***주의*** 싱글톤 생성&파괴순서를 결정합니다. 변수 선언 순서를 지키십시오.

	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(SuperPeerCandidateArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(P2PGroupSubsetList);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(ReliableDestInfoArray);
	IMPLEMENT_DLL_SINGLETON_ClassObjectPool(UnreliableDestInfoArray);

#ifdef _WIN32
	IMPLEMENT_DLL_SINGLETON(CNTService);
#endif // _WIN32
}
