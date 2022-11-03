#pragma once


#include "../include/AgentCommon.h"
#include "AgentC2S_proxy.h"
#include "AgentS2C_stub.h"

namespace Proud
{
	class CAgentConnectorImpl : 
		public CAgentConnector,
		public INetClientEvent,
		public AgentS2C::Stub
	{
		AgentC2S::Proxy m_proxy;
		CHeldPtr<CNetClient> m_client;
		IAgentConnectorDelegate* m_dg;

		int m_cookie;	// Agent에게 발급받은 cookie(Agent내에서 serverapp을 식별하기 위한값)

		// 상태 정보를 보내는 delay
		uint32_t m_delayTimeAboutSendAgentStatus;
		uint32_t m_lastSendStatusTime;

		uint32_t m_lastTryConnectTime;

		// 커널 정보에 대한 필요한 함수
		ULONGLONG m_prevKernelTime100Ns,m_prevUserTime100Ns;
		int64_t m_prevInstrumentedTimeMs;


		virtual void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer) PN_OVERRIDE;
		virtual void OnLeaveServer(ErrorInfo *errorInfo) PN_OVERRIDE;
		
		virtual void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField)  PN_OVERRIDE {}
		virtual void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount)  PN_OVERRIDE {}
		
		virtual void OnSynchronizeServerTime()  PN_OVERRIDE {}
		virtual void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) PN_OVERRIDE{}
		virtual void OnError(ErrorInfo *errorInfo) PN_OVERRIDE {}
		virtual void OnWarning(ErrorInfo *errorInfo) PN_OVERRIDE {}
		virtual void OnInformation(ErrorInfo *errorInfo) PN_OVERRIDE {}
		virtual void OnException(Exception &e) PN_OVERRIDE {}
		virtual void OnNoRmiProcessed(RmiID rmiID) PN_OVERRIDE {}

		virtual bool Start() PN_OVERRIDE;
		virtual bool SendReportStatus(CReportStatus& reportStatus) PN_OVERRIDE;
		virtual bool EventLog(CReportStatus::StatusType logType, const PNTCHAR* text) PN_OVERRIDE;

		virtual void FrameMove() PN_OVERRIDE;
		virtual void SetDelayTimeAboutSendAgentStatus(uint32_t delay) PN_OVERRIDE;

	public:
		CAgentConnectorImpl(IAgentConnectorDelegate* dg);
		~CAgentConnectorImpl();

	public:
		void GetCpuTime(float &outUserTime, float &outKernelTime);

		DECRMI_AgentS2C_NotifyCredential;
		DECRMI_AgentS2C_RequestServerAppStop;
	};
}
