#include "stdafx.h"
#include "../include/BasicTypes.h"
#include "NetClient.h"
#include "acrimpl_c.h"
#include "acrimpl.h"
#include "STLUtil.h"
#include "LeanDynamicCast.h"
#include "SendFragRefs.h"

namespace Proud
{
	CAutoConnectionRecoveryContext::CAutoConnectionRecoveryContext(CNetClientImpl* owner)
	{
		m_owner = owner;

		m_waitingForNetworkAddressAvailable = true;

		int64_t x = GetPreciseCurrentTimeMs() + m_owner->m_autoConnectionRecoveryDelayMs;
		m_firstStartTime = x;
		m_startTime = x;
	}

	CAutoConnectionRecoveryContext::~CAutoConnectionRecoveryContext()
	{
		// 재접속이 성공하면 m_tcpSocket은 null로 변경된다.
		// 따라서 여기서 다 청소해 버리자.
		if (m_tcpSocket)
		{
			m_owner->AutoConnectionRecovery_GarbageTempRemoteServerAndSocket();
		}
	}

	// 새 로컬 네트워크 주소가 할당되었는지를 검사한다.
	// 스마트폰의 특징: Wifi<=>셀룰러 전환시, 몇초간은 주소가 없거나 있더라도 아래 무효 3가지 주소가 나오다가, 새 네트워크 주소가 등장한다.
	// isTransitionUnicastAddress == true -> 네트워크 주소가 변환 되는것으로 전환 체크를 할때 여기까지 오면 네트워크 전환이 되었다는것이다.
	// isTransitionUnicastAddress == false -> 단순히 유니캐스트 주소가 나옴으로 전환 체크를 할 경우 여기까지 오면 네트워크 전환이 안되었다는것이다.
	bool CNetClientImpl::IsNetworkTransiting()
	{
		CFastArray<String> addresses;
		GetCachedLocalIpAddressesSnapshot(addresses);

		if (addresses.IsEmpty())
			return false;

		String newNetworkAddress;

		// 실제 네트워크 주소가 변환 되는것으로 전환 체크를 할 경우
		for (int i = 0; i < addresses.GetCount(); ++i)
		{
			// 루프백과 Any 주소는 비교 대상에서 제외
			if (!CNetUtil::IsAddressPhysical(addresses[i])
				|| addresses[i].Compare(m_oldUnicastIpAddress.GetString()) == 0)
			{
				continue;
			}
			else
			{
				newNetworkAddress = addresses[i];
			}
		}

		if (m_oldUnicastIpAddress.IsEmpty() == false &&
			newNetworkAddress.IsEmpty() == false &&
			newNetworkAddress.Compare(m_oldUnicastIpAddress) != 0)
		{
			m_oldUnicastIpAddress = newNetworkAddress;
			return true;
		}
		return false;
	}

	bool CNetClientImpl::IsNetworkAddressAvailable()
	{
		CFastArray<String> addresses;
		GetCachedLocalIpAddressesSnapshot(addresses);

		for (int i = 0; i < addresses.GetCount(); ++i)
		{
			if (!CNetUtil::IsAddressPhysical(addresses[i]))
			{
				StringA str = StringA::NewFormat("continue %s", StringT2A(addresses[i]).GetString());
				CFakeWin32::OutputDebugStringA(str.GetString());
				continue;
			}
			else
			{
				StringA str = StringA::NewFormat("return true %s", StringT2A(addresses[i]).GetString());
				CFakeWin32::OutputDebugStringA(str.GetString());

				// 실제 로컬 주소를 찾았다.
				// true를 리턴한다.
				return true;
			}
		}

		return false;
	}

	// 서버,피어와의 모든 UDP socket을 폐기한다.
	void CNetClientImpl::AutoConnectionRecovery_CleanupUdpSocket(const shared_ptr<CHostBase>& host)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (host == m_remoteServer)
		{
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				SocketToHostsMap_RemoveForAnyAddr(m_remoteServer->m_ToServerUdp);
				GarbageSocket(m_remoteServer->m_ToServerUdp);
				m_remoteServer->m_ToServerUdp = shared_ptr<CSuperSocket>();
			}
		}
		else
		{
			shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(host);
			if (peer != nullptr && peer->m_udpSocket != nullptr)
			{
				SocketToHostsMap_RemoveForAnyAddr(peer->m_udpSocket);
				peer->m_udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(peer->m_UdpAddrFromServer);
				GarbageSocket(peer->m_udpSocket);
				peer->m_udpSocket = shared_ptr<CSuperSocket>();

				// JIT 홀펀칭 관련 변수들 초기화
				// (재접속 됬을때 자동으로 홀펀칭 할 수 있도록...)
				peer->m_jitDirectP2PNeeded = false;
				peer->m_jitDirectP2PTriggered = false;
				peer->m_newP2PConnectionNeeded = false;
			}
		}
	}

	// ACR 상태를 시작한다. 즉 재접속 과정을 시작한다.
	// 리턴값이 없는 대신 이 함수의 콜러에서 ACR context 존재 여부로 성공 여부를 판단하자.
	void CNetClientImpl::StartAutoConnectionRecovery(const ErrorInfo& errorInfo)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 이미 시작 시도했다가 실패한 경우를 위해
		if (!m_enableAutoConnectionRecovery)
			return;

		// 심플 패킷 모드에선 ACR 작동 안함
		if (IsSimplePacketMode() == true)
			return;

		// ACR 기능이 활성중이면 스킵. 아니면 활성화하자.
		if (m_autoConnectionRecoveryContext != nullptr)
			return;

		// 본 연결 자체도 사실상 연결 성사가 안된 상태이면 즐
		// 드물지만 이러한 경우도 이론적으로 가능
		if (GetVolatileLocalHostID() == HostID_None || !m_toServerSessionKey->m_aesKey.KeyExists())
			return;

		// 모든 UDP 를 fallback 시킨다.
		// ACR 이 시작 됬다는거 자체가 현재 네트워크 상황이 안좋다는 뜻
		FallbackParam s;
		s.m_notifyToServer = false;
		s.m_resetFallbackCount = true;
		s.m_reason = ErrorType_ServerUdpFailed;

		FirstChanceFallbackServerUdpToTcp(s);
		FirstChanceFallbackEveryPeerUdpToTcp(s);

		/* 기존 TCP 소켓을 못 쓰게 한다.[1] 더 이상 통신을 못하게. 단, garbage는 안한다. 이것이 가진 AMR을 재접속시 보존하기 위해서다.
		dead TCP socket의 AddToSendQueueWithSplitter가 계속해서 호출될 때 구 TCP socket이 있어야 한다.
		재접속이 최종 실패하게 되면 그때 TCP socket은 OnHostGarbaged에 의해 garbage collect된다.
		[1]: close를 하면 다른 스레드에서 new socket이 재수없게 같은 handle값일 수 있다.
		close socket은 최후의 스레드가 하는 것이 원칙이므로 즉 garbage collect를 할 때 해야 하므로 여기서는 안한다. */
		m_remoteServer->m_ToServerTcp->RequestStopIo();

		// to server 의 udp socket 을 recycle & 참조 해제
		AutoConnectionRecovery_CleanupUdpSocket(m_remoteServer);

		// peer 들의 udp socket 을 recycle & 참조 해제
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());
			if (peer != nullptr)
			{
				AutoConnectionRecovery_CleanupUdpSocket(peer);
			}
		}

		// 연결 offline이 되었음을 유저에게 노티
		LocalEvent e;
		e.m_type = LocalEventType_ServerOffline;
		e.m_remoteHostID = HostID_Server;

		// caller가 전달해준 ErrorInfo가 caller의 stack에 있는 경우도 있다.
		e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
		// Deep Copy는 필요가 없다.
		*e.m_errorInfo = errorInfo;

		EnqueLocalEvent(e, m_remoteServer);

		// ACR 진행중 상태 객체를 생성
		m_autoConnectionRecoveryContext = CAutoConnectionRecoveryContextPtr(new CAutoConnectionRecoveryContext(this));
		m_autoConnectionRecoveryContext->m_owner = this;
	}

	/* 재접속 후보 socket의 TCP 연결이 성공이나 실패한 직후의 처리를 한다.
	성공시, 인증 메시지 보내기 등을 수행한다.
	만약 이 함수가 false이 경우 caller에서 GarbageSocket and 참조 해제를 해야 한다.
	당연히, 해당하는 TCP 연결 후보도 제거해야 하고. */
	bool CNetClientImpl::AutoConnectionRecovery_OnTcpConnection(shared_ptr<CSuperSocket> acrTcpSocket)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		assert(acrTcpSocket != m_remoteServer->m_ToServerTcp);

		// 성공한 연결이 후보와 같아야
		if (m_autoConnectionRecoveryContext->m_tcpSocket != acrTcpSocket)
		{
			return false;
		}

		if (m_worker->GetState() != CNetClientWorker::Connected // 기존에 연결 잘되던 NC가 아닌데
			|| acrTcpSocket->m_fastSocket->Tcp_Send0ByteForConnectivityTest() != SocketErrorCode_Ok // TCP 연결이 무효 즉 실패이면
			|| m_autoConnectionRecoveryContext == nullptr // ACR 과정 안하고 있으면
			|| m_toServerSessionKey->m_aesKeyBlock.GetCount() == 0) // 일전에 암호키 교환한 적 없으면
		{
			return false;
		}

		assert(!acrTcpSocket->m_fastSocket->StopIoRequested());

		// Temp RemoteServer 를 생성함과 동시에 HostMap 에 등록하자
		// (ACR 용 Socket 이 프라우드넷 Layer 를 처리하기 위함)
		// 주의!! 여기서 생성된 Temp RemoteServer 객체는 RecoverySuccess 메시지에서 orgRemoteServer 와 교체된 뒤, 삭제 되어야 한다.
		shared_ptr<CSuperSocket>& conn = m_autoConnectionRecoveryContext->m_tcpSocket;
		shared_ptr<CRemoteServer_C> tempRemoteServer = shared_ptr<CRemoteServer_C>(new CRemoteServer_C(this, conn));

		// 해당 임시 객체를 garbage 요청하면 엉뚱한 원본 remoteServer 를 삭제 할 수 있기 때문에
		// tempRemoteServer 는 None 이다.
		tempRemoteServer->m_HostID = HostID_None;

		SocketToHostsMap_SetForAnyAddr(conn, tempRemoteServer);

		// tempRemoteServer 객체를 temporaryRemoteServers list 에 추가
		m_autoConnectionRecovery_temporaryRemoteServers.Add(tempRemoteServer.get(),
			tempRemoteServer);

		// 여기까지 왔으면, 재접속 중인 후보라는 셈.
		// TCP 연결이 성공했다. 다른 connection들은 이제 필요없으니 모두 지우자.
		// 어차피 지워야 한다. 안그러면 불필요한 session key 암복호화 과정이 들어가니까.


		// 서버에 보낼 새 credential(구 Credential+1)을 만든다.
		// 1가지 key만 하도록 하자. 나머지 key는 예전 것을 그대로 재사용해도 된다.
		// 1가지 +1된 key만 갖고도 얼마든지 ACR 연결을 인증하는데 충분하다.
		// 괜히 쓸데없이 서버에서 RSA decryption으로 시간 쓰지 말자.
		assert(m_toServerSessionKey->m_aesKeyBlock.GetCount() > 0);
		assert(m_toServerSessionKey->m_fastKeyBlock.GetCount() > 0);
		assert(m_credentialBlock.GetCount() > 0);

		m_toServerSessionKey->CopyTo(*m_acrCandidateSessionKey);

		// Credential만 +1.
		ByteArray_IncreaseEveryByte(m_credentialBlock);

		ByteArray encryptedCredential;

		// 기존 공개키로 암호화해서 보내자.
		if (!CCryptoRsa::EncryptSessionKeyByPublicKey(
			encryptedCredential,
			m_credentialBlock,
			m_publicKeyBlob))
		{
			// 실패하면 막장. ACR 과정을 중단한다.
			EnqueueDisconnectionEvent(ErrorType_AutoConnectionRecoveryFailed);
			m_worker->SetState(CNetClientWorker::Disconnecting);
			return false;
		}

		assert(!acrTcpSocket->m_fastSocket->StopIoRequested());

		// 최종 단계로, first issue recv를 걸자.
		// NOTE: ACR socket이 connect 과정이 성공했으니 이것을 시작하는거임.
#if defined(_WIN32)
		acrTcpSocket->IssueRecv(acrTcpSocket, true);
#endif

		// ACR을 위한 첫번째 메시지 송신.
		CMessage sendMsg;
		sendMsg.UseInternalBuffer();
		Message_Write(sendMsg, MessageType_RequestAutoConnectionRecovery);
		sendMsg.Write(GetVolatileLocalHostID());
		sendMsg.Write(encryptedCredential);

		acrTcpSocket->AddToSendQueueWithSplitterAndSignal_Copy(acrTcpSocket, CSendFragRefs(sendMsg), SendOpt());
		return true;
	}

	// ACR 연결 후 인증 과정이 모두 마침.
	// ACR 연결 후보 중 하나 즉 생존한 TCP connection(m_surviveConnection)가 이 메시지를 받는다.
	// 여기서 to-server TCP를 생존한 m_surviveConnection 로 교체해 버린다.
	void CNetClientImpl::ProcessMessage_NotifyAutoConnectionRecoverySuccess(CMessage& /*msg*/)
	{
#if defined(_WIN32)
		LockMain_AssertIsLockedByCurrentThread();
#endif
		CAutoConnectionRecoveryContextPtr acr = m_autoConnectionRecoveryContext;

		if (!acr)
			return;

		assert(acr->m_tcpSocket); // 이것을 통해 이 메시지를 받았을테니!

								  // 기존 서버 연결. 즉 죽은 연결일 것임.
		shared_ptr<CSuperSocket> oldTcpSocket = m_remoteServer->m_ToServerTcp;
		// 메시지를 받은 연결. 즉 신 연결.
		shared_ptr<CSuperSocket> newTcpSocket = acr->m_tcpSocket;

#ifdef _WIN32
		AddrPort oldTcpSocketLocalAddr = oldTcpSocket->GetLocalAddr();
		AddrPort newTcpSocketLocalAddr = newTcpSocket->GetLocalAddr();
#endif

		// 여기서 임시로 생성된 tempRemoteServer 객체를 가져온다.
		// (아래 구 TCP 와 새로운 TCP 소켓 교체 작업이 완료 된 후 삭제하여야 한다.)
		shared_ptr<CRemoteServer_C> tempRemoteServer =
			dynamic_pointer_cast<CRemoteServer_C>
			(SocketToHostsMap_Get_NOLOCK(newTcpSocket, AddrPort::Unassigned));
		if (!tempRemoteServer)
		{
			// tempRemoteServer 를 가져오지 못했다.
			// 심각한 문제 일단 assert!
			assert(0);

			// 연결 복원이 성공했는데 정작 클라쪽에서 상태가 깨졌다.
			// 따라서 그냥 연결 실패 처리를 해야.
			EnqueueDisconnectionEvent(ErrorType_AutoConnectionRecoveryFailed);
			m_worker->SetState(CNetClientWorker::Disconnecting);
			return;
		}

		// 임시로 생성 된 remote server 객체들을 전부 파괴 및
		// 해당 함수에서 host map 에 등록 된 것도 삭제하는 것은
		// 여기서 불필요. Disconnecting state가 될 때 호출된다.
		//AutoConnectionRecovery_DestroyCandidateRemoteServersAndSockets(newTcpSocket);

		// 구 TCP socket이 갖고 있던 AMR을 새 TCP socket으로 옮긴다.
		Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		CSuperSocket::AcrMessageRecovery_MoveSocketToSocket(oldTcpSocket, newTcpSocket);

		/* 생존한 TCP 연결을 to-server TCP로 자리 교체한다.
		어차피 process message도 main lock 후 처리하고,
		여기 또한 main lock 상태이므로, 수신 처리 중에 바꿔버리는 사태 없음. */
		m_remoteServer->m_ToServerTcp = newTcpSocket;
		SocketToHostsMap_SetForAnyAddr(m_remoteServer->m_ToServerTcp, m_remoteServer);
		acr->m_tcpSocket = shared_ptr<CSuperSocket>();

		// 기존 TCP 연결은 폐기 처리
		// SocketToHostMap_Remove(oldTcpSocket); don't need it. OnSocketGarbageCollected will do it.
		GarbageSocket(oldTcpSocket); // 이미 참조 변수가 다른 것으로 overwrite되었으므로 safe.

									 // 임시로 생성된 ToServer 소켓을 초기화 시킨 후, Garbage 처리 해야 한다.

									 // tempRemoteServer 객체에는 ToServerTcp 의 값이 없어야 한다.
									 // 해당 값이 acrContext 의 Tcp 소켓이라면 바로 Garbage 를 하였을때 문제가 생긴다.
									 // (GC 내부에서 ToServerTcp 를 이용하여 SocketToHost 맵에서 Host 를 제거하므로)
									 // 혹시 모르니 ToServerTcp 객체를 초기화 시키고 Garbage 한다.
		tempRemoteServer->m_ToServerTcp = shared_ptr<CSuperSocket>();

		// 재접속을 진행했던 remote 객체를 garbage 한다.
		AutoConnectionRecovery_GarbageTempRemoteServerAndSocket(tempRemoteServer);

		// survive connection을 구 TCP 연결로 교체한 후 시행해야 한다.
		// session key를 +1 된 새 session key로 교체
		m_toServerSessionKey = m_acrCandidateSessionKey;

		// 수신미보장 부분(메시지들의 집합)을 재송신을 걸자.
		// 이것들에 대해서 미래에 ack가 오면 그때 제거될 것이므로 여기서 제거하지는 말자.
		m_remoteServer->m_ToServerTcp->AcrMessageRecovery_ResendUnguarantee(m_remoteServer->m_ToServerTcp);

		// 과거 연결의 local addr과 현재 연결의 local addr가 서로 다른 경우,
		// TCP 연결이 바뀌었으니,
		// upnp를 다시 하라고 지시한다.
#if defined(_WIN32)
		if (oldTcpSocketLocalAddr != newTcpSocketLocalAddr)
		{
			m_manager->m_upnp.Reset();
		}
#endif

		// 유저에게 노티
		LocalEvent e;

		e.m_type = LocalEventType_ServerOnline;
		e.m_remoteHostID = HostID_Server;
		EnqueLocalEvent(e, m_remoteServer);

		// 이제 더 이상 ACR context를 쓸 필요가 없다.
		m_autoConnectionRecoveryContext = CAutoConnectionRecoveryContextPtr();
	}

	// 모든 temp remote host  를 garbage 요청
	void CNetClientImpl::AutoConnectionRecovery_GarbageEveryTempRemoteServerAndSocket()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		CFastMap<void*, shared_ptr<CRemoteServer_C> >::iterator iter = m_autoConnectionRecovery_temporaryRemoteServers.begin();
		while (iter != m_autoConnectionRecovery_temporaryRemoteServers.end())
		{
			AutoConnectionRecovery_GarbageTempRemoteServerAndSocket(iter->GetSecond());

			++iter;
		}
	}

	// ACR 작업때 생성한 임시 remote host 객체만 garbage 한다.
	// (default parameter 는 nullptr 이며, nullptr 이라면 ACR Context 의 기본 tcpSocket 만 garbage 한다.)
	// 주의!! NetClient 가 가지고 있는 실제 m_remoteServer 객체와는 별도의 임시 객체를 줘야 한다.
	void CNetClientImpl::AutoConnectionRecovery_GarbageTempRemoteServerAndSocket(shared_ptr<CRemoteServer_C> tempRemoteServer)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 사용자 실수 예방
		if (tempRemoteServer == m_remoteServer)
		{
			// 만일 여기에 온다면 단순 코드 에러이므로 바로 처리 할 수 있도록 assert
			assert(0);
			return;
		}

		if (tempRemoteServer)
		{
			GarbageHost(tempRemoteServer, ErrorType_Ok, ErrorType_Ok, ByteArray(), _PNT("ACR_GTR"), SocketErrorCode_Ok);
		}

		if (m_autoConnectionRecoveryContext && m_autoConnectionRecoveryContext->m_tcpSocket)
		{
			GarbageSocket(m_autoConnectionRecoveryContext->m_tcpSocket);
			SocketToHostsMap_RemoveForAnyAddr(m_autoConnectionRecoveryContext->m_tcpSocket);
			m_autoConnectionRecoveryContext->m_tcpSocket = shared_ptr<CSuperSocket>();
		}
	}

	// 재접속 과정이 진행중인 새 연결이 인증 실패하면.
	void CNetClientImpl::ProcessMessage_NotifyAutoConnectionRecoveryFailed(CMessage &msg)
	{
		AssertIsLockedByCurrentThread();

		ErrorType reason;
		Message_Read(msg, reason);  // 그냥 버리자. 로그 남기는 용도로 추후 사용하겠지.

									// ACR 재연결이 실패했다. 그냥 디스 처리로 넘어가자.
									// 서버가 "실패했다고" 알려주는 상황이면, 네트웍 불량이 아니라, 뭔가 로직이 꼬인거다.
									// 따라서 이때는 그냥 연결 실패 과정으로 가 버리는 것이 낫다.
		EnqueueDisconnectionEvent(ErrorType_AutoConnectionRecoveryFailed);
		m_worker->SetState(CNetClientWorker::Disconnecting);
	}

	// remote peer 중 누군가가 offline이 되었다. 이를 노티한다.
	DEFRMI_ProudS2C_P2P_NotifyP2PMemberOffline(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerHostID);
		if (rp && !rp->m_garbaged)
		{
			LocalEvent e;
			e.m_remoteHostID = remotePeerHostID;
			e.m_type = LocalEventType_P2PMemberOffline;
			m_owner->EnqueLocalEvent(e, rp);
		}

		return true;
	}

	// 역시, 위 함수처럼, 노티.
	DEFRMI_ProudS2C_P2P_NotifyP2PMemberOnline(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> rp = m_owner->GetPeerByHostID_NOLOCK(remotePeerHostID);
		if (rp != nullptr && !rp->m_garbaged)
		{
			LocalEvent e;
			e.m_remoteHostID = remotePeerHostID;
			e.m_type = LocalEventType_P2PMemberOnline;
			m_owner->EnqueLocalEvent(e, rp);
		}

		return true;
	}

	// 서버와의 연결이 정상적인 상황에서 작동하는 루틴.
	void CNetClientImpl::Heartbeat_AutoConnectionRecovery()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 서버와 연결이 아예 없으면 즐.
		// NOTE: 서버와의 연결은 없어도 remoteServer가 살아있다. 연결유지 기능 상황에서는 이러함.
		if (m_remoteServer == nullptr)
			return;

		// 재접속 진행 중이 아니면 스킵
		if (m_autoConnectionRecoveryContext == nullptr)
			return;

		int64_t currTime = GetPreciseCurrentTimeMs();
		// m_autoConnectionRecoveryDelayMs만큼 뒤로 미뤄야.

		if (m_autoConnectionRecoveryContext->m_tcpSocket == nullptr
			&& currTime - m_autoConnectionRecoveryContext->m_startTime >= 0)
		{
			// 네트워크 망 전환 감지 기능
			if (m_autoConnectionRecoveryContext->m_waitingForNetworkAddressAvailable)
			{
				StringA str = StringA::NewFormat("Checking transition network...");
				CFakeWin32::OutputDebugStringA(str.GetString());

				// 네트워크 망이 전환 되었는지 체크 한다.
				// (ACR Context 에서 체크하는 전환 감지 기능은 단순히 유니캐스트 주소가 나온것으로 체크)
				// 로컬 주소를 확보했는지 체크한다.
				// 모바일 기기에서는, 핸드오버를 하는 동안 로컬 주소가 없을 수 있는데,
				// 이때는 아무것도 하지 않아야 한다.
				if (IsNetworkAddressAvailable())
				{
					CFakeWin32::OutputDebugStringA("Transition network!!");

					// 전환이 되었다면 실제 재접속 루틴으로 바꾼다.
					m_autoConnectionRecoveryContext->m_waitingForNetworkAddressAvailable = false;
					m_autoConnectionRecoveryContext->m_startTime = currTime + 500;
				}
				else
				{
					// 일부 디바이스에서 계속해서 연속으로 주소를 가져오려 하면 와이파이에 새로 연결 되었을 때, AP에 접속 자체를 하지 않는다.
					// 이에 따른 수정 사항
					m_autoConnectionRecoveryContext->m_startTime = currTime + 1500;
				}
			}
			else
			{
				IssueTcpConnectOrProcessAcrFailure_();
			}
		}

		if (currTime - m_autoConnectionRecoveryContext->m_firstStartTime > m_settings.m_autoConnectionRecoveryTimeoutTimeMs)
		{
			EnqueueDisconnectionEvent(ErrorType_AutoConnectionRecoveryFailed);
			m_worker->SetState(CNetClientWorker::Disconnecting);
			return;
		}
	}

	// TCP socket이 없으면 만든 후 비동기 연결을 건다.
	// 라우트 테이블이나 dns가 세팅 안되어 있어 실패할 수도 있는데 이때는 잠시 뒤에 다시 하도록 하자.
	void CNetClientImpl::IssueTcpConnectOrProcessAcrFailure_()
	{
		Proud::AssertIsLockedByCurrentThread(GetCriticalSection());

		// 서버의 주소 정보가 새롭게 갱신 되었을 때, 실제 재접속 과정을 진행한다.
		// (아래 if 구문이 실패할 경우 뫼비우스의 띠 현상이 벌어질 일은 없다. 아래에서 재접속 카운트를 세어 종료 처리 하므로)
		if (m_RefreshServerAddrInfoState == RefreshServerAddrInfoState_NotWorking)
		{
			m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_Working;

			NetClient_LegacyThreadContext* context = new NetClient_LegacyThreadContext();
			TryGetReferrerHeart(context->m_referrerHeart);
			context->m_netClient = this;

			// 우선 여기서 한번 체크
			if (context->m_referrerHeart == nullptr)
			{
				CFakeWin32::OutputDebugStringA("ITCOPAF_ referrerHeart is nullptr");

				m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_NotWorking;
				delete context;
				return;
			}

			Thread thread(RefreshServerAddrInfo_WorkerProcedure, (void*)context, false);
			thread.Start();

			// 워커 스레드 실행시간 마킹
			m_refreshServerAddrInfoWorkerThreadStartTimeMs = GetPreciseCurrentTimeMs();

			CFakeWin32::OutputDebugStringA("ITCOPAF_ Notworking");
		}
		else if (m_RefreshServerAddrInfoState == RefreshServerAddrInfoState_Working)
		{
			// state류가 있는 비동기 실행 루틴은 언제든지 바보(아무것도 안하는) 현상이 생길 수 있다.
			// 이를 알아차리기 위해 로그를 남긴다.
			// 추후에는 타임스탬프를 찍어 일정시간이 지날동안 완료를 못하면 NetClientEvent.OnWarning을 내뿝도록 하자.
			NTTNTRACE("ITCOPAF_, RefreshServerAddrInfo state is working...");

			// timeout 시간동안 결과가 안나오면 NotWorking 상태로 돌려서 다시 시도하도록 한다.
			if (m_refreshServerAddrInfoWorkerThreadStartTimeMs != -1 &&
				(GetPreciseCurrentTimeMs() - m_refreshServerAddrInfoWorkerThreadStartTimeMs > CNetConfig::AutoConnectionRecoveryRefreshServerAddrInfoTimeoutMs))
			{
				m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_NotWorking;
				m_refreshServerAddrInfoWorkerThreadStartTimeMs = -1;
			}

			return;
		}
		else if (m_RefreshServerAddrInfoState == RefreshServerAddrInfoState_Finished)
		{
			// Finished가 사실상 Issue TCP Connect의 시발점이므로 소켓 생성 and Bind and Connect를 여기서 한다.

			// TCP socket 생성 과정.
			shared_ptr<CSuperSocket> conn;
			SuperSocketCreateResult r = CSuperSocket::New(this, SocketType_Tcp);
			if (r.socket)
				conn = r.socket;
			else
			{
				ProcessAcrCandidateFailure();
				return;
			}

			m_autoConnectionRecoveryContext->m_tcpSocket = conn;
			SetTcpDefaultBehavior_Client(conn->GetSocket());
			SocketErrorCode bindResult = conn->m_fastSocket->Bind(); // full ANY address
			if (bindResult != SocketErrorCode_Ok)
			{
				ProcessAcrCandidateFailure();
				return;
			}

			// 소켓 만들기 성공.
			SocketErrorCode connectResult;
			bool isPendingOrWouldBlock = false;

			// [1]의 이유: #iOS_ENOTCONN_issue
			m_netThreadPool->AssociateSocket(conn, false/*[1]*/);

#if defined(_WIN32)
			connectResult = conn->IssueConnectEx(m_serverAddrPort);
			isPendingOrWouldBlock = CFastSocket::IsPendingErrorCode(connectResult);
#else
			// epoll에는 non-block을 해야.
			// 막 IP handover가 된 직후에는 Connect() 안에서의 hostname lookup이 실패할 수도 있다.
			connectResult = conn->SetNonBlockingAndConnect(m_serverAddrPort);
			// connect가 실패한 채로 add to epoll을 하면 hang up event가 발생할 것이다. 어차피 뒤에서 처리된다.
			isPendingOrWouldBlock = CFastSocket::IsWouldBlockError(connectResult);
#endif
			if (connectResult != SocketErrorCode_Ok && isPendingOrWouldBlock == false)
			{
				StringA str = StringA::NewFormat("ITCOPAF_ socket reconnect fail. : %d", connectResult);
				CFakeWin32::OutputDebugStringA(str.GetString());

				// RefreshServerAddrInfo가 실패 했다는것은 아직까지도 네트워크상 문제일 확률이 높다.
				// 이때는 컨텍스트를 롤백시켜 처음부터 다시 진행하도록 한다.
				m_autoConnectionRecoveryContext->m_waitingForNetworkAddressAvailable = true;
				ProcessAcrCandidateFailure();
			}

			m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_NotWorking;
		}
		else
		{
			CFakeWin32::OutputDebugStringA("ITCOPAF_ failed.");

			// 그 외의 상황 (가령 RefreshServerAddrInfo가 실패했다든지)
			m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_NotWorking;

			// RefreshServerAddrInfo가 실패 했다는것은 아직까지도 네트워크상 문제일 확률이 높다.
			// 이때는 컨텍스트를 롤백시켜 처음부터 다시 진행하도록 한다.
			m_autoConnectionRecoveryContext->m_waitingForNetworkAddressAvailable = true;
		}
	}



	// 연결이 끊어졌을 때 바로 ACR이 작동하지 않고 일부러 조금 있다 작동하게 한다.
	void CNetClientImpl::TEST_SetAutoConnectionRecoverySimulatedDelay(int timeMs)
	{
		m_autoConnectionRecoveryDelayMs = timeMs;
	}

	void CNetClientImpl::ProcessAcrCandidateFailure()
	{
		// 이 함수는 1회의 연결 실패에서도 동시에 여럿 호출될 수 있다. if조건의 이유.
		if (m_autoConnectionRecoveryContext != nullptr)
		{
			if (m_autoConnectionRecoveryContext->m_tcpSocket)
			{
				int64_t currTime = GetPreciseCurrentTimeMs();

				// 바로 시작하지 말고, 약간의 짧은 시간 후에 하자.
				// 예전에는 2초 뒤에 시작했으나, 재접속 한계가 십수초라는 짧은 시간이므로,
				// CPU를 처먹지 않는 선에서 조금만 기다리자.
				m_autoConnectionRecoveryContext->m_startTime = currTime + 300;

				// TCP 연결 후보 폐기.
				AutoConnectionRecovery_GarbageTempRemoteServerAndSocket();
			}
		}
	}
}
