#include "stdafx.h"
#include "RemoteServer.h"
#include "NetClient.h"
#include "RmiContextImpl.h"
#include "SendFragRefs.h"
#include "FallbackParam.h"
#include "CompactFieldMap.h"

namespace Proud
{

	CriticalSection& CRemoteServer_C::GetOwnerCriticalSection()
	{
		return m_owner->GetCriticalSection();
	}

	// 생성자.
	// 생성자 호출 후 TCP socket이 없으면 처리 불가능하므로 에러 처리해야 한다.
	CRemoteServer_C::CRemoteServer_C(CNetClientImpl* owner, shared_ptr<CSuperSocket> toServerTcp)
	{
		m_owner = owner;
		m_toServerUdpSocketCreateHasBeenFailed = false;
		m_owner = owner;
		m_HostID = HostID_Server;
		if (toServerTcp)
		{
			m_ToServerTcp = toServerTcp;
		}
		else
		{
			SuperSocketCreateResult r = CSuperSocket::New(owner, SocketType_Tcp);
			if(r.socket)
				m_ToServerTcp = r.socket;
		}

		m_ToServerUdp_fallbackable = CFallbackableUdpLayerPtr_C(new CFallbackableUdpLayer_C(this));
		m_shutdownIssuedTime = 0;
	}

	// private을 접근하기 위해 이 함수가.
	void CRemoteServer_C::ResetUdpEnable()
	{
		m_ToServerUdp_fallbackable->Set_RealUdpEnabled(false);
	}

	bool CRemoteServer_C::MustDoServerHolepunch()
	{
		if (m_ToServerUdp_fallbackable == nullptr)
			return false;

		// 이미 서버와 실 UDP 통신중이라면 홀펀칭 시도가 무의미하다.
		if (m_ToServerUdp_fallbackable->IsRealUdpEnabled() == true)
			return false;

		/* UDP 서버 주소를 클라에서 인식 못할 경우
		(예: NAT 뒤의 서버가 외부 인터넷에서 인식 가능한 주소가 파라메터로 지정되지 않은 경우)
		홀펀칭을 시도하지 않는다. */
		if(m_ToServerUdp_fallbackable->m_serverAddr.IsUnicastEndpoint() == false)
		{
			return false;
		}

		if (m_ToServerUdp_fallbackable->m_holepunchTimeMs != INT64_MAX)
		{
			if (m_ToServerUdp_fallbackable->m_holepunchTimeMs - GetPreciseCurrentTimeMs() < 0)
			{
				m_ToServerUdp_fallbackable->m_holepunchTimeMs = GetPreciseCurrentTimeMs() + CNetConfig::ServerHolepunchIntervalMs;

				// 서버와의 홀펀칭 횟수는 제한을 둔다. 안 그러면 홀펀칭 안되는 동안 영원히 하더라.
				m_ToServerUdp_fallbackable->m_holepunchTrialCount++;

				if (m_ToServerUdp_fallbackable->m_holepunchTrialCount > CNetConfig::ServerUdpHolepunchMaxTrialCount)
				{
					m_ToServerUdp_fallbackable->m_holepunchTimeMs = INT64_MAX;
				}

				return true;
			}
		}

		return false;
	}

	bool CRemoteServer_C::FallbackServerUdpToTcpOnNeed(int64_t currTime)
	{
		// 너무 오랜 시간동안 서버에 대한 UDP ping이 실패하면 UDP도 TCP fallback mode로 전환한다.
		// [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
		// 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.
		if (m_ToServerUdp_fallbackable->IsRealUdpEnabled() &&
			currTime - m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedTimeMs > CNetConfig::GetFallbackServerUdpToTcpTimeoutMs())
		{
			// check m_lastServerUdpPacketReceivedCount here

			FallbackParam param;
			param.m_reason = ErrorType_ServerUdpFailed;

			return FirstChanceFallbackServerUdpToTcp_WITHOUT_NotifyToServer(param);
		}

		return false;
	}

	// 이미 fallback 상태였으면 false를 리턴.
	// 주의: 서버에 노티는 하지 않는다.
	bool CRemoteServer_C::FirstChanceFallbackServerUdpToTcp_WITHOUT_NotifyToServer(const FallbackParam &param)
	{
		if (m_ToServerUdp_fallbackable->IsRealUdpEnabled() == true)
		{
			// 이것을 먼저 시행할 것!
			// Proud::CRemotePeer_C::FirstChanceChangeToRelay에서 이 메서드를 재귀호출할 터이니.
			m_ToServerUdp_fallbackable->Set_RealUdpEnabled(false);

			// ACR 재접속 등에 의한 것이면
			if(param.m_resetFallbackCount)
				m_ToServerUdp_fallbackable->m_fallbackCount = 0;

			// 로컬 이벤트
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_ServerUdpChanged;
			e.m_errorInfo->m_errorType = ErrorType_ServerUdpFailed;
			e.m_remoteHostID = HostID_Server;
			m_owner->EnqueLocalEvent(e, shared_from_this());

			// 서버와의 UDP 포트 매핑이 증발했지만, 일정 시간 후에 다시 재시도해야 한다.
			// 단, 서버와의 UDP 포트 매핑 재시도는 제한을 두도록 한다. 자세한 것은 'NAT 홀펀칭 관련 노트.docx'.
			bool byebyeForever = false;
			if (m_ToServerUdp_fallbackable->m_fallbackCount < CNetConfig::ServerUdpRepunchMaxTrialCount)
			{
				m_ToServerUdp_fallbackable->m_holepunchTimeMs = GetPreciseCurrentTimeMs() + CNetConfig::ServerUdpRepunchIntervalMs;
				m_ToServerUdp_fallbackable->m_fallbackCount++;

				m_ToServerUdp_fallbackable->m_holepunchTrialCount = 0;
			}
			else
			{
				// 횟수를 초과했다. 영원히 서버와의 UDP는 즐이다.
				m_ToServerUdp_fallbackable->m_holepunchTimeMs = INT64_MAX;
				byebyeForever = true;
			}

#ifndef _DEBUG // 디버그 빌드에서는 의레 있는 일이므로



			//MessageBox(nullptr,txt,L"AA",MB_OK);//TEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMP

			//	CErrorReporter_Indeed::Report(txt);  // TEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMP

#endif
//			// 서버에 TCP fallback을 해야 함을 노티.
//			m_c2sProxy.NotifyUdpToTcpFallbackByClient(HostID_Server, g_ReliableSendForPN);
//			/* 클라에서 to-server-UDP가 증발해도 per-peer UDP는 증발하지 않는다. 아예 internal port 자체가 다르니까.
//			따라서 to-peer UDP는 그대로 둔다. */

			return true;
		}

		return false;
	}

	// 서버로부터 UDP 데이터를 받은 시간을 최신으로 갱신한다.
	void CRemoteServer_C::UpdateServerUdpReceivedTime()
	{
		if (m_ToServerUdp_fallbackable != nullptr)
		{
			int64_t currTime = GetPreciseCurrentTimeMs();

			// pong 체크를 했다고 처리하도록 하자.
			// 이게 없으면 대량 통신시 pong 수신 지연으로 인한 튕김이 발생하니까.
			int64_t oktime = currTime - m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedTimeMs;

			if (oktime > 0)
				m_ToServerUdp_fallbackable->m_lastUdpPacketReceivedIntervalMs = oktime;

			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedTimeMs = currTime;
			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedCount++;

#ifdef UDP_PACKET_RECEIVE_LOG
			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedQueue.Add(m_lastServerUdpPacketReceivedTime);
#endif // UDP_PACKET_RECEIVE_LOG
		}
	}

	void CRemoteServer_C::Send_ToServer_Directly_Copy(
		HostID destHostID, MessageReliability reliability, const CSendFragRefs &sendData2, const SendOpt& sendOpt, bool simplePacketMode)
	{
		// send_core to server via UDP or TCP
		if (reliability == MessageReliability_Reliable)
		{
			m_ToServerTcp->AddToSendQueueWithSplitterAndSignal_Copy(
				m_ToServerTcp,
				sendData2, sendOpt, simplePacketMode);
		}
		else
		{
			// 클라-서버간 UDP 소켓을 JIT로 생성한다.
			// 서버에게 딱 1번, UDP 소켓 생성을 요청한다.
			RequestServerUdpSocketReady_FirstTimeOnly();

			// unrealiable이 되기전까지 TCP로 통신
			// unreliable일때만 uniqueid를 사용하므로...
			m_ToServerUdp_fallbackable->SendWithSplitterViaUdpOrTcp_Copy(
				destHostID, sendData2, sendOpt);
		}
	}

	// 서버에게 "UDP 소켓을 만들어라"를 요청한다.
	// 클라-서버간 UDP 소켓은 JIT로 만들어진다.
	void CRemoteServer_C::RequestServerUdpSocketReady_FirstTimeOnly()
	{
		if (m_ToServerUdp == nullptr && m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting == false &&
			m_owner->m_settings.m_fallbackMethod <= FallbackMethod_PeersUdpToTcp && m_toServerUdpSocketCreateHasBeenFailed == false)
		{
			m_owner->m_c2sProxy.C2S_RequestCreateUdpSocket(HostID_Server, g_ReliableSendForPN,
				CompactFieldMap());
			m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting = true;
		}
	}


	Proud::AddrPort CRemoteServer_C::Get_ToServerUdpSocketLocalAddr()
	{
		if (m_ToServerUdp == nullptr)
			return AddrPort::Unassigned;

		return m_ToServerUdp->GetLocalAddr();
	}

	bool CRemoteServer_C::IsRealUdpEnable()
	{
		return (m_ToServerUdp_fallbackable != nullptr) ? m_ToServerUdp_fallbackable->IsRealUdpEnabled() : false;
	}

	void CRemoteServer_C::SetToServerUdpFallbackable(AddrPort addrPort)
	{
		m_ToServerUdp_fallbackable->m_serverAddr = addrPort;

		assert(m_ToServerUdp_fallbackable->m_serverAddr.IsAnyOrUnicastEndpoint());
	}

	CRemoteServer_C::~CRemoteServer_C()
	{
		// 여기서 owner를 접근하지 말 것.
		// PopUserTask 후 RAII로 인해 no main lock 상태에서 여기가 호출될 수 있다.
		// 정리해야 할게 있으면 여기가 아니라 OnHostGarbageCollected or OnHostGarbaged에 구현할 것.

		// 여기 왔을 때는 소유하고 있는 SuperSocket ptr 멤버들이 이미 위 함수에 의해 null로 세팅되어 있어야 한다.
		assert(m_ToServerTcp == nullptr);
		assert(m_ToServerUdp_fallbackable == nullptr);
	}

	CRemoteServer_C::CFallbackableUdpLayer_C::CFallbackableUdpLayer_C(CRemoteServer_C* owner)
	{
		m_realUdpEnabledTimeMs = 0;
		m_serverUdpReadyWaiting = false;
		m_serverAddr = AddrPort::Unassigned;
		m_owner = owner;
		m_holepunchTimeMs = INT64_MAX;
		m_holepunchTrialCount = 0;
		m_fallbackCount = 0;
		m_realUdpEnabled_USE_FUNCTION = false;
		m_realUdpEnabledTimeMs = 0;
		m_lastServerUdpPacketReceivedCount = 0;
		m_lastUdpPacketReceivedIntervalMs = -1000;
		m_lastServerUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs();
	}

	// 서버를 향한 메시지 송신.
	// udp 연결이 살아있으면 udp로, 그렇지 않으면 tcp fallback으로 전송을 수행.
	// hostID: P2P relay 전송인 경우에, 그 hostID이다.
	void CRemoteServer_C::CFallbackableUdpLayer_C::SendWithSplitterViaUdpOrTcp_Copy(
		HostID hostID, const CSendFragRefs& sendData, const SendOpt &sendOpt)
	{
		m_owner->m_owner->LockMain_AssertIsLockedByCurrentThread();

		if (IsRealUdpEnabled() && !m_owner->m_ToServerUdp->StopIoRequested())
		{
			// Server에게 보내는 udp send count를 한다.
			if (sendOpt.m_INTERNAL_USE_isProudNetSpecificRmi == false)
				m_owner->m_owner->m_toServerUdpSendCount++;

			// UDP 송신 큐에 넣는다.
			// unreliable을 UDP로 보내는 경우 MTU size에 제한해서 split하는 과정을
			// UDP socket 객체 내 frag board를 통해 시행한다. PMTU discovery 과정에서 발생하는 다수 ICMP 패킷을
			// 해커 공격으로 오인해서 차단하는 몇몇 네트웍 환경을 극복하기 위해.
			m_owner->m_ToServerUdp->AddToSendQueueWithSplitterAndSignal_Copy(
				m_owner->m_ToServerUdp,
				hostID,
				FilterTag::CreateFilterTag(m_owner->m_owner->GetVolatileLocalHostID(), HostID_Server),
				m_serverAddr, sendData, GetPreciseCurrentTimeMs(), sendOpt);
		}
		else
		{
			// fallback mode
			m_owner->m_ToServerTcp->AddToSendQueueWithSplitterAndSignal_Copy(
				m_owner->m_ToServerTcp,
				sendData, sendOpt, m_owner->m_owner->m_simplePacketMode);
		}
	}

	// RealUdpEnabled 값을 세팅한다. true로 바뀌는 경우 부차 변수 변경도 한다.
	void CRemoteServer_C::CFallbackableUdpLayer_C::Set_RealUdpEnabled(bool flag)
	{
		if (m_realUdpEnabled_USE_FUNCTION == flag)
			return;

		m_realUdpEnabled_USE_FUNCTION = flag;

		// flag=F인데 이 값을 세팅하면 곤란. 세팅할 경우 클라-서버 연결 끊어짐을 검사할 때 이 시간 이후부터 set default timeout만큼 기다려 버리기 때문이다.
		if (flag)
			m_lastServerUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs();

		m_lastServerUdpPacketReceivedCount = 0;
		m_lastUdpPacketReceivedIntervalMs = -1;

#ifdef UDP_PACKET_RECEIVE_LOG
		m_lastServerUdpPacketReceivedQueue.Clear();
#endif // UDP_PACKET_RECEIVE_LOG

		if (flag)
			m_realUdpEnabledTimeMs = GetPreciseCurrentTimeMs();
	}
}
