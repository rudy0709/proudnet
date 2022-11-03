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
		String ret;
		ret.Format(_PNT("ServerUdpEnabled=%d,RemotePeerCount=%d,DirectP2PEnabledPeerCount=%d,TotalUdpReceiveBytes=%I64d"),
			m_serverUdpEnabled,
			m_remotePeerCount,
			m_directP2PEnabledPeerCount,
			m_totalUdpReceiveBytes);

		return ret;
	}
}