/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "RemoteClient.h"
#include "../include/NetClientInfo.h"
#include "P2PGroup_S.h"
#include "NetServer.h"
#include "NetUtil.h"
#include "../include/numutil.h"

namespace Proud
{

	CriticalSection& CRemoteClient_S::GetOwnerCriticalSection()
	{
		return m_owner->GetCriticalSection();
	}

	CRemoteClient_S::CRemoteClient_S(
		CNetServerImpl *owner,
		const shared_ptr<CSuperSocket>& newSocket,
		AddrPort tcpRemoteAddr,
		int defaultTimeoutTimeMs,
		int autoConnectionRecoveryTimeoutMs)
		: m_ToClientUdp_Fallbackable (this)
		, m_timeoutTimeMs(defaultTimeoutTimeMs)
		, m_autoConnectionRecoveryTimeoutMs(autoConnectionRecoveryTimeoutMs)
	{
		if (newSocket == nullptr)
		{
			ShowUserMisuseError(_PNT("Bad state at CRemoteClient_S! Report this to Nettention."));
		}

		m_owner = owner;

		// 외부에서 만들어진 socket을 가져온다.
		m_tcpLayer = newSocket;
		m_tcpLayer->m_remoteAddr = tcpRemoteAddr;

		//m_tcpLayer->CreateSocket(); 이미 소켓을 받았으니.

		// Per RC UDP socket이라면 이 RC가 직접 소유권을 가진다.
		/*if(owner->m_udpAssignMode == ServerUdpAssignMode_PerClient)
		{
		m_ownedUdpSocket = assignedUdpSocket;
		}*/

		m_safeTimes.Set_lastRequestMeasureSendSpeedTime(0);
		m_sendSpeed = 0;

		m_disposeCaller = NULL;
		int64_t createdTime = owner->GetCachedServerTimeMs();

		m_maxDirectP2PConnectionCount = CNetConfig::DefaultMaxDirectP2PMulticastCount;
		m_HostID = HostID_None;

		m_sessionKey = shared_ptr<CSessionKey>(new CSessionKey);

		m_lastPingMs = 0;
		m_sendQueuedAmountInBytes = 0;
		m_owner = owner;
		m_createdTime = createdTime;
        
		m_safeTimes.Set_arbitraryUdpTouchedTime(createdTime);

		m_encryptCount = m_decryptCount = 0;

		m_autoConnectionRecoveryWaitBeginTime = 0;

		m_enableAutoConnectionRecovery = false;
		m_closeNoPingPongTcpConnections = false;
		m_enquedClientOfflineEvent = false;

		m_SuperPeerRating = 0;
		m_lastApplicationHint.m_recentFrameRate = 0;

		m_safeTimes.Set_requestAutoPruneStartTime(0);
		
		m_toRemotePeerSendUdpMessageSuccessCount = 0;
		m_toRemotePeerSendUdpMessageTrialCount = 0;
		m_toServerSendUdpMessageSuccessCount = 0;
		m_toServerSendUdpMessageTrialCount = 0;

		m_safeTimes.Set_sendQueueWarningStartTime(0);
		
		m_unreliableRecentReceivedSpeed = 0;

#ifndef PROUDSRCCOPY
		m_RT = RuntimePlatform_Last;
#endif

		if(CNetConfig::EnableSpeedHackDetectorByDefault)
			m_speedHackDetector.Attach(new CSpeedHackDetector);

		//m_finalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		if (m_tcpLayer->m_fastSocket)	// Websocket은 이게 null이라서...
		{
		m_tcpLayer->SetEnableNagleAlgorithm(owner->m_settings.m_enableNagleAlgorithm);
		}

		// C/S 첫 핑퐁 작업 : 클라와의 reliable 핑의 마지막 시간과 평균값
		m_lastReliablePingMs = 0;
	}

#ifdef SUPPORTS_WEBGL
	// websocket client를 위한 또다른 생성자.
	CRemoteClient_S::CRemoteClient_S(CNetServerImpl *owner, const shared_ptr<CSuperSocket>& newSocket)
		: m_ToClientUdp_Fallbackable(this)
	{
		m_owner = owner;
		m_tcpLayer = newSocket;
	}
#endif

	void CRemoteClient_S::ToNetClientInfo( CNetClientInfo &ret )
	{
		m_owner->AssertIsLockedByCurrentThread();

		ret.m_HostID = m_HostID;
		ret.m_UdpAddrFromServer = m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
		if (m_tcpLayer != NULL)
		{
			ret.m_TcpAddrFromServer = m_tcpLayer->m_remoteAddr;
			ret.m_TcpLocalAddrFromServer = m_tcpLayer->GetLocalAddr();
		}
		ret.m_UdpAddrInternal = m_ToClientUdp_Fallbackable.m_UdpAddrInternal;
		ret.m_recentPingMs = m_recentPingMs;
		ret.m_sendQueuedAmountInBytes = m_sendQueuedAmountInBytes;
		ret.m_realUdpEnabled = m_ToClientUdp_Fallbackable.m_realUdpEnabled;
		
		for (JoinedP2PGroups_S::iterator ig = m_joinedP2PGroups.begin();ig != m_joinedP2PGroups.end();ig++)
		{
			ret.m_joinedP2PGroups.Add(ig->GetFirst());
		}
		for (int i=0;i<(int)m_hadJoinedP2PGroups.GetCount();i++)
		{
			ret.m_joinedP2PGroups.Add(m_hadJoinedP2PGroups[i]);
		}

		ret.m_RelayedP2P = false;
		ret.m_isBehindNat = IsBehindNat();
		ret.m_natDeviceName = m_natDeviceName;
		ret.m_hostTag = m_hostTag;
		ret.m_recentFrameRate = m_lastApplicationHint.m_recentFrameRate;

		//add by rekfkno1
		ret.m_hostIDRecycleCount = m_owner->m_HostIDFactory->GetRecycleCount(m_HostID);
		ret.m_toServerSendUdpMessageTrialCount = m_toServerSendUdpMessageTrialCount;
		ret.m_toServerSendUdpMessageSuccessCount = m_toServerSendUdpMessageSuccessCount;
		ret.m_unreliableMessageRecentReceiveSpeed = m_unreliableRecentReceivedSpeed;
	}

	bool CRemoteClient_S::IsBehindNat()
	{
		return AddrPort::IsEqualAddress(m_ToClientUdp_Fallbackable.GetUdpAddrFromHere(), m_ToClientUdp_Fallbackable.m_UdpAddrInternal);
	}

	Proud::CNetClientInfoPtr CRemoteClient_S::ToNetClientInfo()
	{
		CNetClientInfoPtr ret(new CNetClientInfo);

		ToNetClientInfo(*ret);

		return ret;
	}

// 	bool CRemoteClient_S::IsFinalReceiveQueueEmpty()
// 	{
// 		m_owner->AssertIsLockedByCurrentThread();
// 		return m_finalUserWorkItemList.GetCount() == 0;
// 	}
// 
	CRemoteClient_S::~CRemoteClient_S()
	{
		// 여기서 owner를 접근하지 말 것. 
		// PopUserTask 후 RAII로 인해 no main lock 상태에서 여기가 호출될 수 있다.
		// 정리해야 할게 있으면 여기가 아니라 OnHostGarbageCollected or OnHostGarbaged에 구현할 것.

		// 여기 왔을 때는 소유하고 있는 SuperSocket ptr 멤버들이 이미 위 함수에 의해 null로 세팅되어 있어야 한다.
		assert(!m_ToClientUdp_Fallbackable.m_udpSocket);
		assert(!m_tcpLayer);
	}

	void CRemoteClient_S::DetectSpeedHack()
    {
        m_owner->AssertIsLockedByCurrentThread();

        if (m_speedHackDetector == nullptr)
            return;

        // 아직 HostID가 없으면, 초기 접속을 시도중이거나, ACR 접속이 시도중인거다.
        // 초기 접속이 시도중인데 스핵감지 핑(MessageType_SpeedHackDetectorPing)이 오면 해커의 소행이거나 ACR 완료 후 AMR에 의한 대량 핑일 수 있다.
        // 이런건 다 버리자.
        if (m_HostID == HostID_None)
            return;
        
        // 여러가지 필터아웃 끝
        // 이제 본작업 시작.

        int64_t lastPing = m_speedHackDetector->m_lastPingReceivedTimeMs;
        int64_t currPing = GetPreciseCurrentTimeMs();

        if (m_speedHackDetector->m_firstPingReceivedTimeMs == 0)
        {
            // 처음에는 그냥 넘어간다.
            m_speedHackDetector->m_firstPingReceivedTimeMs = currPing;
            m_speedHackDetector->m_lastPingReceivedTimeMs = currPing;
        }
        else if (m_speedHackDetector->m_hackSuspected == false)
        {
            //if (m_autoConnectionRecoverySuccessTime != 0)
            //{
            //    int a = 1;
            //}

            int reckness = min(90, m_owner->m_SpeedHackDetectorReckRatio);
            reckness = max(20, reckness);

            m_speedHackDetector->m_recentPingIntervalMs = LerpInt(currPing - lastPing, m_speedHackDetector->m_recentPingIntervalMs, (int64_t)reckness, (int64_t)100);
            //NTTNTRACE("RECENT PING INTERVAL = %f\n",m_speedHackDetector->m_recentPingInterval);

            // 클라 생성 후 충분한 시간이 지났으며


            // 현재 스핵은 주기가 짧아지는 경우만 체크한다. 주기가 긴 경우는 TCP fallback의 의심 상황일 수 있으므로 넘어간다.
            if (currPing - m_speedHackDetector->m_firstPingReceivedTimeMs > CNetConfig::SpeedHackDetectorPingIntervalMs * 10)
            {
                if (m_speedHackDetector->m_recentPingIntervalMs < (CNetConfig::SpeedHackDetectorPingIntervalMs * 7) / 10)
                    m_speedHackDetector->m_hackSuspected = true;
            }
            //디스를 시키는건 오인에 민감하다. 따라서 경고만 날린다. 디스를 시키거나 통계를 수집하는건 엔진 유저의 몫이다.
            // 단, 1회만 날린다.
            if (m_speedHackDetector->m_hackSuspected)
            {
                m_owner->EnqueueHackSuspectEvent(shared_from_this(), "Speedhack", HackType_SpeedHack);
            }

            m_speedHackDetector->m_lastPingReceivedTimeMs = currPing;
        }
    }

	void CRemoteClient_S::WarnTooShortDisposal(const PNTCHAR* funcWhere)
	{
		m_owner->AssertIsLockedByCurrentThread();
		if(!m_disposeCaller)
		{
			m_disposeCaller = funcWhere;
			if(m_owner->GetCachedServerTimeMs() - m_createdTime < 300)
			{
				if(m_owner->m_logWriter)
				{
					m_owner->m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, _PNT("A client just joined here and left. Is it your intention or bug?"));
				}
			}
		}
	}

	// 이 클라가 속한 P2P 그룹 안에서,
	// 클라와 P2P 연결되어 있는 다른 피어들과의 레이턴시의 총합을 리턴한다.
	// 그룹 자체가 없으면 -1 리턴.
	// 수퍼피어 판단을 하는데 사용되는 함수.
	int CRemoteClient_S::GetP2PGroupTotalRecentPing(HostID groupID)
	{
		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);
		CP2PGroupPtr_S group = m_owner->GetP2PGroupByHostID_NOLOCK(groupID);

		// 그룹 자체가 없으면 -1 리턴.
		if (!group)
			return -1;

		if (group->m_members.empty())
			return 0;

		// touch jit p2p & get recent ping ave
// TODO: 터치 안하는데? dev003 끝나고 나면 추후 고치자. (아래 주석)
		int cnt = 0;
		int total = 0;
		int unknownCount = 0;
		for (P2PGroupMembers_S::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
		{
			int ping = m_owner->GetRecentUnreliablePingMs(iMember->first); 

/*	이게 맞지만 dev003에서는 일단 이렇게 냅두자
문제 더 있다: 이걸 얻는데 너무 많은 시간이 걸린다. ReportP2PPeerPing RMI를 통해서 받는데, 이게 양이 많음.
배열 형태로 바꾸고 인터벌을 사용자의 선택에 따라 짧게 해주는 기능이 있어야 할지도.
수퍼피어 선정 뿐만 아니라 routed multicast에서도 피어간 핑을 쓴다. 따라서 필수.
그리고 수퍼피어 선정 기능 작동하면 JIT P2P 터치를 해주어야 하는데 그런 곳도 없다.

			P2PConnectionStatePtr P2PPair = m_owner->m_p2pConnectionPairList.GetPair(m_HostID, iMember->first);
			if(P2PPair)
			ping = P2PPair->m_recentPingMs;
			else 
			ping =-1;
*/

			if (ping >=0)
			{
				cnt++;
				total += ping;
			}
			else
				unknownCount++;
		}

		// 피어는 있지만 레이턴시가 미측정인 것들은, 그룹 내 총 평균으로 대체되어 합산된다.
		int average;
		if (cnt == 0)
			average = 0;
		else
			average = total / cnt;

		return total + (unknownCount * average);
	}

	bool CRemoteClient_S::IsDispose()
	{
		return (m_disposeWaiter != NULL);
	}

// 	void CRemoteClient_S::IssueDispose(ErrorType errorType, ErrorType detailType, const ByteArray& comment, const PNTCHAR* where, SocketErrorCode socketErrorCode)
// 	{
// 		m_owner->GarbageHost(this,errorType,detailType,comment,where,socketErrorCode);
// 	}
// 
	bool CRemoteClient_S::IsValidEnd()
	{
		return (m_owner->m_TcpListenSockets.GetCount() > 0);
	}

// 	void CRemoteClient_S::IssueSendOnNeed(int64_t currTime)
// 	{
// 		m_tcpLayer->IssueSendOnNeed(currTime);
// 	}
#ifdef USE_HLA

	CFastSet<CHlaSpace_S*>& CRemoteClient_S::GetViewingSpaces()
	{
		return m_hlaViewingSpaces;
	}
#endif
	bool CRemoteClient_S::IsLockedByCurrentThread()
	{
		// TCP Socket이 NULL이라면 처리하지 않습니다.
		if (m_tcpLayer == NULL)
			return false;

		// 일부라도 잠겼으면 true 리턴
		return (m_tcpLayer->IsLockedByCurrentThread() || m_tcpLayer->IsSendQueueLockedByCurrentThread());	
	}

// 	void CRemoteClient_S::CheckTcpInDanger( int64_t currTime )
// 	{
// 		m_owner->AssertIsLockedByCurrentThread();
// 
// 		if(m_ownedUdpSocket)
// 		{
// 			if(m_ownedUdpSocket->m_lastUdpSendIssueStopTime == 0)
// 			{
// 				int64_t tcpLastSendIssuedTime = m_tcpLayer->m_lastSendIssuedTime;
// 				//tcp sendissue가 5초이상 completion이 되지 않았다면,
// 				if(tcpLastSendIssuedTime != 0 
// 					&& currTime - tcpLastSendIssuedTime > CNetConfig::TcpInDangerThresholdMs)
// 				{
// 					m_owner->EnqueWarning(ErrorInfo::From(ErrorType_TcpCrisisDetected, m_HostID));
// 					m_ownedUdpSocket->m_lastUdpSendIssueStopTime = currTime; // 지금부터 UDP를 일시 정지하자.
// 				}
// 			}
// 			else
// 			{
// 				// 2초 이상 UDP 송신을 일시정지했다면 이제 풀어주자.
// 				if(currTime - m_ownedUdpSocket->m_lastUdpSendIssueStopTime >= CNetConfig::PauseUdpSendDurationOnTcpInDangerMs)
// 					m_ownedUdpSocket->m_lastUdpSendIssueStopTime = 0;
// 			}
// 		}
// 	}

	// 패킷양이 많은지 판단
// 	bool CRemoteClient_S::CheckMessageOverloadAmount()
// 	{
// 		if(m_finalUserWorkItemList.GetCount()
// 			>= CNetConfig::MessageOverloadWarningLimit)
// 		{
// 			return true;
// 		}
// 		return false;
// 	}

	// SuperSocket 뿐만 아니라 remote도 자체적인 doForLongInterval이 있다.
	void CRemoteClient_S::DoForLongInterval(int64_t /*currTime*/)
	{
		// currTime는 안쓰이지만 DoForLongIntervalFunctor가 요구하므로.

		// 필요시 RTT기반 coal interval 설정후
		SetManualOrAutoCoalesceInterval(m_coalesceIntervalMs);

		// UDP sendto addr에 대응하는 send queue의 coal interval로 전파한다.
		if (m_ToClientUdp_Fallbackable.m_udpSocket != NULL)
		{
			CriticalSectionLock lock(m_ToClientUdp_Fallbackable.m_udpSocket->m_sendQueueCS, true);

			m_ToClientUdp_Fallbackable.m_udpSocket->SetCoalesceInterval(m_ToClientUdp_Fallbackable.GetUdpAddrFromHere(), m_coalesceIntervalMs);
		}

		//m_tcpLayer->DoForLongInterval(currTime); // CNetServerImpl.DoForLongInterval의 맨 밑단에서 하고 있다. 여기서는 하지 말자.
	}

	// 클라 입장에서 서버쪽과 UDP 통신 가능한, 서버쪽에서 준비된 클라의 UDP 주소
	NamedAddrPort CRemoteClient_S::GetRemoteIdentifiableLocalAddr()
	{
		AssertIsSocketNotLockedByCurrentThread();//mainlock만으로 진행이 가능할듯.

		AddrPort addr = m_ToClientUdp_Fallbackable.m_udpSocket->GetSocketName();
		NamedAddrPort ret = NamedAddrPort::From(addr);
		ret.OverwriteHostNameIfExists(m_tcpLayer->GetSocketName().IPToString());  // TCP 연결을 수용한 소켓의 주소. 가장 연결 성공률이 낮다. NIC가 두개 이상 있어도 TCP 연결을 받은 주소가 UDP 연결도 받을 수 있는 확률이 크므로 OK.
		ret.OverwriteHostNameIfExists(m_owner->m_localNicAddr); // 만약 NIC가 두개 이상 있는 서버이며 NIC 주소가 지정되어 있다면 지정된 NIC 주소를 우선 선택한다.
		ret.OverwriteHostNameIfExists(m_owner->m_serverAddrAlias); // 만약 서버용 공인 주소가 따로 있으면 그것을 우선해서 쓰도록 한다.

// GetOneLocalAddress 사용 자체가 NIC 하나만 있다는 가정이므로 형편에 안 맞음.
// 		if (!ret.IsUnicastEndpoint())
// 		{
// 			//assert(0); // 드물지만 있더라. 어쨌거나 어설션 즐
// 			ret.m_addr = CNetUtil::GetOneLocalAddress();
// 		}

		return ret;
	}


}
