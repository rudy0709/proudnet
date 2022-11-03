#pragma once

#include "../include/EmergencyLogCommon.h"
#include "../include/EmergencyLogServer.h"
#include "../include/ProudNetServer.h"
#include "EmergencyC2S_stub.h"
#include "EmergencyLogData.h"
#include "EmergencyS2C_proxy.h"
#include "FastMap2.h"
#include <fstream>

#ifdef USE_EMERGENCY_LOG

//////////////////////////////////////////////////////////////////////////
// Emergency Log 모듈
// 비록 본 모듈은 사용되고 있지 않지만, 추후 다시 필요할 경우를 대비해서, 코드는 남겨놓는다.

namespace Proud
{
	class CEmergencyLogServerImpl;

	class CEmergencyCli_S
	{
		CEmergencyLogServerImpl* m_server;
		HostID m_hostID;
	public:
		CEmergencyLogData m_logData;
		CEmergencyLogServerImpl* GetServer() { return m_server; }

		CEmergencyCli_S(CEmergencyLogServerImpl* server, HostID hostID);
		~CEmergencyCli_S(void);
	};

	typedef RefCount<CEmergencyCli_S> CEmergencyPtr_S;
	typedef CFastMap2<HostID, CEmergencyPtr_S,int> CEmergencyClients;

	class CEmergencyLogServerImpl :
		public CEmergencyLogServer,
		public EmergencyC2S::Stub,
		public INetServerEvent
	{
		EmergencyS2C::Proxy m_s2cProxy;

		//float m_elapsedTime;
		IEmergencyLogServerDelegate* m_dg;

		//CHeldPtr<CMilisecTimer> m_timer;
		CHeldPtr<CNetServer> m_server;
		CEmergencyClients m_clients;

#ifdef _PNUNICODE
		// win32, UE4 등은 이것을 쓰니까.
		wofstream m_logFile;
#else
		ofstream m_logFile;
#endif

		int m_logFileCount;

		void FrameMove();
		//float GetElapsedTime();
		CEmergencyPtr_S GetClientByHostID(HostID hostid);
		bool CreateLogFile();
		void WriteClientLog(HostID hostid, const CEmergencyLogData& data);

		void OnClientJoin(CNetClientInfo *clientInfo);
		void OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorInfo, const ByteArray& comment);

		virtual void OnError(ErrorInfo *errorInfo) {}
		virtual void OnWarning(ErrorInfo *errorInfo) {}
		virtual void OnInformation(ErrorInfo *errorInfo) {}
		virtual void OnException(Exception &e) {}
		virtual void OnUnhandledException() {}
		virtual void OnNoRmiProcessed(Proud::RmiID rmiID) {}

		virtual bool OnConnectionRequest(AddrPort clientAddr, ByteArray &userDataFromClient, ByteArray &reply) {
			return true;
		}
		virtual void OnP2PGroupJoinMemberAckComplete(HostID groupHostID,HostID memberHostID,ErrorType result) {}
		virtual void OnUserWorkerThreadBegin() PN_OVERRIDE {}
		virtual void OnUserWorkerThreadEnd() PN_OVERRIDE {}

		DECRMI_EmergencyC2S_EmergencyLogData_Begin;
		DECRMI_EmergencyC2S_EmergencyLogData_Error;
		DECRMI_EmergencyC2S_EmergencyLogData_Stats;
		DECRMI_EmergencyC2S_EmergencyLogData_OSVersion;
		DECRMI_EmergencyC2S_EmergencyLogData_LogEvent;
		DECRMI_EmergencyC2S_EmergencyLogData_End;

	public:
		CEmergencyLogServerImpl(IEmergencyLogServerDelegate* dg);
		~CEmergencyLogServerImpl(void);

		void RunMainLoop();
	};
}

#endif