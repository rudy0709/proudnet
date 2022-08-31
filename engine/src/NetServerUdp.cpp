#include "stdafx.h"
#include "NetServer.h"
#include "RmiContextImpl.h"

namespace Proud
{
	// 클라에서 먼저 UDP 핑 실패로 인한 fallback을 요청한 경우
	DEFRMI_ProudC2S_NotifyUdpToTcpFallbackByClient(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		// 해당 remote client를 업데이트한다.
		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if (rc != nullptr)
		{
			m_owner->SecondChanceFallbackUdpToTcp(rc);
		}

		return true;
	}

	DEFRMI_ProudC2S_NotifyNatDeviceNameDetected(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if (rc != nullptr)
		{
			rc->m_natDeviceName = natDeviceName;
		}

		return true;
	}

	DEFRMI_ProudC2S_NotifyNatDeviceName(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);
		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if (rc != nullptr)
		{
			rc->m_natDeviceName = deviceName;
		}
		return true;
	}

	DEFRMI_ProudC2S_C2S_RequestCreateUdpSocket(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if (rc != nullptr)
		{
			m_owner->RemoteClient_New_ToClientUdpSocket(rc);
			bool udpExists = rc->m_ToClientUdp_Fallbackable.m_udpSocket ? true : false;

			NamedAddrPort sockAddr;

			if (udpExists)
				sockAddr = rc->GetRemoteIdentifiableLocalAddr();

			// 상대측 요청에 의해 UDP 소켓 생성 끝. 상대는 이미 있을터이고. 따라서 ack를 노티.
			m_owner->m_s2cProxy.S2C_CreateUdpSocketAck(remote, g_ReliableSendForPN, udpExists, sockAddr);

			if (udpExists)
			{
				m_owner->RemoteClient_RequestStartServerHolepunch_OnFirst(rc);
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_C2S_CreateUdpSocketAck(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if (rc != nullptr)
		{
			//m_owner->RemoteClient_New_ToClientUdpSocket(rc);
			rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting = false;

			if (succeed)
			{
				m_owner->RemoteClient_RequestStartServerHolepunch_OnFirst(rc);
			}
			else
			{
				// 클라이언트의 udp 소켓생성이 실패 했다면 udp통신을 하지 않는다.

				// comment by ulelio : 여기서 refCount가 2여야 정상임.
				// perclient일때는 m_ownedudpsocket이 있고 static일때는 udpsockets가 있기때문..
				if (m_owner->m_logWriter)
				{
					Tstringstream ss;
					ss << _PNT("Failed to create Client UDP socket. HostID=") << rc->m_HostID;

					m_owner->m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
				}

				// modify by ulelio : udpsocket이 없으므로 realudpEnable이 false다.
				rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled = false;
				rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
				rc->m_ToClientUdp_Fallbackable.m_udpSocket = nullptr;
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_ReportC2CUdpMessageCount(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
		if (rc != nullptr && peer != HostID_None)
		{
			// 받은 packet 갯수를 업데이트한다.
			rc->m_toRemotePeerSendUdpMessageTrialCount += udpMessageTrialCount;
			rc->m_toRemotePeerSendUdpMessageSuccessCount += udpMessageSuccessCount;

			for (CRemoteClient_S::P2PConnectionPairs::iterator iPair = rc->m_p2pConnectionPairs.begin(); iPair != rc->m_p2pConnectionPairs.end(); iPair++)
			{
				shared_ptr<P2PConnectionState> pair = iPair.GetSecond();
				if (pair->ContainsHostID(peer))
				{
					pair->m_toRemotePeerSendUdpMessageTrialCount = udpMessageTrialCount;
					pair->m_toRemotePeerSendUdpMessageSuccessCount = udpMessageSuccessCount;
				}
			}
		}

		return true;
	}

	DEFRMI_ProudC2S_ReportC2SUdpMessageTrialCount(CNetServerImpl::C2SStub)
	{
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(m_owner);

		shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);

		if (rc != nullptr)
			rc->m_toServerSendUdpMessageTrialCount = toServerUdpTrialCount;

		return true;
	}

	// TCP fallback과 관련해서 서버 로컬에서의 처리 부분
// 이미 넷클라 측은 fallback을 진행한 상태에서 콜 해야 한다.
	void CNetServerImpl::SecondChanceFallbackUdpToTcp(const shared_ptr<CRemoteClient_S>& rc)
	{
		AssertIsLockedByCurrentThread();

		if (rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
		{
			rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled = false;

			// addr-to-remote 관계도 끊어야 함.
			// 넷클라가 ip handover를 한 경우라서 끊어진 경우 다른 클라가 이 addr을 가져갈 수 있으니까.
			assert(rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr);
			SocketToHostsMap_RemoveForAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket, rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
			//m_UdpAddrPortToRemoteClientIndex.RemoveKey(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());

			P2PGroup_RefreshMostSuperPeerSuitableClientID(rc);

			// 로그를 남긴다.
			if (m_logWriter)
			{
				Tstringstream ss;
				ss << "Client " << rc->m_HostID << " cancelled the UDP Hole-Punching to the server: Client local Addr=" << rc->m_tcpLayer->m_remoteAddr.ToString().GetString();
				m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
			}
		}
	}

	void CNetServerImpl::GetUdpListenerLocalAddrs(CFastArray<AddrPort>& output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (m_udpAssignMode != ServerUdpAssignMode_Static)
			throw Exception("Cannot call GetUdpListenerLocalAddrs unless ServerUdpAssignMode_Static is used!");

		output.Clear();

		for (int i = 0; i < (int)m_staticAssignedUdpSockets.GetCount(); i++)
		{
			if (m_staticAssignedUdpSockets[i]->GetSocket())
			{
				output.Add(m_staticAssignedUdpSockets[i]->GetSocketName());
			}
		}
	}


	void CNetServerImpl::SetDefaultFallbackMethod(FallbackMethod value)
	{
		if (value == FallbackMethod_CloseUdpSocket)
		{
			throw Exception("Not supported value yet!");
		}

		if (m_simplePacketMode)
			m_settings.m_fallbackMethod = FallbackMethod_ServerUdpToTcp;
		else
			m_settings.m_fallbackMethod = value;
	}

	void CNetServerImpl::FallbackServerUdpToTcpOnNeed(const shared_ptr<CRemoteClient_S>& rc, int64_t currTime)
	{
		// [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
		// 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.

		//if (currTime - rc->m_lastUdpPacketReceivedTime > m_defaultTimeoutTimeMs) 이게 아니라
		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr)
		{
			if (rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled
				&& currTime - rc->m_ToClientUdp_Fallbackable.m_udpSocket->m_lastReceivedTime > CNetConfig::GetFallbackServerUdpToTcpTimeoutMs())
			{
				// 굳이 이걸 호출할 필요는 없다. 클라에서 다시 서버로 fallback 신호를 할 테니까.
				// -> 위와 같이 해버리면 TCP 연결은 되어 있으나 응답 안할 수도 있다.
				// 그래서 무조건 FallBack을 해야한다. by JiSamHong
				SecondChanceFallbackUdpToTcp(rc);

				// 클라에게 TCP fallback 노티를 한다.
				// 한편 서버에서는 TCP fallback을 해버린다. 이게 없으면 아래 RMI를 마구 보내는 불상사 발생.
				m_s2cProxy.NotifyUdpToTcpFallbackByServer(rc->m_HostID, g_ReliableSendForPN,
					CompactFieldMap());
			}
		}
	}

	void CNetServerImpl::ArbitraryUdpTouchOnNeed(const shared_ptr<CRemoteClient_S>& rc, int64_t currTime)
	{
		AssertIsLockedByCurrentThread();
		assert(CNetConfig::GetFallbackServerUdpToTcpTimeoutMs() > (CNetConfig::UnreliablePingIntervalMs * 5) / 2);

		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr)
		{
			// 클라로부터 최근까지도 UDP 수신은 됐지만 정작 ping만 안오면 다른 형태의 pong 보내기.
			// 클라에서 서버로 대량으로 UDP를 쏘느라 정작 핑이 늑장하는 경우 필요하다.
			if (currTime - rc->m_safeTimes.Get_lastUdpPingReceivedTime() > (CNetConfig::UnreliablePingIntervalMs * 3) / 2 &&
				currTime - rc->m_ToClientUdp_Fallbackable.m_udpSocket->m_lastReceivedTime < CNetConfig::UnreliablePingIntervalMs)
			{
				if (currTime - rc->m_safeTimes.Get_arbitraryUdpTouchedTime() > CNetConfig::UnreliablePingIntervalMs)
				{
					rc->m_safeTimes.Set_arbitraryUdpTouchedTime(currTime);

					// 에코를 unreliable하게 보낸다.
					CMessage sendMsg;
					sendMsg.UseInternalBuffer();
					Message_Write(sendMsg, MessageType_ArbitraryTouch);

					rc->m_ToClientUdp_Fallbackable.AddToSendQueueWithSplitterAndSignal_Copy(rc->GetHostID(), CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
				}
			}
		}
	}

	void CNetServerImpl::GetUdpSocketAddrList(CFastArray<AddrPort>& output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (m_udpAssignMode != ServerUdpAssignMode_Static)
			throw Exception("Cannot call GetUdpSocketAddrList unless ServerUdpAssignMode_Static is used!");

		output.Clear();
		for (int i = 0; i < (int)m_staticAssignedUdpSockets.GetCount(); i++)
		{
			shared_ptr<CSuperSocket>  sk = m_staticAssignedUdpSockets[i];
			output.Add(sk->GetSocketName());
		}
	}

	/* to-client UDP socket을 생성하거나 풀에서 할당한다.
자세한 것은 함수 루틴 내 주석 참고.

리턴값은 없지만, 이 함수 호출 후 m_ToClientUdp_Fallbackable.m_udpSocket 존재
여부로 UDP socket 존재 여부를 판단하자.
(예전에는 bool이었지만, "이미 있는데 만들 필요 없음"이라는 의미로 오해될 수 있어서
이렇게 고침. 결국 중요한 것은 멱등성이니까.) */
	void CNetServerImpl::RemoteClient_New_ToClientUdpSocket(const shared_ptr<CRemoteClient_S>& rc)
	{
		AssertIsLockedByCurrentThread();

		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr)
			return;

		// 한번 UDP socket을 생성하는 것을 실패한 적이 있다면 또 반복하지 않는다.
		if (rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed)
			return;

		shared_ptr<CSuperSocket> assignedUdpSocket;

		bool UdpSocketRecycleSuccess = false;
		if (m_udpAssignMode == ServerUdpAssignMode_PerClient)
		{
			// 일전에 같은 HostID로 사용되었던 UDP socket을 재사용한다.
			UdpSocketRecycleSuccess = m_recycles.TryGetValue(rc->GetHostID(), assignedUdpSocket);

			if (UdpSocketRecycleSuccess == false)
			{
				NamedAddrPort addrPort = NamedAddrPort::FromAddrPort(m_localNicAddr, 0);

				// 재사용 가능 UDP socket을 못 뽑았으므로, 새로 만든다.
				SuperSocketCreateResult r = CSuperSocket::New(this, SocketType_Udp);
				if (!r.socket)
				{
					// 자원 고갈? 아무튼 에러 처리.
					rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
					EnqueError(ErrorInfo::From(ErrorType_UnknownAddrPort, rc->m_HostID, r.errorText));
					return;
				}
				assignedUdpSocket = r.socket;

				// per-remote UDP socket은 reuse를 끈다. 오차로 인해 같은 UDP port를 2개 이상의 UDP socket이 가지는 경우,
				// PNTest에서 "서버가 RMI 3003를 받는 오류"가 생길 수 있으므로.
				// 어떤 OS는 UDP socket가 기본 reuse ON인 것으로 기억함... 그래서 강제로 reuse OFF를 설정.
				assignedUdpSocket->m_fastSocket->SetSocketReuseAddress(false);

				// bind
				SocketErrorCode socketErrorCode = SocketErrorCode_Ok;
				if (!m_localNicAddr.IsEmpty())
				{
					socketErrorCode = assignedUdpSocket->Bind(m_localNicAddr.GetString(), 0);
				}
				else
					socketErrorCode = assignedUdpSocket->Bind(0);

				if (socketErrorCode != SocketErrorCode_Ok)
				{
					// bind가 실패하는 경우다. 이런 경우 자체가 있을 수 없는데.
					// UDP 소켓 생성 실패에 준한다. 에러를 내자. 그리고 이 클라의 접속을 거부한다.
					rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
					return;
				}

				// socket local 주소를 얻고 그것에 대한 매핑 정보를 설정한다.
				assert(assignedUdpSocket->GetLocalAddr().m_port > 0);
				//				m_localAddrToUdpSocketMap.Add(assignedUdpSocket->GetLocalAddr(), assignedUdpSocket);

								//assignedUdpSocket->ReceivedAddrPortToVolatileHostIDMap_Set(assignedUdpSocket->GetLocalAddr(), rc->GetHostID());

				m_netThreadPool->AssociateSocket(assignedUdpSocket, true);

				// overlapped send를 하므로 송신 버퍼는 불필요하다.
				// socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
				// send buffer 없애기는 coalsecse, throttling을 위해 필수다.
				// recv buffer 제거는 백해무익이므로 즐

				// CSuperSocket::Bind 내부에서 처리하기때문에 불필요
				// #ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
				// 			assignedUdpSocket->m_socket->SetSendBufferSize(0);
				// #endif // ALLOW_ZERO_COPY_SEND
				SetUdpDefaultBehavior_Server(assignedUdpSocket->GetSocket());

				// 첫번째 issue recv는 networker thread에서 할 것이다.
			}
		}
		else
		{
			// 고정된 UDP 소켓을 공용한다.
			assignedUdpSocket = GetAnyUdpSocket();
			if (assignedUdpSocket == nullptr)
			{
				rc->m_ToClientUdp_Fallbackable.m_createUdpSocketHasBeenFailed = true;
				EnqueError(ErrorInfo::From(ErrorType_UnknownAddrPort, rc->m_HostID, _PNT("No available UDP socket exists in Static Assign Mode.")));
				return;
			}
		}

		// UDP socket 연결관계 형성
		rc->m_ToClientUdp_Fallbackable.m_udpSocket = assignedUdpSocket;

		if (m_udpAssignMode == ServerUdpAssignMode_PerClient)
		{
			if (UdpSocketRecycleSuccess == false)
			{
				rc->m_ownedUdpSocket = assignedUdpSocket;
				SocketToHostsMap_SetForAnyAddr(rc->m_ownedUdpSocket, rc);

#if defined (_WIN32)
				// per RC UDP이면 첫 수신 이슈를 건다.
				rc->m_ownedUdpSocket->IssueRecv(rc->m_ownedUdpSocket, true); // first
#endif
			}
		}

		SendOpt opt;
		opt.m_enableLoopback = false;
		opt.m_priority = MessagePriority_Ring0;
		opt.m_uniqueID.m_value = 0;
		if (!m_usingOverBlockIcmpEnvironment)
			opt.m_ttl = 1; // NRM1 공유기 등은 알수없는 인바운드 패킷이 오면 멀웨어로 감지해 버린다. 따라서 거기까지 도착할 필요는 없다.

		// Windows 2003의 경우, (포트와 상관없이) 서버쪽에서 클라에게 UDP패킷을 한번 쏴줘야 해당 클라로부터 오는 패킷을 블러킹하지 않는다.
		CMessage ignorant;
		ignorant.UseInternalBuffer();
		Message_Write(ignorant, MessageType_Ignore);

		rc->m_ToClientUdp_Fallbackable.m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
			rc->m_ToClientUdp_Fallbackable.m_udpSocket,
			rc->GetHostID(),
			FilterTag::CreateFilterTag(HostID_Server, HostID_None),
			rc->m_tcpLayer->m_remoteAddr,
			ignorant,
			GetPreciseCurrentTimeMs(),
			opt);

		return;
	}


	void CNetServerImpl::RemoteClient_RequestStartServerHolepunch_OnFirst(const shared_ptr<CRemoteClient_S>& rc)
	{
		AssertIsLockedByCurrentThread();

		// create magic number
		if (rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber == Guid())
		{
			if (m_simplePacketMode)
			{
				//{899F5C4D-D1C8-4F33-A7BF-04A89F9C88C3}
				PNGUID guidMagicNumber = { 0x899f5c4d, 0xd1c8, 0x4f33, {0xa7, 0xbf, 0x4, 0xa8, 0x9f, 0x9c, 0x88, 0xc3} };
				Guid magicNumber = Guid(guidMagicNumber);
				rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = magicNumber;
			}
			else
				rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = Guid::RandomGuid();

			// send magic number and any UDP address via TCP
			{
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				Message_Write(sendMsg, MessageType_RequestStartServerHolepunch);
				sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);

				rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
					rc->m_tcpLayer,
					CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
			}
		}
	}

	// 서버에서 먼저 UDP socket을 생성. 그리고 클라에게 "너도 만들어"를 지시.
	// 단, 아직 지시한 적도 없고, UDP socket을 생성한 적도 없을 때만.
	// 클라-서버 UDP 통신용.
	void CNetServerImpl::RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(
		shared_ptr<CRemoteClient_S>& rc) // code profile 결과 호출빈도가 잦아, &를 붙임.
	{
		if (rc->m_ToClientUdp_Fallbackable.m_udpSocket == nullptr
			&& rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting == false
			&& m_settings.m_fallbackMethod == FallbackMethod_None)
		{
			RemoteClient_New_ToClientUdpSocket(rc);
			if (rc->m_ToClientUdp_Fallbackable.m_udpSocket)
			{
				// UDP 소켓을 성공적으로 생성했으니, 클라도 생성하라고 지시.
				NamedAddrPort sockAddr = rc->GetRemoteIdentifiableLocalAddr();
				m_s2cProxy.S2C_RequestCreateUdpSocket(rc->m_HostID, g_ReliableSendForPN, sockAddr);
				rc->m_ToClientUdp_Fallbackable.m_clientUdpReadyWaiting = true;

				//NTTNTRACE("send requestcreateudpsocket = %s\n",StringT2A(sockAddr.ToString()).GetString());
			}
		}
	}

	ErrorType CNetServerImpl::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int* outputPercent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		if (outputPercent == nullptr)
			return ErrorType_PermissionDenied;

		if (remotePeerID == HostID_Server) // 자기 자신이면 로스률은 없다.
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remotePeerID);
		if (rc == nullptr)
			return ErrorType_InvalidHostID;

		if (!rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		AddrPort PeerAddr = rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
		if (PeerAddr == AddrPort::Unassigned)	// 아직 홀펀칭이 안된 경우 로스률은 없다.
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		shared_ptr<CSuperSocket> pUdpSocket = rc->m_ToClientUdp_Fallbackable.m_udpSocket;
		if (pUdpSocket == nullptr)
		{
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(PeerAddr);
		*outputPercent = nPercent;
		return ErrorType_Ok;
	}
}
