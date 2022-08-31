#include "stdafx.h"

#include "NetServer.h"
#include "RemoteClient.h"
#include "RmiContextImpl.h"
#include "SendFragRefs.h"

namespace Proud
{
	// rc가 가진 UDP socket을 모두 정리한다.
	// garbageSocketNow: true이면, UDP socket에 대해 garbage를 호출하고, false이면, recycle을 한다.
	void CNetServerImpl::RemoteClient_CleanupUdpSocket(const shared_ptr<CRemoteClient_S>& rc, bool garbageSocketNow)
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
			rc->m_ownedUdpSocket = nullptr;
		}

		// static assigned or per-remote UDP socket 중 하나를 참조하므로, 그냥 참조 해제만.
		rc->m_ToClientUdp_Fallbackable.m_udpSocket = nullptr;
	}

	void CNetServerImpl::RemoteClient_FallbackUdpSocketAndCleanup(const shared_ptr<CRemoteClient_S>& rc, bool /*garbageSocketNow*/)
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
		const shared_ptr<CRemoteClient_S>& rc,
		ErrorType errorType,
		ErrorType detailType,
		const ByteArray& shutdownCommentFromClient,
		const String& commentFromErrorInfo,
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
		rc->m_tcpLayer->RequestStopIo();

		// NOTE: UDP는 bind to any이므로 재사용해야 함.
		RemoteClient_FallbackUdpSocketAndCleanup(rc, false);

		// note: UDP frag의 packet id 상태는 그대로 유지해야 하므로 ResetPacketFragState 등은 콜 안함
		// 수신 미보장 부분과 HostID도 그대로 갖고 있음. authed 상태를 유지함.

		// 다른 피어들에게 offline이 된 peer가 있음을 노티
		for (CRemoteClient_S::P2PConnectionPairs::iterator i = rc->m_p2pConnectionPairs.begin(); i != rc->m_p2pConnectionPairs.end(); i++)
		{
			shared_ptr<P2PConnectionState> pair = i.GetSecond();
			shared_ptr<CRemoteClient_S> rc2 = pair->GetOppositePeer(rc);
			m_s2cProxy.P2P_NotifyP2PMemberOffline(rc2->m_HostID, g_ReliableSendForPN, rc->m_HostID,
				CompactFieldMap());
		}

		// ACR 완전 실패시 남길 오류 내용을 백업
		rc->m_errorBeforeAutoConnectionRecoveryErrorType = errorType;
		rc->m_errorBeforeAutoConnectionRecoveryDetailType = detailType;
		rc->m_errorBeforeAutoConnectionRecoveryComment = shutdownCommentFromClient;
		rc->m_errorBeforeAutoConnectionRecoveryWhere = where;
		rc->m_errorBeforeAutoConnectionRecoverySocketErrorCode = socketErrorCode;

		// OnClientOffline을 트리거
		LocalEvent e;

		e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
		e.m_errorInfo->m_errorType = errorType;
		e.m_errorInfo->m_detailType = detailType;
		e.m_errorInfo->m_comment = commentFromErrorInfo;
		e.m_netClientInfo = rc->ToNetClientInfo();
		e.m_type = LocalEventType_ClientOffline;
		e.m_byteArrayComment = shutdownCommentFromClient;
		e.m_socketErrorCode = socketErrorCode;
		e.m_remoteHostID = rc->m_HostID;

		EnqueLocalEvent(e, rc);

		// main lock 으로 보호되므로 안전
		rc->m_enquedClientOfflineEvent = true;
	}

	// 재접속을 시도한 TCP 연결로부터 받는다.
	void CNetServerImpl::IoCompletion_ProcessMessage_RequestAutoConnectionRecovery(
		CMessage &msg
		, const shared_ptr<CRemoteClient_S>& rc		// ACR 성공 후 rc의 내용은 orgRC로 넘어가고 rc는 버려진다.
		, const shared_ptr<CSuperSocket>& candidateSocket)
	{
		// 코딩시 주의사항:
		// 재접속 시도하는 클라는, 서버가 아직 remote나 old remote의 상태가 준비중인데 시도하는 경우도 있다.
		// 이러한 경우 잠시 뒤 클라는 재시도할 것이다. 그런데 여기서 '실패했으니 TCP 끊음'을 해버리면
		// 그 재시도 기회를 날려버리게 된다.
		// 상대가 해커이거나 도저히 뭔가 안되는 상황에서만 GarbageSocket을 호출할 것.

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// rc의 상태 변화가 중도에 없어야 하므로 위 main lock을 하고 아래 조건문을 한다.

		if (rc == nullptr	// remote client가 없으면, 처리 불가능.
			|| rc->m_tcpLayer == nullptr // remote는 있는데 TCP socket이 없으면 처리 불가능
			|| candidateSocket == nullptr  // completion된 socket이 null이면 처리 불가능
			|| rc->m_tcpLayer != candidateSocket // completion된 소켓이 정작 새 remote client의 TCP socket이 아니면 처리 의미없음.
			|| candidateSocket->GetSocketType() != SocketType_Tcp)
		{
#ifdef SUPERSOCKET_DETECT_HANG_ON_GARBAGE
			if (rc == nullptr)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #1-1")));
				AtomicIncrement32(&g_rcIsNullACR);
			}
			else if (rc->m_tcpLayer == nullptr)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #1-2")));
				AtomicIncrement32(&g_rcTcpLayerIsNullACR);
			}
			else if (candidateSocket == nullptr)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #1-3")));
				AtomicIncrement32(&g_candidateSocketIsNullACR);
			}
			else if (rc->m_tcpLayer != candidateSocket)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #1-4")));
				AtomicIncrement32(&g_rcTcpLayerIsNotCandidateACR);
			}
			else if (candidateSocket->GetSocketType() != SocketType_Tcp)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #1-5")));
				AtomicIncrement32(&g_candidateIsNotTCPACR);
			}
#endif

			// 수신된 소켓이 온전히 remote가 갖고 있는 것이면 에러를 알려주긴 하자.
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_PermissionDenied);

			assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
			return;
		}

		// 코딩시 주의: 이하부터는 candidateSocket 변수를 쓰지를 말자. 코드 이해에 헷갈리게 한다.

		// 이미 HostID를 가진 authed remote client는 이 메시지를 처리할 수 없다!
		// 현재 rc는 candidate 상태에 있어야 한단 말이다.
		if (rc->m_HostID != HostID_None)
		{
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_PermissionDenied);
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// 메시지 읽기 과정

		// 재접속 시도하는 TCP 연결이 "주장"하는 자신의 hostID.
		HostID candidateClientID = HostID_None;
		if (!msg.Read(candidateClientID))
		{
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_InvalidPacketFormat);
			return;
		}

		if (false == m_enableAutoConnectionRecoveryOnServer)
		{
			// 서버에서 거절하기로 했는데 클라가 ACR request를 보냈다면, 그냥 ACR request를 끊어버리자.
			// 그러면 클라에서는 ACR을 최종 실패 처리 해버린다.
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_InvalidPacketFormat);
			return;
		}

		// credential 값을 읽어들인다.
		ByteArray encryptedCredential;
		if (!msg.Read(encryptedCredential))
		{
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_InvalidPacketFormat);
			return;
		}

		// rc와 이 rc가 ACR 요청의 원본 연결(remote client)는 서로 다르다.
		// 그것을 찾는다.
		shared_ptr<CRemoteClient_S> orgRC = GetAuthedClientByHostID_NOLOCK(candidateClientID);

		// 클라가 이 메시지를 연속으로 보낸 경우 rc->m_tcpLayer may be null.
		// 따라서 이 체크를 하자.
		if (orgRC == nullptr || orgRC == rc)
		{
#ifdef SUPERSOCKET_DETECT_HANG_ON_GARBAGE
			if (orgRC == nullptr)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #2-1")));
				AtomicIncrement32(&g_orgRCisNullACR);
			}
			else if (orgRC == rc)
			{
				EnqueWarning(ErrorInfo::From(ErrorType_PermissionDenied, HostID_None, _PNT("ACR failed #2-2")));
				AtomicIncrement32(&g_orgRCisRcACR);
			}
#endif
			// oldRC의 상태에 문제가 있다. 처리 불가능.
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_PermissionDenied);
			return;
		}

		// 암호화된 Credential을 복호화해서 클라이언트 Credential에 넣는다.
		ByteArray newCredentialBlock;

		if (ErrorInfoPtr err = CCryptoRsa::DecryptSessionKeyByPrivateKey(newCredentialBlock, encryptedCredential, m_selfXchgKey))
		{
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_DecryptFail);
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// 메시지 다 읽었으니 나머지 처리 시작

		// 새 인증키와 구 인증키는 +10 차이 안에만 있어야 한다. 이를 비교한다.
		ByteArray& oldCredentialBlock = orgRC->m_credentialBlock;
		ByteArray expectedCredentialBlock = oldCredentialBlock; // 사본
		bool isEqualCredential = false;

		for (int i = 0; i < 10; i++)
		{
			ByteArray_IncreaseEveryByte(expectedCredentialBlock);

			if (newCredentialBlock.Equals(expectedCredentialBlock))
			{
				isEqualCredential = true;
				break;
			}
		}

		if (!isEqualCredential)
		{
			NotifyAutoConnectionRecoveryFailed(candidateSocket, ErrorType_InvalidSessionKey);
			return;
		}

		/* 정상적인 넷클라의 ACR 인증이 성공했다!

		rc에 대해: rc가 가진 새 TCP socket을 orgRC에 이양. 그리고 rc는 폐기.
		orgRC: 부활! socket to host map에도 갱신하자.

		Q: 이거 하는 동안 다른 스레드에서 rc에 대한 송수신 처리를 하고 있으면 위험하지 않나요?
		A: 안전합니다. rc에 대한 수신 처리는 main lock 후 socket to host map을 통해 rc를 얻은 후 진행되는데
		여기도 main lock 상태이므로 두 처리는 동시에 진행될 수 없기 때문입니다. */

		// 완료되었다는 메시지를 전송하도록 하자.
		// 여기서 NotifyAutoConnectionRecoverySuccess 메시지를 던져줘야 messageId 가 갱신 되지 않는다.
		// 이 패킷까지 no messageId 로 보내야함. AcrMessageRecovery_MoveSocketToSocket는 이거 이후에 해야.
		{
			CMessage msg_;
			msg_.UseInternalBuffer();
			Message_Write(msg_, MessageType_NotifyAutoConnectionRecoverySuccess);
			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(rc->m_tcpLayer, msg_, SendOpt());
		}

		// Offline 이 콜 되지 않았다.
		// 명시적으로 GarbageOrGoOffline 을 orgRC 에 대해서 콜 한다.
		if (orgRC->m_enquedClientOfflineEvent == false)
		{
			RemoteClient_ChangeToAcrWaitingMode(orgRC,
				ErrorType_DisconnectFromRemote,
				ErrorType_ConnectServerTimeout,
				ByteArray(),
				Proud::String(_PNT("")),
				_PNT("NC.RACR"),
				SocketErrorCode_Ok);
		}

		// rc가 갖고 있던 TCP socket을 orgRC로 이양
		shared_ptr<CSuperSocket> socketToDestroy = orgRC->m_tcpLayer;
		orgRC->m_tcpLayer = rc->m_tcpLayer;
		SocketToHostsMap_SetForAnyAddr(orgRC->m_tcpLayer, orgRC);

		// 연관되는 다른 P2P 연결 클라들에게 노티하자.
		for (CRemoteClient_S::P2PConnectionPairs::iterator i = orgRC->m_p2pConnectionPairs.begin(); i != orgRC->m_p2pConnectionPairs.end(); i++)
		{
			shared_ptr<P2PConnectionState> pair = i.GetSecond();
			shared_ptr<CRemoteClient_S> otherRC = pair->GetOppositePeer(orgRC);

			m_s2cProxy.P2P_NotifyP2PMemberOnline(otherRC->m_HostID, g_ReliableSendForPN, orgRC->m_HostID, CompactFieldMap());
		}

		// 백업되었던 AMR을 새것에 넣도록 하자.
		Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		if (socketToDestroy != nullptr)
		{
			CSuperSocket::AcrMessageRecovery_MoveSocketToSocket(socketToDestroy, orgRC->m_tcpLayer);

			// 구 TCP socket을 제거
			GarbageSocket(socketToDestroy);
			socketToDestroy = nullptr;
		}

		// rc 의 tcpLayer 는 이미 orgRC 에게 이양 되었으므로 참조 해제
		// (이걸 안하면, rc를 garbage collect할 때 orgRC가 이미 가져간 것을 지워버리는 사태 발생.
		// 아니면... flag를 하나 더 두어, garbage를 생략하게 할까?)
		rc->m_tcpLayer = nullptr;

		// 임시로 쓰였던 rc를 garbage한다.
		GarbageHost(rc,
			ErrorType_Ok,
			ErrorType_Ok,
			ByteArray(),
			_PNT("ACR candidate connection has been handed over to the original connection."),
			SocketErrorCode_Ok);

		// 클라로 부터 받은 인증키로 변경. copy.
		orgRC->m_credentialBlock = newCredentialBlock;

		// NOTE: AES session key +1만 한 후 인증을 하므로, 불필요하게 fast key를 갖고 그럴 필요는 없다.
		// 괜히 RSA decrypt 연산량만 낭비다.
		// 따라서 fast key에 대해서는 갱신할 것이 없다.

		// ACR 재연결 받기가 성공했으므로 시한부를 해제하자.
		orgRC->m_autoConnectionRecoveryWaitBeginTime = 0;

		// orgRC의 스피드핵 감지 기능이 있었다면 초기화하자. 재접속을 하느라 메롱 상태였던 orgRC에게 스피드핵 기능은 별 의미가 없다.
		if (orgRC->m_speedHackDetector != nullptr)
		{
			orgRC->m_speedHackDetector.Free();
			orgRC->m_speedHackDetector.Attach(new CSpeedHackDetector);
		}

		// heartbeat에서 candidate 이양 처리를 하라고 지시.
		//rc->m_acrCandidateID = candidateClientID;

		// 수신미보장 부분(메시지들의 집합)을 재송신을 걸자.
		// 당연히, 이것들에 대해서 ack가 오면 제거될 것이므로 여기서 제거하지는 말자.
		orgRC->m_tcpLayer->AcrMessageRecovery_ResendUnguarantee(orgRC->m_tcpLayer);

		// 자, 이제 orgRC는 새 TCP socket를 가졌다.
		// 아직 orgRC는 비파괴 보장이므로 안전

		// 유저에게 OnClientOnline 이벤트 트리거
		LocalEvent e;
		e.m_type = LocalEventType_ClientOnline;
		e.m_remoteHostID = orgRC->m_HostID;

		EnqueLocalEvent(e, orgRC);
	}

	// 너무 오래 기다려도 ACR waiting mode에서 ACR finish로 이어지지 못하면 님 디스요.
	void CNetServerImpl::Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(const shared_ptr<CRemoteClient_S>& rc)
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		if (rc->m_autoConnectionRecoveryWaitBeginTime != 0
			&& currTime - rc->m_autoConnectionRecoveryWaitBeginTime > rc->m_autoConnectionRecoveryTimeoutMs)	// 로미오 줄리엣 버그를 예방차, 클라측 대기 시간이 +5초
		{
			rc->m_autoConnectionRecoveryWaitBeginTime = 0;
			GarbageHost(rc,
				rc->m_errorBeforeAutoConnectionRecoveryErrorType,
				rc->m_errorBeforeAutoConnectionRecoveryDetailType,
				rc->m_errorBeforeAutoConnectionRecoveryComment,
				rc->m_errorBeforeAutoConnectionRecoveryWhere.GetString(),
				rc->m_errorBeforeAutoConnectionRecoverySocketErrorCode);
		}
	}

	void CNetServerImpl::NotifyAutoConnectionRecoveryFailed(const shared_ptr<CSuperSocket>& socket, ErrorType reason)
	{
		CMessage msg;
		msg.UseInternalBuffer();
		Message_Write(msg, MessageType_NotifyAutoConnectionRecoveryFailed);
		Message_Write(msg, reason);
		socket->AddToSendQueueWithSplitterAndSignal_Copy(socket, msg, SendOpt());
	}

	// MessageID를 받은 적이 있으면 ack를 보내준다.
	void CNetServerImpl::Heartbeat_AcrSendMessageIDAckOnNeed(const shared_ptr<CRemoteClient_S>& /*rc*/)
	{
		// 해당 부분은 RMI C2S Ping - Pong 에 구현 되어 있다. 참고 바람
		// by. hyeonmin.yoon

//		int ackID;
//		if (rc->m_tcpLayer && rc->m_tcpLayer->AcrMessageRecovery_PeekMessageIDToAck(&ackID))
//		{
//			CMessage header;
//			header.UseInternalBuffer();
//			Message_Write(header, MessageType_MessageIDAck);
//			header.Write(ackID);
//
//			CSendFragRefs sd;
//			sd.Add(header);
//
//			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(sd, SendOpt());
//		}
	}

	void CNetServerImpl::SetDefaultAutoConnectionRecoveryTimeoutTimeMs(int newValInMs)
	{
		AssertIsNotLockedByCurrentThread();

		if (newValInMs < CNetConfig::AutoConnectionRecoveryTimeoutMinTimeMs)
		{
			ShowUserMisuseError(_PNT("Too short auto connection recovery value. It may cause false-positive disconnection."));
			return;
		}

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (m_listening == true)
		{
			clk.Unlock();
			AssertIsNotLockedByCurrentThread();
			ShowUserMisuseError(_PNT("Can only be used before calling CNetServer.Start."));
			return;
		}

		m_settings.m_autoConnectionRecoveryTimeoutTimeMs = newValInMs;
	}

	void CNetServerImpl::SetAutoConnectionRecoveryTimeoutTimeMs(HostID host, int newValInMs)
	{
		AssertIsNotLockedByCurrentThread();

		if (newValInMs < CNetConfig::AutoConnectionRecoveryTimeoutMinTimeMs)
		{
			ShowUserMisuseError(_PNT("Too short auto connection recovery value. It may cause false-positive disconnection."));
			return;
		}

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (m_listening == false)
		{
			clk.Unlock();
			AssertIsNotLockedByCurrentThread();
			ShowUserMisuseError(_PNT("Can only be used after calling CNetServer.Start."));
			return;
		}

		shared_ptr<CHostBase> hb;
		if (m_authedHostMap.TryGetValue(host, hb) == true)
		{
			if (hb->GetHostID() == HostID_Server)
			{
				return;
			}

			shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(hb);
			if (rc->m_garbaged)
			{
				return;
			}

			// 실제 리모트 클라이언트 객체에 적용
			rc->m_autoConnectionRecoveryTimeoutMs = newValInMs;

			// 클라에게 적용
			m_s2cProxy.NotifyChangedAutoConnectionRecoveryTimeoutTimeMs(host, g_ReliableSendForPN, newValInMs);
		}
	}
}
