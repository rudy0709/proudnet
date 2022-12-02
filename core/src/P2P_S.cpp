#include "stdafx.h"
#include "NetServer.h"
#include "ReliableUdpHelper.h"
#include "RmiContextImpl.h"
#include "LeanDynamicCast.h"

namespace Proud 
{
	bool CNetServerImpl::JoinP2PGroup_Internal(HostID newMemberHostID,
		HostID groupHostID,
		ByteArray message,
		int joinP2PGroupKeyGen)
	{
		AssertIsLockedByCurrentThread();

		// 그룹 유효성 체크
		CP2PGroupPtr_S g = GetP2PGroupByHostID_NOLOCK(groupHostID);
		if (g == NULL)
			return false;

		uint32_t ackKeySerial = joinP2PGroupKeyGen;

		// 클라 유효성 체크
		P2PGroupMember_S newMember_temp;
		shared_ptr<CP2PGroupMemberBase_S> newMember;

		P2PGroupMembers_S::iterator it = g->m_members.find(newMemberHostID);
		if (it != g->m_members.end())
		{
			// 이미 그룹내에 들어가 있다고 이벤트를 한다.
			EnqueAddMemberAckCompleteEvent(groupHostID, newMemberHostID, ErrorType_AlreadyExists);
			return true;
		}

		// 새로 넣으려는 멤버가 유효한 값인지?
		if (m_settings.m_allowServerAsP2PGroupMember && newMemberHostID == HostID_Server)
			newMember = m_loopbackHost;
		else
		{
			shared_ptr<CRemoteClient_S>  m = GetAuthedClientByHostID_NOLOCK(newMemberHostID);
			if (m == NULL || m->m_garbaged)
				return false;
			newMember = m;
		}

		int64_t currTime = GetPreciseCurrentTimeMs();

		bool addMemberAckWaiterDone = false;

		// 그룹데이터 업데이트
		newMember_temp.m_ptr = newMember;
		newMember_temp.m_joinedTime = currTime;
		g->m_members.insert(P2PGroupMembers_S::value_type(newMemberHostID, newMember_temp));

		// 그룹원도 자신이 속한 그룹들을 인지하고 있는다.
		JoinedP2PGroupInfo ginfo;
		ginfo.m_groupPtr = g;
		newMember->m_joinedP2PGroups.Add(groupHostID, ginfo);

		// add ack-waiters(with event time) for current members to new member
		for (P2PGroupMembers_S::iterator iOldMember = g->m_members.begin(); iOldMember != g->m_members.end(); ++iOldMember)
		{
			shared_ptr<CP2PGroupMemberBase_S> oldMember = iOldMember->second.m_ptr.lock();  // 기존 그룹의 각 멤버

			if(oldMember)
			{
				auto pThis = m_loopbackHost;
				if (oldMember != pThis)	// 서버로부터 join ack 메시지 수신은 없으므로
				{
					// 기존 멤버들에게 새로운 멤버를 추가시키고 ack를 받아야 한다.
					CP2PGroup_S::AddMemberAckWaiter ackWaiter;
					ackWaiter.m_joiningMemberHostID = newMemberHostID;
					ackWaiter.m_oldMemberHostID = oldMember->GetHostID();
					ackWaiter.m_eventID = ackKeySerial;
					ackWaiter.m_eventTime = currTime;
					g->m_addMemberAckWaiters.Add(ackWaiter);

					addMemberAckWaiterDone = true;
				}

				// 새 멤버로부터 기 멤버에 대한 join ack들이 일괄 와야 하므로 이것도 추가해야.
				// 단, 서버로부터는 join ack가 안오므로 제낀다.
				if (newMember != m_loopbackHost && newMember != oldMember)
				{
					// 새 멤버는 기존 멤버들을 추가시키고 ack를 받아야 한다.
					CP2PGroup_S::AddMemberAckWaiter ackWaiter;
					ackWaiter.m_joiningMemberHostID = oldMember->GetHostID();
					ackWaiter.m_oldMemberHostID = newMemberHostID;
					ackWaiter.m_eventID = ackKeySerial;
					ackWaiter.m_eventTime = currTime;
					g->m_addMemberAckWaiters.Add(ackWaiter);

					addMemberAckWaiterDone = true;
				}

				// 아래 RMI 보낼때 사용될 변수들
				ByteArray emptyData;
				RmiContext* pRmiContext = &g_ReliableSendForPN;
				ByteArray* aesSessionKey = &emptyData;
				ByteArray* fastSessionKey = &emptyData;
				bool pairRecycled; // 이게 false이면 클라는 기존 재사용 가능한 remote peer를 강제로 릴레이 타게 만든다. 홀펀칭 정보 재사용 안함.
				RuntimePlatform runtimePlatform = RuntimePlatform_Last;

				int firstFrameNumber = 0; // 0 이외의 값이 들어가면 클라는 reset하라는 뜻.
				Guid magicNumber = Guid();

				// P2P connection pair를 갱신
				P2PConnectionStatePtr p2pstate;
				if (oldMember->GetHostID() != HostID_Server && newMember->GetHostID() != HostID_Server)
				{
					shared_ptr<CRemoteClient_S>  oldMemberAsRC = static_pointer_cast<CRemoteClient_S>(oldMember);
					shared_ptr<CRemoteClient_S>  newMemberAsRC = static_pointer_cast<CRemoteClient_S>(newMember);

					runtimePlatform = newMemberAsRC->m_runtimePlatform;

					p2pstate = m_p2pConnectionPairList.GetPair(oldMemberAsRC->m_HostID, newMemberAsRC->m_HostID);
					if (p2pstate != NULL)
					{
						// 이미 통신중이었다면 그냥 카운트만 증가시킨다.
						p2pstate->m_dupCount++;

						// RMI로 보내는 데이터는 암호화하지 말고 그냥 평범한 데이터를 보낸다.
						pairRecycled = true;
					}
					else
					{
						// 최초의 피어간 연결이다. 과거 해제된 연결을 재사용하거나 신규 연결을 만든다.

						// P2P 연결을 재사용 가능한지?
						p2pstate = m_p2pConnectionPairList.RecyclePair_Pop(oldMemberAsRC->m_HostID, newMemberAsRC->m_HostID);
						if (p2pstate == NULL || p2pstate->IsRelayed())
						{
							// 재사용 안되니, 새로 만들자.
							// 새로 만드는 것이면 세션키도 새로 만들고, 홀펀칭도 없으므로 새로 시작.
							p2pstate = P2PConnectionStatePtr(new P2PConnectionState(this, oldMemberAsRC->m_HostID == newMemberAsRC->m_HostID));
							p2pstate->SetRelayed(true);
							pairRecycled = false;

						}
						else
						{
							// 재사용 성공. 
							// 홀펀칭은 재사용가능하거나 아닐 수 있고, 세션키는 반드시 재사용된다.
							pairRecycled = true;
						}

						// P2P 간 통신도 암호화 기능이 켜져있으면 서로간 통신을 위한 세션키를 만들어 제공한다.
						// 단, pair가 재사용되는 경우에는 예외.
						if (m_settings.m_enableP2PEncryptedMessaging
							&& !pairRecycled)
						{
							if (CCryptoRsa::CreateRandomBlock(p2pstate->m_p2pAESSessionKey,
								m_settings.m_encryptedMessageKeyLength) != true
								|| CCryptoRsa::CreateRandomBlock(p2pstate->m_p2pFastSessionKey,
									m_settings.m_fastEncryptedMessageKeyLength) != true)
							{
								if (m_logWriter)
								{
									Tstringstream ss;
									ss << "Making P2P session key for " << oldMember->GetHostID() <<
										" and " << newMember->GetHostID() <<
										" is failed!";
									m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
								}

								p2pstate->m_p2pAESSessionKey = emptyData;
								p2pstate->m_p2pFastSessionKey = emptyData;
							}
						}

						// NOTE: P2P pair 및 remote peer가 재사용될 때는, P2P용 세션키와 홀펀칭 정보가 재사용되고
						// reliable UDP data frame number는 보관 안된다.

						// reliable UDP를 위한 프레임 넘버
						// P2P pair를 재사용하더라도 이것은 새로 세팅한다. 피어간 미처리된 data-ack 즉 sender window는 다시 보낼 의무가 없다.
						p2pstate->m_p2pFirstFrameNumber = CRemotePeerReliableUdpHelper::GetRandomFrameNumber(m_random, m_simplePacketMode);

						// 다른 P2P 홀펀칭과 뒤섞여 오인하는 것을 방지
						// P2P pair를 재사용할 경우 기존 값 사용
						if (pairRecycled)
							assert(p2pstate->m_magicNumber != Guid());
						else
							p2pstate->m_magicNumber = m_random.NextGuid();

						// 재사용 혹은 새 pair이므로.
						p2pstate->m_dupCount = 1;

						// pair를 보관한다.
						// NOTE: recycle pair에서는 이미 위에서 뺐다.
						m_p2pConnectionPairList.AddPair(oldMemberAsRC, newMemberAsRC, p2pstate);

						// P2P 연결 정보 초기화
						if (!pairRecycled)
							p2pstate->MemberJoin_Start(oldMemberAsRC->m_HostID, newMemberAsRC->m_HostID);

						// 각 RC에도 pair를 넣어주어야.
						oldMemberAsRC->m_p2pConnectionPairs.Add(p2pstate.get(), p2pstate);
						newMemberAsRC->m_p2pConnectionPairs.Add(p2pstate.get(), p2pstate);

						if (m_logWriter)
						{
							Tstringstream ss;
							ss << "Preparing P2P pair between " << oldMember->GetHostID() << " and " << newMember->GetHostID();
							m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
						}

						AssertIsLockedByCurrentThread(); // 이게 있어야 받는 쪽에서는 member join noti를 받은 후에야 P2P RMI(relayed)를 받는 순서가 보장된다.

						// 내부 처리 끝. 이제 각 클라들에게 알려줄 데이터를 준비.

						// 새 pair를 만든 경우이고 암호키를 발급해 주는 것이면
						if (m_settings.m_enableP2PEncryptedMessaging && !pairRecycled)
						{
							pRmiContext = &g_SecureReliableSendForPN;
							aesSessionKey = &p2pstate->m_p2pAESSessionKey;
							fastSessionKey = &p2pstate->m_p2pFastSessionKey;
						}
						magicNumber = p2pstate->m_magicNumber;

						// reliable UDP용 frame number는 항상 새로 발급된다.
						firstFrameNumber = p2pstate->m_p2pFirstFrameNumber;

						// P2P 재사용되는 경우 양 클라는 이미 가진 값을 괜히 부하 걸리게 암호화해서 보내지 않는다.
						if (pairRecycled)
						{
							aesSessionKey = &emptyData;
							fastSessionKey = &emptyData;
							magicNumber = Guid();
							pRmiContext = &g_ReliableSendForPN;
						}

						if (m_logWriter && p2pstate)
						{
							Tstringstream ss;
							ss << "A magic number is given by server: " << p2pstate->m_magicNumber.ToString().GetString();
							m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
						}
					} // 새 pair 만들기
				} // P2P 멤버가 remote client인 경우
				else // P2P 멤버가 서버인 경우
				{
					// member join을 클라에게 알리는 정보가 대부분 무의미하다. 
					// custom field, ack key serial 등 몇개만 빼고.
					pairRecycled = false;

				}

				CompactFieldMap fieldMap;
				fieldMap.SetField(FieldType_RuntimePlatform, runtimePlatform);

				// added member가 서버인 경우도 클라에게 알려주어야 하므로 이것을 실행
				if (oldMember != m_loopbackHost)
				{
					// C/S 홀펀징에서 얻은 RTT를 보낸다.
					shared_ptr<CRemoteClient_S>  m = GetAuthedClientByHostID_NOLOCK(oldMember->GetHostID());
					int reliableRTT = 0;
					int unreliableRTT = 0;

					if (m)
					{
						reliableRTT = m->m_lastReliablePingMs;
						unreliableRTT = m->m_lastPingMs;
					}

					// P2P_AddMember RMI with heterogeneous session keys and event time
					// session key와 1st frame 번호를 보내는 것이 필요한 이유:
					// 릴레이 모드에서도 어쨌거나 frame number와 보안 통신이 필요하니.
					m_s2cProxy.P2PGroup_MemberJoin(oldMember->GetHostID(), *pRmiContext,
						g->m_groupHostID,
						newMemberHostID,
						message,
						ackKeySerial,
						*aesSessionKey,
						*fastSessionKey,
						firstFrameNumber,
						magicNumber,
						g->m_option.m_enableDirectP2P,
						pairRecycled,
						reliableRTT,
						unreliableRTT,
						fieldMap);
				}

				// 자기 자신에 대해서의 경우 이미 한번은 위 라인에서 보냈으므로
				// 이번에는 또 보내지는 않는다.
				if (oldMember != newMember && newMemberHostID != HostID_Server)
				{
					// C/S 홀펀징에서 얻은 RTT를 보낸다.
					shared_ptr<CRemoteClient_S>  m = GetAuthedClientByHostID_NOLOCK(newMemberHostID);
					int reliableRTT = 0;
					int unreliableRTT = 0;

					if (m)
					{
						reliableRTT = m->m_lastReliablePingMs;
						unreliableRTT = m->m_lastPingMs;
					}

					m_s2cProxy.P2PGroup_MemberJoin(newMemberHostID, *pRmiContext,
						g->m_groupHostID,
						oldMember->GetHostID(),
						message,
						ackKeySerial,
						*aesSessionKey,
						*fastSessionKey,
						firstFrameNumber,
						magicNumber,
						g->m_option.m_enableDirectP2P,
						pairRecycled,
						reliableRTT,
						unreliableRTT,
						fieldMap);
				}
			}

		}

		P2PGroup_RefreshMostSuperPeerSuitableClientID(g);
		P2PGroup_CheckConsistency();

		// 성공적으로 멤버를 넣었으나 ack waiter가 추가되지 않은 경우(예: 빈 그룹에 서버 1개만 추가) 바로 완료 콜백을 때린다.
		if (!addMemberAckWaiterDone)
		{
			EnqueAddMemberAckCompleteEvent(groupHostID, HostID_Server, ErrorType_Ok);
		}
		return true;
	}

	DEFRMI_ProudC2S_P2PGroup_MemberJoin_Ack(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// remove from group's relevant ack-waiter
		CP2PGroupPtr_S g = m_owner->GetP2PGroupByHostID_NOLOCK(groupHostID);
		if (g != NULL)
		{
			// 제대로 지운 다음...
			// ack-waiter for new member became empty?
			if (g->m_addMemberAckWaiters.RemoveEqualItem(remote, addedMemberHostID, eventID) == true &&
				g->m_addMemberAckWaiters.AckWaitingItemExists(addedMemberHostID, eventID) == false)
			{
				m_owner->EnqueAddMemberAckCompleteEvent(groupHostID, addedMemberHostID, ErrorType_Ok);
			}
		}
		else
		{
#ifdef _DEBUG
			// 최근에 destroy한 P2P 그룹인데 뒤늦게 이게 온거면, 경고를 내지 않는다.
			if (!m_owner->m_HostIDFactory->IsDroppedRecently(groupHostID))
			{
				NTTNTRACE("Warning: %d의 OnP2PMemberJoin(A=%d)에 대한 ack가 왔지만, 서버에서 애당초 OnP2PMemberJoin(A)를 보낸 바 없습니다.\n", remote, addedMemberHostID);
			}
#endif
		}

		// 두 피어의 연결 정보를 얻는다. 
		P2PConnectionStatePtr state = m_owner->m_p2pConnectionPairList.GetPair(remote, addedMemberHostID);

		if (state == NULL)
		{
			/* 서버에서 P2P member join,leave를 짧은 시간에 반복하면,
			이미 P2P connection state 객체 자체가 없어졌는데,
			뒤늦게 앞서 join에 대한 것이 도착할 수도 있다.
			이는 정상적인 상황이므로 경고를 보여주거나 하지 말고 조용하게 무시해 버리도록 하자. */
			return true;
		}



		// 양 클라는 P2P용 UDP는 닫지 않고 일정 시간 냅두며, 서버도 이를 안다.
		// 그래서 서버는 같은 피어간 P2P가 짧은 시간(수십초)안에 재개될 때 홀펀칭을 생략하고 바로 통신한다.
		// 그러나 예상치 못한 상황 가령 
		if (state->m_memberJoinAndAckInProgress)
		{
			state->MemberJoin_Acked(remote);

			if (state->MemberJoin_IsAckedCompletely())
			{
				// 양쪽의 ack가 다 왔으므로 상태 갱신
				state->m_releaseTimeMs = 0;
				state->m_memberJoinAndAckInProgress = false;
			}
		}

		//printf("P2PGroup_MemberJoin_Ack: %d->%d\n",addedMemberHostID,remote);
		return true;
	}

	bool CNetServerImpl::LeaveP2PGroup(HostID memberHostID, HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		// 체크
		CP2PGroupPtr_S g = GetP2PGroupByHostID_NOLOCK(groupHostID);
		if (g == NULL)
			return false;

		shared_ptr<CP2PGroupMemberBase_S> memberRC;
		P2PGroupMembers_S::iterator it = g->m_members.find(memberHostID);
		if (it == g->m_members.end())
			return false;

		// 유효한 객체인지 체크
		// NOTE: 서버도 P2P 멤버가 될 수 있다.
		if (memberHostID == HostID_Server)
			memberRC = m_loopbackHost;
		else
		{
			memberRC = GetAuthedClientByHostID_NOLOCK(memberHostID);
			if (memberRC == NULL)
				return false;
		}

		// notify P2PGroup_MemberLeave to related members
		// notify it to the banished member
		for (P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
		{
			if (i->first != HostID_Server)
			{
				m_s2cProxy.P2PGroup_MemberLeave(i->first, g_ReliableSendForPN, memberHostID, groupHostID,
					CompactFieldMap());
			}
			if (memberHostID != HostID_Server && memberHostID != i->first)
			{
				m_s2cProxy.P2PGroup_MemberLeave(memberHostID, g_ReliableSendForPN, i->first, groupHostID,
					CompactFieldMap());
			}
		}

		// P2P 연결 쌍 리스트에서도 제거하되 중복 카운트를 감안한다.
		if (memberHostID != HostID_Server)
		{
			for (P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
			{
				auto groupMemberPtr = i->second.m_ptr.lock();
				if(groupMemberPtr)
				{
					if (groupMemberPtr->GetHostID() != HostID_Server && memberRC->GetHostID() != HostID_Server)
					{
						m_p2pConnectionPairList.ReleasePair(
							this,
							static_pointer_cast<CRemoteClient_S>(memberRC),
							static_pointer_cast<CRemoteClient_S>(groupMemberPtr));
					}
				}
			}
		}

		// 멤버 목록에서 삭제
		g->m_members.erase(memberHostID);
		memberRC->m_joinedP2PGroups.Remove(groupHostID);

		// remove from every P2PGroup's add-member-ack list
		AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(g, memberHostID, ErrorType_UserRequested);

		// 그룹을 파괴하던지 재정비하던지, 옵션에 따라.
		if (g->m_members.size() == 0 && !m_allowEmptyP2PGroup)
		{
			// 그룹을 파괴
			HostID bak = g->m_groupHostID;
			if (m_P2PGroups.Remove(bak))
			{
				//hostid를 drop한다. 재사용을 위해.
				m_HostIDFactory->Drop(bak);
				EnqueueP2PGroupRemoveEvent(bak);
			}
		}
		else
		{
			// 멤버가 모두 나갔어도 P2P group을 파괴하지는 않는다. 이는 명시적으로 파괴되는 것이 정책이다. 
			P2PGroup_RefreshMostSuperPeerSuitableClientID(g);
			P2PGroup_CheckConsistency();
		}

		return true;
	}

	int CNetServerImpl::GetP2PRecentPingMs(HostID HostA, HostID HostB)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(HostA);
		if (rc != nullptr)
		{
			for (CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin(); iPair != rc->m_p2pConnectionPairs.end(); iPair++)
			{
				shared_ptr<P2PConnectionState> pair = iPair.GetSecond();
				if (pair->ContainsHostID(HostB))
				{
					if (pair->IsRelayed())
					{
						shared_ptr<CRemoteClient_S> rc2 = pair->m_firstClient.lock();
						if (rc2 == rc)
							rc2 = pair->m_secondClient.lock();

						if (rc && rc2)
							return rc->m_recentPingMs + rc2->m_recentPingMs;
						else
							return 0;
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
		if (g == nullptr)
			return false;

		while (g->m_members.size() > 0)
		{
			LeaveP2PGroup((g->m_members.begin())->first, g->m_groupHostID);
		}

		// 다 끝났다. 이제 P2P 그룹 자체를 파괴해버린다. 
		if (m_P2PGroups.Remove(groupHostID))
		{
			//add by rekfkno1 - hostid를 drop한다.재사용을 위해.
			m_HostIDFactory->Drop(groupHostID);
			EnqueueP2PGroupRemoveEvent(groupHostID);
		}

		return true;
	}

	HostID CNetServerImpl::CreateP2PGroup(const HostID* clientHostIDList0,
		int count, ByteArray message,
		CP2PGroupOption &option,
		HostID assignedHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// 빈 그룹도 만드는 것을 허용한다. (옵션에 따라)
		if (count < 0 || m_HostIDFactory == nullptr)
			return HostID_None;

		if (!m_allowEmptyP2PGroup && count == 0)
			return HostID_None;

		CFastArray<HostID, false, true, int> clientHostIDList;

		if (count > 0)
		{
			// 클라 목록에서 중복 항목을 제거한다.
			clientHostIDList.CopyFrom(clientHostIDList0, count);
			UnionDuplicates<CFastArray<HostID, false, true, int>, HostID, int>(clientHostIDList);
			count = (int)clientHostIDList.GetCount();

			// 클라이언트 유효성 체크
			for (int i = 0; i < count; i++)
			{
				// 2009.11.02 add by ulelio : Server인경우 클라이언트가 아니기때문에 유효성 체크 넘어감
				if (clientHostIDList[i] == HostID_Server)
					continue;

				shared_ptr<CRemoteClient_S> peer = GetAuthedClientByHostID_NOLOCK(clientHostIDList[i]);
				if (peer == nullptr)
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
		if (HostID_None == groupHostID)
			return HostID_None;

		CP2PGroupPtr_S NG(new CP2PGroup_S());
		NG->m_groupHostID = groupHostID;
		m_P2PGroups.Add(NG->m_groupHostID, NG);
		NG->m_option = option;


		m_joinP2PGroupKeyGen++;

		// NOTE: HostID_Server에 대한 OnP2PGroupJoinMemberAckComplete 이벤트가 2번 발생되는 문제때문에 for문을 역순으로 돌리게하였습니다.
		bool vaildGroup = false;
		for (int ee = count - 1; ee >= 0; ee--)
		{
			vaildGroup |= JoinP2PGroup_Internal(clientHostIDList[ee], NG->m_groupHostID, message, m_joinP2PGroupKeyGen);
		}

		if (m_allowEmptyP2PGroup == false && vaildGroup == false)
		{
			// 그룹멤버가 모두 유효하지 않은 경우.
			// 별도 처리를 해야 하는지 여부는 파악 중.
		}

		return NG->m_groupHostID;
	}

	bool CNetServerImpl::JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray message)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);
		return JoinP2PGroup_Internal(memberHostID, groupHostID, message, ++m_joinP2PGroupKeyGen);
	}



}
