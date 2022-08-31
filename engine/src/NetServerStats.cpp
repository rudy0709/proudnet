#include "stdafx.h"
#include "../include/NetServerStats.h"

namespace Proud
{
	CNetServerStats::CNetServerStats()
	{
		m_p2pConnectionPairCount = 0;
		m_p2pDirectConnectionPairCount = 0;
		m_clientCount = 0;
		m_realUdpEnabledClientCount = 0;
		m_occupiedUdpPortCount = 0;
	}

	String CNetServerStats::ToString() const
	{
		return String::NewFormat(_PNT("P2P connections=%d, P2P direct connections=%d, Clients=%d, UDP alive clients=d, Used UDP ports=%d"),
			m_p2pConnectionPairCount, m_p2pDirectConnectionPairCount, m_clientCount, m_realUdpEnabledClientCount, m_occupiedUdpPortCount);
	}
}
