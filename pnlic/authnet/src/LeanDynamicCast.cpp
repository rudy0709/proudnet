#include "stdafx.h"
#include "LeanDynamicCast.h"

namespace Proud 
{
	CRemotePeer_C* LeanDynamicCastForRemotePeer(CHostBase* src)
	{
		if (src && src->GetLeanType() == LeanType_CRemotePeer_C)
			return (CRemotePeer_C*)src;
		
		return NULL;
	}

	CRemoteClient_S* LeanDynamicCastForRemoteClient(CHostBase* src)
	{
		if (src && src->GetLeanType() == LeanType_CRemoteClient_S)
		{
			// xxx_cast는 dynamic_cast가 아닌한 굳이 쓰지 마세요. 클래스간 상속 관계에 영향을 주는 경우 이게 문제를 일으킬 수 있어요.
			// xxx_cast는 의도하고자 할때 가령 성능을 올린다던지 등이 있을 때 쓰는거임.
			return (CRemoteClient_S*)src;
		}
		
		return NULL;
	}
}