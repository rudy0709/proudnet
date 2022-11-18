/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"

#ifdef USE_EMERGENCY_LOG

#include "../include/CoInit.h"
#include "EmergencyLogServer.h"
#include "EmergencyC2S_stub.cpp"
#include "EmergencyS2C_proxy.cpp"
#include "RmiContextImpl.h"
#include "../include/PnTime.h"
#include <comdef.h>
#include <locale>

#ifdef SUPPORTS_LAMBDA_EXPRESSION
#include <codecvt>
#endif


const int logFileSize = 1024 * 1024 * 100;

namespace Proud
{

#ifdef SUPPORTS_LAMBDA_EXPRESSION

	CEmergencyLogServerImpl::CEmergencyLogServerImpl(IEmergencyLogServerDelegate* dg) : m_dg(dg)
	{
		//m_timer.Attach(CMilisecTimer::New(timerType));

		//m_timer->Start();
		m_server.Attach(CNetServer::Create());
		m_server->AttachProxy(&m_s2cProxy);
		m_server->AttachStub(this);
		m_server->SetEventSink(this);
		m_logFileCount = 0;
		CreateLogFile(); // 미리 로그 파일을 생성
	}

	CEmergencyLogServerImpl::~CEmergencyLogServerImpl(void)
	{
	}

	void CEmergencyLogServerImpl::RunMainLoop()
	{
		CCoInitializer coi;

		// server start
		CStartServerParameter sp;
		m_dg->OnStartServer(sp);

		sp.m_protocolVersion = EmergencyProtocolVersion;
		sp.m_threadCount = GetNoofProcessors();

		// udp 사용안함.
		m_server->SetDefaultFallbackMethod(FallbackMethod_ServerUdpToTcp);

		try
		{
			m_server->Start(sp);
		}
		catch (Proud::Exception &e)
		{
			m_dg->OnServerStartComplete(e.m_errorInfoSource);
		}

		m_dg->OnServerStartComplete(ErrorInfoPtr());

		while (m_dg->MustStopNow() == false)
		{
			try
			{
				//m_elapsedTime = (float)m_timer->GetElapsedTimeMs();
				FrameMove();
			}
			catch (_com_error &e)
			{
				e;
				//WARN_COMEXCEPTION(e);
			}
			Sleep(100);
		}
	}

	void CEmergencyLogServerImpl::FrameMove()
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		m_dg->OnFrameMove();
	}

	// 	float CEmergencyLogServerImpl::GetElapsedTime()
	// 	{
	// 		return m_elapsedTime;
	// 	}

	void CEmergencyLogServerImpl::OnClientJoin(CNetClientInfo *clientInfo)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);

		CEmergencyPtr_S newCl(new CEmergencyCli_S(this, clientInfo->m_HostID));
		m_clients.Add(clientInfo->m_HostID, newCl);
	}

	void CEmergencyLogServerImpl::OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorInfo, const ByteArray& comment)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);

		CEmergencyClients::iterator it = m_clients.find(clientInfo->m_HostID);
		if (it->m_pos != NULL)
			m_clients.erase(it);
	}

	DEFRMI_EmergencyC2S_EmergencyLogData_Begin(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			cli->m_logData.m_loggingTime = loggingTime;
			cli->m_logData.m_connectCount = connectCount;
			cli->m_logData.m_remotePeerCount = remotePeerCount;
			cli->m_logData.m_directP2PEnablePeerCount = directP2PEnablePeerCount;
			cli->m_logData.m_natDeviceName = natDeviceName;
			cli->m_logData.m_HostID = peerID;
			cli->m_logData.m_ioPendingCount = iopendingcount;
			cli->m_logData.m_totalTcpIssueSendBytes = totalTcpIssueSendBytes;
		}
		return true;
	}

	DEFRMI_EmergencyC2S_EmergencyLogData_Error(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			cli->m_logData.m_msgSizeErrorCount = msgSizeErrorCount;
			cli->m_logData.m_connResetErrorCount = connResetErrorCount;
			cli->m_logData.m_netResetErrorCount = netResetErrorCount;
			cli->m_logData.m_intrErrorCount = intrErrorCount;
			cli->m_logData.m_lastErrorCompletionLength = lastErrorCompletionLength;
		}
		return true;
	}

	DEFRMI_EmergencyC2S_EmergencyLogData_Stats(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			cli->m_logData.m_totalTcpReceiveBytes = totalTcpReceiveBytes;
			cli->m_logData.m_totalTcpSendBytes = totalTcpSendBytes;
			cli->m_logData.m_totalUdpReceiveBytes = totalUdpReceiveBytes;
			cli->m_logData.m_totalUdpReceiveCount = totalUdpReceiveCount;
			cli->m_logData.m_totalUdpSendBytes = totalUdpSendBytes;
			cli->m_logData.m_totalUdpSendCount = totalUdpSendCount;
		}
		return true;
	}
	DEFRMI_EmergencyC2S_EmergencyLogData_OSVersion(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			cli->m_logData.m_osMajorVersion = osMajorVersion;
			cli->m_logData.m_osMinorVersion = osMinorVersion;
			cli->m_logData.m_processorArchitecture = processorArchitecture;
			cli->m_logData.m_productType = productType;
		}
		return true;
	}
	DEFRMI_EmergencyC2S_EmergencyLogData_LogEvent(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			CEmergencyLogData::EmergencyLog log;
			log.m_logCategory = (LogCategory)logCategory;
			log.m_hostID = logHostID;
			log.m_logLevel = logLevel;
			log.m_message = logMessage;
			log.m_function = logFunction;
			log.m_line = logLine;

			//log.m_addedTime = addedTime;
			//log.m_text = text;

			cli->m_logData.m_logList.AddTail(log);
		}
		return true;
	}
	DEFRMI_EmergencyC2S_EmergencyLogData_End(CEmergencyLogServerImpl)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);

		CEmergencyPtr_S cli = GetClientByHostID(remote);
		if (cli)
		{
			// 나머지 로그 갱신
			cli->m_logData.m_serverUdpAddrCount = serverUdpAddrCount;
			cli->m_logData.m_remoteUdpAddrCount = remoteUdpAddrCount;

			// log파일을 교체한다.
			uint64_t size = m_logFile.tellp();

			if (size >= logFileSize)
			{
				m_logFile.close();

				// 파일을 새로 만든다.
				CreateLogFile();

				// 클라이언트 정보를 파일에 쓴다.
				WriteClientLog(remote, cli->m_logData);
			}
			else
				WriteClientLog(remote, cli->m_logData);

			m_s2cProxy.EmergencyLogData_AckComplete(remote, g_ReliableSendForPN);
		}

		return true;
	}

	bool CEmergencyLogServerImpl::CreateLogFile()
	{
		String filename;
		filename.Format(_PNT("ProudNetEmergencyLog_%d"), m_logFileCount++);

		try
		{
			// Write file in UTF-8 https://gist.github.com/leafbird/5859381
			throw Exception("Code below are not tested yet!");
			m_logFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<PNTCHAR, 0x10ffff, std::generate_header>));
			m_logFile.open(filename, ios::app | ios::out);
		}
		catch (std::exception&)
		{
			return false;
		}

		return true;
	}

	void CEmergencyLogServerImpl::WriteClientLog(HostID hostid, const CEmergencyLogData& data)
	{
		// 클라이언트 하나의 정보를 순서대로 적는다.

		CPnTime loggingTime(data.m_loggingTime);
		StringW loggingTimeString;
		loggingTimeString.Format(_PNT("%d/%d/%d, %d:%d:%d (%dms)"),
			loggingTime.GetYear(),
			loggingTime.GetMonth(),
			loggingTime.GetDay(),
			loggingTime.GetHour(),
			loggingTime.GetMinute(),
			loggingTime.GetSecond(),
			loggingTime.GetMillisecond());

		String text;
		text.Format(_PNT("%s | %s | %s | Client HostID:%d,iopendingcount:%d, totalTcpSendBytes:%u, ConnectCount:%u, NatDeviceName:%s\r\n"), &loggingTimeString,
			_PNT("INFO"), _PNT("proudnet.clientinfo"), data.m_HostID, data.m_ioPendingCount, data.m_totalTcpIssueSendBytes, data.m_connectCount, data.m_natDeviceName);
		m_logFile << text;

		text.Format(_PNT("%s | %s | %s | RemotePeerCount:%u, DirectPeerCount:%u\r\n"), &loggingTimeString,
			_PNT("INFO"), _PNT("proudnet.clientinfo.p2pstat"), data.m_remotePeerCount, data.m_directP2PEnablePeerCount);
		m_logFile << text;

		text.Format(_PNT("%s | %s | %s | TotalTcpRecevieBytes:%u, TotalTcpSendBytes:%u, TotalUdpReceiveBytes:%u, TotalUdpSendBytes:%u, TotalUdpReceiveCount:%u, TotalUdpSendCount:%u\r\n"),
			&loggingTimeString,
			_PNT("INFO"), _PNT("proudnet.clientinfo.netstat"), data.m_totalTcpReceiveBytes, data.m_totalTcpSendBytes,
			data.m_totalUdpReceiveBytes, data.m_totalUdpSendBytes, data.m_totalUdpReceiveCount, data.m_totalUdpSendCount);
		m_logFile << text;

		text.Format(_PNT("%s | %s | %s | OSMajorVersion:%u, OSMinorVersion:%u, ProcessArchitecture:%u, ProductType:%u \r\n"), &loggingTimeString,
			_PNT("INFO"), _PNT("proudnet.clientinfo.os"), data.m_osMajorVersion, data.m_osMinorVersion,
			data.m_processorArchitecture, data.m_productType);
		m_logFile << text;

		text.Format(_PNT("%s | %s | %s | ServerUdpAddrCount:%u, RemoteUdpAddrCount:%u\r\n"), &loggingTimeString,
			_PNT("INFO"), _PNT("proudnet.clientinfo.udpaddrcount"), data.m_serverUdpAddrCount, data.m_remoteUdpAddrCount);
		m_logFile << text;

		text.Format(_PNT("%s | %s | %s | MsgsizeErrorCount:%u, ConnResetErrorCount:%u, NetResetErrorCount:%u, IntrErrorCount:%u, GetLastErrorToCompletionDataLength:%u\r\n"), &loggingTimeString,
			_PNT("ERROR"), _PNT("proudnet.clientinfo.error"), data.m_msgSizeErrorCount, data.m_connResetErrorCount, data.m_netResetErrorCount, data.m_intrErrorCount,
			data.m_lastErrorCompletionLength);
		m_logFile << text;

		// 로그 추가 시간 계산
		CPnTime currentTime = CPnTime::GetCurrentTime();
		String addedDateAndTime;
		addedDateAndTime.Format(_PNT("%d-%d %d:%d:%d.%d"), currentTime.GetMonth(),
			currentTime.GetDay(),
			currentTime.GetHour(),
			currentTime.GetMinute(),
			currentTime.GetSecond(),
			currentTime.GetMillisecond());

		CEmergencyLogData::EmergencyLogList::const_iterator it;
		for (it = data.m_logList.begin(); it != data.m_logList.end(); it++)
		{
			const CEmergencyLogData::EmergencyLog& logData = *it;

			if (logData.m_function.IsEmpty())
			{
				// Non-Use Function / Line		
				text.Format(_PNT("%s: %d / %s(%d): %s\r\n"), addedDateAndTime,
					logData.m_logLevel,
					ToString(logData.m_logCategory),
					logData.m_hostID,
					logData.m_message);
			}
			else
			{
				// Use Function / Line 
				text.Format(_PNT("%s: %d / %s(%d): %s {%s(%d)}\r\n"), addedDateAndTime,
					logData.m_logLevel,
					ToString(logData.m_logCategory),
					logData.m_hostID,
					logData.m_message.GetString(),
					logData.m_function.GetString(),
					logData.m_line);
			}

			// 			text.Format(_PNT("%s | %s | %s | LogCategory:%d Log:%s\r\n"), 
			// 				(const PNTCHAR*)CTime((*it).m_addedTime).Format(_PNT("%m/%d/%Y, %H:%M:%S (T0+0ms)")),
			// 				_PNT("LOG"),
			// 				_PNT("proudnet.clientinfo.emergencylog"),
			// 				(*it).m_logCategory,
			// 				(*it).m_text);

			m_logFile << text;
		}
	}

	Proud::CEmergencyPtr_S CEmergencyLogServerImpl::GetClientByHostID(HostID hostid)
	{
		CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);

		CEmergencyClients::iterator it = m_clients.find(hostid);
		return it->GetSecond();
	}

	CEmergencyLogServer* CEmergencyLogServer::Create(IEmergencyLogServerDelegate* dg)
	{
		return new CEmergencyLogServerImpl(dg);
	}

	CEmergencyCli_S::CEmergencyCli_S(CEmergencyLogServerImpl* server, HostID hostID)
	{
		m_server = server;
		m_hostID = hostID;
	}

	CEmergencyCli_S::~CEmergencyCli_S(void)
	{

	}
#endif
}

#endif

#endif // _WIN32
