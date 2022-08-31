#pragma once

#include "RemotePeer.h"
#include "RemoteClient.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// code profile 결과, dynamic_pointer_cast가 제법 크다.
	// 따라서 코딩이 번잡하더라도 속도를 위해 이렇게 한다.

	template<typename Type, LeanType leanType>
	inline shared_ptr<Type> LeanDynamicCastT(const shared_ptr<CHostBase> &src)
	{
		if (src && src->GetLeanType() == leanType)
			return static_pointer_cast<Type>(src);
		return shared_ptr<Type>();
	}

	template<typename Type, LeanType leanType>
	inline Type* LeanDynamicCastT_PlainPtr(CHostBase* src)
	{
		if (src && src->GetLeanType() == leanType)
			return (Type*)src;
		return NULL;
	}

#define LeanDynamicCast_RemotePeer_C LeanDynamicCastT<CRemotePeer_C, LeanType_CRemotePeer_C>
#define LeanDynamicCast_RemotePeer_C_PlainPtr LeanDynamicCastT_PlainPtr<CRemotePeer_C, LeanType_CRemotePeer_C>
#define LeanDynamicCast_RemoteClient_S LeanDynamicCastT<CRemoteClient_S, LeanType_CRemoteClient_S>
#define LeanDynamicCast_RemoteClient_S_PlainPtr LeanDynamicCastT_PlainPtr<CRemoteClient_S, LeanType_CRemoteClient_S>
}
