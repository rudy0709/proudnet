#include "stdafx.h"
#include "../include/NetCoreStats.h"

namespace Proud
{

	CNetCoreStats::CNetCoreStats()
	{
		m_totalTcpReceiveBytes = 0;
		m_totalTcpReceiveCount = 0;
		m_totalTcpSendBytes = 0;
		m_totalTcpSendCount = 0;
		m_totalUdpReceiveBytes = 0;
		m_totalUdpReceiveCount = 0;
		m_totalUdpSendBytes = 0;
		m_totalUdpSendCount = 0;

		m_totalWebSocketReceiveCount = 0;
		m_totalWebSocketReceiveBytes = 0;
		m_totalWebSocketSendCount = 0;
		m_totalWebSocketSendBytes = 0;
	}

}