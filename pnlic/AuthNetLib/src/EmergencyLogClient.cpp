/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/


#include "stdafx.h"

#if defined(_WIN32)

#include <new>
#include "../include/ProudNetClient.h"
#include "RmiContextImpl.h"
#include "EmergencyLogClient.h"
#include "EmergencyS2C_stub.cpp"
#include "EmergencyC2S_proxy.cpp"

namespace Proud
{

	CEmergencyLogClient::CEmergencyLogClient(void)
	{
		m_logData = NULL;
		m_state=Connecting;
		m_client.Attach(CNetClient::Create());
		m_client->AttachProxy(&m_c2sProxy);
		m_client->AttachStub(this);
		m_client->SetEventSink(this);
	}

	CEmergencyLogClient::~CEmergencyLogClient(void)
	{
		m_client.Free();
	}

	void CEmergencyLogClient::OnJoinServerComplete( ErrorInfo *info, const ByteArray &replyFromServer )
	{
		if(info->m_errorType!=ErrorType_Ok)
			throw Exception("Failed to connect the Emergency log Server");
		else
		{
			// 모아둔 로그를 보내자. 크기는 크지 않을테니 한방에 보낸다.
			m_state = Sending;
			if(m_logData)
			{
				m_c2sProxy.EmergencyLogData_Begin(HostID_Server, g_ReliableSendForPN, m_logData->m_loggingTime,
					m_logData->m_connectCount, m_logData->m_remotePeerCount, m_logData->m_directP2PEnablePeerCount, m_logData->m_natDeviceName,m_logData->m_HostID,m_logData->m_ioPendingCount, m_logData->m_totalTcpIssueSendBytes);
				m_c2sProxy.EmergencyLogData_Error(HostID_Server, g_ReliableSendForPN, m_logData->m_msgSizeErrorCount, m_logData->m_netResetErrorCount, m_logData->m_intrErrorCount, m_logData->m_connResetErrorCount, m_logData->m_lastErrorCompletionLength);
				m_c2sProxy.EmergencyLogData_Stats(HostID_Server, g_ReliableSendForPN, m_logData->m_totalTcpReceiveBytes, m_logData->m_totalTcpSendBytes, m_logData->m_totalUdpSendCount, m_logData->m_totalUdpSendBytes, m_logData->m_totalUdpReceiveCount, m_logData->m_totalUdpReceiveBytes);
				m_c2sProxy.EmergencyLogData_OSVersion(HostID_Server, g_ReliableSendForPN, m_logData->m_osMajorVersion, m_logData->m_osMinorVersion, m_logData->m_productType, m_logData->m_processorArchitecture);

				CEmergencyLogData::EmergencyLogList::const_iterator it;
				for(it=m_logData->m_logList.begin(); it!=m_logData->m_logList.end();it++)
				{
					const CEmergencyLogData::EmergencyLog& log = *it;

					m_c2sProxy.EmergencyLogData_LogEvent(HostID_Server,
						g_ReliableSendForPN,
						log.m_logLevel,
						log.m_logCategory,
						log.m_hostID,
						log.m_message,
						log.m_function,
						log.m_line);
				}

				m_c2sProxy.EmergencyLogData_End(HostID_Server, g_ReliableSendForPN, m_logData->m_serverUdpAddrCount, m_logData->m_remoteUdpAddrCount);
			}
		}

	}

	DEFRMI_EmergencyS2C_EmergencyLogData_AckComplete(CEmergencyLogClient)
	{
		// log도착한게 확인됐다. 클라를 닫자.
		m_state = Closing;
		return true;
	}

	void CEmergencyLogClient::OnLeaveServer( ErrorInfo *errorInfo )
	{
		m_state = Stopped;
	}

	void CEmergencyLogClient::OnP2PMemberJoin( HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField )
	{

	}

	void CEmergencyLogClient::OnP2PMemberLeave( HostID memberHostID, HostID groupHostID, int memberCount )
	{

	}

	
	void CEmergencyLogClient::OnException( Exception &e )
	{
		// Emergencylog를 보낼수 없는 상황이므로 종료 하도록한다.
		NTTNTRACE("CEmergencyLogClient.OnException:%s\n", e.what());
		m_state = Closing;
	}

	void CEmergencyLogClient::OnError( ErrorInfo *errorInfo )
	{
		// Emergencylog를 보낼수 없는 상황이므로 종료 하도록한다.
		NTTNTRACE("CEmergencyLogClient.OnError:%s\n", StringT2A(errorInfo->ToString()));
		m_state = Closing;
	}

	void CEmergencyLogClient::Start( String serverAddr, uint16_t serverPort, CEmergencyLogData* logData )
	{
		m_logData = logData;
		CNetConnectionParam p;
		p.m_protocolVersion = EmergencyProtocolVersion;
		p.m_serverIP=serverAddr;
		p.m_serverPort=serverPort;

		if(!m_client->Connect(p))
			throw Exception("Failed to connect the Emergency log Server");
	}

	void CEmergencyLogClient::FrameMove()
	{
		m_client->FrameMove();

		if(m_state==Closing)
		{
			m_client->Disconnect();
			m_state = Stopped;
		}
	}

	CEmergencyLogClient::State CEmergencyLogClient::GetState()
	{
		return m_state;
	}

}

#endif // _WIN32
