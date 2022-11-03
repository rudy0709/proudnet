#pragma once

#include "RemotePeer.h"
#include "RemoteClient.h"

namespace Proud 
{
	//CSuperSocket* LeanDynamicCast(IIoEventContext* src);
	CRemotePeer_C* LeanDynamicCastForRemotePeer(CHostBase* src);
	CRemoteClient_S* LeanDynamicCastForRemoteClient(CHostBase* src);
	//CRemoteClient_S* LeanDynamicCast2(IIoEventContext* src);
	//CLanRemoteClient_S* LeanDynamicCastForLanClient(CHostBase* src);
	//CLanRemoteClient_S* LeanDynamicCast2ForLanClient(IIoEventContext* src);

	//CSuperSocket* LeanDynamicCastTcpLayer_C(IIoEventContext* src);
	//CLanRemotePeer_C* LeanDynamicCastRemotePeer_C(IIoEventContext* src);

	//CAcceptedInfo* LeanDynamicCastAcceptedInfo(IIoEventContext* src);
}