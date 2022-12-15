#include "stdafx.h"
#include "NetServer.h"
#include "SendDest_S.h"
#include "RmiContextImpl.h"

namespace Proud
{
	DEFRMI_ProudC2S_NotifyP2PHolepunchSuccess(CNetServerImpl::C2SStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		/* ignore if A-B is already passed
		다중 경로로 holepunch가 짧은 시간차로
		다수 성공할 경우 가장 최초의 것만
		선택해주는 중재역할을 서버가 해야 하므로 */

		P2PConnectionStatePtr state;
		state = m_owner->m_p2pConnectionPairList.GetPair(A, B);

		if (state == nullptr)
		{
			NTTNTRACE("경고: P2P 연결 정보가 존재하지 않으면서 홀펀칭이 성공했다고라고라?\n");
		}
		else
		{
			// 직접 연결이 되었다는 신호를 보내도록 한다.
			if (state->IsRelayed() == true)
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
				m_owner->m_s2cProxy.NotifyDirectP2PEstablish(A, g_ReliableSendForPN, A, B, ABSendAddr, ABRecvAddr, BASendAddr, BARecvAddr, reliableRTT, unreliableRTT,
					CompactFieldMap());
				m_owner->m_s2cProxy.NotifyDirectP2PEstablish(B, g_ReliableSendForPN, A, B, ABSendAddr, ABRecvAddr, BASendAddr, BARecvAddr, reliableRTT, unreliableRTT,
					CompactFieldMap());

				// 로그를 남긴다.
				if (m_owner->m_logWriter)
				{
					Tstringstream ss;
					ss << "The Final Stage of P2P UDP Hole-Punching between Client " << A
						<< " and Client " << B
						<< " success: ABSendAddr=" << ABSendAddr.ToString().GetString()
						<< " ABRecvAddr=" << ABRecvAddr.ToString().GetString()
						<< " BASendAddr=" << BASendAddr.ToString().GetString()
						<< " BARecvAddr=" << BARecvAddr.ToString().GetString();
					m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
				}

			}
		}
		return true;
	}

	DEFRMI_ProudC2S_P2P_NotifyDirectP2PDisconnected(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		NTTNTRACE("%d-%d P2P connection lost. Reason=%d\n", remote, remotePeerHostID, reason);
		// check validation
		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remotePeerHostID);
		if (rc != nullptr)
		{
			// 서버에 저장된 두 클라간 연결 상태 정보를 갱신한다.
			P2PConnectionStatePtr state = m_owner->m_p2pConnectionPairList.GetPair(remote, remotePeerHostID);
			if (state != nullptr && !state->IsRelayed())
			{
				state->SetRelayed(true);

				// ack를 날린다.
				m_owner->m_s2cProxy.P2P_NotifyDirectP2PDisconnected2(remotePeerHostID, g_ReliableSendForPN, remote, reason,
					CompactFieldMap());

				// 로그를 남긴다
				if (m_owner->m_logWriter)
				{
					Tstringstream ss;
					ss << "Cancelled P2P UDP hole-punching between Client " << remote << " and Client " << remotePeerHostID;
					m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
				}
			}
		}
		return true;
	}

	DEFRMI_ProudC2S_NotifyJitDirectP2PTriggered(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// check validation
		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(peerB_ID);
		if (rc != nullptr)
		{
			// 서버에 저장된 두 클라간 연결 상태 정보를 완전히 초기화한다.
			// 이미 Direct P2P로 지정되어 있어도 말이다.
			shared_ptr<P2PConnectionState> state = m_owner->m_p2pConnectionPairList.GetPair(remote, peerB_ID);
			if (state != nullptr)
			{
				auto state_f = state->m_firstClient.lock();
				auto state_s = state->m_secondClient.lock();
				if (state_f->m_p2pConnectionPairs.GetCount() < state_f->m_maxDirectP2PConnectionCount
					&& state_s->m_p2pConnectionPairs.GetCount() < state_s->m_maxDirectP2PConnectionCount)
				{
					// UDP Socket이 생성되어있지 않다면, UDP Socket 생성을 요청한다.
					// (함수 안에서, S2C_RequestCreateUdpSocket도 전송함.)
					m_owner->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(state_f);
					m_owner->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(state_s);

					// (reliable messaging이므로, 연이어 아래 요청을 해도 순서 보장된다. 아래 요청을 받는 클라는 이미 UDP socket이 있을테니까.)
					// 양측 클라 모두에게, Direct P2P 시작을 해야하는 조건이 되었으니 서로 홀펀칭을 시작하라는 지령을 한다.
					m_owner->m_s2cProxy.NewDirectP2PConnection(peerB_ID, g_ReliableSendForPN, remote);
					m_owner->m_s2cProxy.NewDirectP2PConnection(remote, g_ReliableSendForPN, peerB_ID);
					state->m_jitDirectP2PRequested = true;

					// 로그를 남긴다.
					if (m_owner->m_logWriter)
					{
						Tstringstream ss;
						ss << "Ready for P2P UDP hole-punching between Client " << remote << " and Client " << peerB_ID << ". Starting hole-punching.";
						m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
					}
				}
			}
		}
		return true;
	}


	DEFRMI_ProudC2S_ReportP2PPeerPing(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		// 어차피 congestion control이 들어가야 함. 20초보다는 빨리 디스를 감지해야 하므로.
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		if (recentPing >= 0)
		{
			shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
			if (rc != nullptr)
			{
				// 두 클라의 P2P 연결 정보에다가 레이턴시를 기록한다.
				for (CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin();
					iPair != rc->m_p2pConnectionPairs.end();
					iPair++)
				{
					shared_ptr<P2PConnectionState> pair = iPair.GetSecond();
					if (pair->ContainsHostID(peerID))
						pair->m_recentPingMs = recentPing;
				}
			}
		}
		return true;
	}



	bool CNetServerImpl::GetP2PGroupInfo(HostID groupHostID, CP2PGroup& output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CP2PGroupPtr_S g = GetP2PGroupByHostID_NOLOCK(groupHostID);

		if (g != nullptr)
		{
			g->ToInfo(output);
			return true;
		}
		return false;
	}





	bool CNetServerImpl::GetP2PConnectionStats(HostID remoteHostID, CP2PConnectionStats& status)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CRemoteClient_S> peer = GetAuthedClientByHostID_NOLOCK(remoteHostID);
		shared_ptr<CP2PGroup_S>  g = nullptr;
		if (peer != nullptr)
		{
			status.m_TotalP2PCount = 0;
			status.m_directP2PCount = 0;
			status.m_toRemotePeerSendUdpMessageSuccessCount = peer->m_toRemotePeerSendUdpMessageSuccessCount;
			status.m_toRemotePeerSendUdpMessageTrialCount = peer->m_toRemotePeerSendUdpMessageTrialCount;

			for (CRemoteClient_S::P2PConnectionPairs::iterator itr = peer->m_p2pConnectionPairs.begin(); itr != peer->m_p2pConnectionPairs.end(); ++itr)
			{
				//이것은 전에 한번이라도 p2p로 묶을것을 요청받은적이 있는지에 대한값이다.
				//그렇기 때문에 이것이 false라면 카운트에 들어가지 않는다.
				if (itr.GetSecond()->m_jitDirectP2PRequested)
				{
					++status.m_TotalP2PCount;
					if (itr.GetSecond()->IsRelayed() == false)
					{
						++status.m_directP2PCount;
					}
				}
			}

			/*for (RCPairMap::iterator i = m_p2pConnectionPairList.m_list.begin();i!=m_p2pConnectionPairList.m_list.end();i++)
			{
			shared_ptr<P2PConnectionState> pair = i->second;

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
		else if ((g = GetP2PGroupByHostID_NOLOCK(remoteHostID)) != nullptr)
		{
			//그룹아이디라면 그룹에서 얻는다.
			status.m_TotalP2PCount = 0;
			status.m_directP2PCount = 0;
			status.m_toRemotePeerSendUdpMessageSuccessCount = 0;
			status.m_toRemotePeerSendUdpMessageTrialCount = 0;

			CFastArray<shared_ptr<P2PConnectionState>> tempConnectionStates;
			//tempConnectionStates.SetMinCapacity(멤버갯수의 제곱의 반, 단 최소 1이상.)
			intptr_t t = g->m_members.size() * g->m_members.size(); // pow가 SSE를 쓰는 경우 일부 VM에서 크래시. 그냥 정수 제곱 계산을 하는게 안정.
			t /= 2;
			if (t >= 0)
				tempConnectionStates.SetMinCapacity(t);

			//for (RCPairMap::iterator i = m_p2pConnectionPairList.m_list.begin();i!=m_p2pConnectionPairList.m_list.end();i++)
			//{
			//	shared_ptr<P2PConnectionState> pair = i->second;

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

			for (P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); ++i)
			{
				auto g_member_ptr = i->second.m_ptr.lock();
				if (g_member_ptr && g_member_ptr->GetHostID() != HostID_Server)
				{
					shared_ptr<CRemoteClient_S> iMemberAsRC = static_pointer_cast<CRemoteClient_S>(g_member_ptr);

					// 2010.08.24 : add by ulelio 모든 group원의 총합을 구한다.
					if (iMemberAsRC == nullptr)
						continue;

					status.m_toRemotePeerSendUdpMessageSuccessCount += iMemberAsRC->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemotePeerSendUdpMessageTrialCount += iMemberAsRC->m_toRemotePeerSendUdpMessageTrialCount;

					for (CRemoteClient_S::P2PConnectionPairs::iterator itr = iMemberAsRC->m_p2pConnectionPairs.begin(); itr != iMemberAsRC->m_p2pConnectionPairs.end(); ++itr)
					{
						const P2PConnectionStatePtr& connPairOfMember = itr.GetSecond();
						auto pair_f = connPairOfMember->m_firstClient.lock();
						auto pair_s = connPairOfMember->m_secondClient.lock();

						//다른 그룹일경우 제외하자.
						if (!pair_f || !pair_s
							|| g->m_members.end() == g->m_members.find(pair_f->m_HostID)
							|| g->m_members.end() == g->m_members.find(pair_s->m_HostID))
						{
							continue;
						}

						//이미 검사한 커넥션 페어는 검사하지 않는다.
						if (-1 != tempConnectionStates.FindByValue(connPairOfMember))
						{
							continue;
						}

						if (connPairOfMember->m_jitDirectP2PRequested)
						{
							++status.m_TotalP2PCount;
							if (!connPairOfMember->IsRelayed())
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

		const shared_ptr<CRemoteClient_S>& rcA = GetAuthedClientByHostID_NOLOCK(remoteA);
		const shared_ptr<CRemoteClient_S>& rcB = GetAuthedClientByHostID_NOLOCK(remoteB);

		if (rcA != nullptr && rcB != nullptr)
		{
			CRemoteClient_S::P2PConnectionPairs::iterator iPair;
			for (iPair = rcA->m_p2pConnectionPairs.begin(); iPair != rcA->m_p2pConnectionPairs.end(); iPair++)
			{
				shared_ptr<P2PConnectionState> pair = iPair.GetSecond();
				if (pair->ContainsHostID(rcB->m_HostID))
				{
					status.m_toRemoteBSendUdpMessageSuccessCount = pair->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemoteBSendUdpMessageTrialCount = pair->m_toRemotePeerSendUdpMessageTrialCount;
				}
			}

			for (iPair = rcB->m_p2pConnectionPairs.begin(); iPair != rcB->m_p2pConnectionPairs.end(); iPair++)
			{
				shared_ptr<P2PConnectionState> pair = iPair.GetSecond();
				if (pair->ContainsHostID(rcA->m_HostID))
				{
					status.m_toRemoteASendUdpMessageSuccessCount = pair->m_toRemotePeerSendUdpMessageSuccessCount;
					status.m_toRemoteASendUdpMessageTrialCount = pair->m_toRemotePeerSendUdpMessageTrialCount;
				}
			}

			P2PConnectionStatePtr pair = m_p2pConnectionPairList.GetPair(remoteA, remoteB);
			if (pair != nullptr)
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

	bool CNetServerImpl::SetDirectP2PStartCondition(DirectP2PStartCondition newVal)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (newVal >= DirectP2PStartCondition_LAST)
		{
			ThrowInvalidArgumentException();
		}

		m_settings.m_directP2PStartCondition = newVal;

		return true;
	}

	// P2P route가 가능한 항목끼리 linked list를 구축한다.
	// 2009.11.25 modify by ulelio : ping과 direct P2P 연결이 많은 놈을 선별하여 list를 구축하도록 개선.
	void CNetServerImpl::MakeP2PRouteLinks(SendDestInfoArray& tgt, int unreliableS2CRoutedMulticastMaxCount, int64_t routedSendMaxPing)
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
		if (unreliableS2CRoutedMulticastMaxCount == 0)
			return;

		m_connectionInfoList.Clear(); // 그러나 메모리 블럭은 남아있음쓰

		ConnectionInfo info;

		int i, j, k;
		int tgtCount = (int)tgt.GetCount();
		for (i = 0; i < tgtCount; i++)
		{
			SendDestInfo* lastLinkedItem = &tgt[i];
			lastLinkedItem->mP2PRoutePrevLink = nullptr;
			lastLinkedItem->mP2PRouteNextLink = nullptr;

			for (j = i; j < tgtCount; j++)
			{
				shared_ptr<P2PConnectionState> state;
				if (i != j &&
					(state = m_p2pConnectionPairList.GetPair(tgt[i].mHostID, tgt[j].mHostID)) != nullptr)
				{
					if (!state->IsRelayed() && state->m_recentPingMs < routedSendMaxPing)
					{
						info.state = state;
						info.hostIndex0 = i;
						info.hostIndex1 = j;
						m_connectionInfoList.Add(info);
						int connectioninfolistCount = (int)m_connectionInfoList.GetCount();
						// 현재 host의 directP2P 연결 갯수를 센다.
						for (k = 0; k < connectioninfolistCount; ++k)
						{
							if (m_connectionInfoList[k].hostIndex0 == i)
								++m_connectionInfoList[k].connectCount0;
							else if (m_connectionInfoList[k].hostIndex1 == i)
								++m_connectionInfoList[k].connectCount1;

							if (m_connectionInfoList[k].hostIndex0 == j)
								++m_connectionInfoList[k].connectCount0;
							else if (m_connectionInfoList[k].hostIndex1 == j)
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
		for (i = 0; i < (int)m_connectionInfoList.GetCount(); ++i)
		{
			bContinue = false;

			ConnectionInfo info2 = m_connectionInfoList[i];

			CFastArray<int, false, true>::iterator itr = m_routerIndexList.begin();
			for (; itr != m_routerIndexList.end(); ++itr)
			{
				// 이미 들어간 놈은 제외하자.
				if (*itr == info2.hostIndex0 || *itr == info2.hostIndex1)
				{
					bContinue = true;
					break;
				}
			}

			if (bContinue)
				continue;

			if (info2.connectCount0 >= info2.connectCount1)
				m_routerIndexList.Add(info2.hostIndex0);
			else
				m_routerIndexList.Add(info2.hostIndex1);
		}

		// 링크를 연결해준다.
		int hostindex, nextindex;
		hostindex = nextindex = -1;
		SendDestInfo* LinkedItem = nullptr;
		SendDestInfo* NextLinkedItem = nullptr;

		intptr_t routerindexlistCount = m_routerIndexList.GetCount();
		for (intptr_t routerindex = 0; routerindex < routerindexlistCount; ++routerindex)
		{
			hostindex = m_routerIndexList[routerindex];
			LinkedItem = &tgt[hostindex];

			if (LinkedItem->mP2PRoutePrevLink != nullptr || LinkedItem->mP2PRouteNextLink != nullptr)
				continue;

			intptr_t count = 0;
			intptr_t connectioninfolistCount = m_connectionInfoList.GetCount();
			for (i = 0; i < connectioninfolistCount && count < unreliableS2CRoutedMulticastMaxCount; ++i)
			{
				if (hostindex == m_connectionInfoList[i].hostIndex0)
				{
					//hostindex1을 뒤로 붙인다.
					nextindex = m_connectionInfoList[i].hostIndex1;
					NextLinkedItem = &tgt[nextindex];

					if (nullptr != NextLinkedItem->mP2PRoutePrevLink || nullptr != NextLinkedItem->mP2PRouteNextLink)
						continue;

					LinkedItem->mP2PRouteNextLink = NextLinkedItem;
					NextLinkedItem->mP2PRoutePrevLink = LinkedItem;
					LinkedItem = LinkedItem->mP2PRouteNextLink;
					count++;
				}
				else if (hostindex == m_connectionInfoList[i].hostIndex1)
				{
					//hostindex0을 뒤로 붙인다.
					nextindex = m_connectionInfoList[i].hostIndex0;
					NextLinkedItem = &tgt[nextindex];

					if (nullptr != NextLinkedItem->mP2PRoutePrevLink || nullptr != NextLinkedItem->mP2PRouteNextLink)
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


	void CNetServerImpl::P2PGroup_CheckConsistency()
	{

	}

	void CNetServerImpl::EnqueueP2PGroupRemoveEvent(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		LocalEvent evt;
		evt.m_remoteHostID = groupHostID;
		evt.m_type = LocalEventType_P2PGroupRemoved;
		EnqueLocalEvent(evt, m_loopbackHost);
	}


	void CNetServerImpl::AllowEmptyP2PGroup(bool enable)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (AsyncCallbackMayOccur()) // 서버가 시작되었는가?
		{
			throw Exception("Cannot set AllowEmptyP2PGroup after the server has started.");
		}

		// 		if(!enable && m_startCreateP2PGroup && nullptr == dynamic_cast<CAssignHostIDFactory*>(m_HostIDFactory.m_p))
		// 		{
		// 			throw Exception("Cannot set false after create P2PGroup when started");
		// 		}

		m_allowEmptyP2PGroup = enable;
	}

    bool CNetServerImpl::IsEmptyP2PGroupAllowed() const
    {
        return m_allowEmptyP2PGroup;
    }

	bool P2PConnectionState::ContainsHostID(HostID PeerID)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (m_peers[i].m_ID == PeerID)
				return true;
		}
		return false;
	}

	// peerID의 홀펀칭 내외부 주소를 세팅한다.
	void P2PConnectionState::SetServerHolepunchOk(HostID peerID, AddrPort internalAddr, AddrPort externalAddr)
	{
		for (int i = 0; i < 2; i++)
		{
			if (m_peers[i].m_ID == peerID)
			{
				m_peers[i].m_internalAddr = internalAddr;
				m_peers[i].m_externalAddr = externalAddr;
				return;
			}
		}

		for (int i = 0; i < 2; i++)
		{
			if (m_peers[i].m_ID == HostID_None)
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
		for (int i = 0; i < 2; i++)
		{
			if (m_peers[i].m_ID == peerID)
			{
				return m_peers[i].m_internalAddr;
			}
		}

		return AddrPort::Unassigned;
	}


	AddrPort P2PConnectionState::GetExternalAddr(HostID peerID)
	{
		for (int i = 0; i < 2; i++)
		{
			if (m_peers[i].m_ID == peerID)
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
		for (int i = 0; i < 2; i++)
		{
			if (m_peers[i].m_externalAddr != AddrPort::Unassigned)
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
		if (!m_isLoopbackConnection)
		{
			//	m_owner->m_stats.m_p2pConnectionPairCount++;
		}
	}

	P2PConnectionState::~P2PConnectionState()
	{
		// 통계 정보를 업뎃한다.
		if (!m_isLoopbackConnection)
		{
			//m_owner->m_stats.m_p2pConnectionPairCount--;
		}

		if (m_relayed_USE_FUNCTION == false)
		{
			//m_owner->m_stats.m_p2pDirectConnectionPairCount--;
		}
	}

	// direct or relayed를 세팅한다.
	// relayed로 세팅하는 경우 홀펀칭되었던 내외부 주소를 모두 리셋한다.
	void P2PConnectionState::SetRelayed(bool val)
	{
		if (m_relayed_USE_FUNCTION != val)
		{
			if (val)
			{
				//m_owner->m_stats.m_p2pDirectConnectionPairCount--;
				for (int i = 0; i < 2; i++)
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

		for (int i = 0; i < 2; ++i)
		{
			if (m_peers[i].m_memberJoinAcked)
			{
				++count;
			}
		}

		return (count == 2);
	}

	void P2PConnectionState::MemberJoin_Acked(HostID peerID)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (m_peers[i].m_ID == peerID)
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



	void CNetServerImpl::IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(CMessage& msg, AddrPort from, const shared_ptr<CSuperSocket>& udpSocket)
	{
		// caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
		AssertIsNotLockedByCurrentThread();
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (m_simplePacketMode)
			return;

		// 이 메시지를 TCP로 의도적으로 해킹 전송을 할 경우, 그냥 버려야
		if (udpSocket == nullptr || udpSocket->GetSocketType() != SocketType_Udp)
		{
			assert(false);
			return;
		}

		Guid magicNumber;
		if (msg.Read(magicNumber) == false)
			return;

		HostID peerID;
		if (msg.Read(peerID) == false)
			return;

		// ack magic number and remote UDP address via UDP once
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		Message_Write(sendMsg, MessageType_PeerUdp_ServerHolepunchAck);
		sendMsg.Write(magicNumber);
		sendMsg.Write(from);
		sendMsg.Write(peerID);

		udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
			udpSocket,
			HostID_None,
			FilterTag::CreateFilterTag(HostID_Server, HostID_None),
			from,
			CSendFragRefs(sendMsg),
			GetPreciseCurrentTimeMs(),
			SendOpt(MessagePriority_Holepunch, true));


		//이하는 메인 lock하에 진행.
		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (m_logWriter)
		{
			Tstringstream ss;
			ss << "MessageType_PeerUdp_ServerHolepunch from " << from.ToString().GetString();
			m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
		}
	}


	void CNetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1(
		CMessage& msg,
		const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 처리 불가능.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		AssertIsNotLockedByCurrentThread(); // code profile 결과 이거 꽤 크다.

		// 메시지 읽기

		EncryptMode encMode;
		// EM_None : 그대로 전달
		/* EM_Fast or EM_Secure : 송신자가 Non-WebGL이면 C2S키로 복호화한다.
		송신자가 WebGL일 때, 수신자가 Non-WebGL이면 S2C키로 해당 암호화방식으로 암호화하고, 수신자가 WebGL이면 암호화하지 않는다. */
		if (!Message_Read(msg, encMode))
		{
			return;
		}

		MessagePriority priority;
		int64_t uniqueID;
		bool fragmentOnNeed;
		if (!Message_Read(msg, priority) ||
			!msg.ReadScalar(uniqueID) ||
			!msg.Read(fragmentOnNeed))
		{
			return;
		}

		POOLED_LOCAL_VAR(HostIDArray, relayDest);

		int payloadLength;
		if (!Message_Read(msg, relayDest))
			return;

		if (!msg.ReadScalar(payloadLength))
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if (payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength)
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if (!msg.ReadWithShareBuffer(payload, payloadLength))
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		if (encMode == EM_None)	// encMode가 None이면 암호화가 안되어있거나 C2C키로 되어있다.
		{
			IoCompletion_MulticastUnreliableRelay2(nullptr, relayDest, rc->m_HostID, encMode, payload, priority, uniqueID, fragmentOnNeed);
		}
		else
		{
			// 송신자가 Non-WebGL일 경우 암호화했을 수 있다. 이때 복호화 해준다.
			CMessage decryptedOutput;
			if (GetDecryptedPayloadByRemoteClient(rc, encMode, payload, decryptedOutput) == DecryptResult_Success)
				IoCompletion_MulticastUnreliableRelay2(nullptr, relayDest, rc->m_HostID, encMode, decryptedOutput, priority, uniqueID, fragmentOnNeed);
		}
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(
		CMessage& msg,
		const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 처리 불가능.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		AssertIsNotLockedByCurrentThread(); // 성능에 민감해서 main lock은 최소한만.

		EncryptMode encMode;
		// EM_None : 그대로 전달
		/* EM_Fast or EM_Secure : 송신자가 Non-WebGL이면 C2S키로 복호화한다.
		송신자가 WebGL일 때, 수신자가 Non-WebGL이면 S2C키로 해당 암호화방식으로 암호화하고, 수신자가 WebGL이면 암호화하지 않는다. */
		if (!Message_Read(msg, encMode))
		{
			return;
		}

		MessagePriority priority;
		int64_t uniqueID;
		bool fragmentOnNeed;
		if (!Message_Read(msg, priority) ||
			!msg.ReadScalar(uniqueID) ||
			!msg.Read(fragmentOnNeed))
		{
			return;
		}

		// p2p group 어디에도 비종속 클라들
		POOLED_LOCAL_VAR(HostIDArray, includeeHostIDList);


		POOLED_LOCAL_VAR(P2PGroupSubsetList, p2pGroupSubsetList);

		int p2pGroupSubsetListCount = 0;
		int payloadLength = 0;

		// P2P 일대일 전송되는 릴레이 타겟들. 그룹이 아니다.
		if (!Message_Read(msg, includeeHostIDList))
			return;

		// P2P 그룹 및 거기서 열외될 멤버들을 얻는다.
		if (!msg.ReadScalar(p2pGroupSubsetListCount))
			return;

		// ikpil.choi 2016-11-23 : 데이터가 이미 깨져있을 때, 위험할 수 있으므로, 필요할 때마다 생성하는 코드로 변경
		//p2pGroupSubsetList.SetCount(p2pGroupSubsetListCount);
		if (0 > p2pGroupSubsetListCount)
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		for (int i = 0; i < p2pGroupSubsetListCount; i++)
		{
			P2PGroupSubset_S p2pDest;

			// P2P 그룹 ID
			if (!msg.Read(p2pDest.m_groupID))
				return;

			// 열외 리스트
			if (!Message_Read(msg, p2pDest.m_excludeeHostIDList))
				return;

			// ikpil.choi 2016-11-23 : 데이터가 이미 깨져있을 때, 위험할 수 있으므로, 필요할 때마다 생성하는 코드로 변경
			p2pGroupSubsetList.Add(p2pDest);
		}

		// 보낼 데이터
		if (!msg.ReadScalar(payloadLength))
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if (payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength)
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if (!msg.ReadWithShareBuffer(payload, payloadLength))
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// relay할 호스트들을 뽑아낸다.
		POOLED_LOCAL_VAR(HostIDArray, uncompressedRelayDest);
		includeeHostIDList.CopyTo(uncompressedRelayDest);	// individual로 들어온것들은 그대로 copy

		// P2P 그룹 정보로부터 멤버 정보를 모두 추출한다.
		// rc를 얻어내야하기 때문에 main lock이 필요하다.
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		for (int i = 0; i < p2pGroupSubsetList.GetCount(); i++)
		{
			P2PGroupSubset_S& p2pDest = p2pGroupSubsetList[i];

			shared_ptr<CP2PGroup_S> g = GetP2PGroupByHostID_NOLOCK(p2pDest.m_groupID);

			if (g != nullptr)
			{
				// NOTE: m_excludeeHostIDList와 g->m_members는 정렬되어 있다.
				HostIDArray::iterator itA = p2pDest.m_excludeeHostIDList.begin();		// directp2p로 이미 보낸 리스트
				P2PGroupMembers_S::iterator itB = g->m_members.begin();	// p2p 전체 인원 리스트

				// merge sort와 비슷한 방법으로, group member들로부터 excludee를 제외한 것들을 찾는다.
				for (; itA != p2pDest.m_excludeeHostIDList.end() || itB != g->m_members.end(); )
				{
					if (itA == p2pDest.m_excludeeHostIDList.end())
					{
						uncompressedRelayDest.Add(itB->first);
						itB++;
						continue;
					}
					if (itB == g->m_members.end())
						break;

					if (*itA > itB->first) // 릴레이 인원에 추가시킴
					{
						uncompressedRelayDest.Add(itB->first);
						itB++;
					}
					else if (*itA < itB->first)
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

		IoCompletion_MulticastUnreliableRelay2(&mainLock, uncompressedRelayDest, rc->m_HostID, encMode, payload, priority, uniqueID, fragmentOnNeed);
	}


	void CNetServerImpl::IoCompletion_ProcessMessage_ReliableRelay1(
		CMessage& msg,
		const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 처리 불가능.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		AssertIsNotLockedByCurrentThread(); // main lock도 무거우니까 필요할 때만 최소한으로 한다.

		POOLED_LOCAL_VAR(RelayDestList, relayDest);

		EncryptMode encMode;
		// EM_None : 그대로 전달
		/* EM_Fast or EM_Secure : 송신자가 Non-WebGL이면 C2S키로 복호화한다.
		송신자가 WebGL일 때, 수신자가 Non-WebGL이면 S2C키로 해당 암호화방식으로 암호화하고, 수신자가 WebGL이면 암호화하지 않는다. */
		if (!Message_Read(msg, encMode))
		{
			return;
		}

		int payloadLength;
		if (!Message_Read(msg, relayDest) ||	// relay list(각 수신자별 프레임 번호 포함)
			!msg.ReadScalar(payloadLength))	// 크기
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if (payloadLength < 0 || payloadLength >= m_settings.m_serverMessageMaxLength)
		{
			ASSERT_OR_HACKED(0);   // 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if (!msg.ReadWithShareBuffer(payload, payloadLength))
		{
			ASSERT_OR_HACKED(0);		// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 주의 : 넷마블에서 올린 3305 크래시 이슈로 인하여 드러난 페이로드가 null이 되는 경우를 막기 위한 땜빵 코드입니다.
		// 근원적인 원인을 찾지 못한 상태입니다.
		if (payload.m_msgBuffer.IsNull())
			return;

		intptr_t relayCount = relayDest.GetCount();
		if (relayCount <= 0)
			return;

		CMessage decryptedOutput;
		// EM_None일경우 서버는 신경쓸 필요없이 그대로 전달.
		// EM_None이 아니면 WebGL클라의 PlainText를 서버에서 Relay2때 암호화 해달라는 요청이거나, Non-WebGL클라의 Payload가 암호화 되어있다는 알림.
		// 송신자가 Non-WebGL일 경우 암호화했을 수 있다. 이때 복호화 해준다. 복호화에 실패한 경우 relay하지 않는다.
		if (encMode != EM_None && GetDecryptedPayloadByRemoteClient(rc, encMode, payload, decryptedOutput) == DecryptResult_Fail)
			return;

		// NOTE: 여기서부터는 encMode에 따라 payload or decryptedOutput중 택일되어 사용된다.

		// 각 수신자마다 받아야 하는 데이터 내용이 다르다. 따라서 이렇게 구조체를 만든다.
		POOLED_LOCAL_VAR(ReliableDestInfoArray, destinfolist);

		HostID remote = rc->GetHostID();

		{
			CriticalSectionLock mainlock(GetCriticalSection(), true);

			// 각 수신 대상에게 브로드캐스트하기 위해, 수신 대상의 참조관계를 형성한다.
			// 즉 돌발 파괴를 없게 한다.
			for (int i = 0; i < relayCount; i++)
			{
				const RelayDest& dest = relayDest[i];

				ReliableDestInfo destinfo;
				if (TryGetAuthedClientByHostID(dest.m_sendTo, destinfo.sendToRC))
				{
					// remote가 가진 TCP socket 객체 변수가 다른 스레드에 의해 값이 바뀔 수 있다.
					// 이때 이 스레드는 기존 TCP socket을 액세스하다가 댕글링 하면 안된다.
					// 따라서 참조 관계를 미리 형성해서, 댕글링을 예방.
					destinfo.tcpSocket = destinfo.sendToRC->m_tcpLayer;
					destinfo.frameNumber = dest.m_frameNumber;
					destinfo.sendTo = dest.m_sendTo;
					destinfolist.Add(destinfo);
				}
			}
		} // main lock unlock

		// 어차피 send queue lock을 거는 것이라 contention이 짧긴 하지만 성능에 민감하고
		// 기존 코드가 이렇게 짜져 있으므로.
		LowContextSwitchingLoop<ReliableDestInfo, int>(destinfolist.GetData(), (int)destinfolist.GetCount(),
			//---GetCritSec function
			[](ReliableDestInfo& dest)
			{
				return &(dest.tcpSocket->GetSendQueueCriticalSection());
			},
			//---work function
				[this, encMode, remote, payloadLength, &payload, &decryptedOutput](ReliableDestInfo& dest, CriticalSectionLock& solock) ->bool
			{
				// send queue lock은 contention이 적지만 그래도 없지는 않다.
				// relay1은 서버에서 가장 많이 사용되는 것 중 하나다.
				// 따라서 성능을 최대한 올리기 위해 이렇게 굳이 low context switch loop를 돌도록 한다.
				CSmallStackAllocMessage header;
				Message_Write(header, MessageType_ReliableRelay2);
				header.Write(remote);				// sender
				header.Write(dest.frameNumber);	// frame number
#ifdef _DEBUG
				CMessageTestSplitterTemporaryDisabler dd(header);
#endif
				CSendFragRefs fragList;

				// 수신자가 WebSocket이거나 encMode가 None이면 payload를 전달
				if (encMode == EM_None || dest.tcpSocket->GetSocketType() == SocketType_WebSocket)
				{
					// NOTE: 앞서 말했듯이, encMode에 따라 payload or decryptedOutput중 택일되어 사용된다.
					header.WriteScalar(payloadLength);		// payload length

					fragList.Add(header);
					fragList.Add(payload);
				}
				else if (encMode == EM_Secure || encMode == EM_Fast)
				{
					// NOTE: 앞서 말했듯이, encMode에 따라 payload or decryptedOutput중 택일되어 사용된다.
					MessageType msgType;
					CMessage encryptedPayload;

					bool encryptOK = false;
					ErrorInfoPtr errorInfo;
					if (encMode == EM_Secure)
					{
						msgType = MessageType_Encrypted_Reliable_EM_Secure;
						encryptOK = CCryptoAes::EncryptMessage(dest.sendToRC->m_sessionKey->m_aesKey, decryptedOutput, encryptedPayload, 0);
					}
					else
					{
						msgType = MessageType_Encrypted_Reliable_EM_Fast;
						encryptOK = CCryptoFast::EncryptMessage(dest.sendToRC->m_sessionKey->m_fastKey, decryptedOutput, encryptedPayload, 0, errorInfo);
					}

					if (encryptOK) // 암호화 성공하면
					{
						// 보낼 메시지 만들고... 보내기 및 unlock은 아래에서 한다.
						fragList.Add(header);

						CSmallStackAllocMessage encHeader;
						encHeader.WriteScalar(sizeof(int8_t) + encryptedPayload.GetLength());
						encHeader.Write((int8_t)msgType);

						fragList.Add(encHeader);
						fragList.Add(encryptedPayload);
					}
					else // 실패하면
					{
						// true는 리턴해야지. 나가자.
						solock.Unlock();
						return true;
					}
				}

				// 암호화 성공하거나 건너뛰었다면 tcp send를 하고 
				dest.tcpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
					dest.tcpSocket,
					fragList, SendOpt(), m_simplePacketMode);
				// unlock 역시 하고
				solock.Unlock();

				// 나가자
				return true;
			}
			);
	}

	void CNetServerImpl::IoCompletion_ProcessMessage_LingerDataFrame1(CMessage& msg, const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 못 처리되었던 RUDP data frame들을 릴레이로 재송신하는 본 루틴은 의미가 없다.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		AssertIsNotLockedByCurrentThread();

		HostID destRemoteHostID; // 누구한테 릴레이할건지?
		int frameNumber = 0;
		int frameLength = 0;

		shared_ptr<CRemoteClient_S> destRC;
		shared_ptr<CSuperSocket> destRC_TcpSocket;
		CSendFragRefs sendData;
		CSmallStackAllocMessage header; // 아래 {} 안에 넣지 말 것. sendData에 의해 참조되므로.
		CMessage frameData; // 아래 {} 안에 넣지 말 것. sendData에 의해 참조되므로.

		CriticalSectionLock lock(GetCriticalSection(), true); // 아래에서 여기저기에서 main access를 하므로.

		if (!msg.Read(destRemoteHostID) ||
			!msg.Read(frameNumber) ||
			!msg.ReadScalar(frameLength))
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if (frameLength < 0 || frameLength >= m_settings.m_serverMessageMaxLength)
		{
			EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType_PacketRig);// 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		ASSERT_OR_HACKED(msg.GetReadOffset() + frameLength == msg.GetLength());
		if (!msg.ReadWithShareBuffer(frameData, frameLength))
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

		if (!TryGetAuthedClientByHostID(destRemoteHostID, destRC))
		{
			return;
		}

		// 미리 참조 관계를 형성해서 비파괴 보장을 한다.
		// 타 코드에서 m_tcpLayer를 assign하는 경우가 없으면 이럴 필요가 없었을 것이다.
		destRC_TcpSocket = destRC->m_tcpLayer;

		Message_Write(header, MessageType_LingerDataFrame2);
		header.Write(remote);
		header.Write(frameNumber);
#ifdef _DEBUG
		CMessageTestSplitterTemporaryDisabler dd(header);
#endif
		header.WriteScalar(frameLength);


		sendData.Add(header);
		sendData.Add(frameData);

		// 여기는 성능에 별 영향을 안 주는 곳이다. 따라서 main unlock을 해도 되지만 일부러 안한다.

		// TCP로 send.
		destRC_TcpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
			destRC_TcpSocket,
			sendData, SendOpt(), m_simplePacketMode);
	}


	void CNetServerImpl::IoCompletion_ProcessMessage_NotifyHolepunchSuccess(CMessage& msg, const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 처리 불가능.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket == nullptr)
		{
			return;
		}

		// caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
		AssertIsNotLockedByCurrentThread();
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// udp socket 객체가 없는데도 m_realUdpEnabled 가 true 설정 되는 문제가 있음
		// 또한, 해당 문제 때문에 ACR 과정중 m_udpSocket assert validation 체크 로직에서 abort 나는 현상 수정
		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket == nullptr)
		{
			return;
		}

		Guid magicNumber;
		AddrPort clientLocalAddr, clientAddrFromHere;
		if (msg.Read(magicNumber) == false ||
			msg.Read(clientLocalAddr) == false ||
			msg.Read(clientAddrFromHere) == false)
			return;

#ifdef _DEBUG
		NTTNTRACE("clientlocalAddr:%s\n", StringT2A(clientLocalAddr.ToString()).GetString());
#endif

		rc->AssertIsSocketNotLockedByCurrentThread();

		AssertIsLockedByCurrentThread();
		// 		CriticalSectionLock clk(GetCriticalSection(), true);
		// 		CHECK_CRITICALSECTION_DEADLOCK(this);

		// find relevant mature or unmature client by magic number
		if (rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber == magicNumber && rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled == false)
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
			Message_Write(sendMsg, MessageType_NotifyClientServerUdpMatched);
			sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);

			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
				rc->m_tcpLayer,
				CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);

			// 로그를 남긴다
			if (m_logWriter)
			{
				Tstringstream ss;
				ss << "Client " << rc->m_HostID <<
					": UDP hole-punch to server success. Client local addr=" << clientLocalAddr.ToString().GetString() <<
					", client addr at server=" << clientAddrFromHere.ToString().GetString();
				m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
			}
		}
	}


	void CNetServerImpl::IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(CMessage& msg, const shared_ptr<CRemoteClient_S>& rc)
	{
		// remote client가 없으면, 처리 불가능.
		if (!rc)
		{
			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		// caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
		AssertIsNotLockedByCurrentThread();
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		AddrPort clientLocalAddr, clientAddrFromHere;
		HostID remotePeerID;
		if (msg.Read(clientLocalAddr) == false ||
			msg.Read(clientAddrFromHere) == false ||
			msg.Read(remotePeerID) == false)
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

		shared_ptr<CRemoteClient_S> rc2 = GetAuthedClientByHostID_NOLOCK(remotePeerID);
		if (rc2 == nullptr)
			return;

		// 쌍방이 모두 홀펀칭 OK가 와야만 쌍방에게 P2P 시도를 지시.
		shared_ptr<P2PConnectionState> p2pConnState = m_p2pConnectionPairList.GetPair(remotePeerID, rc->m_HostID);

		//modify by rekfkno1 - //늦장 도착일 경우가 있다...가령 예를 들면,...
		//s->c로 leave,join을 연달아 보내는 경우... 클라에서는 미처 leave,join을 처리 하지 못한 상태에서
		//보낸 메시지 일수 있다는 이야기.
		if (p2pConnState == nullptr || p2pConnState->m_memberJoinAndAckInProgress)
			return;

		if (p2pConnState->IsRelayed() == false)
			return;

		int holepunchOkCountOld = p2pConnState->GetServerHolepunchOkCount();

		p2pConnState->SetServerHolepunchOk(rc->m_HostID, clientLocalAddr, clientAddrFromHere);
		if (p2pConnState->GetServerHolepunchOkCount() != 2 || holepunchOkCountOld != 1)
		{
			//NTTNTRACE("#### %d %d ####\n",p2pConnState->GetServerHolepunchOkCount(),holepunchOkCountOld);
			return;
		}

		assert(p2pConnState->m_magicNumber != Guid());
		// send to rc
		m_s2cProxy.RequestP2PHolepunch(rc->m_HostID, g_ReliableSendForPN,
			rc2->m_HostID,
			p2pConnState->GetInternalAddr(rc2->m_HostID),
			p2pConnState->GetExternalAddr(rc2->m_HostID),
			CompactFieldMap());

		// send to rc2
		m_s2cProxy.RequestP2PHolepunch(rc2->m_HostID, g_ReliableSendForPN,
			rc->m_HostID,
			p2pConnState->GetInternalAddr(rc->m_HostID),
			p2pConnState->GetExternalAddr(rc->m_HostID),
			CompactFieldMap());

		// 로그를 남긴다
		if (m_logWriter)
		{
			Tstringstream ss;
			ss << "P2P hole-punch between client " << rc->m_HostID <<
				" and client " << rc2->m_HostID <<
				" success: client " << rc->m_HostID <<
				" external addr=" << p2pConnState->GetExternalAddr(rc->m_HostID).ToString().GetString() <<
				", client " << rc2->m_HostID <<
				" external addr=" << p2pConnState->GetExternalAddr(rc2->m_HostID).ToString().GetString();

			m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
		}
	}

	// rc가 모든 direct p2p 연결을 잃어버렸을 때의 처리. 릴레이를 해야 함.
	// 이미 rc 넷클라는 자신이 p2p 연결을 잃어버린 것을 안다는 가정하에 콜 되는 함수.
	void CNetServerImpl::FallbackP2PToRelay(const shared_ptr<CRemoteClient_S>& rc, ErrorType reason)
	{
		for (CRemoteClient_S::P2PConnectionPairs::iterator i = rc->m_p2pConnectionPairs.begin(); i != rc->m_p2pConnectionPairs.end(); i++)
		{
			shared_ptr<P2PConnectionState> state = i.GetSecond();
			FallbackP2PToRelay(rc, state, reason);
		}
	}

	// rc와 state가 가리키는 rc의 peer간의 direct p2p 연결을 잃어버렸을 때의 처리. 릴레이를 해야 함.
	// 이미 rc 넷클라는 자신이 p2p 연결을 잃어버린 것을 안다는 가정하에 콜 되는 함수.
	void CNetServerImpl::FallbackP2PToRelay(const shared_ptr<CRemoteClient_S>& rc, shared_ptr<P2PConnectionState> state, ErrorType reason)
	{
		if (!state->IsRelayed())
		{
			state->SetRelayed(true);

			HostID remotePeerHostID;

			auto state_f = state->m_firstClient.lock();
			auto state_s = state->m_secondClient.lock();

			if (state_f && state_s)
			{
				if (state_f == rc)
					remotePeerHostID = state_s->m_HostID;
				else
					remotePeerHostID = state_f->m_HostID;

				// rc의 상대방 peer에게 p2p 연결 증발을 알린다.
				m_s2cProxy.P2P_NotifyDirectP2PDisconnected2(remotePeerHostID, g_ReliableSendForPN, rc->m_HostID, reason,
					CompactFieldMap());

				// 로그를 남긴다
				if (m_logWriter)
				{
					Tstringstream ss;
					ss << "P2P UDP hole-punching between Client " << rc->m_HostID <<
						" and Client %d " << remotePeerHostID <<
						" is disabled.";
					m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
				}
			}
		}
	}


	void CNetServerImpl::DestroyEmptyP2PGroups()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		for (P2PGroups_S::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); )
		{
			CP2PGroupPtr_S grp = i->GetSecond();
			if (grp->m_members.size() == 0)
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
	void CNetServerImpl::SetMaxDirectP2PConnectionCount(HostID clientID, int newVal)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CRemoteClient_S>  cl = GetAuthedClientByHostID_NOLOCK(clientID);
		if (cl)
		{
			cl->m_maxDirectP2PConnectionCount = max(newVal, 0);
		}
	}

	// memberHostID이 들어있는 AddMemberAckWaiter 객체를 목록에서 찾아서 모두 제거한다.
	void CNetServerImpl::AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(shared_ptr<CP2PGroup_S> group, HostID memberHostID, ErrorType reason)
	{
		CFastArray<int> delList;
		CFastSet<HostID> joiningMemberDelList;

		for (int i = (int)group->m_addMemberAckWaiters.GetCount() - 1; i >= 0; i--)
		{
			CP2PGroup_S::AddMemberAckWaiter& a = group->m_addMemberAckWaiters[i];
			if (a.m_joiningMemberHostID == memberHostID || a.m_oldMemberHostID == memberHostID)
			{
				delList.Add(i);

				// 제거된 항목이 추가완료대기를 기다리는 피어에 대한 것일테니
				// 추가완료대기중인 신규진입피어 목록만 따로 모아둔다.
				joiningMemberDelList.Add(a.m_joiningMemberHostID);
			}
		}

		for (int i = 0; i < (int)delList.GetCount(); i++)
		{
			group->m_addMemberAckWaiters.RemoveAndPullLast(delList[i]);
		}

		// memberHostID에 대한 OnP2PGroupJoinMemberAckComplete 대기에 대한 콜백에 대한 정리.
		// 중도 실패되어도 OnP2PGroupJoinMemberAckComplete 콜백을 되게 해주어야 하니까.
		for (CFastSet<HostID>::iterator i = joiningMemberDelList.begin(); i != joiningMemberDelList.end(); i++)
		{
			HostID joiningMember = *i;

			if (group->m_addMemberAckWaiters.AckWaitingItemExists(joiningMember) == false)
				EnqueAddMemberAckCompleteEvent(group->m_groupHostID, joiningMember, reason);
		}
	}


}
