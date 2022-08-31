#pragma once

namespace Proud
{
	enum LeanType
	{
		LeanType_None,
		LeanType_Loopback,
		LeanType_CNetClient,
		LeanType_CNetServer,
		LeanType_CRemotePeer_C,
		LeanType_CRemoteServer_C,
		LeanType_CRemoteClient_S,
		LeanType_CLanRemoteClient_S,
		LeanType_CLanRemotePeer_C,
	};
}
