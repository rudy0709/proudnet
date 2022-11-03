#pragma once

#include "RemoteBase.h"

namespace Proud
{
	class CLoopbackHost : public CHostBase
	{
	public:
		CLoopbackHost(HostID hostID);
		~CLoopbackHost();

		virtual HostID GetHostID();

		virtual LeanType GetLeanType() { return LeanType_Loopback;  }

		HostID m_backupHostID;
	};

}