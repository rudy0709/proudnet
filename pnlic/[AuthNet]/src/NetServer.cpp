/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <stack>
#include "../include/INetServerEvent.h"
#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/P2PGroup.h"
//#include "SendFragRefs.h"
#include "../include/sysutil.h"
//#include "../include/ErrorInfo.h"
#include "../include/MessageSummary.h"

#include "LeanDynamicCast.h"
#include "NetServer.h"
#include "P2PGroup_S.h"
#include "ReaderWriterMonitorTester.h"
#include "ReliableUDP.h"
#include "ReliableUDPFrame.h"
#include "ReliableUdpHelper.h"
#include "Functors.h"
#include "RemoteClient.h"
//#include "ServerSocketPool.h"
#include "TestUdpConnReset.h"
#include "UdpSocket_S.h"
//#include "networker_c.h"
#include "STLUtil.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif
#include "RmiContextImpl.h"
#include "ReportError.h"
#include <typeinfo>

#ifndef _DEBUG
#define HIDE_RMI_NAME_STRING
#endif


#include "NetC2S_stub.cpp"
#include "NetS2C_proxy.cpp"

#include "CollUtil.h"
//#include "LowContextSwitchingLoop.h"
#include "ReusableLocalVar.h"
#include "SendDestInfo.h"
#include "quicksort.h"

#ifndef PROUDSRCCOPY
#include "../PNLic/PNLicenseManager/PNLic.h"
#include "../PNLic/PNLicenseManager/Binary.inl"
#include "../PNLic/PNLicenseManager/PNLic.inl"

// @@@ dev003 배타 릴리즈 위해 넣은 코드 : 삭제 필요
#include "../include/pnguid-win32.h"
#include "../PNLic/PNLicenseManager/RunLicenseApp.inl"

#endif
#include "RunOnScopeOut.h"


#define PARALLEL_RSA_DECRYPTION // 이걸 끄면 RSA 복호화 과정을 병렬화를 안한다. 


namespace Proud
{

	StringA NotFirstRequestText = "!!";

#define ASSERT_OR_HACKED(x) { if(!(x)) { EnqueueHackSuspectEvent(rc,#x,HackType_PacketRig); } }

	int GetChecksum(ByteArray& array)
	{
		int ret = 0;
		for ( int i = 0; i < (int)array.GetCount(); i++ )
		{
			ret += array[i];
		}
		return ret;
	}

	DEFRMI_ProudC2S_NotifyP2PHolepunchSuccess(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		/* ignore if A-B is already passed
		다중 경로로 holepunch가 짧은 시간차로
		다수 성공할 경우 가장 최초의 것만
		선택해주는 중재역할을 서버가 해야 하므로 */

		P2PConnectionStatePtr state;
		state = m_owner->m_p2pConnectionPairList.GetPair(A, B);

		if ( state == NULL )
		{
			NTTNTRACE("경고: P2P 연결 정보가 존재하지 않으면서 홀펀칭이 성공했다고라고라?\n");
		}
		else
		{
			// 직접 연결이 되었다는 신호를 보내도록 한다.
			if ( state->IsRelayed() == true )
			{
				state->SetRelayed(false);

/*				if (m_owner->m_verbose_TEMP)
				{
					printf("Hole punch between %d and %d ok!\n", A, B);
				}*/

				/* 예전에는 받은 값들을 홀펀칭 정보를 재사용하기 위해 저장했지만, 
				이제는 괜히 서버 메모리만 먹고... 저장 안한다.
				이제는 홀펀칭 정보 재사용을 위해서 서버의 remote client,
				클라의 remote peer 및 UDP socket을 일정 시간 preserve하는 방식으로 바뀌었기 때문이다. */

				// pass it to A and B
				m_owner->m_s2cProxy.NotifyDirectP2PEstablish(A, g_ReliableSendForPN, A, B, ABSendAddr, ABRecvAddr, BASendAddr, BARecvAddr);
				m_owner->m_s2cProxy.NotifyDirectP2PEstablish(B, g_ReliableSendForPN, A, B, ABSendAddr, ABRecvAddr, BASendAddr, BARecvAddr);

				// 로그를 남긴다.
				if ( m_owner->m_logWriter )
				{
					String txt;
					txt.Format(
						_PNT("The Final Stage of P2P UDP Hole-Punching between Client %d and Client %d Succeeded: ABSendAddr=%s ABRecvAddr=%s BASendAddr=%s BARecvAddr=%s"),
						A, B,
						ABSendAddr.ToString().GetString(),
						ABRecvAddr.ToString().GetString(),
						BASendAddr.ToString().GetString(),
						BARecvAddr.ToString().GetString());
					m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
				}

			}
		}
		return true;
	}

	DEFRMI_ProudC2S_P2P_NotifyDirectP2PDisconnected(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		NTTNTRACE("%d-%d P2P connection lost. Reason=%d\n", remote, remotePeerHostID, reason);
		// check validation
		CRemoteClient_S *rc = m_owner->GetAuthedClientByHostID_NOLOCK(remotePeerHostID);
		if ( rc != NULL )
		{
			// 서버에 저장된 두 클라간 연결 상태 정보를 갱신한다.
			P2PConnectionStatePtr state = m_owner->m_p2pConnectionPairList.GetPair(remote, remotePeerHostID);
			if ( state != NULL && !state->IsRelayed() )
			{
				state->SetRelayed(true);

				// ack를 날린다.
				m_owner->m_s2cProxy.P2P_NotifyDirectP2PDisconnected2(remotePeerHostID, g_ReliableSendForPN, remote, reason);

				// 로그를 남긴다
				if ( m_owner->m_logWriter )
				{
					String txt;
					txt.Format(_PNT("Cancelled P2P UDP Hole-Punching between Client %d and Client %d"), remote, remotePeerHostID);
					m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
				}
			}
		}
		return true;
	}

	DEFRMI_ProudC2S_NotifyNatDeviceNameDetected(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S *rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_natDeviceName = natDeviceName;
		}

		return true;
	}

	DEFRMI_ProudC2S_NotifyJitDirectP2PTriggered(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// check validation
		CRemoteClient_S *rc = m_owner->GetAuthedClientByHostID_NOLOCK(peerB_ID);
		if ( rc != NULL )
		{
			// 서버에 저장된 두 클라간 연결 상태 정보를 완전히 초기화한다.
			// 이미 Direct P2P로 지정되어 있어도 말이다.
			P2PConnectionState* state = m_owner->m_p2pConnectionPairList.GetPair(remote, peerB_ID);
			if ( state != NULL )
			{
				if ( state->m_firstClient->m_p2pConnectionPairs.GetCount() < state->m_firstClient->m_maxDirectP2PConnectionCount 
					&& state->m_secondClient->m_p2pConnectionPairs.GetCount() < state->m_secondClient->m_maxDirectP2PConnectionCount )
				{
					// UDP Socket이 생성되어있지 않다면, UDP Socket 생성을 요청한다.
					// (함수 안에서, S2C_RequestCreateUdpSocket도 전송함.)
					m_owner->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(state->m_firstClient);
					m_owner->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(state->m_secondClient);

					// (reliable messaging이므로, 연이어 아래 요청을 해도 순서 보장된다. 아래 요청을 받는 클라는 이미 UDP socket이 있을테니까.)
					// 양측 클라 모두에게, Direct P2P 시작을 해야하는 조건이 되었으니 서로 홀펀칭을 시작하라는 지령을 한다.
					m_owner->m_s2cProxy.NewDirectP2PConnection(peerB_ID, g_ReliableSendForPN, remote);
					m_owner->m_s2cProxy.NewDirectP2PConnection(remote, g_ReliableSendForPN, peerB_ID);
					state->m_jitDirectP2PRequested = true;

					// 로그를 남긴다.
					if ( m_owner->m_logWriter )
					{
						String txt;
						txt.Format(_PNT("Start Conditions of P2P UDP Hole-Punching between Client %d and Client %d Established. Now Command Hole-Punching Starts between Two Peers."), remote, peerB_ID);
						m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
					}
				}
			}
		}
		return true;
	}

	DEFRMI_ProudC2S_NotifySendSpeed(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// check validation
		CRemoteClient_S *rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_sendSpeed = speed;
		}
		return true;
	}


	DEFRMI_ProudC2S_NotifyLog(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		if ( m_owner->m_logWriter != NULL )
		{
			m_owner->m_logWriter->WriteLine(logLevel, logCategory, logHostID, logMessage, logFunction, logLine);
		}

		return true;
	}

	DEFRMI_ProudC2S_NotifyLogHolepunchFreqFail(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CNetServerStats stat;
		m_owner->GetStats(stat);

		if ( m_owner->m_logWriter != NULL )
		{
			String txt2;
			txt2.Format(_PNT("[Client %d] Pair=%d/%d##CCU=%d##"),
						remote,
						stat.m_p2pDirectConnectionPairCount,
						stat.m_p2pConnectionPairCount,
						stat.m_clientCount);

			m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt2 + text);  // printf가 너무 길어지므로
		}

		// 가장 랭킹 높게 측정된 로그인지만 체크해서 저장한다. 아직 기록은 아님.
		CRemoteClient_S* rc;
		if ( m_owner->m_freqFailLogMostRank < rank && (rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote)) != NULL )
		{
			String txt2;
			txt2.Format(_PNT("[Client %d %s] Pair=%d/%d##CCU=%d"),
						remote,
						rc->m_tcpLayer->m_remoteAddr.ToString().GetString(),
						stat.m_p2pDirectConnectionPairCount,
						stat.m_p2pConnectionPairCount,
						stat.m_clientCount);

			m_owner->m_freqFailLogMostRank = rank;
			m_owner->m_freqFailLogMostRankText = txt2 + text; // printf가 너무 길어지므로

			AtomicCompareAndSwap32(0, 1, &m_owner->m_fregFailNeed);
		}
		return true;
	}

	// 클라에서 먼저 UDP 핑 실패로 인한 fallback을 요청한 경우
	DEFRMI_ProudC2S_NotifyUdpToTcpFallbackByClient(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// 해당 remote client를 업데이트한다.
		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			m_owner->SecondChanceFallbackUdpToTcp(rc);
		}

		return true;
	}

	DEFRMI_ProudC2S_ReliablePing(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			//rc->m_safeTimes.Set_lastTcpStreamReceivedTime(m_owner->GetCachedServerTimeMs());
			// 사용자가 입력한 frameRate update

			int ackNum = 0;

			// ACR 이 켜져 있을때만 아래 부분을 처리하는것이 좋은데 문제는 Server 측에서 알 수 있는 방법이 없음
			// 일단 messageID 가 0 이 아니라면 ACR On 으로 간주하고 처리 하는데
			// 만약 0 값도 valid 한 값이라면 문제가 있다.
			if ( messageID != 0 )
			{
				// 수신 미보장 부분중 해당 부분을 제거
				rc->m_tcpLayer->AcrMessageRecovery_RemoveBeforeAckedMessageID(messageID);
				rc->m_tcpLayer->AcrMessageRecovery_PeekMessageIDToAck(&ackNum);
			}

			rc->m_lastApplicationHint.m_recentFrameRate = recentFrameRate;
			m_owner->m_s2cProxy.ReliablePong(remote, g_ReliableSendForPN, localTimeMs, ackNum);
		}
		return true;
	}

	DEFRMI_ProudC2S_ShutdownTcp(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_shutdownComment = comment;
			rc->m_enableAutoConnectionRecovery = false;
			m_owner->m_s2cProxy.ShutdownTcpAck(remote, g_ReliableSendForPN);
		}
		return true;
	}

	DEFRMI_ProudC2S_NotifyNatDeviceName(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);
		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_natDeviceName = deviceName;
		}
		return true;
	}

	DEFRMI_ProudC2S_ReportP2PPeerPing(CNetServerImpl::C2SStub)
	{
		// 어차피 congestion control이 들어가야 함. 20초보다는 빨리 디스를 감지해야 하므로.
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		if ( recentPing >= 0 )
		{
			CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
			if ( rc != NULL )
			{
				// 두 클라의 P2P 연결 정보에다가 레이턴시를 기록한다.
				for ( CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin();
					 iPair != rc->m_p2pConnectionPairs.end();
					 iPair++ )
				{
					P2PConnectionState* pair = *iPair;
					if ( pair->ContainsHostID(peerID) )
						pair->m_recentPingMs = recentPing;
				}
			}
		}
		return true;
	}

	DEFRMI_ProudC2S_C2S_RequestCreateUdpSocket(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if ( NULL != rc )
		{
			m_owner->RemoteClient_New_ToClientUdpSocket(rc);
			bool udpExists = rc->m_ToClientUdp_Fallbackable.m_udpSocket ? true : false;
			
			NamedAddrPort sockAddr;

			if ( udpExists )
				sockAddr = rc->GetRemoteIdentifiableLocalAddr();

			// 상대측 요청에 의해 UDP 소켓 생성 끝. 상대는 이미 있을터이고. 따라서 ack를 노티.
			m_owner->m_s2cProxy.S2C_CreateUdpSocketAck(remote, g_ReliableSendForPN, udpExists, sockAddr);

			if ( udpExists )
			{
				m_owner->RemoteClient_RequestStartServerHolepunch_OnFirst(rc);
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_C2S_CreateUdpSocketAck(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if ( NULL != rc )
		{
			//m_owner->RemoteClient_New_ToClientUdpSocket(rc);
			rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting = false;

			if ( succeed )
			{
				m_owner->RemoteClient_RequestStartServerHolepunch_OnFirst(rc);
			}
			else
			{
				// 클라이언트의 udp 소켓생성이 실패 했다면 udp통신을 하지 않는다.

				// comment by ulelio : 여기서 refCount가 2여야 정상임. 
				// perclient일때는 m_ownedudpsocket이 있고 static일때는 udpsockets가 있기때문..
				if ( m_owner->m_logWriter )
				{
					String text;
					text.Format(_PNT("Failed to create UDP socket of Client. HostID=%d, udpSocketRefCount=%d"), rc->m_HostID,
								(int)rc->m_ToClientUdp_Fallbackable.m_udpSocket.GetRefCount());

					m_owner->m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, text);
				}

				// modify by ulelio : udpsocket이 없으므로 realudpEnable이 false다.
				rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled = false;
				rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
				rc->m_ToClientUdp_Fallbackable.m_udpSocket = CSuperSocketPtr();
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_ReportC2CUdpMessageCount(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if ( NULL != rc && peer != HostID_None )
		{
			// 받은 packet 갯수를 업데이트한다.
			rc->m_toRemotePeerSendUdpMessageTrialCount += udpMessageTrialCount;
			rc->m_toRemotePeerSendUdpMessageSuccessCount += udpMessageSuccessCount;

			for ( CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin(); iPair != rc->m_p2pConnectionPairs.end(); iPair++ )
			{
				P2PConnectionState* pair = *iPair;
				if ( pair->ContainsHostID(peer) )
				{
					pair->m_toRemotePeerSendUdpMessageTrialCount = udpMessageTrialCount;
					pair->m_toRemotePeerSendUdpMessageSuccessCount = udpMessageSuccessCount;
				}
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_ReportC2SUdpMessageTrialCount(CNetServerImpl::C2SStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		CRemoteClient_S* rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if ( rc != NULL )
			rc->m_toServerSendUdpMessageTrialCount = toServerUdpTrialCount;

		return true;
	}

	bool CNetServerImpl::GetJoinedP2PGroups(HostID clientHostID, HostIDArray &output)
	{
		output.Clear();

		if ( clientHostID == HostID_None )
			return false;

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CHostBase* hb = AuthedHostMap_Get(clientHostID);
		if ( hb == NULL )
			return false;

		CRemoteClient_S* c = LeanDynamicCastForRemoteClient(hb);
		if ( c == NULL )
			return false;

		for ( JoinedP2PGroups_S::iterator ii = c->m_joinedP2PGroups.begin(); ii != c->m_joinedP2PGroups.end(); ii++ )
		{
			HostID i = ii->GetFirst();
			output.Add(i);
		}
		return true;
	}

	void CNetServerImpl::GetP2PGroups(CP2PGroups &ret)
	{
		ret.Clear();

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		for ( P2PGroups_S::iterator ig = m_P2PGroups.begin(); ig != m_P2PGroups.end(); ig++ )
		{
			CP2PGroupPtr_S g = ig->GetSecond();
			CP2PGroupPtr gd = g->ToInfo();
			ret.Add(gd->m_groupHostID, gd);
		}
	}

	int CNetServerImpl::GetP2PGroupCount()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		return (int)m_P2PGroups.GetCount();
	}

	CRemoteClient_S* CNetServerImpl::GetRemoteClientBySocket_NOLOCK(CSuperSocket* socket, const AddrPort& remoteAddr)
	{
		AssertIsLockedByCurrentThread();

		CHostBase* hostBase = SocketToHostsMap_Get_NOLOCK(socket, remoteAddr);

		return LeanDynamicCastForRemoteClient(hostBase);
	}

	void CNetServerImpl::PurgeTooOldAddMemberAckItem()
	{
		//try
		{
			CriticalSectionLock clk(GetCriticalSection(), true);
			CHECK_CRITICALSECTION_DEADLOCK(this);

			// clear ack-info if it is too old
			int64_t currTime = GetPreciseCurrentTimeMs();

			for ( P2PGroups_S::iterator ig = m_P2PGroups.begin(); ig != m_P2PGroups.end(); ig++ )
			{
				CP2PGroupPtr_S g = ig->GetSecond();
				for ( int im = 0; im < (int)g->m_addMemberAckWaiters.GetCount(); im++ )
				{
					CP2PGroup_S::AddMemberAckWaiter& ack = g->m_addMemberAckWaiters[im];
					if ( currTime - ack.m_eventTime > CNetConfig::PurgeTooOldAddMemberAckTimeoutMs )
					{
						HostID joiningPeerID = ack.m_joiningMemberHostID;
						g->m_addMemberAckWaiters.RemoveAt(im); // 이거 하나만 지우고 리턴. 나머지는 다음 기회에서 지워도 충분하니까.

						// 너무 오랫동안 ack가 안와서 제거되는 항목이 영구적 콜백 없음으로 이어져서는 안되므로 아래가 필요
						if ( !g->m_addMemberAckWaiters.AckWaitingItemExists(joiningPeerID) )
						{
							EnqueAddMemberAckCompleteEvent(g->m_groupHostID, joiningPeerID, ErrorType_ConnectServerTimeout);
						}
						return;
					}
				}
			}
		}
		// 		catch (...)	//사용자 정의 루틴을 콜 하는 곳이 없으므로 주석화
		// 		{
		// 			if(m_logWriter)
		// 			{				
		// 				m_logWriter->WriteLine(TID_System, _PNT("FATAL ** Unhandled Exception at PurgeTooOldUnmatureClient!"));
		// 			}
		// 			throw;
		// 		}
	}

	// 너무 오랫동안 unmature 상태를 못 벗어나는 remote client들을 청소.]
	void CNetServerImpl::PurgeTooOldUnmatureClient()
	{
		CriticalSectionLock mainlock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		intptr_t listCount = m_candidateHosts.GetCount();
		if ( listCount <= 0 )
			return;

		POOLED_ARRAY_LOCAL_VAR(CSuperSocketArray, timeoutSockets);
		
		int64_t currTime = GetPreciseCurrentTimeMs();

		// 중국의 경우 TcpSocketConnectTimeoutMs * 2 + 3 일때 63이 나오므로 상향조정함.
		assert(CNetConfig::ClientConnectServerTimeoutTimeMs < 70000);

		for ( CFastMap2<CHostBase*, CHostBase*, int>::iterator i = m_candidateHosts.begin(); i != m_candidateHosts.end(); i++ )
		{
			// 아니 이런 무작정 다운캐스트를 하다니! 수정 바랍니다.
			// dynamic_cast를 사용하세요. dynamic_cast도 성능 문제로 가급적 피하는 것이 좋긴 합니다만.
			CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(i->GetSecond());
			if ( currTime - rc->m_createdTime > CNetConfig::ClientConnectServerTimeoutTimeMs && rc->m_purgeRequested == false )
			{
				// UseCount + 1
				rc->m_tcpLayer->IncreaseUseCount();
				rc->m_purgeRequested = true;

				// 조건을 만족하는 rc들만 list에 추가하자.
				timeoutSockets.Add(rc->m_tcpLayer);
			}
		}

		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_ConnectServerTimedout);
		CSendFragRefs sd(sendMsg);

		// main unlock
		mainlock.Unlock();

		AddToSendQueueWithSplitterAndSignal_Copy_Functor<CRemoteClient_S*> functor(&sd);
		LowContextSwitchingLoop(timeoutSockets.GetData(), timeoutSockets.GetCount(), functor);
	}

	bool CNetServerImpl::OnHostGarbageCollected(CHostBase *remote)
	{
		AssertIsLockedByCurrentThread();

		if ( remote == m_loopbackHost )
			return false;

		// 목록에서 제거.
		m_HostIDFactory->Drop(remote->m_HostID);

		CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(remote);

		// OnHostGarbaged가 아니라 OnHostGarbageCollected에서 GarbageSocket을 호출해야 합니다.
		// 수정 바랍니다.
		// 단, socket to host map에서의 제거는 OnHostGarbaged에서 하는 것이 맞습니다.
		CSuperSocketPtr tcpSocket = rc->m_tcpLayer;

		// TCP socket을 garbage 한다.

		// 기존 tcpSocket 이 NULL 일때 assert 가 나도록 되있었는데 ACR 의 RC는 tcpLayer 가 제거 되어 해당 함수가 호출 된다.
		// 따라서 아래처럼 코드를 변경
		// dev003 에서 OnHostGarbageCollected 의 behavior 가 바뀌어 문제가 생김
		if ( tcpSocket != NULL )
		{
#ifndef PROUDSRCCOPY
			((CPNLic*)m_lic)->POCL(rc->m_RT, rc->m_tcpLayer->m_remoteAddr);
#endif
			GarbageSocket(tcpSocket);
			rc->m_tcpLayer = CSuperSocketPtr();
		}

		// remote client가 폐기되는 상황. 즉 ACR이 아니다. 따라서 UDP socket을 recycle이 아닌 garbage를 해 버리자.
		RemoteClient_CleanupUdpSocket(rc, true);

		Unregister(rc);

		return true;
	}

	CSessionKey *CNetServerImpl::GetCryptSessionKey(HostID remote, String &errorOut, bool& outEnqueError)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);

		CSessionKey *key = NULL;

		if ( rc != NULL )
		{
			key = &rc->m_sessionKey;
		}
		else if ( remote == HostID_Server )
		{
			key = &m_selfSessionKey;
		}

		if ( key == NULL )
		{
			// error 처리를 하지 않고 무시한다.
			errorOut.Format(_PNT("client %d is %s in NetServer!"), (int)remote, rc == NULL ? _PNT("NULL") : _PNT("not NULL"));
			outEnqueError = false;

			return NULL;
		}

		if ( !key->EveryKeyExists() )
		{
			// error 처리를 해주어야 한다.
			errorOut.Format(_PNT("Key does not exist. Note that P2P encryption can be enabled on NetServer.Start()."));
			outEnqueError = true;

			return NULL;
		}

		return key;
	}

	bool CNetServerImpl::Send(const CSendFragRefs &sendData,
							  const SendOpt& sendContext,
							  const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength)
	{
		/* 네트워킹 비활성 상태이면 무조건 그냥 아웃.
		여기서 사전 검사를 하는 이유는, 아래 하위 callee들은 많은 validation check를 하는데, 그걸
		다 거친 후 안보내는 것 보다는 앗싸리 여기서 먼저 안보내는 것이 성능상 장점이니까.*/
		if ( !m_listening )
			//if (!m_listenerThread)
			return false;

		// 설정된 한계보다 큰 메시지인 경우
		if ( sendData.GetTotalLength() > m_settings.m_serverMessageMaxLength )
			throw Exception("Too long message cannot be sent. (Size=%d)", sendData.GetTotalLength());

		// 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
		// 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
		return Send_CompressLayer(sendData,
								  sendContext,
								  sendTo, numberOfsendTo, compressedPayloadLength);
	}

	bool CNetServerImpl::Send_BroadcastLayer(
		const CSendFragRefs& payload,
		const CSendFragRefs* encryptedPayload,
		const SendOpt& sendContext,
		const HostID *sendTo, int numberOfsendTo)
	{
		// 		sendContextWithoutUniqueID = sendContext;
		// 		sendContextWithoutUniqueID.m_uniqueID = 0;
		// modify by ulelio : lock을 이제 여기서 따로 건다.
		CriticalSectionLock mainlock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// NOTE: maxDirectBroadcastCount는 서버에서 안쓰임
		int unreliableS2CRoutedMulticastMaxCount = max(sendContext.m_unreliableS2CRoutedMulticastMaxCount, 0);
		int unreliableS2CRoutedMulticastMaxPing = max(sendContext.m_unreliableS2CRoutedMulticastMaxPingMs, 0);

		// P2P 그룹을 HostID 리스트로 변환, 즉 ungroup한다.		
		POOLED_ARRAY_LOCAL_VAR(HostIDArray, sendDestList0);

		ConvertGroupToIndividualsAndUnion(numberOfsendTo, sendTo, sendDestList0);

		// 각 수신 대상의 remote host object를 얻는다.
		POOLED_ARRAY_LOCAL_VAR(SendDestInfoArray, sendDestList);
		
		Convert_NOLOCK(sendDestList, sendDestList0);

		int sendDestListCount = (int)sendDestList.GetCount();

		// 실제로 보낼 메시지. 
		const CSendFragRefs* realPayload = NULL;

		int64_t curTime = GetPreciseCurrentTimeMs();

		if ( encryptedPayload != NULL )
		{
			// 압축 또는 암호화 또는 둘다 적용된 메시지일 경우 securedPayload 는 NULL 이 아니다. securedPayload 를 보낸다. (loopback 제외)
			realPayload = encryptedPayload;
		}
		else
		{
			//securedPayload 가 NULL 일 경우 압축이나 암호화 가 적용된 메시지가 아니다. 원본을 보낸다.
			realPayload = &payload;
		}

		// main lock이 있는 상태에서 수행해야 할것들을 처리하자
		if ( sendContext.m_reliability == MessageReliability_Reliable )
		{
			//int reliableCount = 0;
			POOLED_ARRAY_LOCAL_VAR(CSuperSocketArray, reliableSendList);

			for ( int i = 0; i < sendDestListCount; i++ )
			{
				SendDestInfo& SD1 = sendDestList[i];
				CHostBase *SD2 = SD1.mObject;

				// if loopback
				if ( SD2 == m_loopbackHost && sendContext.m_enableLoopback )
				{
					// loopback 메시지는 압축, 암호화가 적용되어 있으면 메시지가 버려진다. 원본 메시지를 보낸다.
					// 즉시 final user work로 넣어버리자. 루프백 메시지는 사용자가 발생하는 것 말고는 없는 것을 전제한다.
					CMessage payloadMsg;
					payload.ToAssembledMessage(payloadMsg);

					CReceivedMessage ri;
					ri.m_remoteHostID = HostID_Server;
					ri.m_unsafeMessage = payloadMsg;
					ri.m_unsafeMessage.SetReadOffset(0);
					UserTaskQueue_Add(m_loopbackHost, ri, GetWorkTypeFromMessageHeader(payloadMsg), true);
					
          
          
/*          //3003!!!
          
          MessageInternalLayer match;
					match.m_simplePacketMode = m_simplePacketMode;
					match.Analyze(ri.m_unsafeMessage);
					if (match.m_rmiLayer.m_rmiID == 3003)
					{
						assert(0);
					}*/

				}
				else if ( SD2 != NULL && LeanDynamicCastForRemoteClient(SD2) != NULL )
				{
					CRemoteClient_S *SD3 = (CRemoteClient_S*)SD2;
					// main lock 상태이므로 아래 변수는 중도 증발 안하니 걱정 뚝
					if ( SD3->m_tcpLayer != NULL )
					{
						SD3->m_tcpLayer->IncreaseUseCount();
						reliableSendList.Add(SD3->m_tcpLayer);
						// 						reliableSendList[reliableCount] = SD3->m_tcpLayer;
						// 						reliableCount++;
					}
				}
			}

			mainlock.Unlock();

			// reliable message 수신자들에 대한 처리
			for ( int i = 0; i < reliableSendList.GetCount(); i++ )
			{
				CSuperSocket *SD3 = reliableSendList[i];
				SD3->AddToSendQueueWithSplitterAndSignal_Copy(
					*realPayload, SendOpt(), m_simplePacketMode);

				SD3->DecreaseUseCount();
			}
		}
		else // MessageReliability_Unreliable
		{
			POOLED_ARRAY_LOCAL_VAR(SendDestInfoPtrArray, unreliableSendInfoList);

			// HostID 리스트간에, P2P route를 할 수 있는 것들끼리 묶는다. 단, unreliable 메시징인 경우에 한해서만.
			MakeP2PRouteLinks(sendDestList, unreliableS2CRoutedMulticastMaxCount, unreliableS2CRoutedMulticastMaxPing);

			// 각 수신자에 대해...
			int sendDestListCount = (int)sendDestList.GetCount();
			for ( int i = 0; i < sendDestListCount; i++ )
			{
				SendDestInfo& SD1 = sendDestList[i];
				CHostBase *SD2 = SD1.mObject;

				// if loopback
				if ( SD2 == m_loopbackHost && sendContext.m_enableLoopback )
				{
					// loopback 메시지는 압축, 암호화가 적용되어 있으면 메시지가 버려진다. 원본 메시지를 보낸다.
					// enqueue final recv queue and signal
					CMessage payloadMsg;
					payload.ToAssembledMessage(payloadMsg);

					CReceivedMessage ri;
					ri.m_remoteHostID = HostID_Server;
					ri.m_unsafeMessage = payloadMsg;
					ri.m_unsafeMessage.SetReadOffset(0);
					UserTaskQueue_Add(m_loopbackHost, ri, GetWorkTypeFromMessageHeader(payloadMsg), true);
					
          
          
          
          
/*          //3003!!!
          MessageInternalLayer match;
					match.m_simplePacketMode = m_simplePacketMode;
					if (match.m_rmiLayer.m_rmiID == 3003)
					{
						assert(0);
					}*/




				}
				else if ( SD2 != NULL && LeanDynamicCastForRemoteClient(SD2) != NULL )
				{
					// remote가 가진 socket을 비파괴 보장 시킨 후 수신 대상 목록만 만든다.
					// 나중에 main lock 후 low context switch loop를 돌면서 각 socket에 대해 멀티캐스트를 한다.
					// PN의 멀티캐스트 관련 서버 성능이 빵빵한 이유가 여기에 있다.
					CRemoteClient_S *SD3 = (CRemoteClient_S*)SD2;

					// udp Socket이 생성되어있지 않다면, udp Socket 생성을 요청한다.
					RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(SD3);

					if ( SD3->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
					{
						// UDP로 보내기
						SD3->m_ToClientUdp_Fallbackable.m_udpSocket->IncreaseUseCount();
						sendDestList[i].m_socket = SD3->m_ToClientUdp_Fallbackable.m_udpSocket;
						sendDestList[i].mDestAddr = SD3->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
					}
					else // TCP
					{
						// TCP fallback되어 있다. TCP를 대신 사용하자.
						SD3->m_tcpLayer->IncreaseUseCount();
						sendDestList[i].m_socket = SD3->m_tcpLayer;

						// NOTE: main lock을 한 상태이어야 한다. 자세한 것은 ACR 명세서 참고.
						AssertIsLockedByCurrentThread();
					}

					unreliableSendInfoList.Add(&(sendDestList[i]));
				}
			}

			/* main unlock을 하는 이유:
			이 이후부터는 low context switch loop가 돈다.
			제아무리 low context라고 해도 다수의 socket들의 send queue lock을 try or block lock을 하기 때문에
			context switch가 결국 여러 차례 발생한다.
			따라서 main lock이 많은 시간을 차지할 가능성이 낮게나마 존재한다.
			이를 없애기 위한 목적이다. */
			mainlock.Unlock();

			// 각 unreliable message에 대해...
			for ( int i = 0; i < unreliableSendInfoList.GetCount(); i++ )
			{
				// 이미 위에서 객체가 있음을 보장하기때문에 따로 검사를 수행하지 않는다.
				SendDestInfo* SDOrg = unreliableSendInfoList[i];
				assert(SDOrg != NULL);
				SendDestInfo* SD1 = SDOrg;
				CHostBase *SD2 = SD1->mObject;
				CRemoteClient_S *SD3 = (CRemoteClient_S*)SD2;

				// 항목의 P2P route prev link가 있으면, 즉 이미 P2P route로 broadcast가 된 상태이다.
				// 그러므로, 넘어간다.
				if ( SD1->mP2PRoutePrevLink != NULL )
				{
					// 아무것도 안함
				}
				else if ( SD1->mP2PRouteNextLink != NULL ) // 항목의 P2P next link가 있으면, 
				{
					// link의 끝까지 찾아서 P2P route 메시지 내용물에 추가한다.
					CMessage header;
					header.UseInternalBuffer();
					header.Write((char)MessageType_S2CRoutedMulticast1);

					header.Write(sendContext.m_priority);
					header.WriteScalar(sendContext.m_uniqueID.m_value);

					POOLED_ARRAY_LOCAL_VAR(HostIDArray, p2pRouteList);
					
					while ( SD1 != NULL )
					{
						p2pRouteList.Add(SD1->mHostID);
						SD1 = SD1->mP2PRouteNextLink;
					}

					header.WriteScalar((int)p2pRouteList.GetCount());
					for ( int routeListIndex = 0; routeListIndex < (int)p2pRouteList.GetCount(); routeListIndex++ )
					{
						header.Write(p2pRouteList[routeListIndex]);
					}

					// #ifndef XXXXXXXXXXXXXXX
					// 					printf("S2C routed broadcast to %d: ",SD3->m_HostID);
					// 					for (int i = 0;i < p2pRouteList.GetCount();i++)
					// 					{
					// 						printf("%d ",p2pRouteList[i]);
					// 					}
					// 					printf("\n");
					// #endif

					// sendData의 내용을 추가한다.
					header.WriteScalar(realPayload->GetTotalLength());

					CSendFragRefs s2cRoutedMulticastMsg;
					s2cRoutedMulticastMsg.Add(header);
					s2cRoutedMulticastMsg.Add(*realPayload);

					SendOpt sendContext2(sendContext);
					sendContext2.m_uniqueID.m_relayerSeparator = (char)UniqueID_RelayerSeparator_RoutedMulticast1;
					// hostid_none는 uniqueid로 씹히지 않는다.
					SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
						HostID_None,
						FilterTag::CreateFilterTag(HostID_Server, SD3->GetHostID()),
						SDOrg->mDestAddr,
						s2cRoutedMulticastMsg,
						curTime, //Send함수는 속도에 민감합니다. 매번 GetPreciseCurrentTimeMs호출하지 말고 미리 변수로 받아서 쓰세요. 아래도 마찬가지.
						sendContext);
				}
				else
				{
					// 항목의 P2P link가 전혀 없다. 그냥 보내도록 한다.
					if ( SDOrg->m_socket->GetSocketType() == SocketType_Tcp )
					{
						SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
							*realPayload,
							sendContext);
					}
					else
					{
						SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
							SD3->GetHostID(),
							FilterTag::CreateFilterTag(HostID_Server, SD3->GetHostID()),
							SDOrg->mDestAddr,
							*realPayload,
							curTime,
							sendContext);
					}
				}

				SDOrg->m_socket->DecreaseUseCount();
			}

		}

		return true;
	}

	bool CNetServerImpl::CloseConnection(HostID clientHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(clientHostID);
		if ( rc == NULL )
			return false;

		if ( m_logWriter )
		{
			String text;
			text.Format(_PNT("Call CNetServer.CloseConnection(Client: %d)."), clientHostID);

			m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, text);
		}

		RequestAutoPrune(rc);

		return true;
	}

	void CNetServerImpl::CloseEveryConnection()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
		{
			CHostBase* hb = i->GetSecond();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( hb->GetHostID() == HostID_Server )
				continue;

			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(hb);

			if ( rc != NULL )
			{
				//dispose로 들어간 rc는 requestAutoPrune하지 않는다.
				if ( rc->m_disposeWaiter != NULL )
					continue;

				RequestAutoPrune(rc);
			}
		}
	}

	// 	CRemoteClient_S *CNetServerImpl::GetRemoteClientByHostID_NOLOCK(HostID clientHostID)
	// 	{
	// 		AssertIsLockedByCurrentThread();
	// 		//CriticalSectionLock clk(GetCriticalSection(), true);
	// 		CRemoteClient_S *rc = NULL;
	// 		m_authedHosts.TryGetValue(clientHostID, rc);
	// 		return rc;
	// 	}

	CRemoteClient_S * CNetServerImpl::GetAuthedClientByHostID_NOLOCK(HostID clientHostID)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock clk(GetCriticalSection(), true);

		if ( clientHostID == HostID_None )
			return NULL;

		if ( clientHostID == HostID_Server )
			return NULL;

		CRemoteClient_S *rc = LeanDynamicCastForRemoteClient(AuthedHostMap_Get(clientHostID));
		//		m_authedHosts.TryGetValue(clientHostID, rc);

		if ( rc != NULL && rc->m_disposeWaiter == NULL )
			return rc;

		return NULL;
	}

	int CNetServerImpl::GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* peer = GetAuthedClientByHostID_NOLOCK(peerHostID);  //여기에 일단 백업
		if ( peer == NULL )
			return -1;
		else
		{
			return peer->m_lastPingMs;
		}
	}

	int  CNetServerImpl::GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* peer = GetAuthedClientByHostID_NOLOCK(peerHostID);  //여기에 일단 백업
		if ( peer == NULL )
			return -1;
		else
		{
			return peer->m_recentPingMs;
		}
	}

	int CNetServerImpl::GetP2PRecentPingMs(HostID HostA, HostID HostB)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* rc = GetAuthedClientByHostID_NOLOCK(HostA);
		if ( rc != NULL )
		{
			for ( CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin(); iPair != rc->m_p2pConnectionPairs.end(); iPair++ )
			{
				P2PConnectionState* pair = *iPair;
				if ( pair->ContainsHostID(HostB) )
				{
					if ( pair->IsRelayed() )
					{
						CRemoteClient_S *rc2 = pair->m_firstClient;
						if ( rc2 == rc )
							rc2 = pair->m_secondClient;

						return rc->m_recentPingMs + rc2->m_recentPingMs;
					}
					else
						return pair->m_recentPingMs;
				}
			}
		}
		return 0;
	}

	bool CNetServerImpl::DestroyP2PGroup(HostID groupHostID)
	{
		// 모든 멤버를 다 쫓아낸다.
		// 그러면 그룹은 자동 소멸한다.
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CP2PGroupPtr_S g = GetP2PGroupByHostID_NOLOCK(groupHostID);
		if ( g == NULL )
			return false;

		while ( g->m_members.size() > 0 )
		{
			LeaveP2PGroup((g->m_members.begin())->first, g->m_groupHostID);
		}

		// 다 끝났다. 이제 P2P 그룹 자체를 파괴해버린다. 
		if ( m_P2PGroups.Remove(groupHostID) )
		{
			//add by rekfkno1 - hostid를 drop한다.재사용을 위해.
			m_HostIDFactory->Drop(groupHostID);
			EnqueueP2PGroupRemoveEvent(groupHostID);
		}

		return true;
	}

	HostID CNetServerImpl::CreateP2PGroup(const HostID* clientHostIDList0,
										  int count, ByteArray customField,
										  CP2PGroupOption &option,
										  HostID assignedHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// 빈 그룹도 만드는 것을 허용한다. (옵션에 따라)
		if ( count < 0 || m_HostIDFactory == NULL )
			return HostID_None;

		if ( !m_allowEmptyP2PGroup && count == 0 )
			return HostID_None;

		CFastArray<HostID, false, true, int> clientHostIDList;

		if ( count > 0 )
		{
			// 클라 목록에서 중복 항목을 제거한다.
			clientHostIDList.CopyFrom(clientHostIDList0, count);
			UnionDuplicates<CFastArray<HostID, false, true, int>, HostID, int>(clientHostIDList);
			count = (int)clientHostIDList.GetCount();

			// 클라이언트 유효성 체크
			for ( int i = 0; i < count; i++ )
			{
				// 2009.11.02 add by ulelio : Server인경우 클라이언트가 아니기때문에 유효성 체크 넘어감
				if ( clientHostIDList[i] == HostID_Server )
					continue;

				CRemoteClient_S *peer = GetAuthedClientByHostID_NOLOCK(clientHostIDList[i]);
				if ( peer == NULL )
					return HostID_None;

				/*if( peer->m_sessionKey.KeyExists() == false )
				{
				EnqueError(ErrorInfo::From(ErrorType_Unexpected,peer->m_HostID,_PNT("CreateP2P fail: An Error Regarding a Session Key!")));
				}*/
			}
		}

		// 일단 빈 P2P group을 만든다.
		// 그리고 즉시 member들을 하나씩 추가한다.
		HostID groupHostID = m_HostIDFactory->Create(assignedHostID);
		if ( HostID_None == groupHostID )
			return HostID_None;

		CP2PGroupPtr_S NG(new CP2PGroup_S());
		NG->m_groupHostID = groupHostID;
		m_P2PGroups.Add(NG->m_groupHostID, NG);
		NG->m_option = option;


		m_joinP2PGroupKeyGen++;

		// NOTE: HostID_Server에 대한 OnP2PGroupJoinMemberAckComplete 이벤트가 2번 발생되는 문제때문에 for문을 역순으로 돌리게하였습니다.
		bool vaildGroup = false;
		for ( int ee = count - 1; ee >= 0; ee-- )
		{
			vaildGroup |= JoinP2PGroup_Internal(clientHostIDList[ee], NG->m_groupHostID, customField, m_joinP2PGroupKeyGen);
		}

		if ( m_allowEmptyP2PGroup == false && vaildGroup == false )
		{
			// 그룹멤버가 모두 유효하지 않은 경우.
			// 별도 처리를 해야 하는지 여부는 파악 중.
		}

		return NG->m_groupHostID;
	}

	bool CNetServerImpl::JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray customField)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		return JoinP2PGroup_Internal(memberHostID, groupHostID, customField, ++m_joinP2PGroupKeyGen);
	}

	CSuperSocketPtr CNetServerImpl::GetAnyUdpSocket()
	{
		if ( m_staticAssignedUdpSockets.GetCount() <= 0 )
			return CSuperSocketPtr();

		int randomIndex = m_random.Next((int)m_staticAssignedUdpSockets.GetCount() - 1);
		return m_staticAssignedUdpSockets[randomIndex];
	}

	void CNetServerImpl::EnqueueClientLeaveEvent(CRemoteClient_S *rc, ErrorType errorType, ErrorType detailtype, const ByteArray& comment, SocketErrorCode socketErrorCode)
	{
		AssertIsLockedByCurrentThread();
		if ( m_eventSink_NOCSLOCK && rc->m_HostID != HostID_None )
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_errorInfo->m_errorType = errorType;
			e.m_errorInfo->m_detailType = detailtype;
			e.m_netClientInfo = rc->ToNetClientInfo();
			e.m_type = LocalEventType_ClientLeaveAfterDispose;
			e.m_byteArrayComment = comment;
			e.m_remoteHostID = rc->m_HostID;

			//modify by rekfkno1 - per-remote event로 변경.
			//EnqueLocalEvent(e);
			EnqueLocalEvent(e, rc);

			// 			// 121 에러 감지를 위함
			// 			if(socketErrorCode == 121)
			// 			{
			// 				String txt;
			// 				txt.Format(_PNT("SocketErrorCode=121! Source={%s,%d},ClientInfo={%s}, CCU = %d, SrvConfig={%s}"),
			// 					rc->GetDisposeCaller() ? rc->GetDisposeCaller() : _PNT(""),
			// 					rc->m_disposeWaiter ? rc->m_disposeWaiter->m_reason : 0,
			// 					(LPCWSTR) e.m_netPeerInfo->ToString(true), 
			// 					m_authedHosts.GetCount(), 
			// 					(LPCWSTR)GetConfigString() );
			// 
			// 				CErrorReporter_Indeed::Report(txt);
			// 			}
		}

		/*if(m_vizAgent)
		{
		CriticalSectionLock vizlock(m_vizAgent->m_cs,true);
		m_vizAgent->m_c2sProxy.NotifySrv_Clients_Remove(HostID_Server, g_ReliableSendForPN, rc->m_HostID);
		}*/

	}

	INetCoreEvent * CNetServerImpl::GetEventSink_NOCSLOCK()
	{
		AssertIsNotLockedByCurrentThread();
		return m_eventSink_NOCSLOCK;
	}

	CNetServerImpl::CNetServerImpl()
		: //m_userTaskQueue(this),
		m_listening(false),
		m_heartbeatWorking(0),
		m_PurgeTooOldUnmatureClient_Timer(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs),
		m_PurgeTooOldAddMemberAckItem_Timer(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs),
		//m_disposeGarbagedHosts_Timer(CNetConfig::DisposeGarbagedHostsTimeoutMs),
		m_electSuperPeer_Timer(CNetConfig::ElectSuperPeerIntervalMs),
		m_removeTooOldUdpSendPacketQueue_Timer(CNetConfig::LongIntervalMs),
		//m_DisconnectRemoteClientOnTimeout_Timer(CNetConfig::UnreliablePingIntervalMs),
		m_removeTooOldRecyclePair_Timer(CNetConfig::RecyclePairReuseTimeMs / 2),
		m_GarbageTooOldRecyclableUdpSockets_Timer(CNetConfig::GarbageTooOldRecyclableUdpSocketsIntervalMs)
	{
		m_globalTimer = CGlobalTimerThread::GetSharedPtr();

#ifndef PROUDSRCCOPY
		BINARYreference();
		m_lic = new CPNLic;
#endif
		m_forceDenyTcpConnection_TEST = false;

		m_internalVersion = CNetConfig::InternalNetVersion;
#ifdef _WIN32
		EnableLowFragmentationHeap();
#endif

		//m_shutdowning = false;
		m_eventSink_NOCSLOCK = NULL;

		//m_timer.Start();

		AttachProxy(&m_s2cProxy);
		AttachStub(&m_c2sStub);

		m_c2sStub.m_owner = this;

		//		m_userTaskRunning = false;

		m_SpeedHackDetectorReckRatio = 100;

		//m_finalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		m_allowEmptyP2PGroup = true;

		m_s2cProxy.m_internalUse = true;
		m_c2sStub.m_internalUse = true;

		// 용량을 미리 잡아놓는다.
		m_routerIndexList.SetCapacity(CNetConfig::OrdinaryHeavyS2CMulticastCount);

		m_joinP2PGroupKeyGen = 0;

		m_timerCallbackInterval = 0;
		m_timerCallbackParallelMaxCount = 1;
		m_timerCallbackContext = NULL;
		m_startCreateP2PGroup = false;

		m_loopbackHost = NULL;
	}


	int CNetServerImpl::GetClientHostIDs(HostID* ret, int count)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		int c = 0;
		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); c < count && i != m_authedHostMap.end(); i++ )
		{
			HostID hostID = i->GetFirst();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( hostID == HostID_Server )
				continue;

			ret[c++] = hostID;
		}
		return c;
	}

	bool CNetServerImpl::GetClientInfo(HostID clientHostID, CNetClientInfo &ret)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( clientHostID == HostID_None )
			return false;

		if ( clientHostID == HostID_Server )
			return false;

		CHostBase * hb = AuthedHostMap_Get(clientHostID);
		if ( hb == NULL )
			return false;

		CRemoteClient_S *rc = LeanDynamicCastForRemoteClient(hb);
		if ( rc == NULL )
			return false;

		rc->ToNetClientInfo(ret);

		return true;
	}

	CP2PGroupPtr_S CNetServerImpl::GetP2PGroupByHostID_NOLOCK(HostID groupHostID)
	{
		AssertIsLockedByCurrentThread(); // 1회라도 CS 접근을 막고자

		//CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_S ret;
		m_P2PGroups.TryGetValue(groupHostID, ret);
		return ret;
	}

	bool CNetServerImpl::GetP2PGroupInfo(HostID groupHostID, CP2PGroup &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CP2PGroupPtr_S g = GetP2PGroupByHostID_NOLOCK(groupHostID);

		if ( g != NULL )
		{
			g->ToInfo(output);
			return true;
		}
		return false;
	}

	// P2P 그룹을 host id list로 변환 후 배열에 이어붙인다.
	void CNetServerImpl::ConvertAndAppendP2PGroupToPeerList(HostID sendTo, HostIDArray &sendTo2)
	{
		AssertIsLockedByCurrentThread();
		// convert sendTo group to remote hosts

		// ConvertP2PGroupToPeerList는 한번만 호출되는 것이 아니다.
		// sendTo2.SetSize(0);

		CP2PGroup_S *g = GetP2PGroupByHostID_NOLOCK(sendTo);
		if ( g == NULL )
		{
			//modify by rekfkno1 - 이미 dispose로 들어간 remote는 추가 하지 말자.
			if ( HostID_Server == sendTo || NULL != GetAuthedClientByHostID_NOLOCK(sendTo) )
			{
				sendTo2.Add(sendTo);
			}
		}
		else
		{
			for ( P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); i++ )
			{
				HostID memberID = i->first;
				sendTo2.Add(memberID);
			}
		}
	}

	bool CNetServerImpl::GetP2PConnectionStats(HostID remoteHostID, CP2PConnectionStats& status)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* peer = GetAuthedClientByHostID_NOLOCK(remoteHostID);
		CP2PGroup_S * g = NULL;
		if ( NULL != peer )
		{
			status.m_TotalP2PCount = 0;
			status.m_directP2PCount = 0;
			status.m_toRemotePeerSendUdpMessageSuccessCount = peer->m_toRemotePeerSendUdpMessageSuccessCount;
			status.m_toRemotePeerSendUdpMessageTrialCount = peer->m_toRemotePeerSendUdpMessageTrialCount;

			for ( CRemoteClient_S::P2PConnectionPairs::iterator itr = peer->m_p2pConnectionPairs.begin(); itr != peer->m_p2pConnectionPairs.end(); ++itr )
			{
				//이것은 전에 한번이라도 p2p로 묶을것을 요청받은적이 있는지에 대한값이다.
				//그렇기 때문에 이것이 false라면 카운트에 들어가지 않는다.
				if ( (*itr)->m_jitDirectP2PRequested )
				{
					++status.m_TotalP2PCount;
					if ( !(*itr)->IsRelayed() )
					{
						++status.m_directP2PCount;
					}
				}
			}

			/*for (RCPairMap::iterator i = m_p2pConnectionPairList.m_list.begin();i!=m_p2pConnectionPairList.m_list.end();i++)
			{
			P2PConnectionState* pair = i->second;

			if(pair->m_firstClient == peer || pair->m_secondClient == peer)
			if(pair->m_jitDirectP2PRequested)
			{
			++status.m_TotalP2PCount;
			if(!pair->Relayed)
			++status.m_directP2PCount;
			}
			}*/

			return true;
		}
		else if ( NULL != (g = GetP2PGroupByHostID_NOLOCK(remoteHostID)) )
		{
			//그룹아이디라면 그룹에서 얻는다.
			status.m_TotalP2PCount = 0;
			status.m_directP2PCount = 0;
			status.m_toRemotePeerSendUdpMessageSuccessCount = 0;
			status.m_toRemotePeerSendUdpMessageTrialCount = 0;

			CFastArray<P2PConnectionState*> tempConnectionStates;
			//tempConnectionStates.SetMinCapacity(멤버갯수의 제곱의 반, 단 최소 1이상.)
			intptr_t t = g->m_members.size() * g->m_members.size(); // pow가 SSE를 쓰는 경우 일부 VM에서 크래시. 그냥 정수 제곱 계산을 하는게 안정.
			t /= 2;
			if (t >= 0)
				tempConnectionStates.SetMinCapacity(t);

			//for (RCPairMap::iterator i = m_p2pConnectionPairList.m_list.begin();i!=m_p2pConnectionPairList.m_list.end();i++)
			//{
			//	P2PConnectionState* pair = i->second;

			//	//그룹멤버 아닌경우 제외
			//	if(g->m_members.end() == g->m_members.find(pair->m_firstClient->m_HostID) && g->m_members.end() == g->m_members.find(pair->m_secondClient->m_HostID))
			//		continue;

			//	//이미 검사한 페어 제외
			//	if(-1 != tempConnectionStates.FindByValue(pair))
			//		continue;

			//	if(pair->m_jitDirectP2PRequested)
			//	{
			//		status.m_TotalP2PCount++; 
			//		if(!pair->Relayed)
			//			status.m_directP2PCount++;
			//	}
			//}

			for ( P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); ++i )
			{
				if ( i->second.m_ptr->GetHostID() != HostID_Server )
				{
					CRemoteClient_S* iMemberAsRC = (CRemoteClient_S*)i->second.m_ptr;

					// 2010.08.24 : add by ulelio 모든 group원의 총합을 구한다. 
					if ( iMemberAsRC == NULL )
						continue;

					status.m_toRemotePeerSendUdpMessageSuccessCount += iMemberAsRC->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemotePeerSendUdpMessageTrialCount += iMemberAsRC->m_toRemotePeerSendUdpMessageTrialCount;

					for ( CRemoteClient_S::P2PConnectionPairs::iterator itr = iMemberAsRC->m_p2pConnectionPairs.begin(); itr != iMemberAsRC->m_p2pConnectionPairs.end(); ++itr )
					{
						const P2PConnectionStatePtr &connPairOfMember = *itr;
						//다른 그룹일경우 제외하자.
						if ( g->m_members.end() == g->m_members.find(connPairOfMember->m_firstClient->m_HostID)
							|| g->m_members.end() == g->m_members.find(connPairOfMember->m_secondClient->m_HostID) )
						{
							continue;
						}

						//이미 검사한 커넥션 페어는 검사하지 않는다.
						if ( -1 != tempConnectionStates.FindByValue(connPairOfMember) )
						{
							continue;
						}

						if ( connPairOfMember->m_jitDirectP2PRequested )
						{
							++status.m_TotalP2PCount;
							if ( !connPairOfMember->IsRelayed() )
							{
								++status.m_directP2PCount;
							}
						}

						tempConnectionStates.Add(connPairOfMember);
					}
				}
			}
			return true;
		}

		return false;
	}

	bool CNetServerImpl::GetP2PConnectionStats(HostID remoteA, HostID remoteB, CP2PPairConnectionStats& status)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		status.m_toRemoteASendUdpMessageSuccessCount = 0;
		status.m_toRemoteASendUdpMessageTrialCount = 0;
		status.m_toRemoteBSendUdpMessageSuccessCount = 0;
		status.m_toRemoteBSendUdpMessageTrialCount = 0;
		status.m_isRelayed = false;

		CRemoteClient_S* rcA = GetAuthedClientByHostID_NOLOCK(remoteA);
		CRemoteClient_S* rcB = GetAuthedClientByHostID_NOLOCK(remoteB);

		if ( rcA != NULL && rcB != NULL )
		{
			CRemoteClient_S::P2PConnectionPairs::iterator iPair;
			for ( iPair = rcA->m_p2pConnectionPairs.begin(); iPair != rcA->m_p2pConnectionPairs.end(); iPair++ )
			{
				P2PConnectionState* pair = *iPair;
				if ( pair->ContainsHostID(rcB->m_HostID) )
				{
					status.m_toRemoteBSendUdpMessageSuccessCount = pair->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemoteBSendUdpMessageTrialCount = pair->m_toRemotePeerSendUdpMessageTrialCount;
				}
			}

			for ( iPair = rcB->m_p2pConnectionPairs.begin(); iPair != rcB->m_p2pConnectionPairs.end(); iPair++ )
			{
				P2PConnectionState* pair = *iPair;
				if ( pair->ContainsHostID(rcA->m_HostID) )
				{
					status.m_toRemoteASendUdpMessageSuccessCount = pair->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemoteASendUdpMessageTrialCount = pair->m_toRemotePeerSendUdpMessageTrialCount;
				}
			}

			P2PConnectionStatePtr pair = m_p2pConnectionPairList.GetPair(remoteA, remoteB);
			if ( pair != NULL )
			{
				status.m_isRelayed = pair->IsRelayed();
			}
			else
			{
				assert(false);
			}
			return true;
		}

		return false;
	}

	CHostBase * CNetServerImpl::GetSendDestByHostID_NOLOCK(HostID peerHostID)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock clk(GetCriticalSection(), true);
		if ( peerHostID == HostID_Server )
			return m_loopbackHost;
		if ( peerHostID == HostID_None )
			return NULL;

		return GetAuthedClientByHostID_NOLOCK(peerHostID);
	}

	// 	bool CNetServerImpl::IsFinalReceiveQueueEmpty()
	// 	{
	// 		AssertIsLockedByCurrentThread();
	// 
	// 		return m_finalUserWorkItemList.GetCount() == 0;
	// 	}
	// 
	// NS에서 remote client에 대한 first dispose issue에 대한 추가 처리
	void CNetServerImpl::OnHostGarbaged(CHostBase* remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode)
	{
		LockMain_AssertIsLockedByCurrentThread();

		// NOTE: LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
		if ( remote == m_loopbackHost )
			return;

		CRemoteClient_S* rc = (CRemoteClient_S*)remote;

		// 이벤트 노티
		// 원래는 remoteClient에 enquelocalevent를 처리 하려 했으나.
		// delete되기 전에 enque하는 것이므로 다른 쓰레드에서 이 remoteclient에 대한 이벤트가
		// 나오지 않을 것이므로, 이렇게 처리해도 무방하다.
		if ( rc->m_disposeWaiter != NULL )
		{
			EnqueueClientLeaveEvent(rc,
									rc->m_disposeWaiter->m_reason,
									rc->m_disposeWaiter->m_detail,
									rc->m_disposeWaiter->m_comment,
									rc->m_disposeWaiter->m_socketErrorCode);
		}
		else
		{
			EnqueueClientLeaveEvent(rc,
									ErrorType_DisconnectFromLocal,
									ErrorType_ConnectServerTimeout,
									ByteArray(),
									SocketErrorCode_Ok);
		}

		/* 내부 절차는 아래와 같다.
		먼저 TCP socket을 close한다. (UDP는 공용되므로 불필요)
		그리고 issue중이던 것들은 에러가 발생하게 된다. 혹은 issuerecv/send에서 이벤트 없이 에러가 발생한다.
		이때 recv on progress, send on progress가 중지될 것이다. 물론 TCP에 한해서 말이다.
		양쪽 모두 중지 확인되면 즉시 dispose를 한다.
		group info 등을 파괴하는 것은 즉시 하지 않고 이 객체가 파괴되는 즉시 하는게 맞다. */

#ifdef USE_HLA

		if ( m_hlaSessionHostImpl != NULL )
			m_hlaSessionHostImpl->DestroyViewer(rc);
#endif
		// 이렇게 소켓을 닫으면 issue중이던 것들이 모두 종료한다. 그리고 나서 재 시도를 하려고 해도
		// m_disposeWaiter가 있으므로 되지 않을 것이다. 그러면 안전하게 객체를 파괴 가능.
		// 소켓을 닫기전에 이 리모트에게 보냈던 총량을 기록한다.
		if ( m_logWriter && rc->m_tcpLayer != NULL ) // ACR 연결을 본 연결에 던져주고 시체가 된 to-client의 TCP는 null.
		{
			m_logWriter->WriteLine(0, LogCategory_System, rc->m_HostID, String::NewFormat(_PNT("Total remoteClient SendBytes. [TotalSendBytes:%u]"), rc->m_tcpLayer->GetTotalIssueSendBytes()));
		}

		// TCP socket에서의 수신 처리에 대해서 더 이상 이 host가 처리하지 않는다.
		// 단, 댕글링 위험 때문에, TCP and UDP socket을 garbage는 안하고, 
		// 추후 OnHostGarbageCollected에서 수행한다.
		if (rc->m_tcpLayer != NULL) // ACR 연결을 본 연결에 던져주고 시체가 된 remote client가 가진 TCP는 null이므로, null check를 해야.
		{
			SocketToHostsMap_RemoveForAnyAddr(rc->m_tcpLayer);
		}

		// UDP socket에 대해서도 위에와 마찬가지로.
		CFallbackableUdpLayer_S& udp = rc->m_ToClientUdp_Fallbackable;
		if (udp.m_udpSocket)
		{
			SocketToHostsMap_RemoveForAddr(udp.m_udpSocket, udp.GetUdpAddrFromHere());
			udp.m_udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(udp.GetUdpAddrFromHere());
		}
		if (rc->m_ownedUdpSocket)
		{
			SocketToHostsMap_RemoveForAnyAddr(rc->m_ownedUdpSocket);
		}

		rc->WarnTooShortDisposal(_PNT("NS.OHG"));

		// P2P 그룹 관계를 모두 청산해버린다.
		for ( JoinedP2PGroups_S::iterator i = rc->m_joinedP2PGroups.begin(); i != rc->m_joinedP2PGroups.end(); i++ )
		{
			// remove from P2PGroup_Add ack info
			CP2PGroupPtr_S gp = i->GetSecond().m_groupPtr;
			AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
				gp, rc->m_HostID, ErrorType_DisconnectFromRemote);

			// notify member leave to related group members
			// modify by ulelio : Server는 P2PGroup_MemberLeave를 받을 필요가 없다.
			for ( P2PGroupMembers_S::iterator iMember = gp->m_members.begin(); iMember != gp->m_members.end(); ++iMember )
			{
				if ( iMember->first != HostID_Server )
				{
					m_s2cProxy.P2PGroup_MemberLeave(
						iMember->first,
						g_ReliableSendForPN,
						rc->m_HostID,
						gp->m_groupHostID);
				}
			}

			// P2P그룹에서 제명
			gp->m_members.erase(rc->m_HostID);

			// 제명한 후 P2P그룹이 잔존해야 한다면...
			if ( gp->m_members.size() > 0 || m_allowEmptyP2PGroup )
				P2PGroup_RefreshMostSuperPeerSuitableClientID(gp);
			else
			{
				// P2P그룹이 파괴되어야 한다면...
				HostID bak = gp->m_groupHostID;
				if ( m_P2PGroups.Remove(bak) )
				{
					m_HostIDFactory->Drop(bak);
					EnqueueP2PGroupRemoveEvent(bak);
				}
			}

			// 클라 나감 이벤트에서 '나가기 처리 직전까지 잔존했던 종속 P2P그룹'을 줘야 하므로 여기서 백업.
			rc->m_hadJoinedP2PGroups.Add(gp->m_groupHostID);
		}
		rc->m_joinedP2PGroups.Clear();

		// AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent 내에서
		// rc->m_disposeWaiter가 NULL이 아니면 EnqueLocalEvent를 할 수 없게 된다.
		// 순서를 바꿔서 해결.
		if ( rc->m_disposeWaiter == NULL )
		{
			rc->m_disposeWaiter.Free();
			rc->m_disposeWaiter.Attach(new CRemoteClient_S::CDisposeWaiter());
			rc->m_disposeWaiter->m_reason = errorType;
			rc->m_disposeWaiter->m_detail = detailType;
			rc->m_disposeWaiter->m_comment = comment;
			rc->m_disposeWaiter->m_socketErrorCode = socketErrorCode;

			if ( m_logWriter )
			{
				m_logWriter->WriteLine(0, LogCategory_System, rc->m_HostID, _PNT("IssueDisposeRemoteClient client"));
			}
		}

		// 클라가 P2P 연결이 되어 있는 상대방 목록을 찾아서 모두 제거
		m_p2pConnectionPairList.RemovePairOfAnySide(rc);
#ifdef TRACE_NEW_DISPOSE_ROUTINE
		NTTNTRACE("%d RemovePair Call!!\n", (int)rc->m_HostID);
#endif // TRACE_NEW_DISPOSE_ROUTINE

		//return true;
		//PNTRACE(TID_System,"%s(this=%p,WSAGetLastError=%d)\n",__FUNCTION__,this,WSAGetLastError());

		// 인덱스 정리. 없애기로 한 remote는 더 이상 UDP 데이터를 받을 자격이 없다.
		//m_UdpAddrPortToRemoteClientIndex.RemoveKey(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
	}


	// TCP 연결이 장애가 생긴 상황을 처리한다. 
	// TCP receive timeout 혹은 TCP connection lost 상태에서 호출됨.
	// ACR을 발동하거나 디스 시킨다.
	void CNetServerImpl::RemoteClient_GarbageOrGoOffline(
		CRemoteClient_S *rc,
		ErrorType errorType,
		ErrorType detailType,
		const ByteArray& comment,
		const PNTCHAR* where,
		SocketErrorCode socketErrorCode)
	{
		AssertIsLockedByCurrentThread();

		if (rc->m_enableAutoConnectionRecovery)
		{
			RemoteClient_ChangeToAcrWaitingMode(
				rc,
				errorType,
				detailType,
				comment,
			where,
				socketErrorCode);
		}
		else
		{
			// ACR 안함. 그냥 님 디스요.
			GarbageHost(rc, errorType, detailType, comment, where, socketErrorCode);
		}
	}

	int64_t CNetServerImpl::GetTimeMs()
	{
		//if(m_timer != NULL)
		return GetPreciseCurrentTimeMs();

		//return -1;
	}

	int CNetServerImpl::GetClientCount()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// CNetServerImpl의 LoopbackHost는 제외
		return m_authedHostMap.GetCount() - 1;
	}

	CNetServerImpl::~CNetServerImpl()
	{
		Stop();
#ifdef USE_HLA

		m_hlaSessionHostImpl.Free();
#endif
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		// PN 내부에서도 쓰는 RMI까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
		CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.

#ifndef PROUDSRCCOPY
		delete ((CPNLic*)m_lic);
#endif
	}

	// 사용자 API를 구현.
	void CNetServerImpl::GetRemoteIdentifiableLocalAddrs(CFastArray<NamedAddrPort> &output)
	{
		output.Clear();

		CFastArray<Proud::String> localAddresses;
		CNetUtil::GetLocalIPAddresses(localAddresses);

		for ( int i = 0; i < (int)m_TcpListenSockets.GetCount(); i++ )
		{
			CSuperSocketPtr listenSocket = m_TcpListenSockets[i];
			if ( listenSocket != NULL )
			{
				NamedAddrPort namedAddr = NamedAddrPort::From(listenSocket->GetSocketName());

				// local NIC이 지정된 바 있으면 그것을 넣자.
				namedAddr.OverwriteHostNameIfExists(m_localNicAddr);
				// external 주소가 지정된 바 있으면 그것을 넣자.
				namedAddr.OverwriteHostNameIfExists(m_serverAddrAlias);

				// 아직까지도 비어있다고? 그렇다면 모든 NIC에 대해서 잔뜩 채워야 한다.
				if (namedAddr.m_addr.IsEmpty() || namedAddr.m_addr == _PNT("0.0.0.0"))
				{
					for (int j = 0; j < (int)localAddresses.GetCount(); ++j)
					{
						NamedAddrPort a2;
						a2.m_port = namedAddr.m_port;
						a2.m_addr = localAddresses[j];
						output.Add(a2);
					}
				}
				output.Add(namedAddr);
			}
		}
	}

	// *주의* 이 함수는 reentrant하게 하지 말 것. 멤버 변수를 로컬변수처럼 쓰는 데가 있어서다.
	// => 동적 크기의 로컬 변수를 obj-pool 방식으로 고친 것 같은데, 그렇다면 이 주의사항은 사라져도 됨.
	void CNetServerImpl::DoForLongInterval()
	{
		EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient();

		CriticalSectionLock mainlock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		intptr_t i = 0;
		/* 각 remote에 대해서는 main lock 건 상태에서 즉시 처리하고
		각 remote가 가진 socket들 포함 모든 socket에 대해서는 LowContextSwitchingLoop를 통해
		main lock 없이 처리하도록 하자.
		per-socket lock은 다른 곳에서 syscall을 하기 때문에 contention이 잦기 때문. */
		int64_t currentTime = GetCachedServerTimeMs();

		POOLED_ARRAY_LOCAL_VAR(CSuperSocketArray, udpSocketList);

		for ( int i = 0; i < m_staticAssignedUdpSockets.GetCount(); i++ )
		{
			CSuperSocket* us = m_staticAssignedUdpSockets[i];
			us->IncreaseUseCount();
			udpSocketList.Add(us);
		}

		// 각 remote에 대해서 처리를. 이때 remote가 가진 socket들도 목록에 추가된다.
		for ( AuthedHostMap::iterator irc = m_authedHostMap.begin(); irc != m_authedHostMap.end(); irc++ )
		{
			CHostBase* hb = irc->GetSecond();

			if ( hb->GetHostID() == HostID_Server )
				continue;

			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(hb);
			rc->DoForLongInterval(currentTime);
			if ( rc->m_ownedUdpSocket != NULL )
			{
				rc->m_ownedUdpSocket->IncreaseUseCount();
				udpSocketList.Add(rc->m_ownedUdpSocket);
			}

			// GarbageHost를 통하여 m_tcpLayer의 연결을 끊기 때문에 NULL 확인을 해야한다.
			if ( rc->m_tcpLayer != NULL )
			{
				rc->m_tcpLayer->IncreaseUseCount();
				udpSocketList.Add(rc->m_tcpLayer);
			}
		}

		for ( CFastMap2<CHostBase*, CHostBase*, int>::iterator irc = m_candidateHosts.begin(); irc != m_candidateHosts.end(); irc++ )
		{
			CRemoteClient_S* rc = (CRemoteClient_S*)irc->GetSecond();
			rc->DoForLongInterval(currentTime);
			if ( rc->m_ownedUdpSocket )
			{
				rc->m_ownedUdpSocket->IncreaseUseCount();
				udpSocketList.Add(rc->m_ownedUdpSocket);
			}

			if ( rc->m_tcpLayer != NULL )
			{
				rc->m_tcpLayer->IncreaseUseCount();
				udpSocketList.Add(rc->m_tcpLayer);
			}
		}

		// main unlock
		mainlock.Unlock();
		DoForLongIntervalFunctor<CSuperSocket*> functor(currentTime);
		LowContextSwitchingLoop(udpSocketList.GetData(), udpSocketList.GetCount(), functor);
	}


	void CNetServerImpl::SetDefaultFallbackMethod(FallbackMethod value)
	{
		if ( value == FallbackMethod_CloseUdpSocket )
		{
			throw Exception("Not supported value yet!");
		}

		if ( m_simplePacketMode )
			m_settings.m_fallbackMethod = FallbackMethod_ServerUdpToTcp;
		else
			m_settings.m_fallbackMethod = value;
	}

	void CNetServerImpl::EnableLog(const PNTCHAR* logFileName)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_logWriter )
			return;
#if defined (_WIN32)
		m_logWriter.Attach(CLogWriter::New(logFileName));
#endif

		// 모든 클라이언트들에게 로그를 보내라는 명령을 한다.
		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
		{
			CHostBase* hb = i->GetSecond();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( hb->GetHostID() == HostID_Server )
				continue;

			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(hb);
			if ( rc->m_disposeWaiter != NULL )
				continue;

			m_s2cProxy.EnableLog(i->GetFirst(), g_ReliableSendForPN);
		}
	}

	void CNetServerImpl::DisableLog()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		m_logWriter.Free();

		// 모든 클라이언트들에게 로그를 보내지 말라는 명령을 한다.
		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
		{
			CHostBase* rc = i->GetSecond();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( rc->GetHostID() == HostID_Server )
				continue;

			if ( rc->m_disposeWaiter != NULL )
				continue;

			m_s2cProxy.DisableLog(i->GetFirst(), g_ReliableSendForPN);
		}
	}

	void CNetServerImpl::Heartbeat_EveryRemoteClient()
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		int64_t currTime = GetCachedServerTimeMs();

		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
		{
			CHostBase* hb = i->GetSecond();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( hb->GetHostID() == HostID_Server )
				continue;

			CRemoteClient_S *rc = LeanDynamicCastForRemoteClient(hb);

			if ( rc->m_garbaged )
				continue;

			// 너무 오랫동안 TCP 수신을 못한 경우 디스 처리 한다.
			// - 심플 패킷 모드가 false 일때 해당 사항 체크
			if ( IsSimplePacketMode() == false &&
				rc->m_tcpLayer != NULL &&
				rc->m_autoConnectionRecoveryWaitBeginTime == 0 &&
				(currTime - rc->m_tcpLayer->m_lastReceivedTime) > m_settings.m_defaultTimeoutTime )
			{
				if ( m_logWriter )
				{
					String text;
					text.Format(_PNT("Server: Client %d TCP connection timed out."), rc->GetHostID());
					m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, text);
				}

				RemoteClient_GarbageOrGoOffline(rc,
												ErrorType_DisconnectFromRemote,
												ErrorType_ConnectServerTimeout,
												ByteArray(),
												_PNT("RC_GOGO"),
												SocketErrorCode_Ok);

				continue;
			}

			// tcp가 위험에 빠졌는지 체크 한다.
			// 			if(CNetConfig::EnableSendBrake)
			// 			{
			// 				rc->CheckTcpInDanger(currTime);
			// 			}

			Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(rc);

			// 매 2초마다 next expected Message ID(마지막 받은 것+1)를 ack 전송
			//Heartbeat_AcrSendMessageIDAckOnNeed(rc);

			// ProudNet level의 graceful disconnect 과정이 진행중인 클라가 아직도 더디고 있으면 강제로 TCP 연결을 닫아버림
			// NOTE: graceful disconnect가 별도로 있는 이유는, 연결 끊기 함수를 사용자가 호출한 직전에도 보내려던 RMI를 모두 전송하기 위함.
			HardDisconnect_AutoPruneGoesTooLongClient(rc);

			FallbackServerUdpToTcpOnNeed(rc, currTime);
			ArbitraryUdpTouchOnNeed(rc, currTime);
			RefreshSendQueuedAmountStat(rc);

			// MessageOverload Warning 추가
			if ( rc->MessageOverloadChecking(GetPreciseCurrentTimeMs()) )
			{
				EnqueWarning(ErrorInfo::From(ErrorType_MessageOverload, rc->GetHostID()));
			}
		}
	}

	// TCP fallback과 관련해서 서버 로컬에서의 처리 부분
	// 이미 넷클라 측은 fallback을 진행한 상태에서 콜 해야 한다.
	void CNetServerImpl::SecondChanceFallbackUdpToTcp(CRemoteClient_S* rc)
	{
		AssertIsLockedByCurrentThread();

		if ( rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
		{
			rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled = false;

			// addr-to-remote 관계도 끊어야 함. 
			// 넷클라가 ip handover를 한 경우라서 끊어진 경우 다른 클라가 이 addr을 가져갈 수 있으니까.
			assert(rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL);
			SocketToHostsMap_RemoveForAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket, rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
			//m_UdpAddrPortToRemoteClientIndex.RemoveKey(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());

			P2PGroup_RefreshMostSuperPeerSuitableClientID(rc);

			// 로그를 남긴다.
			if ( m_logWriter )
			{
				String txt;
				txt.Format(_PNT("Client %d cancelled the UDP Hole-Punching to the Servers : Client local Addr=%s"),
						   rc->m_HostID, rc->m_tcpLayer->m_remoteAddr.ToString().GetString());
				m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
			}
		}
	}


	// 	void CNetServerImpl::EnqueueHackSuspectEvent(CRemoteClient_S* rc, const char* statement, HackType hackType)
	// 	{
	// 		CriticalSectionLock clk(GetCriticalSection(), true);
	// 		CHECK_CRITICALSECTION_DEADLOCK(this);
	// 
	// 		if (m_eventSink_NOCSLOCK)
	// 		{
	// 			LocalEvent evt;
	// 			evt.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
	// 			evt.m_hackType = hackType;
	// 			evt.m_remoteHostID = rc ? rc->m_HostID : HostID_None;
	// 			evt.m_type = LocalEventType_HackSuspected;
	// 			evt.m_errorInfo->m_comment = StringA2T(statement);
	// 			EnqueLocalEvent(evt);
	// 		}
	// 	}


	// 	double CNetServerImpl::GetSpeedHackLongDetectMinInterval()
	// 	{
	// 		return max(60,CNetConfig::DefaultConnectionTimeoutSec) * m_SpeedHackDetectorReckRatio;
	// 	}
	// 
	// 	double CNetServerImpl::GetSpeedHackLongDeviationRatio()
	// 	{
	// 		return 2;
	// 	}
	// 
	// 	double CNetServerImpl::GetSpeedHackDeviationThreshold()
	// 	{
	// 		return 0.3;
	// 	}
	// 
	// 	int CNetServerImpl::GetSpeedHackDetectorSeriesLength()
	// 	{
	// 		double ret = max(10, CNetConfig::GetFallbackServerUdpToTcpTimeout() / CNetConfig::CSPingInterval);
	// 		ret *= m_SpeedHackDetectorReckRatio;
	// 		ret = max(ret,2);
	// 
	// 		return (int)ret;
	// 	}
	// 
	void CNetServerImpl::SetSpeedHackDetectorReckRatioPercent(int newValue)
	{
		if ( newValue <= 0 || newValue > 100 )
			ThrowInvalidArgumentException();

		m_SpeedHackDetectorReckRatio = newValue;
	}

	void CNetServerImpl::SetMessageMaxLength(int value_s, int value_c)
	{
		CNetConfig::ThrowExceptionIfMessageLengthOutOfRange(value_s);
		CNetConfig::ThrowExceptionIfMessageLengthOutOfRange(value_c);

		//{
		//CriticalSectionLock l(CNetConfig::GetWriteCriticalSection(),true);
		//CNetConfig::MessageMaxLength = max(CNetConfig::MessageMaxLength ,value); // 아예 전역 값도 수정하도록 한다.
		//}
		// 		int compareValue =  max(value_s, value_c);
		// 		CNetConfig::MessageMaxLength = max(CNetConfig::MessageMaxLength , compareValue); // 아예 전역 값도 수정하도록 한다.

		m_settings.m_serverMessageMaxLength = value_s;
		m_settings.m_clientMessageMaxLength = value_c;
	}

	void CNetServerImpl::SetDefaultTimeoutTimeSec(double newValInSec)
	{
		int newValInMs = DoubleToInt(newValInSec * 1000);
		SetDefaultTimeoutTimeMs(newValInMs);
	}

	void CNetServerImpl::SetDefaultTimeoutTimeMs(int newValInMs)
	{
		AssertIsNotLockedByCurrentThread();
		if ( newValInMs < 1000 )
		{
			if ( m_eventSink_NOCSLOCK != NULL )
			{
				ShowUserMisuseError(_PNT("Too short timeout value. It may cause unfair disconnection."));
				return;
			}
		}
#ifndef _DEBUG
		if ( newValInMs > 240000 )
		{
			if ( m_eventSink_NOCSLOCK != NULL )
			{
				//ShowUserMisuseError(_PNT("Too long timeout value. It may take a lot of time to detect lost connection."));
			}
		}
#endif

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		m_settings.m_defaultTimeoutTime = newValInMs;

	}

	bool CNetServerImpl::SetDirectP2PStartCondition(DirectP2PStartCondition newVal)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( newVal >= DirectP2PStartCondition_LAST )
		{
			ThrowInvalidArgumentException();
		}

		m_settings.m_directP2PStartCondition = newVal;

		return true;
	}

	void CNetServerImpl::GetStats(CNetServerStats &outVal)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CriticalSectionLock statsLock(m_statsCritSec, true);
		outVal = m_stats;

		// 통계 정보는 좀 부정확해도 된다.
		// 하지만 P2P 카운트 정보는 정확해야 한다.
		// 그래서 여기서 즉시 계산한다.

		// CNetServerImpl의 LoopbackHost는 제외
		outVal.m_clientCount = (uint32_t)m_authedHostMap.GetCount() - 1;

		outVal.m_realUdpEnabledClientCount = 0;

		outVal.m_occupiedUdpPortCount = (int)m_staticAssignedUdpSockets.GetCount();

		for ( RemoteClients_S::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
		{
			CHostBase* hb = i->GetSecond();

			// CNetServerImpl의 LoopbackHost는 제외
			if ( hb->m_HostID == HostID_Server )
				continue;

			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(hb);
			if ( rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
				outVal.m_realUdpEnabledClientCount++;
		}

		for ( RCPairMap::iterator i = m_p2pConnectionPairList.m_activePairs.begin(); i != m_p2pConnectionPairList.m_activePairs.end(); i++ )
		{
			P2PConnectionState* pair = i->GetSecond();
			if ( pair->m_jitDirectP2PRequested )
			{
				outVal.m_p2pConnectionPairCount++;
				if ( !pair->IsRelayed() )
					outVal.m_p2pDirectConnectionPairCount++;
			}
		}
	}

	void CNetServerImpl::GetTcpListenerLocalAddrs(CFastArray<AddrPort> &output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		output.Clear();
		if ( (int)m_TcpListenSockets.GetCount() == 0 )
		{
			output.Add(AddrPort::Unassigned);
			return;
		}

		m_TcpListenSockets[0]->GetSocketName();

		for ( int i = 0; i < (int)m_TcpListenSockets.GetCount(); i++ )
		{
			if ( m_TcpListenSockets[i] != NULL )
			{
				output.Add(m_TcpListenSockets[i]->GetSocketName());
			}
		}
	}

	void CNetServerImpl::GetUdpListenerLocalAddrs(CFastArray<AddrPort> &output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_udpAssignMode != ServerUdpAssignMode_Static )
			throw Exception("Cannot call GetUdpListenerLocalAddrs unless ServerUdpAssignMode_Static is used!");

		output.Clear();

		for ( int i = 0; i < (int)m_staticAssignedUdpSockets.GetCount(); i++ )
		{
			if ( m_staticAssignedUdpSockets[i]->GetSocket() )
			{
				output.Add(m_staticAssignedUdpSockets[i]->GetSocketName());
			}
		}
	}

	void CNetServerImpl::ConvertGroupToIndividualsAndUnion(
		int numberOfsendTo, const HostID * sendTo, HostIDArray &sendDestList)
	{
		for ( int i = 0; i < numberOfsendTo; ++i )
		{
			if ( HostID_None != sendTo[i] )
			{
				ConvertAndAppendP2PGroupToPeerList(sendTo[i], sendDestList);
			}
		}

		// 이 구문은 아래 p2p relay 정리 전에 필요할 듯
		UnionDuplicates<HostIDArray, HostID, int>(sendDestList);
	}

	bool CNetServerImpl::IsValidHostID(HostID id)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		return IsValidHostID_NOLOCK(id);

	}

	// P2P route가 가능한 항목끼리 linked list를 구축한다.
	// 2009.11.25 modify by ulelio : ping과 direct P2P 연결이 많은 놈을 선별하여 list를 구축하도록 개선.
	void CNetServerImpl::MakeP2PRouteLinks(SendDestInfoArray &tgt, int unreliableS2CRoutedMulticastMaxCount, int64_t routedSendMaxPing)
	{
		AssertIsLockedByCurrentThread();

		// #ifndef XXXXXXXXXXXXXXX
		// 	printf("MakeP2PRouteLinks 입력: ");
		// 	for(int i=0;i<tgt.GetCount();i++)
		// 	{
		// 		printf("%d ",tgt[i].mHostID);
		// 	}
		// 	printf("\n");
		// #endif

		// 이럴경우 link할 필요가 없다. 아래 로직은 많은 연산량을 요구하므로.
		if ( unreliableS2CRoutedMulticastMaxCount == 0 )
			return;

		m_connectionInfoList.Clear(); // 그러나 메모리 블럭은 남아있음쓰

		ConnectionInfo info;
		int i, j, k;
		int tgtCount = (int)tgt.GetCount();
		for ( i = 0; i < tgtCount; i++ )
		{
			SendDestInfo *lastLinkedItem = &tgt[i];
			lastLinkedItem->mP2PRoutePrevLink = NULL;
			lastLinkedItem->mP2PRouteNextLink = NULL;

			for ( j = i; j < tgtCount; j++ )
			{
				P2PConnectionState* state;
				if ( i != j &&
					(state = m_p2pConnectionPairList.GetPair(tgt[i].mHostID, tgt[j].mHostID)) != NULL )
				{
					if ( !state->IsRelayed() && state->m_recentPingMs < routedSendMaxPing )
					{
						info.state = state;
						info.hostIndex0 = i;
						info.hostIndex1 = j;
						m_connectionInfoList.Add(info);
						int connectioninfolistCount = (int)m_connectionInfoList.GetCount();
						// 현재 host의 directP2P 연결 갯수를 센다.
						for ( k = 0; k < connectioninfolistCount; ++k )
						{
							if ( m_connectionInfoList[k].hostIndex0 == i )
								++m_connectionInfoList[k].connectCount0;
							else if ( m_connectionInfoList[k].hostIndex1 == i )
								++m_connectionInfoList[k].connectCount1;

							if ( m_connectionInfoList[k].hostIndex0 == j )
								++m_connectionInfoList[k].connectCount0;
							else if ( m_connectionInfoList[k].hostIndex1 == j )
								++m_connectionInfoList[k].connectCount1;
						}
						info.Clear();
					}
				}
			}
		}

		// Ping 순으로 정렬한다.
		//StacklessQuickSort(connectioninfolist.GetData(), connectioninfolist.GetCount());
		HeuristicQuickSort(m_connectionInfoList.GetData(), (int)m_connectionInfoList.GetCount(), 100);//threshold를 100으로 지정하자...

		// router 후보 선별하여 indexlist에 담는다.
		m_routerIndexList.Clear();

		bool bContinue = false;
		for ( i = 0; i < (int)m_connectionInfoList.GetCount(); ++i )
		{
			bContinue = false;

			ConnectionInfo info2 = m_connectionInfoList[i];

			CFastArray<int, false, true>::iterator itr = m_routerIndexList.begin();
			for ( ; itr != m_routerIndexList.end(); ++itr )
			{
				// 이미 들어간 놈은 제외하자.
				if ( *itr == info2.hostIndex0 || *itr == info2.hostIndex1 )
				{
					bContinue = true;
					break;
				}
			}

			if ( bContinue )
				continue;

			if ( info2.connectCount0 >= info2.connectCount1 )
				m_routerIndexList.Add(info2.hostIndex0);
			else
				m_routerIndexList.Add(info2.hostIndex1);
		}

		// 링크를 연결해준다.
		int hostindex, nextindex;
		hostindex = nextindex = -1;
		SendDestInfo* LinkedItem = NULL;
		SendDestInfo* NextLinkedItem = NULL;

		intptr_t routerindexlistCount = m_routerIndexList.GetCount();
		for ( intptr_t routerindex = 0; routerindex < routerindexlistCount; ++routerindex )
		{
			hostindex = m_routerIndexList[routerindex];
			LinkedItem = &tgt[hostindex];

			if ( LinkedItem->mP2PRoutePrevLink != NULL || LinkedItem->mP2PRouteNextLink != NULL )
				continue;

			intptr_t count = 0;
			intptr_t connectioninfolistCount = m_connectionInfoList.GetCount();
			for ( i = 0; i < connectioninfolistCount && count < unreliableS2CRoutedMulticastMaxCount; ++i )
			{
				if ( hostindex == m_connectionInfoList[i].hostIndex0 )
				{
					//hostindex1을 뒤로 붙인다.
					nextindex = m_connectionInfoList[i].hostIndex1;
					NextLinkedItem = &tgt[nextindex];

					if ( NULL != NextLinkedItem->mP2PRoutePrevLink || NULL != NextLinkedItem->mP2PRouteNextLink )
						continue;

					LinkedItem->mP2PRouteNextLink = NextLinkedItem;
					NextLinkedItem->mP2PRoutePrevLink = LinkedItem;
					LinkedItem = LinkedItem->mP2PRouteNextLink;
					count++;
				}
				else if ( hostindex == m_connectionInfoList[i].hostIndex1 )
				{
					//hostindex0을 뒤로 붙인다.
					nextindex = m_connectionInfoList[i].hostIndex0;
					NextLinkedItem = &tgt[nextindex];

					if ( NULL != NextLinkedItem->mP2PRoutePrevLink || NULL != NextLinkedItem->mP2PRouteNextLink )
						continue;

					LinkedItem->mP2PRouteNextLink = NextLinkedItem;
					NextLinkedItem->mP2PRoutePrevLink = LinkedItem;
					LinkedItem = LinkedItem->mP2PRouteNextLink;
					count++;
				}
			}
		}

		// #ifndef XXXXXXXXXXXXXXX
		// 	printf("MakeP2PRouteLinks 출력: ");
		// 	for(int i=0;i<tgt.GetCount();i++)
		// 	{
		// 		printf("%d:",tgt[i].mHostID);
		// 		SendDestInfo* p=tgt[i].mP2PRouteNextLink;
		// 		while(p)
		// 		{
		// 			printf("->%d",p->mHostID);
		// 			p=p->mP2PRouteNextLink;
		// 		}
		// 		printf(" , ");
		// 	}
		// 	printf("\n");
		// #endif

		// 쓰고 남은거 정리
		m_connectionInfoList.Clear();
		m_routerIndexList.Clear();
	}

	void CNetServerImpl::Convert_NOLOCK(SendDestInfoArray& dest, HostIDArray& src)
	{
		AssertIsLockedByCurrentThread();
		dest.SetCount(src.GetCount());
		for ( int i = 0; i < src.GetCount(); i++ )
		{
			dest[i].mHostID = src[i];
			dest[i].mObject = GetSendDestByHostID_NOLOCK(src[i]);
		}
	}

	String CNetServerImpl::DumpGroupStatus()
	{
		return String();
	}

	bool CNetServerImpl::NextEncryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			output = rc->m_encryptCount;
			rc->m_encryptCount++;
			return true;
		}
		else if ( remote == HostID_Server )
		{
			output = m_selfEncryptCount;
			m_selfEncryptCount++;
			return true;
		}
		return false;
	}

	void CNetServerImpl::PrevEncryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_encryptCount--;
		}
		else if ( remote == HostID_Server )
		{
			m_selfEncryptCount--;
		}
	}


	bool CNetServerImpl::GetExpectedDecryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			output = rc->m_decryptCount;
			return true;
		}
		else if ( remote == HostID_Server )
		{
			output = m_selfDecryptCount;
			return true;
		}
		return false;
	}

	bool CNetServerImpl::NextDecryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);
		if ( rc != NULL )
		{
			rc->m_decryptCount++;
			return true;
		}
		else if ( remote == HostID_Server )
		{
			m_selfDecryptCount++;
			return true;
		}
		return false;
	}

	bool CNetServerImpl::IsConnectedClient(HostID clientHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(clientHostID);

		if ( rc == NULL )
			return false;
		else
			return true;
	}

	bool CNetServerImpl::EnableSpeedHackDetector(HostID clientID, bool enable)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* rc = GetAuthedClientByHostID_NOLOCK(clientID);
		if ( rc == NULL )
			return false;

		if ( enable && IsSimplePacketMode() == false )
		{
			if ( rc->m_speedHackDetector == NULL )
			{
				rc->m_speedHackDetector.Attach(new CSpeedHackDetector);
				m_s2cProxy.NotifySpeedHackDetectorEnabled(clientID, g_ReliableSendForPN, true);
			}
		}
		else
		{
			if ( rc->m_speedHackDetector != NULL )
			{
				rc->m_speedHackDetector.Free();
				m_s2cProxy.NotifySpeedHackDetectorEnabled(clientID, g_ReliableSendForPN, false);
			}
		}

		return true;
	}

	void CNetServerImpl::P2PGroup_CheckConsistency()
	{

	}

	void CNetServerImpl::DestroyEmptyP2PGroups()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		for ( P2PGroups_S::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); )
		{
			CP2PGroupPtr_S grp = i->GetSecond();
			if ( grp->m_members.size() == 0 )
			{
				HostID groupID = grp->m_groupHostID;
				i = m_P2PGroups.erase(i);
				m_HostIDFactory->Drop(groupID);
				EnqueueP2PGroupRemoveEvent(groupID);  // 이게 빠져있었네?
			}
			else
				i++;
		}
	}

	void CNetServerImpl::EnqueueP2PGroupRemoveEvent(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_eventSink_NOCSLOCK )
		{
			LocalEvent evt;
			evt.m_remoteHostID = groupHostID;
			evt.m_type = LocalEventType_P2PGroupRemoved;
			EnqueLocalEvent(evt, m_loopbackHost);
		}
	}

	void CNetServerImpl::TEST_SomeUnitTests()
	{
		TestAddrPort();

		// 서버를 실행한 상태이어야 한다. test 중 발생하는 에러는 event로 콜백될 것이다.
		//TestReliableUdpFrame();
		TestSendBrake();
		TestFastArray();
		TestStringA();
		TestStringW();
#if defined (_WIN32)
		// $$$; == > 본 작업이 완료되면 반드시 #ifdef를 제거해주시기 바랍니다.
		// CReaderWriterMonitor_NORECURSE에서 Proud::Event를 사용해서 현재는 사용 불가능
		TestReaderWriterMonitor();
#endif
		TestUdpConnReset();
	}

	void CNetServerImpl::TestSendBrake()
	{
		// 		CSendBrake b;
		// 		// 일단 속도 지정
		// 		b.SetMaxSendSpeed(3000);
		// 		// 최초 콜
		// 		b.MoveTimeToCurrent(10000);
		// 		// 송신한 거 있음
		// 		b.Accumulate(5000, 10000);
		// 		if (b.BrakeNeeded(10000))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
		// 		}
		// 		if (b.BrakeNeeded(10001))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
		// 		}
		// 		if (!b.BrakeNeeded(10002))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Time to Break!"));
		// 		}
		// 
		// 		b.Accumulate(5000, 10003);
		// 
		// 		if (b.BrakeNeeded(10003))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
		// 		}
		// 		if (b.BrakeNeeded(10004))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
		// 		}
		// 		if (!b.BrakeNeeded(10005))
		// 		{
		// 			EnqueueUnitTestFailEvent(_PNT("Time to Break!"));
		// 		}

	}

	// 	void CNetServerImpl::TestReliableUdpFrame()
	// 	{
	// 		CriticalSectionLock lock(GetCriticalSection(),true);
	// 		CHECK_CRITICALSECTION_DEADLOCK(this);
	// 
	// 		CHeldPtr<CFastHeap> heap(CFastHeap::New());
	// 
	// 		CUncompressedFrameNumberArray v(heap);
	// 		v.Add(5);
	// 		v.Add(1);
	// 		v.Add(2);
	// 		v.Add(4);
	// 
	// 		QuickSort(v.GetData(),4);
	// 		if(v[0]!=1 || v[1]!=2 || v[2]!=4 || v[3] !=5)
	// 			EnqueueUnitTestFailEvent(_PNT("Proud.QuickSort() failed!"));
	// 
	// 		ReliableUdpFrame a(heap);
	// 		a.m_ackedFrameNumbers=CCompressedFrameNumbers::NewInstance(heap);
	// 		for(int i=0;i<(int)v.GetCount();i++)
	// 		{
	// 			a.m_ackedFrameNumbers->AddSortedNumber(v[i]);
	// 		}
	// 
	// 		CMessage msg;
	// 		msg.UseInternalBuffer();
	// 		a.m_ackedFrameNumbers->WriteTo(msg);
	// 
	// 		msg.SetReadOffset(0);
	// 
	// 		ReliableUdpFrame b(heap);
	// 		b.m_ackedFrameNumbers=CCompressedFrameNumbers::NewInstance(heap);
	// 		b.m_ackedFrameNumbers->ReadFrom(msg);
	// 
	// 		CUncompressedFrameNumberArray w(heap);
	// 		b.m_ackedFrameNumbers->Uncompress(w);
	// 
	// 		if(!w.Equals(v))
	// 			EnqueueUnitTestFailEvent(_PNT("FrameNumberList pack/unpack failed!"));
	// 	}

	void CNetServerImpl::EnqueueUnitTestFailEvent(String msg)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		if ( m_eventSink_NOCSLOCK )
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_UnitTestFail;
			e.m_errorInfo->m_comment = msg;
			EnqueLocalEvent(e, m_loopbackHost);
		}
	}

	void CNetServerImpl::TestFastArray()
	{
		CFastArray<int> v;
		v.Add(5);
		v.Add(1);
		v.Add(2);
		v.Add(4);

		CFastArray<int> w;
#if (_MSC_VER>=1400)
		for each(int i in v)
		{
			w.Add(i);
		}
#else
		for ( CFastArray<int>::iterator i = v.begin(); i != v.end(); i++ )
		{
			w.Add(*i);
		}
#endif
		if ( !w.Equals(v) )
			EnqueueUnitTestFailEvent(_PNT("CFastArray iterator failed!"));
	}

	void CNetServerImpl::TestStringA()
	{
		StringA a = "123가나다";
		StringA b;
		CMessage msg;
		msg.UseInternalBuffer();
		msg << a;
		msg.SetReadOffset(0);
		msg >> b;

		if ( a != b )
		{
			EnqueueUnitTestFailEvent(_PNT("Proud.StringA serialization failed!"));
		}
	}

	void CNetServerImpl::TestStringW()
	{
		StringA a = "123가나다";
		StringA b;
		CMessage msg;
		msg.UseInternalBuffer();
		msg << a;
		msg.SetReadOffset(0);
		msg >> b;

		if ( a != b )
		{
			EnqueueUnitTestFailEvent(_PNT("Proud.String serialization failed!"));
		}
	}

#ifdef USE_HLA
	void CNetServerImpl::HlaFrameMove()
	{
		if (m_hlaSessionHostImpl != NULL)
			return m_hlaSessionHostImpl->HlaFrameMove();
		else
			throw Exception(HlaUninitErrorText);
	}

	void CNetServerImpl::HlaAttachEntityTypes(CHlaEntityManagerBase_S* entityManager)
	{
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaAttachEntityTypes(entityManager);
		else
			throw Exception(HlaUninitErrorText);
	}

	void CNetServerImpl::HlaSetDelegate(IHlaDelegate_S* dg)
	{
		if (m_hlaSessionHostImpl == NULL)
		{
			m_hlaSessionHostImpl.Attach(new CHlaSessionHostImpl_S(this));
		}
		m_hlaSessionHostImpl->HlaSetDelegate(dg);
	}

	CHlaSpace_S* CNetServerImpl::HlaCreateSpace()
	{
		if (m_hlaSessionHostImpl != NULL)
			return m_hlaSessionHostImpl->HlaCreateSpace();
		else
			throw Exception(HlaUninitErrorText);

	}

	void CNetServerImpl::HlaDestroySpace(CHlaSpace_S* space)
	{
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaDestroySpace(space);
		else
			throw Exception(HlaUninitErrorText);
	}

	CHlaEntity_S* CNetServerImpl::HlaCreateEntity(HlaClassID classID)
	{
		if (m_hlaSessionHostImpl != NULL)
			return m_hlaSessionHostImpl->HlaCreateEntity(classID);
		else
			throw Exception(HlaUninitErrorText);
	}

	void CNetServerImpl::HlaDestroyEntity(CHlaEntity_S* Entity)
	{
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaDestroyEntity(Entity);
		else
			throw Exception(HlaUninitErrorText);
	}

	void CNetServerImpl::HlaViewSpace(HostID viewerID, CHlaSpace_S* space, double levelOfDetail)
	{
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaViewSpace(viewerID, space, levelOfDetail);
		else
			throw Exception(HlaUninitErrorText);
	}

	void CNetServerImpl::HlaUnviewSpace(HostID viewerID, CHlaSpace_S* space)
	{
		if (m_hlaSessionHostImpl != NULL)
			m_hlaSessionHostImpl->HlaUnviewSpace(viewerID, space);
		else
			throw Exception(HlaUninitErrorText);
	}
#endif

	void CNetServerImpl::FallbackServerUdpToTcpOnNeed(CRemoteClient_S * rc, int64_t currTime)
	{
		// [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
		// 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.

		//if (currTime - rc->m_lastUdpPacketReceivedTime > m_defaultTimeoutTime) 이게 아니라 
		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL )
		{
			if ( rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled
				&& currTime - rc->m_ToClientUdp_Fallbackable.m_udpSocket->m_lastReceivedTime > CNetConfig::GetFallbackServerUdpToTcpTimeoutMs() )
			{
				// 굳이 이걸 호출할 필요는 없다. 클라에서 다시 서버로 fallback 신호를 할 테니까. 
				// -> 위와 같이 해버리면 TCP 연결은 되어 있으나 응답 안할 수도 있다.
				// 그래서 무조건 FallBack을 해야한다. by JiSamHong
				SecondChanceFallbackUdpToTcp(rc);

				// 클라에게 TCP fallback 노티를 한다.
				// 한편 서버에서는 TCP fallback을 해버린다. 이게 없으면 아래 RMI를 마구 보내는 불상사 발생.
				m_s2cProxy.NotifyUdpToTcpFallbackByServer(rc->m_HostID, g_ReliableSendForPN);
			}
		}
	}

	void CNetServerImpl::ArbitraryUdpTouchOnNeed(CRemoteClient_S * rc, int64_t currTime)
	{
		AssertIsLockedByCurrentThread();
		assert(CNetConfig::GetFallbackServerUdpToTcpTimeoutMs() > (CNetConfig::UnreliablePingIntervalMs * 5) / 2);

		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL )
		{
			// 클라로부터 최근까지도 UDP 수신은 됐지만 정작 ping만 안오면 다른 형태의 pong 보내기. 
			// 클라에서 서버로 대량으로 UDP를 쏘느라 정작 핑이 늑장하는 경우 필요하다.
			if ( currTime - rc->m_safeTimes.Get_lastUdpPingReceivedTime() > (CNetConfig::UnreliablePingIntervalMs * 3) / 2 &&
				currTime - rc->m_ToClientUdp_Fallbackable.m_udpSocket->m_lastReceivedTime < CNetConfig::UnreliablePingIntervalMs )
			{
				if ( currTime - rc->m_safeTimes.Get_arbitraryUdpTouchedTime() > CNetConfig::UnreliablePingIntervalMs )
				{
					rc->m_safeTimes.Set_arbitraryUdpTouchedTime(currTime);

					// 에코를 unreliable하게 보낸다.
					CMessage sendMsg;
					sendMsg.UseInternalBuffer();
					sendMsg.Write((char)MessageType_ArbitraryTouch);

					rc->m_ToClientUdp_Fallbackable.AddToSendQueueWithSplitterAndSignal_Copy(rc->GetHostID(), CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
				}
			}
		}
	}


	void CNetServerImpl::TestAddrPort()
	{
		AddrPort a = AddrPort::FromIPPort(_PNT("1.2.3.4"), 1);
		AddrPort b = AddrPort::FromIPPort(_PNT("1.2.3.5"), 1);
		AddrPort c = AddrPort::FromIPPort(_PNT("5.6.7.8"), 1);
		if ( !a.IsSameSubnet24(b) )
			EnqueueUnitTestFailEvent(_PNT("AddrPort.IsSameLan fail!"));
		if ( b.IsSameSubnet24(c) )
			EnqueueUnitTestFailEvent(_PNT("AddrPort.IsSameLan fail!"));
	}

	void CNetServerImpl::SetMaxDirectP2PConnectionCount(HostID clientID, int newVal)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* cl = GetAuthedClientByHostID_NOLOCK(clientID);
		if ( cl )
		{
			cl->m_maxDirectP2PConnectionCount = max(newVal, 0);
		}
	}

	void CNetServerImpl::GetUdpSocketAddrList(CFastArray<AddrPort> &output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_udpAssignMode != ServerUdpAssignMode_Static )
			throw Exception("Cannot call GetUdpSocketAddrList unless ServerUdpAssignMode_Static is used!");

		output.Clear();
		for ( int i = 0; i < (int)m_staticAssignedUdpSockets.GetCount(); i++ )
		{
			CSuperSocket* sk = m_staticAssignedUdpSockets[i];
			output.Add(sk->GetSocketName());
		}
	}

	void CNetServerImpl::OnSocketWarning(CSuperSocket *s, String msg)
	{
		//여기는 socket가 접근하므로, 메인락을 걸면 데드락 위험이 있다.
		//logwriter의 포인터를 보호할 크리티컬 섹션을 따로 걸어야 하나??ㅠㅠ
		//CriticalSectionLock lock(GetCriticalSection(),true);
		if ( m_logWriter )
		{
			m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, msg);
		}
	}

	int CNetServerImpl::GetInternalVersion()
	{
		return m_internalVersion;
	}

	void CNetServerImpl::ShowError_NOCSLOCK(ErrorInfoPtr errorInfo)
	{
		if ( m_logWriter )
		{
			m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, errorInfo->ToString());
		}

		CNetCoreImpl::ShowError_NOCSLOCK(errorInfo);
	}

	void CNetServerImpl::ShowWarning_NOCSLOCK(ErrorInfoPtr errorInfo)
	{
		String txt;
		txt.Format(_PNT("WARNING: %s"), errorInfo->ToString().GetString());
		if ( m_logWriter )
		{
			m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, txt);
		}
		if ( m_eventSink_NOCSLOCK )
		{
			m_eventSink_NOCSLOCK->OnWarning(errorInfo);
		}
	}

#if defined (_WIN32)
	// $$$; == > 본 작업이 완료되면 반드시 #ifdef를 제거해주시기 바랍니다.
	void CNetServerImpl::TestReaderWriterMonitor()
	{
		CReaderWriterMonitorTest test;
		test.m_main = this;
		test.RunTest();
	}
#endif

	void CNetServerImpl::SetEventSink(INetServerEvent *eventSink)
	{
		if ( AsyncCallbackMayOccur() )
			throw Exception(AsyncCallbackMayOccurErrorText);

		AssertIsNotLockedByCurrentThread();
		m_eventSink_NOCSLOCK = eventSink;
	}

	void CNetServerImpl::EnqueError(ErrorInfoPtr e)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_eventSink_NOCSLOCK )
		{
			LocalEvent e2;
			e2.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e2.m_type = LocalEventType_Error;

			e2.m_errorInfo->m_errorType = e->m_errorType;
			e2.m_errorInfo->m_comment = e->m_comment;
			e2.m_remoteHostID = e->m_remote;
			e2.m_remoteAddr = e->m_remoteAddr;

			EnqueLocalEvent(e2, m_loopbackHost);
		}
	}

	void CNetServerImpl::EnqueWarning(ErrorInfoPtr e)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_eventSink_NOCSLOCK )
		{
			LocalEvent e2;
			e2.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e2.m_type = LocalEventType_Warning;

			e2.m_errorInfo->m_errorType = e->m_errorType;
			e2.m_errorInfo->m_comment = e->m_comment;
			e2.m_remoteHostID = e->m_remote;
			e2.m_remoteAddr = e->m_remoteAddr;

			EnqueLocalEvent(e2, m_loopbackHost);
		}
	}

	void CNetServerImpl::EnquePacketDefragWarning(CSuperSocket* superSocket, AddrPort sender, const PNTCHAR* text)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* rc = GetRemoteClientBySocket_NOLOCK(superSocket, sender);

		if ( CNetConfig::EnablePacketDefragWarning )
			EnqueWarning(ErrorInfo::From(ErrorType_InvalidPacketFormat, rc ? rc->m_HostID : HostID_None, text));
	}

	// 클라이언트로부터 도착한 user defined request data와 클라의 IP 주소 등을 인자로 받는 콜백을 일으킨다.
	// 사용자는 그 콜백에서 클라의 연결 수락 혹은 거부를 판단할 것이다.
	// 그리고 그 콜백의 결과에서 나머지 연결 수락 처리(HostID 발급 등)을 마무리한다.
	void CNetServerImpl::EnqueClientJoinApproveDetermine(CRemoteClient_S *rc, const ByteArray& request)
	{
		AssertIsLockedByCurrentThread();

		assert(rc->m_tcpLayer->m_remoteAddr.IsUnicastEndpoint());

		if ( m_eventSink_NOCSLOCK )
		{
			LocalEvent e2;
			e2.m_type = LocalEventType_ClientJoinCandidate;
			e2.m_remoteAddr = rc->m_tcpLayer->m_remoteAddr;
			e2.m_connectionRequest = request;

			EnqueLocalEvent(e2, rc);
		}
	}

	void CNetServerImpl::ProcessOnClientJoinRejected(CRemoteClient_S *rc, const ByteArray &response)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock lock(GetCriticalSection(),true);

		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.		
		sendMsg.Write((char)MessageType_NotifyServerDeniedConnection);
		sendMsg.Write(response);

		rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
	}

	// 클라이언트-서버 인증 과정이 다 끝나면 
	void CNetServerImpl::ProcessOnClientJoinApproved(CRemoteClient_S *rc, const ByteArray &response)
	{
		AssertIsLockedByCurrentThread();

		if ( NULL == rc )
			throw Exception("Unexpected at candidate remote client removal!");

		// promote unmature client to mature, give new HostID
		HostID newHostID = m_HostIDFactory->Create();
		rc->m_HostID = newHostID;

		m_candidateHosts.Remove(rc);
		m_authedHostMap.Add(newHostID, rc);

		//// create magic number
		//rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = Win32Guid::NewGuid();

		//// send magic number and any UDP address via TCP
		//{
		//	CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		//	sendMsg.Write((char)MessageType_RequestStartServerHolepunch);
		//	sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);
		//	rc->m_ToClientTcp->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt());
		//}
#ifdef USE_HLA

		if ( m_hlaSessionHostImpl != NULL )
			m_hlaSessionHostImpl->CreateViewer(rc);
#endif
		// enqueue new client event
		{
			LocalEvent e;
			e.m_type = LocalEventType_ClientJoinApproved;
			e.m_netClientInfo = rc->ToNetClientInfo();
			e.m_remoteHostID = rc->m_HostID;
			EnqueLocalEvent(e, rc);
		}

		// 테스트 차원에서 강제로 TCP를 연결해제하기로 했다면
		if ( m_forceDenyTcpConnection_TEST )
		{
			/* HostID를 할당, 즉 OnClientJoin 조건을 만족하는 순간 바로 TCP 연결을 끊어버린다.
			이렇게 해야 유저가 OnClientJoin에서 SetHostTag를 콜 하기 전에
			OnClientLeave를 위한 local event가 enque되게 한다.
			이렇게 되어야
			OnClientJoin(X)->내부에서 X디스->SetHostTag(X,Y)->OnClientLeave(X)->GetHostTag(X)가 Y리턴하는지?
			를 테스트 할 수 있다.
			*/
			GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure, ByteArray(), _PNT("POCJA"), SocketErrorCode_Ok);
		}
		else // send welcome with HostID
		{
			assert(rc->m_tcpLayer->m_remoteAddr.IsUnicastEndpoint());

			CMessage sendMsg;
			sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
			sendMsg.Write((char)MessageType_NotifyServerConnectSuccess);
			sendMsg.Write(newHostID);
			sendMsg.Write(response);
			sendMsg.Write(NamedAddrPort::From(rc->m_tcpLayer->m_remoteAddr));
			if ( !IsSimplePacketMode() )
			{
				sendMsg.Write(m_instanceGuid);

				int c2s = 0;
				int s2c = 0;

				// ACR이 활성화되어 있는 경우, rc의 AMR 객체를 생성해 넣도록 한다.
				if ( rc->m_enableAutoConnectionRecovery )
				{
					c2s = CRandom::StaticGetInt();
					if ( c2s > 0 )
						c2s = -c2s;  // C2S는 음수, S2C는 양수. 디버깅을 편하게 하기 위함.
					s2c = CRandom::StaticGetInt();
					if ( s2c < 0 )
						s2c = -s2c;

					rc->m_tcpLayer->AcrMessageRecovery_Init(s2c, c2s);
				}
				sendMsg.Write(c2s);
				sendMsg.Write(s2c);
			}
			//NTTNTRACE("RemoteClient(%d)'s instance GUID = %s\n", rc->m_HostID, StringW2A(m_instanceGuid.ToString()).GetString());

			{
				// send queue에 넣는 것과 AMR set은 atomic해야 하므로 이 lock이 필요
				CriticalSectionLock lock(rc->m_tcpLayer->GetSendQueueCriticalSection(), true);

				rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
			}
		}
		/*	if(m_vizAgent)
		{
		CriticalSectionLock vizlock(m_vizAgent->m_cs,true);
		m_vizAgent->m_c2sProxy.NotifySrv_Clients_AddOrEdit(HostID_Server, g_ReliableSendForPN, rc->m_HostID);
		}*/
	}

	bool CNetServerImpl::AsyncCallbackMayOccur()
	{
		if ( m_listening )
			return true;
		else
			return false;
	}

	void CNetServerImpl::ProcessOneLocalEvent(LocalEvent& e, CHostBase* subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult)
	{
		AssertIsNotLockedByCurrentThread();

		if ( m_eventSink_NOCSLOCK != NULL/* && !m_shutdowning*/ )
		{
			INetServerEvent* eventSink_NOCSLOCK = (INetServerEvent*)m_eventSink_NOCSLOCK;

			switch ( (int)e.m_type )
			{
				case LocalEventType_ClientLeaveAfterDispose:
					outFunctionName = _PNT("OnClientLeave");
					eventSink_NOCSLOCK->OnClientLeave(e.m_netClientInfo, e.m_errorInfo, e.m_byteArrayComment);
#ifdef SUPPORTS_LAMBDA_EXPRESSION
					if (m_event_OnClientLeave)
						m_event_OnClientLeave(e.m_netClientInfo, e.m_errorInfo, e.m_byteArrayComment);
#endif // SUPPORTS_LAMBDA_EXPRESSION
					break;

				case LocalEventType_ClientOffline:
				{
					CRemoteOfflineEventArgs args;
					args.m_remoteHostID = e.m_remoteHostID;
					eventSink_NOCSLOCK->OnClientOffline(args);
				}
				break;
				case LocalEventType_ClientOnline:
				{
					CRemoteOnlineEventArgs args;
					args.m_remoteHostID = e.m_remoteHostID;
					eventSink_NOCSLOCK->OnClientOnline(args);

					CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(subject);
					if (rc != NULL)
					{
						// 초기화. 즉, ONClientOffline event를 enque를 할 수 있게 허락을.
						rc->m_enquedClientOfflineEvent = false;
					}
					else
					{
						// rc가 null 인데 요게 콜 되었으면 막장
						// (기반 로직이 잘못 되었다는 뜻 acrimpl_s.cpp 분석 요망)
						throw Exception(_PNT("Fatal error: %s"), _PNT("POLE"));
					}
				}
				break;

				case LocalEventType_ClientJoinApproved:
					outFunctionName = _PNT("OnClientJoin");
					eventSink_NOCSLOCK->OnClientJoin(e.m_netClientInfo);
#ifdef SUPPORTS_LAMBDA_EXPRESSION
					if (m_event_OnClientJoin)
						m_event_OnClientJoin->Run(e.m_netClientInfo);
#endif // SUPPORTS_LAMBDA_EXPRESSION

					break;

				case LocalEventType_AddMemberAckComplete:
					outFunctionName = _PNT("OnP2PGroupJoinMemberAckComplete");
					eventSink_NOCSLOCK->OnP2PGroupJoinMemberAckComplete(e.m_groupHostID, e.m_memberHostID, e.m_errorInfo->m_errorType);
					break;

				case LocalEventType_HackSuspected:
					outFunctionName = _PNT("OnClientHackSuspected");
					eventSink_NOCSLOCK->OnClientHackSuspected(e.m_remoteHostID, e.m_hackType);
					break;

				case LocalEventType_P2PGroupRemoved:
					outFunctionName = _PNT("OnP2PGroupRemoved");
					eventSink_NOCSLOCK->OnP2PGroupRemoved(e.m_remoteHostID);
					break;

				case LocalEventType_TcpListenFail:
					ShowError_NOCSLOCK(ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_Server));
					break;

				case LocalEventType_UnitTestFail:
					ShowError_NOCSLOCK(ErrorInfo::From(ErrorType_UnitTestFailed, HostID_Server, e.m_errorInfo->m_comment));
					break;

				case LocalEventType_Error:
					ShowError_NOCSLOCK(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
					break;

				case LocalEventType_Warning:
					ShowWarning_NOCSLOCK(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
					break;

				case LocalEventType_ClientJoinCandidate:
				{
					ByteArray response;
					outFunctionName = _PNT("OnConnectionRequest");

					// 사용자에게 접속 승인을 묻기
					bool app = eventSink_NOCSLOCK->OnConnectionRequest(e.m_remoteAddr, e.m_connectionRequest, response);

					CriticalSectionLock lock(GetCriticalSection(), true);
					CHECK_CRITICALSECTION_DEADLOCK(this);

					CRemoteClient_S *rc = LeanDynamicCastForRemoteClient(subject);
					if ( rc != NULL )
					{
						if ( app )
						{
							// 승인
							ProcessOnClientJoinApproved(rc, response);
						}
						else
						{
							// 거절
							ProcessOnClientJoinRejected(rc, response);
						}
					}
				}
				break;
				default:
					return;
			}
		}
	}

	String CNetServerImpl::GetConfigString()
	{
		String ret;
		ret.Format(_PNT("Listen=%s"), m_localNicAddr.GetString());

		return ret;
	}

	int CNetServerImpl::GetMessageMaxLength()
	{
		return m_settings.m_serverMessageMaxLength;
	}

	// 	void CNetServerImpl::PruneTooOldDefragBoardOnNeed()
	// 	{
	// 		CriticalSectionLock clk(GetCriticalSection(), true);		
	// 
	// 		if(GetCachedServerTime() - m_lastPruneTooOldDefragBoardTime > CNetConfig::AssembleFraggedPacketTimeout / 2)
	// 		{
	// 			m_lastPruneTooOldDefragBoardTime = GetCachedServerTime();
	// 
	// 			m_udpPacketDefragBoard->PruneTooOldDefragBoard();
	// 		}
	// 	}

	void CNetServerImpl::LogFreqFailOnNeed()
	{
		//CriticalSectionLock mainlock(GetCriticalSection(), true);
		//이거 쓰기싫어 m_fregFailNeed를 만듬.

		if ( m_fregFailNeed > 0 && GetCachedServerTimeMs() - m_lastHolepunchFreqFailLoggedTime > 30000 )
		{
			LogFreqFailNow();
		}
	}

	void CNetServerImpl::LogFreqFailNow()
	{
		if ( !m_freqFailLogMostRankText.IsEmpty() )
		{
			CErrorReporter_Indeed::Report(m_freqFailLogMostRankText);
		}
		m_lastHolepunchFreqFailLoggedTime = GetCachedServerTimeMs();
		m_freqFailLogMostRank = 0;
		m_freqFailLogMostRankText = _PNT("");

		AtomicCompareAndSwap32(1, 0, &m_fregFailNeed);
	}

	Proud::HostID CNetServerImpl::GetVolatileLocalHostID() const
	{
		return HostID_Server;
	}

	Proud::HostID CNetServerImpl::GetLocalHostID()
	{
		return HostID_Server;
	}

	void CNetServerImpl::RequestAutoPrune(CRemoteClient_S* rc)
	{
		// 클라 추방시 소켓을 바로 닫으면 직전에 보낸 RMI가 안갈 수 있다. 따라서 클라에게 자진 탈퇴를 종용하되 시한을 둔다.
		if ( !rc->m_safeTimes.Get_requestAutoPruneStartTime() )
		{
			rc->m_safeTimes.Set_requestAutoPruneStartTime(GetCachedServerTimeMs());
			m_s2cProxy.RequestAutoPrune(rc->m_HostID, g_ReliableSendForPN);
			rc->m_enableAutoConnectionRecovery = false;
		}
	}

	// PN 레벨 shutdown handshake 과정이 덜된 상태로 오래 방치된 것은 그냥 중단하고 TCP 디스
	void CNetServerImpl::HardDisconnect_AutoPruneGoesTooLongClient(CRemoteClient_S* rc)
	{
		// 5초로 잡아야 서버가 클라에게 많이 보내고 있던 중에도 직전 RMI의 확실한 송신이 되므로
		if ( rc->m_safeTimes.Get_requestAutoPruneStartTime() > 0
			&& GetCachedServerTimeMs() - rc->m_safeTimes.Get_requestAutoPruneStartTime() > 5000 )
		{
			rc->m_safeTimes.Set_requestAutoPruneStartTime(0);
			// caller에서는 Proud.CNetServerImpl.m_authedHosts를 iter중이 아니므로 안전
			GarbageHost(
				rc,
				ErrorType_DisconnectFromLocal,
				ErrorType_ConnectServerTimeout,
				ByteArray(),
				_PNT("APGTL"),
				SocketErrorCode_Ok);
		}
	}

	void CNetServerImpl::TEST_SetOverSendSuspectingThresholdInBytes(int newVal)
	{
		newVal = PNMIN(newVal, CNetConfig::DefaultOverSendSuspectingThresholdInBytes);
		newVal = PNMAX(newVal, 0);
		m_settings.m_overSendSuspectingThresholdInBytes = newVal;
	}

	void CNetServerImpl::TEST_SetTestping(HostID hostID0, HostID hostID1, int ping)
	{
		P2PConnectionStatePtr stat;
		stat = m_p2pConnectionPairList.GetPair(hostID0, hostID1);
		if ( stat != NULL )
		{
			stat->m_recentPingMs = ping;
		}
	}

	void CNetServerImpl::TEST_ForceDenyTcpConnection()
	{
		m_forceDenyTcpConnection_TEST = true;

	}

	bool CNetServerImpl::IsNagleAlgorithmEnabled()
	{
		return m_settings.m_enableNagleAlgorithm;
	}

	bool CNetServerImpl::IsValidHostID_NOLOCK(HostID id)
	{
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( id == HostID_None )
			return false;

		if ( id == HostID_Server )
			return true;

		if ( AuthedHostMap_Get(id) != NULL )
			return true;

		if ( GetP2PGroupByHostID_NOLOCK(id) )
			return true;

		return false;
	}

	void CNetServerImpl::RefreshSendQueuedAmountStat(CRemoteClient_S *rc)
	{
		AssertIsLockedByCurrentThread();

		// 송신큐 잔여 총량을 산출한다.
		int tcpQueuedAmount = 0;
		int udpQueuedAmount = 0;

		if ( rc->m_tcpLayer != NULL )
		{
			//CriticalSectionLock tcplock(rc->m_tcpLayer->GetSendQueueCriticalSection(),true);<= 내부에서 이미 잠그므로 불필요
			tcpQueuedAmount = rc->m_tcpLayer->GetSendQueueLength();
		}

		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket && rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
		{
			udpQueuedAmount = rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetUdpSendQueueLength(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
		}
		rc->m_sendQueuedAmountInBytes = tcpQueuedAmount + udpQueuedAmount;

		//add by rekfkno1 - sendqueue의 용량을 검사한다.
		int64_t curtime = GetCachedServerTimeMs();
		if ( rc->m_safeTimes.Get_sendQueueWarningStartTime() != 0 )
		{
			if ( rc->m_sendQueuedAmountInBytes > CNetConfig::SendQueueHeavyWarningCapacity )
			{
				if ( curtime - rc->m_safeTimes.Get_sendQueueWarningStartTime() > CNetConfig::SendQueueHeavyWarningTimeMs )
				{
					String str;
					str.Format(_PNT("sendQueue %dBytes"), rc->m_sendQueuedAmountInBytes);
					EnqueWarning(ErrorInfo::From(ErrorType_SendQueueIsHeavy, rc->GetHostID(), str));

					rc->m_safeTimes.Set_sendQueueWarningStartTime(curtime);
				}
			}
			else
			{
				rc->m_safeTimes.Set_sendQueueWarningStartTime(0);
			}
		}
		else if ( rc->m_sendQueuedAmountInBytes > CNetConfig::SendQueueHeavyWarningCapacity )
		{
			rc->m_safeTimes.Set_sendQueueWarningStartTime(curtime);
		}
	}

	void CNetServerImpl::AllowEmptyP2PGroup(bool enable)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( AsyncCallbackMayOccur() ) // 서버가 시작되었는가?
		{
			throw Exception("Cannot set AllowEmptyP2PGroup after the server has started.");
		}

		// 		if(!enable && m_startCreateP2PGroup && NULL == dynamic_cast<CAssignHostIDFactory*>(m_HostIDFactory.m_p))
		// 		{
		// 			throw Exception("Cannot set false after create P2PGroup when started");
		// 		}

		m_allowEmptyP2PGroup = enable;
	}

	void CNetServerImpl::TestUdpConnReset()
	{
		CTestUdpConnReset tester;
		tester.m_owner = this;
		tester.DoTest();
	}

#ifdef VIZAGENT
	void CNetServerImpl::EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey)
	{
		if ( !m_vizAgent )
		{
			m_vizAgent.Attach(new CVizAgent(this, vizServerAddr, vizServerPort, loginKey));
		}
}
#endif

#ifdef VIZAGENT
	void CNetServerImpl::Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext)
	{
		if ( m_vizAgent )
		{
			CFastArray<HostID> tgt;
			tgt.SetCount(remoteCount);
			UnsafeFastMemcpy(tgt.GetData(), remotes, sizeof(HostID) * remoteCount);
			m_vizAgent->m_c2sProxy.NotifyCommon_SendRmi(HostID_Server, g_ReliableSendForPN, tgt, summary);
		}
	}

	void CNetServerImpl::Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString)
	{
		if ( m_vizAgent )
		{
			m_vizAgent->m_c2sProxy.NotifyCommon_ReceiveRmi(HostID_Server, g_ReliableSendForPN, sender, RmiName, RmiID);
		}
	}
#endif

	/* to-client UDP socket을 생성하거나 풀에서 할당한다. 
	자세한 것은 함수 루틴 내 주석 참고.

	리턴값은 없지만, 이 함수 호출 후 m_ToClientUdp_Fallbackable.m_udpSocket 존재 
	여부로 UDP socket 존재 여부를 판단하자.
	(예전에는 bool이었지만, "이미 있는데 만들 필요 없음"이라는 의미로 오해될 수 있어서 
	이렇게 고침. 결국 중요한 것은 멱등성이니까.) */
	void CNetServerImpl::RemoteClient_New_ToClientUdpSocket(CRemoteClient_S* rc)
	{
		AssertIsLockedByCurrentThread();

		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL )
			return;

		// 한번 UDP socket을 생성하는 것을 실패한 적이 있다면 또 반복하지 않는다.
		if ( rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed )
			return;

		CSuperSocketPtr assignedUdpSocket;

		bool UdpSocketRecycleSuccess;
		if ( m_udpAssignMode == ServerUdpAssignMode_PerClient )
		{
			// 일전에 같은 HostID로 사용되었던 UDP socket을 재사용한다.
			UdpSocketRecycleSuccess = m_recycles.TryGetValue(rc->GetHostID(), assignedUdpSocket);

			if ( UdpSocketRecycleSuccess == false )
			{
				// 재사용 가능 UDP socket을 못 뽑았으므로, 새로 만든다.
				assignedUdpSocket = CSuperSocketPtr(CSuperSocket::New(this, SocketType_Udp));

				// per-remote UDP socket은 reuse를 끈다. 오차로 인해 같은 UDP port를 2개 이상의 UDP socket이 가지는 경우,
				// PNTest에서 "서버가 RMI 3003를 받는 오류"가 생길 수 있으므로.
				// 어떤 OS는 UDP socket가 기본 reuse ON인 것으로 기억함... 그래서 강제로 reuse OFF를 설정.
				assignedUdpSocket->m_fastSocket->SetSocketReuseAddress(false);

				// bind
				SocketErrorCode socketErrorCode = SocketErrorCode_Ok;
				if ( !m_localNicAddr.IsEmpty() )
					socketErrorCode = assignedUdpSocket->Bind(m_localNicAddr, 0);
				else
					socketErrorCode = assignedUdpSocket->Bind(0);

				if ( socketErrorCode != SocketErrorCode_Ok)
				{
					// bind가 실패하는 경우다. 이런 경우 자체가 있을 수 없는데.
					// UDP 소켓 생성 실패에 준한다. 에러를 내자. 그리고 이 클라의 접속을 거부한다.
					rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
					return;
				}

				// socket local 주소를 얻고 그것에 대한 매핑 정보를 설정한다.
				assert(assignedUdpSocket->GetLocalAddr().m_port > 0);
				m_localAddrToUdpSocketMap.Add(assignedUdpSocket->GetLocalAddr(), assignedUdpSocket);

				//assignedUdpSocket->ReceivedAddrPortToVolatileHostIDMap_Set(assignedUdpSocket->GetLocalAddr(), rc->GetHostID());

				m_netThreadPool->AssociateSocket(assignedUdpSocket->GetSocket());

				// overlapped send를 하므로 송신 버퍼는 불필요하다.
				// socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
				// send buffer 없애기는 coalsecse, throttling을 위해 필수다.
				// recv buffer 제거는 백해무익이므로 즐

				// CSuperSocket::Bind 내부에서 처리하기때문에 불필요
				// #ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
				// 			assignedUdpSocket->m_socket->SetSendBufferSize(0);
				// #endif // ALLOW_ZERO_COPY_SEND
				SetUdpDefaultBehavior_Server(assignedUdpSocket->GetSocket());

				// 첫번째 issue recv는 netio thread에서 할 것이다.
			}
		}
		else
		{
			// 고정된 UDP 소켓을 공용한다.
			assignedUdpSocket = GetAnyUdpSocket();
			if ( assignedUdpSocket == NULL )
			{
				rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
				EnqueError(ErrorInfo::From(ErrorType_UnknownAddrPort, rc->m_HostID, _PNT("No available UDP socket exists in Static Assign Mode.")));
				return;
			}
		}

		rc->m_ToClientUdp_Fallbackable.m_udpSocket = assignedUdpSocket;

		if ( m_udpAssignMode == ServerUdpAssignMode_PerClient )
		{
			if ( UdpSocketRecycleSuccess == false )
			{
				rc->m_ownedUdpSocket = assignedUdpSocket;
				SocketToHostsMap_SetForAnyAddr(assignedUdpSocket, rc);
				m_validSendReadyCandidates.Add(assignedUdpSocket, rc->m_ownedUdpSocket);

#if defined (_WIN32)
				CUseCounter counter(*(rc->m_ownedUdpSocket));
				// per RC UDP이면 첫 수신 이슈를 건다.
				rc->m_ownedUdpSocket->IssueRecv(true); // first
#endif
			}
		}

		SendOpt opt;
		opt.m_enableLoopback = false;
		opt.m_priority = MessagePriority_Ring0;
		opt.m_uniqueID.m_value = 0;
		if ( !m_usingOverBlockIcmpEnvironment )
			opt.m_ttl = 1; // NRM1 공유기 등은 알수없는 인바운드 패킷이 오면 멀웨어로 감지해 버린다. 따라서 거기까지 도착할 필요는 없다.

		// Windows 2003의 경우, (포트와 상관없이) 서버쪽에서 클라에게 UDP패킷을 한번 쏴줘야 해당 클라로부터 오는 패킷을 블러킹하지 않는다.
		CMessage ignorant;
		ignorant.UseInternalBuffer();
		ignorant.Write((char)MessageType_Ignore);

		CUseCounter counter(*(rc->m_ToClientUdp_Fallbackable.m_udpSocket));
		rc->m_ToClientUdp_Fallbackable.m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
			rc->GetHostID(), 
			FilterTag::CreateFilterTag(HostID_Server, HostID_None), 
			rc->m_tcpLayer->m_remoteAddr,
			ignorant, 
			GetPreciseCurrentTimeMs(),
			opt);

		return;
	}

	void CNetServerImpl::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		//CriticalSectionLock lock(GetCriticalSection(),true);
		//CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_userThreadPool )
		{
			m_userThreadPool->GetThreadInfos(output);
		}
	}

	void CNetServerImpl::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		if ( m_netThreadPool )
		{
			m_netThreadPool->GetThreadInfos(output);
		}
	}

	void CNetServerImpl::RemoteClient_RequestStartServerHolepunch_OnFirst(CRemoteClient_S* rc)
	{
		AssertIsLockedByCurrentThread();

		// create magic number
		if ( rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber == Guid() )
		{
			if ( m_simplePacketMode )
			{
				//{899F5C4D-D1C8-4F33-A7BF-04A89F9C88C3}
				PNGUID guidMagicNumber = {0x899f5c4d, 0xd1c8, 0x4f33, {0xa7, 0xbf, 0x4, 0xa8, 0x9f, 0x9c, 0x88, 0xc3}};
				Guid magicNumber = Guid(guidMagicNumber);
				rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = magicNumber;
			}
			else
				rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = Guid::RandomGuid();

			// send magic number and any UDP address via TCP
			{
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				sendMsg.Write((char)MessageType_RequestStartServerHolepunch);
				sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);

				rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
			}
		}
	}

	// 서버에서 먼저 UDP socket을 생성. 그리고 클라에게 "너도 만들어"를 지시.
	// 단, 아직 지시한 적도 없고, UDP socket을 생성한 적도 없을 때만.
	// 클라-서버 UDP 통신용.
	void CNetServerImpl::RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(CRemoteClient_S* rc)
	{
		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket == NULL
			&& rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting == false 
			&& m_settings.m_fallbackMethod == FallbackMethod_None )
		{
			RemoteClient_New_ToClientUdpSocket(rc);
			if (rc->m_ToClientUdp_Fallbackable.m_udpSocket)
			{
				// UDP 소켓을 성공적으로 생성했으니, 클라도 생성하라고 지시.
				NamedAddrPort sockAddr = rc->GetRemoteIdentifiableLocalAddr();
				m_s2cProxy.S2C_RequestCreateUdpSocket(rc->m_HostID, g_ReliableSendForPN, sockAddr);
				rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting = true;

				//NTTNTRACE("send requestcreateudpsocket = %s\n",StringT2A(sockAddr.ToString()).GetString());
			}
		}
	}

	// 	void CNetServerImpl::RemoteClient_RemoveFromCollections(CRemoteClient_S * rc)
	// 	{
	// 		// 목록에서 제거.
	// 		m_candidateHosts.Remove(rc);
	// 		m_authedHosts.RemoveKey(rc->m_HostID);
	// 		m_HostIDFactory->Drop(rc->m_HostID);
	// 		//m_UdpAddrPortToRemoteClientIndex.Remove(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
	// 		if (m_udpAssignMode == ServerUdpAssignMode_Static)
	// 			SocketToHostsMap_RemoveForAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket, rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
	// 		else
	// 			SocketToHostsMap_RemoveForAnyAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket);
	// 	}

	void CNetServerImpl::EnqueAddMemberAckCompleteEvent(HostID groupHostID, HostID addedMemberHostID, ErrorType result)
	{
		// enqueue 'completed join' event
		LocalEvent le;
		le.m_type = LocalEventType_AddMemberAckComplete;
		le.m_groupHostID = groupHostID;
		le.m_memberHostID = addedMemberHostID;
		le.m_remoteHostID = addedMemberHostID;
		le.m_errorInfo = ErrorInfo::From(result);

		// local event는 해당 host에 넣어주자.
		// OnP2PMemberJoinAck는 local host에 대한 것을 처리하는 것으로도 충분하겠지만
		// 혹시 모르니.
		if ( addedMemberHostID == HostID_Server )
			EnqueLocalEvent(le, m_loopbackHost);
		else
			EnqueLocalEvent(le, GetAuthedClientByHostID_NOLOCK(addedMemberHostID));
	}

	// memberHostID이 들어있는 AddMemberAckWaiter 객체를 목록에서 찾아서 모두 제거한다.
	void CNetServerImpl::AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(CP2PGroup_S* group, HostID memberHostID, ErrorType reason)
	{
		CFastArray<int> delList;
		CFastSet<HostID> joiningMemberDelList;

		for ( int i = (int)group->m_addMemberAckWaiters.GetCount() - 1; i >= 0; i-- )
		{
			CP2PGroup_S::AddMemberAckWaiter &a = group->m_addMemberAckWaiters[i];
			if ( a.m_joiningMemberHostID == memberHostID || a.m_oldMemberHostID == memberHostID )
			{
				delList.Add(i);

				// 제거된 항목이 추가완료대기를 기다리는 피어에 대한 것일테니
				// 추가완료대기중인 신규진입피어 목록만 따로 모아둔다.
				joiningMemberDelList.Add(a.m_joiningMemberHostID);
			}
		}

		for ( int i = 0; i < (int)delList.GetCount(); i++ )
		{
			group->m_addMemberAckWaiters.RemoveAndPullLast(delList[i]);
		}

		// memberHostID에 대한 OnP2PGroupJoinMemberAckComplete 대기에 대한 콜백에 대한 정리.
		// 중도 실패되어도 OnP2PGroupJoinMemberAckComplete 콜백을 되게 해주어야 하니까.
		for ( CFastSet<HostID>::iterator i = joiningMemberDelList.begin(); i != joiningMemberDelList.end(); i++ )
		{
			HostID joiningMember = *i;

			if ( group->m_addMemberAckWaiters.AckWaitingItemExists(joiningMember) == false )
				EnqueAddMemberAckCompleteEvent(group->m_groupHostID, joiningMember, reason);
		}
	}

	bool CNetServerImpl::SetHostTag(HostID hostID, void* hostTag)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CHostBase* subject = GetTaskSubjectByHostID_NOLOCK(hostID);
		if ( subject == NULL )
			return false;

		subject->m_hostTag = hostTag;
		return true;
	}

	// 	CNetServerImpl::CUdpAddrPortToRemoteClientIndex::CUdpAddrPortToRemoteClientIndex()
	// 	{
	// 		//InitHashTable(CNetConfig::ClientListHashTableValue);
	// 
	// 	}

	CNetServer *CNetServer::Create()
	{
		return new CNetServerImpl();
	}

	bool P2PConnectionState::ContainsHostID(HostID PeerID)
	{
		for ( int i = 0; i < 2; ++i )
		{
			if ( m_peers[i].m_ID == PeerID )
				return true;
		}
		return false;
	}

	// peerID의 홀펀칭 내외부 주소를 세팅한다.
	void P2PConnectionState::SetServerHolepunchOk(HostID peerID, AddrPort internalAddr, AddrPort externalAddr)
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( m_peers[i].m_ID == peerID )
			{
				m_peers[i].m_internalAddr = internalAddr;
				m_peers[i].m_externalAddr = externalAddr;
				return;
			}
		}

		for ( int i = 0; i < 2; i++ )
		{
			if ( m_peers[i].m_ID == HostID_None )
			{
				m_peers[i].m_internalAddr = internalAddr;
				m_peers[i].m_externalAddr = externalAddr;
				m_peers[i].m_ID = peerID;
				return;

			}
		}
	}

	AddrPort P2PConnectionState::GetInternalAddr(HostID peerID)
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( m_peers[i].m_ID == peerID )
			{
				return m_peers[i].m_internalAddr;
			}
		}

		return AddrPort::Unassigned;
	}


	AddrPort P2PConnectionState::GetExternalAddr(HostID peerID)
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( m_peers[i].m_ID == peerID )
			{
				return m_peers[i].m_externalAddr;
			}
		}

		return AddrPort::Unassigned;
	}

	// 서버와의 홀펀칭이 되어 있어, 외부 주소가 알려진 것의 갯수.
	int P2PConnectionState::GetServerHolepunchOkCount()
	{
		int ret = 0;
		for ( int i = 0; i < 2; i++ )
		{
			if ( m_peers[i].m_externalAddr != AddrPort::Unassigned )
				ret++;
		}

		return ret;
	}

	P2PConnectionState::P2PConnectionState(CNetServerImpl* owner, bool isLoopbackConnection)
	{
		m_jitDirectP2PRequested = false;
		m_owner = owner;
		m_relayed_USE_FUNCTION = true;
		m_dupCount = 0;
		m_isLoopbackConnection = isLoopbackConnection;
		m_recentPingMs = 0;
		m_releaseTimeMs = 0;

		m_memberJoinAndAckInProgress = false;

		m_toRemotePeerSendUdpMessageSuccessCount = 0;
		m_toRemotePeerSendUdpMessageTrialCount = 0;

		// 통계 정보를 업뎃한다.
		if ( !m_isLoopbackConnection )
		{
			//	m_owner->m_stats.m_p2pConnectionPairCount++;
		}
	}

	P2PConnectionState::~P2PConnectionState()
	{
		// 통계 정보를 업뎃한다.
		if ( !m_isLoopbackConnection )
		{
			//m_owner->m_stats.m_p2pConnectionPairCount--;
		}

		if ( m_relayed_USE_FUNCTION == false )
		{
			//m_owner->m_stats.m_p2pDirectConnectionPairCount--;
		}
	}

	// direct or relayed를 세팅한다. 
	// relayed로 세팅하는 경우 홀펀칭되었던 내외부 주소를 모두 리셋한다.
	void P2PConnectionState::SetRelayed(bool val)
	{
		if ( m_relayed_USE_FUNCTION != val )
		{
			if ( val )
			{
				//m_owner->m_stats.m_p2pDirectConnectionPairCount--;
				for ( int i = 0; i < 2; i++ )
					m_peers[i] = PeerAddrInfo();
			}
			else
			{
				//m_owner->m_stats.m_p2pDirectConnectionPairCount++;
			}
			m_relayed_USE_FUNCTION = val;
		}
	}

	bool P2PConnectionState::IsRelayed()
	{
		return m_relayed_USE_FUNCTION;
	}

	void P2PConnectionState::MemberJoin_Start(HostID peerIDA, HostID peerIDB)
	{
		m_peers[0] = PeerAddrInfo();
		m_peers[0].m_memberJoinAcked = false;
		m_peers[0].m_ID = peerIDA;

		m_peers[1] = PeerAddrInfo();
		m_peers[1].m_memberJoinAcked = false;
		m_peers[1].m_ID = peerIDB;

		m_memberJoinAndAckInProgress = true;
	}

	// 두 피어에 대한 P2P member join ack RMI가 왔는지?
	bool P2PConnectionState::MemberJoin_IsAckedCompletely()
	{
		int count = 0;

		for ( int i = 0; i < 2; ++i )
		{
			if ( m_peers[i].m_memberJoinAcked )
			{
				++count;
			}
		}

		return (count == 2);
	}

	void P2PConnectionState::MemberJoin_Acked(HostID peerID)
	{
		for ( int i = 0; i < 2; ++i )
		{
			if ( m_peers[i].m_ID == peerID )
			{
				m_peers[i].m_memberJoinAcked = true;
				return;
			}
		}
	}



	void P2PConnectionState::ClearPeerInfo()
	{
		m_peers[0] = PeerAddrInfo();
		m_peers[1] = PeerAddrInfo();
	}


	CHostBase* CNetServerImpl::GetTaskSubjectByHostID_NOLOCK(HostID subjectHostID)
	{
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);
		//CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		if ( subjectHostID == HostID_None )
			return NULL;

		if ( subjectHostID == HostID_Server )
			return m_loopbackHost;

		return AuthedHostMap_Get(subjectHostID);
	}

#ifdef _DEBUG // 이 함수는 매우 느리다. 디버깅 용도로만 써야 하므로.
	void CNetServerImpl::CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname)
	{
		AssertIsLockedByCurrentThread();

		for ( AuthedHostMap::iterator itr = m_authedHostMap.begin(); itr != m_authedHostMap.end(); ++itr )
		{
			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(itr->GetSecond());
			if ( rc != NULL )
			{
				if ( rc->IsLockedByCurrentThread() == true )
				{
					String strfmt;
					strfmt.Format(_PNT("RemoteClient DeadLock - %s"), functionname);
					ShowUserMisuseError(strfmt);
				}

				if ( m_udpAssignMode == ServerUdpAssignMode_PerClient )
				{
					if ( rc->m_ownedUdpSocket != NULL )
					{
#ifdef PN_LOCK_OWNER_SHOWN
						if ( Proud::IsCriticalSectionLockedByCurrentThread(rc->m_ownedUdpSocket->m_cs) )
						{
							String strfmt;
							strfmt.Format(_PNT("UDPSocket DeadLock - %s"), functionname);
							ShowUserMisuseError(strfmt);
						}
#endif
					}
				}
			}
		}

		if ( m_udpAssignMode == ServerUdpAssignMode_Static )
		{
			intptr_t count = m_staticAssignedUdpSockets.GetCount();
			for ( intptr_t i = 0; i < count; ++i )
			{
				if ( m_staticAssignedUdpSockets[i]->IsLockedByCurrentThread() == true )
				{
					String strfmt;
					strfmt.Format(_PNT("UDPSocket DeadLock - %s"), functionname);
					ShowUserMisuseError(strfmt);
				}
			}
		}
	}
#endif // _DEBUG

	// 받은 메시지를 networker thread에서 직접 처리하거나, user work로 넣어 버려 user worker thread에서 처리되게 한다.
	void CNetServerImpl::ProcessMessageOrMoveToFinalRecvQueue(
		SocketType socketType,
		CSuperSocket* socket,
		CReceivedMessageList &extractedMessages)
	{
		AssertIsNotLockedByCurrentThread();

		// main lock을 걸었으니 송신자의 HostID를 이제 찾을 수 있다.
		// 		CHostBase* pFrom = SocketToHostMap_Get(socket);
		// 		HostID fromID = HostID_None;
		// 		if (pFrom != NULL)
		// 			fromID = pFrom->GetHostID();

		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// final recv queue로 옮기거나 여기서 처리한다.
		for ( CReceivedMessageList::iterator i = extractedMessages.begin(); i != extractedMessages.end(); i++ )
		{
			// code profile 결과, main lock의 cost가 꽤 크다. 따라서 한번 main lock하고 다 하자.
      // 다만, 아래 케이스 중 unlock을 중도 하는 경우가 있어, lock if unlocked를 한다.
			if (!mainLock.IsLocked())
				mainLock.Lock();

			// Q: main lock을 여기서 걸면 성능 병목 아닌가요?
			// A: 이미 패킷 조립 과정과 syscall은 잠금 없이 수행했다. 이제부터는 NS 내부의
			// 로직 처리를 하는 단계이므로 잠금이 있어도 괜찮다.
			// syscall이 성능의 대부분을 잡아먹었기 때문에 충분하다.
			CHECK_CRITICALSECTION_DEADLOCK(this);

			CReceivedMessage &receivedMessage = *i;

			// 아직 MessageType_NotifyServerConnectSuccess가 미처리 remote는 hostID=none이다. 
			// 따라서 messageList의 hostID를 믿지 말고, socket으로부터 remote를 찾도록 하자.

			// ProcessMessage 내부에서 Unlock을 수행하는 로직이 있어서 매번 루프마다 mainLock을 잡도록 수정
			CRemoteClient_S* rc = GetRemoteClientBySocket_NOLOCK(socket, receivedMessage.m_remoteAddr_onlyUdp);  // super socket이 연관된 remote 객체를 찾자.

			// Socket에 해당하는 RC가 없다면 메세지 처리를 중단
			if ( rc == NULL )
			{
				if ( socketType == SocketType_Udp )
				{
					receivedMessage.GetReadOnlyMessage().SetReadOffset(0);
					ProcessMessage_FromUnknownClient(receivedMessage.m_remoteAddr_onlyUdp,
													 receivedMessage.GetReadOnlyMessage(),
													 socket);
					continue;
				}
				else
				{
					// 연결유지기능이 들어가기전까지는 Remote가 NULL인 경우가 발생하는지 확인차 추가한 코드입니다.

					// 해당 경우가 발생
					//assert(0);
					break;
				}
			}

			if (rc)
			{
				// messageList 각 항목의 m_remoteHostID 채우기
				receivedMessage.m_remoteHostID = rc->m_HostID;


/*				// 3003 버그 잡기 위해!
				rc->m_MessageAnalyzeLog.Add(receivedMessage.m_unsafeMessage, new MessageInternalLayer);
*/

			}
			else
				receivedMessage.m_remoteHostID = HostID_None;

/*			// 3003 오류 검사용
			if (!rc)
			{
				int64_t currTime = GetPreciseCurrentTimeMs();
				CriticalSectionLock lock(DebugCommon::m_critSec, true);
				for (auto i:DebugCommon::m_closedSockets)
				{
					ClosedSocketInfo info = i;
					if (info.m_localAddr == receivedMessage.m_remoteAddr_onlyUdp)
					{
						// 이미 닫은 바 있는 소켓의 주소가 재사용되고 있네! 일단 저장해 두자.
						// RMI 3003을 받을 때 여기서 받은 주소가 매치되는지 검사하자.
						DebugCommon::m_suspectSockets.AddTail(info);
					}
				}
			}*/

			// NOTE: 추출된 message들은 모두 같은 endpoint로부터 왔다. 
			// 따라서 모두 같은 hostID이므로 아래 if 구문은 safe.
			ASSERT_OR_HACKED(receivedMessage.m_unsafeMessage.GetReadOffset() == 0);

			// 수신 미보장 부분임에도 불구하고 이미 처리한 메시지면 false로 세팅된다.
			// 즉 무시하고 버려진다.
			bool useMessage;
			if ( !receivedMessage.m_hasMessageID || socketType != SocketType_Tcp )
			{
				useMessage = true;
			}
			else
			{
				if (rc)
				{
					useMessage = rc->m_tcpLayer->AcrMessageRecovery_ProcessReceivedMessageID(receivedMessage.m_messageID);
				}
				else
				{
					useMessage = true;
				}
			}

			if ( useMessage )
			{
				ProcessMessage_ProudNetLayer(&mainLock,
					receivedMessage,
					rc,
					(socketType == SocketType_Tcp) ? NULL : socket);

				// 주의: RequestServerConnection는 내부적으로 main unlock을 한번 한다. 
				// 이 때문에, 이 줄 이하에서는 상태가 다른 스레드에 변해 있을 수 있다. 
				// 따라서, 루틴을 추가할 때 주의하도록 하자.
			}
		}
	}

	bool CNetServerImpl::ProcessMessage_ProudNetLayer(CriticalSectionLock* mainLocker, CReceivedMessage &receivedInfo, CRemoteClient_S *rc, CSuperSocket* udpSocket)
	{


		/*try
		{*/
		AssertIsLockedByCurrentThread();

		CMessage &recvedMsg = receivedInfo.m_unsafeMessage;
		int orgReadOffset = recvedMsg.GetReadOffset();

		// _mm_prefetch((char*)(recvedMsg.GetData() + orgReadOffset), _MM_HINT_NTA);이거 적용해봐도 성능 차이 거의 없는데?

		MessageType type;
		if ( recvedMsg.Read(type) == false )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		//		NTTNTRACE("[ACR] Processing message %d in server~\n", type);

		bool messageProcessed = false;
		bool isRealUdp = (udpSocket != NULL ? true : false);

		switch ( (int)type )
		{
			case MessageType_ServerHolepunch:
				// 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
				ASSERT_OR_HACKED(receivedInfo.GetRemoteAddr().IsUnicastEndpoint());
				IoCompletion_ProcessMessage_ServerHolepunch(
					receivedInfo.GetReadOnlyMessage(),
					receivedInfo.GetRemoteAddr(), udpSocket);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_ServerHolepunch:
				// 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
				ASSERT_OR_HACKED(receivedInfo.GetRemoteAddr().IsUnicastEndpoint());
				IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(
					receivedInfo.GetReadOnlyMessage(),
					receivedInfo.GetRemoteAddr(), udpSocket);
				messageProcessed = true;
				break;
			case MessageType_RequestServerConnection:
				// isRealUdp를 멤버로 넣고 그 이유가 AssertIsZeroUseCount 때문인데요, 차라리 그냥 
				// 이 함수 윗단에서 AssertIsZeroUseCount를 검사하게 하고 아래 각 함수 안에서는
				// 그걸 검사하지 말게 합시다. 
				IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(mainLocker, recvedMsg, rc);
				messageProcessed = true;
				break;

			case MessageType_RequestAutoConnectionRecovery:
				IoCompletion_ProcessMessage_RequestAutoConnectionRecovery(recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_UnreliablePing:
				IoCompletion_ProcessMessage_UnreliablePing(recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_SpeedHackDetectorPing:
				IoCompletion_ProcessMessage_SpeedHackDetectorPing(recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_UnreliableRelay1:
				IoCompletion_ProcessMessage_UnreliableRelay1(mainLocker, recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_ReliableRelay1:
				IoCompletion_ProcessMessage_ReliableRelay1(mainLocker, recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_UnreliableRelay1_RelayDestListCompressed:
				IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(mainLocker, recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_LingerDataFrame1:
				IoCompletion_ProcessMessage_LingerDataFrame1(mainLocker, recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_NotifyHolepunchSuccess:
				IoCompletion_ProcessMessage_NotifyHolepunchSuccess(recvedMsg, rc);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_NotifyHolepunchSuccess:
				IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(recvedMsg, rc);
				messageProcessed = true;
				break;
				//case MessageType_ReliableUdp_P2PDisconnectLinger1:
				//    NetWorkerThread_ReliableUdp_P2PDisconnectLinger1(recvedMsg, rc);
				//    messageProcessed = true;
				//    break;
			case MessageType_Rmi:
				IoCompletion_ProcessMessage_Rmi(receivedInfo, messageProcessed, rc, isRealUdp);
				break;

			case MessageType_UserMessage:
				IoCompletion_ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_UserMessage, messageProcessed, rc, isRealUdp);
				break;
			case MessageType_Hla:
				IoCompletion_ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_Hla, messageProcessed, rc, isRealUdp);
				break;

			case MessageType_Encrypted_Reliable_EM_Secure:
			case MessageType_Encrypted_Reliable_EM_Fast:
			case MessageType_Encrypted_UnReliable_EM_Secure:
			case MessageType_Encrypted_UnReliable_EM_Fast:
			{
				CReceivedMessage decryptedReceivedMessage;
				if ( ProcessMessage_Encrypted(type, receivedInfo, decryptedReceivedMessage.m_unsafeMessage) )
				{
					decryptedReceivedMessage.m_relayed = receivedInfo.m_relayed;
					decryptedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
					decryptedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
					decryptedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

					messageProcessed |= ProcessMessage_ProudNetLayer(mainLocker,
																	 decryptedReceivedMessage,
																	 rc, udpSocket);
				}
			}
			break;

			case MessageType_Compressed:
			{
				CReceivedMessage uncompressedReceivedMessage;
				if ( ProcessMessage_Compressed(receivedInfo, uncompressedReceivedMessage.m_unsafeMessage) )
				{
					uncompressedReceivedMessage.m_relayed = receivedInfo.m_relayed;
					uncompressedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
					uncompressedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
					uncompressedReceivedMessage.m_compressMode = receivedInfo.m_compressMode;
					// Send 할 때 압축 후 암호화를 한다. Recv 시에 복호화 후 압축을 푼다. 따라서 여기서 m_encryptMode 값을 셋팅해 주어야 압축정보를 제대로 받을 수 있다.
					uncompressedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

					messageProcessed |= ProcessMessage_ProudNetLayer(mainLocker,
																	 uncompressedReceivedMessage,
																	 rc, udpSocket);
				}
				break;
			}

			case MessageType_PolicyRequest:
				//IoCompletion_ProcessMessage_PolicyRequest(rc);
				messageProcessed = true;
			default:
				break;
		}

		// 만약 잘못된 메시지가 도착한 것이면 이미 ProudNet 계층에서 처리한 것으로 간주하고
		// 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
		// 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
		if ( messageProcessed == true &&
			receivedInfo.GetReadOnlyMessage().GetLength() != receivedInfo.GetReadOnlyMessage().GetReadOffset() 
			&& type != MessageType_Encrypted_Reliable_EM_Secure 
			&& type != MessageType_Encrypted_Reliable_EM_Fast 
			&& type != MessageType_Encrypted_UnReliable_EM_Secure 
			&& type != MessageType_Encrypted_UnReliable_EM_Fast ) // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
		{
			messageProcessed = true;
			// 2009.11.26 add by ulelio : 에러시에 마지막 메세지를 저장한다.
			String comment;
			comment.Format(_PNT("%s, type[%d]"), _PNT("PMPNL"), (int)type);
			ByteArray lastreceivedMessage(recvedMsg.GetData(), recvedMsg.GetLength());
			EnqueError(ErrorInfo::From(ErrorType_InvalidPacketFormat, receivedInfo.m_remoteHostID, comment, lastreceivedMessage));
		}
		//AssureMessageReadOkToEnd(messageProcessed, type,receivedInfo,rc);
		if ( messageProcessed == false )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		return true;
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_ServerHolepunch(CMessage &msg, AddrPort from, CSuperSocket *udpSocket)
	{
		if ( m_simplePacketMode )
			return;

		// 이 메시지를 TCP로 의도적으로 해킹 전송을 할 경우, 그냥 버려야
		assert(udpSocket != NULL);
		if ( udpSocket == NULL )
		{
			return;
		}
		udpSocket->AssertIsZeroUseCount();
		// UDP socket으로 온게 아니면 무시한다. 어차피 UDP 전용 메시지니까.
		//if (udpSocket == NULL)
		//	return;

		NTTNTRACE("C2S Holepunch from %s\n", from.ToString().GetString());

		Guid magicNumber;
		if ( msg.Read(magicNumber) == false )
			return;

		// ack magic number and remote UDP address via UDP once
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_ServerHolepunchAck);
		sendMsg.Write(magicNumber);
		sendMsg.Write(from);

		//위에서 usecount +1하고 와야함.

		udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(HostID_None,
															FilterTag::CreateFilterTag(HostID_Server, HostID_None),
															from,
															CSendFragRefs(sendMsg),
															GetPreciseCurrentTimeMs(),
															SendOpt(MessagePriority_Holepunch, true));

		//이하는 메인 lock하에 진행.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_logWriter )
		{
			String txt;
			txt.Format(_PNT("MessageType_ServerHolepunch from %s"), from.ToString().GetString());
			m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(CMessage &msg, AddrPort from, CSuperSocket *udpSocket)
	{
		if ( m_simplePacketMode )
			return;

		// 이 메시지를 TCP로 의도적으로 해킹 전송을 할 경우, 그냥 버려야
		assert(udpSocket != NULL);
		if ( udpSocket == NULL )
		{
			return;
		}
		udpSocket->AssertIsZeroUseCount();

		// UDP socket으로 온게 아니면 무시한다. 어차피 UDP 전용 메시지니까.
		/*if (udpSocket == NULL)
		return;*/

		Guid magicNumber;
		if ( msg.Read(magicNumber) == false )
			return;

		HostID peerID;
		if ( msg.Read(peerID) == false )
			return;

		// ack magic number and remote UDP address via UDP once
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_PeerUdp_ServerHolepunchAck);
		sendMsg.Write(magicNumber);
		sendMsg.Write(from);
		sendMsg.Write(peerID);

		udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(HostID_None,
															FilterTag::CreateFilterTag(HostID_Server, HostID_None),
															from,
															CSendFragRefs(sendMsg),
															GetPreciseCurrentTimeMs(),
															SendOpt(MessagePriority_Holepunch, true));


		//이하는 메인 lock하에 진행.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( m_logWriter )
		{
			String txt;
			txt.Format(_PNT("MessageType_PeerUdp_ServerHolepunch from %s"), from.ToString().GetString());
			m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
		}
	}

	// 클라로부터 온 최초의 메시지이다.
	// 주의: 중간에 main unlock을 한다. 따라서 이 함수의 caller에서는 상태가 변한다는 전제의 코딩이 필수.
	void CNetServerImpl::IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(
		CriticalSectionLock* mainLocker,
		CMessage &msg,
		CRemoteClient_S *rc)
	{
		rc->AssertIsSocketNotLockedByCurrentThread();

		AssertIsLockedByCurrentThread();

		// 미인증 클라인지 확인.
		// 해킹된 클라에서 이게 반복적으로 오는 것일 수 있다. 그런 경우 무시하도록 하자.
		if ( !m_candidateHosts.ContainsKey(rc) )
		{
			NotifyProtocolVersionMismatch(rc);
			return;
		}

		ByteArray outRandomBlock;
		ByteArray outFastRandomBlock;
		ByteArray encryptedAESKeyBlob;
		ByteArray encryptedFastKeyBlob;

		ByteArray userData;
		Guid protocolVersion;
		int internalVersion;

		bool enableAutoConnectionRecovery = false;

		int runtimePlatform = 0;

		// simple packet mode가 아닌 경우 복잡한 처리를 수행.
		if ( !m_simplePacketMode )
		{
			// 라이선스 유효 검사
			if ( !msg.Read(runtimePlatform) )
			{
				NotifyProtocolVersionMismatch(rc);
				return;
			}

			// ACR 설정을 켠 클라인지?
			if ( !msg.Read(enableAutoConnectionRecovery) )
			{
				NotifyProtocolVersionMismatch(rc);
				return;
			}
			if (m_settings.m_enableEncryptedMessaging == true) {
				// AES key를 얻는다.
				if (msg.Read(encryptedAESKeyBlob) == false)
					return;

				// Fast key를 얻는다.
				if (msg.Read(encryptedFastKeyBlob) == false)
					return;
			}
		}

		if ( msg.Read(userData) == false )
		{
			NotifyProtocolVersionMismatch(rc);
			return;
		}
		if ( msg.Read(protocolVersion) == false )
		{
			NotifyProtocolVersionMismatch(rc);
			return;
		}
		if ( msg.Read(internalVersion) == false )
		{
			NotifyProtocolVersionMismatch(rc);
			return;
		}

		if ( !m_simplePacketMode )
		{
#ifndef PROUDSRCCOPY
			if ( !((CPNLic*)m_lic)->POCJ(runtimePlatform, rc->m_tcpLayer->m_remoteAddr) )
			{
				NotifyProtocolVersionMismatch(rc);
				return;
			}
			else
			{
				rc->m_RT = runtimePlatform;
			}
#endif

#ifndef ENCRYPT_NONE
			if (m_settings.m_enableEncryptedMessaging == true) {
				// main unlock을 한다. 키 복호화는 많은 시간을 차지하기 때문에, 병렬 병목을 피하기 위해서다.
				// 다시 main lock 후에는 상태가 변해있을 수 있음에 주의해서 코딩할 것.

				// rc가 돌발 delete되면 안되므로
				CSessionKey rc_sessionKey;

				// 비파괴 보장 상태로 만든다.
				// main lock 상태에서 rc는 파괴되지 않으므로 댕글링 걱정 뚝.			
#ifdef PARALLEL_RSA_DECRYPTION
				AtomicIncrement32(&rc->m_useCount);
				mainLocker->Unlock();
				AssertIsNotLockedByCurrentThread();
#endif

				// 암호화 실패나 성공시 아래가 반드시 실행되게 한다.
				RunOnScopeOut rollback([&mainLocker, rc](){
#ifdef PARALLEL_RSA_DECRYPTION
					mainLocker->Lock();
					AtomicDecrement32(&rc->m_useCount); // 주의: main lock 후 이것의 refcount -1을 해야.
#endif
				});

				// 암호화된 세션키를 복호화해서 클라이언트 세션키에 넣는다.
				// m_selfXchgKey는 서버 시작시에 한번만 만드므로 락을 안걸어도 괜찮을것 같다.
				// TODO: 여기는 처리가 오래 걸리며 하필 여기를 처리할 때 main lock 상태인데...병목이다. 방법 없을까?
				if (ErrorInfoPtr err = CCryptoRsa::DecryptSessionKeyByPrivateKey(outRandomBlock, encryptedAESKeyBlob, m_selfXchgKey))
				{
					return;
				}

				// 실패시 실행할 마무리 루틴
				auto failCase = [this, rc](){
					EnqueError(ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("Failed to create SessionKey.")));

					CMessage sendMsg;
					sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
					sendMsg.Write((char)MessageType_NotifyServerDeniedConnection);
					sendMsg.Write(ByteArray());   // user-defined reply data

					rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt());
				};

				// AES 키 세팅 / Fast 키 세팅
				if (CCryptoAes::ExpandFrom(rc_sessionKey.m_aesKey, outRandomBlock.GetData(), m_settings.m_encryptedMessageKeyLength / 8) != true)
				{
					rollback.Run();
					failCase();
					return;
				}
				if (CCryptoAes::DecryptByteArray(rc_sessionKey.m_aesKey, encryptedFastKeyBlob, outFastRandomBlock) != true)
				{
					rollback.Run();
					failCase();
					return;
				}

				if (CCryptoFast::ExpandFrom(rc_sessionKey.m_fastKey, outFastRandomBlock.GetData(), m_settings.m_fastEncryptedMessageKeyLength / 8) != true)
				{
					rollback.Run();
					failCase();
					return;
				}
				rollback.Run();

				// 복호화 성공
				rc->m_sessionKey = rc_sessionKey;
				rc->m_sessionKey.m_aesKeyBlock = outRandomBlock;	// copy
				rc->m_sessionKey.m_fastKeyBlock = outFastRandomBlock;	// copy
			}
		}
#endif

		// protocol version match and assign received session key
		if ( protocolVersion != m_protocolVersion || internalVersion != m_internalVersion )
		{
			NotifyProtocolVersionMismatch(rc);
			return;
		}

		rc->m_enableAutoConnectionRecovery = enableAutoConnectionRecovery;

		// 접속 허가를 해도 되는지 유저에게 묻는다.
		// 이 스레드는 이미 CS locked이므로 여기서 하지 말고 스레드 풀에서 콜백받은 후 후반전으로 ㄱㄱ
		if ( m_eventSink_NOCSLOCK != NULL )
		{
			// 콜백을 일으킴. 자세한 것은 이 함수의 주석을 참고.
			EnqueClientJoinApproveDetermine(rc, userData);
		}
		else
		{
			// 사용자 정의 콜백 함수가 없음.
			// 무조건 성공 처리.
			ProcessOnClientJoinApproved(rc, ByteArray());
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_UnreliablePing(CMessage &msg, CRemoteClient_S *rc)
	{
		int64_t clientLocalTime;
		int measuredLastLag;
		int64_t speed = 0;
		int CSLossPercent = 0;

		if ( !msg.Read(clientLocalTime) )
			return;

		if ( !msg.Read(measuredLastLag) )
			return;

		if ( !msg.ReadScalar(speed) )
			return;

		if ( !msg.Read(CSLossPercent) )
			return;

		rc->AssertIsSocketNotLockedByCurrentThread();
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		int64_t currTime = GetPreciseCurrentTimeMs();

		// 가장 마지막에 unreliable ping을 받은 시간을 체크해둔다.
		rc->m_safeTimes.Set_lastUdpPingReceivedTime(GetCachedServerTimeMs());

		// 측정된 랙도 저장해둔다.
		rc->m_lastPingMs = max(measuredLastLag, 1); //측정된 값이 1보다 작을경우 1로 측정값을 바꾼다.
		rc->m_recentPingMs = max(LerpInt(rc->m_recentPingMs, measuredLastLag, 1, 2), 1);
		rc->m_unreliableRecentReceivedSpeed = speed;

		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL )
		{
			CUseCounter counter(*(rc->m_ToClientUdp_Fallbackable.m_udpSocket));
			CriticalSectionLock lock(rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetSendQueueCriticalSection(), true);
			rc->m_ToClientUdp_Fallbackable.m_udpSocket->SetReceiveSpeedAtReceiverSide(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere(), speed, CSLossPercent, GetPreciseCurrentTimeMs());
		}

		// 에코를 unreliable하게 보낸다.
		CMessage sendMsg;
		sendMsg.UseInternalBuffer();
		sendMsg.Write((char)MessageType_UnreliablePong);
		sendMsg.Write(clientLocalTime);
		sendMsg.Write(currTime);
		speed = 0;
		if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket != NULL )
		{
			CriticalSectionLock lock(rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetSendQueueCriticalSection(), true);
			speed = rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetRecentReceiveSpeed(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
		}
		GetUnreliableMessagingLossRatioPercent(rc->m_HostID, &CSLossPercent);
		sendMsg.WriteScalar(speed);
		sendMsg.Write(CSLossPercent);

		rc->m_ToClientUdp_Fallbackable.AddToSendQueueWithSplitterAndSignal_Copy(rc->GetHostID(), CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_SpeedHackDetectorPing(CMessage &msg, CRemoteClient_S *rc)
	{
		rc->AssertIsSocketNotLockedByCurrentThread();

		// 		if (!isReadlUdp)
		// 			rc->m_tcpLayer->AssertIsZeroUseCount();
		// 		else
		// 			rc->m_ownedUdpSocket->AssertIsZeroUseCount();

		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		// 스피드핵 체크를 한다.
		rc->DetectSpeedHack();
	}

	struct UnreliableDestInfo
	{
		// 송신에 사용할 socket
		CSuperSocket* m_socket;
		bool useUdpSocket;
		HostID sendTo;
		AddrPort destAddr;
	};

	void CNetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc)
	{
		MessagePriority priority;
		int64_t uniqueID;
		if ( !msg.Read(priority) ||
			!msg.ReadScalar(uniqueID) )
			return;

		POOLED_ARRAY_LOCAL_VAR(HostIDArray, relayDest);
		
		int payloadLength;
		if ( !msg.Read(relayDest) )
			return;
		if ( !msg.ReadScalar(payloadLength) )
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if ( !msg.ReadWithShareBuffer(payload, payloadLength) )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		AssertIsLockedByCurrentThread();
		//CriticalSectionLock mainLock(GetCriticalSection(), true);
		IoCompletion_MulticastUnreliableRelay2_AndUnlock(mainLocker, relayDest, rc->m_HostID, payload, priority, uniqueID);
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc)
	{
		MessagePriority priority;
		int64_t uniqueID;
		if ( !msg.Read(priority) ||
			!msg.ReadScalar(uniqueID) )
			return;

		// p2p group 어디에도 비종속 클라들
		POOLED_ARRAY_LOCAL_VAR(HostIDArray, includeeHostIDList);
		

		//2003에서 에러 나므로 선언을 클래스 헤더로 옮깁니다.
		/*struct P2PGroupSubset_S
		{
		HostID m_groupID;
		HostIDArray m_excludeeHostIDList;
		};*/

		// TODO: 이것 관련 코드가 느릴 수 있다. 잦은 할당 때문. 언젠가 최적화를 해야.
		CFastArray<P2PGroupSubset_S> p2pGroupSubsetList;

		int p2pGroupSubsetListCount;
		int payloadLength;

		if ( !msg.Read(includeeHostIDList) )
			return;

		if ( !msg.ReadScalar(p2pGroupSubsetListCount) )
			return;

		p2pGroupSubsetList.SetCount(p2pGroupSubsetListCount);

		for ( int i = 0; i < p2pGroupSubsetListCount; i++ )
		{
			P2PGroupSubset_S &p2pDest = p2pGroupSubsetList[i];

			if ( !msg.Read(p2pDest.m_groupID) )
				return;
			if ( !msg.Read(p2pDest.m_excludeeHostIDList) )
				return;
		}

		if ( !msg.ReadScalar(payloadLength) )
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if ( !msg.ReadWithShareBuffer(payload, payloadLength) )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// relay할 호스트들을 뽑아낸다.
		POOLED_ARRAY_LOCAL_VAR(HostIDArray, uncompressedRelayDest);
		
		uncompressedRelayDest.SetMinCapacity(10000);

		includeeHostIDList.CopyTo(uncompressedRelayDest);	// individual로 들어온것들은 그대로 copy

		// rc를 얻어내야하기 때문에 main lock이 필요하다.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock mainlock(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		for ( int i = 0; i < p2pGroupSubsetList.GetCount(); i++ )
		{
			P2PGroupSubset_S &p2pDest = p2pGroupSubsetList[i];

			CP2PGroup_S *g = GetP2PGroupByHostID_NOLOCK(p2pDest.m_groupID);

			if ( g != NULL )
			{
				// NOTE: m_excludeeHostIDList와 g->m_members는 정렬되어 있다.
				HostIDArray::iterator itA = p2pDest.m_excludeeHostIDList.begin();		// directp2p로 이미 보낸 리스트
				P2PGroupMembers_S::iterator itB = g->m_members.begin();	// p2p 전체 인원 리스트

				// merge sort와 비슷한 방법으로, group member들로부터 excludee를 제외한 것들을 찾는다.
				for ( ; itA != p2pDest.m_excludeeHostIDList.end() || itB != g->m_members.end(); )
				{
					if ( itA == p2pDest.m_excludeeHostIDList.end() )
					{
						uncompressedRelayDest.Add(itB->first);
						itB++;
						continue;
					}
					if ( itB == g->m_members.end() )
						break;

					if ( *itA > itB->first ) // 릴레이 인원에 추가시킴
					{
						uncompressedRelayDest.Add(itB->first);
						itB++;
					}
					else if ( *itA < itB->first )
					{
						// 그냥 넘어감
						itA++;
					}
					else
					{
						// 양쪽 다 같음, 즉 부전승. 둘다 그냥 넘어감.
						itA++; itB++;
					}
				}
			}
		}

		IoCompletion_MulticastUnreliableRelay2_AndUnlock(mainLocker, uncompressedRelayDest, rc->m_HostID, payload, priority, uniqueID);
	}

	struct ReliableDestInfo
	{
		int frameNumber;
		CRemoteClient_S* sendToRC;
		HostID sendTo;
	};

	struct ReliableRelay1_Functor
	{
		// captured values
		HostID m_remote;
		int m_payloadLength;
		CMessage &m_payload;
		bool m_simplePacketMode;

		inline ReliableRelay1_Functor(HostID remote, int payloadLength, CMessage& payload, bool simplePacketMode)
			:m_payload(payload)
		{
			m_remote = remote;
			m_payloadLength = payloadLength;
			m_simplePacketMode = simplePacketMode;
		}

		inline CriticalSection* GetCriticalSection(ReliableDestInfo& dest)
		{
			return &(dest.sendToRC->m_tcpLayer->GetSendQueueCriticalSection());
		}

		// send queue lock은 contention이 적지만 그래도 없지는 않다.
		// relay1은 서버에서 가장 많이 사용되는 것 중 하나다.
		// 따라서 성능을 최대한 올리기 위해 이렇게 굳이 low context switch loop를 돌도록 한다.
		inline bool DoElementAndUnlock(ReliableDestInfo& dest, CriticalSectionLock& solock)
		{
			CSmallStackAllocMessage header;
			header.Write((char)MessageType_ReliableRelay2);
			header.Write(m_remote);				// sender
			header.Write(dest.frameNumber);	// frame number
			CMessageTestSplitterTemporaryDisabler dd(header);
			header.WriteScalar(m_payloadLength);		// payload length

			CSendFragRefs fragList;
			fragList.Add(header);
			fragList.Add(m_payload);

			// tcp send
			dest.sendToRC->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(fragList, SendOpt(), m_simplePacketMode);
			solock.Unlock();
			dest.sendToRC->m_tcpLayer->DecreaseUseCount();

			return true;
		}
	};

	void CNetServerImpl::IoCompletion_ProcessMessage_ReliableRelay1(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc)
	{
		RelayDestList relayDest;
		int payloadLength;
		if ( !msg.Read(relayDest) ||		// relay list(각 수신자별 프레임 번호 포함)
			!msg.ReadScalar(payloadLength) )	// 크기
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength )
		{
			ASSERT_OR_HACKED(0);   // 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if ( !msg.ReadWithShareBuffer(payload, payloadLength) )
		{
			ASSERT_OR_HACKED(0);		// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		intptr_t relayCount = relayDest.GetCount();
		if ( relayCount <= 0 )
			return;

		// 각 수신자마다 받아야 하는 데이터 내용이 다르다. 따라서 이렇게 구조체를 만든다.
		POOLED_ARRAY_LOCAL_VAR(CFastArray<ReliableDestInfo>,destinfolist);

		// rc를 얻어내야하기 때문에 main lock이 필요하다.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock mainlock(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		HostID remote = rc->GetHostID();


		// 각 수신 대상에게 브로드캐스트한다.
		for ( int i = 0; i < relayCount; i++ )
		{
			const RelayDest& dest = relayDest[i];

			ReliableDestInfo destinfo;
			destinfo.sendToRC = GetAuthedClientByHostID_NOLOCK(dest.m_sendTo);

			if ( destinfo.sendToRC != NULL )
			{
				destinfo.sendToRC->m_tcpLayer->IncreaseUseCount();// 사용카운트를 올려 놓아야 안심.
				destinfo.frameNumber = dest.m_frameNumber;
				destinfo.sendTo = dest.m_sendTo;
				destinfolist.Add(destinfo);
			}
		}

		// main lock unlock
		mainLocker->Unlock();
		// 어차피 send queue lock을 거는 것이라 contention이 짧긴 하지만 성능에 민감하고 
		// 기존 코드가 이렇게 짜져 있으므로.
		ReliableRelay1_Functor functor(remote, payloadLength, payload, m_simplePacketMode);
		LowContextSwitchingLoop(destinfolist.GetData(), destinfolist.GetCount(), functor);
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_LingerDataFrame1(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc)
	{
		HostID destRemoteHostID;
		int frameNumber;
		int frameLength;

		if ( !msg.Read(destRemoteHostID) ||
			!msg.Read(frameNumber) ||
			!msg.ReadScalar(frameLength) )
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( frameLength < 0 || frameLength >= m_settings.m_serverMessageMaxLength )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + frameLength == msg.GetLength());
		CMessage frameData;
		if ( !msg.ReadWithShareBuffer(frameData, frameLength) )
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		rc->AssertIsSocketNotLockedByCurrentThread();
		// rc를 얻어내야 하기때문에 mainlock이 필요하다.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock mainlock(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		HostID remote = rc->GetHostID();
		CRemoteClient_S* destRC = GetAuthedClientByHostID_NOLOCK(destRemoteHostID);
		if ( destRC == NULL )
		{
			return;
		}

		// main lock unlock
		// 하단의 AddToSendQueueWithSplitterAndSignal_Copy를 사용하는 tcpLayer의 객체가 파괴되지 않음을 보장해주어야해서 mainLock을 Unlock하지 않도록 변경
		//mainLocker->Unlock();

		CSmallStackAllocMessage header;
		header.Write((char)MessageType_LingerDataFrame2);
		header.Write(remote);
		header.Write(frameNumber);
		CMessageTestSplitterTemporaryDisabler dd(header);
		header.WriteScalar(frameLength);

		CSendFragRefs sendData;
		sendData.Add(header);
		sendData.Add(frameData);

		// tcp로 send
		//CriticalSectionLock lock(destRC->m_tcpLayer->GetSendQueueCriticalSection(),true);<= 내부에서 이미 잠그므로 불필요
		destRC->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(sendData, SendOpt(), m_simplePacketMode);
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_NotifyHolepunchSuccess(CMessage &msg, CRemoteClient_S *rc)
	{
		Guid magicNumber;
		AddrPort clientLocalAddr, clientAddrFromHere;
		if ( msg.Read(magicNumber) == false ||
			msg.Read(clientLocalAddr) == false ||
			msg.Read(clientAddrFromHere) == false )
			return;

#ifdef _DEBUG
		NTTNTRACE("clientlocalAddr:%s\n", StringT2A(clientLocalAddr.ToString()).GetString());
#endif

		rc->AssertIsSocketNotLockedByCurrentThread();

		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		// find relevant mature or unmature client by magic number
		if ( rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber == magicNumber && rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled == false )
		{
			// associate remote UDP address with matured or unmatured client
			rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled = true;
			//rc->m_safeTimes.Set_lastUdpPacketReceivedTime(GetCachedServerTimeMs());  // 따끈따끈하게 세팅하자.
			rc->m_safeTimes.Set_lastUdpPingReceivedTime(GetCachedServerTimeMs());  // 따끈따끈하게 세팅하자.

			rc->m_ToClientUdp_Fallbackable.SetUdpAddrFromHere(clientAddrFromHere);
			rc->m_ToClientUdp_Fallbackable.m_UdpAddrInternal = clientLocalAddr;

			P2PGroup_RefreshMostSuperPeerSuitableClientID(rc);

			// asend UDP matched via TCP
			CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
			sendMsg.Write((char)MessageType_NotifyClientServerUdpMatched);
			sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);

			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);

			// 로그를 남긴다
			if ( m_logWriter )
			{
				String txt;
				txt.Format(_PNT("Client %d Succeeded UDP Hole-Punching with the Server : Client local Addr=%s,Client addr at server=%s"),
						   rc->m_HostID, clientLocalAddr.ToString().GetString(), clientAddrFromHere.ToString().GetString());
				m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
			}
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(CMessage &msg, CRemoteClient_S *rc)
	{
		AddrPort clientLocalAddr, clientAddrFromHere;
		HostID remotePeerID;
		if ( msg.Read(clientLocalAddr) == false ||
			msg.Read(clientAddrFromHere) == false ||
			msg.Read(remotePeerID) == false )
		{
			return;
		}

#ifdef _DEBUG
		NTTNTRACE("%s\n", StringT2A(String::NewFormat(_PNT("P2P UDP hole punch success. %d(%s,%s)"),
			remotePeerID,
			clientLocalAddr.ToString().GetString(),
			clientAddrFromHere.ToString().GetString())).GetString());
#endif
		rc->AssertIsSocketNotLockedByCurrentThread();

		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* rc2 = GetAuthedClientByHostID_NOLOCK(remotePeerID);
		if ( !rc2 )
			return;

		// 쌍방이 모두 홀펀칭 OK가 와야만 쌍방에게 P2P 시도를 지시.
		P2PConnectionState* p2pConnState = m_p2pConnectionPairList.GetPair(remotePeerID, rc->m_HostID);

		//modify by rekfkno1 - //늦장 도착일 경우가 있다...가령 예를 들면,...
		//s->c로 leave,join을 연달아 보내는 경우... 클라에서는 미처 leave,join을 처리 하지 못한 상태에서 
		//보낸 메시지 일수 있다는 이야기.
		if ( !p2pConnState || p2pConnState->m_memberJoinAndAckInProgress )
			return;

		if ( !p2pConnState->IsRelayed() )
			return;

		int holepunchOkCountOld = p2pConnState->GetServerHolepunchOkCount();

		p2pConnState->SetServerHolepunchOk(rc->m_HostID, clientLocalAddr, clientAddrFromHere);
		if ( p2pConnState->GetServerHolepunchOkCount() != 2 || holepunchOkCountOld != 1 )
		{
			//NTTNTRACE("#### %d %d ####\n",p2pConnState->GetServerHolepunchOkCount(),holepunchOkCountOld);
			return;
		}

		assert(p2pConnState->m_magicNumber != Guid());
		// send to rc
		m_s2cProxy.RequestP2PHolepunch(rc->m_HostID, g_ReliableSendForPN,
									   rc2->m_HostID,
									   p2pConnState->GetInternalAddr(rc2->m_HostID),
									   p2pConnState->GetExternalAddr(rc2->m_HostID));

		// send to rc2
		m_s2cProxy.RequestP2PHolepunch(rc2->m_HostID, g_ReliableSendForPN,
									   rc->m_HostID,
									   p2pConnState->GetInternalAddr(rc->m_HostID),
									   p2pConnState->GetExternalAddr(rc->m_HostID));

		// 로그를 남긴다
		if ( m_logWriter )
		{
			String txt;
			txt.Format(_PNT("Stage 1 for Pair Hole-Punching between the Client %d and Client %d Succeeded: Client %d External Addr=%s,Client %d External Addr=%s"),
					   rc->m_HostID,
					   rc2->m_HostID,
					   rc->m_HostID,
					   p2pConnState->GetExternalAddr(rc->m_HostID).ToString().GetString(),
					   rc2->m_HostID,
					   p2pConnState->GetExternalAddr(rc2->m_HostID).ToString().GetString());

			m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_Rmi(CReceivedMessage &receivedInfo, bool messageProcessed, CRemoteClient_S * rc, bool isRealUdp)
	{
		// ProudNet layer의 RMI이면 아래 구문에서 true가 리턴되고 결국 user thread로 넘어가지 않는다.
		// 따라서 아래처럼 하면 된다.
		{
			// 이 값은 MessageType_Rmi 다음의 offset이다.
			CMessage& payload = receivedInfo.GetReadOnlyMessage();
			int rmiReadOffset = payload.GetReadOffset();

			//AssertIsNotLockedByCurrentThread();
			rc->AssertIsSocketNotLockedByCurrentThread();

			AssertIsLockedByCurrentThread();
			// 			CriticalSectionLock clk(GetCriticalSection(), true);
			// 			CHECK_CRITICALSECTION_DEADLOCK(this);

			messageProcessed |= m_c2sStub.ProcessReceivedMessage(receivedInfo, rc->m_hostTag);
			if ( !messageProcessed )
			{
				// 유저 스레드에서 RMI를 처리하도록 enque한다.
				payload.SetReadOffset(rmiReadOffset);

				CReceivedMessage receivedInfo2;
				receivedInfo2.m_unsafeMessage.UseInternalBuffer();
				receivedInfo2.m_unsafeMessage.AppendByteArray(
					payload.GetData() + payload.GetReadOffset(),
					payload.GetLength() - payload.GetReadOffset());
				receivedInfo2.m_relayed = receivedInfo.m_relayed;
				receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
				receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
				receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
				receivedInfo2.m_compressMode = receivedInfo.m_compressMode;

				if ( rc != NULL && isRealUdp )
				{
					rc->m_toServerSendUdpMessageSuccessCount++;
				}




/*				if (rc == NULL && isRealUdp)
				{
					// RMI 3003?
					assert(0);
				}*/




				UserTaskQueue_Add(rc, receivedInfo2, UWI_RMI);
			}
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_UserOrHlaMessage(CReceivedMessage &receivedInfo, FinalUserWorkItemType UWIType, bool messageProcessed, CRemoteClient_S * rc, bool isRealUdp)
	{
		//AssertIsNotLockedByCurrentThread();
		rc->AssertIsSocketNotLockedByCurrentThread();

		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		// 이거 치우거나 mainlock아래로 내리지 말것...no lock CReceivedMessage파괴되면서 refcount꼬임.
		{
			CMessage& payload = receivedInfo.GetReadOnlyMessage();

			// 유저 스레드에서 사용자 정의 메시지를 처리하도록 enque한다.
			CReceivedMessage receivedInfo2;
			receivedInfo2.m_unsafeMessage.UseInternalBuffer();
			receivedInfo2.m_unsafeMessage.AppendByteArray(
				payload.GetData() + payload.GetReadOffset(),
				payload.GetLength() - payload.GetReadOffset());
			receivedInfo2.m_relayed = receivedInfo.m_relayed;
			receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
			receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
			receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
			receivedInfo2.m_compressMode = receivedInfo.m_compressMode;

			if ( rc != NULL && isRealUdp )
			{
				rc->m_toServerSendUdpMessageSuccessCount++;
			}

			UserTaskQueue_Add(rc, receivedInfo2, UWIType);
		}
	}

	void CNetServerImpl::NotifyProtocolVersionMismatch(CRemoteClient_S * rc)
	{
		AssertIsLockedByCurrentThread();

		CMessage sendMsg;
		sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_NotifyProtocolVersionMismatch);

		rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
	}

	bool CNetServerImpl::ProcessMessage_FromUnknownClient(AddrPort from, CMessage& recvedMsg, CSuperSocket *udpSocket)
	{
		AssertIsLockedByCurrentThread();

		int orgReadOffset = recvedMsg.GetReadOffset();
		MessageType type;
		if ( recvedMsg.Read(type) == false )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		bool messageProcessed = false;
		switch ( (int)type )
		{
			case MessageType_ServerHolepunch:
				IoCompletion_ProcessMessage_ServerHolepunch(recvedMsg, from, udpSocket);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_ServerHolepunch:
				IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(
					recvedMsg,
					from,
					udpSocket);
				messageProcessed = true;
				break;
		}

		if ( !messageProcessed )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		return true;
	}

	class SendToUnreliableDestInfo
	{
		CSendFragRefs &m_fragList;
		SendOpt &m_sendOpt;
		int64_t m_currTime;

	public:
		SendToUnreliableDestInfo(CSendFragRefs &fragList, SendOpt &sendOpt) :m_fragList(fragList), m_sendOpt(sendOpt), m_currTime(GetPreciseCurrentTimeMs()) {}
		CriticalSection* GetCriticalSection(UnreliableDestInfo& dest)
		{
			return &(dest.m_socket->GetSendQueueCriticalSection());
		}

		// send queue lock은 contention이 적지만 그래도 없지는 않다.
		// relay1은 서버에서 가장 많이 사용되는 것 중 하나다.
		// 따라서 성능을 최대한 올리기 위해 이렇게 굳이 low context switch loop를 돌도록 한다.
		bool DoElementAndUnlock(UnreliableDestInfo& dest, CriticalSectionLock& sendQueueLock)
		{
			if ( dest.useUdpSocket )
			{
				dest.m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
					dest.sendTo,
					FilterTag::CreateFilterTag(HostID_Server, dest.sendTo),
					dest.destAddr,
					m_fragList,
					m_currTime, //성능에 민감합니다. 미리 SendToUnreliableDestInfo의 멤버 변수로 시간을 넣은 후 여기서 그것을 사용하게 하세요.
					m_sendOpt);
			}
			else
			{
				dest.m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
					m_fragList,
					m_sendOpt);
			}
			sendQueueLock.Unlock();
			dest.m_socket->DecreaseUseCount();
			
			return true;
		}
	};

	// caller는 이미 main 잠금을 only 1 recursion 상태로 이 함수를 콜 한 상태이어야 한다.
	void CNetServerImpl::IoCompletion_MulticastUnreliableRelay2_AndUnlock(CriticalSectionLock* mainLocker, const HostIDArray &relayDest, HostID relaySenderHostID, CMessage &payload, MessagePriority priority, int64_t uniqueID)
	{
		intptr_t relayCount = relayDest.GetCount();
		if ( relayCount <= 0 )
			return;

		int payloadLength = payload.GetLength();

		POOLED_ARRAY_LOCAL_VAR(CFastArray<UnreliableDestInfo>, relayDestlist);

		/* rc를 얻어내야하기 때문에 main lock이 필요하다.
		단, 콜러에서 이미 잠금을 한 상태이면(당연히 recursion count = 1이어야) 굳이 또 잠그지 않는다.
		이 함수의 하단에서 멀티코어를 사용하기 위해 잠금을 풀어야 하니까.
		*/
		assert(mainLocker->IsLocked());
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// 각 dest들을 리스트에 추가한다.
		for ( int i = 0; i < relayCount; i++ )
		{
			HostID dest = relayDest[i];

			UnreliableDestInfo destinfo;
			CRemoteClient_S* destRC = GetAuthedClientByHostID_NOLOCK(dest);

			destinfo.sendTo = dest;

			if ( NULL != destRC )
			{
				RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(destRC);

				if ( destRC->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
				{
					// UDP인 경우 이것을 inc해야
					destRC->m_ToClientUdp_Fallbackable.m_udpSocket->IncreaseUseCount();
					destinfo.m_socket = destRC->m_ToClientUdp_Fallbackable.m_udpSocket;
					destinfo.destAddr = destRC->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
					destinfo.useUdpSocket = true;
				}
				else
				{
					// TCP인 경우 remote 자체를 inc해야. 
					// NOTE: super socket으로 대체하는 버전에서는 remote가 use count를 더 이상 가지지 않으므로 
					// 그것이 가지는 TCP socket을 대상으로 해야 한다.
					destRC->m_tcpLayer->IncreaseUseCount();
					destinfo.m_socket = destRC->m_tcpLayer;
					destinfo.useUdpSocket = false;
				}

				relayDestlist.Add(destinfo);
			}
		}

		// 잠금을 푼다.
		mainLocker->Unlock();
		// 이 스레드는 잠그고 있지 않음이 보장되어야 아래 멀티코어 멀티캐스트가 효력을 낸다.
		AssertIsNotLockedByCurrentThread();

		// packet setting
		CSmallStackAllocMessage header;
		header.Write((char)MessageType_UnreliableRelay2);
		header.Write(relaySenderHostID);		// sender
		// (unreliable은 frame number가 불필요)
		CMessageTestSplitterTemporaryDisabler dd(header);
		header.WriteScalar(payloadLength);	// payload length

		CSendFragRefs fragList;
		fragList.Add(header);
		fragList.Add(payload);

		SendOpt sendOpt(g_UnreliableSendForPN);
		sendOpt.m_priority = priority;
		sendOpt.m_uniqueID.m_value = uniqueID;

		// NOTE: relay1 메시지 자체가 MTU를 넘는 경우가 있을 수 있다. 따라서 relay2에서도 MTU 단위로 쪼개는 것이 필요할 수 있다. false로 세팅하지 말자.
		//sendOpt.m_INTERNAL_USE_fraggingOnNeed = false; 

		SendToUnreliableDestInfo functor(fragList, sendOpt);
		LowContextSwitchingLoop(relayDestlist.GetData(), relayDestlist.GetCount(), functor);
	}

	void CNetServerImpl::ShowThreadExceptionAndPurgeClient(CRemoteClient_S* client, const PNTCHAR* where, const PNTCHAR* reason)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if ( client )
		{
			if ( m_eventSink_NOCSLOCK )
			{
				String txt;
				txt.Format(_PNT("Expelling the Client %d from %s due to %s."), client->m_HostID, where, reason);
				EnqueError(ErrorInfo::From(ErrorType_ExceptionFromUserFunction, client->m_HostID, txt));
			}
			GarbageHost(client, ErrorType_ExceptionFromUserFunction, ErrorType_TCPConnectFailure, ByteArray(), _PNT("STEXAX"), SocketErrorCode_Ok);
		}
		else
		{
			if ( m_eventSink_NOCSLOCK )
			{
				String txt;
				txt.Format(_PNT("%s Error Occurred from %s but Clients Could Not Be Discerned."), reason, where);
				EnqueError(ErrorInfo::From(ErrorType_ExceptionFromUserFunction, HostID_None, txt));
			}
		}
	}

	// 이 객체는 IThreadReferrer를 상속받는다.
	// 이 객체에 대해 CThreadPoolImpl.RegisterReferrer를 호출한 전제하에
	// 이 객체에 대해 post custom event를 한 경우 이것이 호출된다.
	void CNetServerImpl::OnCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue)
	{
		AssertIsNotLockedByCurrentThread();

		switch ((int)customValue)
		{

			/*이제는 listner thread와 networker thread가 동일합니다.
				따라서 CustomValueEvent_NewClient를 제거해 버립시다.
				그리고 CustomValueEvent_NewClient를 post하는 곳에서 직접 CustomValueEvent_NewClient를
				받아 처리하는 루틴이 직접 실행되게 수정합시다.

				LS에서는 ProcessNewClients에 대응하는 것이 CLanServerImpl::IoCompletion_NewClientCase 이므로 그걸 손대시길.
				*/
			// 		case CustomValueEvent_NewClient:      // 처음 시작 조건
			// 			// 서버 종료 조건이 아니면 클라를 IOCP에 엮고 다음 송신 절차를 시작한다.
			// 			if (m_netThreadPool != NULL)
			// 			{
			// 				ProcessNewClients(); 
			// 			}
			// 			break;

			case CustomValueEvent_Heartbeat:
				/*Heartbeat은 NS,NC,LS,LC 공통으로 필요로 합니다.
					Heartbeat은 OnTick과 달리 PN 내부로직만을 처리합니다.
					속도에 민감하므로 절대 블러킹 즉 CPU idle time이 없어야 하고요.

					따라서 아래 Heartbeat 함수 중에서 ProcessCustomValueEvent에 옮겨야 할 루틴은 옮기시고,
					나머지는 CNetServerImpl.Heartbeat에 그대로 남깁시다.
					*/
				Heartbeat();
				break;

				// 		case CustomValueEvent_OnTick:
				// 			/*OnTick을 호출하는 로직도 NetCore의 공통분모로 옮겨버립시다.
				// 			   즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
				// 			   함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
				// 			   모두 NetCore로 옮겨야 합니다.
				// 			   */
				// 
				// 			// 여기 왔을때, 이 스레드는 user worker thread이다.
				// 			OnTick();
				// 			break;
				// 		case CustomValueEvent_DoUserTask:
				// 			/*DoUserTask을 호출하는 로직도 NetCore의 공통분모로 옮겨버립시다.
				// 				즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
				// 				함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
				// 				모두 NetCore로 옮겨야 합니다.
				// 				*/
				// 
				// 			// 여기 왔을때, 이 스레드는 user worker thread이다.
				// 			DoUserTask();
				// 			break;
#if defined (_WIN32)
			case CustomValueEvent_IssueFirstRecv:
				// Start or Connect를 호출한 스레드에서 직접 socket을 생성할 때에 대한 처리입니다.
				// NS,NC,LS,LC의 중복코드 방지를 위해 
				// NetCore.m_issueFirstRecvNeededSockets라는 큐를 만들어 거기 넣기만 하면 알아서 first recv가 
				// 걸리게 만드는 것이 좋아 보입니다. 수정합시다.
				TcpListenSockets_IssueFirstAccept();
				StaticAssignedUdpSockets_IssueFirstRecv();
				return;

#endif // _WIN32
				// 		case CustomValueEvent_End:
				// 			/* NetCore의 공통분모로 옮겨버립시다.
				// 				즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
				// 				함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
				// 				모두 NetCore로 옮겨야 합니다.
				// 				*/
				// 			EndCompletion();
				// 			break;
				// 		default:
				// 			assert(0);
				// 			break;
		}

		// NetCore도 custom value event를 처리해야 한다.
		CNetCoreImpl::ProcessCustomValueEvent(workResult, customValue);
	}

	void CNetServerImpl::Heartbeat()
	{
#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
		try
		{
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION

			//한번에 한스레드에서 작업함을 개런티하기 위함.
			if ( 0 == AtomicCompareAndSwap32(0, 1, &m_heartbeatWorking) )
			{
				//m_cachedTimes.Refresh(m_timer);
				Heartbeat_One();

#ifndef PROUDSRCCOPY
				((CPNLic*)m_lic)->POSH(GetPreciseCurrentTimeMs());
#endif

				AtomicCompareAndSwap32(1, 0, &m_heartbeatWorking);
			}

#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
		}
		catch ( std::bad_alloc &ex )
		{
			BadAllocException(ex);
		}
		catch ( std::exception &e )
		{
			String txt;
			txt.Format(_PNT("std.exception(%s)"), StringA2T(e.what()));
			ShowThreadUnexpectedExit(_PNT("HB1"), txt);
		}
		catch ( _com_error &e )
		{
			String txt;
			txt.Format(_PNT("_com_error(%s)"), (const PNTCHAR*)e.Description());
			ShowThreadUnexpectedExit(_PNT("HB2"), txt);
		}
		catch ( void* )
		{
			ShowThreadUnexpectedExit(_PNT("HB3"), _PNT("void*"), true);
		}
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
	}

	void CNetServerImpl::Heartbeat_One()
	{
		int64_t currentTimeMs = GetPreciseCurrentTimeMs();

		if ( m_PurgeTooOldUnmatureClient_Timer.IsTimeToDo(currentTimeMs) == true )
			PurgeTooOldUnmatureClient();

		if ( m_PurgeTooOldAddMemberAckItem_Timer.IsTimeToDo(currentTimeMs) == true )
			PurgeTooOldAddMemberAckItem();

		// 통신량에 의해 적합 수퍼피어가 달라질 수 있기 때문에
		if ( m_electSuperPeer_Timer.IsTimeToDo(currentTimeMs) == true )
			ElectSuperPeer();

		if ( m_removeTooOldUdpSendPacketQueue_Timer.IsTimeToDo(currentTimeMs) == true )
		{
			DoForLongInterval();

			CSingleton<CFavoritePooledObjects>::Instance().ShrinkOnNeed();
		}

		if ( m_DisconnectRemoteClientOnTimeout_Timer.IsTimeToDo(currentTimeMs) == true )
		{
			Heartbeat_EveryRemoteClient();
		}

		if ( m_removeTooOldRecyclePair_Timer.IsTimeToDo(currentTimeMs) == true )
		{
			CriticalSectionLock mainlock(GetCriticalSection(), true);
			CHECK_CRITICALSECTION_DEADLOCK(this);
			m_p2pConnectionPairList.RecyclePair_RemoveTooOld(GetCachedServerTimeMs());
		}
		//DoGarbageCollect(); OnCustomValueEvent에서 처리하므로 여기는 불필요

		if ( m_GarbageTooOldRecyclableUdpSockets_Timer.IsTimeToDo(currentTimeMs) == true )
		{
			GarbageTooOldRecyclableUdpSockets();
		}

		LogFreqFailOnNeed();

	}

	//이 함수를 CNetCoreImpl로 옮기자.
	// 	void CNetServerImpl::EveryRemote_IssueSendOnNeed(CNetworkerLocalVars& localVars)
	// 	{
	// 		//한 스레드만 작업하는것을 개런티
	// 		if (0 == InterlockedCompareExchange(&m_issueSendOnNeedWorking, 1, 0))
	// 		{
	// 			int64_t currTime = GetCachedServerTimeMs();
	// 
	// 			// main lock 한 상태에서 issue send를 할 것들을 추린다. 추려진 것은 usecount+1이 되므로 비파괴 보장된다.
	// 			{
	// 				AssertIsNotLockedByCurrentThread();
	// 				CriticalSectionLock lock(m_cs, true); // 이 잠금 범위는 빠르게 작동한다. system call이 없기 때문이다.
	// 
	// 				localVars.sendIssuedPool.SetCapacity(m_sendReadyList->GetTotalSocketCount()); // time-to-issue가 도달하지 않은 것들이라도 모두 그냥 얻어버리자. 얻는 함수 자체가 이게 더 빠르기 때문이다.
	// 				localVars.sendIssuedPool.SetCount(0);
	// 
	// 				while (true)
	// 				{
	// 					CSuperSocket* sk;
	// 					if (m_sendReadyList->PopKey(&sk))
	// 					{
	// 						// 얻은 key는 ptr이지만 자체 잠금이므로 main lock 상태에서의 유효성 검사를 해야 한다.
	// 						if (m_validSendReadyCandidates.ContainsKey(sk))
	// 						{
	// 							sk->IncreaseUseCount(); // 아래 LowContextSwitchingLoop에서 dec할 것이다.
	// 							localVars.sendIssuedPool.Add(sk);
	// 						}
	// 						// 아직 얻을 키가 더 있을 수 있으므로 중단하지 말아야.
	// 					}
	// 					else
	// 					{
	// 						// 더 이상 얻을 키가 없다. 그냥 중단.
	// 						break;
	// 					}
	// 				}
	// 			}
	// 
	// 			// 이 안에서는 오랜 시간 per-remote lock을 수행한다. 따라서 unlock main 후 수행한다
	// 			// 그래도 안전하다. 왜냐하면 이미 use count+1씩 해놨으니까.
	// 			// 이 함수 안에서 각각에 대해 issue-send를 한 후에 use count-1을 할 것이다.
	// 			LowContextSwitchingLoop(localVars.sendIssuedPool.GetData(), localVars.sendIssuedPool.GetCount(), IssueSendFunctor<CNetServerImpl>(this, currTime));
	// 
	// 			InterlockedExchange(&m_issueSendOnNeedWorking, 0);
	// 		}
	// 	}
	// 
	// 	void CNetServerImpl::OnTick()
	// 	{
	// 		if (m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 		{
	// 			int result = AtomicIncrement32(&m_timerCallbackParallelCount);
	// 			if (result <= m_timerCallbackParallelMaxCount) // 동시에 실행 가능한 timer callback 수를 제한함
	// 			{
	// 				m_eventSink_NOCSLOCK->OnTick(m_timerCallbackContext);
	// 			}
	// 			AtomicDecrement32(&m_timerCallbackParallelCount);
	// 		}
	// 	}
	// 
	// 	void CNetServerImpl::DoUserTask()
	// 	{
	// 		CUserWorkerThreadCallbackContext ctx;
	// 		CFinalUserWorkItem Item;
	// 
	// 		bool rs = false;
	// 		do
	// 		{
	// 			void* hostTag = NULL;
	// 			{
	// 				CriticalSectionLock clk(GetCriticalSection(), true);
	// 				CHECK_CRITICALSECTION_DEADLOCK(this);
	// 
	// 				rs = m_userTaskQueue.PopAnyTaskNotRunningAndMarkAsRunning(Item, &hostTag);
	// 			}
	// 
	// 			if (rs)
	// 			{
	// 				if (m_eventSink_NOCSLOCK != NULL)
	// 					m_eventSink_NOCSLOCK->OnUserWorkerThreadCallbackBegin(&ctx);
	// 
	// 				switch (Item.m_type)
	// 				{
	// 				case UWI_RMI:
	// 					UserWork_FinalReceiveRmi(Item, hostTag);
	// 					break;
	// 				case UWI_UserMessage:
	// 					UserWork_FinalReceiveUserMessage(Item, hostTag);
	// 					break;
	// 				case UWI_Hla:
	// 					UserWork_FinalReceiveHla(Item, hostTag);
	// 					break;
	// 				default:
	// 					UserWork_LocalEvent(Item, hostTag);
	// 				}
	// 
	// 				if (m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 					m_eventSink_NOCSLOCK->OnUserWorkerThreadCallbackEnd(&ctx);
	// 
	// 			}
	// 
	// 		} while (rs);
	// 	}
	// 
	// 	void CNetServerImpl::UserWork_FinalReceiveRmi(CFinalUserWorkItem& UWI, void* hostTag)
	// 	{
	// 		AssertIsNotLockedByCurrentThread();
	// 
	// 		// 		if (m_owner->IsValidHostID(UWI.m_unsafeMessage.m_remoteHostID) == false) IsValidHostID_NOLOCK caller 참고
	// 		// 			return;
	// 
	// 		CMessage& msgContent = UWI.m_unsafeMessage.GetReadOnlyMessage();
	// 		int oldReadPointer = msgContent.GetReadOffset();
	// 
	// 		if (!(oldReadPointer == 0))
	// 			EnqueueHackSuspectEvent(NULL, __FUNCTION__, HackType_PacketRig);
	// 
	// 		bool processed = false;
	// 		RmiID rmiID = RmiID_None;
	// 
	// 		//CReaderLock_NORECURSE clk(m_owner->m_callbackMon, true);
	// 
	// 		if (msgContent.Read(rmiID))
	// 		{
	// 			// 각 stub에 대한 처리를 수행한다.
	// 			intptr_t stubCount = m_stubList_NOCSLOCK.GetCount();
	// 			for (intptr_t iStub = 0; iStub < stubCount; iStub++)
	// 			{
	// 				IRmiStub *stub = m_stubList_NOCSLOCK[iStub];
	// 
	// 				msgContent.SetReadOffset(oldReadPointer);
	// 				// 이렇게 해줘야 막판에 task running flag를 수정할 수 있으니까 try-catch 구문이 있는거다.
	// 				try
	// 				{
	// 					//if(!m_owner->m_shutdowning)
	// 					{
	// 						processed |= stub->ProcessReceivedMessage(UWI.m_unsafeMessage, hostTag);
	// 					}
	// 				}
	// 				catch (std::exception &e)
	// 				{
	// 					if (m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 					{
	// 						Exception ex(e);
	// 						ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 						m_eventSink_NOCSLOCK->OnException(ex);
	// 					}
	// 				}
	// 				catch (void* e)
	// 				{
	// 					if (m_eventSink_NOCSLOCK != NULL/*&& !m_owner->m_shutdowning*/)
	// 					{
	// 						Exception ex(e);
	// 						ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 						m_eventSink_NOCSLOCK->OnException(ex);
	// 					}
	// 				}
	// 				catch (_com_error &e)
	// 				{
	// 					if (m_eventSink_NOCSLOCK != NULL/* && !m_owner->m_shutdowning*/)
	// 					{
	// 						Exception ex(e);
	// 						ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 						m_eventSink_NOCSLOCK->OnException(ex);
	// 					}
	// 				}
	// #ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
	// 				catch (...)
	// 				{
	// 					if (m_owner->m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 					{
	// 						Exception ex;
	// 						ex.m_exceptionType = ExceptionType_Unhandled;
	// 						m_owner->m_eventSink_NOCSLOCK->OnException(ex);
	// 					}
	// 				}
	// #endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
	// 			}
	// 
	// 			if (!processed)
	// 			{
	// 				msgContent.SetReadOffset(oldReadPointer);
	// 				if (m_eventSink_NOCSLOCK /*&& !m_owner->m_shutdowning*/)
	// 					m_eventSink_NOCSLOCK->OnNoRmiProcessed(rmiID);
	// 			}
	// 
	// 			m_userTaskQueue.SetTaskRunningFlagByHostID(UWI.m_unsafeMessage.m_remoteHostID, false);
	// 		}
	// 	}
	// 
	// 	void CNetServerImpl::UserWork_FinalReceiveUserMessage(CFinalUserWorkItem& UWI, void* hostTag)
	// 	{
	// 		AssertIsNotLockedByCurrentThread();
	// 
	// 		CMessage& msgContent = UWI.m_unsafeMessage.GetReadOnlyMessage();
	// 		int oldReadPointer = msgContent.GetReadOffset();
	// 
	// 		if (!(oldReadPointer == 0))
	// 			EnqueueHackSuspectEvent(NULL, __FUNCTION__, HackType_PacketRig);
	// 
	// 		bool processed = false;
	// 		RmiID rmiID = RmiID_None;
	// 
	// 		//CReaderLock_NORECURSE clk(m_owner->m_callbackMon, true);
	// 
	// 		if (/*!m_owner->m_shutdowning &&*/ m_eventSink_NOCSLOCK)
	// 		{
	// 			RmiContext rmiContext;
	// 			rmiContext.m_sentFrom = UWI.m_unsafeMessage.m_remoteHostID;
	// 			rmiContext.m_relayed = UWI.m_unsafeMessage.m_relayed;
	// 			rmiContext.m_hostTag = hostTag;
	// 			rmiContext.m_encryptMode = UWI.m_unsafeMessage.m_encryptMode;
	// 			rmiContext.m_compressMode = UWI.m_unsafeMessage.m_compressMode;
	// 
	// 			try
	// 			{
	// 				int payloadLength = msgContent.GetLength() - msgContent.GetReadOffset();
	// 
	// 				m_eventSink_NOCSLOCK->OnReceiveUserMessage(UWI.m_unsafeMessage.m_remoteHostID, rmiContext, msgContent.GetData() + msgContent.GetReadOffset(), payloadLength);
	// 
	// 			}
	// 			catch (std::exception &e)
	// 			{
	// 				if (m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 				{
	// 					Exception ex(e);
	// 					ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 					m_eventSink_NOCSLOCK->OnException(ex);
	// 				}
	// 			}
	// 			catch (void* e)
	// 			{
	// 				if (m_eventSink_NOCSLOCK != NULL/*&& !m_owner->m_shutdowning*/)
	// 				{
	// 					Exception ex(e);
	// 					ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 					m_eventSink_NOCSLOCK->OnException(ex);
	// 				}
	// 			}
	// 			catch (_com_error &e)
	// 			{
	// 				if (m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 				{
	// 					Exception ex(e);
	// 					ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
	// 					m_eventSink_NOCSLOCK->OnException(ex);
	// 				}
	// 			}
	// #ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
	// 			catch (...)
	// 			{
	// 				if (m_owner->m_eventSink_NOCSLOCK != NULL /*&& !m_owner->m_shutdowning*/)
	// 				{
	// 					Exception ex;
	// 					ex.m_exceptionType = ExceptionType_Unhandled;
	// 					m_owner->m_eventSink_NOCSLOCK->OnException(ex);
	// 				}
	// 			}
	// #endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
	// 		}
	// 		m_userTaskQueue.SetTaskRunningFlagByHostID(UWI.m_unsafeMessage.m_remoteHostID, false);
	// 	}
	// 
	// 	void CNetServerImpl::UserWork_FinalReceiveHla(CFinalUserWorkItem& UWI, void* hostTag)
	// 	{
	// 		AssertIsNotLockedByCurrentThread();
	// 
	// 		CMessage& msgContent = UWI.m_unsafeMessage.GetReadOnlyMessage();
	// 		int oldReadPointer = msgContent.GetReadOffset();
	// 
	// 		if (!(oldReadPointer == 0))
	// 			EnqueueHackSuspectEvent(NULL, __FUNCTION__, HackType_PacketRig);
	// 
	// 		if (m_hlaSessionHostImpl != NULL)
	// 			m_hlaSessionHostImpl->ProcessMessage(UWI.m_unsafeMessage);
	// 
	// 		m_userTaskQueue.SetTaskRunningFlagByHostID(UWI.m_unsafeMessage.m_remoteHostID, false);
	// 	}

	// 	void CNetServerImpl::UserWork_LocalEvent(CFinalUserWorkItem &UWI, void* hostTag)
	// 	{
	// 		/* OnClientJoin에서 setHostTag를 하고, OnClientLeave에서 hostTag값을 사용하는 유저는 흔하다.
	// 		그런데, 클라가 서버와 접속을 하자 마자 끊는 경우를 고려해야 한다.
	// 
	// 		OnClientJoin에서 setHostTag를 호출했는데, net worker thread에서 접속 해제가 감지되어서 LocalEventType_ClientLeaveAfterDispose를 enque하는 경우가 있다.
	// 		이때 LocalEventType_ClientLeaveAfterDispose와 함께 들어간 hostTag 값은 setHostTag가 호출되기 전의 상태일 가능성이 있다.
	// 		이러한 경우 OnClientLeave에서 얻어진 hostTag은 setHostTag 호출하기 전의 값이 들어올 수 있다.
	// 		이렇게 되면, OnClientJoin(X)와 OnClientLeave(X)는 동시성이 없음을 보장해주는 안전함이 있더라도
	// 		OnClientJoin(X)때 호출한 setHostTag가 OnClientLeave(X)에서는 소용없을 수 있다. 이러면 안된다.
	// 
	// 		아래 patchwork는 그것을 해결하기 위한 방법이다.
	// 		*/
	// 		if (UWI.m_event.m_netClientInfo != NULL)
	// 			UWI.m_event.m_netClientInfo->m_hostTag = hostTag;
	// 
	// 		ProcessOneLocalEvent(UWI.m_event);
	// 		m_userTaskQueue.SetTaskRunningFlagByHostID(UWI.m_unsafeMessage.m_remoteHostID, false);
	// 	}
	// 

	void CNetServerImpl::OnThreadBegin()
	{
		if ( m_eventSink_NOCSLOCK != NULL )
			((INetServerEvent*)m_eventSink_NOCSLOCK)->OnUserWorkerThreadBegin();
	}

	void CNetServerImpl::OnThreadEnd()
	{
		if ( m_eventSink_NOCSLOCK != NULL )
			((INetServerEvent*)m_eventSink_NOCSLOCK)->OnUserWorkerThreadEnd();
	}

	// remote obj를 파괴하기 전에 콜 하는 정리 함수
	// C# 버전으로 간다면 이 함수가 왜 존재하는지 깨달을 것임
	void CNetServerImpl::Unregister(CRemoteClient_S * rc)
	{
		AssertIsLockedByCurrentThread();

		if ( rc->IsInUserTaskQueue() )
		{
			throw Exception("Cannot delete remote object whose task is still pending!");
		}

		//rc->m_finalUserWorkItemList.Clear();
		rc->ClearFinalUserWorkItemList();
	}

	ErrorType CNetServerImpl::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if ( outputPercent == NULL )
			return ErrorType_PermissionDenied;

		if ( remotePeerID == HostID_Server ) // 자기 자신이면 로스률은 없다.
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remotePeerID);
		if ( rc == NULL )
			return ErrorType_InvalidHostID;

		if ( !rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		AddrPort PeerAddr = rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
		if ( PeerAddr == AddrPort::Unassigned )	// 아직 홀펀칭이 안된 경우 로스률은 없다.
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		CSuperSocketPtr pUdpSocket = rc->m_ToClientUdp_Fallbackable.m_udpSocket;
		if ( pUdpSocket == NULL )
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(PeerAddr);
		*outputPercent = nPercent;
		return ErrorType_Ok;
	}

	// 	void CNetServerImpl::SetTimerCallbackIntervalMs(int newVal)
	// 	{
	// 		m_periodicPoster_Tick->SetPostInterval((newVal));
	// 	}

	// 패킷양이 많은지 판단
	// 	bool CNetServerImpl::CheckMessageOverloadAmount()
	// 	{
	// 		if (m_finalUserWorkItemList.GetCount() >= CNetConfig::MessageOverloadWarningLimit)
	// 		{
	// 			return true;
	// 		}
	// 		return false;
	// 	}

	ErrorType CNetServerImpl::SetCoalesceIntervalMs(HostID remote, int intervalMs)
	{
		if ( intervalMs < 0 || intervalMs > 1000 )
		{
			throw Exception("Set out of interval range! Can set interval at 0 ~ 1000.");
		}

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);

		if ( rc != NULL )
		{
			rc->m_autoCoalesceInterval = false;
			rc->SetManualOrAutoCoalesceInterval(intervalMs);

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetServerImpl::SetCoalesceIntervalToAuto(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		CRemoteClient_S *rc = GetAuthedClientByHostID_NOLOCK(remote);

		if ( rc )
		{
			rc->m_autoCoalesceInterval = true;
			rc->SetManualOrAutoCoalesceInterval();

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}
	// 
	// 	// 이 함수를 지우고 이 기능을 CNetCoreImpl.SendReadyList_Add로 옮기자.
	// 	void CNetServerImpl::SendReadyList_Add(CSuperSocket* socket)
	// 	{
	// 		CriticalSectionLock lock(m_cs, true);
	// 		// send ready list에도 추가한다. main lock 상태가 아닐 수 있음에 주의.
	// 		m_sendReadyList->AddOrSet(sender);
	// 		m_netThreadPool->PostCompletionStatus(this, CustomValueEvent_SendEnqueued);
	// 
	// 		// 그리고 tag도 지워버리고(NC에서는 remote peer의 host ID를 미리 알 수 있지만 서버는 안그럼) tag에는 remote object ptr을 가리키되
	// 		// map에서 찾아 validation을 하게 해야 하겠다. 아니면, remote 객체들을 모두 smart ptr, weak ptr로 만들어 안전하게 건드려지는 tag로 바꾸던가.
	// 	}

	void CNetServerImpl::OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/)
	{
		AssertIsNotLockedByCurrentThread();

		CriticalSectionLock statsLock(m_statsCritSec, true); // 통계 정보만 갱신
		// 주의: 여기서 main lock 하면 데드락 생긴다! 잠금 순서 지키자.
		if ( socketType == SocketType_Tcp )
		{
			m_stats.m_totalTcpSendCount++;
			m_stats.m_totalTcpSendBytes += doneBytes;
		}
		else
		{
			m_stats.m_totalUdpSendCount++;
			m_stats.m_totalUdpSendBytes += doneBytes;
		}
	}

	// overrided function
	void CNetServerImpl::OnMessageReceived(int doneBytes, SocketType socketType, CReceivedMessageList& messageList, CSuperSocket* socket)
	{
		AssertIsNotLockedByCurrentThread();

		// socket의 critsec은 unlock 상태이다. 따라서 언제든지 다른 스레드에 의해
		// 송신 버퍼는 변경될 수 있다. 하지만 수신 버퍼는 바뀌지 않는다.
		// 왜냐하면 next recv issue를 이 스레드가 추후 걸기 전에는 안오며,
		// 이 소켓에 대한 epoll recv avail event도 이 스레드에서만 발생하기 때문이다.
		// 따라서 이 함수에서 수신버퍼를 건드리는 로직들은 모두 안전하다.
		{
			CriticalSectionLock statsLock(m_statsCritSec, true); // 통계 정보만 갱신

			if ( socketType == SocketType_Tcp )
			{
				m_stats.m_totalTcpReceiveCount++;
				m_stats.m_totalTcpReceiveBytes += doneBytes;

			}
			else if ( socketType == SocketType_Udp )
			{
				m_stats.m_totalUdpReceiveCount++;
				m_stats.m_totalUdpReceiveBytes += doneBytes;
			}
		}


/*		// 3003 오류 검사용
		// 서버가 3003을 받을 일 없다.
		for (auto i : messageList)
		{
			MessageInternalLayer match;
			match.m_simplePacketMode = m_simplePacketMode;
			match.Analyze(i.m_unsafeMessage);
			if (match.m_rmiLayer.m_rmiID == 3003)
			{
				puts("---3003!!!---");
				for (auto i : messageList)
				{
					MessageInternalLayer match;
					match.m_simplePacketMode = m_simplePacketMode;
					match.Analyze(i.m_unsafeMessage);
					match.Dump();
				}
				puts("------");
				assert(0);
			}
		}*/



		ProcessMessageOrMoveToFinalRecvQueue(socketType, socket, messageList);
	}

	void CNetServerImpl::ProcessDisconnecting(CSuperSocket* socket, const ErrorInfo& errorInfo)
	{
		Proud::AssertIsNotLockedByCurrentThread(socket->m_cs);

		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CRemoteClient_S* rc = GetRemoteClientBySocket_NOLOCK(socket, AddrPort::Unassigned);
		if ( rc != NULL )
		{
			// TCP socket에서 연결이 실패했으므로 연결 해제 처리를 한다.
			// NOTE: GarbageHost 함수 내부의 OnHostGarbaged에서 이미 처리하도록 되어 있습니다. 
			RemoteClient_GarbageOrGoOffline(
				rc,
				ErrorType_DisconnectFromRemote,
				ErrorType_TCPConnectFailure,
				ByteArray(),
				//errorInfo.m_comment,
				_PNT("PD"),
				errorInfo.m_socketError);

			// 혹시 이거 해야 하나? 상관없지만. rc->IssueDispose(ErrorType_DisconnectFromRemote, ErrorType_TCPConnectFailure, ByteArray(), _PNT("PD"), r);
			rc->WarnTooShortDisposal(_PNT("PD"));

			/*  == > 여기에 도착했을 때는 remoteAddr이 없어서 오류가 발생하여 임시로 주석처리
				=> rc->m_tcpLayer->m_remoteAddr 쓰지 말고,
				socket->m_remoteAddr을 쓰면 될 것 같아요.
				*/
			if ( m_logWriter != NULL ) // 사용자가 EnableLog를 켰다면
			{
				String text;
				text.Format(_PNT("Server: Client is disconnected. HostID=%d, Addr=%s, error code = %d"),
							rc->m_HostID,
							socket->m_remoteAddr.ToString().GetString(),
							errorInfo.m_socketError);
				m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, text);
			}
		}
		else
		{
			// 대응하는 socket이 없는 경우 그냥 무시
			// listen socket에 대해서는 다른 곳에서 핸들링함
			if ( socket->m_isListeningSocket && m_listening )
			{
				// 에러를 토한다. 
				EnqueErrorOnIssueAcceptFailed(errorInfo.m_socketError);
			}

			/* Q: ACR 연결 후보는 처리 안 하나요?
			A: 그건 NC에서 하는 일이에요 -_-
			*/
		}
	}

	void CNetServerImpl::EnqueErrorOnIssueAcceptFailed(SocketErrorCode e)
	{
		if ( e != SocketErrorCode_NotSocket && e != SocketErrorCode_Ok )
		{
			String cmt;
			cmt.Format(_PNT("FATAL: AcceptEx failed with error code %d! No more accept will be possible."), e);
			EnqueError(ErrorInfo::From(ErrorType_Unexpected, HostID_Server, cmt));
		}
	}

	void CNetServerImpl::EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		for ( RemoteToInt64Map::iterator i = m_garbagedHosts.begin(); i != m_garbagedHosts.end(); ++i )
		{
			CRemoteClient_S* rc = LeanDynamicCastForRemoteClient(i->GetFirst());

			if ( rc )
			{
				HardDisconnect_AutoPruneGoesTooLongClient(rc);
			}
		}
	}

	// remote client의, 외부 인터넷에서도 인식 가능한 TCP 연결 주소를 리턴한다.
	Proud::NamedAddrPort CNetServerImpl::GetRemoteIdentifiableLocalAddr(CRemoteClient_S* rc)
	{
		AssertIsNotLockedByCurrentThread();//mainlock만으로 진행이 가능할듯.

		NamedAddrPort ret = NamedAddrPort::From(rc->m_tcpLayer->GetSocketName());
		ret.OverwriteHostNameIfExists(rc->m_tcpLayer->GetSocketName().ToDottedIP());  // TCP 연결을 수용한 소켓의 주소. 가장 연결 성공률이 낮다. NIC가 두개 이상 있어도 TCP 연결을 받은 주소가 UDP 연결도 받을 수 있는 확률이 크므로 OK.
		ret.OverwriteHostNameIfExists(m_localNicAddr); // 만약 NIC가 두개 이상 있는 서버이며 NIC 주소가 지정되어 있다면 지정된 NIC 주소를 우선 선택한다.
		ret.OverwriteHostNameIfExists(m_serverAddrAlias); // 만약 서버용 공인 주소가 따로 있으면 그것을 우선해서 쓰도록 한다.

		if ( !ret.IsUnicastEndpoint() )
		{
			//assert(0); // 드물지만 있더라. 어쨌거나 어설션 즐
			ret.m_addr = CNetUtil::GetOneLocalAddress();
		}

		return ret;
	}

	void CNetServerImpl::OnSocketGarbageCollected(CSuperSocket* socket)
	{

	}

	// 메모리 고갈 상황에서, 잔여 공간 더 확보 후 네트웍 연결 끊기를 수행한다.
	void CNetServerImpl::BadAllocException(Exception& ex)
	{
		FreePreventOutOfMemory();
		if ( m_eventSink_NOCSLOCK != NULL )
		{
			m_eventSink_NOCSLOCK->OnException(ex);
		}
		Stop();
	}

	bool CNetServerImpl::Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult)
	{
		return stub->ProcessReceivedMessage(receivedMessage, hostTag);
	}

	// rc가 모든 direct p2p 연결을 잃어버렸을 때의 처리. 릴레이를 해야 함.
	// 이미 rc 넷클라는 자신이 p2p 연결을 잃어버린 것을 안다는 가정하에 콜 되는 함수.
	void CNetServerImpl::FallbackP2PToRelay(CRemoteClient_S * rc, ErrorType reason)
	{
		for ( CRemoteClient_S::P2PConnectionPairs::iterator i = rc->m_p2pConnectionPairs.begin(); i != rc->m_p2pConnectionPairs.end(); i++ )
		{
			P2PConnectionState* state = *i;
			FallbackP2PToRelay(rc, state, reason);
		}
	}

	// rc와 state가 가리키는 rc의 peer간의 direct p2p 연결을 잃어버렸을 때의 처리. 릴레이를 해야 함.
	// 이미 rc 넷클라는 자신이 p2p 연결을 잃어버린 것을 안다는 가정하에 콜 되는 함수.
	void CNetServerImpl::FallbackP2PToRelay(CRemoteClient_S * rc, P2PConnectionState* state, ErrorType reason)
	{
		if ( !state->IsRelayed() )
		{
			state->SetRelayed(true);

			HostID remotePeerHostID;
			if ( state->m_firstClient == rc )
				remotePeerHostID = state->m_secondClient->m_HostID;
			else
				remotePeerHostID = state->m_firstClient->m_HostID;

			// rc의 상대방 peer에게 p2p 연결 증발을 알린다.
			m_s2cProxy.P2P_NotifyDirectP2PDisconnected2(remotePeerHostID, g_ReliableSendForPN, rc->m_HostID, reason);

			// 로그를 남긴다
			if ( m_logWriter )
			{
				String txt;
				txt.Format(_PNT("P2P UDP hole-punching between Client %d and Client %d is disabled."), rc->m_HostID, remotePeerHostID);
				m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
			}
		}
	}

}
