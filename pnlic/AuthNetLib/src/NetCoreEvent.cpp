#include "stdafx.h"
#include <new>
#include "../include/INetCoreEvent.h"
#include "../include/RMIContext.h"

namespace Proud
{
	INetCoreEvent::~INetCoreEvent()
	{
	}

	void INetCoreEvent::OnReceiveUserMessage(HostID /*sender*/, const RmiContext& /*rmiContext*/, uint8_t* /*payload*/, int /*payloadLength*/)
	{
	}
}