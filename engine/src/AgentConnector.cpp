/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/ProudNetServer.h"
#include "AgentConnector.h"
#include "AgentC2S_common.cpp"
#include "AgentC2S_proxy.cpp"
#include "AgentS2C_common.cpp"
#include "AgentS2C_stub.cpp"
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace Proud
{
	CAgentConnector* CAgentConnector::Create(IAgentConnectorDelegate* dg)
	{		
		return new CAgentConnectorImpl(dg);
	}

	CAgentConnectorImpl::CAgentConnectorImpl( IAgentConnectorDelegate* dg) : m_dg(dg)
	{
		m_client = NULL;

		m_delayTimeAboutSendAgentStatus = 1000;
		m_prevKernelTime100Ns = 0;
		m_prevUserTime100Ns = 0;
		m_prevInstrumentedTimeMs = 0;
	}

	CAgentConnectorImpl::~CAgentConnectorImpl()
	{

	}

	void CAgentConnectorImpl::OnJoinServerComplete( ErrorInfo *info, const ByteArray &replyFromServer )
	{
		replyFromServer;

		if(info->m_errorType != ErrorType_Ok)
		{
			NTTNTRACE("OnJoinServerComplete is failed. ErrorInfo:%s\n", StringT2A(info->ToString()));
		}

		m_proxy.RequestCredential(HostID_Server, RmiContext::SecureReliableSend, m_cookie);
	}

	void CAgentConnectorImpl::OnLeaveServer( ErrorInfo *errorInfo )
	{
		if(errorInfo->m_errorType != ErrorType_Ok)
		{
			NTTNTRACE("OnLeaveServer is failed. ErrorInfo:%s\n", StringT2A(errorInfo->ToString()));
		}
	}

	bool CAgentConnectorImpl::Start()
	{
		CNetConnectionParam param;
		
		// Agent가 CreateProcess할때 넣은 환경변수를 구하기
		char szport[128], szcookie[128];
		uint32_t ret1 = GetEnvironmentVariableA("agent_port", szport, 128);
		uint32_t ret2 = GetEnvironmentVariableA("agent_cookie", szcookie, 128);

 		if(ret1 == 0 || ret2 == 0)
 		{
			NTTNTRACE("GetEnvironmentVariable is failed. GetLastError : %u\n", GetLastError());
 			return false;
 		}

		m_cookie = atoi(szcookie);
		param.m_serverPort = static_cast<uint16_t>(atoi(szport));
		param.m_serverIP = _PNT("localhost"); // ipv4,6 모두 같이 써야 하니까. 127.0.0.1이나 ::1 쓰지 말자.
		
		m_client = CNetClient::Create();

		m_client->SetEventSink(this);
		m_client->AttachProxy(&m_proxy);
		m_client->AttachStub(this);
		
		m_lastTryConnectTime = CFakeWin32::GetTickCount();

		ErrorInfoPtr outError;
		if(!m_client->Connect(param, outError))
		{
			NTTNTRACE("Connect is failed.\n");
			return false;
		}

		m_lastSendStatusTime = CFakeWin32::GetTickCount();
		return true;
	}

	bool CAgentConnectorImpl::SendReportStatus( CReportStatus& reportStatus )
	{
		if(m_client == NULL)
		{
			return false;
		}

		// 문자열이 비어서는 안된다.
		if(reportStatus.m_statusText.GetLength() == 0)
		{
			m_dg->OnWarning(ErrorInfo::From(ErrorType_Unexpected, HostID_None, _PNT("An Empty String Entered in Agent SendReportStatus.")));
	
			m_proxy.ReportStatusBegin(HostID_Server, RmiContext::ReliableSend, (byte)CReportStatus::StatusType_Warning, _PNT("Please Enter Status Information in reportStatus"));
			m_proxy.ReportStatusEnd(HostID_Server, RmiContext::ReliableSend);
			return false;
		}

		m_proxy.ReportStatusBegin(HostID_Server, RmiContext::ReliableSend, (byte)reportStatus.m_statusType, reportStatus.m_statusText);

		for(CReportStatus::KeyValueList::iterator iter = reportStatus.m_list.begin(); iter != reportStatus.m_list.end() ; ++iter)
		{
			if(iter->GetFirst().GetLength() == 0 || iter->GetSecond().GetLength() == 0 )
			{
				m_dg->OnWarning(ErrorInfo::From(ErrorType_Unexpected, HostID_None, _PNT("An Empty String Entered in Agent SendReportStatus.")));
				m_proxy.ReportStatusValue(HostID_Server, RmiContext::ReliableSend, _PNT("OnWarning"), _PNT("An Empty String Entered in Agent SendReportStatus."));
			}else
			{
				m_proxy.ReportStatusValue(HostID_Server, RmiContext::ReliableSend, iter->GetFirst(), iter->GetSecond());
			}

		}

		m_proxy.ReportStatusEnd(HostID_Server, RmiContext::ReliableSend);
		return true;
	}

	bool CAgentConnectorImpl::EventLog( CReportStatus::StatusType logType, const PNTCHAR* text )
	{
		if(m_client == NULL || text == NULL || Tstrlen(text) == 0)
		{
			return false;
		}

		m_proxy.ReportStatusBegin(HostID_Server, RmiContext::ReliableSend, (byte)logType, text);
		m_proxy.ReportStatusEnd(HostID_Server, RmiContext::ReliableSend);
		return true;
	}

	DEFRMI_AgentS2C_NotifyCredential(CAgentConnectorImpl)
	{
		rmiContext;
		remote;

		if(authentication == true)
			m_dg->OnAuthentication(ErrorInfo::From(ErrorType_Ok));
		else
			m_dg->OnAuthentication(ErrorInfo::From(ErrorType_Unexpected));

		return true;
	}

	DEFRMI_AgentS2C_RequestServerAppStop(CAgentConnectorImpl)
	{
		rmiContext;
		remote;

		NTTNTRACE("RequestServerAppStop\n");
		m_dg->OnStopCommand();
		return true;
	}

	void CAgentConnectorImpl::FrameMove() 
	{
		if(m_client == NULL)
		{
			return ;
		}

		CServerConnectionState scs;
		ConnectionState cs = m_client->GetServerConnectionState(scs);
		if(cs != ConnectionState_Connected)
		{
			if(cs != ConnectionState_Connecting
				&& GetTickCount() - m_lastTryConnectTime > 1000)
			{
				Start();
				m_lastTryConnectTime = CFakeWin32::GetTickCount();
			}
			return;
		}

		if(GetTickCount() - m_lastSendStatusTime > m_delayTimeAboutSendAgentStatus)
		{
			// cpu 타임 알아오기
			float cpuUserTime;
			float cpuKernelTime;
			GetCpuTime(cpuUserTime, cpuKernelTime);
			
			PROCESS_MEMORY_COUNTERS_EX memoryInfo;
			memset(&memoryInfo, 0, sizeof(PROCESS_MEMORY_COUNTERS_EX));
			memoryInfo.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

			GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memoryInfo, sizeof(PROCESS_MEMORY_COUNTERS_EX));
			uint32_t memorySize = (uint32_t)memoryInfo.WorkingSetSize;
			
			m_proxy.ReportServerAppState(HostID_Server, RmiContext::ReliableSend, cpuUserTime, cpuKernelTime, memorySize);

			m_lastSendStatusTime = CFakeWin32::GetTickCount();

// 			String txt;
// 			txt.Format(_PNT("userTime:%f, cpuKernelTime:%f memorySize:%u AppMemorySize:%u"), cpuUserTime, cpuKernelTime, memoryInfo.WorkingSetSize, memoryInfo.PrivateUsage);
// 			OutputDebugStringW(txt);

		}
	}

	void CAgentConnectorImpl::SetDelayTimeAboutSendAgentStatus( uint32_t delay ) 
	{
		m_delayTimeAboutSendAgentStatus = delay;
	}


	void CAgentConnectorImpl::GetCpuTime(float &outUserTime, float &outKernelTime)
	{
		__declspec(align(8)) int64_t currTimeMs = GetPreciseCurrentTimeMs(); // align이 붙은 이유: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284(v=vs.85).aspx
		
		FILETIME creationTime,exitTime,kernelTime,userTime;
		::GetProcessTimes(GetCurrentProcess(),&creationTime,&exitTime,&kernelTime,&userTime);

		__declspec(align(8)) ULONGLONG userTime100Ns = *(ULONGLONG*)&userTime;	// align이 붙은 이유: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284(v=vs.85).aspx
		__declspec(align(8)) ULONGLONG kernelTime100Ns = *(ULONGLONG*)&kernelTime;

		if(m_prevUserTime100Ns == 0)
			m_prevUserTime100Ns = userTime100Ns;
		if(m_prevKernelTime100Ns == 0)
			m_prevKernelTime100Ns = kernelTime100Ns;
		if(m_prevInstrumentedTimeMs==0)
			m_prevInstrumentedTimeMs=currTimeMs;

		ULONGLONG userTimeDiff100Ns = userTime100Ns - m_prevUserTime100Ns;
		ULONGLONG kernelTimeDiff100Ns = kernelTime100Ns - m_prevKernelTime100Ns;
		int64_t timeDiffMs = currTimeMs - m_prevInstrumentedTimeMs;

		m_prevUserTime100Ns = userTime100Ns;
		m_prevKernelTime100Ns = kernelTime100Ns;
		m_prevInstrumentedTimeMs = currTimeMs;

		if(timeDiffMs > 0)
		{
			outUserTime = (float)((double)userTimeDiff100Ns / 100000 / timeDiffMs / GetNoofProcessors());
			outKernelTime = (float)((double)kernelTimeDiff100Ns / 100000 / timeDiffMs / GetNoofProcessors());
		}
		else
		{
			outUserTime = 0;
			outKernelTime = 0;
		}
		
	}

}

