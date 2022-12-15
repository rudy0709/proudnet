#include "stdafx.h"
#include "NetClient.h"
#include "RmiContextImpl.h"
#include "LeanDynamicCast.h"
#include "SendFragRefs.h"
#include "quicksort.h"
#include "../include/numutil.h"

namespace Proud
{
	// 새로운 member가 들어와서 p2p group을 update한다.
	void CNetClientImpl::UpdateP2PGroup_MemberJoin(
		const HostID &groupHostID,
		const HostID &memberHostID,
		const ByteArray &message,
		const uint32_t &eventID,
		const int &p2pFirstFrameNumber,
		const Guid &connectionMagicNumber,
		const ByteArray &p2pAESSessionKey,
		const ByteArray &p2pFastSessionKey,
		bool allowDirectP2P,
		bool pairRecycled,
		int reliableRTT,
		int unreliableRTT,
		const RuntimePlatform &runtimePlatform)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 서버와 연결이 끊어지고 있는 상황이면 무시해야 한다.
		if (m_worker->GetState() != CNetClientWorker::Connected)
			return;

		// create or update P2P group
		// 이 그룹은 local이 소속된 그룹이기도 하다. 어차피 이 RMI는 local이 소속된 그룹에 대해서 호출되니까.
		CP2PGroupPtr_C GP = GetP2PGroupByHostID_Internal(groupHostID);
		if (GP == nullptr)
			GP = CreateP2PGroupObject_INTERNAL(groupHostID);

		shared_ptr<CRemotePeer_C> memberRP;

		bool remotePeerIsRecycled = false;

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
				GP->m_members.Add(memberHostID, m_loopbackHost);
			}
			else
			{
				// 활성화 되어 있는 remote peer를 얻는다.
				memberRP = GetPeerByHostID_NOLOCK(memberHostID);
				// 없으면, 비활성화 되어 있는 remote peer를 얻는다.
				if (!memberRP)
				{
					memberRP = RemotePeerRecycles_Pop(memberHostID);
					if (memberRP)
					{
						// 뒤에서, 홀펀칭 재사용된 경우 즉시 OnChangeP2PRelayState 콜백하도록 하자.
						// 과거 홀펀칭이 살아있었으면.
						remotePeerIsRecycled = true;
					}
				}

				if (memberRP == nullptr)
				{
					// 새 remote peer 생성
					memberRP = shared_ptr<CRemotePeer_C>(new CRemotePeer_C(this));
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
					if (CCryptoAes::ExpandFrom(memberRP->m_p2pSessionKey->m_aesKey,
						p2pAESSessionKey.GetData(),
						m_settings.m_encryptedMessageKeyLength / 8) != true)
					{
						throw Exception("Failed to create SessionKey");
					}
				}
				if (p2pFastSessionKey.GetCount() != 0)
				{
					if (CCryptoFast::ExpandFrom(memberRP->m_p2pSessionKey->m_fastKey,
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

				// 홀펀칭이 안되어 있을때의 RTT를 계산하여 넣는다. 클라-서버-클라2 레이턴시 합산이다.
				// 단, 과거 홀펀칭이 되어 있었던 것을 재사용하는 경우, 갱신하지 않는다.
				if (memberRP->IsRelayedP2P())
				{
					if (memberRP->m_lastReliablePingMs == 0)
						memberRP->m_lastReliablePingMs = m_serverTcpLastPingMs + reliableRTT;

					if (memberRP->m_lastPingMs == 0)
						memberRP->m_lastPingMs = m_serverUdpLastPingMs + unreliableRTT;

					if (memberRP->m_peerToServerPingMs == 0)
						memberRP->m_peerToServerPingMs = unreliableRTT;
				}

				// update peer's joined groups
				memberRP->m_joinedP2PGroups.Add(GP->m_groupHostID, GP);
				GP->m_members.Add(memberHostID, memberRP);

				// remote peer가 홀펀칭을 유지하고 있다면 
				if(!memberRP->IsRelayedP2P())
				{
					// last received time을 현재 시간으로 바꾸도록 한다.
					// 뒤이어 P2PPingOnNeed()를 하므로, '홀펀칭 증발을 인식 못하는 문제'는 없을 것이다.
					memberRP->m_lastDirectUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs();
				}

				// 피어의 runtimePlatform 저장
				memberRP->m_runtimePlatform = runtimePlatform;
			

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
			eventID,
			CompactFieldMap());

		// enqueue event
		LocalEvent le;
		le.m_type = LocalEventType_AddMember;
		le.m_groupHostID = groupHostID;
		le.m_memberHostID = memberHostID;
		le.m_remoteHostID = memberHostID;
		le.m_memberCount = (int)GP->m_members.GetCount();

		// message / sizeOfField 값을 로컬 이벤트에 채워준다.
		message.CopyTo(le.m_customField);

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

			if (!memberRP->IsRelayedP2P() && remotePeerIsRecycled)
			{
				// remote peer가 재사용되었고 과거에도 direct P2P였다.
				// 처음에는 relayed 상태로 노티되기 때문에 곧장 direct로 바뀌었다는 이벤트를 넣어야 한다.
				LocalEvent e;
				e.m_type = LocalEventType_DirectP2PEnabled;
				e.m_remoteHostID = memberHostID;
				//m_owner->EnqueLocalEvent(e, m_owner->m_loopbackHost);
				EnqueLocalEvent(e, memberRP);
			}
		}
	}

	DEFRMI_ProudS2C_P2PGroup_MemberLeave(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

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
		shared_ptr<CRemotePeer_C> pMemberRC = m_owner->GetPeerByHostID_NOLOCK(memberHostID);
		CP2PGroupPtr_C pGroup = m_owner->GetP2PGroupByHostID_Internal(groupHostID);

		if (pGroup != nullptr)
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
		if (pGroup != nullptr)
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
			CRemotePeer_C* remotePeer = LeanDynamicCast_RemotePeer_C_PlainPtr(i->GetSecond().get());
			if (remotePeer != nullptr && !remotePeer->m_garbaged && remotePeer->m_HostID != HostID_Server)
			{
				if (remotePeer->m_ReliablePingDiffTimeMs - currTime <= 0)
				{
					int64_t t0 = remotePeer->m_ReliablePingDiffTimeMs;
					assert(CNetConfig::ReliablePingIntervalMs > 0);
					int64_t t1 = currTime + CNetConfig::ReliablePingIntervalMs;
					remotePeer->m_ReliablePingDiffTimeMs = t1;

					Tstringstream strStm;

					strStm << _PNT("P2P reliable ping diff time changed from ") << t0;
					strStm << _PNT(" to ") << t1;
					strStm << _PNT(".\n");

					Log(0, LogCategory_P2P, strStm.str().c_str());

					// reliableudp ping을 보낸다. ( relay이건 directp2p 이건 상관없이 보낸다. )
					// unreliable ping만 으로는 p2p간 패킷 유실율과 같이쓰기가 애매하기 때문
					CMessage header2; header2.UseInternalBuffer();
					Message_Write(header2, MessageType_P2PReliablePing);
					header2.Write(currTime);
					// ReportServerTimeAndFrameRateAndPing/Pong 이 사라지면서 추가된 정보.
					header2.Write(m_applicationHint.m_recentFrameRate);
					header2.Write(m_serverUdpRecentPingMs);

					CSendFragRefs sd2(header2);
					SendOpt opt = SendOpt::CreateFromRmiContextAndClearRmiContextSendFailedRemotes(g_ReliableSendForPN);
					opt.m_priority = MessagePriority_Ring0;
					Send_BroadcastLayer(sd2, nullptr, opt, &remotePeer->m_HostID, 1); // reliable ping은, direct P2P가 아니면 릴레이를 타서라도 측정되어야 한다. 그래서 이 함수를 썼다.
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
						Message_Write(header, MessageType_P2PUnreliablePing);
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

	DEFRMI_ProudS2C_P2PGroup_MemberJoin(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
//		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		int32_t runtimePlatform;
		// 혹시 값을 찾아오는데 실패하면 RuntimePlatform_C로 가정.
		if (!fieldMap.GetInt32Field(FieldType_RuntimePlatform, &runtimePlatform))
			runtimePlatform = RuntimePlatform_C;

		// p2p group에 새로운 member가 들어온것을 update한다
		m_owner->UpdateP2PGroup_MemberJoin(groupHostID,
			memberHostID,
			message,
			eventID,
			p2pFirstFrameNumber,
			connectionMagicNumber,
			p2pAESSessionKey,
			p2pFastSessionKey,
			allowDirectP2P,
			pairRecycled,
			reliableRTT,
			unreliableRTT,
			RuntimePlatform(runtimePlatform));

		return true;
	}

	DEFRMI_ProudS2C_RequestP2PHolepunch(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerID);
		if (rp == nullptr || rp->m_garbaged)
			return true;

		// P2P 홀펀칭 시도 자체가 되고 있지 않은 상태이면 즐
		if (!rp->m_p2pConnectionTrialContext)
			return true;

		// 상대측 피어의 주소를 넣는다.
		rp->m_UdpAddrFromServer = externalAddr;
		rp->m_UdpAddrInternal = internalAddr;

		assert(rp->m_UdpAddrFromServer.IsUnicastEndpoint());

		// 쌍방 홀펀칭 천이를 시작한다.
		if (rp->m_p2pConnectionTrialContext->m_state == nullptr ||
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
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

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
				Swap(A, B);
				Swap(ABSendAddr, BASendAddr);
				Swap(BARecvAddr, ABRecvAddr);
			}

			shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(B);
			if (rp == nullptr || rp->m_garbaged || !rp->m_udpSocket)
				return true;
			rp->m_P2PHolepunchedLocalToRemoteAddr = ABSendAddr;
			rp->m_P2PHolepunchedRemoteToLocalAddr = BARecvAddr;

			/////////////////////////////////////////////////////////////////////////
			// C/S 홀펀징에서 RTT 처리
			rp->m_lastPingMs = unreliableRTT;
			rp->m_lastReliablePingMs = reliableRTT;

			if (rp->m_recentReliablePingMs > 0)
			{
				rp->m_recentReliablePingMs = LerpInt(
					rp->m_recentReliablePingMs,
					rp->m_lastReliablePingMs,
					CNetConfig::LagLinearProgrammingFactorPercent, 100);
			}
			else
			{
				rp->m_recentReliablePingMs = rp->m_lastReliablePingMs;
			}

			rp->m_recentReliablePingMs = max(rp->m_recentReliablePingMs, 1);

			if (rp->m_recentPingMs > 0)
				rp->m_recentPingMs = LerpInt(rp->m_recentPingMs, rp->m_lastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
			else
				rp->m_recentPingMs = rp->m_lastPingMs;

			rp->m_recentPingMs = max(rp->m_recentPingMs, 1);
			/////////////////////////////////////////////////////////////////////////

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
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 이 RMI는 상대방과 P2P 그룹 연계가 있음에도 불구하고 P2P 연결이 끊어질 경우에도 도착한다.
		// 따라서 P2P relay mode로 전환해야 한다.
		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerHostID);
		if (rp != nullptr && !rp->m_garbaged)
		{
			if (!rp->IsRelayedP2P())
			{
				FallbackParam param;
				param.m_notifyToServer = false;
				param.m_reason = reason;

				rp->FallbackP2PToRelay(param);
			}
		}

		return true;
	}

	// pidl에 설명 있음
	DEFRMI_ProudS2C_S2C_RequestCreateUdpSocket(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// to-peer UDP Socket 생성
		m_owner->New_ToServerUdpSocket();
		bool success = m_owner->m_remoteServer->m_ToServerUdp ? true : false;

		if (success)
		{
			// Upnp 기능을 켠다. connect에서 다시 하도록 옮김 //( Connect에서 하던걸 여기로 옮김 )
			//m_owner->StartupUpnpOnNeed();

			// 서버로부터 받은 '서버 외부 주소'의 포트값과
			// 클라가 서버에 TCP 접속할 때 썼던 서버 주소의 IP값을 합치자.
			// 이렇게 하는 이유: http://nettention-wiki.pearlabyss.com:8090/pages/viewpage.action?pageId=18056099
			AddrPort serverUdpAddrAssembled = m_owner->m_serverAddrPort;
			serverUdpAddrAssembled.m_port = AddrPort::From(serverUdpAddr).m_port;

			// 홀펀칭 시도
			m_owner->m_remoteServer->SetToServerUdpFallbackable(serverUdpAddrAssembled);
		}

		// 서버에게, UDP 소켓을 열었다는 확인 메세지를 보낸다.
		m_owner->m_c2sProxy.C2S_CreateUdpSocketAck(HostID_Server, g_ReliableSendForPN, success, CompactFieldMap());

		return true;
	}

	DEFRMI_ProudS2C_S2C_CreateUdpSocketAck(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		if (succeed)
		{
			// UDP Socket 생성
			m_owner->New_ToServerUdpSocket();
			if (m_owner->m_remoteServer->m_ToServerUdp)
			{
				// Upnp 기능을 켠다. connect에서 다시 하도록 옮김 //( Connect에서 하던걸 여기로 옮김 )
				//m_owner->StartupUpnpOnNeed();

				// 서버로부터 받은 '서버 외부 주소'의 포트값과
				// 클라가 서버에 TCP 접속할 때 썼던 서버 주소의 IP값을 합치자.
				AddrPort serverUdpAddrAssembled = m_owner->m_serverAddrPort;
				serverUdpAddrAssembled.m_port = AddrPort::From(serverudpaddr).m_port;

				// 홀펀칭 시도
				m_owner->m_remoteServer->SetToServerUdpFallbackable(serverUdpAddrAssembled);
			}
		}

		// Request 초기화
		m_owner->m_remoteServer->SetServerUdpReadyWaiting(false);

		return true;
	}

	// 상대와의 P2P 홀펀칭 시도하던 것들을 모두 중지시킨다.
	// 이미 한 경로가 성공했기 때문이다. hole punch race condition이 발생하면 안되니까.
	DEFRMI_ProudC2C_HolsterP2PHolepunchTrial(CNetClientImpl::C2CStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remote);
		if (rp != nullptr && !rp->m_garbaged)
		{
			rp->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
		}

		return true;
	}

	bool CNetClientImpl::GetP2PGroupByHostID(HostID groupHostID, CP2PGroup &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CP2PGroupPtr_C g = GetP2PGroupByHostID_Internal(groupHostID);
		if (g == nullptr)
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

	void CNetClientImpl::ExpandHostIDArray_Append(HostID sendTo, HostIDArray &sendTo2)
	{
		LockMain_AssertIsLockedByCurrentThread();
		// convert sendTo group to remote hosts

		CP2PGroupPtr_C g = GetP2PGroupByHostID_Internal(sendTo);
		if (g == nullptr)
		{
			// 그냥 원본 그대로 추가
			sendTo2.Add(sendTo);
		}
		else
		{
			// P2P그룹을 풀어서 추가
			for (CP2PGroup_C::P2PGroupMembers::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
			{
				HostID memberID = i->GetFirst();
				sendTo2.Add(memberID);
			}
		}
	}

	shared_ptr<CRemotePeer_C> CNetClientImpl::GetPeerByHostID_NOLOCK(HostID peerHostID)
	{
		AssertIsLockedByCurrentThread();

		shared_ptr<CHostBase> ret;

		ret = AuthedHostMap_Get(peerHostID);

		return LeanDynamicCast_RemotePeer_C(ret);
	}

	// <홀펀칭이 성공한 주소>를 근거로 peer를 찾는다.
	// findInRecycleToo: true이면 recycled remote peers에서도 찾는다.
	shared_ptr<CRemotePeer_C> CNetClientImpl::GetPeerByUdpAddr(AddrPort UdpAddr, bool /*findInRecycleToo*/)
	{
		//CriticalSectionLock clk(m_critSecPtr, true);
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CRemotePeer_C> rp = LeanDynamicCast_RemotePeer_C(i->GetSecond());
			if (rp && !rp->m_garbaged && rp->m_P2PHolepunchedRemoteToLocalAddr == UdpAddr)
			{
				return rp;
			}
		}
		for (HostIDToRemotePeerMap::iterator i = m_remotePeerRecycles.begin(); i != m_remotePeerRecycles.end(); i++)
		{
			shared_ptr<CRemotePeer_C> rp = i->GetSecond();
			if (rp && !rp->m_garbaged && rp->m_P2PHolepunchedRemoteToLocalAddr == UdpAddr)
			{
				return rp;
			}
		}
		return shared_ptr<CRemotePeer_C>();
	}

	bool CNetClientImpl::GetPeerInfo(HostID remoteHostID, CNetPeerInfo &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		shared_ptr<CHostBase> hostBase ;
		m_authedHostMap.TryGetValue(remoteHostID, hostBase);
		m_authedHostMap.AssertConsist();

		shared_ptr<CRemotePeer_C> remotePeer = LeanDynamicCast_RemotePeer_C(hostBase);
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

	int64_t CNetClientImpl::GetP2PServerTimeMs(HostID groupHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		// 서버와 그룹 멤버의 서버 시간의 평균을 구한다.
		int count = 1;
		int64_t diffSum = GetServerTimeDiffMs();
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(groupHostID);
		if (group != nullptr)
		{
			// 			if(m_timer == nullptr)
			// 				return -1;

			for (CP2PGroup_C::P2PGroupMembers::iterator i = group->m_members.begin(); i != group->m_members.end(); i++)
			{
				shared_ptr<IP2PGroupMember> peer = i->GetSecond().lock();
				if (peer != nullptr)
				{
					if(peer->GetHostID() != HostID_Server)
					{
						count++;
						//NTTNTRACE("%3.31f indirectServerTimediff\n", double(peer->GetIndirectServerTimeDiff()) / 1000);
						diffSum += peer->GetIndirectServerTimeDiff();
					}
					else
					{
						// P2P member가, 서버다.
						diffSum += GetServerTimeDiffMs(); 
						count++;
					}
				}
			}

			int64_t groupServerTimeDiff;
			if(count>0)
			{
				groupServerTimeDiff = diffSum / ((int64_t)count);
			}
			else
			{
				groupServerTimeDiff = 0;
			}
			int64_t clientTime = GetPreciseCurrentTimeMs();

			return clientTime - groupServerTimeDiff;
		}
		else
		{
			return GetServerTimeMs();
		}
	}

	// 모든 그룹을 뒤져서, local과 제거하려는 remote 양쪽 모두 들어있는 P2P group이 하나도 없으면
	// P2P 연결 해제를 한다.
	void CNetClientImpl::RemoveRemotePeerIfNoGroupRelationDetected(shared_ptr<CRemotePeer_C> memberRC)
	{
		LockMain_AssertIsLockedByCurrentThread();

		for (P2PGroups_C::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); i++)
		{
			CP2PGroupPtr_C g = i->GetSecond();
			for (CP2PGroup_C::P2PGroupMembers::iterator j = g->m_members.begin(); j != g->m_members.end(); j++)
			{
				shared_ptr<IP2PGroupMember> p = j->GetSecond().lock();
				if (memberRC == p)
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
		if (g != nullptr)
		{
			for (CP2PGroup_C::P2PGroupMembers::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
			{
				HostID mnum = i->GetFirst();
				output.Add(mnum);
			}
		}
	}

	int64_t CNetClientImpl::GetIndirectServerTimeMs(HostID peerHostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 		if(m_timer == nullptr)
		// 			return -1;

		int64_t clientTime = GetPreciseCurrentTimeMs();

		shared_ptr<CRemotePeer_C> peer = GetPeerByHostID_NOLOCK(peerHostID);
		if (peer != nullptr)
		{
			if (!peer->m_forceRelayP2P)
				peer->m_jitDirectP2PNeeded = true;
			return clientTime - peer->GetIndirectServerTimeDiff();
		}

		return clientTime - m_serverTimeDiff;
	}

	DEFRMI_ProudC2C_ReportUdpMessageCount(CNetClientImpl::C2CStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 상대방으로부터 상대방이 보낸갯수와 받은갯수를 업데이트한다.
		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remote);
		if (rp != nullptr && !rp->m_garbaged)
		{
			// 갯수 업데이트
			rp->m_toRemotePeerSendUdpMessageSuccessCount = udpSuccessCount;
			m_owner->m_c2sProxy.ReportC2CUdpMessageCount(HostID_Server, g_ReliableSendForPN,
				rp->m_HostID, rp->m_toRemotePeerSendUdpMessageTrialCount, rp->m_toRemotePeerSendUdpMessageSuccessCount,
				CompactFieldMap());
		}

		return true;
	}


	void CNetClientImpl::ReportRealUdpCount()
	{
		// 기능사용이 꺼져있으면 그냥 리턴
		if (CNetConfig::UseReportRealUdpCount == false)
			return;

		CriticalSectionLock clk(GetCriticalSection(), true);

		if (HostID_None != GetVolatileLocalHostID() 
			&& GetPreciseCurrentTimeMs() - m_ReportUdpCountTime_timeToDo > 0)
		{
			m_ReportUdpCountTime_timeToDo = GetPreciseCurrentTimeMs() + CNetConfig::ReportRealUdpCountIntervalMs;

			// 서버에게 udp 보낸 갯수를 보고한다.
			m_c2sProxy.ReportC2SUdpMessageTrialCount(HostID_Server, g_ReliableSendForPN, m_toServerUdpSendCount,
				CompactFieldMap());

			// peer들에게 내가 peer들에게 보내거나 혹은 받은 udp 갯수를 보고한다.
			// (매 10초마다 쏘면 트래픽이 스파이크를 칠지 모르나, 피어가 100개라 하더라도 2000바이트 정도이다. 이 정도는 묵인해도 되겠다능.)
			for (AuthedHostMap::iterator iRP = m_authedHostMap.begin(); iRP != m_authedHostMap.end(); iRP++)
			{
				shared_ptr<CRemotePeer_C> rp = LeanDynamicCast_RemotePeer_C(iRP->GetSecond());

				if (rp != nullptr)
				{
					if (rp->m_garbaged)
						continue;

					m_c2cProxy.ReportUdpMessageCount(rp->m_HostID, g_ReliableSendForPN, rp->m_receiveudpMessageSuccessCount,
						CompactFieldMap());
				}
			}
		}
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
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				m_remoteServer->m_ToServerUdp->RequestStopIo();
			}

			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());
				// 강제로 소켓을 다 닫아버린다. 즉 UDP 소켓이 맛간 상태를 만들어버린다.
				if (peer != nullptr && peer->m_udpSocket != nullptr)
					peer->m_udpSocket->RequestStopIo();
				/* Fallback이 이루어짐을 테스트해야 하므로
				FirstChanceFallbackEveryPeerUdpToTcp,FirstChanceFallbackServerUdpToTcp 즐 */
			}
			break;
		case FallbackMethod_ServerUdpToTcp:
		{
			FallbackParam param;
			param.m_notifyToServer = true;
			param.m_reason = ErrorType_UserRequested;
			FirstChanceFallbackServerUdpToTcp(param);
		}
			break;
		case FallbackMethod_PeersUdpToTcp:
		{
			FallbackParam param;
			param.m_notifyToServer = true;
			param.m_reason = ErrorType_UserRequested;
			FirstChanceFallbackEveryPeerUdpToTcp(param);
		}
			break;
		default:
			break;
		}
	}


	int CNetClientImpl::GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 서버
		if (peerHostID == HostID_Server)
		{
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			return m_serverUdpLastPingMs;
		}

		// 피어
		shared_ptr<CRemotePeer_C> p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != nullptr)
		{
			if (error != nullptr)
			{
				if (p)
					*error = ErrorType_Ok;
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
			for (CP2PGroup_C::P2PGroupMembers::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
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
				if (error != nullptr)
					*error = ErrorType_Ok;

				return total / cnt;
			}
		}

		// 상기 루틴을 모두 성공하지 못하고 여기까지 왔으면 에러.
		if (error != nullptr)
		{
			*error = ErrorType_ValueNotExist;
		}
		return -1;
	}

	int  CNetClientImpl::GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 서버
		if (peerHostID == HostID_Server)
		{
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			return m_serverUdpRecentPingMs;
		}

		// 피어
		shared_ptr<CRemotePeer_C> p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != nullptr)
		{
			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
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
			for (CP2PGroup_C::P2PGroupMembers::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
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
				if (error != nullptr)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}

		// 여기까지 못 얻었으면 실패
		if (error != nullptr)
		{
			*error = ErrorType_ValueNotExist;
		}
		return -1;
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


	// 모든 peer들과의 UDP를 relay화한다.
	// firstChance=true이면 서버에게도 RMI로 신고한다.
	void CNetClientImpl::FirstChanceFallbackEveryPeerUdpToTcp(const FallbackParam &param)
	{
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());
			if (peer != nullptr)
			{

				peer->FallbackP2PToRelay(param);
			}
		}
	}

	bool CNetClientImpl::RestoreUdpSocket(HostID peerID)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> peer = GetPeerByHostID_NOLOCK(peerID);
		if (peer && peer->m_udpSocket)
		{
			peer->m_udpSocket->m_turnOffSendAndReceive = false;
			return true;
		}

		return false;
	}

	DEFRMI_ProudS2C_NewDirectP2PConnection(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> peer = m_owner->GetPeerByHostID_NOLOCK(remotePeerID);
		if (peer)
		{
			if (peer->m_udpSocket == nullptr)
			{
				peer->m_newP2PConnectionNeeded = true;

				if (m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0)
					m_owner->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Request p2p connection to Client %d."), peer->m_HostID));
			}
		}

		return true;
	}


	bool CNetClientImpl::GetDirectP2PInfo(HostID peerID, CDirectP2PInfo &outInfo)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (peerID == HostID_Server)
		{
			return false;
		}

		shared_ptr<CRemotePeer_C> peer = GetPeerByHostID_NOLOCK(peerID);
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

		shared_ptr<CRemotePeer_C> peer = GetPeerByHostID_NOLOCK(peerID);
		if (peer != nullptr)
		{
			peer->GetDirectP2PInfo(outDirectP2PInfo);
			bool ret = outDirectP2PInfo.HasBeenHolepunched();

			if (peer->m_udpSocket != nullptr && !peer->m_udpSocket->m_turnOffSendAndReceive)
			{
				// 과거에는 UDP socket을 진짜로 닫았지만,
				// 이는 홀펀칭 증발과 동일한 상황도 아니며,
				// 게다가 UDP socket을 지운 후 다시 만드는 경우 포트 재사용을 못하는 일부 OS때문에
				// 정확한 테스트가 안된다. 따라서 아래와 같이 통신을 차단만 한다.
				peer->m_udpSocket->m_turnOffSendAndReceive = true;

				// 홀펀칭 진행중이던 것도 중단해야.
				peer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

				// Fallback이 바로 일어나야 하므로
				FallbackParam param;
				param.m_notifyToServer = true;
				param.m_reason = ErrorType_UserRequested;
				peer->FallbackP2PToRelay(param);
			}

			return ret;
		}

		return false;
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
					shared_ptr<CRemotePeer_C> rp = LeanDynamicCast_RemotePeer_C(iter->GetSecond());

					if (rp != nullptr)
					{
						if (!rp->IsRelayedP2P())
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
								ping,
								CompactFieldMap());
						}
					}
				}
			}
		}
	}


	CompressedRelayDestList_C::CompressedRelayDestList_C()
	{
		// 이 객체는 로컬 변수로 주로 사용되며, rehash가 일단 발생하면 대쪽 느려진다. 따라서 충분한 hash table을 미리 준비해 두고
		// rehash를 못하게 막아버리자.
		if (!m_p2pGroupList.InitHashTable(1627))
			throw std::bad_alloc();

		m_p2pGroupList.DisableAutoRehash();
	}


	// 이 안에 있는 group id, host id들의 총 갯수를 얻는다.
	// compressed relay dest list를 serialize하기 전에 크기 예측을 위함.
	int CompressedRelayDestList_C::GetAllHostIDCount()
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
	void CompressedRelayDestList_C::AddSubset(const HostIDArray &subsetGroupHostID, HostID hostID)
	{
		for (int i = 0; i < subsetGroupHostID.GetCount(); i++)
		{
			P2PGroupSubset_C& subset = m_p2pGroupList[subsetGroupHostID[i]]; // 없으면 새 항목을 넣고 있으면 그걸 찾는다.

			if (hostID != HostID_None)
				subset.m_excludeeHostIDList.Add(hostID);
		}
	}

	void CompressedRelayDestList_C::AddIndividual(HostID hostID)
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

		shared_ptr<CRemotePeer_C> p = GetPeerByHostID_NOLOCK(remotePeerID);
		if (p != nullptr)
		{
			p->m_forceRelayP2P = enable;
			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}


}