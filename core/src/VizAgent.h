#pragma once

#include "../include/ProudNetClient.h"
#include "enumimpl.h"
#ifdef VIZAGENT
#include "VizMessageSummary.h"
#include "Viz_proxy.h"
#include "Viz_stub.h"
#endif
#include "HeartbeatWork.h"

namespace Proud
{
#ifdef VIZAGENT
	class IVizAgentDg;
	class CNetClientImpl;

	class CVizAgent :public INetClientEvent, public VizS2C::Stub, public IHeartbeatWork
	{
		IVizAgentDg* m_dg;
		CHeldPtr<CNetClientImpl> m_netClient;

		String m_serverAddr;
		int m_serverPort;
		String m_loginKey;
	public:
		VizC2S::Proxy m_c2sProxy;
	private:
		enum State
		{
			Disconnected, Connecting, Connected
		};
		State m_state;
		int64_t m_tryConnectBeginTime;
	public:
		// **주의** vizagent를 가진 객체는 자기 상태를 viz에 노티하는 RMI를 콜 하기 전에 반드시 m_cs를 걸고 움직여야 한다! 안그러면 보내는 순서가 꼬여서 받는 쪽에서 상태가 교란될 수 있다.
		CriticalSection m_cs;
	private:
		// 내부 이벤트 콜백
		virtual void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer) PN_OVERRIDE;
		virtual void OnLeaveServer(ErrorInfo *errorInfo) PN_OVERRIDE;
		virtual void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &message) PN_OVERRIDE{}
		virtual void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount) PN_OVERRIDE{}
		virtual void OnError(ErrorInfo *errorInfo) PN_OVERRIDE{}
		virtual void OnWarning(ErrorInfo *errorInfo) PN_OVERRIDE{}
		virtual void OnInformation(ErrorInfo *errorInfo) PN_OVERRIDE{}
		virtual void OnException(const Exception &e) PN_OVERRIDE{}
		virtual void OnNoRmiProcessed(Proud::RmiID rmiID) PN_OVERRIDE{}

		virtual void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) PN_OVERRIDE{}
		virtual void OnSynchronizeServerTime() PN_OVERRIDE{}
	public:
		static Guid m_protocolVersion;

		CVizAgent(IVizAgentDg* dg, const PNTCHAR* serverAddr, int serverPort, const String &loginKey);
		~CVizAgent(void);

		void Heartbeat();
	private:
		void Heartbeat_DisconnectedCase();
		void Heartbeat_ConnectingCase();
		void Heartbeat_ConnectedCase();

		void NotifyInitState();
		HostID GetOwnerHostID();

		DECRMI_VizS2C_NotifyLoginOk;
		DECRMI_VizS2C_NotifyLoginFailed;
	};
#endif
}

PROUDNET_SERIALIZE_ENUM(Proud::ConnectionState)