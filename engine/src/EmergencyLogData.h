#pragma once

#include "../include/ProudNetCommon.h"
#include "FastList2.h"
#include "../include/PnTime.h"


namespace Proud
{
	// EmergencyLogServer로 보낼 data
	class CEmergencyLogData
	{
	public:
		CEmergencyLogData();

		// UTC, timestamp
		timespec m_loggingTime;

		// netClientstat에서 얻은값
		uint64_t m_totalTcpReceiveBytes;
		uint64_t m_totalTcpSendBytes;
		uint64_t m_totalUdpSendCount;
		uint64_t m_totalUdpSendBytes;
		uint64_t m_totalUdpReceiveCount;
		uint64_t m_totalUdpReceiveBytes;

		HostID m_HostID;
		uint64_t m_totalTcpIssueSendBytes;	// tcp Issue를 건 총 Send Byte양
		int m_ioPendingCount;
		int32_t m_connectCount;	// NetClient 객체가 있는동안 NetServer와 연결한 횟수
		uint32_t m_remotePeerCount;		// 연결된 remotePeer 갯수
		uint32_t m_directP2PEnablePeerCount;	// direct로 연결된 remotePeer 갯수
		String m_natDeviceName;			// NAT 장치 이름
		int32_t m_msgSizeErrorCount;		// WSAEMSGSIZE 에러난 횟수
		int32_t m_netResetErrorCount;		// WSAENETRESET(10052) 에러난 횟수
		int32_t m_intrErrorCount; // WSAEINTR (linux에서는 EINTR) 에러난 횟수
		int32_t m_connResetErrorCount;		// WSAECONNRESET(10054) 에러난 횟수

		// os version을 알아내기 위한 변수들
		uint32_t m_osMajorVersion;
		uint32_t m_osMinorVersion;
		uint8_t m_productType;
		uint16_t m_processorArchitecture;

		uint32_t m_serverUdpAddrCount;	// 최근의 server udp addr 갯수
		uint32_t m_remoteUdpAddrCount;	// 최근의 remote udp addr 갯수

		uint32_t m_lastErrorCompletionLength;	// Completion data length < 0 일때 GetLastError값

		struct EmergencyLog
		{
			int m_logLevel;
			LogCategory m_logCategory;
			HostID m_hostID;
			String m_message;
			String m_function;
			int m_line;
		};
		typedef CFastList2<EmergencyLog, int> EmergencyLogList;
		EmergencyLogList m_logList;

		String m_serverAddr;
		uint16_t m_serverPort;

		void CopyTo(CEmergencyLogData& ret);
	};
}
