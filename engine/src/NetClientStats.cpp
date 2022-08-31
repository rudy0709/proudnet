#include "stdafx.h"
#include "../include/NetClientStats.h"

namespace Proud
{
	CNetClientStats::CNetClientStats()
	{
		m_serverUdpEnabled = false;
		m_remotePeerCount = 0;
		m_directP2PEnabledPeerCount = 0;

		m_sendQueueTotalBytes = 0;
		m_sendQueueTcpTotalBytes = 0;
		m_sendQueueUdpTotalBytes = 0;
	}

	String CNetClientStats::ToString() const
	{
		Tstringstream retStream;

		retStream << _PNT("ServerUdpEnabled=") << m_serverUdpEnabled;
		retStream << _PNT(",RemotePeerCount=") << m_remotePeerCount;
		retStream << _PNT(",DirectP2PEnabledPeerCount=") << m_directP2PEnabledPeerCount;
		retStream << _PNT(",TotalUdpReceiveBytes=") << m_totalUdpReceiveBytes;

		String ret;

		ret = retStream.str().c_str();

		return ret;
	}
}
