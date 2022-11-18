#if defined(_WIN32)

#include "stdafx.h"
#include <new>
#include "EmergencyLogData.h"

namespace Proud
{
	CEmergencyLogData::CEmergencyLogData()
	{
		m_totalTcpReceiveBytes = m_totalTcpSendBytes = m_totalUdpSendCount = m_totalUdpSendBytes = m_totalUdpReceiveCount = m_totalUdpReceiveBytes = 0;
		m_connectCount = 0;
		m_remotePeerCount = 0;
		m_directP2PEnablePeerCount = 0;
		m_msgSizeErrorCount = m_netResetErrorCount = m_intrErrorCount = m_connResetErrorCount = 0;
		m_osMajorVersion = 0;
		m_osMinorVersion = 0;
		m_productType = 0;
		m_processorArchitecture = 0;

		m_serverUdpAddrCount = m_remoteUdpAddrCount = 0;
		m_lastErrorCompletionLength = 0;

		m_logList.Clear();
	}

	void CEmergencyLogData::CopyTo(CEmergencyLogData& ret)
	{
#if defined(_WIN32)
		ret.m_loggingTime = m_loggingTime;
#endif
		ret.m_connectCount = m_connectCount;
		ret.m_connResetErrorCount = m_connResetErrorCount;
		ret.m_directP2PEnablePeerCount = m_directP2PEnablePeerCount;
		ret.m_lastErrorCompletionLength = m_lastErrorCompletionLength;
		ret.m_msgSizeErrorCount = m_msgSizeErrorCount;
		ret.m_natDeviceName = m_natDeviceName;
		ret.m_netResetErrorCount = m_netResetErrorCount;
		ret.m_intrErrorCount = m_intrErrorCount;
		ret.m_osMajorVersion = m_osMajorVersion;
		ret.m_osMinorVersion = m_osMinorVersion;
		ret.m_processorArchitecture = m_processorArchitecture;
		ret.m_productType = m_productType;
		ret.m_remotePeerCount = m_remotePeerCount;
		ret.m_remoteUdpAddrCount = m_remoteUdpAddrCount;
		ret.m_serverUdpAddrCount = m_serverUdpAddrCount;
		ret.m_totalTcpReceiveBytes = m_totalUdpReceiveBytes;
		ret.m_totalTcpSendBytes = m_totalTcpSendBytes;
		ret.m_totalUdpReceiveBytes = m_totalUdpReceiveBytes;
		ret.m_totalUdpReceiveCount = m_totalUdpReceiveCount;
		ret.m_totalUdpSendBytes = m_totalUdpSendBytes;
		ret.m_totalUdpSendCount = m_totalUdpSendCount;

		ret.m_logList.Clear();
		for (EmergencyLogList::const_iterator i = m_logList.begin();
			i != m_logList.end(); i++)
		{
			ret.m_logList.AddTail(*i);
		}
	}
}

#endif // _WIN32
