#pragma once

#ifdef _WIN32

#include "../include/NetClientInterface.h"
#include "../include/EmergencyLogCommon.h"
#include "EmergencyLogData.h"
#include "EmergencyC2S_proxy.h"
#include "EmergencyS2C_stub.h"

namespace Proud
{

	// Net 또는 Lan클라이언트가 의도치않게 종료되었을때의 Log를 보내기 위한 client
	class CEmergencyLogClient : 
		public EmergencyS2C::Stub,
		public INetClientEvent
	{
		CHeldPtr<CNetClient> m_client;
	
		EmergencyC2S::Proxy m_c2sProxy;
		CEmergencyLogData* m_logData;
	private:
		virtual void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer);
		virtual void OnLeaveServer(ErrorInfo *errorInfo);
		virtual void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField);
		virtual void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount);
		virtual void OnError(ErrorInfo *errorInfo);
		virtual void OnWarning(ErrorInfo *errorInfo) { errorInfo; }
		virtual void OnInformation(ErrorInfo *errorInfo) { errorInfo; }
		virtual void OnException(Exception &e);
		virtual void OnUnhandledException() {}
		virtual void OnNoRmiProcessed(Proud::RmiID rmiID) { rmiID; }

		virtual void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) { remoteHostID; reason; }
		virtual void OnSynchronizeServerTime() {}

		DECRMI_EmergencyS2C_EmergencyLogData_AckComplete;
	public:
		enum State { Connecting, Sending, Closing, Stopped };
	private:
		State m_state;
	public:
		CEmergencyLogClient(void);
		~CEmergencyLogClient(void);

		State GetState();
		void Start(String serverAddr, uint16_t serverPort, CEmergencyLogData* logData);
		void FrameMove();
	};

}

#endif // _WIN32