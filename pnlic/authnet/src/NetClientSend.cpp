#include "stdafx.h"

#include <typeinfo>
#include <stack>

#include "NetClient.h"
#include "CollUtil.h"
#include "SendFragRefs.h"
#include "Relayer.h"


namespace Proud
{
	// 싱글캐스트,멀티캐스트,릴레이 등을 처리
	bool CNetClientImpl::Send_BroadcastLayer(
		const CSendFragRefs& payload,
		const CSendFragRefs* encryptedPayload,
		const SendOpt& sendContext,
		const HostID *sendTo, int numberOfsendTo)
	{
		CRemotePeer_C *rp = NULL;

		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == NULL || GetVolatileLocalHostID() == HostID_None) // 이미 서버와 연결 해제된 상태면 즐
		{
			EnqueError(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, NoServerConnectionErrorText));
			return false;
		}

		// hostid list를 정렬할 array를 따로 만든다.
		POOLED_ARRAY_LOCAL_VAR(HostIDArray, sortedHostIDList);

		sortedHostIDList.SetCount(numberOfsendTo);
		UnsafeFastMemcpy(sortedHostIDList.GetData(), sendTo, (size_t)numberOfsendTo * sizeof(HostID));

		// NOTE: 이 구간 이후부터 sendTo, numberOfsendTo는 쓰지 말 것 .

		// 정렬&중복 제거 한다.
		//QuickSort(sortedHostIDList.GetData(), sortedHostIDList.GetCount());modify by rekfkno1 - unionduplicates안에서 sort하고 있음.
		UnionDuplicates<HostIDArray, HostID, int>(sortedHostIDList);

		// 수신 대상을 ungroup한다. 즉 p2p group은 모두 분해해서 개별 remote들로만 추려낸다.
		POOLED_ARRAY_LOCAL_VAR(ISendDestArray, individualDestList);

		ConvertGroupToIndividualsAndUnion(
			(int)sortedHostIDList.GetCount(),
			sortedHostIDList.GetData(),
			individualDestList);

		// 릴레이 타야 하는 dest list. 단, 비압축.
		POOLED_ARRAY_LOCAL_VAR(RelayDestList_C, relayDestList);
		
		int directSendToWanPeerCount = 0; // 다른 LAN 환경의 피어에게 메시징한 횟수

		// 릴레이 타야 하는 dest list. 단, 압축. 비압축 형태보다 더 커질 수 있는데 이런 경우 비압축 버전이 사용될 것이다.
		POOLED_LOCAL_VAR(CompressedRelayDestList_C, compressedRelayDestList);

		// 이 함수가 실행되는 동안 재사용될 것임. 그래서 여기서 선언을.
		POOLED_ARRAY_LOCAL_VAR(HostIDArray, subsetGroupHostIDList);
		

		// 실제로 보낼 메시지. 
		const CSendFragRefs* realPayload = NULL;

		if (encryptedPayload != NULL)
		{
			// 압축 또는 암호화 또는 둘다 적용된 메시지일 경우 securedPayload 는 NULL 이 아니다. securedPayload 를 보낸다. (loopback 제외)
			realPayload = encryptedPayload;
		}
		else
		{
			//securedPayload 가 NULL 일 경우 압축이나 암호화 가 적용된 메시지가 아니다. 원본을 보낸다.
			realPayload = &payload;
		}

		// for each sendTo items BEGIN
		for (int i = 0; i < individualDestList.GetCount(); i++)
		{
			CHostBase *sendDest = individualDestList[i];
			if (sendDest == m_loopbackHost && sendContext.m_enableLoopback)
			{
				// 루프백인 경우

				// enqueue final recv queue and signal
				CMessage payloadMsg;
				payload.ToAssembledMessage(payloadMsg);

				CReceivedMessage ri;
				ri.m_remoteHostID = GetVolatileLocalHostID();
				ri.m_unsafeMessage = payloadMsg;
				ri.m_unsafeMessage.SetReadOffset(0);

				UserTaskQueue_Add(m_loopbackHost, ri, GetWorkTypeFromMessageHeader(payloadMsg), true);
			}
			else if (sendDest == (CHostBase*)m_remoteServer)
			{
				// 서버에게 보내는 경우
				m_remoteServer->Send_ToServer_Directly_Copy(HostID_Server, sendContext.m_reliability, *realPayload, sendContext, m_simplePacketMode);
			}
			else if (sendDest != NULL)
			{
				// P2P로 보내는 메시지인 경우
				try
				{
					rp = dynamic_cast<CRemotePeer_C*>(sendDest);
				}
				catch (std::bad_cast& e)
				{
					CFakeWin32::OutputDebugStringA(e.what());
					CauseAccessViolation();
				}

				if (rp != NULL)
				{
					bool isRemoteSameLanToLocal = false; // 같은 LAN 내 피어게 전송하는 것인지?
					if (CNetConfig::UseIsSameLanToLocalForMaxDirectP2PMulticast)
						isRemoteSameLanToLocal = rp->IsSameLanToLocal();

					if (!rp->IsRelayedP2P() // 홀펀칭 되어 있고
						&& !rp->m_forceRelayP2P	// forceRelayP2P 가 false 인 경우는 강제 릴레이 대상이 아니다.(이 경우는 P2P 메시징 허락)
						&& !rp->IsRelayMuchFasterThanDirectP2P(m_serverUdpRecentPingMs, sendContext.m_forceRelayThresholdRatio) // 릴레이가 되레 훨씬 빠른 상황이 아니고
						&& (directSendToWanPeerCount < sendContext.m_maxDirectBroadcastCount || isRemoteSameLanToLocal)) // 최대 멀티캐스트 갯수 제한에 안 걸렸으면
					{
						if (!isRemoteSameLanToLocal)
							directSendToWanPeerCount++;

						// P2P 메시지 전송 횟수를 카운팅. 내부 전용 제외하고.
						if (!sendContext.m_INTERNAL_USE_isProudNetSpecificRmi)
							rp->m_toRemotePeerSendUdpMessageTrialCount++;

						// send_core to connected peer via reliable or unreliable UDP
						if (sendContext.m_reliability == MessageReliability_Reliable)
						{
							rp->m_ToPeerReliableUdp.SendWithSplitter_Copy(*realPayload);
						}
						else
						{
							// remote peer에게 UDP로 송신.
							rp->m_ToPeerUdp.SendWithSplitter_Copy(*realPayload, SendOpt(sendContext));

							/* remote peer A에게 다이렉트로 쏜 경우, 그리고 A가 소속된 p2p group G이
							애당초 송신할 대상에 있다면, G와 ~A를 compressed relay dest list에 넣는다.
							어차피 루프를 계속 돌면서 G 안의 피어들이 direct send가 가능해지면 compressed relay dest list의 차집합
							으로서 계속 추가될 것이므로 안전. */
							if (GetIntersectionOfHostIDListAndP2PGroupsOfRemotePeer(sortedHostIDList, rp, &subsetGroupHostIDList))
							{
								compressedRelayDestList.AddSubset(subsetGroupHostIDList, rp->m_HostID);
							}
						}
					}
					else
					{
						/*						if (::g_MustNotRelay)
						assert(0);*/
						// 릴레이로 보내는 경우.
						// gather P2P unconnected sendTo

						assert(rp->m_HostID != GetVolatileLocalHostID());

						if (sendContext.m_reliability == MessageReliability_Reliable) // 이건 할당해줘야
						{
							/* remote의 reliable UDP에 stream 및 sender window에 뭔가가 이미 들어있을 수 있다.
							그것들을 먼저 UDP send queue로 flush를 해준 다음에야 정확한 reliable UDP용 next frame number를 얻을 수 있다.
							reliable UDP에 있는 data frame들은 추후 보내진다. 당연히 보내지는 순서는 다르겠지만,
							MessageType_ReliableRelay2를 받는 클라는 frame number를 얻어서 reliable UDP에 조립 후 뽑아내기 때문에 안전하다. */
							RelayDest_C rd;
							rd.m_frameNumber = rp->m_ToPeerReliableUdp.AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame();
							rd.m_remotePeer = rp;

							relayDestList.Add(rd);
						}
						else if (sendContext.m_allowRelaySend)	// unreliable인 경우, AllowRelaySend가 false일 경우 relay로도 보내지 않는다.
						{
							// 자,이제 릴레이로 unreliable 메시징을 하자.
							RelayDest_C rd;
							rd.m_frameNumber = 0; // 어차피 안쓰이니까
							rd.m_remotePeer = rp;
							relayDestList.Add(rd);

							/* rp에게 릴레이로 쏴야 한다. 그런데 rp가 소속된 그룹이 애당초 보내야 할 리스트에 있다고 치자.
							그렇다면 그 그룹을 compressed relay dest에 모두 넣어야 한다.
							그래도 괜찮은 것이, 각 individual에 대한 루프를 돌면서 direct send를 한 경우 compressed relay dest에 릴레이 제외 대상
							으로서 계속 추가된다. */
							if (GetIntersectionOfHostIDListAndP2PGroupsOfRemotePeer(sortedHostIDList, rp, &subsetGroupHostIDList))
							{
								compressedRelayDestList.AddSubset(subsetGroupHostIDList, HostID_None);
							}
							else
							{
								compressedRelayDestList.AddIndividual(rp->m_HostID);
							}
						}

						// 직접 P2P로 패킷을 쏘는 경우, JIT P2P 홀펀칭 조건이 된다.
						if (sendContext.m_enableP2PJitTrigger)
						{
							if (!rp->m_forceRelayP2P)
								rp->m_jitDirectP2PNeeded = true;
						}
					}
				}
			}
		}

		// 릴레이 송신 대상이 존재할 경우
		if (relayDestList.GetCount() > 0)
		{
			/* 릴레이 송신 대상이 존재하는 상황. 이때 서버와 UDP 통신을 안하더라도 Per-Peer UDP socket 방식이고
			그게 자체적인 클라-서버간 UDP 홀펀칭을 시행하므로 굳이 별도의 클라-서버간 UDP 통신 개시는 불필요. */
			//RequestServerUdpSocketReady_FirstTimeOnly();
			// if unreliable send
			if (sendContext.m_reliability == MessageReliability_Unreliable)
			{
				// send_core relayed message of gathered list to server via UDP or fake UDP
				POOLED_ARRAY_LOCAL_VAR(HostIDArray, relayDestList2);
				//relayDestList2.reserve(relayDestList.GetCount());

				// 보낼 메시지 헤더
				CMessage header;

				// 압축된 relay dest를 쓸건지 말 건지를 파악한다.
				if (!CNetConfig::ForceCompressedRelayDestListOnly
					&& relayDestList.GetCount() <= compressedRelayDestList.GetAllHostIDCount() + 1)
				{
					// 비압축 버전이 압축 버전보다 더 경제적인 경우.
					for (int i = 0; i < relayDestList.GetCount(); i++)
					{
						RelayDest_C rd = relayDestList[i];
						relayDestList2.Add(rd.m_remotePeer->m_HostID);
					}

					header.UseInternalBuffer();
					header.Write((char)MessageType_UnreliableRelay1); // 헤더 ID				
					header.Write(sendContext.m_priority);
					header.WriteScalar(sendContext.m_uniqueID.m_value);
					header.Write(relayDestList2);		// 릴레이 리스트

					/*static int noncompheadersize =0;
					noncompheadersize+= header.GetLength();
					ATLTRACE("NonCompress Header size : %u\n", noncompheadersize);*/
				}
				else
				{
					// 압축 버전.
					header.UseInternalBuffer();
					header.Write((char)MessageType_UnreliableRelay1_RelayDestListCompressed); // 헤더 ID				
					header.Write(sendContext.m_priority);
					header.WriteScalar(sendContext.m_uniqueID.m_value);

					// 그룹에 들어가있지않은 호스트들의 릴레이 리스트
					header.Write(compressedRelayDestList.m_includeeHostIDList);

					// (그룹 및 그 그룹에서 제거되어야 할 호스트들)의 리스트
					int groupListCount = compressedRelayDestList.m_p2pGroupList.GetCount();
					header.WriteScalar(groupListCount);
					CFastMap2<HostID, P2PGroupSubset_C, int>::iterator it = compressedRelayDestList.m_p2pGroupList.begin();
					for (; it != compressedRelayDestList.m_p2pGroupList.end(); it++)
					{
						header.Write(it->GetFirst());
						header.Write(it->GetSecond().m_excludeeHostIDList);
					}

					/*static int compheadersize =0;
					compheadersize+= header.GetLength();
					ATLTRACE("Compress Header size : %u\n", compheadersize);*/
				}

				CMessageTestSplitterTemporaryDisabler dd(header);
				header.WriteScalar(realPayload->GetTotalLength()); // 보낼 데이터 크기

				CSendFragRefs unreliableRelayMsg;
				unreliableRelayMsg.Add(header);
				unreliableRelayMsg.Add(*realPayload);

				// 릴레이 메시지를 보낸다.
				// 복사 없이 바로 보낸다.				
				// HostID_None을 넣어도 무방하다.hostID_Server이랑만 겹치지 않으면 되니까...

				// 릴레이 메시지도 내부메시지로 처리한다.
				SendOpt opt(sendContext);
				opt.m_INTERNAL_USE_isProudNetSpecificRmi = true;
				opt.m_uniqueID.m_relayerSeparator = (char)UniqueID_RelayerSeparator_Relay1;
				assert(sendContext.m_INTERNAL_USE_fraggingOnNeed == true); // relay 패킷은 relay dest가 여럿일 수 있다. 상당히 커질 수 있으므로 fragging은 꼭 켜야.
				m_remoteServer->Send_ToServer_Directly_Copy(
					HostID_None,
					MessageReliability_Unreliable,
					unreliableRelayMsg,
					opt,
					m_simplePacketMode);
			}
			else
			{
				// send_core relayed-long-frame with gathered list to server via *TCP* with each frame number
				CSendFragRefs longFrame;

				CSmallStackAllocMessage tempHeader;
				CTcpLayer_Common::AddSplitterButShareBuffer(*realPayload, longFrame, tempHeader, m_simplePacketMode);

				RelayDestList rdList2;
				relayDestList.ToSerializable(rdList2);

				CSendFragRefs relayedLongFrame;

				// 보낼 메시지 헤더
				CMessage header;
				header.UseInternalBuffer();
				header.Write((char)MessageType_ReliableRelay1); // 헤더 ID
				header.Write(rdList2);		// 릴레이 리스트 (각 수신자별 프레임 number 포함)
				CMessageTestSplitterTemporaryDisabler dd(header);
				header.WriteScalar(longFrame.GetTotalLength()); // 보낼 데이터 크기

				relayedLongFrame.Add(header);
				relayedLongFrame.Add(longFrame);

				/* 릴레이 메시지를 보낸다.
				복사 없이 바로 보낸다.
				(수신자별 프레임 번호가 포함되어있으므로 받는 쪽에서는 reliable UDP 층에 보내서 정렬한다.
				따라서 relay/direct 전환중에도 안전하게 데이터가 송달될 것이다.) */

				// 릴레이 메시지도 내부메시지로 처리한다.
				SendOpt opt(sendContext);
				opt.m_uniqueID.m_relayerSeparator = (char)UniqueID_RelayerSeparator_Relay1;
				opt.m_INTERNAL_USE_isProudNetSpecificRmi = true;
				m_remoteServer->Send_ToServer_Directly_Copy(
					HostID_None,
					MessageReliability_Reliable,
					relayedLongFrame,
					opt,
					m_simplePacketMode);
			}
		}

		return true;
	}


}