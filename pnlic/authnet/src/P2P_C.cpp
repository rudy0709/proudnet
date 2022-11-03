#include "stdafx.h"
#include "NetClient.h"
#include "RmiContextImpl.h"
#include "LeanDynamicCast.h"
#include "SendFragRefs.h"

namespace Proud
{
	// 새로운 member가 들어와서 p2p group을 update한다.
	void CNetClientImpl::UpdateP2PGroup_MemberJoin(
		const HostID &groupHostID,
		const HostID &memberHostID,
		const ByteArray &customField,
		const uint32_t &eventID,
		const int &p2pFirstFrameNumber,
		const Guid &connectionMagicNumber,
		const ByteArray &p2pAESSessionKey,
		const ByteArray &p2pFastSessionKey,
		bool allowDirectP2P,
		bool pairRecycled)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 서버와 연결이 끊어지고 있는 상황이면 무시해야 한다.
		if (!m_worker || m_worker->GetState() != CNetClientWorker::Connected)
			return;

		// create or update P2P group
		// 이 그룹은 local이 소속된 그룹이기도 하다. 어차피 이 RMI는 local이 소속된 그룹에 대해서 호출되니까.
		CP2PGroupPtr_C GP = GetP2PGroupByHostID_Internal(groupHostID);
		if (GP == NULL)
			GP = CreateP2PGroupObject_INTERNAL(groupHostID);

		CRemotePeer_C *memberRP = NULL;

		// P2P 멤버가 서버인 경우도 있다.
		if (memberHostID == HostID_Server)
		{
			// 서버도 그룹에 추가한다.
			GP->m_members.Add(memberHostID, m_remoteServer);
		}
		else
		{
			// 자기 자신인 경우
			if (GetVolatileLocalHostID() == memberHostID)
			{
				GP->m_members.Add(memberHostID, this);
			}
			else
			{
				// 활성화 되어 있는 remote peer를 얻는다.
				memberRP = GetPeerByHostID_NOLOCK(memberHostID);
				// 없으면, 비활성화 되어 있는 remote peer를 얻는다.
				if (!memberRP)
				{
					memberRP = RemotePeerRecycles_Pop(memberHostID);
				}

				if (memberRP == NULL)
				{
					// 새 remote peer 생성
					memberRP = new CRemotePeer_C(this);
					memberRP->m_HostID = memberHostID;
					m_authedHostMap.Add(memberRP->m_HostID, memberRP);
				}
				else
				{
					// 아직 remote는 garbage에 있을 수 있다.
					// 거기에 있다는 것은, final user work item이 남아있는 상태라는 뜻.
					// 따라서 그걸 그대로 꺼내서 계속 사용해야 한다.
					// socket이었다면 못 살리는게 답이지만.
					UngarbageHost(memberRP);
				}

				// 재사용한 remote peer이면 다시 넣어야. 이미 있으면 말고.
				m_authedHostMap.Add(memberHostID, memberRP);

				// remote peer를 재사용 가능해도, 서버가 못했다고 하면, 릴레이로 빠꾸해야!
				if (!pairRecycled)
				{
					memberRP->SetRelayedP2P(true);
				}

				// 재사용하지 않는 상황이면 모든 값을 채우도록 하자.
				if (connectionMagicNumber != Guid())
				{
					memberRP->m_magicNumber = connectionMagicNumber;
				}
				
				// 이 값은 생성될 때만 적용해야지, 이미 RP가 있는 상태에서는 오버라이드하지 않는다. 
				// 이미 Direct P2P를 맺고있으면 그것이 우선이니까.
				memberRP->m_forceRelayP2P = !allowDirectP2P;

				// 이미 있으므로 불필요 => m_authedHostMap.Add(memberRP->m_HostID, memberRP);

				// 재사용하지 못했거나, direct P2P가 금지되어 있으면 릴레이로 전환
				// 재사용 중이고 directP2P가 금지 아니면 direct or relay 즉 종전 상태가 사용된다.
				if (!allowDirectP2P)
				{
					memberRP->SetRelayedP2P(true);
				}

				// P2P 통신용 암호키 세팅
				// 서버에서는 P2P 연결 재사용을 하거나, 암호화 옵션 끈 경우면 이것이 비어있을테니 그때는 패스.
				if (p2pAESSessionKey.GetCount() != 0)
				{
					if (CCryptoAes::ExpandFrom(memberRP->m_p2pSessionKey.m_aesKey,
						p2pAESSessionKey.GetData(),
						m_settings.m_encryptedMessageKeyLength / 8) != true)
					{
						throw Exception("Failed to create SessionKey");
					}
				}
				if (p2pFastSessionKey.GetCount() != 0)
				{
					if (CCryptoFast::ExpandFrom(memberRP->m_p2pSessionKey.m_fastKey,
						p2pFastSessionKey.GetData(),
						m_settings.m_fastEncryptedMessageKeyLength / 8) != true)
					{
						throw Exception("Failed to create SessionKey");
					}
				}

				if (p2pFirstFrameNumber)
				{
					// 서버에서 이걸 채웠다는 것은, 새 P2P pair가 만들어진 것이므로,
					// 재사용 혹은 새 remote peer에 대해 reset하라는 의미다.
					// 0인 경우는, 기존 P2P pair에 ref+1만 했을 뿐이므로, 
					// 다른 것 하지 말아야 한다.
					// NOTE: P2P join-leave를 반복하는 동안 피어간 reliable send는 보장될 필요가 없다.
					memberRP->m_ToPeerReliableUdp.ResetEngine(p2pFirstFrameNumber);
				}

				// update peer's joined groups
				memberRP->m_joinedP2PGroups.Add(GP->m_groupHostID, GP);
				GP->m_members.Add(memberHostID, memberRP);

			} // P2P 멤버가 타 클라인 경우

			// 사용자가 P2P member join-leave를 자주 반복할 경우, P2P keep alive 를 위한 ping을 할 타이밍을 heartbeat에서 자주 놓칠 수 있다.
			// 이를 예방하기 위해 ping on need를 하는 함수를 호출해 준다.
			P2PPingOnNeed();
		}

		// 자기 자신에 대한 것이건 (멤버로서의) 서버이건 타 클라에 대한 것이건, 
		// P2P-member-add-ack RMI with received event time.
		m_c2sProxy.P2PGroup_MemberJoin_Ack(HostID_Server,
			g_ReliableSendForPN,
			groupHostID,
			memberHostID,
			eventID);

		// enqueue event
		LocalEvent le;
		le.m_type = LocalEventType_AddMember;
		le.m_groupHostID = groupHostID;
		le.m_memberHostID = memberHostID;
		le.m_remoteHostID = memberHostID;
		le.m_memberCount = (int)GP->m_members.GetCount();

		// customField / sizeOfField 값을 로컬 이벤트에 채워준다.
		customField.CopyTo(le.m_customField);

		// local event는 해당 host에 넣어주어야 한다.
		// OnP2PMemberJoin or Leave 이벤트 후 해당 host에 대한 RMI가 줄줄이 호출되어야 하며 순서가 뒤바뀌면 안되니까.
		if (memberHostID == HostID_Server)
			EnqueLocalEvent(le, m_remoteServer);
		else if (memberHostID == GetVolatileLocalHostID())
			EnqueLocalEvent(le, m_loopbackHost);
		else
		{
			// 당연한 것을 체크
			assert(m_authedHostMap.ContainsKey(memberHostID));

			EnqueLocalEvent(le, memberRP);

			// remote peer가 재사용되었고 과거에도 direct P2P였다.
			// 처음에는 relayed 상태로 노티되기 때문에 곧장 direct로 바뀌었다는 이벤트를 넣어야 한다.
			LocalEvent e;
			e.m_type = LocalEventType_DirectP2PEnabled;
			e.m_remoteHostID = memberHostID;
			//m_owner->EnqueLocalEvent(e, m_owner->m_loopbackHost);
			EnqueLocalEvent(e, memberRP);
		}


	}

	DEFRMI_ProudS2C_P2PGroup_MemberLeave(CNetClientImpl::S2CStub)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 로그 남기기
		if (m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0)
		{
			m_owner->Log(0, LogCategory_P2P,
				String::NewFormat(_PNT("Received P2PGroup_MemberLeave : remote peer=%d, group=%d\n"),
				memberHostID,
				groupHostID));
		}

		// remote peer 및 P2P group 개체 찾기
		CRemotePeer_C* pMemberRC = m_owner->GetPeerByHostID_NOLOCK(memberHostID);
		CP2PGroupPtr_C pGroup = m_owner->GetP2PGroupByHostID_Internal(groupHostID);

		if (pGroup != NULL)
		{
			// 자신인 경우라도 그룹 객체에서 제거는 해야 한다.
			pGroup->m_members.Remove(memberHostID);
		}

		if (memberHostID == m_owner->GetVolatileLocalHostID()) // 자신인 경우
		{
			m_owner->m_P2PGroups.Remove(groupHostID);
		}

		// enqueue 'OnLeaveP2PGroup' event
		LocalEvent e;
		e.m_type = LocalEventType_DelMember;
		e.m_memberHostID = memberHostID;
		e.m_remoteHostID = memberHostID;
		e.m_groupHostID = groupHostID;

		// P2P group이 이미 없을 수도 있다. 방어 코딩.
		if (pGroup != NULL)
		{
			e.m_memberCount = (int)pGroup->m_members.GetCount();
		}
		else
		{
			e.m_memberCount = 0;
		}

		if (memberHostID == HostID_Server)
		{
			// 서버
			m_owner->EnqueLocalEvent(e, m_owner->m_remoteServer);
		}
		else if (memberHostID == m_owner->GetVolatileLocalHostID())
		{
			// 로컬 host
			m_owner->EnqueLocalEvent(e, m_owner->m_loopbackHost);
		}
		else
		{
			// remote peer를 찾아, 있으면 제거 처리를 수행.
			// Q: remote peer에 대해서 leave 처리를 하는데 이미 remote peer가 사라질 수도 있나요?
			// A: 예. NetClient가 종료중이면 remote peer들이 garbaged 상태일테니.
			if (pMemberRC)
			{
				// P2P member leave 이벤트를 enqueue하고 나서 garbageHost를 한다.
				// 이래야 사용자는 P2P member leave 이벤트를 수신한 후 remote peer가 비로소 파괴된다.
				m_owner->EnqueLocalEvent(e, pMemberRC);

				pMemberRC->m_joinedP2PGroups.Remove(groupHostID);

				// 무턱대고 지우는게 아니라, local<->remote를 포함하는 P2P group의 갯수가 0일때만 지운다.
				m_owner->RemoveRemotePeerIfNoGroupRelationDetected(pMemberRC);
			}
		}

		return true;
	}

	void CNetClientImpl::P2PPingOnNeed()	// ReliablePing 과 UnreliablePing 이 같이 공존함
	{
		LockMain_AssertIsLockedByCurrentThread();
		int64_t currTime = GetPreciseCurrentTimeMs();
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(i->GetSecond());
			if (remotePeer != NULL && !remotePeer->m_garbaged && remotePeer->m_HostID != HostID_Server)
			{
				if (remotePeer->m_ReliablePingDiffTimeMs - currTime <= 0)
				{
					int64_t t0 = remotePeer->m_ReliablePingDiffTimeMs;
					assert(CNetConfig::ReliablePingIntervalMs > 0);
					int64_t t1 = currTime + CNetConfig::ReliablePingIntervalMs;
					remotePeer->m_ReliablePingDiffTimeMs = t1;

					Log(0, LogCategory_P2P, String::NewFormat(_PNT("P2P reliable ping diff time changed from %I64d to %I64d.\n"), t0, t1));

					// reliableudp ping을 보낸다. ( relay이건 directp2p 이건 상관없이 보낸다. )
					// unreliable ping만 으로는 p2p간 패킷 유실율과 같이쓰기가 애매하기 때문
					CMessage header2; header2.UseInternalBuffer();
					header2.Write((char)MessageType_P2PReliablePing);
					header2.Write(currTime);
					// ReportServerTimeAndFrameRateAndPing/Pong 이 사라지면서 추가된 정보.
					header2.Write(m_applicationHint.m_recentFrameRate);
					header2.Write(m_serverUdpRecentPingMs);

					CSendFragRefs sd2(header2);
					SendOpt opt(g_ReliableSendForPN);
					opt.m_priority = MessagePriority_Ring0;
					Send_BroadcastLayer(sd2, NULL, opt, &remotePeer->m_HostID, 1); // reliable ping은, direct P2P가 아니면 릴레이를 타서라도 측정되어야 한다. 그래서 이 함수를 썼다.
				}

				if (remotePeer->m_UnreliablePingDiffTimeMs - currTime <= 0)
				{
					assert(CNetConfig::UnreliablePingIntervalMs > 0);
					remotePeer->m_UnreliablePingDiffTimeMs = currTime + CNetConfig::UnreliablePingIntervalMs;

					if (!remotePeer->IsRelayedP2P())
					{
						/* peer가 가진 server time을 다른 peer에게 전송한다. 즉 간접 서버 시간을 동기화하고자 한다.

						일정 시간마다 각 peer에게 P2P_SyncIndirectServerTime(서버에 의해 동기화된 시간, 랙, 프레임 레이트)을 보낸다.
						이걸 받은 상대는 해당 peer 기준으로의 time diff 값을 갖고 있는다. 모든 peer로부터
						이값을 받으면 그리고 peer가 속한 각 p2p group 범위 내에서의 time diff 평균값을 계산한다.

						또한 이 메시지는 P2P 간 keep alive check를 하는 용도로도 쓰인다. */
						CMessage header; header.UseInternalBuffer();
						header.Write((char)MessageType_P2PUnreliablePing);
						header.Write(currTime);

						CSendFragRefs sd(header);
						remotePeer->m_ToPeerUdp.SendWithSplitter_Copy(sd, SendOpt(MessagePriority_Ring0, true));
					}
					else
					{
						// peer와 relayed 통신을 하는 경우.
						// 이미 보내서 받은것처럼 fake한다.
						int64_t oktime = currTime - remotePeer->m_lastDirectUdpPacketReceivedTimeMs;
						if (oktime > 0)
							remotePeer->m_lastUdpPacketReceivedInterval = oktime;
						remotePeer->m_lastDirectUdpPacketReceivedTimeMs = currTime;
						remotePeer->m_directUdpPacketReceiveCount++;

						remotePeer->m_indirectServerTimeDiff = 0;
						remotePeer->m_lastPingMs = m_serverUdpRecentPingMs + remotePeer->m_peerToServerPingMs; // relay의 경로(A->서버->B)의 랙을 합산한다.
						remotePeer->m_lastPingMs = max(remotePeer->m_lastPingMs, 1);

						if (remotePeer->m_p2pPingChecked == false)
							remotePeer->m_p2pPingChecked = true;

						if (remotePeer->m_setToRelayedButLastPingIsNotCalulcatedYet)
						{
							remotePeer->m_recentPingMs = 0;
							remotePeer->m_setToRelayedButLastPingIsNotCalulcatedYet = false;
						}

						if (remotePeer->m_recentPingMs > 0)
							remotePeer->m_recentPingMs = LerpInt(remotePeer->m_recentPingMs, remotePeer->m_lastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
						else
							remotePeer->m_recentPingMs = remotePeer->m_lastPingMs;

						remotePeer->m_recentPingMs = max(remotePeer->m_recentPingMs, 1);
					}
				}
			}
		}
	}


}