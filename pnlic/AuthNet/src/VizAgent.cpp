#include "stdafx.h"
#include "VizAgent.h"
#include "IVizAgentDg.h"

#ifdef VIZAGENT
#include "Viz_common.cpp"
#include "Viz_proxy.cpp"
#include "Viz_stub.cpp"
#endif

#include "RemotePeer.h"
#include "RemoteClient.h"
#include "NetClient.h"
#include "NetServer.h"
#include "RmiContextImpl.h"

namespace Proud 
{
#ifdef VIZAGENT
	static PNGUID v = { 0x9b5e4767, 0x3b4f, 0x4f74, { 0x9b, 0xb, 0xfe, 0x23, 0xfa, 0x67, 0x61, 0xdf } };
	Guid CVizAgent::m_protocolVersion = v;

	CVizAgent::CVizAgent(IVizAgentDg* dg, const PNTCHAR* serverAddr, int serverPort, const String &loginKey)
	{
		m_dg = dg;
		m_serverAddr = serverAddr;
		m_serverPort = serverPort;
		m_loginKey = loginKey;

		m_netClient.Attach(new CNetClientImpl());
		m_netClient->SetEventSink(this);
		m_netClient->AttachProxy(&m_c2sProxy);
		m_netClient->AttachStub(this);

		m_tryConnectBeginTime = 0;
		m_state = Disconnected;

		// 이게 반드시 있어야 됨! 안그러면 내부 RMI 추적까지 나와버림!
		m_c2sProxy.m_internalUse = true;
		IRmiStub::m_internalUse = true;

		CHeartbeatWorkThread::Instance().Register(this, 10);
	}

	CVizAgent::~CVizAgent(void)
	{
		CHeartbeatWorkThread::Instance().Unregister(this);
		m_netClient.Free();
	}

	// 이 메서드는 일정 시간마다 반드시 호출되어야 한다. 그것도 owner의 networker thread에서!
	void CVizAgent::Heartbeat()
	{
		m_netClient->FrameMove();

		CriticalSectionLock lock(m_cs,true);

		switch(m_state)
		{
		case Disconnected:
			Heartbeat_DisconnectedCase();
			break;
		case Connecting:
			Heartbeat_ConnectingCase();
			break;
		case Connected:
			Heartbeat_ConnectedCase();
			break;
		}
	}

	void CVizAgent::Heartbeat_DisconnectedCase()
	{

		// 시간이 되면 서버로의 연결을 시도한다.
		if(GetPreciseCurrentTimeMs() > m_tryConnectBeginTime)
		{
			CNetConnectionParam sp;
			sp.m_serverIP = m_serverAddr;
			sp.m_serverPort = m_serverPort;
			sp.m_protocolVersion = m_protocolVersion;

			m_netClient->Connect(sp);

			m_tryConnectBeginTime = CNetConfig::VizReconnectTryIntervalMs + GetPreciseCurrentTimeMs();
			m_state = Connecting;
		}

	}

	void CVizAgent::Heartbeat_ConnectingCase()
	{
		
	}

	void CVizAgent::Heartbeat_ConnectedCase()
	{

	}

	void CVizAgent::OnJoinServerComplete( ErrorInfo *info, const ByteArray &replyFromServer ) 
	{
		CriticalSectionLock lock(m_cs,true);
		
		// 서버 연결 결과에 따라 상태 천이
		if(info->m_errorType == ErrorType_Ok)
		{
			m_state = Connected;

			// 인증
			m_c2sProxy.RequestLogin(HostID_Server,g_SecureReliableSendForPN,m_loginKey, GetOwnerHostID());
		}
		else
		{
			m_state = Disconnected;
		}
	}

	void CVizAgent::OnLeaveServer( ErrorInfo *errorInfo ) 
	{
		CriticalSectionLock lock(m_cs,true);
		
		m_state = Disconnected;
	}

	DEFRMI_VizS2C_NotifyLoginOk(CVizAgent)
	{
		CriticalSectionLock lock(m_cs,true);

		// 자기 상태 보내기
		NotifyInitState();

		return true;
	}

	DEFRMI_VizS2C_NotifyLoginFailed(CVizAgent)
	{
		CriticalSectionLock lock(m_cs,true);

		// 서버와의 연결을 끊는다. 그리고 한참 뒤에 다시 재시작.
		m_netClient->Disconnect();
		m_state	= Disconnected;
		m_tryConnectBeginTime = CNetConfig::VizReconnectTryIntervalMs + GetPreciseCurrentTimeMs();

		return true;
	}

	void CVizAgent::NotifyInitState()
	{
		CriticalSectionLock lock(m_cs,true);

		if(CNetClientImpl* cli = m_dg->QueryNetClient())
		{
			// 서버로의 연결 진행 상황
			CServerConnectionState ignore;
			m_c2sProxy.NotifyCli_ConnectionState(HostID_Server, g_ReliableSendForPN, cli->GetServerConnectionState(ignore));

			// P2P 연결 갯수
			m_c2sProxy.NotifyCli_Peers_Clear(HostID_Server, g_ReliableSendForPN);
			for(RemotePeers_C::iterator i=cli->m_remotePeers.begin();i!=cli->m_remotePeers.end();i++)
			{
				CRemotePeer_C* rp = i->second;
				m_c2sProxy.NotifyCli_Peers_AddOrEdit(HostID_Server, g_ReliableSendForPN, rp->m_HostID);
			}
		}
		else if(CNetServerImpl* srv = m_dg->QueryNetServer())
		{
			m_c2sProxy.NotifySrv_ClientEmpty(HostID_Server, g_ReliableSendForPN);
			for(RemoteClients_S::iterator i=srv->m_authedHostMap.begin();i!=srv->m_authedHostMap.end();i++)
			{
				CRemoteClient_S* rc = i->GetSecond();
				m_c2sProxy.NotifySrv_Clients_AddOrEdit(HostID_Server, g_ReliableSendForPN, rc->m_HostID);
			}
		}
	}

	Proud::HostID CVizAgent::GetOwnerHostID()
	{
		if(m_dg->QueryNetClient())
			return m_dg->QueryNetClient()->m_localHostID;
		else if(m_dg->QueryNetServer())
			return HostID_Server;

		assert(0);
		return HostID_None;
	}
#endif
}
