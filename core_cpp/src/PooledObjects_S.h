#pragma once 

#include "PooledObjects_C.h"
#include "PooledObjects.h"

namespace Proud
{
	PROUDSRV_API void PooledObjects_ShrinkOnNeed_ServerDll();


	class SuperPeerCandidateArray;
	class P2PGroupSubsetList;
	class ReliableDestInfoArray;
	class UnreliableDestInfoArray;

	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUDSRV_API, SuperPeerCandidateArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUDSRV_API, P2PGroupSubsetList);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUDSRV_API, ReliableDestInfoArray);
	DECLARE_DLL_SINGLETON_ClassObjectPool(PROUDSRV_API, UnreliableDestInfoArray);

}