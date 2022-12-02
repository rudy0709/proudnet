#include "stdafx.h"
#include "NetServer.h"
#include "PooledObjects_C.h"

namespace Proud
{
	bool CNetServerImpl::Send(const CSendFragRefs &sendData,
		const SendOpt& sendContext0,
		const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength)
	{
		SendOpt sendContext = sendContext0;
		AdjustSendOpt(sendContext);

		/* 네트워킹 비활성 상태이면 무조건 그냥 아웃.
		여기서 사전 검사를 하는 이유는, 아래 하위 callee들은 많은 validation check를 하는데, 그걸
		다 거친 후 안보내는 것 보다는 앗싸리 여기서 먼저 안보내는 것이 성능상 장점이니까.*/
		if (!m_listening)
		{
			FillSendFailListOnNeed(sendContext, sendTo, numberOfsendTo, ErrorType_DisconnectFromLocal);
			return false;
		}

		// 설정된 한계보다 큰 메시지인 경우
		if (sendData.GetTotalLength() > m_settings.m_serverMessageMaxLength)
		{
			stringstream ss;
			ss << "Too long message cannot be sent. length=" << sendData.GetTotalLength();
			throw Exception(ss.str().c_str());
		}
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
		// P2P group 정보 등을 액세스하므로 main lock 해야.
		CriticalSectionLock mainlock(GetCriticalSection(), true);

		// NOTE: maxDirectBroadcastCount는 서버에서 안쓰임
		int unreliableS2CRoutedMulticastMaxCount = max(sendContext.m_unreliableS2CRoutedMulticastMaxCount, 0);
		int unreliableS2CRoutedMulticastMaxPing = max(sendContext.m_unreliableS2CRoutedMulticastMaxPingMs, 0);

		// P2P 그룹을 HostID 리스트로 변환, 즉 ungroup한다.		
		POOLED_LOCAL_VAR(HostIDArray, sendDestList0);

		// sendDestList0에는 garbaged인 것들도 일단 다 포함된다.
		ExpandHostIDArray(numberOfsendTo, sendTo, sendDestList0);

		// 각 수신 대상의 remote host object를 얻는다.
		POOLED_LOCAL_VAR(SendDestInfoArray, sendDestList);

		// 여기서 garbaged인 것들이 제외된다.
		HostIDArrayToHostPtrArray_NOLOCK(sendDestList, sendDestList0, sendContext);

		int sendDestListCount = (int)sendDestList.GetCount();

		// 실제로 보낼 메시지. 
		const CSendFragRefs* realPayload = nullptr;

		int64_t curTime = GetPreciseCurrentTimeMs();

		if (encryptedPayload != nullptr)
		{
			// 압축 또는 암호화 또는 둘다 적용된 메시지일 경우 securedPayload 는 nullptr 이 아니다. securedPayload 를 보낸다. (loopback 제외)
			realPayload = encryptedPayload;
		}
		else
		{
			//securedPayload 가 nullptr 일 경우 압축이나 암호화 가 적용된 메시지가 아니다. 원본을 보낸다.
			realPayload = &payload;
		}

		bool ret = true;	// 하나라도 보내지 못하면 false

		// main lock이 있는 상태에서 수행해야 할것들을 처리하자
		if (sendContext.m_reliability == MessageReliability_Reliable)
		{
 			// 예전에는 AddToSendQueue 직전에 했으나, 보아하니 아래 루프도 main을 액세스 안하므로, 병렬 향상을 위해 여기다 옮겼다. code profile 후 판단한거임.
 			// 게다가 sendDestList가 shared_ptr로써 remote를 잡고 있으므로, 댕글링 걱정 뚝.
 			mainlock.Unlock();
			
			//int reliableCount = 0;
			POOLED_LOCAL_VAR(CSuperSocketArray, reliableSendList);

			for (int i = 0; i < sendDestListCount; i++)
			{
				SendDestInfo& SD1 = sendDestList[i];
				shared_ptr<CHostBase>& SD2 = SD1.mObject;

				// if loopback
				if (SD2 == m_loopbackHost && sendContext.m_enableLoopback)
				{
					// loopback 메시지는 압축, 암호화가 적용되어 있으면 메시지가 버려진다. 원본 메시지를 보낸다.
					// 즉시 final user work로 넣어버리자. 루프백 메시지는 사용자가 발생하는 것 말고는 없는 것을 전제한다.
					CMessage payloadMsg;
					payload.ToAssembledMessage(payloadMsg);

					CFinalUserWorkItem workItem;
					CReceivedMessage& ri = workItem.Internal().m_unsafeMessage;
					ri.m_remoteHostID = HostID_Server;
					ri.m_unsafeMessage = payloadMsg;
					ri.m_unsafeMessage.SetReadOffset(0);
					workItem.Internal().ModifyForLoopback();
					TryGetReferrerHeart(workItem.Internal().m_netCoreReferrerHeart);

					// 서버가 이미 셧다운 중이면 루프백 메시지조차 처리하지 말도록 하자.
					if (workItem.Internal().m_netCoreReferrerHeart)
						m_userTaskQueue.Push(SD2, workItem);
				}
				else if (SD2)
				{
					// SD3자체는 list에 추가되지 않는다. 멤버가 추가되지.
					// 그리고 shared_ptr의 cast는 비싸다.
					CRemoteClient_S* SD3 = LeanDynamicCast_RemoteClient_S_PlainPtr(SD2.get());
					if (SD3 && SD3->m_tcpLayer)
					{
						// main lock 상태이므로 아래 변수는 중도 증발 안하니 걱정 뚝
						reliableSendList.Add(SD3->m_tcpLayer);
						// 						reliableSendList[reliableCount] = SD3->m_tcpLayer;
						// 						reliableCount++;
					}
					else
					{
						// 보낼 상대가 정작 없다. 에러 처리.
						FillSendFailListOnNeed(sendContext, &SD1.mHostID, 1, ErrorType_InvalidHostID);
						ret = false;
					}
				}
				else
				{
					// 보낼 상대가 정작 없다. 에러 처리.
					FillSendFailListOnNeed(sendContext, &SD1.mHostID, 1, ErrorType_InvalidHostID);
					ret = false;
				}
			}

			// reliable message 수신자들에 대한 처리
			for (int i = 0; i < reliableSendList.GetCount(); i++)
			{
				auto &SD3 = reliableSendList[i];
				if (SD3->GetSocketType() == SocketType_WebSocket)	// WebSocket은 압축과 암호화를 하지않은 데이터를 보낸다.
				{
					SD3->AddToSendQueueWithSplitterAndSignal_Copy(
						SD3,
						payload, SendOpt(), m_simplePacketMode);
				}
				else
				{
					SD3->AddToSendQueueWithSplitterAndSignal_Copy(
						SD3,
						*realPayload, SendOpt(), m_simplePacketMode);
				}
			}
		}
		else // MessageReliability_Unreliable
		{
			AssertIsLockedByCurrentThread();  // 위에서 main lock 이미 걸었잖아.

			POOLED_LOCAL_VAR(SendDestInfoPtrArray, unreliableSendInfoList);

			// HostID 리스트간에, P2P route를 할 수 있는 것들끼리 묶는다. 단, unreliable 메시징인 경우에 한해서만.
			MakeP2PRouteLinks(sendDestList, unreliableS2CRoutedMulticastMaxCount, unreliableS2CRoutedMulticastMaxPing);

			// 각 수신자에 대해...
			int sendDestListCount_ = (int)sendDestList.GetCount();
			for (int i = 0; i < sendDestListCount_; i++)
			{
				SendDestInfo& SD1 = sendDestList[i];
				shared_ptr<CHostBase> &SD2 = SD1.mObject;

				// if loopback
				if (SD2 == m_loopbackHost && sendContext.m_enableLoopback) // <== main lock 필요
				{
					// loopback 메시지는 압축, 암호화가 적용되어 있으면 메시지가 버려진다. 원본 메시지를 보낸다.
					// enqueue final recv queue and signal
					CMessage payloadMsg;
					payload.ToAssembledMessage(payloadMsg);

					CFinalUserWorkItem workItem;
					CReceivedMessage& ri = workItem.Internal().m_unsafeMessage;
					ri.m_remoteHostID = HostID_Server;
					ri.m_unsafeMessage = payloadMsg;
					ri.m_unsafeMessage.SetReadOffset(0);
					workItem.Internal().ModifyForLoopback();
					TryGetReferrerHeart(workItem.Internal().m_netCoreReferrerHeart);
					if(workItem.Internal().m_netCoreReferrerHeart)
						m_userTaskQueue.Push(m_loopbackHost, workItem);
				}
				else if (SD2.get() != nullptr)
				{
					// remote가 가진 socket을 비파괴 보장 시킨 후 수신 대상 목록만 만든다.
					// 나중에 main lock 후 low context switch loop를 돌면서 각 socket에 대해 멀티캐스트를 한다.
					// PN의 멀티캐스트 관련 서버 성능이 빵빵한 이유가 여기에 있다.
					shared_ptr<CRemoteClient_S> SD3 = LeanDynamicCast_RemoteClient_S(SD2);
					if (SD3)
					{
						// udp Socket이 생성되어있지 않다면, udp Socket 생성을 요청한다.
						RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(SD3); // <== main lock 필요

						if (SD3->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
						{
							// UDP로 보내기
							sendDestList[i].m_socket = SD3->m_ToClientUdp_Fallbackable.m_udpSocket;
							sendDestList[i].mDestAddr = SD3->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
						}
						else // TCP
						{
							// TCP fallback되어 있다. TCP를 대신 사용하자.
							sendDestList[i].m_socket = SD3->m_tcpLayer;

							// NOTE: main lock을 한 상태이어야 한다. 자세한 것은 ACR 명세서 참고.
							AssertIsLockedByCurrentThread();
						}

						unreliableSendInfoList.Add(&(sendDestList[i]));
					}
					else 
					{
						// 보낼 상대가 정작 없다. 에러 처리.
						FillSendFailListOnNeed(sendContext, &SD1.mHostID, 1, ErrorType_InvalidHostID);
						ret = false;
					}
				}
				else
				{
					// 보낼 상대가 정작 없다. 에러 처리.
					FillSendFailListOnNeed(sendContext, &SD1.mHostID, 1, ErrorType_InvalidHostID);
					ret = false;
				}
			}

 			/* main unlock을 하는 이유:
 			이 이후부터는 low context switch loop가 돈다.
 			제아무리 low context라고 해도 다수의 socket들의 send queue lock을 try or block lock을 하기 때문에
 			context switch가 결국 여러 차례 발생한다.
 			따라서 main lock이 많은 시간을 차지할 가능성이 낮게나마 존재한다.
 			이를 없애기 위한 목적이다. */
 			mainlock.Unlock();

			// 각 unreliable message 수신 대상에 대해...
			for (int i = 0; i < unreliableSendInfoList.GetCount(); i++)
			{
				// 이미 위에서 객체가 있음을 보장하기때문에 따로 검사를 수행하지 않는다.
				SendDestInfo* SDOrg = unreliableSendInfoList[i];
				assert(SDOrg != nullptr);
				SendDestInfo* SD1 = SDOrg;
				CRemoteClient_S *SD3 = (CRemoteClient_S*)SD1->mObject.get();

				// 항목의 P2P route prev link가 있으면, 즉 이미 P2P route로 broadcast가 된 상태이다.
				// 그러므로, 넘어간다.
				if (SD1->mP2PRoutePrevLink != nullptr)
				{
					// 아무것도 안함
				}
				else if (SD1->mP2PRouteNextLink != nullptr) // 항목의 P2P next link가 있으면, 
				{
					// link의 끝까지 찾아서 P2P route 메시지 내용물에 추가한다.
					CMessage header;
					header.UseInternalBuffer();
					Message_Write(header, MessageType_S2CRoutedMulticast1);
					Message_Write(header, sendContext.m_priority);
					header.WriteScalar(sendContext.m_uniqueID.m_value);

					POOLED_LOCAL_VAR(HostIDArray, p2pRouteList);

					while (SD1 != nullptr) // 리스트 순회하며...
					{
						p2pRouteList.Add(SD1->mHostID);
						SD1 = SD1->mP2PRouteNextLink;
					}

					header.WriteScalar((int)p2pRouteList.GetCount());
					for (int routeListIndex = 0; routeListIndex < (int)p2pRouteList.GetCount(); routeListIndex++)
					{
						header.Write(p2pRouteList[routeListIndex]);
					}

					// 					printf("S2C routed broadcast to %d: ",SD3->m_HostID);
					// 					for (int i = 0;i < p2pRouteList.GetCount();i++)
					// 					{
					// 						printf("%d ",p2pRouteList[i]);
					// 					}
					// 					printf("\n");

					// sendData의 내용을 추가한다.
					header.WriteScalar(realPayload->GetTotalLength());

					CSendFragRefs s2cRoutedMulticastMsg;
					s2cRoutedMulticastMsg.Add(header);
					s2cRoutedMulticastMsg.Add(*realPayload);

					SendOpt sendContext2(sendContext);
					sendContext2.m_uniqueID.m_relayerSeparator = (int8_t)UniqueID_RelayerSeparator_RoutedMulticast1;
					// hostid_none는 uniqueid로 씹히지 않는다.
					SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
						SDOrg->m_socket,
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
					if (SDOrg->m_socket->GetSocketType() == SocketType_Tcp)
					{
						SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
							SDOrg->m_socket,
							*realPayload,
							sendContext);
					}
					else if (SDOrg->m_socket->GetSocketType() == SocketType_WebSocket)	// WebSocket은 압축과 암호화를 하지않은 데이터를 보낸다.
					{
						SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
							SDOrg->m_socket,
							payload,
							SendOpt());
					}
					else
					{
						SDOrg->m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
							SDOrg->m_socket,
							SD3->GetHostID(),
							FilterTag::CreateFilterTag(HostID_Server, SD3->GetHostID()),
							SDOrg->mDestAddr,
							*realPayload,
							curTime,
							sendContext);
					}
				}

			}

		}

		return ret;
	}


}