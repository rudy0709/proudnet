#include "stdafx.h"
#include "NetServer.h"

namespace Proud
{


#ifdef SUPPORTS_LAMBDA_EXPRESSION

	void CNetServerImpl::Set_OnClientJoin(OnClientJoin_ParamType* handler)
	{
		m_event_OnClientJoin = std::shared_ptr<OnClientJoin_ParamType>(handler);
	}

	void CNetServerImpl::Set_OnClientLeave(std::function<void(CNetClientInfo *, ErrorInfo *, const ByteArray&)> handler)
	{
		m_event_OnClientLeave = handler;
	}

#endif // SUPPORTS_LAMBDA_EXPRESSION

}
