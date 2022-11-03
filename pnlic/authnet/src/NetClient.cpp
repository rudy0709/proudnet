/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"

#if defined(_WIN32) 
#include <Lm.h>
#pragma comment(lib, "netapi32.lib")
#endif

// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <new>
#include <stack>
#include <stdexcept>
#ifdef _WIN32
#include <time.h>
#endif // _WIN32

#include "../include/INetClientEvent.h"
#include "../include/Marshaler.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/FakeClrBase.h"
#include "../include/sysutil.h"
#include "../include/atomic.h"

#include "NetClient.h"

#include "ThreadPoolImpl.h"
#include "Relayer.h"
#include "ReliableUDPFrame.h"
#include "STLUtil.h"
#include "CollUtil.h"
#include "SocketUtil.h"
#include "RmiContextImpl.h"
#include "LoopbackHost.h"

#include "EmergencyLogClient.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif
//#include "Upnp.h"
#include "ReusableLocalVar.h"
#include <typeinfo>

#ifndef _DEBUG
#define HIDE_RMI_NAME_STRING
#endif

#include "NetS2C_stub.cpp"
#include "NetC2C_stub.cpp"
#include "NetC2S_proxy.cpp"
#include "NetC2C_proxy.cpp"
#include "LeanDynamicCast.h"
#include "SendFragRefs.h"
#include "../include/ClassBehavior.h"

namespace Proud
{
	void AssureIPAddressIsUnicastEndpoint(const AddrPort &udpLocalAddr)
	{
		if (udpLocalAddr.m_binaryAddress == 0 || udpLocalAddr.m_binaryAddress == 0xffffffff)
		{
#if defined(_WIN32)
			String text;
			text.Format(_PNT("FATAL: It should already have been TCP-Connected prior to the Creation of the UDP Socket but localAddr of TCP Appeared to be %s!"), udpLocalAddr.ToString());
			CErrorReporter_Indeed::Report(text);
#endif
			assert(0);
		}

		/*Q: throw 하지 않고 assert를 하는 이유는?
		A: 조건 불충분이라고 해봤자 홀펀칭이 안될 뿐입니다. 그런데 여기서 throw를 하면 클라는 뻗어버립니다.
		NetClient가 UDP socket의 bind address가 TCP의 address이어야 하는데 그거 대신 any를 넣는 실수를 막기 위해서 이것이 존재합니다.
		*/
	}

	//////////////////////////////////////////////////////////////////////////
	const PNTCHAR* NoServerConnectionErrorText = _PNT("Cannot send messages unless connection to server exists!");
	DEFRMI_ProudS2C_P2PGroup_MemberJoin(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// p2p group에 새로운 member가 들어온것을 update한다
		m_owner->UpdateP2PGroup_MemberJoin(groupHostID,
			memberHostID,
			customField,
			eventID,
			p2pFirstFrameNumber,
			connectionMagicNumber,
			p2pAESSessionKey,
			p2pFastSessionKey,
			allowDirectP2P,
			pairRecycled);

		return true;
	}

	DEFRMI_ProudS2C_RequestP2PHolepunch(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		CRemotePeer_C *rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerID);
		if (rp == NULL || rp->m_garbaged)
			return true;

		// P2P 홀펀칭 시도 자체가 되고 있지 않은 상태이면 즐
		if (!rp->m_p2pConnectionTrialContext)
			return true;

		// 상대측 피어의 주소를 넣는다.
		rp->m_UdpAddrFromServer = externalAddr;
		rp->m_UdpAddrInternal = internalAddr;

		assert(rp->m_UdpAddrFromServer.IsUnicastEndpoint());

		// 쌍방 홀펀칭 천이를 시작한다.
		if (rp->m_p2pConnectionTrialContext->m_state == NULL ||
			rp->m_p2pConnectionTrialContext->m_state->m_state != CP2PConnectionTrialContext::S_PeerHolepunch)
		{
			rp->m_p2pConnectionTrialContext->m_state.Free();

			CP2PConnectionTrialContext::PeerHolepunchState* newState = new CP2PConnectionTrialContext::PeerHolepunchState;
			newState->m_shotgunMinPortNum = externalAddr.m_port;

			rp->m_p2pConnectionTrialContext->m_state.Attach(newState);
		}

		return true;
	}

	DEFRMI_ProudS2C_NotifyDirectP2PEstablish(CNetClientImpl::S2CStub)
	{
		HostID A = aPeer, B = bPeer;
		AddrPort ABSendAddr = X0, ABRecvAddr = Y0, BASendAddr = Z0, BARecvAddr = W0;

		/* establish virtual UDP connection between A and B
		*
		* status;
		A can send to B via X
		B can send to A via Z
		A can choose B's message if it is received from W
		B can choose A's messages if it is received from Y

		* swap A-B / X-Z / W-Y if local is B */

		{
			CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
			assert(m_owner->GetVolatileLocalHostID() == A || m_owner->GetVolatileLocalHostID() == B);
			if (m_owner->GetVolatileLocalHostID() == B)
			{
#if !defined(_WIN32)
				quickswap(A, B);
				quickswap(ABSendAddr, BASendAddr);
				quickswap(BARecvAddr, ABRecvAddr);
#else
				swap(A, B);
				swap(ABSendAddr, BASendAddr);
				swap(BARecvAddr, ABRecvAddr);
#endif
			}

			CRemotePeer_C *rp = m_owner->GetPeerByHostID_NOLOCK(B);
			if (rp == NULL || rp->m_garbaged || !rp->m_udpSocket)
				return true;
			rp->m_P2PHolepunchedLocalToRemoteAddr = ABSendAddr;
			rp->m_P2PHolepunchedRemoteToLocalAddr = BARecvAddr;

			rp->SetRelayedP2P(false);
			//			rp->BackupDirectP2PInfo();

			// 연결 시도중이던 객체를 파괴한다.
			// (이미 이쪽에서 저쪽으로 연결 시도가 성공해서 이미 trial context를 지운 상태일 수 있지만 무관함)
			rp->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

			// enqueue event
			LocalEvent e;
			e.m_type = LocalEventType_DirectP2PEnabled;
			e.m_remoteHostID = B;
			//m_owner->EnqueLocalEvent(e, m_owner->m_loopbackHost);
			m_owner->EnqueLocalEvent(e, rp);
		}

		return true;
	}

	
	DEFRMI_ProudS2C_P2P_NotifyDirectP2PDisconnected2(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 이 RMI는 상대방과 P2P 그룹 연계가 있음에도 불구하고 P2P 연결이 끊어질 경우에도 도착한다.
		// 따라서 P2P relay mode로 전환해야 한다.
		CRemotePeer_C *rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerHostID);
		if (rp != NULL && !rp->m_garbaged)
		{
			if (!rp->IsRelayedP2P())
			{
				rp->FallbackP2PToRelay(false, reason);
			}
		}

		return true;
	}

	// pidl에 설명 있음
	DEFRMI_ProudS2C_S2C_RequestCreateUdpSocket(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// to-peer UDP Socket 생성
		m_owner->New_ToServerUdpSocket();
		bool success = m_owner->m_remoteServer->m_ToServerUdp ? true : false;

		if (success)
		{
			// Upnp 기능을 켠다. connect에서 다시 하도록 옮김 //( Connect에서 하던걸 여기로 옮김 )
			//m_owner->StartupUpnpOnNeed();

			// 홀펀칭 시도
			AddrPort udpServerIP(Proud::DnsForwardLookup(serverUdpAddr.m_addr), serverUdpAddr.m_port);
			m_owner->m_remoteServer->SetToServerUdpFallbackable(udpServerIP);
		}

		// 서버에게, UDP 소켓을 열었다는 확인 메세지를 보낸다.
		m_owner->m_c2sProxy.C2S_CreateUdpSocketAck(HostID_Server, g_ReliableSendForPN, success);

		return true;
	}

	DEFRMI_ProudS2C_S2C_CreateUdpSocketAck(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		if (succeed)
		{
			// UDP Socket 생성
			m_owner->New_ToServerUdpSocket();
			if (m_owner->m_remoteServer->m_ToServerUdp)
			{
				// Upnp 기능을 켠다. connect에서 다시 하도록 옮김 //( Connect에서 하던걸 여기로 옮김 )
				//m_owner->StartupUpnpOnNeed();

				// 홀펀칭 시도
				AddrPort udpServerIP(Proud::DnsForwardLookup(serverudpaddr.m_addr), serverudpaddr.m_port);
				m_owner->m_remoteServer->SetToServerUdpFallbackable(udpServerIP);
			}
		}

		// Request 초기화
		m_owner->m_remoteServer->SetServerUdpReadyWaiting(false);

		return true;
	}

	DEFRMI_ProudS2C_ReliablePong(CNetClientImpl::S2CStub)
	{
		if (m_owner->m_enableAutoConnectionRecovery == true && messageID != 0)
		{
			m_owner->m_remoteServer->m_ToServerTcp->AcrMessageRecovery_RemoveBeforeAckedMessageID(messageID);
		}

		// 보낼때 기재했던 ping값과의 시간값 차이를 통해 RTT를 구한다.
		int clientTime = (int)GetPreciseCurrentTimeMs();
		int newLag = (clientTime - localTimeMs) / 2;

		m_owner->ServerTcpPing_UpdateValues(newLag);

		return true;
	}

	// 상대와의 P2P 홀펀칭 시도하던 것들을 모두 중지시킨다.
	// 이미 한 경로가 성공했기 때문이다. hole punch race condition이 발생하면 안되니까.
	DEFRMI_ProudC2C_HolsterP2PHolepunchTrial(CNetClientImpl::C2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		CRemotePeer_C *rp = m_owner->GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL && !rp->m_garbaged)
		{
			rp->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
		}

		return true;
	}

	DEFRMI_ProudC2C_ReportUdpMessageCount(CNetClientImpl::C2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 상대방으로부터 상대방이 보낸갯수와 받은갯수를 업데이트한다.
		CRemotePeer_C* rp = m_owner->GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL && !rp->m_garbaged)
		{
			// 갯수 업데이트     
			rp->m_toRemotePeerSendUdpMessageSuccessCount = udpSuccessCount;
			m_owner->m_c2sProxy.ReportC2CUdpMessageCount(HostID_Server, g_ReliableSendForPN,
				rp->m_HostID, rp->m_toRemotePeerSendUdpMessageTrialCount, rp->m_toRemotePeerSendUdpMessageSuccessCount);
		}

		return true;
	}

	// 	DEFRMI_ProudC2C_ReportServerTimeAndFrameRateAndPing(CNetClientImpl::C2CStub)
	// 	{
	// 		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
	//
	// 		CRemotePeer_C* rp = m_owner->GetPeerByHostID(remote);
	// 		if(rp != NULL && !rp->m_garbaged)
	// 		{
	// 			rp->m_recentFrameRate = recentFrameRate;
	//
	// 			int CSLossPercent = 0;
	// 			m_owner->GetUnreliableMessagingLossRatioPercent(HostID_Server, &CSLossPercent);
	//
	// 			CApplicationHint hint;
	// 			m_owner->GetApplicationHint(hint);
	// 			m_owner->m_c2cProxy.ReportServerTimeAndFrameRateAndPong(remote, g_ReliableSendForPN,
	// 				clientLocalTime, m_owner->GetServerTimeMs(), m_owner->m_serverUdpRecentPingMs, hint.m_recentFrameRate, CSLossPercent);
	// 		}
	//
	// 		return true;
	// 	}
	//
	// 	DEFRMI_ProudC2C_ReportServerTimeAndFrameRateAndPong(CNetClientImpl::C2CStub)
	// 	{
	// 		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
	//
	// 		CRemotePeer_C* rp = m_owner->GetPeerByHostID(remote);
	// 		if(rp != NULL && !rp->m_garbaged)
	// 		{
	// 			// 각 값들을 갱신한다.
	// 			int peerToServerUdpPing = max(serverUdpRecentPing, 0);
	// 			rp->m_peerToServerPingMs = peerToServerUdpPing;
	// 			rp->m_recentFrameRate = recentFrameRate;
	// 			rp->m_CSPacketLossPercent = CSPacketLossPercent;
	//
	// 			int64_t currTime = GetPreciseCurrentTimeMs();
	// 			int64_t serverTime = serverLocalTime + rp->m_recentPingMs;
	//
	// 			rp->m_indirectServerTimeDiff = currTime - serverTime;
	// 		}
	//
	// 		return true;
	// 	}

	bool CNetClientImpl::S2CStub::EnableLog(Proud::HostID remote, Proud::RmiContext &rmiContext)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_enableLog = true;

		return true;
	}

	bool CNetClientImpl::S2CStub::DisableLog(Proud::HostID remote, Proud::RmiContext &rmiContext)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_enableLog = false;

		return true;
	}

	// 서버로부터 TCP fallback의 필요함을 노티받을 때의 처리
	DEFRMI_ProudS2C_NotifyUdpToTcpFallbackByServer(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		m_owner->FirstChanceFallbackServerUdpToTcp(false, ErrorType_ServerUdpFailed);

		return true;
	}

	static CFastArray<String> g_cachedLocalIpAddress_PROTECTED_CS;

	// lock order -> mainLock then g_cachedLocalIpAddress_CS
	static CriticalSection g_cachedLocalIpAddress_CS;
	static int64_t g_nextCacheLocalIpAddressTimeMs = 0;

	void CNetClientImpl::CacheLocalIpAddress_MustGuaranteeOneThreadCalledByCaller()
	{
		int64_t currTimeMs = GetPreciseCurrentTimeMs();
		if (currTimeMs - g_nextCacheLocalIpAddressTimeMs >= 0)
		{
			// main lock 이 잠겨 있으면 안됨
			AssertIsNotLockedByCurrentThread();

			// 반드시 lock 을 잡지 않은 상태에서 IP 주소를 가져와야 한다.
			CFastArray<String> addresses;
			CNetUtil::GetLocalIPAddresses(addresses);

			CriticalSectionLock lock(g_cachedLocalIpAddress_CS, true);

			g_cachedLocalIpAddress_PROTECTED_CS.Clear();
			g_cachedLocalIpAddress_PROTECTED_CS = addresses;

			g_nextCacheLocalIpAddressTimeMs += currTimeMs + 1000;
		}
	}

	void CNetClientImpl::GetCachedLocalIpAddressSnapshot(CFastArray<String>& out)
	{
		{
			CriticalSectionLock lock(g_cachedLocalIpAddress_CS, true);

			out.Clear();
			out = g_cachedLocalIpAddress_PROTECTED_CS;
		}
	}

	CNetClientImpl::CNetClientImpl()
		: 
		m_ReliablePing_Timer(CNetConfig::GetDefaultNoPingTimeoutTimeMs()), // 어차피 이 인터벌은 중간에 바뀜.
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer(CNetConfig::LongIntervalMs),
		m_enablePingTestEndTime(0),
		m_lastCheckSendQueueTime(0),
		m_isProcessingHeartbeat(0),
		m_lastUpdateNetClientStatCloneTime(0),
		m_disconnectCallTime(0),
		m_sendQueueHeavyStartTime(0),
		m_sendQueueTcpTotalBytes(0),
		m_sendQueueUdpTotalBytes(0)
	{
		m_everyRemoteIssueSendOnNeedIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
		m_checkNextTransitionNetworkTimeMs = GetPreciseCurrentTimeMs();

#if defined(_WIN32)
		CKernel32Api::Instance();
#endif
		m_manager = CNetClientManager::GetSharedPtr(); // 이게 CNetClientImpl.Disconnect의 간헐적 무한 비종료를 해결할지도

#if defined(_WIN32)
		EnableLowFragmentationHeap();
#endif
		m_natDeviceNameDetected = false;
		m_internalVersion = CNetConfig::InternalNetVersion;
		//m_minExtraPing = 0;
		//m_extraPingVariance = 0;
		m_eventSink_NOCSLOCK = NULL;
		m_lastFrameMoveInvokedTime = 0;
		m_disconnectInvokeCount = m_connectCount = 0;
		m_toServerUdpSendCount = 0;
		m_lastReportUdpCountTime = GetPreciseCurrentTimeMs() + CNetConfig::ReportRealUdpCountIntervalMs;

		AttachProxy(&m_c2cProxy);
		AttachProxy(&m_c2sProxy);
		AttachStub(&m_s2cStub);
		AttachStub(&m_c2cStub);
		m_c2cProxy.m_internalUse = true;
		m_c2sProxy.m_internalUse = true;
		m_s2cStub.m_internalUse = true;
		m_c2cStub.m_internalUse = true;

		m_c2cStub.m_owner = this;
		m_s2cStub.m_owner = this;

		// 접속하는 서버의 ip와 port 등을 확인하기 위해 서버접속 시점부터 로그를 남기기 위함이였습니다.
		m_enableLog = false;
		m_virtualSpeedHackMultiplication = 1;

		CFastHeapSettings fss;
		fss.m_pHeap = NULL;
		fss.m_accessMode = FastHeapAccessMode_UnsafeSingleThread;
		fss.m_debugSafetyCheckCritSec = &GetCriticalSection();

		//m_preFinalRecvQueue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_finalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_postponedFinalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_loopbackFinalReceivedMessageQueue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		m_ioPendingCount = 0;

		m_simplePacketMode = false;
		m_isDisconnectCalled = false;
		m_isFrameMoveCalled = false;

		m_lastHeartbeatTime = GetPreciseCurrentTimeMs();
		m_recentElapsedTime = 0;

		m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_Real;
		m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_Real;

		m_loopbackHost = NULL;
		m_remoteServer = NULL;

		m_pendedConnectionParam = NULL;

		m_TcpAndUdp_DoForShortInterval_lastTime = 0;
	}

	// ErrorInfoPtr을 따로 주지 않는 레거시 함수
	bool CNetClientImpl::Connect(const CNetConnectionParam &param)
	{
		ErrorInfoPtr errorInfo;
		return Connect(param, errorInfo);
	}

	// ErrorInfoPtr을 따로 주는 함수. 그리고 no throw를 보장한다. UE4에서 쓸 수 있는 함수.
	bool CNetClientImpl::Connect(const CNetConnectionParam &param, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();

			CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);
			LockMain_AssertIsNotLockedByCurrentThread();

			CriticalSectionLock mainLock(GetCriticalSection(), true);

			// 사용자가 Disconnect를 호출한 경우에는 처리하지 않습니다.
			if (m_isDisconnectCalled)
			{
				outError = ErrorInfo::From(ErrorType_AlreadyExists, HostID_None, _PNT("Cannot call Connect() while Disconnect() is working on another thread."));
				return false;
			}

			if (m_worker == NULL)
			{
				return Connect_Internal(param);
			}

			if (m_pendedConnectionParam == NULL)
			{
				m_pendedConnectionParam = new CNetConnectionParam;
			}
			*m_pendedConnectionParam = param;

			return true;
		}
		NOTHROW_FUNCTION_CATCH_ALL

		return false;
	}

	bool CNetClientImpl::IsCalledByWorkerThread()
	{
		if (m_netThreadPool == NULL)
		{
			assert(0);
		}

		return m_netThreadPool->ContainsCurrentThread();
	}

	// connect 과정의 내부 본체
	bool CNetClientImpl::Connect_Internal(const CNetConnectionParam &param)
	{
		CriticalSectionLock clk(GetCriticalSection(), true); // for atomic oper

		Connect_CheckStateAndParameters(param);

		AtomicIncrement32(&m_connectCount);

		// 		CServerConnectionState cs;
		// 		ConnectionState retcs = GetServerConnectionState(cs);
		// 		if ( retcs != ConnectionState_Disconnected)
		// 		{
		// 			throw Exception(_PNT("Wrong state(%s)! Disconnect() or GetServerConnectionState() may be required."), ToString(retcs));
		// 		}
		SetTimerCallbackParameter(param.m_timerCallbackIntervalMs, param.m_timerCallbackParallelMaxCount, param.m_timerCallbackContext);

		assert(m_sendReadyList == NULL);
		m_sendReadyList = new CSendReadySockets;

		// loopback host와 remote server host 인스턴스를 생성한다.
		// 이들은 Connect 호출 순간부터 Disconnect 리턴 까지 시간대에서 항상 존재한다.
		m_loopbackHost = new CLoopbackHost(HostID_None);

		// 일단은 candidate에 들어간다. local hostID를 아직 모르기에.
		m_candidateHosts.Add(m_loopbackHost, m_loopbackHost);

		m_remoteServer = new CRemoteServer_C(this);

		// NOTE: 서버와의 연결을 시작도 안했는데 서버 주소를 dns lookup하거나 string to ip 같은 짓을 하지 말자.
		// 전자는 소폭 블러킹 때문에 이 함수가 블러킹을 일으키고, 후자는 hostname을 처리 못하니까.

		m_candidateHosts.Add(m_remoteServer, m_remoteServer); // 아직 서버와의 연결이 된 것은 아니니, 원리원칙대로 candidate에 들어가야.

		SocketToHostsMap_SetForAnyAddr(m_remoteServer->m_ToServerTcp, m_remoteServer);

		if (m_remoteServer->m_ToServerUdp /*|| m_remoteServer->GetToServerUdpFallbackable()*/) // 하지만 이건 이미 제거된 상태이어야 한다.
		{
			String txt;
			txt.Format(_PNT("Unstability in Connect #1! Process=%s"), GetProcessName().GetString());
			CErrorReporter_Indeed::Report(txt);
		}

		/* 이제, 상태 청소를 해도 된다!
		서버 접속에 관련된 모든 값들을 초기화한다.
		(disconnected state에서도 이게 안 비어있을 수 있다.
		왜냐하면 서버에서 추방직전 쏜 RMI를 클라가 모두 처리하려면 disconnected state에서도 미처리 항목을 유지해야 하기 때문이다.) */
		if (param.m_tunedNetworkerSendIntervalMs_TEST > 0)
		{
			m_everyRemoteIssueSendOnNeedIntervalMs = param.m_tunedNetworkerSendIntervalMs_TEST;
		}
		else
		{
			m_everyRemoteIssueSendOnNeedIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
		}

		// thread pool 초기화.
		if (m_netThreadPool == NULL)
		{
			switch (param.m_netWorkerThreadModel)
			{
			case ThreadModel_SingleThreaded:
				m_netThreadPool = m_manager->m_netWorkerZeroThreadPool;
				break;

			case ThreadModel_MultiThreaded:
				m_netThreadPool = m_manager->m_netWorkerMultiThreadPool;
				break;

			case ThreadModel_UseExternalThreadPool:
				if (param.m_externalNetWorkerThreadPool == NULL)
				{
					throw Exception(_PNT("External thread pool is not set!"));
				}

				m_netThreadPool = ((CThreadPoolImpl*)param.m_externalNetWorkerThreadPool);
				break;
			}

			m_netThreadPool->RegisterReferrer(this);
		}

		if (m_userThreadPool == NULL)
		{
			switch (param.m_userWorkerThreadModel)
			{
			case ThreadModel_SingleThreaded:
				// Zero Thread 로 사용함
				m_userThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(NULL, 0));
				break;

			case ThreadModel_MultiThreaded:
				// Multi Thread 로 사용함
				m_userThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(NULL, GetNoofProcessors()));
				break;

			case ThreadModel_UseExternalThreadPool:
				if (param.m_externalUserWorkerThreadPool == NULL)
				{
					throw Exception();
				}

				m_userThreadPool = ((CThreadPoolImpl*)param.m_externalUserWorkerThreadPool);
				break;
			}

			m_userThreadPool->RegisterReferrer(this);
		}

		m_hasWarnedStarvation = false;

		// 마지막 Disconnect에서 이게 리셋 안되어 있고 예전에 쓰던 값이 있으면 곤란하므로
		m_settings = CNetSettings();

		int64_t currentTimeMs = GetPreciseCurrentTimeMs();
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.SetIntervalMs(CNetConfig::LongIntervalMs);
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.Reset(currentTimeMs);
		m_ReliablePing_Timer.SetIntervalMs(GetReliablePingTimerIntervalMs());
		m_ReliablePing_Timer.Reset(currentTimeMs);

		m_natDeviceNameDetected = false;

		m_P2PConnectionTrialEndTime = CNetConfig::GetP2PHolepunchEndTimeMs();
		m_P2PHolepunchInterval = CNetConfig::P2PHolepunchIntervalMs;

		// reset it
		m_stats = CNetClientStats();

		// ACR
		m_enableAutoConnectionRecovery = param.m_enableAutoConnectionRecovery;

		// ACR은 simple packet mode에서 강제로 꺼진다.
		// message id 교환 등 복잡한 것들은 사용자가 다루기 힘들다.
		if (param.m_simplePacketMode)
			m_enableAutoConnectionRecovery = false;

		m_autoConnectionRecoveryDelayMs = 0;
		m_autoConnectionRecoveryContext = CAutoConnectionRecoveryContextPtr();

		m_lastFrameMoveInvokedTime = currentTimeMs;

		m_toServerEncryptCount = 0;
		m_toServerDecryptCount = 0;

		m_lastRequestServerTimeTime = 0;
		m_RequestServerTimeCount = 0;

		m_serverTimeDiff = 0;

		m_serverUdpRecentPingMs = 0;
		m_serverUdpLastPingMs = 0;
		m_serverUdpPingChecked = false;

		m_serverTcpRecentPingMs = 0;
		m_serverTcpLastPingMs = 0;
		m_serverTcpPingChecked = false;
		m_firstTcpPingedTime = 0;

		m_disconnectCallTime = 0;
		m_isDisconnectCalled = false;

		//m_lastMessageOverloadCheckingTime = GetPreciseCurrentTimeMs() + CNetConfig::NetClientMessageOverloadTimerIntervalMs;

		// 서버에 접속을 시작하는 상황이므로, 전부다 리셋.
		m_loopbackHost->m_backupHostID = m_loopbackHost->m_HostID = HostID_None;

		m_speedHackDetectorPingTime = CNetConfig::EnableSpeedHackDetectorByDefault ? 0 : INT64_MAX;

		m_selfEncryptCount = 0;
		m_selfDecryptCount = 0;

		// m_slowReliableP2P 모드가 사용중인가 판단하여 적절한 값을 실제 사용할 변수 공간에 저장한다.
		// m_slowReliableP2P 는 per-Client 값
		// m_slowReliableP2P 을 참조해야 하는 곳에서 이값만 참조하여 3항 연산자(?:) 를 사용해도 되겠지만 하드코딩(0.001L) 값이 다른 값으로 바뀌어야 하거나(유지보수)
		// 계속 값을 참조하여 3항 연산자(?:) 를 써야 한다면 속도상 약간 손해를 볼 수 있어서 이렇게 구현하기로 결정
		// 또한 Renewal 된 ReliableUDP 는 ReliableUdpSender/ReliableUdpReceiver 가 ReliableUDPHost 로 통합되어 포인터 참조 횟수가 줄어든다
		if (param.m_slowReliableP2P)
		{
			m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_ForDummyTest;
			m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_ForDummyTest;
		}
		else // case false
		{
			m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_Real/*0.001L*/;		// Maximize P2P trans throughtput
			m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_Real/*0.001L*/;
		}

		m_supressSubsequentDisconnectionEvents = false;

		m_connectionParam = param;
		m_simplePacketMode = param.m_simplePacketMode;
		m_allowOnExceptionCallback = param.m_allowOnExceptionCallback;

		// 빈 문자열 들어가면 localhost로 자동으로 채움
		m_connectionParam.m_serverIP = m_connectionParam.m_serverIP.Trim();
		if (m_connectionParam.m_serverIP.IsEmpty())
		{
			m_connectionParam.m_serverIP = _PNT("localhost");
		}

		//m_ToServerUdp_fallbackable = CFallbackableUdpLayerPtr_C(new CFallbackableUdpLayer_C(this));

		m_checkTcpUnstable_timeToDo = m_lastHeartbeatTime = GetPreciseCurrentTimeMs();
		m_recentElapsedTime = 0;

		m_issueSendOnNeedWorking = 0;

		// 클라이언트 워커 스레드에 이 객체를 등록시킨다. 없으면 새로 만든다.
		// 이 객체가 생성되므로 인해 state machine이 작동을 시작한다.
		m_worker = CNetClientWorkerPtr(new CNetClientWorker(this));

		// heartbeat과 coalesce send를 일으키는 event push를 일정 시간마다 일어나게 한다.
		if (m_periodicPoster_Heartbeat == NULL)
		{
			m_periodicPoster_Heartbeat.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_Heartbeat,
				m_netThreadPool,
				CNetConfig::ServerHeartbeatIntervalMs));
		}

		if (m_periodicPoster_SendEnqueued == NULL)
		{
			m_periodicPoster_SendEnqueued.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_SendEnqueued,
				m_netThreadPool,
				CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs));
		}

		if (m_timerCallbackInterval > 0)
		{
			m_timerCallbackParallelCount = 0;
			m_periodicPoster_Tick.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_OnTick,
				m_userThreadPool,
				m_timerCallbackInterval));
		}

		return true;
	}

	void CNetClientImpl::EnqueueDisconnectionEvent(const ErrorType errorType,
		const ErrorType detailType, const String &comment)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (!m_supressSubsequentDisconnectionEvents)
		{
			// 아직 서버로부터 hostid를 받지도 않았는데 연결 해제되는 경우에는, OnClientLeave가 아니라 OnJoinServerComplete(fail)이 떠야 한다.
			if (GetVolatileLocalHostID() == HostID_None)
			{
				EnqueueConnectFailEvent(errorType, comment, SocketErrorCode_Ok);
			}
			else
			{
				LocalEvent e;
				e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
				e.m_type = LocalEventType_ClientServerDisconnect;
				e.m_errorInfo->m_errorType = errorType;
				e.m_errorInfo->m_detailType = detailType;
				e.m_errorInfo->m_comment = comment;
				e.m_remoteHostID = HostID_Server;
				EnqueLocalEvent(e, m_remoteServer);

				m_supressSubsequentDisconnectionEvents = true;
			}

#ifdef VIZAGENT
			// VIZ
			if (m_vizAgent)
			{
				CriticalSectionLock vizlock(m_vizAgent->m_cs, true);
				CServerConnectionState ignore;
				m_vizAgent->m_c2sProxy.NotifyCli_ConnectionState(HostID_Server, g_ReliableSendForPN, GetServerConnectionState(ignore));
			}
#endif
		}
	}

	// 클라가 서버에 접속 과정을 시행 중에 실패하는 이벤트를 쌓는다.
	void CNetClientImpl::EnqueueConnectFailEvent(ErrorType errorType, ErrorInfoPtr errorInfo)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_supressSubsequentDisconnectionEvents == false)
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_ConnectServerFail;
			e.m_errorInfo->m_errorType = errorType;
			e.m_errorInfo->m_comment = errorInfo->m_comment;
			e.m_remoteHostID = HostID_Server;
			e.m_remoteAddr = AddrPort::FromHostNamePort(m_connectionParam.m_serverIP, m_connectionParam.m_serverPort);
			e.m_socketErrorCode = SocketErrorCode_Ok;
			EnqueLocalEvent(e, m_remoteServer);

			m_supressSubsequentDisconnectionEvents = true;
		}
	}

	// 클라가 서버와 연결하는 도중 에러가 남을 이벤트 큐
	void CNetClientImpl::EnqueueConnectFailEvent(ErrorType errorType, 
		const String& comment,
		SocketErrorCode socketErrorCode,
		ByteArrayPtr reply)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		if (m_supressSubsequentDisconnectionEvents == false)
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_ConnectServerFail;
			e.m_errorInfo->m_errorType = errorType;
			e.m_errorInfo->m_comment = comment;
			e.m_remoteHostID = HostID_Server;
			e.m_remoteAddr = AddrPort::FromHostNamePort(m_connectionParam.m_serverIP, m_connectionParam.m_serverPort);
			e.m_socketErrorCode = socketErrorCode;
			e.m_userData = reply;
			EnqueLocalEvent(e, m_remoteServer);

			m_supressSubsequentDisconnectionEvents = true;
		}
	}

	void CNetClientImpl::SetInitialTcpSocketParameters()
	{
		// TCP only & relay only
		//m_ToServerUdp_fallbackable->SetRealUdpEnabled(false);
		m_remoteServer->SetRealUdpEnable(false);
	}

	INetCoreEvent * CNetClientImpl::GetEventSink_NOCSLOCK()
	{
		return m_eventSink_NOCSLOCK;
	}

	DEFRMI_ProudS2C_RequestAutoPrune(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		// 서버와의 연결을 당장 끊는다. Shutdown-shake 과정을 할 필요가 없다. 클라는 디스가 불특정 시간에 일어나는 셈이므로.
		if (m_owner->m_worker != NULL
			&& m_owner->m_worker->GetState() <= CNetClientWorker::Connected)
		{
			if (m_owner->m_remoteServer != NULL)
			{
				// NetServer.CloseConnection()이 호출된 경우에는 아래의 변수를 True로 설정합니다.
				m_owner->m_remoteServer->m_ToServerTcp->m_userCalledDisconnectFunction = true;
			}

			String comment = _PNT("VIA-AP");
			m_owner->EnqueueDisconnectionEvent(ErrorType_DisconnectFromRemote, 
				ErrorType_TCPConnectFailure, comment);
			m_owner->m_worker->SetState(CNetClientWorker::Disconnecting);
		}

		return true;
	}

	DEFRMI_ProudS2C_ShutdownTcpAck(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		// shutdown ack를 받으면 바로 종료 처리를 진행하도록 한다.
		/* 종료시간 시프트 문제로 인한 삭제
		if(m_owner->m_worker && m_owner->Get_ToServerTcp()->GetShutdownIssueTime() > 0 && m_owner->m_worker->m_gracefulDisconnectTimeout)
		{
		//gracefultime에 따라서 처리한다.바로 처리하면, gracefultime가 의미 없음.
		m_owner->Get_ToServerTcp()->SetShutdownIssueTime(GetPreciseCurrentTimeMs());// - m_owner->m_worker->m_gracefulDisconnectTimeout * 2;
		} */

		/* 예전에는 서버로 ShutdownTcpHandshake RMI를 보내고, 서버에서 TCP close를 했다.
		그러나 이 방식은 서버에서 한시적 TIME_WAIT가 계속 쌓이는 버그가 있다.
		자세한 설명은 'client-server 연결 해제.pptx'에.
		*/
		m_owner->EnqueueDisconnectionEvent(ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
			_PNT("User called Disconnect()."));
		m_owner->m_worker->SetState(CNetClientWorker::Disconnecting);

		return true;
	}

	DEFRMI_ProudS2C_RequestMeasureSendSpeed(CNetClientImpl::S2CStub)
	{
		//CriticalSectionLock lock(m_owner->GetCriticalSection(),true);

		//m_owner->m_manager->RequestMeasureSendSpeed(enable);

		return true;
	}

	void CNetClientImpl::Disconnect()
	{
		CDisconnectArgs args;
		Disconnect(args);
	}

	void CNetClientImpl::Disconnect(ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			Disconnect(); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void CNetClientImpl::Disconnect(const CDisconnectArgs &args)
	{
		CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);

		if (m_isFrameMoveCalled)
			throw Exception("Cannot call Disconnect in UserMessage, RMI, LocalEvent.");

		LockMain_AssertIsNotLockedByCurrentThread();

		// 		// 드물지만 CNetClientManager가 먼저 이미 파괴된 경우라면 프로세스 종료 상황이겠다. 이럴 때는 이 함수는 즉시 리턴한다.
		// 		if(!m_manager->m_cs.IsValid())
		// 			goto LEXIT;

		int64_t startTime = 0;

		uint32_t tickCount = 0;
		int64_t timeOut = PNMAX(args.m_gracefulDisconnectTimeoutMs * 2, 100000); // modify by rekfkno1 - 시간을 100초로 늘립니다.

		{
			CriticalSectionLock mainLock(GetCriticalSection(), true);

			// Disconnect가 중복으로 실행하는 경우
			if (m_worker == NULL)
				return;

			// 통계
			AtomicIncrement32(&m_disconnectInvokeCount);

			tickCount = CFakeWin32::GetTickCount();

			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
			{
				Log(0, LogCategory_System, _PNT("User call CNetClient.Disconnect."));
			}

			DisconnectAsync(args);

			// 사용자가 Disconnect를 호출한 이상, 앞으로 호출되는 DisconnectAsync 및 Connect 함수에 대한 처리를 하지 않습니다.
			m_isDisconnectCalled = true;

			// 이전에 사용자가 Connect를 호출하여 저장되었던 pendedConnectionParam를 지웁니다.
			if (m_pendedConnectionParam != NULL)
			{
				delete m_pendedConnectionParam;
				m_pendedConnectionParam = NULL;
			}

			startTime = GetPreciseCurrentTimeMs();
		}

		int32_t waitCount = 0;

		// 완전히 디스될 때까지 루프를 돈다. 디스 이슈는 필요시 하고.
		while (1)
		{
			/* Disconnect 함수에서 Blocking이 되어 UserThreadPool 이벤트를 처리하는 FrameMove 함수를 호출하지 못합니다.
			이로인하여 UserThreadPool으로 Post한 DoUserTask 메세지가 정상적으로 처리가 되지 않는 문제가 발생할 수 있다. 
			그래서 여기서 대신 처리해 준다. */
			if (m_connectionParam.m_netWorkerThreadModel == ThreadModel_SingleThreaded)
			{
				m_netThreadPool->Process(0);
			}

			if (m_connectionParam.m_userWorkerThreadModel == ThreadModel_SingleThreaded)
			{
				m_userThreadPool->Process(0);
			}

			CriticalSectionLock lock(GetCriticalSection(), true); // 이걸로 보호한 후 m_worker등을 모두 체크하도록 하자.

			// 이미 디스된 상황이면 그냥 나간다.
			if (m_worker == NULL)
			{
#if defined(_WIN32)
				if (m_remoteServer != NULL)
				{
					if (m_remoteServer->m_ToServerUdp || m_remoteServer->GetToServerUdpFallbackable()) // 불안정한 상황인가?
					{
						String txt;
						txt.Format(_PNT("Unstability in Disconnect #1! Process=%s"), GetProcessName().GetString());
						CErrorReporter_Indeed::Report(txt);
					}
				}
#endif
				// 어쨌건 나가도 되는 상황
				return;
			}

			// tcp,udp socket들이 모두 io event가 더 이상 없음이 보장되는 상황이면 리턴.
			if (m_worker->GetState() == CNetClientWorker::Disconnected)
			{
				break;
			}

#if defined(_WIN32)            
			// 프로세스 종료중이면 아무것도 하지 않는다.
			if (Thread::m_dllProcessDetached_INTERNAL)
			{
				break;
			}
#endif
			if (CNetConfig::ConcealDeadlockOnDisconnect && CFakeWin32::GetTickCount() - tickCount > timeOut)
			{
#if defined(_WIN32)
				// 오류 상황을 제보한다.
				String txt;
				txt.Format(_PNT("CNetClient.Disconnect seems to be freezed ## State=%d##Process=%s"),
					m_worker->GetState(),
					GetProcessName().GetString());
				CErrorReporter_Indeed::Report(txt);
#endif
				m_worker->SetState(CNetClientWorker::Disconnected);
				break;
			}

			// gracefulDisconnectTimeout 지정 시간이 지난 후 실 종료 과정이 시작된다
			// 실 종료 과정이 완전히 끝날 때까지 대기
			if (waitCount > 0) // 보다 빠른 응답성을 위해
			{
				lock.Unlock();
				Proud::Sleep(args.m_disconnectSleepIntervalMs);
				if (((GetPreciseCurrentTimeMs() - startTime)) > 5000)
				{
					//OutputDebugString(_PNT("NetClient Disconnect is not yet ended.\n"));
					NTTNTRACE("NetClient Disconnect is not yet ended.\n");
					startTime = GetPreciseCurrentTimeMs();
				}
			}

			waitCount++;
		}

		LockMain_AssertIsNotLockedByCurrentThread();

		// TimeThreadPool에 등록되었던것들을 지운다.
		m_periodicPoster_Tick.Free();
		m_periodicPoster_Heartbeat.Free();
		m_periodicPoster_SendEnqueued.Free();

		// thread pool로부터 독립한다.
		if (m_userThreadPool != NULL)
		{
			m_userThreadPool->UnregisterReferrer(this);
			if (m_connectionParam.m_userWorkerThreadModel == ThreadModel_UseExternalThreadPool)
			{
				m_userThreadPool = NULL;
			}
			else
			{
				SAFE_DELETE(m_userThreadPool);
			}
		}

		if (m_netThreadPool != NULL)
		{
			m_netThreadPool->UnregisterReferrer(this);
			m_netThreadPool = NULL;
		}

		// 스레드도 싹 종료. 이제 완전히 클리어하자.
		{
			CriticalSectionLock lock(GetCriticalSection(), true); // 이걸로 보호한 후 m_worker등을 모두 체크하도록 하자.
			Cleanup();

			m_worker = CNetClientWorkerPtr();

			m_isDisconnectCalled = false;
		}
#if defined(_WIN32)
		XXXHeapChk();
#endif
	}

	void CNetClientImpl::Disconnect(const CDisconnectArgs &args, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			Disconnect(args);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void CNetClientImpl::DisconnectAsync()
	{
		CDisconnectArgs args;
		DisconnectAsync(args);
	}

	void CNetClientImpl::DisconnectAsync(ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			DisconnectAsync(); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	/* Disconnect()와 달리, 이함수는 블러킹 없이 연결 해제를 건다.
	user callback 안에서 호출해도 된다.
	다만 Disconnect()와 달리 이 함수 호출 후에도 OnLeaveServer등
	유저콜백이 발생할 수 있다. */
	void CNetClientImpl::DisconnectAsync(const CDisconnectArgs &args)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// ACR 용 remote server 와 super socket garbage 요청
		// Disconnecting state가 될 때 호출되므로 여기서 불필요. 해서도 안됨.
		//AutoConnectionRecovery_DestroyCandidateRemoteServersAndSockets(CSuperSocketPtr());

		// 사용자가 Disconnect를 호출한 경우에는 처리하지 않습니다.
		if (m_isDisconnectCalled)
			return;

		if (m_disconnectCallTime == 0)
		{
			if (m_worker->GetState() == CNetClientWorker::Connected)
			{
				// 언제 디스 작업을 시작할건가를 지정
				if (m_remoteServer != NULL)
				{
					// NetClient.Disconnect()이 호출된 경우에는 아래의 값을 True로 지정합니다.
					m_remoteServer->m_ToServerTcp->m_userCalledDisconnectFunction = true;
					m_remoteServer->m_shutdownIssuedTime = GetPreciseCurrentTimeMs(); // 이거에 의해 Networker State가 disconnecting으로 곧 바뀐다.
				}

				// 서버와의 연결 해제를 서버에 먼저 알린다.
				// 바로 소켓을 닫고 클라 프로세스가 바로 종료하면 shutdown 신호가 TCP 서버로 넘어가지 못하는 경우가 있다.
				// 따라서 서버에서 연결 해제를 주도시킨 후 클라에서 종료하되 시간 제한을 두는 형태로 한다.
				// (즉 TCP의 graceful shutdown을 대신한다. )
				m_worker->m_gracefulDisconnectTimeout = args.m_gracefulDisconnectTimeoutMs;

				if (GetVolatileLocalHostID() != HostID_None)
				{
					// 아직 HostID조차 발급받지 못한 상황이면 ShutdownTcp 메시지를 보내지 않습니다.
					m_c2sProxy.ShutdownTcp(HostID_Server, g_ReliableSendForPN, args.m_comment);
				}
			}
			else if (m_worker->GetState() < CNetClientWorker::Connected)
			{
				// 서버와 연결이 완료되기 전이었으면 당장 디스모드로 전환
				m_worker->SetState(CNetClientWorker::Disconnecting);
			}

			m_disconnectCallTime = GetPreciseCurrentTimeMs();
		}

	}

	void CNetClientImpl::DisconnectAsync(const CDisconnectArgs &args, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			DisconnectAsync(args); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}


	// 사용자가 호출한 DisconnectAsync가 완전히 처리가 끝났는지?
	bool CNetClientImpl::IsDisconnectAsyncEnded()
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (m_worker == NULL)
			return true;

		return (m_worker->GetState() == CNetClientWorker::Disconnected);
	}

	CNetClientImpl::~CNetClientImpl()
	{
		LockMain_AssertIsNotLockedByCurrentThread();

		// Disconnect()는 throwable이다. 그러나 여기는 dtor다. 따라서 throw하지 말고 딱 정리해 주어야.
		try { Disconnect(); }
		catch (std::exception&) {}

		// PN 내부에서도 쓰는 RMI까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
		CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.

		//m_hlaSessionHostImpl.Free();

#ifdef VIZAGENT
		m_vizAgent.Free();
#endif
	}

	bool CNetClientImpl::GetP2PGroupByHostID(HostID groupHostID, CP2PGroup &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_C g = GetP2PGroupByHostID_Internal(groupHostID);
		if (g == NULL)
			return false;
		else
		{
			g->ToInfo(output);
			return true;
		}
	}

	Proud::CP2PGroupPtr_C CNetClientImpl::GetP2PGroupByHostID_Internal(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_C g;
		m_P2PGroups.TryGetValue(groupHostID, g);
		return g;
	}

	bool CNetClientImpl::ConvertAndAppendP2PGroupToPeerList(HostID sendTo, ISendDestArray &sendTo2)
	{
		LockMain_AssertIsLockedByCurrentThread();
		// convert sendTo group to remote hosts

		CP2PGroupPtr_C g = GetP2PGroupByHostID_Internal(sendTo);
		if (g == NULL)
		{
			sendTo2.Add(AuthedHostMap_Get(sendTo));
		}
		else
		{
			for (P2PGroupMembers_C::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
			{
				HostID memberID = i->GetFirst();
				sendTo2.Add(AuthedHostMap_Get(memberID));
			}
		}
		return true;
	}
	
	Proud::CRemotePeer_C* CNetClientImpl::GetPeerByHostID_NOLOCK(HostID peerHostID)
	{
		AssertIsLockedByCurrentThread();

		CHostBase* ret = NULL;

		ret = AuthedHostMap_Get(peerHostID);

		return LeanDynamicCastForRemotePeer(ret);
	}

	Proud::CSessionKey *CNetClientImpl::GetCryptSessionKey(HostID remote, String &errorOut, bool& outEnqueError)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);

		CSessionKey *key = NULL;

		if (rp != NULL)
		{
			key = &rp->m_p2pSessionKey;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			key = &m_selfP2PSessionKey;
		}
		else if (remote == HostID_Server)
		{
			key = &m_toServerSessionKey;
		}

		if (key == NULL)
		{
			// error 처리를 하지 않고 무시한다.
			errorOut.Format(_PNT("peer(%d) is %s in NetClient!"), (int)remote, rp == NULL ? _PNT("NULL") : _PNT("not NULL"));
			outEnqueError = false;

			return NULL;
		}

		if (!key->EveryKeyExists())
		{
			// error 처리를 해주어야 한다.
			errorOut.Format(_PNT("Key does not exist. Note that P2P encryption can be enabled on NetServer.Start()."));
			outEnqueError = true;

			return NULL;
		}

		return key;
	}

	// <홀펀칭이 성공한 주소>를 근거로 peer를 찾는다.
	// findInRecycleToo: true이면 recycled remote peers에서도 찾는다.
	CRemotePeer_C* CNetClientImpl::GetPeerByUdpAddr(AddrPort UdpAddr, bool findInRecycleToo)
	{
		//CriticalSectionLock clk(m_critSecPtr, true);
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			CRemotePeer_C* rp = LeanDynamicCastForRemotePeer(i->GetSecond());
			if (rp && !rp->m_garbaged && rp->m_P2PHolepunchedRemoteToLocalAddr == UdpAddr)
			{
				return rp;
			}
		}
		for (HostIDToRemotePeerMap::iterator i = m_remotePeerRecycles.begin(); i != m_remotePeerRecycles.end(); i++)
		{
			CRemotePeer_C* rp = i->GetSecond();
			if (rp && !rp->m_garbaged && rp->m_P2PHolepunchedRemoteToLocalAddr == UdpAddr)
			{
				return rp;
			}
		}
		return NULL;
	}

	void CNetClientImpl::SendServerHolepunch()
	{
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_ServerHolepunch);
		sendMsg.Write(m_remoteServer->GetServerUdpHolepunchMagicNumber());

		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			String a = m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr.ToString();
			Log(0, LogCategory_P2P, String::NewFormat(_PNT("Sending ServerHolepunch: %s"), a.GetString()));
		}

		m_remoteServer->m_ToServerUdp->AddToSendQueueWithSplitterAndSignal_Copy(HostID_Server,
			FilterTag::CreateFilterTag(GetVolatileLocalHostID(), HostID_Server),
			m_remoteServer->GetServerUdpAddr(),
			sendMsg,
			GetPreciseCurrentTimeMs(),
			SendOpt(MessagePriority_Holepunch, true));
	}

	void CNetClientImpl::SendServerHolePunchOnNeed()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// UDP를 강제로 끈 상태가 아니라면 시도조차 하지 않도록 한다.
		if (m_settings.m_fallbackMethod == FallbackMethod_ServerUdpToTcp)
			return;

		//소켓이 준비 되지 않았다면 하지 않는다.
		if (m_remoteServer->m_ToServerUdp == NULL || m_remoteServer->m_ToServerUdp->IsSocketClosed())
			return;

		// 아직 HostID를 배정받지 않은 클라이면 서버 홀펀칭을 시도하지 않는다.
		// 어차피 서버로 시도해봤자 서버측에서 HostID=0이면 splitter에서 실패하기 때문에 무시된다.
		if (GetVolatileLocalHostID() == HostID_None)
			return;

		// 필요시 서버와의 홀펀칭 수행
		if (m_remoteServer->MustDoServerHolepunch())
			SendServerHolepunch();
	}

	bool CNetClientImpl::GetPeerInfo(HostID remoteHostID, CNetPeerInfo &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		CHostBase* hostBase = NULL;
		m_authedHostMap.TryGetValue(remoteHostID, hostBase);
		m_authedHostMap.AssertConsist();

		CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(hostBase);
		if (remotePeer)
		{
			remotePeer->ToNetPeerInfo(output);
			return true;
		}

		return false;
	}

	void CNetClientImpl::GetLocalJoinedP2PGroups(HostIDArray &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		output.Clear();
		for (P2PGroups_C::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); i++)
		{
			output.Add(i->GetFirst());
		}
	}

	//#ifdef DEPRECATE_SIMLAG
	//	void CNetClientImpl::SimulateBadTraffic( DWORD minExtraPing, DWORD extraPingVariance )
	//	{
	//		m_minExtraPing = ((double)minExtraPing) / 1000;
	//		m_extraPingVariance = ((double)extraPingVariance) / 1000;
	//	}
	//#endif // DEPRECATE_SIMLAG

	Proud::CP2PGroupPtr_C CNetClientImpl::CreateP2PGroupObject_INTERNAL(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_C GP(new CP2PGroup_C());
		GP->m_groupHostID = groupHostID;
		m_P2PGroups.Add(groupHostID, GP);

		return GP;
	}

	void CNetClientImpl::RequestServerTimeOnNeed()
	{
		if (GetVolatileLocalHostID() != HostID_None)
		{
			int64_t currTime = GetPreciseCurrentTimeMs();

			// UDP 핑
			if (currTime - m_lastRequestServerTimeTime > CNetConfig::UnreliablePingIntervalMs / m_virtualSpeedHackMultiplication)
			{
				m_RequestServerTimeCount++;
				m_lastRequestServerTimeTime = currTime;

				// UDP로 주고 받는 메시지
				// 핑으로도 쓰인다.
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				sendMsg.Write((char)MessageType_UnreliablePing);
				sendMsg.Write(currTime);
				sendMsg.Write(m_serverUdpRecentPingMs);

				int CSLossPercent = 0;
				GetUnreliableMessagingLossRatioPercent(HostID_Server, &CSLossPercent);

				int64_t speed = 0;
				if (m_remoteServer->m_ToServerUdp != NULL)
				{
					// 서버->클라 실제 수신한 속도
					speed = m_remoteServer->m_ToServerUdp->GetRecentReceiveSpeed(m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr);
				}

				// 수신속도, 패킷로스율 전송
				sendMsg.WriteScalar(speed);
				sendMsg.Write(CSLossPercent);

				m_remoteServer->GetToServerUdpFallbackable()->SendWithSplitterViaUdpOrTcp_Copy(HostID_Server, CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
			}

			if (m_ReliablePing_Timer.IsTimeToDo(currTime))
			{
				int ackNum = 0;
				m_remoteServer->m_ToServerTcp->AcrMessageRecovery_PeekMessageIDToAck(&ackNum);
				m_c2sProxy.ReliablePing(HostID_Server, g_ReliableSendForPN, m_applicationHint.m_recentFrameRate, (int)currTime, ackNum);
			}
		}
		else
		{
			// 서버에 연결 상태가 아니면 미리미리 이렇게 준비해둔다.
			m_remoteServer->UpdateServerUdpReceivedTime();
			//m_lastReliablePongReceivedTime = m_lastServerUdpPacketReceivedTime;
		}
	}

	void CNetClientImpl::SpeedHackPingOnNeed()
	{
		if (GetVolatileLocalHostID() != HostID_None)
		{
			if (GetPreciseCurrentTimeMs() - m_speedHackDetectorPingTime > 0)
			{
				// virtual speed hack
				m_speedHackDetectorPingTime = GetPreciseCurrentTimeMs() + CNetConfig::SpeedHackDetectorPingIntervalMs / m_virtualSpeedHackMultiplication;

				//ATLTRACE("%d client Send SpeedHackDetectorPing\n",GetVolatileLocalHostID());
				// UDP로 주고 받는 메시지
				// 핑으로도 쓰인다.
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				sendMsg.Write((char)MessageType_SpeedHackDetectorPing);

				m_remoteServer->GetToServerUdpFallbackable()->SendWithSplitterViaUdpOrTcp_Copy(HostID_Server, CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
			}
		}
	}

	int64_t CNetClientImpl::GetReliablePingTimerIntervalMs()
	{
		/* 이제는 TCP in crisis 체크 기능이 사라졌다. UDP 혼잡제어 기능이 알아서 처리하니까.
		그리고 reliable 메시징의 ping-pong은 unreliable과 달리 20-30% 이런 식이 아니라
		(time out - unfeasible RTO)가 기준이 되어야 한다.
		(패킷 로스면 알아서 RTT based retransmission을 하기 때문임.)
		단, 1초 이상은 보장해 주어야 함.
		따라서 아래와 같이 연산해서 리턴한다.

		NOTE: TCP retransmission timeout은 OS딴에서 이미 20초이다. 따라서 이 값은 20-5=15초를 넘길 이유가 없다.
		*/

		// defaultTimeoutTime 의 값은 tcp retransmission timeout 시간보다 작아야 한다.
		// NOTE: GetDefaultNoPingTimeoutTimeMs 함수 내에 설명 되어 있음
		//assert(m_settings.m_defaultTimeoutTime < 20000);

		return PNMAX(m_settings.m_defaultTimeoutTime - 5000, 1000);

		// NOTE: 위의 식은 m_defaultTimeoutTime 의 최대 20초로 세팅되어있는걸 가정하여 -5 초를 한것이다.
		//		 따라서 20 - 5 = 15초 근사치에 맞게 defaultTimeoutTime 의 75% 로 잡는다.
		//return PNMAX((m_settings.m_defaultTimeoutTime * 75) / 100, 1000);

		// ==퇴역 노트==
		// 연속 두세번 보낸 것이 일정 시간 안에 도착 안하면 사실상 막장이므로 이정도 주기면 OK.
		// (X / 10) * 3 보다 (X * 3) / 10 이 정확도가 더 높다.
		// int64_t 로 캐스팅 해주지 않으면 int 범위를 벗어날 경우 음수가 return  되어 문제가 발생할 수 있다.

		// 과거에는 30%였으나 TCP timeout을 30초로 지정할 경우, 9초마다 주고받는다.
		// 그러나 이러한 경우 마지막 TCP 데이터 수신 시간 후 60% 즉 18초가 지나야 위험 상황임을 알고
		// 혼잡 제어를 들어가는데 윈도 OS는 20초동안 retransmission이 실패하면 10053 디스를 일으킨다.
		// 따라서 무용지물. 이를 막기 위해서는 10%로 줄이고 위험 수위를 30%로 재조정해야 한다.
		// 그래서 1로 변경.
		//return (((int64_t)m_settings.m_defaultTimeoutTime) * 1) / 10;
		//		return (((int64_t)m_settings.m_defaultTimeoutTime) * 3) / 10;
	}

	DEFRMI_ProudS2C_NotifySpeedHackDetectorEnabled(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		if (enable)
		{
			m_owner->m_speedHackDetectorPingTime = 0;
		}
		else
		{
			m_owner->m_speedHackDetectorPingTime = INT64_MAX;
		}

		return true;
	}

	int64_t CNetClientImpl::GetIndirectServerTimeMs(HostID peerHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 		if(m_timer == NULL)
		// 			return -1;

		int64_t clientTime = GetPreciseCurrentTimeMs();

		CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(peerHostID);
		if (peer != NULL)
		{
			if (!peer->m_forceRelayP2P)
				peer->m_jitDirectP2PNeeded = true;
			return clientTime - peer->GetIndirectServerTimeDiff();
		}

		return clientTime - m_serverTimeDiff;
	}

	int64_t CNetClientImpl::GetServerTimeMs()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 		if(m_timer == NULL)
		// 			return -1;

		int64_t clientTime = GetPreciseCurrentTimeMs();

		return clientTime - m_serverTimeDiff;
	}

	void CNetClientImpl::FallbackServerUdpToTcpOnNeed()
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		// 너무 오랜 시간동안 서버에 대한 UDP ping이 실패하면 UDP도 TCP fallback mode로 전환한다.
		// [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
		// 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.
		if (m_remoteServer->FallbackServerUdpToTcpOnNeed(currTime))
		{
			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
			{
				String TrafficStat = GetTrafficStatText();
				int64_t UdpDuration = GetPreciseCurrentTimeMs() - m_remoteServer->GetToServerUdpFallbackable()->m_realUdpEnabledTimeMs;
				int64_t LastUdpRecvIssueDuration = 0;

				if (m_remoteServer->m_ToServerUdp)
					LastUdpRecvIssueDuration = GetPreciseCurrentTimeMs() - m_remoteServer->m_ToServerUdp->m_lastReceivedTime;

				String s1 = Get_ToServerUdpSocketLocalAddr().ToString();

				int rank = 1;
				bool LBN = false;
				if (IsLocalHostBehindNat(LBN) && !LBN)
					rank++;
#ifdef _WIN32
				if (!m_manager->m_upnp->GetNatDeviceName().IsEmpty())
					rank++;
#endif
				String txt;
				txt.Format(_PNT("(first chance) to-server UDP punch lost##Reason:%d##CliInstCount=%d##DisconnedCount=%d##recv count=%d##last ok recv interval=%3.3lf##Recurred:%d##LocalIP:%s##UDP kept time:%3.3lf##Time diff since RecvIssue:%3.3lf##NAT name=%s##%s##Process=%s"),
					ErrorType_ServerUdpFailed,
					m_manager->m_instanceCount,
					m_manager->m_disconnectInvokeCount,
					m_remoteServer->GetToServerUdpFallbackable()->m_lastServerUdpPacketReceivedCount,
					double(m_remoteServer->GetToServerUdpFallbackable()->m_lastUdpPacketReceivedIntervalMs) / 1000,
					m_remoteServer->GetToServerUdpFallbackable()->m_TcpFallbackCount,
					s1.GetString(),
					double(UdpDuration) / 1000,
					double(LastUdpRecvIssueDuration) / 1000,
#ifdef _WIN32
					m_manager->m_upnp->GetNatDeviceName().GetString(),
#else
					_PNT("Not support"),
#endif
					TrafficStat.GetString(),
					GetProcessName().GetString());

				LogHolepunchFreqFail(rank, _PNT("%s"), txt.GetString());
			}
			// 서버에 TCP fallback을 해야 함을 노티.
			m_c2sProxy.NotifyUdpToTcpFallbackByClient(HostID_Server, g_ReliableSendForPN);
			/* 클라에서 to-server-UDP가 증발해도 per-peer UDP는 증발하지 않는다. 아예 internal port 자체가 다르니까.
			따라서 to-peer UDP는 그대로 둔다. */
		}
	}

	int64_t CNetClientImpl::GetP2PServerTimeMs(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		// 서버와 그룹 멤버의 서버 시간의 평균을 구한다.
		int count = 1;
		int64_t diffSum = GetServerTimeDiffMs();
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(groupHostID);
		if (group != NULL)
		{
			// 			if(m_timer == NULL)
			// 				return -1;

			for (P2PGroupMembers_C::iterator i = group->m_members.begin(); i != group->m_members.end(); i++)
			{
				IP2PGroupMember* peer = i->GetSecond();
				assert(peer->GetHostID() != HostID_Server);
				if (peer != NULL)
				{
					count++;
					//NTTNTRACE("%3.31f indirectServerTimediff\n", double(peer->GetIndirectServerTimeDiff()) / 1000);
					diffSum += peer->GetIndirectServerTimeDiff();
				}
			}

			int64_t groupServerTimeDiff = diffSum / ((int64_t)count);
			int64_t clientTime = GetPreciseCurrentTimeMs();

			return clientTime - groupServerTimeDiff;
		}
		else
		{
			return GetServerTimeMs();
		}
	}

	// 모든 그룹을 뒤져서 local과 제거하려는 remote가 모두 들어있는 p2p group이 하나도 없으면
	// P2P 연결 해제를 한다.
	void CNetClientImpl::RemoveRemotePeerIfNoGroupRelationDetected(CRemotePeer_C* memberRC)
	{
		LockMain_AssertIsLockedByCurrentThread();

		for (P2PGroups_C::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); i++)
		{
			CP2PGroupPtr_C g = i->GetSecond();
			for (P2PGroupMembers_C::iterator j = g->m_members.begin(); j != g->m_members.end(); j++)
			{
				IP2PGroupMember* p = j->GetSecond();
				if (p == memberRC)
					return;
			}
		}

		// 아래에서 없애긴 한다만 아직 다른 곳에서 참조할 수도 있으니 이렇게 dispose 유사 처리를 해야 한다.
		// (없어도 될 것 같긴 하다. 왜냐하면 m_remotePeers.Remove를 한 마당인데 p2p 통신이 불가능하니까.)
		//memberRC->SetRelayedP2P(true);여기서 하면 소켓 재활용이 되지 않는다.

		// repunch 예비 불필요
		// 상대측에도 P2P 직빵 연결이 끊어졌으니 relay mode로 전환하라고 알려야 한다.
		// => 불필요. 상대 B 입장에서도 이 호스트 A가 사라지기 때문에 이런거 안보내도 된다.
		//m_c2sProxy.P2P_NotifyDirectP2PDisconnected(HostID_Server, g_ReliableSendForPN, memberRC->m_HostID, ErrorType_NoP2PGroupRelation);

		//EnqueFallbackP2PToRelayEvent(memberRC->m_HostID, ErrorType_NoP2PGroupRelation); P2P 연결 자체를 로직컬하게도 없애는건데, 이 이벤트를 보낼 필요는 없다.

		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			Log(0, LogCategory_P2P, 
				String::NewFormat(_PNT("Client %d: P2P peer %d finally left."),
				GetVolatileLocalHostID(), memberRC->m_HostID));
		}

		// 비활성 상태가 될 뿐 garbage되지는 않는다.
		assert(memberRC->m_HostID > HostID_Server);
		RemotePeerRecycles_Add(memberRC);
		m_authedHostMap.RemoveKey(memberRC->m_HostID);
	}

	void CNetClientImpl::GetGroupMembers(HostID groupHostID, HostIDArray &output)
	{
		// LockMain_AssertIsLockedByCurrentThread();왜 이게 있었지?
		output.Clear();

		CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_C g = GetP2PGroupByHostID_Internal(groupHostID);
		if (g != NULL)
		{
			for (P2PGroupMembers_C::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
			{
				HostID mnum = i->GetFirst();
				output.Add(mnum);
			}
		}
	}

	void CNetClientImpl::HlaFrameMove()
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl != NULL)
			return m_hlaSessionHostImpl->HlaFrameMove();
		else
			throw Exception(HlaUninitErrorText);
#endif
	}

	void CNetClientImpl::TEST_FallbackUdpToTcp(FallbackMethod mode)
	{
		if (!HasServerConnection())
			return;

		CriticalSectionLock clk(GetCriticalSection(), true);

		switch (mode)
		{
		case FallbackMethod_CloseUdpSocket:
			LockMain_AssertIsLockedByCurrentThread();
			if (m_remoteServer->m_ToServerUdp != NULL)
			{
				m_remoteServer->m_ToServerUdp->CloseSocketOnly();
			}

			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());
				// 강제로 소켓을 다 닫아버린다. 즉 UDP 소켓이 맛간 상태를 만들어버린다.
				if (peer != NULL && peer->m_udpSocket != NULL)
					peer->m_udpSocket->CloseSocketOnly();
				/* Fallback이 이루어짐을 테스트해야 하므로
				FirstChanceFallbackEveryPeerUdpToTcp,FirstChanceFallbackServerUdpToTcp 즐 */
			}
			break;
		case FallbackMethod_ServerUdpToTcp:
			FirstChanceFallbackServerUdpToTcp(true, ErrorType_UserRequested);
			break;
		case FallbackMethod_PeersUdpToTcp:
			FirstChanceFallbackEveryPeerUdpToTcp(true, ErrorType_UserRequested);
			break;
		default:
			break;
		}
	}

	void CNetClientImpl::TEST_FakeTurnOffTcpReceive()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// ACR 도 안켜고 이 함수 호출 하면 "미정의 작동" 이 발생할 수 있으므로
		// 여기서 Exception 을 던져 에러를 노출 시킨다.
		if (m_connectionParam.m_enableAutoConnectionRecovery == false)
		{
			throw Exception();
		}

		// shutdown TCP를 하면 shutdown event가 발생해버려 선 뽑는 흉내가 안됨. 따라서 아예 tcp 수신 자체를 무시하게 해야 그게 가능.
		if (m_remoteServer->m_ToServerTcp != NULL)
		{
			m_remoteServer->m_ToServerTcp->m_turnOffSendAndReceive = true;
		}
	}

	void CNetClientImpl::GarbageAllHosts()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CNetCoreImpl::GarbageAllHosts(); // base call

		for (HostIDToRemotePeerMap::iterator i = m_remotePeerRecycles.begin(); i != m_remotePeerRecycles.end(); i++)
		{
			CRemotePeer_C* r = i->GetSecond();
			GarbageHost(r, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
				ByteArray(), _PNT("G-ALL"), SocketErrorCode_Ok);
		}
	}

	bool CNetClientImpl::CanDeleteNow()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		return m_remotePeerRecycles.IsEmpty()
			&& m_autoConnectionRecoveryContext == NULL
			&& CNetCoreImpl::CanDeleteNow();
	}

	void CNetClientImpl::TEST_SetPacketTruncatePercent(Proud::HostType hostType, int percent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerUdp && (hostType == HostType_All || hostType == HostType_Server))
		{
			m_remoteServer->m_ToServerUdp->m_packetTruncatePercent = percent;
		}

		if (hostType == HostType_All || hostType == HostType_Peer)
		{
			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());

				if (peer && peer->m_udpSocket)
				{
					peer->m_udpSocket->m_packetTruncatePercent = percent;
				}
			}
		}
	}

	int CNetClientImpl::GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (peerHostID == HostID_Server)
		{
			if (error != NULL)
			{
				if (m_serverUdpPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return m_serverUdpLastPingMs;
		}

		CRemotePeer_C* p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != NULL)
		{
			if (error != NULL)
			{
				if (p->m_p2pPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			return p->m_lastPingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int cnt = 0;
			int  total = 0;
			for (P2PGroupMembers_C::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int ping = GetLastUnreliablePingMs(iMember->GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != NULL)
					*error = ErrorType_Ok;

				return total / cnt;
			}
		}

		if (error != NULL)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}

	int  CNetClientImpl::GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (peerHostID == HostID_Server)
		{
			if (error != NULL)
			{
				if (m_serverUdpPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return m_serverUdpRecentPingMs;
		}

		CRemotePeer_C* p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != NULL)
		{
			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			if (error != NULL)
			{
				if (p->m_p2pPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return p->m_recentPingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int  cnt = 0;
			int  total = 0;
			for (P2PGroupMembers_C::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int  ping = GetRecentUnreliablePingMs(iMember->GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != NULL)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}
		if (error != NULL)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}

	Proud::AddrPort CNetClientImpl::GetServerAddrPort()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		if (m_remoteServer->m_ToServerTcp == NULL)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->GetLocalAddr();
	}

	void CNetClientImpl::TcpAndUdp_DoForLongInterval()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		int64_t currTime = GetPreciseCurrentTimeMs();
		uint32_t queueLength = 0;

		if (m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.IsTimeToDo(currTime))
		{
			if (m_remoteServer->m_ToServerUdp != NULL)
			{
				m_remoteServer->m_ToServerUdp->DoForLongInterval(currTime, queueLength);

				m_sendQueueUdpTotalBytes = 0;

				// per-peer UDP socket에 대해서도 처리하기.
				for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
				{
					CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());

					if (peer)
					{
						if (peer->m_garbaged)
						{
							continue;
						}

						if (peer->m_udpSocket)
						{
							peer->m_udpSocket->DoForLongInterval(currTime, queueLength);

							m_sendQueueUdpTotalBytes += queueLength;

							// 송신량 측정 결과도 여기다 업뎃한다.
							peer->m_sendQueuedAmountInBytes = peer->m_ToPeerUdp.GetUdpSendBufferPacketFilledCount();

							peer->m_udpSocket->SetCoalesceInteraval(peer->m_UdpAddrFromServer,
								peer->m_coalesceIntervalMs);
						}
					}
				}
			}

			// TCP에 대해서도 처리하기
			if (m_remoteServer->m_ToServerTcp)
			{
				m_remoteServer->m_ToServerTcp->DoForLongInterval(currTime, queueLength);

				m_sendQueueTcpTotalBytes = queueLength;
			}
		}

		if (currTime - m_TcpAndUdp_DoForShortInterval_lastTime > m_ReliableUdpHeartbeatInterval_USE)
		{
			if (m_remoteServer->m_ToServerUdp != NULL)
			{
				m_remoteServer->m_ToServerUdp->DoForShortInterval(currTime);
				//m_remoteServer->m_ToServerUdp->DoForShortInterval(currTime);

#if defined(_WIN32)
				// addrportqueueMapCount 갱신
				if (m_settings.m_emergencyLogLineCount > 0)
				{
					m_emergencyLogData.m_serverUdpAddrCount = (uint32_t)m_remoteServer->m_ToServerUdp->GetUdpSendQueueDestAddrCount();
				}
#endif
			}

			// per-peer UDP socket에 대해서도 처리하기.
			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());

				if (peer)
				{
					if (peer->m_garbaged)
					{
						continue;
					}

					if (peer->m_udpSocket)
					{
						peer->m_udpSocket->DoForShortInterval(currTime);
						//peer->m_udpSocket->DoForShortInterval(currTime);

#if defined(_WIN32)
						// addrportqueueMapCount 갱신
						if (m_settings.m_emergencyLogLineCount > 0)
							m_emergencyLogData.m_serverUdpAddrCount = (uint32_t)peer->m_udpSocket->GetUdpSendQueueDestAddrCount();
#endif
					}
				}
			}

			RemotePeerRecycles_GarbageTooOld();

			m_TcpAndUdp_DoForShortInterval_lastTime = currTime;
		}
	}

	void CNetClientImpl::InduceDisconnect()
	{
		if (m_remoteServer->m_ToServerTcp != NULL)
		{
			m_remoteServer->m_ToServerTcp->CloseSocketOnly();
			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
				Log(0, LogCategory_Udp, _PNT("InduceDisconnect, CloseSocketOnly called."));
		}
	}

	void CNetClientImpl::ConvertGroupToIndividualsAndUnion(int numberOfsendTo, const HostID * sendTo, HostIDArray& output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		POOLED_ARRAY_LOCAL_VAR(ISendDestArray, o1);
		
		ConvertGroupToIndividualsAndUnion(numberOfsendTo, sendTo, o1);
		output.SetCount(o1.GetCount());
		for (int i = 0; i < (int)o1.GetCount(); i++)
		{
			CHostBase* o1_i = o1[i];
			if (o1_i != NULL)
				output[i] = o1_i->GetHostID();
			else
				output[i] = HostID_None;
		}
	}

	// P2P group HostID를 개별 HostID로 바꾼 후 중복되는 것들을 병합한다.
	void CNetClientImpl::ConvertGroupToIndividualsAndUnion(int numberOfsendTo, const HostID * sendTo, ISendDestArray &sendDestList)
	{
		for (int i = 0; i < numberOfsendTo; ++i)
		{
			if (HostID_None != sendTo[i])
			{
				ConvertAndAppendP2PGroupToPeerList(sendTo[i], sendDestList);
			}
		}

		UnionDuplicates<ISendDestArray, CHostBase*, int>(sendDestList);
	}

	// 	void CNetClientImpl::Send_ToServer_Directly_Copy(HostID destHostID,MessageReliability reliability, const CSendFragRefs& sendData2, const SendOpt& sendOpt )
	// 	{
	// 		// send_core to server via UDP or TCP
	// 		if (reliability == MessageReliability_Reliable)
	// 		{
	// 			Get_ToServerTcp()->AddToSendQueueWithSplitterAndSignal_Copy(sendData2, sendOpt, m_simplePacketMode);
	// 		}
	// 		else
	// 		{
	// 			// JIT UDP trigger하기
	// 			RequestServerUdpSocketReady_FirstTimeOnly();
	//
	// 			// unrealiable이 되기전까지 TCP로 통신
	// 			// unreliable일때만 uniqueid를 사용하므로...
	// 			m_ToServerUdp_fallbackable->SendWithSplitterViaUdpOrTcp_Copy(destHostID,sendData2,sendOpt);
	// 		}
	// 	}

	/* 1개의 메시지를 0개 이상의 상대에게 전송한다.
	메시지로 보내는 것의 의미는, 받는 쪽에서도 메시지 단위로 수신해야 함을 의미한다.
	msg: 보내려고 하는 메시지이다. 이 함수의 caller에 의해 header 등이 붙어버린 상태이므로, 메모리 복사 비용을 줄이기 위해 fragment list로서 받는다.
	sendContext: reliable로 보낼 것인지, unique ID를 붙일 것인지 등.
	sendTo, numberOfSendTo: 수신자들
	compressedPayloadLength: 보낸 메시지가 내부적으로 압축됐을 경우 압축된 크기.
	*/
	bool CNetClientImpl::Send(const CSendFragRefs &msg,
		const SendOpt& sendContext,
		const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength)
	{
		/* 네트워킹 비활성 상태이면 무조건 그냥 아웃.
		여기서 사전 검사를 하는 이유는, 아래 하위 callee들은 많은 validation check를 하는데, 그걸
		다 거친 후 안보내는 것 보다는 앗싸리 여기서 먼저 안보내는 것이 성능상 장점이니까.*/
		if (m_worker == NULL)
			return false;

		// 설정된 한계보다 큰 메시지인 경우
		if (msg.GetTotalLength() > m_settings.m_clientMessageMaxLength)
			throw Exception("Too long message cannot be sent. (Size=%d)", msg.GetTotalLength());

		// 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
		// 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
		return Send_CompressLayer(msg,
			sendContext,
			sendTo, numberOfsendTo, compressedPayloadLength);
	}

	bool CNetClientImpl::NextEncryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//modify by rekfkno1 - 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
		if (m_remoteServer->m_ToServerTcp == NULL || HostID_None == GetVolatileLocalHostID())
		{
			return false;
		}

		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			output = rp->m_encryptCount;
			rp->m_encryptCount++;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			output = m_selfEncryptCount;
			m_selfEncryptCount++;
			return true;
		}
		else if (remote == HostID_Server)
		{
			output = m_toServerEncryptCount;
			m_toServerEncryptCount++;
			return true;
		}
		return false;
	}

	void CNetClientImpl::PrevEncryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//modify by rekfkno1 - 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
		if (m_remoteServer->m_ToServerTcp == NULL || HostID_None == GetVolatileLocalHostID())
		{
			return;
		}

		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			rp->m_encryptCount--;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			m_selfEncryptCount--;
		}
		else if (remote == HostID_Server)
		{
			m_toServerEncryptCount--;
		}
	}

	bool CNetClientImpl::GetExpectedDecryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			output = rp->m_decryptCount;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			output = m_selfDecryptCount;
			return true;
		}
		else if (remote == HostID_Server)
		{
			output = m_toServerDecryptCount;
			return true;
		}
		return false;
	}

	bool CNetClientImpl::NextDecryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			rp->m_decryptCount++;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			m_selfDecryptCount++;
			return true;
		}
		else if (remote == HostID_Server)
		{
			m_toServerDecryptCount++;
			return true;
		}
		return false;
	}

	void CNetClientImpl::Log(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction /*= _PNT("")*/, int logLine /*= 0*/)
	{
		if (m_enableLog && GetVolatileLocalHostID() != HostID_None)
		{
			m_c2sProxy.NotifyLog(HostID_Server, g_ReliableSendForPN, logLevel, logCategory, GetVolatileLocalHostID(), logMessage, logFunction, logLine);
		}

#if defined(_WIN32)        
		if (m_settings.m_emergencyLogLineCount > 0)
		{
			AddEmergencyLogList(logLevel, logCategory, logMessage, logFunction, logLine);
		}
#endif
	}

	void CNetClientImpl::LogHolepunchFreqFail(int rank, const PNTCHAR* format, ...)
	{
		String s;

		va_list	pArg;
		va_start(pArg, format);
		s.FormatV(format, pArg);
		va_end(pArg);

		if (m_enableLog)
		{
			m_c2sProxy.NotifyLogHolepunchFreqFail(HostID_Server, g_ReliableSendForPN, rank, s);
		}

#if defined(_WIN32)        
		if (m_settings.m_emergencyLogLineCount > 0)
		{
			//AddEmergencyLogList(TID_HolepunchFreqFail, s);
			//AddEmergencyLogList(LogCategory_P2P, s);

			//###  // LogHolepunchFreqFail 메소드의 매개변수도 바뀌어야 하는지?
			AddEmergencyLogList(1, LogCategory_P2P, _PNT(""));
		}
#endif
	}

	// 클라-서버간 UDP가 receive timeout이 발생했을 때를 처리한다.
	// TCP fallback을 수행한다.
	// notifyToServer=true이면 서버에게 노티 RMI를 보낸다.
	// 만약 서버로부터 TCP fallback 을 명령 받은 경우에는 또 보내는 삽질을 하면 안되므로 false로 할 것.
	void CNetClientImpl::FirstChanceFallbackServerUdpToTcp(bool notifyToServer, ErrorType reasonToShow)
	{
		if (m_remoteServer->FirstChanceFallbackServerUdpToTcp(reasonToShow))
		{
			if (notifyToServer)
				m_c2sProxy.NotifyUdpToTcpFallbackByClient(HostID_Server, g_ReliableSendForPN);
		}
	}

	void CNetClientImpl::TEST_EnableVirtualSpeedHack(int64_t multipliedSpeed)
	{
		if (multipliedSpeed <= 0)
			throw Exception("Invalid parameter!");

		m_virtualSpeedHackMultiplication = multipliedSpeed;
	}

	Proud::HostID CNetClientImpl::GetVolatileLocalHostID() const
	{
		// 이 함수는 UDP packet frag/defrag board에서 filterTag를 검사하기 위해 callback되는 함수입니다. 
		// 그런데 UDP packet frag/defrag board는 super socket의 send queue lock에 의해 보호됩니다.
		// 따라서 여기서 main lock을 하면 deadlock이 발생합니다.
		if (!m_loopbackHost)
			return HostID_None;

		return m_loopbackHost->m_HostID;
	}

	Proud::HostID CNetClientImpl::GetLocalHostID()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		/* Q: main lock을 왜 안하나요?
		A: 사용자가 이 함수를 호출하는 경우, 어차피 사용자는 NetClient에 대한 main lock 액세스를 못합니다. 즉 이 함수를 호출해서 얻은 값은 volatile입니다.
		따라서 main lock이 무의미합니다. 따라서 이렇게 구별하였습니다. => 즐! 하위호환성 위해, 미증유 위해, lock 꼭 해야 함!!!
		*/
		return GetVolatileLocalHostID();
	}

	Proud::ConnectionState CNetClientImpl::GetServerConnectionState(CServerConnectionState &output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (m_remoteServer != NULL)
		{
			output.m_realUdpEnabled = m_remoteServer->IsRealUdpEnable();
		}

		if (m_worker != NULL)
		{
			switch (m_worker->GetState())
			{
			case CNetClientWorker::Disconnected:
				return ConnectionState_Disconnected;
			case CNetClientWorker::IssueConnect:
				return ConnectionState_Connecting;
			case CNetClientWorker::Connecting:
				return ConnectionState_Connecting;
			case CNetClientWorker::JustConnected:
				return ConnectionState_Connecting;
			case CNetClientWorker::Connected:
				return ConnectionState_Connected;
			case CNetClientWorker::Disconnecting:
				return ConnectionState_Disconnecting;
			}
		}

		return ConnectionState_Disconnected;

		// 		if(GetVolatileLocalHostID() != HostID_None)
		// 		{
		// 			output.m_realUdpEnabled = m_ToServerUdp_fallbackable != NULL ? m_ToServerUdp_fallbackable->IsRealUdpEnabled() : false;
		// 			return ConnectionState_Connected;
		// 		}
		// 		else if (m_ToServerTcp != NULL && Get_ToServerTcp()->m_socket != NULL && Get_ToServerTcp()->m_socket->IsSocketEmpty() == false)
		// 		{
		// 			return ConnectionState_Connecting;
		// 		}
		// 		else
		// 		{
		// 			return ConnectionState_Disconnected;
		// 		}
	}

	void CNetClientImpl::FrameMove(int maxWaitTimeMs, CFrameMoveResult *outResult /*= NULL*/)
	{
		// Connect,Disconnect,FrameMove는 병렬로 실행되지 않는다.
		// 그래야 2개 이상의 스레드에서 같은 NC에 대해 FrameMove를 병렬로 실행할 때
		// 이상한 현상 안 생기니까.
		CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);

		if (m_lastFrameMoveInvokedTime != -1)
		{
			m_lastFrameMoveInvokedTime = GetPreciseCurrentTimeMs(); // 어차피 이 값은 부정확해도 되므로.
		}

		// NetThreadPool 이 SingleThreaded 모델일 때 콜 쓰레드가 처리 하도록 작업 수행
		if (m_netThreadPool != NULL &&
			m_connectionParam.m_netWorkerThreadModel == ThreadModel_SingleThreaded)
		{
			// FrameMove의 매개변수 CFrameMoveResult는 하위호환성을 위해 그대로 유지하도록 하였습니다.
			CWorkResult processEventResult;

			m_netThreadPool->Process(&processEventResult, maxWaitTimeMs);

			// 				if (outResult != NULL)
			// 				{
			// 					outResult->m_processedEventCount = processEventResult.m_processedEventCount;
			// 					outResult->m_processedMessageCount = processEventResult.m_processedMessageCount;
			// 				}
		}

		// UserThreadPool 이 SingleThreaded 모델일 때 콜 쓰레드가 처리 하도록 작업 수행
		if (m_userThreadPool != NULL &&
			m_connectionParam.m_userWorkerThreadModel == ThreadModel_SingleThreaded)
		{
			// FrameMove의 매개변수 CFrameMoveResult는 하위호환성을 위해 그대로 유지하도록 하였습니다.
			CWorkResult processEventResult;

			// 본 변수는 LocalEvent, RMI에서 사용자가 Disconnect 함수의 호출을 막기 위하여 사용합니다.
			// TRUE로 설정된 이후에 Disconnect를 호출하면 Exception이 발생됩니다.
			m_isFrameMoveCalled = true;

			m_userThreadPool->Process(&processEventResult, maxWaitTimeMs);

			// 이전에 설정되었던 값을 FALSE로 변경합니다.
			m_isFrameMoveCalled = false;

			if (outResult != NULL)
			{
				outResult->m_processedEventCount = processEventResult.m_processedEventCount;
				outResult->m_processedMessageCount = processEventResult.m_processedMessageCount;
			}
		}
	}

	void CNetClientImpl::FrameMove(int maxWaitTime, CFrameMoveResult* outResult, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			FrameMove(maxWaitTime, outResult); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	AddrPort CNetClientImpl::GetLocalUdpSocketAddr(HostID remotePeerID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(remotePeerID);
		if (peer != NULL && peer->m_udpSocket)
		{
			return peer->m_udpSocket->GetLocalAddr();
		}
		return AddrPort::Unassigned;
	}

	bool CNetClientImpl::GetPeerReliableUdpStats(HostID peerID, ReliableUdpHostStats &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHostBase* hostBase = NULL;

		m_authedHostMap.TryGetValue(peerID, hostBase);

		CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(hostBase);
		if (remotePeer && remotePeer->m_ToPeerReliableUdp.m_host)
		{
			remotePeer->m_ToPeerReliableUdp.m_host->GetStats(output);
			return true;
		}

		return false;
	}

	void CNetClientImpl::EnqueFallbackP2PToRelayEvent(HostID remotePeerID, ErrorType reason)
	{
		LocalEvent e;
		e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
		e.m_type = LocalEventType_RelayP2PEnabled;
		e.m_errorInfo->m_errorType = reason;
		e.m_remoteHostID = remotePeerID;
		EnqueLocalEvent(e, GetPeerByHostID_NOLOCK(remotePeerID));
		//EnqueLocalEvent(e, m_loopbackHost);
	}

	void CNetClientImpl::GetStats(CNetClientStats &outVal)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CriticalSectionLock statsLock(m_statsCritSec, true);
		outVal = m_stats;

		// 통계 정보는 좀 부정확해도 된다.
		// 하지만 P2P 카운트 정보는 정확해야 한다.
		// 그래서 여기서 즉시 계산한다.

		if (m_remoteServer)
			outVal.m_serverUdpEnabled = m_remoteServer->GetToServerUdpFallbackable() ? m_remoteServer->GetToServerUdpFallbackable()->IsRealUdpEnabled() : false;

		m_stats.m_directP2PEnabledPeerCount = 0;
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());
			if (peer && !peer->IsRelayedP2P())
				m_stats.m_directP2PEnabledPeerCount++;
		}

		outVal.m_sendQueueTcpTotalBytes = m_sendQueueTcpTotalBytes;
		outVal.m_sendQueueUdpTotalBytes = m_sendQueueUdpTotalBytes;
		outVal.m_sendQueueTotalBytes = outVal.m_sendQueueTcpTotalBytes + outVal.m_sendQueueUdpTotalBytes;
	}

	// 릴레이 메시지에 들어갈 '릴레이 최종 수신자들' 명단을 만든다.
	void CNetClientImpl::RelayDestList_C::ToSerializable(RelayDestList &ret)
	{
		ret.Clear();
		for (int i0 = 0; i0 < (int)GetCount(); i0++)
		{
			RelayDest_C &i = (*this)[i0];
			RelayDest r;
			r.m_frameNumber = i.m_frameNumber;
			r.m_sendTo = i.m_remotePeer->m_HostID;
			ret.Add(r);
		}
	}

	// 	void CNetClient::UseNetworkerThread_EveryInstance(bool enable)
	// 	{
	// 		CNetClientManager& man = CNetClientManager::Instance();
	// 		man.m_useNetworkerThread = enable;
	// 	}
	// 
	// 	void CNetClient::NetworkerThreadHeartbeat_EveryInstance()
	// 	{
	// 		CNetClientManager& man = CNetClientManager::Instance();
	// 		// 이 함수는 대기가 없어야 한다. 따라서 timeout 값으로 0을 넣는다.
	// 		man.m_netWorkerThread->Process(0);
	// 	}

	CNetClient* CNetClient::Create()
	{
		return new CNetClientImpl();
	}

	// 모든 peer들과의 UDP를 relay화한다.
	// firstChance=true이면 서버에게도 RMI로 신고한다.
	void CNetClientImpl::FirstChanceFallbackEveryPeerUdpToTcp(bool notifyToServer, ErrorType reason)
	{
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(i->GetSecond());
			if (peer != NULL)
			{
				peer->FallbackP2PToRelay(notifyToServer, reason);
			}
		}
	}

	bool CNetClientImpl::RestoreUdpSocket(HostID peerID)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(peerID);
		if (peer && peer->m_udpSocket)
		{
			peer->m_udpSocket->m_turnOffSendAndReceive = false;
			return true;
		}

		return false;
	}

	DEFRMI_ProudS2C_NewDirectP2PConnection(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		CRemotePeer_C* peer = m_owner->GetPeerByHostID_NOLOCK(remotePeerID);
		if (peer)
		{
			if (peer->m_udpSocket == NULL)
			{
				peer->m_newP2PConnectionNeeded = true;

				if (m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0)
					m_owner->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Request p2p connection to Client %d."), peer->m_HostID));
			}
		}

		return true;
	}

	// 핑 측정되면 값들을 갱신
	void CNetClientImpl::ServerTcpPing_UpdateValues(int newLag)
	{
		m_serverTcpLastPingMs = max(newLag, 1); // 핑이 측정됐으나 0이 나오는 경우 아직 안 측정됐다는 오류를 범하지 않기 위함.
		m_serverTcpPingChecked = true;

		if (m_serverTcpRecentPingMs == 0)
			m_serverTcpRecentPingMs = m_serverTcpLastPingMs;
		else
			m_serverTcpRecentPingMs = LerpInt(m_serverTcpRecentPingMs, m_serverTcpLastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
	}

	bool CNetClientImpl::GetDirectP2PInfo(HostID peerID, CDirectP2PInfo &outInfo)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (peerID == HostID_Server)
		{
			return false;
		}

		CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(peerID);
		if (peer)
		{
			if (!peer->m_forceRelayP2P)
				peer->m_jitDirectP2PNeeded = true; // 이 메서드를 호출한 이상 JIT P2P를 켠다.

			peer->GetDirectP2PInfo(outInfo);
			return outInfo.HasBeenHolepunched();
		}

		return false;
	}

	// header 내 주석 참고
	bool CNetClientImpl::InvalidateUdpSocket(HostID peerID, CDirectP2PInfo &outDirectP2PInfo)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (peerID == HostID_Server)
		{
			return false;
		}

		CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(peerID);
		if (peer != NULL)
		{
			peer->GetDirectP2PInfo(outDirectP2PInfo);
			bool ret = outDirectP2PInfo.HasBeenHolepunched();

			if (peer->m_udpSocket != NULL && !peer->m_udpSocket->GetSocket()->IsClosed())
			{
				// 과거에는 UDP socket을 진짜로 닫았지만, 
				// 이는 홀펀칭 증발과 동일한 상황도 아니며,
				// 게다가 UDP socket을 지운 후 다시 만드는 경우 포트 재사용을 못하는 일부 OS때문에
				// 정확한 테스트가 안된다. 따라서 아래와 같이 통신을 차단만 한다.
				peer->m_udpSocket->m_turnOffSendAndReceive = true;

				// 홀펀칭 진행중이던 것도 중단해야.
				peer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

				// Fallback이 바로 일어나야 하므로
				peer->FallbackP2PToRelay(true, ErrorType_UserRequested);
			}

			return ret;
		}

		return false;
	}

	// 마지막으로 UDP 데이터를 받은 시간을 갱신한다. UDP 통신 불량을 감지하는 용도 등.
	// 	void CNetClientImpl::LogLastServerUdpPacketReceived()
	// 	{
	// 		if (m_ToServerUdp_fallbackable != NULL)
	// 		{
	// 			int64_t currTime = GetPreciseCurrentTimeMs();
	//
	// 			// pong 체크를 했다고 처리하도록 하자.
	// 			// 이게 없으면 대량 통신시 pong 수신 지연으로 인한 튕김이 발생하니까.
	// 			int64_t oktime = currTime - m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedTimeMs;
	//
	// 			if (oktime > 0)
	// 				m_ToServerUdp_fallbackable->m_lastUdpPacketReceivedIntervalMs = oktime;
	// 
	// 			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedTimeMs = currTime;
	// 			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedCount++;
	// 
	// #ifdef UDP_PACKET_RECEIVE_LOG
	// 			m_ToServerUdp_fallbackable->m_lastServerUdpPacketReceivedQueue.Add(m_lastServerUdpPacketReceivedTime);
	// #endif // UDP_PACKET_RECEIVE_LOG
	// 		}
	// 	}

	bool CNetClientImpl::IsLocalHostBehindNat(bool &output)
	{
		if (!HasServerConnection())
			return false;

		output = (Get_ToServerUdpSocketLocalAddr() != Get_ToServerUdpSocketAddrAtServer());
		return true;
	}

	// 디버깅 용도
	Proud::String CNetClientImpl::GetTrafficStatText()
	{
		CNetClientStats ss;
		GetStats(ss);

		String ret;
		ret.Format(_PNT("TotalSend=%I64d TotalRecv=%I64d PeerCount=%d/%d NAT Name=%s"),
			ss.m_totalUdpSendBytes,
			ss.m_totalUdpReceiveBytes,
			ss.m_directP2PEnabledPeerCount,
			ss.m_remotePeerCount,
			GetNatDeviceName().GetString());

		return ret;
	}

	Proud::String CNetClientImpl::TEST_GetDebugText()
	{
		return String();
		// 		String ret;
		// 		ret.Format(_PNT("%s %s"),m_localUdpSocketAddr.ToString(),m_localUdpSocketAddrFromServer.ToString());
		// 		return ret;
	}

	Proud::AddrPort CNetClientImpl::Get_ToServerUdpSocketLocalAddr()
	{
		return m_remoteServer->Get_ToServerUdpSocketLocalAddr();
	}

	AddrPort CNetClientImpl::Get_ToServerUdpSocketAddrAtServer()
	{
		// 서버에서 인식한 이 클라의 UDP 소켓 주소
		if (m_remoteServer->m_ToServerUdp == NULL)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerUdp->m_localAddrAtServer;
	}

	// 	void CNetClientImpl::WaitForGarbageCleanup()
	// 	{
	// 		assert(GetCriticalSection().IsLockedByCurrentThread()==false);
	// 
	// 		// per-peer UDP socket들의 completion도 모두 끝났음을 보장한다.
	// 		for(int i=0;i<1000;i++)
	// 		{
	// 			{
	// 				CriticalSectionLock lock(GetCriticalSection(),true);
	// 				if(m_garbages.GetCount()==0)
	// 					return;
	// 				DoGarbageCollect();
	// 			}
	// 			Sleep(10);
	// 		}
	// 	}

	void CNetClientImpl::OnHostGarbaged(CHostBase* remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode)
	{
		LockMain_AssertIsLockedByCurrentThread();

		if (remote == m_loopbackHost)
		{
			// NOTE: LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
			return;
		}
		else if (remote == m_remoteServer)
		{
			CSuperSocketPtr toServerTcp = m_remoteServer->m_ToServerTcp;
			assert(toServerTcp != NULL);

			// 서버와 마찬가지로, OnHostGarbaged가 아니라 OnHostGarbageCollected에서
			// GarbageSocket and 참조 해제를 해야 합니다. 수정 바랍니다.
			// 단, socket to host map에서의 제거는 OnHostGarbaged에서 하는 것이 맞습니다.
			SocketToHostsMap_RemoveForAnyAddr(toServerTcp);

			CSuperSocketPtr toServerUdp = m_remoteServer->m_ToServerUdp;
			if (toServerUdp != NULL)
			{
				SocketToHostsMap_RemoveForAnyAddr(toServerUdp);
			}
		}
		else
		{
			CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(remote);

			// ACR 처리가 들어가면서 해당 assert 구문을 없앴다.
			//assert(remotePeer != NULL);

			// P2P peer를 garbage 처리한다.
			// OnP2PMemberLeave가 호출되기 전까지는 remote peer가 살아야 한다.
			// remote peer용 UDP socket은 garbages에 들어가면서 remote peer와의 관계 끊어짐.
			if (remotePeer != NULL && remotePeer->m_garbaged == false)
			{
				remotePeer->m_garbaged = true;

				if (remotePeer->m_owner == this)
				{
					CSuperSocketPtr udpSocket = remotePeer->m_udpSocket;
					if (udpSocket != NULL)
					{
						// 일관성을 위해, 여기서도 위에처럼 OnHostGarbageCollected에서 remote peer의 UDP socket을 garbageSocket합시다.
						SocketToHostsMap_RemoveForAnyAddr(udpSocket);
						udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(remotePeer->m_UdpAddrFromServer);
					}
				}
			}
		}

		// 재접속 연결 후보에 매치된 temp remote server이면
		if (m_autoConnectionRecovery_temporaryRemoteServers.ContainsKey(remote))
		{
			// SocketToHostsMap 에서 삭제
			CRemoteServer_C* remoteServer = (CRemoteServer_C*)remote;
			SocketToHostsMap_RemoveForAnyAddr(remoteServer->m_ToServerTcp);
		}
	}

	Proud::String CNetClientImpl::GetNatDeviceName()
	{
#if defined(_WIN32)
		if (m_manager->m_upnp)
			return m_manager->m_upnp->GetNatDeviceName();
		else
#endif
			return String();
	}

	void CNetClientImpl::OnSocketWarning(CFastSocket* /*socket*/, String msg)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			Log(0, LogCategory_System, String::NewFormat(_PNT("Socket Warning: %s"), msg.GetString()));
		}
	}

	// #if !defined(_WIN32)
	//
	// 	// edge trigger 방식. 그래서, socket 內 send buffer량이 변화가 없으면 아예 안보냄.
	// 	// 따라서 이게 있어야 함.
	//     void CNetClientImpl::EveryRemote_NonBlockedSendOnNeed(CIoEventStatus &comp)
	// 	{
	// 		CriticalSectionLock lock(GetCriticalSection(),true);
	//
	// 		// 서버 TCP
	// 		if(Get_ToServerTcp())
	// 		{
	// 			Get_ToServerTcp()->NonBlockSendUntilWouldBlock(comp);
	// 		}
	//
	// 		// 서버 UDP
	// 		if(Get_ToServerUdpSocket())
	// 		{
	// 			Get_ToServerUdpSocket()->NonBlockSendUntilWouldBlock(comp);
	// 		}
	//
	// 		// 피어 UDP
	// 		for (RemotePeers_C::iterator i = m_remotePeers.begin();i != m_remotePeers.end();i++)
	// 		{
	// 			CRemotePeerPtr_C rp = i->GetSecond();
	//
	// 			if(rp->m_garbaged)
	// 				continue;
	//
	// 			if(rp->m_udpSocket)
	// 			{
	// 				rp->Get_ToPeerUdpSocket()->NonBlockSendUntilWouldBlock(comp);
	// 			}
	// 		}
	// 	}
	// #else
	//  이 함수를 CNetCoreImpl로 옮기자.
	// 	void CNetClientImpl::EveryRemote_IssueSendOnNeed()
	// 	{
	// 		CriticalSectionLock lock(GetCriticalSection(),true);
	//
	// 		// 서버 TCP
	// 		if(Get_ToServerTcp())
	// 		{
	// 			Get_ToServerTcp()->IssueSendOnNeed();
	//
	// 			m_ioPendingCount = Get_ToServerTcp()->m_socket->m_ioPendingCount;
	// 			m_totalTcpIssueSendBytes = Get_ToServerTcp()->m_totalIssueSendBytes;
	// 		}
	//
	// 			// 서버 UDP
	// 		if(Get_ToServerUdpSocket())
	// 			{
	// 			Get_ToServerUdpSocket()->IssueSendOnNeed_IfPossible(false);
	// 			}
	//
	// 			// 피어 UDP
	// 			for (RemotePeers_C::iterator i = m_remotePeers.begin();i != m_remotePeers.end();i++)
	// 			{
	// 			CRemotePeerPtr_C rp = i->GetSecond();
	//
	// 				if(rp->m_garbaged)
	// 					continue;
	//
	// 			if(rp->m_udpSocket)
	// 			{
	// 				rp->Get_ToPeerUdpSocket()->IssueSendOnNeed_IfPossible(false);
	// 			}
	// 		}
	// 	}
	//#endif

	// NetCore 에서 요구하는 것을 구현
	CriticalSection& CNetClientImpl::GetCriticalSection()
	{
		return m_critSec;
	}

	int CNetClientImpl::GetInternalVersion()
	{
		return m_internalVersion;
	}

	void CNetClientImpl::SetEventSink(INetClientEvent *eventSink)
	{
		if (AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);
		LockMain_AssertIsNotLockedByCurrentThread();
		m_eventSink_NOCSLOCK = eventSink;
	}
	
	void CNetClientImpl::SetEventSink(INetClientEvent *eventSink, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			SetEventSink(eventSink);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	INetClientEvent* CNetClientImpl::GetEventSink()
	{
		return (INetClientEvent*)m_eventSink_NOCSLOCK;
	}

	void CNetClientImpl::EnqueError(ErrorInfoPtr info)
	{
		LocalEvent e;
		e.m_type = LocalEventType_Error;
		e.m_errorInfo = info;
		e.m_remoteHostID = info->m_remote;
		e.m_remoteAddr = info->m_remoteAddr;

		EnqueLocalEvent(e, m_loopbackHost);
	}

	void CNetClientImpl::EnqueWarning(ErrorInfoPtr info)
	{
		LocalEvent e;
		e.m_type = LocalEventType_Warning;
		e.m_errorInfo = info;
		e.m_remoteHostID = info->m_remote;
		e.m_remoteAddr = info->m_remoteAddr;

		EnqueLocalEvent(e, m_loopbackHost);
	}

	bool CNetClientImpl::AsyncCallbackMayOccur()
	{
		if (m_worker)
			return true;
		else
			return false;
	}

	int CNetClientImpl::GetMessageMaxLength()
	{
		return m_settings.m_clientMessageMaxLength;
	}

	void CNetClientImpl::EnquePacketDefragWarning(CSuperSocket* superSocket, AddrPort addrPort, const PNTCHAR* text)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		CRemotePeer_C* rc = GetPeerByUdpAddr(addrPort, false);

		if (CNetConfig::EnablePacketDefragWarning)
		{
			EnqueWarning(ErrorInfo::From(ErrorType_InvalidPacketFormat, rc ? rc->m_HostID : HostID_None, text));
		}
	}

	Proud::AddrPort CNetClientImpl::GetPublicAddress()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == NULL)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->m_localAddrAtServer;
	}

	void CNetClientImpl::ReportP2PPeerPingOnNeed()
	{
		if (m_settings.m_enablePingTest
			&& GetPreciseCurrentTimeMs() - m_enablePingTestEndTime > CNetConfig::ReportP2PPeerPingTestIntervalMs)
		{
			for (AuthedHostMap::iterator iter = m_authedHostMap.begin(); iter != m_authedHostMap.end(); iter++)
			{
				m_enablePingTestEndTime = GetPreciseCurrentTimeMs();

				HostID RemoteID = iter->GetFirst();
				if (GetVolatileLocalHostID() < RemoteID)
				{
					CRemotePeer_C* rp = LeanDynamicCastForRemotePeer(iter->GetSecond());

					if (rp != NULL)
					{
						if (!rp->m_garbaged && !rp->IsRelayedP2P())
						{
							// P2P가 릴레이 서버를 타는 경우보다 느린 경우에는 릴레이 서버를 타게끔 되어 있다.
							// 따라서 서버가 알아야 하는 레이턴시는 릴레이할때의 합산된다.
							// (=Remote Peer와 서버의 핑과 자신과 서버의 핑을 더함)
							int ping;
							if (rp->m_recentPingMs > 0
								&& rp->m_peerToServerPingMs > 0
								&& m_serverUdpRecentPingMs + rp->m_peerToServerPingMs < rp->m_recentPingMs)
							{
								ping = m_serverUdpRecentPingMs + rp->m_peerToServerPingMs;
							}
							else
								ping = rp->m_recentPingMs;

							m_c2sProxy.ReportP2PPeerPing(
								HostID_Server,
								g_ReliableSendForPN,
								rp->m_HostID,
								ping);
						}
					}
				}
			}
		}
	}

	void CNetClientImpl::ReportRealUdpCount()
	{
		// 기능사용이 꺼져있으면 그냥 리턴
		if (CNetConfig::UseReportRealUdpCount == false)
			return;

		CriticalSectionLock clk(GetCriticalSection(), true);

		if (HostID_None != GetVolatileLocalHostID() && m_lastReportUdpCountTime > 0 && GetPreciseCurrentTimeMs() - m_lastReportUdpCountTime > CNetConfig::ReportRealUdpCountIntervalMs)
		{
			m_lastReportUdpCountTime = GetPreciseCurrentTimeMs();

			// 서버에게 udp 보낸 갯수를 보고한다.
			m_c2sProxy.ReportC2SUdpMessageTrialCount(HostID_Server, g_ReliableSendForPN, m_toServerUdpSendCount);

			// peer들에게 내가 peer들에게 보내거나 혹은 받은 udp 갯수를 보고한다.
			// (매 10초마다 쏘면 트래픽이 스파이크를 칠지 모르나, 피어가 100개라 하더라도 2000바이트 정도이다. 이 정도는 묵인해도 되겠다능.)
			for (AuthedHostMap::iterator iRP = m_authedHostMap.begin(); iRP != m_authedHostMap.end(); iRP++)
			{
				CRemotePeer_C* rp = LeanDynamicCastForRemotePeer(iRP->GetSecond());

				if (rp != NULL)
				{
					if (rp->m_garbaged)
						continue;

					m_c2cProxy.ReportUdpMessageCount(rp->m_HostID, g_ReliableSendForPN, rp->m_receiveudpMessageSuccessCount);
				}
			}
		}
	}

	void CNetClientImpl::StartupUpnpOnNeed()
	{
#if defined(_WIN32)
		CriticalSectionLock clk(GetCriticalSection(), true); // 이게 없으면 정확한 체크가 안되더라.
		if (!m_manager->m_upnp && m_settings.m_upnpDetectNatDevice &&
			m_remoteServer->m_ToServerTcp->GetLocalAddr().IsUnicastEndpoint() &&
			m_remoteServer->m_ToServerTcp->m_localAddrAtServer.IsUnicastEndpoint() &&
			IsBehindNat())
		{
			m_manager->m_upnp.Attach(new CUpnp(m_manager));
		}
#endif
	}

	void CNetClientImpl::TEST_GetTestStats(CTestStats2 &output)
	{
		output = m_testStats2;
	}

#ifdef VIZAGENT
	void CNetClientImpl::EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey)
	{
		if (!m_vizAgent)
		{
			m_vizAgent.Attach(new CVizAgent(this, vizServerAddr, vizServerPort, loginKey));
		}
	}
#endif

	void CNetClientImpl::Cleanup()
	{
		LockMain_AssertIsLockedByCurrentThread();

		// NOTE: 본 함수가 호출되었을 시점에는 각 Hosts들과 Socket들이 모두 정리가 되어야만 합니다.
		if(!CanDeleteNow())
		{
			throw Exception("Cannot delete now!");
		}

		m_remoteServer->CleanupToServerUdpFallbackable();

		// 여기까지 왔으면 socket들은 이미 다 사라진 상태이어야 한다.
		// 그런데 아직까지 garbage에 뭔가가 들어있으면 잠재적 크래시 위험.
		if (m_remoteServer->m_ToServerTcp != NULL)
		{
			m_remoteServer->m_ToServerTcp->CloseSocketOnly();
			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
				Log(0, LogCategory_Udp, _PNT("CleanupEvenUnstableSituation, CloseSocketOnly called."));
		}

		//추가로 클린업해야 할것들을 한다.
		m_selfP2PSessionKey.Clear();
		m_selfEncryptCount = 0;
		m_selfDecryptCount = 0;

		m_supressSubsequentDisconnectionEvents = false;
		m_RequestServerTimeCount = 0;
		m_serverTimeDiff = 0;
		m_serverUdpRecentPingMs = 0;
		m_serverUdpLastPingMs = 0;
		m_serverUdpPingChecked = false;
		m_serverTcpRecentPingMs = 0;
		m_serverTcpLastPingMs = 0;
		m_serverTcpPingChecked = false;
		m_firstTcpPingedTime = 0;
		m_lastRequestServerTimeTime = 0;
		m_P2PGroups.Clear(); // 확인사살.
		m_P2PConnectionTrialEndTime = CNetConfig::GetP2PHolepunchEndTimeMs();
		m_P2PHolepunchInterval = CNetConfig::P2PHolepunchIntervalMs;
		m_natDeviceNameDetected = false;
		m_stats = CNetClientStats();
		m_internalVersion = CNetConfig::InternalNetVersion;
		m_settings = CNetSettings();
		m_serverInstanceGuid = Guid();
		m_toServerEncryptCount = 0;
		m_toServerDecryptCount = 0;
		m_toServerSessionKey.Clear();
		//m_toServerUdpSocketFailed = false;
		//m_minExtraPing = 0;
		//m_extraPingVariance = 0;
		m_loopbackHost->m_HostID = HostID_None;
		m_connectionParam = CNetConnectionParam();

		//FinalUserWorkItemList_RemoveReceivedMessagesOnly(); // 몽땅 지우면 안됨. CNetClient.Disconnect 콜 후에도 CNetClient.FrameMove 콜 시 디스에 대한 콜백이 있어야 하니까.

		// 		if(m_timer != NULL)
		// 		{
		// 			m_timer->Stop();
		// 			m_timer.Free();
		// 		}

		m_lastFrameMoveInvokedTime = 0;
		//m_remotePeers.Clear();
		ClearGarbagedHostMap();
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer = CTimeAlarm(CNetConfig::LongIntervalMs);
		//		m_processSendReadyRemotes_Timer = CTimeAlarm(CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs);
		m_virtualSpeedHackMultiplication = 1;
		m_speedHackDetectorPingTime = CNetConfig::EnableSpeedHackDetectorByDefault ? 0 : INT64_MAX;

		m_testStats2 = CTestStats2();
		m_applicationHint.m_recentFrameRate = 0;
		m_lastCheckSendQueueTime = 0;
		m_lastUpdateNetClientStatCloneTime = 0;
		m_sendQueueHeavyStartTime = 0;

		m_TcpAndUdp_DoForShortInterval_lastTime = 0;

		m_unusedUdpPorts.Clear();
		m_usedUdpPorts.Clear();

		// 디버거로 Disconnect 끝자락에서 미정리된 변수들이 발견되어, 여기서 추가로 그것들을 청소.
		m_ReliablePing_Timer.Reset(GetPreciseCurrentTimeMs());
		m_toServerUdpSendCount = 0;
		// 0으로 설정을 하면 처리가 되지 않으므로 0이 아닌 값으로 변경하였습니다.
		m_lastReportUdpCountTime = CNetConfig::ReportRealUdpCountIntervalMs;
		m_ioPendingCount = 0;
		m_enablePingTestEndTime = 0;

		m_disconnectCallTime = 0;

		// NetClient의 경우 Connect ~ Disconnect 범위에서만 RemoteServer 사용
		if (m_remoteServer != NULL)
		{
			delete m_remoteServer;
			m_remoteServer = NULL;
		}

		// NetClient의 경우 Connect ~ Disconnect 범위에서만 LoopbackHost 사용
		if (m_loopbackHost != NULL)
		{
			delete m_loopbackHost;
			m_loopbackHost = NULL;
		}

		DeleteSendReadyList();
	}

#ifdef TEST_DISCONNECT_CLEANUP
	bool CNetClientImpl::IsAllCleanup()
	{
		if (m_ToServerUdp_fallbackable != NULL)
			return false;

		if (m_ToServerTcp != NULL)
			return false;

		if (m_selfP2PSessionKey != NULL)
			return false;

		if (m_selfDecryptCount != 0 || m_selfEncryptCount != 0)
			return false;

		if (m_supressSubsequentDisconnectionEvents)
			return false;

		if (m_RequestServerTimeCount != 0)
			return false;
		if (m_serverTimeDiff != 0)
			return false;
		if (m_serverUdpRecentPingMs != 0)
			return false;
		if (m_serverUdpLastPing != 0)
			return false;

		if (m_lastRequestServerTimeTime != 0)
			return false;

		if (m_P2PGroups.GetCount() != 0)
			return false;
		if (m_P2PConnectionTrialEndTime != CNetConfig::GetP2PHolepunchEndTimeMs())
			return false;

		if (m_P2PHolepunchInterval != CNetConfig::P2PHolepunchIntervalMs)
			return false;

		if (m_natDeviceNameDetected)
			return false;

		if (m_internalVersion != CNetConfig::InternalVersion)
			return false;

		if (m_serverInstanceGuid != Guid())
			return false;

		if (m_preFinalRecvQueue.GetCount() != 0)
			return false;

		if ((m_ToServerEncryptCount | m_ToServerDecryptCount) != 0)
			return false;

		if (m_toServerUdpSocket != NULL)
			return false;

		if (m_toServerUdpSocketFailed)
			return false;

		if (!m_loopbackFinalReceivedMessageQueue.IsEmpty())
			return false;

		//if(m_minExtraPing != 0)
		//	return false;

		if (m_extraPingVariance != 0)
			return false;

		if (GetVolatileLocalHostID() != HostID_None)
			return false;

		if (!m_postponedFinalUserWorkItemList.IsEmpty())
			return false;

		if (m_lastFrameMoveInvokedTime != 0)
			return false;

		if (!m_remotePeers.IsEmpty())
			return false;
		if (!m_garbagedSockets.IsEmpty())
			return false;
		if (m_virtualSpeedHackMultiplication != 1)
			return false;

		// 		if(m_ToServerTcp->m_lastReceivedTime != 0)
		// 			return false;

		if (m_lastTcpStreamReceivedTime != 0)
			return false;

		return true;
	}
#endif

#ifdef VIZAGENT
	void CNetClientImpl::Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext)
	{
		if (m_vizAgent)
		{
			CFastArray<HostID> tgt;
			tgt.SetCount(remoteCount);
			UnsafeFastMemcpy(tgt.GetData(), remotes, sizeof(HostID) * remoteCount);
			m_vizAgent->m_c2sProxy.NotifyCommon_SendRmi(HostID_Server, g_ReliableSendForPN, tgt, summary);
		}
	}

	void CNetClientImpl::Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString)
	{
		if (m_vizAgent)
		{
			m_vizAgent->m_c2sProxy.NotifyCommon_ReceiveRmi(HostID_Server, g_ReliableSendForPN, sender, RmiName, RmiID);
		}
	}
#endif

	// to-server UDP socket을 생성한다.
	// 생성 후 m_remoteServer->m_ToServerUdp is null로 생성 여부를 검사하라.
	void CNetClientImpl::New_ToServerUdpSocket()
	{
		// UDP 소켓생성이 1번이라도 실패한다면, 생성 시도를 하지 않는다.
		if (m_remoteServer->m_toServerUdpSocketCreateHasBeenFailed)
			return;

		if (m_remoteServer->m_ToServerUdp == NULL)
		{
			// NOTE: 여기서 try-catch 하지 말 것. 여기서 catch를 하더라도 rethrow를 하면 모를까.
			AddrPort udpLocalAddr = m_remoteServer->m_ToServerTcp->GetLocalAddr();

			if (!udpLocalAddr.IsUnicastEndpoint())
			{
#if defined(_WIN32)
				String text;
				text.Format(_PNT("FATAL: new UDP socket - Cannot create UDP socket! Cannot get TCP NIC address %s."), udpLocalAddr.ToString());
				CErrorReporter_Indeed::Report(text);
#endif
				m_remoteServer->m_toServerUdpSocketCreateHasBeenFailed = true;
				EnqueWarning(ErrorInfo::From(ErrorType_LocalSocketCreationFailed, GetVolatileLocalHostID(), _PNT("UDP socket for server connection")));
				return;
			}

			CSuperSocketPtr newUdpSocket(CSuperSocket::New(this, SocketType_Udp));
			BindUdpSocketToAddrAndAnyUnusedPort(newUdpSocket, udpLocalAddr);

			AssertIsLockedByCurrentThread();
			m_netThreadPool->AssociateSocket(newUdpSocket->GetSocket());

			m_validSendReadyCandidates.Add(newUdpSocket, newUdpSocket);

			m_remoteServer->m_ToServerUdp = newUdpSocket;

/*// 3003!!!!
			m_remoteServer->m_ToServerUdp->m_tagName = _PNT("to-server UDP"); */



			SocketToHostsMap_SetForAnyAddr(newUdpSocket, m_remoteServer);

#if !defined(_WIN32)
			// issue UDP first recv
			//((CUdpSocket_C*)m_toServerUdpSocket.get())->NonBlockedRecv(CNetConfig::UdpIssueRecvLength); // issue UDP first recv
#else
			// issue UDP first recv
			m_remoteServer->m_ToServerUdp->IssueRecv(true);
#endif
		}
	}

	// 	void CNetClientImpl::RequestServerUdpSocketReady_FirstTimeOnly()
	// 	{
	// 		//udp소켓 생성을 요청.
	// 		if(m_toServerUdpSocket == NULL && m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting == false &&
	// 			m_settings.m_fallbackMethod <= FallbackMethod_PeersUdpToTcp && m_toServerUdpSocketFailed == false)
	// 		{
	// 			m_c2sProxy.C2S_RequestCreateUdpSocket(HostID_Server,g_ReliableSendForPN);
	// 			m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting = true;
	// 		}
	// 	}
	//
	void CNetClientImpl::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		if (m_userThreadPool != NULL)
		{
			m_userThreadPool->GetThreadInfos(output);
		}
	}

	void CNetClientImpl::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		if (m_netThreadPool != NULL)
		{
			m_netThreadPool->GetThreadInfos(output);
		}
	}

	void CNetClientImpl::SetApplicationHint(const CApplicationHint &hint)
	{
		m_applicationHint = hint;
	}
	void CNetClientImpl::GetApplicationHint(CApplicationHint &hint)
	{
		hint = m_applicationHint;
	}

	// 송신한뒤 나머지 tcp stream 처리작업
	void CNetClientImpl::OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/)
	{
		CriticalSectionLock clk(m_statsCritSec, true);
		// 통계를 기록한다.
		if (socketType == SocketType_Tcp)
		{
			m_stats.m_totalTcpSendBytes += doneBytes;
		}
		else
		{
			m_stats.m_totalUdpSendCount++;
			m_stats.m_totalUdpSendBytes += doneBytes;
		}
		// caller 안에서 next issue send on need를 하므로 여기서 next issue send on need 하지 말자.
	}

	// overrided function
	void CNetClientImpl::OnMessageReceived(int doneBytes, SocketType socketType, CReceivedMessageList& messages, CSuperSocket* socket)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CHostBase* hostBase = SocketToHostsMap_Get_NOLOCK(socket, AddrPort::Unassigned);

		/* HostBase 객체가 Socket 객체보다 먼저 삭제된 경우에는 SocketToHostsMap_Get_NOLOCK에서는 NULL이 반환될 수 있습니다.
		이러한 경우 경우에는 지금 처리중인 메세지를 무시합니다. 아래는 그 이유.

		socket to host map에서 제거되었다는 의미는, 이미 host를 garbage collect 즉 delete가 되었다는 상태입니다.
		이는 host가 처리해야 할 user work가 완전히 다 처리되어 제거되어도 되는 조건을 이미 만족했다는 뜻입니다.
		그리고 그러한 상황에서는 이미 host가 가졌던 socket들의 소유권도 다 포기한 상태입니다. */
		if (hostBase == NULL)
			return;

		CriticalSectionLock statsLock(m_statsCritSec, true);

		// 통계
		if (socketType == SocketType_Tcp)
		{
			m_stats.m_totalTcpReceiveBytes += doneBytes;
			statsLock.Unlock(); // stats 갱신 끝. 이제 lock을 풀어버리자. 아래에서 main lock을 하니까.
		}
		else if (socketType == SocketType_Udp)
		{
			m_stats.m_totalUdpReceiveCount++;
			m_stats.m_totalUdpReceiveBytes += doneBytes;
			statsLock.Unlock(); // stats 갱신 끝. 이제 lock을 풀어버리자. 아래에서 main lock을 하니까.

			if (hostBase == m_remoteServer)
			{
				// 마지막 받은 시간을 갱신해서 회선이 살아있음을 표식
				m_remoteServer->UpdateServerUdpReceivedTime();
			}
			else
			{
				CRemotePeer_C* peer = LeanDynamicCastForRemotePeer(hostBase);

				// 마지막 받은 시간을 갱신해서 회선이 살아있음을 표식
				int64_t curTime = GetPreciseCurrentTimeMs();
				int64_t okTime = curTime - peer->m_lastDirectUdpPacketReceivedTimeMs;
				if (okTime > 0)
					peer->m_lastUdpPacketReceivedInterval = okTime;

				peer->m_lastDirectUdpPacketReceivedTimeMs = curTime;
				peer->m_directUdpPacketReceiveCount++;
			}
		}

		ProcessMessageOrMoveToFinalRecvQueue(socketType, socket, messages);
	}

	void CNetClientImpl::ProcessMessageOrMoveToFinalRecvQueue(SocketType socketType, CSuperSocket* socket, CReceivedMessageList &extractedMessages)
	{
		// main lock을 걸었으니 송신자의 HostID를 이제 찾을 수 있다.
		AssertIsLockedByCurrentThread();

		for (CReceivedMessageList::iterator i = extractedMessages.begin(); i != extractedMessages.end(); i++)
		{
			CReceivedMessage &receivedMessage = *i;

			/*  TCP, UDP 모두 Proud.CNetCoreImpl.SocketToHostMap_Get로 찾도록 하자.
			이를 위해 per-peer UDP socket을 생성하는 곳에서는 SocketToHostMap_Set을,
			UDP socket을 버리는 곳에서는 SocketToHostMap_Remove를 호출하게 수정하자.
			VA find ref로 Proud.CRemotePeer_C.m_udpSocket를 건드리는 곳을 찾으면 됨. */
			// NetClient에서는 하나의 UDP Socket이 여러 AddrPort를 가지는 경우가 없으므로 AddrPort::Unassigned로 처리하였습니다.
			CHostBase* pFrom = SocketToHostsMap_Get_NOLOCK(socket, AddrPort::Unassigned);

			receivedMessage.m_remoteHostID = pFrom->m_HostID;

			bool useMessage;

			// TCP socket ptr를 체크하므로
			AssertIsLockedByCurrentThread();

			if (receivedMessage.m_hasMessageID == false
				|| m_remoteServer->m_ToServerTcp == NULL
				|| socketType != SocketType_Tcp)
			{
				useMessage = true;
			}
			else
			{
				useMessage = m_remoteServer->m_ToServerTcp->AcrMessageRecovery_ProcessReceivedMessageID(receivedMessage.m_messageID);
			}

			if (useMessage)
			{
				assert(receivedMessage.m_unsafeMessage.GetReadOffset() == 0);
				m_worker->ProcessMessage_ProudNetLayer(socket, receivedMessage);
			}
		}
	}

	// to-server 본 TCP 연결이 끊어졌거나, TCP 상태가 살아있지만 랙이 심각하다. 즉 불량.
	// TCP 재접속을 시작하던지, disconnecting mode가 되던지 하자.
	void CNetClientImpl::DisconnectOrStartAutoConnectionRecovery(const ErrorInfo& errorInfo)
	{
		if (m_enableAutoConnectionRecovery && (m_remoteServer->m_shutdownIssuedTime == 0))
		{
			// NOTE: 기존 to-server TCP는 닫지 않는다. ACR 재접속 후보가 성공이 되면 그때야 비로소 닫도록 한다.
			// ACR 과정을 시작한다.
			StartAutoConnectionRecovery(); // 이 함수가 끝나고 나면 ACR context가 생성되어 있거나 실패했다면 없을 것임
		}

		if (m_autoConnectionRecoveryContext == NULL)
		{
			// 위 과정 처리를 했음에도 불구하고 ACR is null이다.
			// 즉 ACR 비활성 클라이거나 ACR context 생성이 실패했다.
			// => 그냥 연결 해제 처리.
			// NOTE: deleting remote server will be done later in Disconnecting mode.
			EnqueueDisconnectionEvent(errorInfo.m_errorType, errorInfo.m_detailType, errorInfo.m_comment);
			m_worker->SetState(CNetClientWorker::Disconnecting);
		}
	}

	// TCP 연결이 끊어졌을 때 처리
	void CNetClientImpl::ProcessDisconnecting(CSuperSocket* socket, const ErrorInfo& errorInfo)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (socket == m_remoteServer->m_ToServerTcp)
		{
			// 연결유지기능을 활성화 시키던지 disconnecting mode로 바뀌던지 해야.
			DisconnectOrStartAutoConnectionRecovery(errorInfo);
		}
		else
		{
			// 재접속 연결이 중도 실패한 경우, 재시도를 해야 한다. 
			// 명세서 참고 바람.
			ProcessAcrCandidateFailure();
		}
	}

	// Proud.CNetCoreImpl implementation.
	void CNetClientImpl::AssociateSocket(CFastSocket* socket)
	{
		//m_manager->m_netWorkerThread->AssociateSocket(socket);

		m_netThreadPool->AssociateSocket(socket);

		// #if defined (_WIN32)
		// 		m_manager->m_completionPort->AssociateSocket(socket);
		// #else	// Android/IOS/Marmalade
		// 		m_manager->m_reactor->AssociateSocket(socket);
		// #endif
	}

	// 	void CNetClientImpl::SendReadyInstances_Add()
	// 	{
	// 		m_manager->SendReadyInstances_Add(this);
	// 	}

	bool CNetClientImpl::SetHostTag(HostID hostID, void* hostTag)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (hostID == HostID_None)
		{
			return false;
		}
		else if (hostID == HostID_Server)
		{
			m_remoteServer->m_hostTag = hostTag;
		}
		else if (hostID == GetVolatileLocalHostID())
		{
			m_loopbackHost->m_hostTag = hostTag;
		}
		else
		{
			CHostBase* hb = AuthedHostMap_Get(hostID);
			if (hb == NULL)
				return false;

			hb->m_hostTag = hostTag;
		}

		return true;
	}

	void* CNetClientImpl::GetHostTag(HostID hostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (hostID == HostID_None)
			return NULL;

		if (hostID == HostID_Server)
			return m_remoteServer->m_hostTag;

		if (hostID == GetVolatileLocalHostID())
			return m_loopbackHost->m_hostTag;

		CHostBase* hp = AuthedHostMap_Get(hostID);
		if (hp != NULL)
		{
			return hp->m_hostTag;
		}
		// 		if(hostID==HostID_Server)
		// 		{
		// 			return m_serverAsSendDest.m_hostTag;
		// 		}
		// 		else if(hostID == GetVolatileLocalHostID())
		// 		{
		// 			return m_hostTag;
		// 		}
		// 		else
		// 		{
		// 			CRemotePeerPtr_C peer =  GetPeerByHostID(hostID);
		// 			if(peer != NULL)
		// 				return peer->m_hostTag;
		//
		// 			return NULL;
		// 		}
		return NULL;
	}

	Proud::AddrPort CNetClientImpl::GetTcpLocalAddr()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == NULL)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->GetLocalAddr();
	}

	Proud::AddrPort CNetClientImpl::GetUdpLocalAddr()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerUdp == NULL)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerUdp->GetLocalAddr();
	}

	// 송신큐가 너무 증가하면 사용자에게 경고를 쏜다.
	void CNetClientImpl::CheckSendQueue()
	{
		int64_t curTime = GetPreciseCurrentTimeMs();
		if (m_remoteServer->m_ToServerTcp != NULL
			&& curTime - m_lastCheckSendQueueTime > CNetConfig::SendQueueHeavyWarningCheckCoolTimeMs)
		{
			int length = m_remoteServer->m_ToServerTcp->GetSendQueueLength();

			if (m_remoteServer->m_ToServerUdp)
				length += m_remoteServer->m_ToServerUdp->GetPacketQueueTotalLengthByAddr(m_remoteServer->GetServerUdpAddr());

			if (m_sendQueueHeavyStartTime != 0)
			{
				if (length > CNetConfig::SendQueueHeavyWarningCapacity)
				{
					if (curTime - m_sendQueueHeavyStartTime > CNetConfig::SendQueueHeavyWarningTimeMs)
					{
						m_sendQueueHeavyStartTime = curTime;

						String str;
						str.Format(_PNT("%d bytes in send queue"), length);
						EnqueWarning(ErrorInfo::From(ErrorType_SendQueueIsHeavy, HostID_Server, str));
					}
				}
				else
					m_sendQueueHeavyStartTime = 0;
			}
			else if (length > CNetConfig::SendQueueHeavyWarningCapacity)
				m_sendQueueHeavyStartTime = curTime;

			m_lastCheckSendQueueTime = curTime;
		}
	}

#if defined(_WIN32)    
	void CNetClientImpl::UpdateNetClientStatClone()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// emergency log가 활성화되어있을때만 처리
		if (m_settings.m_emergencyLogLineCount > 0 && m_worker->GetState() < CNetClientWorker::Disconnecting)
		{
			int64_t curTime = GetPreciseCurrentTimeMs();
			if (curTime - m_lastUpdateNetClientStatCloneTime > CNetConfig::UpdateNetClientStatCloneCoolTimeMs)
			{
				m_lastUpdateNetClientStatCloneTime = curTime;
				GetStats(m_recentBackedUpStats);
			}
		}
	}
#endif

	void CNetClientImpl::GetWorkerState(CClientWorkerInfo &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		output.m_isWorkerThreadNull = m_worker == NULL ? true : false;
		//output.m_isWorkerThreadEnded = m_manager->m_threadEnd;
		output.m_disconnectCallCount = m_disconnectInvokeCount;
		output.m_finalWorkerItemCount = (int)GetFinalUserWotkItemCount();
		if (GetVolatileLocalHostID() == HostID_None)
			output.m_peerCount = 0;
		else
			// GetVolatileLocalHostID()가 None이 아닌 이상, RemoteServer와 LoopBackHost는 AuthedHosts로 추가가 되었으므로 제외
			output.m_peerCount = (int)m_authedHostMap.GetCount() - 2;
		output.m_connectCallCount = m_connectCount;
		output.m_currentTimeMs = GetPreciseCurrentTimeMs();
		//		output.m_lastTcpStreamTimeMs = m_lastTcpStreamReceivedTime;
		output.m_workerThreadID = 0;

		// 		if(!output.m_isWorkerThreadEnded)
		// 		{
		// 			output.m_workerThreadID = m_manager->GetWorkerThreadID();
		// 		}

		CServerConnectionState state;
		output.m_connectionState = GetServerConnectionState(state);
	}

#ifdef _DEBUG
	void CNetClientImpl::CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname)
	{}
#endif // _DEBUG

	bool CNetClientImpl::GetSocketInfo(HostID remoteHostID, CSocketInfo& output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

#if !defined(_WIN32)
		output.m_tcpSocket = -1;
		output.m_udpSocket = -1;
#else
		output.m_tcpSocket = INVALID_SOCKET;
		output.m_udpSocket = INVALID_SOCKET;
#endif

		// 서버와의 연결
		if (m_remoteServer->m_ToServerTcp && m_remoteServer->m_ToServerTcp->GetSocket())
		{
			output.m_tcpSocket = m_remoteServer->m_ToServerTcp->GetSocket()->m_socket;
		}
		else
			return false;

		if (HostID_Server == remoteHostID)		// 서버이면
		{
			if (m_remoteServer->m_ToServerUdp && m_remoteServer->m_ToServerUdp->GetSocket())
			{
				output.m_udpSocket = m_remoteServer->m_ToServerUdp->GetSocket()->m_socket;

				return true;
			}
		}
		else	// peer이면
		{
			CRemotePeer_C* peer = GetPeerByHostID_NOLOCK(remoteHostID);

			if (peer == NULL)
				return false;

			if (peer->IsRelayedP2P() && m_remoteServer->m_ToServerUdp && m_remoteServer->m_ToServerUdp->GetSocket())
			{
				// 릴레이인 경우
				output.m_udpSocket = m_remoteServer->m_ToServerUdp->GetSocket()->m_socket;

				return true;
			}
			else if (!peer->IsRelayedP2P() && peer->m_udpSocket && peer->m_udpSocket->GetSocket())
			{
				// 직접 P2P인 경우
				output.m_udpSocket = peer->m_udpSocket->GetSocket()->m_socket;

				return true;
			}
		}

		return false;
	}

#if defined(_WIN32)    
	void CNetClientImpl::AddEmergencyLogList(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction /*= _PNT("")*/, int logLine /*= 0*/)
	{
		//lock하에서 진행되어야 함.
		LockMain_AssertIsLockedByCurrentThread();

		if (m_settings.m_emergencyLogLineCount == 0)
			return;

		//###	// Emergency 로그 추가 시간을 클라이언트에 맞춰야하는지 서버에 맞춰야하는지??
		// 우선 클라에 맞춤
		// 		CEmergencyLogData::EmergencyLog logNode;
		// 		logNode.m_logCategory = logCategory;
		// 		logNode.m_text = text;
		// 		logNode.m_addedTime = CTime::GetCurrentTime().GetTime();

		CEmergencyLogData::EmergencyLog logNode;
		logNode.m_logLevel = logLevel;
		logNode.m_logCategory = logCategory;
		logNode.m_hostID = GetVolatileLocalHostID();
		logNode.m_message = logMessage;

		m_emergencyLogData.m_logList.AddTail(logNode);

		if (m_emergencyLogData.m_logList.GetCount() > m_settings.m_emergencyLogLineCount)
		{
			m_emergencyLogData.m_logList.RemoveHead();
		}
	}

	bool GetWindowsVersion(DWORD& major, DWORD& minor, DWORD& productType)
	{
		LPBYTE pinfoRawData;
		if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
		{
			WKSTA_INFO_100 * pworkstationInfo = (WKSTA_INFO_100 *)pinfoRawData;
			major = pworkstationInfo->wki100_ver_major;
			minor = pworkstationInfo->wki100_ver_minor;
			NetApiBufferFree(pinfoRawData);
#if (_MSC_VER >= 1500)
			GetProductInfo(major, minor, 0, 0, &productType);
#else
			productType = 0;
#endif
			return true;
		}
		return false;
	}

	bool CNetClientImpl::SendEmergencyLogData(String serverAddr, uint16_t serverPort)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		CEmergencyLogData* logData = new CEmergencyLogData();

		// NetClient 가 날라갈것을 대비해 지역으로 데이터를 옮기자.

		// 기존걸 복사하자.
		m_emergencyLogData.CopyTo(*logData);

		// logData의 나머지를 채운다.
		logData->m_HostID = m_loopbackHost->m_backupHostID;

		if (m_remoteServer != NULL)
			logData->m_totalTcpIssueSendBytes = m_remoteServer->m_ToServerTcp->GetTotalIssueSendBytes();
		else
			logData->m_totalTcpIssueSendBytes = 0;

		logData->m_ioPendingCount = m_ioPendingCount;
		logData->m_loggingTime = ::_time64(NULL); // GetPreciseCurrentTime는 시스템 시작 시간마다 다르므로, 사용 불가.
		logData->m_connectCount = m_connectCount;
		logData->m_connResetErrorCount = g_connResetErrorCount;
		logData->m_msgSizeErrorCount = g_msgSizeErrorCount;
		logData->m_netResetErrorCount = g_netResetErrorCount;
		logData->m_intrErrorCount = g_intrErrorCount;
		logData->m_directP2PEnablePeerCount = m_recentBackedUpStats.m_directP2PEnabledPeerCount;
		logData->m_remotePeerCount = m_recentBackedUpStats.m_remotePeerCount;
		logData->m_totalTcpReceiveBytes = m_recentBackedUpStats.m_totalTcpReceiveBytes;
		logData->m_totalTcpSendBytes = m_recentBackedUpStats.m_totalTcpSendBytes;
		logData->m_totalUdpReceiveCount = m_recentBackedUpStats.m_totalUdpReceiveCount;
		logData->m_totalUdpSendBytes = m_recentBackedUpStats.m_totalUdpSendBytes;
		logData->m_totalUdpSendCount = m_recentBackedUpStats.m_totalUdpSendCount;
		logData->m_natDeviceName = GetNatDeviceName();

		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		logData->m_processorArchitecture = sysinfo.wProcessorArchitecture;

		DWORD major = 0;
		DWORD minor = 0;
		DWORD productType = 0;
		GetWindowsVersion(major, minor, productType);

		logData->m_osMajorVersion = major;
		logData->m_osMinorVersion = minor;
		logData->m_productType = (BYTE)productType;

		logData->m_serverAddr = serverAddr;
		logData->m_serverPort = serverPort;

		::QueueUserWorkItem(RunEmergencyLogClient, (PVOID)logData, WT_EXECUTELONGFUNCTION);

		return true;
	}

	DWORD WINAPI CNetClientImpl::RunEmergencyLogClient(void* context)
	{
		CEmergencyLogData* logData = (CEmergencyLogData*)context;
		CEmergencyLogClient* client = new CEmergencyLogClient();

		try
		{
			client->Start(logData->m_serverAddr, logData->m_serverPort, logData);

			while (client->GetState() != CEmergencyLogClient::Stopped)
			{
				client->FrameMove();
				Sleep(10);
			}
		}
		catch (Exception& e)
		{
			CFakeWin32::OutputDebugStringA(e.what());
		}

		// client를 먼저삭제해야 logData의 개런티 보장
		if (client)
			delete client;

		// logData 삭제
		if (logData)
			delete logData;

		return 0;
	}
#endif

	// udpLocalAddr이 가리키는 주소와, m_unusedUdpPorts 안의 포트값 중 하나 중 bind를 할 수 있는 것을 udpSocket에 bind를 한다.
	// 이 함수는 반드시 bind는 성공시킨다. 다만, m_unusedUdpPorts 혹은 임의의 port의 값이 쓰일 뿐이다.
	// returns: m_unusedUdpPorts의 것으로 bind하면 true, m_unusedUdpPorts 외의 임의의 port에 bind하면 false. 
	bool CNetClientImpl::BindUdpSocketToAddrAndAnyUnusedPort(CSuperSocket* udpSocket, AddrPort &udpLocalAddr)
	{
		int trialCount = 0;
		for (CFastSet<uint16_t>::iterator iter = m_unusedUdpPorts.begin(); iter != m_unusedUdpPorts.end(); iter++)
		{
			udpLocalAddr.m_port = *iter;

			AssureIPAddressIsUnicastEndpoint(udpLocalAddr);

			if (SocketErrorCode_Ok == udpSocket->Bind(udpLocalAddr))
			{
				// 사용한 UDP 포트 목록에 집어 넣습니다.
				m_usedUdpPorts.Add(udpLocalAddr.m_port);
				m_unusedUdpPorts.Remove(udpLocalAddr.m_port);
				return true;
			}
			trialCount++;
		}

		// 모든 사용자 udp포트에 대해서 실패했으니, 임의로 지정합니다.
		udpLocalAddr.m_port = 0;
		udpSocket->Bind(udpLocalAddr);
		AssureIPAddressIsUnicastEndpoint(udpSocket->GetLocalAddr());

		if (m_usedUdpPorts.GetCount() > 0 || m_unusedUdpPorts.GetCount() > 0)
		{
			// 유저가 UDP 포트를 지정했다면 실패를 알려줍니다.
			String str;
			str.Format(_PNT("Trial count:%d, Arbitrary port number used: %d"),
				trialCount,
				udpSocket->m_localAddr_USE_FUNCTION.m_port);
			EnqueWarning(ErrorInfo::From(Error_NoneAvailableInPortPool, GetVolatileLocalHostID(), str));
		}

		return false;
	}

	void CNetClientImpl::SetDefaultTimeoutTimeMs(int newValInMs)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (newValInMs < 1000)
		{
			if (m_eventSink_NOCSLOCK != NULL)
			{
				ShowUserMisuseError(_PNT("Too short timeout value. It may cause unfair disconnection."));
				return;
			}
		}
#ifndef _DEBUG
		if (newValInMs > 240000)
		{
			if (m_eventSink_NOCSLOCK != NULL)
			{
				//ShowUserMisuseError(_PNT("Too long timeout value. It may take a lot of time to detect lost connection."));
			}
		}
#endif

		m_settings.m_defaultTimeoutTime = (int)newValInMs;
	}

	// upnp 라우터가 기 감지된 상태이고 TCP 연결이 있는 상태이고 내외부 주소가 모두 인식되었고 
	// 아직 안 했다면 홀펀칭된 정보를 근거로 TCP 포트 매핑을 upnp에 건다.
	void CNetClientImpl::AddUpnpTcpPortMappingOnNeed()
	{
#if defined(_WIN32)
		if (!m_settings.m_upnpTcpAddPortMapping)
			return;

		if (m_upnpTcpPortMappingState != NULL)
			return;

		if (!HasServerConnection())
			return;

		if (!m_manager->m_upnp)
			return;

		// 서버,클라가 같은 LAN에 있거나 리얼IP이면 할 필요가 없다. (해서도 안됨)
		if (!m_remoteServer->m_ToServerTcp->GetLocalAddr().IsUnicastEndpoint() || !m_remoteServer->m_ToServerTcp->m_localAddrAtServer.IsUnicastEndpoint())
			return;

		if (!IsBehindNat())
			return;

		if (m_manager->m_upnp->GetCommandAvailableNatDevice() == NULL)
			return;

		m_upnpTcpPortMappingState.Attach(new CUpnpTcpPortMappingState);
		m_upnpTcpPortMappingState->m_lanAddr = m_remoteServer->m_ToServerTcp->GetLocalAddr();
		m_upnpTcpPortMappingState->m_wanAddr = m_remoteServer->m_ToServerTcp->m_localAddrAtServer;

		m_manager->m_upnp->AddTcpPortMapping(
			m_upnpTcpPortMappingState->m_lanAddr,
			m_upnpTcpPortMappingState->m_wanAddr,
			true);
#endif
	}

	// 하나라도 포함되는 grouphostid를 찾으면 true를 리턴한다. 
	bool CNetClientImpl::GetIntersectionOfHostIDListAndP2PGroupsOfRemotePeer(HostIDArray sortedHostIDList, CRemotePeer_C* rp, HostIDArray* outSubsetGroupHostIDList)
	{
		bool ret = false;

		// 이 객체 자체는 재사용되므로 청소 필수
		outSubsetGroupHostIDList->Clear();

		// remote peer가 소속된 각 p2p group에 대해
		for (P2PGroups_C::iterator it = rp->m_joinedP2PGroups.begin(); it != rp->m_joinedP2PGroups.end(); it++)
		{
			// 2진 검색을 해서, 입력받은 host ID list 안에 있으면 목록에 추가한다.
			if (BinarySearch(sortedHostIDList.GetData(), sortedHostIDList.GetCount(), it->GetFirst()))
			{
				outSubsetGroupHostIDList->Add(it->GetFirst());
				ret = true;
			}
		}
		return ret;
	}

	void CNetClientImpl::DeleteUpnpTcpPortMappingOnNeed()
	{
#if defined(_WIN32)
		if (!m_upnpTcpPortMappingState)
			return;

		if (HasServerConnection())
			return;

		if (!m_manager->m_upnp)
			return;

		m_manager->m_upnp->DeleteTcpPortMapping(
			m_upnpTcpPortMappingState->m_lanAddr,
			m_upnpTcpPortMappingState->m_wanAddr,
			true);

		m_upnpTcpPortMappingState.Free();
#endif
	}

	// 로컬 호스트가 공유기 뒤에 있는지
	bool CNetClientImpl::IsBehindNat()
	{
		if (m_remoteServer->m_ToServerTcp &&
			m_remoteServer->m_ToServerTcp->GetLocalAddr() != m_remoteServer->m_ToServerTcp->m_localAddrAtServer)
			return true;

		return false;
	}

	void CNetClientImpl::HlaAttachEntityTypes(CHlaEntityManagerBase_C* entityManager)
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaAttachEntityTypes(entityManager);
		else
		{
			throw Exception(HlaUninitErrorText);
		}
#endif // USE_HLA
	}

	void CNetClientImpl::HlaSetDelegate(IHlaDelegate_C* dg)
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl == NULL)
		{
			m_hlaSessionHostImpl.Attach(new CHlaSessionHostImpl_C(this));
		}
		m_hlaSessionHostImpl->HlaSetDelegate(dg);
#endif // USE_HLA
	}

	int CNetClientImpl::GetLastReliablePingMs(HostID peerHostID, ErrorType* error /*= NULL */)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (peerHostID == HostID_Server)
		{
			if (error != NULL)
			{
				if (m_serverTcpPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return m_serverTcpLastPingMs;
		}

		CRemotePeer_C* p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != NULL)
		{
			if (error != NULL)
			{
				if (p->m_p2pReliablePingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			return p->m_lastReliablePingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int cnt = 0;
			int total = 0;
			for (P2PGroupMembers_C::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int ping = GetLastReliablePingMs(iMember->GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != NULL)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}
		if (error != NULL)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}

	int CNetClientImpl::GetRecentReliablePingMs(HostID peerHostID, ErrorType* error /*= NULL */)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (peerHostID == HostID_Server)
		{
			if (error != NULL)
			{
				if (m_serverTcpPingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return m_serverTcpRecentPingMs;
		}

		CRemotePeer_C* p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != NULL)
		{
			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			if (error != NULL)
			{
				if (p->m_p2pReliablePingChecked)
					*error = ErrorType_Ok;
				else
					*error = ErrorType_ValueNotExist;
			}

			return p->m_recentReliablePingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int cnt = 0;
			int total = 0;
			for (P2PGroupMembers_C::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int ping = GetRecentReliablePingMs(iMember->GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != NULL)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}
		if (error != NULL)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}

	CNetClientImpl::CompressedRelayDestList_C::CompressedRelayDestList_C()
	{
		// 이 객체는 로컬 변수로 주로 사용되며, rehash가 일단 발생하면 대쪽 느려진다. 따라서 충분한 hash table을 미리 준비해 두고 
		// rehash를 못하게 막아버리자.	
		if (!m_p2pGroupList.InitHashTable(1627)) 
			throw std::bad_alloc();

		m_p2pGroupList.DisableAutoRehash();
	}

	// 이 안에 있는 group id, host id들의 총 갯수를 얻는다.
	// compressed relay dest list를 serialize하기 전에 크기 예측을 위함.
	int CNetClientImpl::CompressedRelayDestList_C::GetAllHostIDCount()
	{
		int ret = (int)m_p2pGroupList.GetCount();	// group hostid count 도 갯수에 포함해야한다.

		CFastMap2<HostID, P2PGroupSubset_C, int>::iterator it = m_p2pGroupList.begin();
		for (; it != m_p2pGroupList.end(); it++)
		{
			ret += (int)it->GetSecond().m_excludeeHostIDList.GetCount();
		}

		ret += (int)m_includeeHostIDList.GetCount();

		return ret;
	}

	// p2p group과 그 group에서 제외되어야 하는 individual을 추가한다. 아직 group이 없을 때에만 추가한다.
	void CNetClientImpl::CompressedRelayDestList_C::AddSubset(const HostIDArray &subsetGroupHostID, HostID hostID)
	{
		for (int i = 0; i < subsetGroupHostID.GetCount(); i++)
		{
			P2PGroupSubset_C& subset = m_p2pGroupList[subsetGroupHostID[i]]; // 없으면 새 항목을 넣고 있으면 그걸 찾는다.

			if (hostID != HostID_None)
				subset.m_excludeeHostIDList.Add(hostID);
		}
	}

	void CNetClientImpl::CompressedRelayDestList_C::AddIndividual(HostID hostID)
	{
		m_includeeHostIDList.Add(hostID);
	}

	// 패킷양이 많은지 판단
	// 	bool CNetClientImpl::CheckMessageOverloadAmount()
	// 	{
	// 		if(GetFinalUserWotkItemCount()
	// 			>= CNetConfig::MessageOverloadWarningLimit)
	// 		{
	// 			return true;
	// 		}
	// 		return false;
	// 	}

	ErrorType CNetClientImpl::ForceP2PRelay(HostID remotePeerID, bool enable)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remotePeerID == HostID_Server)
		{
			return ErrorType_InvalidHostID;
		}

		CRemotePeer_C* p = GetPeerByHostID_NOLOCK(remotePeerID);
		if (p != NULL)
		{
			p->m_forceRelayP2P = enable;
			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remotePeerID == HostID_Server)
		{
			// 서버와의 UDP 통신이 활성화 중이면
			if (m_remoteServer->GetToServerUdpFallbackable() != NULL && m_remoteServer->GetToServerUdpFallbackable()->IsRealUdpEnabled())
			{
				CSuperSocket *pUdpSocket = m_remoteServer->m_ToServerUdp;
				if (pUdpSocket != NULL)
				{
					int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr);
					*outputPercent = nPercent;
					return ErrorType_Ok;
				}
			}
			else
			{
				// 서버와 unreliable 통신이 TCP로 되고 있을때는 로스율은 0입니다. 
				*outputPercent = 0;
				return ErrorType_Ok;
			}
		}

		if (remotePeerID == GetVolatileLocalHostID())
		{
			// 루프백은 당삼 0이지.
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		CRemotePeer_C* pPeer = GetPeerByHostID_NOLOCK(remotePeerID);
		if (pPeer != NULL)
		{
			CSuperSocket *pUdpSocket = pPeer->m_udpSocket;
			if (pUdpSocket != NULL && !pPeer->IsRelayedP2P())
			{
				int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(pPeer->m_P2PHolepunchedRemoteToLocalAddr);
				*outputPercent = nPercent;
				return ErrorType_Ok;
			}
			else
			{
				CSuperSocket *toServerUdpSocket = m_remoteServer->m_ToServerUdp;
				if (toServerUdpSocket != NULL)
				{
					int percent = 0;
					GetUnreliableMessagingLossRatioPercent(HostID_Server, &percent);
					int stats = 100 - (100 - percent) * (100 - pPeer->m_CSPacketLossPercent) / 100;
					*outputPercent = stats;
					return ErrorType_Ok;
				}
				else
				{
					// 여기-서버와의 UDP가 안뚫린 경우에는 여기-서버와의 UDP는 0%입니다. 
					*outputPercent = pPeer->m_CSPacketLossPercent;
					return ErrorType_Ok;
				}
			}
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::SetCoalesceIntervalMs(HostID remote, int interval)
	{
		if (interval < 0 || interval > 1000)
		{
			throw Exception("Coalesce interval out of range! Only 0~1000 is acceptable.");
		}

		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remote == HostID_Server)
		{
			m_remoteServer->m_autoCoalesceInterval = false;
			m_remoteServer->SetManualOrAutoCoalesceInterval(interval);
			if (m_remoteServer->m_ToServerUdp != NULL)
			{
				m_remoteServer->m_ToServerUdp->SetCoalesceInterval(m_remoteServer->GetServerUdpAddr(), m_remoteServer->GetCoalesceIntervalMs());
			}
			return ErrorType_Ok;
		}

		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			rp->m_autoCoalesceInterval = false;
			rp->SetManualOrAutoCoalesceInterval(interval);
			if (rp->m_udpSocket != NULL)
			{
				rp->m_udpSocket->SetCoalesceInteraval(rp->GetDirectP2PLocalToRemoteAddr(), rp->GetCoalesceIntervalMs());
			}

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::SetCoalesceIntervalToAuto(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remote == HostID_Server)
		{
			m_remoteServer->m_autoCoalesceInterval = true;
			m_remoteServer->SetManualOrAutoCoalesceInterval();
			if (m_remoteServer->m_ToServerUdp != NULL)
			{
				m_remoteServer->m_ToServerUdp->SetCoalesceInterval(m_remoteServer->GetServerUdpAddr(), m_remoteServer->GetCoalesceIntervalMs());
			}
			return ErrorType_Ok;
		}

		CRemotePeer_C* rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != NULL)
		{
			rp->m_autoCoalesceInterval = true;
			rp->SetManualOrAutoCoalesceInterval();
			if (rp->m_udpSocket != NULL)
			{
				rp->m_udpSocket->SetCoalesceInteraval(rp->GetDirectP2PLocalToRemoteAddr(), rp->GetCoalesceIntervalMs());
			}

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	void CNetClientImpl::OnCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue)
	{
		// timer tick or no coalesced send 상황에서 쌓이는 신호를 처리.
		switch ((int)customValue)
		{
		case CustomValueEvent_Heartbeat:
			/* CustomValueEvent_Heartbeat를 push할 때 인자로 net worker therad가 들어간다.
			따라서 이 case를 처리하는 스레드 또한 net worker thread이다.

			Heartbeat은 NS,NC,LS,LC 공통으로 필요로 합니다.
			Heartbeat은 OnTick과 달리 PN 내부로직만을 처리합니다.
			속도에 민감하므로 절대 블러킹 즉 CPU idle time이 없어야 하고요.

			따라서 아래 Heartbeat 함수 중에서 ProcessCustomValueEvent에 옮겨야 할 루틴은 옮기시고,
			나머지는 CNetServerImpl.Heartbeat에 그대로 남깁시다.
			*/
			if (AtomicCompareAndSwap32(0, 1, &m_isProcessingHeartbeat) == 0)
			{
				// 하나의 1-thread process 를 위하여 여기서 이 함수를 call 한다.
				CacheLocalIpAddress_MustGuaranteeOneThreadCalledByCaller();

				Heartbeat();

				AtomicCompareAndSwap32(1, 0, &m_isProcessingHeartbeat);
			}

			break;
		}

		// 상위 클래스의 함수 콜
		CNetCoreImpl::ProcessCustomValueEvent(workResult, customValue);
	}

	// CNetCoreImpl method impl
	void CNetClientImpl::OnSocketGarbageCollected(CSuperSocket* socket)
	{
		int udpPort = socket->GetLocalAddr().m_port;
		if (m_usedUdpPorts.ContainsKey(udpPort))
		{
			// 사용자 정의 UDP포트에 지정된 포트라면 반환합니다.
			m_unusedUdpPorts.Add(udpPort);
			m_usedUdpPorts.Remove(udpPort);
		}
	}

	void CNetClientImpl::OnAccepted(CSuperSocket* socket, AddrPort localAddr, AddrPort remoteAddr)
	{
		// NC가 이걸 받을 일이 있나? 이 함수는 빈 상태로 둔다.
	}

	void CNetClientImpl::OnConnectSuccess(CSuperSocket* socket)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (socket == m_remoteServer->m_ToServerTcp)
		{
			// 기존 TCP 연결 소켓
			ProcessFirstToServerTcpConnectOk();
		}
		else
		{
			// ACR 연결이 들어온 듯. 처리하자.
			if (AutoConnectionRecovery_OnTcpConnection(socket) == false)
			{
				ProcessAcrCandidateFailure();
			}
		}
	}

	void CNetClientImpl::OnConnectFail(CSuperSocket* socket, SocketErrorCode code)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == socket)
		{
			// to-server TCP의 실패인 경우
			// 통상 기존 연결실패 과정(go to disconnecting mode)
			Heartbeat_ConnectFailCase(code, _PNT("OCF"));
		}
		else
		{
			// ACR TCP 연결이 실패한 경우
			// 재시도를 또 한다. 명세서 참고 바람.
			ProcessAcrCandidateFailure();
		}
	}

	void CNetClientImpl::ProcessOneLocalEvent(LocalEvent& e, CHostBase* subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult)
	{
		LockMain_AssertIsNotLockedByCurrentThread();

		if (m_eventSink_NOCSLOCK != NULL)
		{
			INetClientEvent* eventSink_NOCSLOCK = (INetClientEvent*)m_eventSink_NOCSLOCK;

			bool processed = true;
			switch ((int)e.m_type)
			{
			case LocalEventType_ConnectServerSuccess:
			{
				ByteArray tempByteArray(e.m_userData.GetData(), e.m_userData.GetCount());
				outFunctionName = _PNT("OnJoinServerComplete");
				eventSink_NOCSLOCK->OnJoinServerComplete(ErrorInfoPtr(new ErrorInfo()), tempByteArray);
			}
			break;
			case LocalEventType_ConnectServerFail:
			{
				ErrorInfoPtr ep = e.m_errorInfo;
				ep->m_remote = HostID_Server;
				ep->m_remoteAddr = e.m_remoteAddr;
				ep->m_socketError = e.m_socketErrorCode;

				ByteArray callbackArray;
				if (!e.m_userData.IsNull() && e.m_userData.GetCount() != 0)
				{
					callbackArray.SetCount(e.m_userData.GetCount());
					e.m_userData.CopyRangeTo(callbackArray, 0, e.m_userData.GetCount());
				}
				outFunctionName = _PNT("OnJoinServerComplete");
				eventSink_NOCSLOCK->OnJoinServerComplete(ep, callbackArray);
			}
			break;
			case LocalEventType_ClientServerDisconnect:
#ifdef USE_HLA
				if (m_hlaSessionHostImpl != NULL)
					m_hlaSessionHostImpl->Clear(); // 여기서 콜 하는 것이 좀 땜빵 같지만 그래도 나쁘지는 않음. no main lock이기도 하니까.
#endif // USE_HLA
				outFunctionName = _PNT("OnLeaveServer");
				eventSink_NOCSLOCK->OnLeaveServer(e.m_errorInfo);
				break;
			case LocalEventType_AddMember:
				outFunctionName = _PNT("OnP2PMemberJoin");
				eventSink_NOCSLOCK->OnP2PMemberJoin(e.m_memberHostID, e.m_groupHostID, e.m_memberCount, e.m_customField);
				break;
			case LocalEventType_DelMember:
				outFunctionName = _PNT("OnP2PMemberLeave");
				eventSink_NOCSLOCK->OnP2PMemberLeave(e.m_memberHostID, e.m_groupHostID, e.m_memberCount);
				break;
			case LocalEventType_DirectP2PEnabled:
				outFunctionName = _PNT("OnChangeP2PRelayState");
				eventSink_NOCSLOCK->OnChangeP2PRelayState(e.m_remoteHostID, ErrorType_Ok);
				break;
			case LocalEventType_RelayP2PEnabled:
				outFunctionName = _PNT("OnChangeP2PRelayState");
				eventSink_NOCSLOCK->OnChangeP2PRelayState(e.m_remoteHostID, e.m_errorInfo->m_errorType);
				break;

			case LocalEventType_P2PMemberOffline:
			{
				CRemoteOfflineEventArgs args;
				args.m_remoteHostID = e.m_remoteHostID;
				eventSink_NOCSLOCK->OnP2PMemberOffline(args);
			}
			break;
			case LocalEventType_P2PMemberOnline:
			{
				CRemoteOnlineEventArgs args;
				args.m_remoteHostID = e.m_remoteHostID;
				eventSink_NOCSLOCK->OnP2PMemberOnline(args);
			}
			break;
			case LocalEventType_ServerOffline:
			{
				CRemoteOfflineEventArgs args;
				args.m_remoteHostID = e.m_remoteHostID;
				eventSink_NOCSLOCK->OnServerOffline(args);
			}
			break;
			case LocalEventType_ServerOnline:
			{
				CRemoteOnlineEventArgs args;
				args.m_remoteHostID = e.m_remoteHostID;
				eventSink_NOCSLOCK->OnServerOnline(args);
			}
			break;

			case LocalEventType_SynchronizeServerTime:
				outFunctionName = _PNT("OnSynchronizeServerTime");
				eventSink_NOCSLOCK->OnSynchronizeServerTime();
				break;
			case LocalEventType_Error:
				eventSink_NOCSLOCK->OnError(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
				break;
			case LocalEventType_Warning:
				eventSink_NOCSLOCK->OnWarning(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
				break;
			case LocalEventType_ServerUdpChanged:
			{
				outFunctionName = _PNT("OnChangeServerUdpState");
				eventSink_NOCSLOCK->OnChangeServerUdpState(e.m_errorInfo->m_errorType);
			}
			break;
			default:
			{
				processed = false;
			}
			return;
			}

			if (workResult != NULL && processed)
				++workResult->m_processedEventCount;
		}
	}

	bool CNetClientImpl::OnHostGarbageCollected(CHostBase* remote)
	{
		AssertIsLockedByCurrentThread();

		if (remote == m_loopbackHost)
		{
			// NOTE: LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
			return false;
		}
		else if (remote == m_remoteServer)
		{
			// remote server가 가진 TCP socket을 garbage 처리 및 reference 해지한다.
			CSuperSocketPtr toServerTcp = m_remoteServer->m_ToServerTcp;
			assert(toServerTcp != NULL);

			GarbageSocket(toServerTcp);
			m_remoteServer->m_ToServerTcp = CSuperSocketPtr();

			CSuperSocketPtr toServerUdp = m_remoteServer->m_ToServerUdp;
			if (toServerUdp != NULL)
			{
				GarbageSocket(toServerUdp);
				m_remoteServer->m_ToServerUdp = CSuperSocketPtr();
			}

			return false;
		}
		else
		{
			// remote peer가 가진 UDP socket을 recycle bin에 넣고 reference 해지한다.
			CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(remote);

			// 데이터 무결성 보장
			if (remotePeer != NULL)
			{
				assert(remotePeer->m_garbaged);
				assert(remotePeer->IsDisposeSafe());

				// remote peer는 재사용될때 Set_Active(false)로 될 뿐, remote peer와 소유한 UDP socket들은 모두 살아있다.
				// 그런데 remote peer가 garbage된다는 것은, inactive 상태에서의 시한 (수십초)도 끝났음을 의미한다.
				// 그러면 UDP socket도 그냥 다 없앤다.
				CSuperSocketPtr udpSocket = remotePeer->m_udpSocket;
				if (udpSocket != NULL)
				{
					GarbageSocket(udpSocket);
					remotePeer->m_udpSocket = CSuperSocketPtr();
				}

				remotePeer->m_owner = NULL;
				remotePeer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
				remotePeer->SetRelayedP2P(true);

				// recycle에 있었으면 거기서도 제거
				m_remotePeerRecycles.RemoveKey(remotePeer->m_HostID);
			}
		}

		// 위 조건문 다 무시하고 여기 왔다면 재접속용 host garbage 시점임
		if (m_autoConnectionRecovery_temporaryRemoteServers.ContainsKey(remote))
		{
			// tempRemoteServer 를 목록에서 삭제
			m_autoConnectionRecovery_temporaryRemoteServers.Remove(remote);
		}

		return true;
	}

	CHostBase* CNetClientImpl::GetTaskSubjectByHostID_NOLOCK(HostID hostID)
	{
		// NetServer,LanClient,LanServer에서만 사용되던 함수. 이 함수는 빈 상태로 둔다.
		// 역시 안되죠. 구현해 주시기 바랍니다.
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (hostID == HostID_None)
			return NULL;

		if (hostID == HostID_Server)
			return m_remoteServer;

		if (hostID == GetVolatileLocalHostID())
			return m_loopbackHost;

		return AuthedHostMap_Get(hostID);
	}

	bool CNetClientImpl::IsValidHostID_NOLOCK(HostID hostID)
	{
		// NetServer,LanClient,LanServer에서만 사용되던 함수. 이 함수는 빈 상태로 둔다.
		// 역시 안되죠. 구현해 주시기 바랍니다.
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);

		/* remoteServer와 loopbackhost는 HostID 발급받은 이후에 CandidateHost에서 AuthedHost로 옮겨집니다.
		즉, HostID를 발급받지 못한 상황이 발생하게되면 (예. ErrorType_TCPConnectFailure) LocalEvent가 정상적으로 발생되지가 않게됩니다.

		=> 이런 문제가 있었군요.
		그렇다면,
		loopback은 항상 존재해야 하고, 자기 자신의 hostID를 아는 이상 항상 valid하다고 리턴해야 합니다.
		remote server도 항상 존재하게 하고 HostID_Server라는 고정값을 갖고 있으므로 서버 연결 여부와 상관없이 항상 valid하다고 리턴하게 합시다.

		즉 remoteServer는 항상 authed에 있어야 하겠고, loopback도 항상 authed에 있게 합시다.
		단, LC,NC에서는 loopback은 자신의 hostID를 모를 때 즉 서버와의 연결이 안 되어 있을 때는 candidates에 있고
		그 외에는 항상 authed에 있게 합시다.

		LC,LS에서도 이렇게 수정해야 합니다. 당장에는 하지 마시고, 뭘 해야 하는지에 대한 메모만 추가해주세요.
		*/
		if (hostID == HostID_None)
			return false;

		if (hostID == HostID_Server)
			return true;

		if (hostID == GetVolatileLocalHostID())
			return true;

		if (AuthedHostMap_Get(hostID) != NULL)
			return true;

		return false;
	}

	void CNetClientImpl::BadAllocException(Exception& ex)
	{
		FreePreventOutOfMemory();
		if (m_eventSink_NOCSLOCK != NULL)
		{
			m_eventSink_NOCSLOCK->OnException(ex);
		}
		m_worker->SetState(CNetClientWorker::Disconnecting);
	}

	bool CNetClientImpl::IsSimplePacketMode()
	{
		return m_simplePacketMode;
	}

	// TCP 클라-연결이 위험한 상태인지, 즉 핑퐁을 포함해서 수신이 지나칠 정도로 지연되고 있는지?
	// NOTE: TCP는 20초 이상 패킷 로스가 심각하면 retransmission timeout 즉 10053 에러가 뜬다.
	// interval: 현재 시간 - 마지막으로 스트림 받은 시간
	bool CNetClientImpl::IsTcpUnstable(int64_t interval)
	{
		int64_t unstableTime = (int64_t)GetReliablePingTimerIntervalMs() + CNetConfig::TcpUnstableDetectionWaitTimeMs;
		if (interval > unstableTime)
			return true;

		return false;
	}

	bool CNetClientImpl::Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult)
	{
		bool processed = stub->ProcessReceivedMessage(receivedMessage, hostTag);

		if (workResult != NULL)
		{
			if (processed && stub != ((IRmiStub*)&m_s2cStub) && ((IRmiStub*)&m_c2cStub))
			{
				++workResult->m_processedMessageCount;
			}
		}

		return processed;
	}

	// 이 함수는 그냥 NetCore로 옮깁시다. 그리고 candidate, authed,dispose issued hosts에 대해서 모두 돌면서 검사하는게 맞을 듯.
	// main lock하는거 잊지 마시고.
	// => m_remoteServer, m_loopbackHost 모두 이미 candidate or authed에 들어가야 합니다. 따라서 NetCore는 m_remoteServer, m_loopbackHost 의 존재를 몰라도 문제 없습니다.
	// NC,LC의 경우: m_loopbackHost는 hostID를 발급받은 후에는 authed에, 그 전에는 candidate에 있어야.
	// NS,LS의 경우: m_loopbackHost는 항상 authed에 있어야. (항상 자기자신은 HostID_Server임)
	// NC,LC의 경우: m_remoteServer 항상 authed에 있어야(항상 HostID_Server니까)

	// 	int CNetClientImpl::GetFinalUserWotkItemCount()
	// 	{
	// 		int workItemCount = 0;
	//
	// 		if (m_remoteServer != NULL)
	// 			workItemCount += m_remoteServer->GetFinalUserWotkItemCount();
	// 		if (m_loopbackHost != NULL)
	// 			workItemCount += m_loopbackHost->GetFinalUserWotkItemCount();
	//
	// 		for (RemotePeers_C::iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); iPeer++)
	// 		{
	// 			CRemotePeer_C* peer = iPeer->GetSecond();
	// 			workItemCount += peer->GetFinalUserWotkItemCount();
	// 		}
	//
	// 		return workItemCount;
	// 	}

	//위에처럼 이것도 NetCore로 모두 옮깁시다.
	// 	void CNetClientImpl::ClearFinalUserWorkItemList()
	// 	{
	// 		if (m_remoteServer != NULL)
	// 			m_remoteServer->ClearFinalUserWorkItemList();
	// 		if (m_loopbackHost != NULL)
	// 			m_loopbackHost->ClearFinalUserWorkItemList();
	//
	// 		for (RemotePeers_C::iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); iPeer++)
	// 		{
	// 			CRemotePeer_C* peer = iPeer->GetSecond();
	// 			peer->ClearFinalUserWorkItemList();
	// 		}
	// 	}
}

#ifdef __linux__
/*
#include "NetClient_NDK.cpp"
*/
#endif



