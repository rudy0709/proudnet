#pragma once

#include "RemoteBase.h"
#include "P2PGroup_S.h"

namespace Proud
{
	class CNetServerImpl;

	class CLoopbackHost_S
		: public CHostBase
		, public CP2PGroupMemberBase_S
	{
	public:
		CNetCoreImpl* m_owner;
		virtual CriticalSection& GetOwnerCriticalSection();

		CLoopbackHost_S(CNetServerImpl* owner);
		~CLoopbackHost_S();

		virtual HostID GetHostID();

		virtual LeanType GetLeanType() { return LeanType_Loopback;  }
	};
}
