#include "stdafx.h"

#include "NetServer.h"
#include "RemoteClient.h"
#include "RmiContextImpl.h"
#include "SendFragRefs.h"

namespace Proud
{
	// rc가 가진 UDP socket들을 모두 정리한다.
	// garbageSocketNow: true이면, UDP socket에 대해 garbage를 호출하고, false이면, recycle을 한ㄷ.
	void CNetServerImpl::RemoteClient_CleanupUdpSocket(CRemoteClient_S* rc, bool garbageSocketNow)
	{
		// to client udp 소켓 처리
		if (rc->m_ownedUdpSocket != NULL)
		{
			/* 이 소켓이 NULL 이 아니면 per-client 모드 세팅이 된 해당 rc 전용 udp 소켓이다. 이건 recycle 시켜야 한다. 
			Q: 어차피 재사용 안하는데 garbage해야 하지 않나요?
			A: UDP socket을 바로 죽이면 뒤늦게 도착하는 UDP 데이터에 대한 ICMP unreachable이 대량으로 remote에게 전송됩니다.
			이때 remote측의 방화벽이 바보인 경우 서버를 DDOS 주체로 간주하고 차단하는 경우가 드물게 있습니다. */
			if (!garbageSocketNow && rc->GetHostID() != HostID_None)
			{
				UdpSocketToRecycleBin(rc->GetHostID(), rc->m_ownedUdpSocket, CNetConfig::ServerUdpSocketLatentCloseMs);
			}
			else
			{
				GarbageSocket(rc->m_ownedUdpSocket);
			}

			// 아래에서 udp 소켓 모두 참조 해제 main lock 이니까 안전!
			// per-client 용 소켓 참조 해제
			rc->m_ownedUdpSocket = CSuperSocketPtr();
		}

		// static assigned or per-remote UDP socket 중 하나를 참조하므로, 그냥 참조 해제만.
		rc->m_ToClientUdp_Fallbackable.m_udpSocket = CSuperSocketPtr();
	}

	void CNetServerImpl::RemoteClient_FallbackUdpSocketAndCleanup(CRemoteClient_S* rc, bool garbageSocketNow)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// 클섭 TCP fallback
		SecondChanceFallbackUdpToTcp(rc);

		// 피어 relay fallback. 다른 피어들에게 릴레이로 모두 전환하라고 지시도.
		// 이걸 별도로 보내야 유저 입장에서는 ACR 과정에서의 릴레이 콜백도 여전히 받을 수 있으니 필수.
		// 그리고, 꼭 이렇게 해야만 한다. 서버,클라 둘다. 안그러면 Reliable P2P UDP간 resend가 마구 발생해버림.
		FallbackP2PToRelay(rc, ErrorType_TCPConnectFailure);

		// 위쪽에서 모두 fallback 처리 되었으므로 아래에서 udp 소켓을 모두 처리 해도 됨

		// to-client UDP 소켓 정리. 
		// per-client UDP인 경우, socket을 garbage가 아닌 recycle을 한다. 
		// 홀펀칭 다시 할 때 NAT 포트 매핑이 더 생기는 것을 줄이기 위함이다.
		RemoteClient_CleanupUdpSocket(rc, false);
	}

	/* remote client를 ACR 대기 상태로 바꾼다.
	TCP socket도 닫아 버린다.
	Q: TCP receive timeout인 경우, 혹시 다시 극적으로 살아날 수 있지 않나요?
	A: 물론 그럴 가능성도 있겠지만, 
	여기까지 올 지경이면 사실상 충분히 기다릴 만큼 기다렸음에도 불구하고 라는 상황입니다. */
	void CNetServerImpl::RemoteClient_ChangeToAcrWaitingMode(
		CRemoteClient_S *rc,
		ErrorType errorType,
		ErrorType detailType, 
		const ByteArray& comment, 
		const PNTCHAR* where, 
		SocketErrorCode socketErrorCode)
	{
		AssertIsLockedByCurrentThread();

		// 심플 패킷 모드에선 ACR 작동 안함
		if (IsSimplePacketMode() == true)
			return;

		// 중복해서 waiting mode로 들어감 안됨
		if (rc->m_autoConnectionRecoveryWaitBeginTime != 0)
		{
			return;
		}

		// ACR을 기다리기 시작한 시간을 기재
		rc->m_autoConnectionRecoveryWaitBeginTime = GetPreciseCurrentTimeMs();

		// TCP socket을 닫되, garbage하지는 않는다. dead TCP socket의 AMR을 계속 냅두어야 하기 때문이다.
		// dead TCP socket의 AddToSendQueue가 계속해서 호출될테니까.
		// 왜 그래야 하는지는 TCP용 AddToSendQueue 함수 내 주석에.
		rc->m_tcpLayer->CloseSocketOnly();

		// NOTE: UDP는 bind to any이므로 재사용해야 함.
		RemoteClient_FallbackUdpSocketAndCleanup(rc, false);

		// note: UDP frag의 packet id 상태는 그대로 유지해야 하므로 ResetPacketFragState 등은 콜 안함
		// 수신 미보장 부분과 HostID도 그대로 갖고 있음. authed 상태를 유지함.

		// 다른 피어들에게 offline이 된 peer가 있음을 노티
		for (CRemoteClient_S::P2PConnectionPairs::iterator i = rc->m_p2pConnectionPairs.begin(); i != rc->m_p2pConnectionPairs.end(); i++)
		{
			P2PConnectionState* pair = *i;
			CRemoteClient_S* rc2 = pair->GetOppositePeer(rc);
			m_s2cProxy.P2P_NotifyP2PMemberOffline(rc2->m_HostID, g_ReliableSendForPN, rc->m_HostID);
		}

		// ACR 완전 실패시 남길 오류 내용을 백업
		rc->m_errorBeforeAutoConnectionRecoveryErrorType = errorType;
		rc->m_errorBeforeAutoConnectionRecoveryDetailType = detailType;
		rc->m_errorBeforeAutoConnectionRecoveryComment = comment;
		rc->m_errorBeforeAutoConnectionRecoveryWhere = where;
		rc->m_errorBeforeAutoConnectionRecoverySocketErrorCode = socketErrorCode;

		// OnClientOffline을 트리거
		if (m_eventSink_NOCSLOCK != NULL && rc->m_enquedClientOfflineEvent == false)
		{
			LocalEvent e;

			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_errorInfo->m_errorType = ErrorType_TCPConnectFailure;
			e.m_netClientInfo = rc->ToNetClientInfo();
			e.m_type = LocalEventType_ClientOffline;
			e.m_byteArrayComment = comment;
			e.m_socketErrorCode = socketErrorCode;
			e.m_remoteHostID = rc->m_HostID;

			EnqueLocalEvent(e, rc);

			// main lock 으로 보호되므로 안전
			rc->m_enquedClientOfflineEvent = true;
		}
	}

	// 재접속을 시도한 TCP 연결로부터 받는다.
	void CNetServerImpl::IoCompletion_ProcessMessage_RequestAutoConnectionRecovery
		(CMessage &msg, CRemoteClient_S *rc)
	{
		assert(rc != NULL);
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		//////////////////////////////////////////////////////////////////////////
		// 메시지 읽기 과정

		// 재접속 시도하는 TCP 연결이 "주장"하는 자신의 hostID.
		HostID candidateClientID = HostID_None;
		if (!msg.Read(candidateClientID))
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_InvalidPacketFormat);
			return;
		}

		// ACR 설정을 켠 클라인지?
		ByteArray encryptedAESKeyBlob;
		if (!msg.Read(encryptedAESKeyBlob))
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_InvalidPacketFormat);
			return;
		}

		// 이미 HostID를 가진 authed remote client는 이 메시지를 처리할 수 없다!
		// 현재 rc는 candidate 상태에 있어야 한단 말이다.
		if (rc->m_HostID != HostID_None)
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_PermissionDenied);
			return;
		}

		// rc와 이 rc가 ACR 요청의 원본 연결(remote client)는 서로 다르다. 
		// 그것을 찾는다.
		CRemoteClient_S* orgRC = GetAuthedClientByHostID_NOLOCK(candidateClientID);
		if (orgRC == NULL || orgRC == rc)
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_PermissionDenied);
			return;
		}

		// 암호화된 세션키를 복호화해서 클라이언트 세션키에 넣는다.
		ByteArray newSessionKeyBlock;

		if (ErrorInfoPtr err = CCryptoRsa::DecryptSessionKeyByPrivateKey(newSessionKeyBlock, encryptedAESKeyBlob, m_selfXchgKey))
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_DecryptFail);
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// 메시지 다 읽었으니 나머지 처리 시작

		// 새 세션키와 구 세션키는 +1 차이만 있어야 한다. 이를 비교한다.
		ByteArray& oldSessionKeyBlock = orgRC->m_sessionKey.m_aesKeyBlock;
		ByteArray expectedSessionKeyBlock = oldSessionKeyBlock; // 사본 

		ByteArray_IncreaseEveryByte(expectedSessionKeyBlock);

		if (!newSessionKeyBlock.Equals(expectedSessionKeyBlock))
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_InvalidSessionKey);
			return;
		}

		// 세션키 블럭은 매치가 됐다. 이제 세션키 블럭으로부터 세션키를 가져오자.
		CCryptoAesKey newSessionKey;
		if (CCryptoAes::ExpandFrom(newSessionKey, newSessionKeyBlock.GetData(), m_settings.m_encryptedMessageKeyLength / 8) != true)
		{
			NotifyAutoConnectionRecoveryFailed(rc, ErrorType_InvalidSessionKey);
			return;
		}

		/* 정상적인 넷클라의 ACR 인증이 성공했다!

		rc에 대해: rc가 가진 새 TCP socket을 orgRC에 이양. 그리고 rc는 폐기.
		orgRC: 부활! socket to host map에도 갱신하자.

		Q: 이거 하는 동안 다른 스레드에서 rc에 대한 송수신 처리를 하고 있으면 위험하지 않나요?
		A: 안전합니다. rc에 대한 수신 처리는 main lock 후 socket to host map을 통해 rc를 얻은 후 진행되는데
		여기도 main lock 상태이므로 두 처리는 동시에 진행될 수 없기 때문입니다.
		*/
		CSuperSocketPtr oldTcpSocket = orgRC->m_tcpLayer;
		CSuperSocketPtr newTcpSocket = rc->m_tcpLayer;

		// 서버측에서 ACR 과정이 완료됐다. 완료되었다는 메시지를 전송하도록 하자.
		//
		// 여기서 NotifyAutoConnectionRecoverySuccess 메시지를 던져줘야 messageId 가 갱신 되지 않는다.
		// 이 패킷까지 no messageId 로 보내야함. AcrMessageRecovery_MoveSocketToSocket는 이거 이후에 해야.
		{
			CMessage msg;
			msg.UseInternalBuffer();
			msg.Write((char)MessageType_NotifyAutoConnectionRecoverySuccess);
			newTcpSocket->AddToSendQueueWithSplitterAndSignal_Copy(msg, SendOpt());
		}

		// Offline 이 콜 되지 않았다.
		// 명시적으로 GarbageOrGoOffline 을 orgRC 에 대해서 콜 한다.
		if (orgRC->m_enquedClientOfflineEvent == false)
		{
			RemoteClient_ChangeToAcrWaitingMode(orgRC,
												ErrorType_DisconnectFromRemote,
												ErrorType_ConnectServerTimeout,
												ByteArray(),
												_PNT("NC.RACR"),
												SocketErrorCode_Ok);
		}

		// rc가 갖고 있던 TCP socket을 orgRC로 이양
		orgRC->m_tcpLayer = newTcpSocket;
		SocketToHostsMap_SetForAnyAddr(orgRC->m_tcpLayer, orgRC);

		// 연관되는 다른 P2P 연결 클라들에게 노티하자.
		for (CRemoteClient_S::P2PConnectionPairs::iterator i = orgRC->m_p2pConnectionPairs.begin(); i != orgRC->m_p2pConnectionPairs.end(); i++)
		{
			P2PConnectionState* pair = *i;
			CRemoteClient_S* otherRC = pair->GetOppositePeer(orgRC);

			m_s2cProxy.P2P_NotifyP2PMemberOnline(otherRC->m_HostID, g_ReliableSendForPN, orgRC->m_HostID);
		}

		// 백업되었던 AMR을 새것에 넣도록 하자.
		Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		CSuperSocket::AcrMessageRecovery_MoveSocketToSocket(oldTcpSocket, newTcpSocket);

		// 구 TCP socket을 제거
		GarbageSocket(oldTcpSocket);
		oldTcpSocket = CSuperSocketPtr();

		// rc 의 tcpLayer 는 이미 orgRC 에게 이양 되었으므로 참조 해제
		// (이걸 안하면, rc를 garbage collect할 때 orgRC가 이미 가져간 것을 지워버리는 사태 발생.
		// 아니면... flag를 하나 더 두어, garbage를 생략하게 할까?)
		rc->m_tcpLayer = CSuperSocketPtr();

		// 임시 rc 를 garbage
		GarbageHost(rc,
					ErrorType_Ok,
					ErrorType_Ok,
					ByteArray(),
					_PNT("ACR candidate connection has been handed over to the original connection."),
					SocketErrorCode_Ok);

		// AES key가 +1된 것으로 변경되었다.
		orgRC->m_sessionKey.m_aesKeyBlock = newSessionKeyBlock;	// copy
		orgRC->m_sessionKey.m_aesKey = newSessionKey;

		// NOTE: AES session key +1만 한 후 인증을 하므로, 불필요하게 fast key를 갖고 그럴 필요는 없다.
		// 괜히 RSA decrypt 연산량만 낭비다.
		// 따라서 fast key에 대해서는 갱신할 것이 없다.

		orgRC->m_autoConnectionRecoveryWaitBeginTime = 0; // ACR 재연결 받기가 성공했으므로 시한부를 해제하자.

		// heartbeat에서 candidate 이양 처리를 하라고 지시.
		//rc->m_acrCandidateID = candidateClientID;

		// 수신미보장 부분(메시지들의 집합)을 재송신을 걸자. 
		// 당연히, 이것들에 대해서 ack가 오면 제거될 것이므로 여기서 제거하지는 말자.
		orgRC->m_tcpLayer->AcrMessageRecovery_ResendUnguarantee();

		// 자, 이제 orgRC는 새 TCP socket를 가졌다. 
		// 아직 orgRC는 비파괴 보장이므로 안전 

		// 유저에게 OnClientOnline 이벤트 트리거
		LocalEvent e;
		e.m_type = LocalEventType_ClientOnline;
		e.m_remoteHostID = orgRC->m_HostID;

		EnqueLocalEvent(e, orgRC);
	}

	// 너무 오래 기다려도 ACR waiting mode에서 ACR finish로 이어지지 못하면 님 디스요.
	void CNetServerImpl::Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(CRemoteClient_S* rc)
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		if (rc->m_autoConnectionRecoveryWaitBeginTime != 0
			&& currTime - rc->m_autoConnectionRecoveryWaitBeginTime > 60 * 1000)	// 로미오 줄리엣 버그를 예방차, 클라측 대기 시간이 +5초
		{
			rc->m_autoConnectionRecoveryWaitBeginTime = 0;
			GarbageHost(rc,
				rc->m_errorBeforeAutoConnectionRecoveryErrorType,
				rc->m_errorBeforeAutoConnectionRecoveryDetailType,
				rc->m_errorBeforeAutoConnectionRecoveryComment,
				rc->m_errorBeforeAutoConnectionRecoveryWhere,
				rc->m_errorBeforeAutoConnectionRecoverySocketErrorCode);
		}
	}

	void CNetServerImpl::NotifyAutoConnectionRecoveryFailed(CRemoteClient_S * rc, ErrorType reason)
	{
		CMessage msg;
		msg.UseInternalBuffer();
		msg.Write((char)MessageType_NotifyAutoConnectionRecoveryFailed);
		msg.Write((int)reason);
		rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(msg, SendOpt());
	}

	// MessageID를 받은 적이 있으면 ack를 보내준다.
	void CNetServerImpl::Heartbeat_AcrSendMessageIDAckOnNeed(CRemoteClient_S* rc)
	{
		// 해당 부분은 RMI C2S Ping - Pong 에 구현 되어 있다. 참고 바람 
		// by. hyeonmin.yoon

// 		int ackID;
// 		if (rc->m_tcpLayer && rc->m_tcpLayer->AcrMessageRecovery_PeekMessageIDToAck(&ackID))
// 		{
// 			CMessage header;
// 			header.UseInternalBuffer();
// 			header.Write((char)MessageType_MessageIDAck);
// 			header.Write(ackID);
// 
// 			CSendFragRefs sd;
// 			sd.Add(header);
// 
// 			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(sd, SendOpt());
// 		}
	}

}
