/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <new>
#include <stack>

#include "../include/INetClientEvent.h"
#include "../include/Marshaler.h"
#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/CoInit.h"
#include "../include/MilisecTimer.h"
#include "../include/ThreadUtil.h"
#include "../include/atomic.h"

#include "ReliableUDPFrame.h"
#include "IntraTracer.h"
#include "FakeWinsock.h"
#include "ReliableUdpHelper.h"
#include "SocketUtil.h"
#include "RmiContextImpl.h"
#include "RemotePeer.h"
#include "Networker_C.h"
#include "LeanDynamicCast.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif

#ifdef __MARMALADE__
#include <s3eDevice.h>
#include <s3eDebug.h>
#endif // __MARMALADE__

#if defined(__linux__) || defined(__MACH__)
#include <poll.h>
#endif

#include "NetClient.h"
#include "ReusableLocalVar.h"

#if defined(_WIN32)        
#define _COUNTOF(array) (sizeof(array)/sizeof(array[0]))
#endif

namespace Proud
{

#ifdef _WIN32
	int GetProcessorType()
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return info.wProcessorArchitecture;
	}
#endif

	CNetClientWorker::CNetClientWorker(CNetClientImpl *owner)
	{
		m_DisconnectingModeHeartbeatCount = 0;
		m_DisconnectingModeStartTime = 0;
		m_DisconnectingModeWarned = false;

		m_owner = owner;
		m_state_USE_FUNC = IssueConnect;

		AtomicIncrement32(&m_owner->m_manager->m_instanceCount);
	}

	CNetClientWorker::~CNetClientWorker()
	{
		int a1 = (GetState() == CNetClientWorker::Disconnected);
		//int a2 = (m_owner->GetCriticalSection().IsLockedByCurrentThread() == false);

		if ( !a1 /*|| !a2 || */ )
		{
			assert(0);
			String txt;
			txt.Format(_PNT("CNetClientManager.dtor assert fail: %d"), a1/*a2,*/);
#if defined(_WIN32)
			CErrorReporter_Indeed::Report(txt);
#endif
		}
		AtomicDecrement32(&m_owner->m_manager->m_instanceCount);
	}

	void CNetClientImpl::Heartbeat()
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		int64_t currTime = GetPreciseCurrentTimeMs();
		int64_t elapsedTime = currTime - m_lastHeartbeatTime;
		m_lastHeartbeatTime = currTime;
		m_recentElapsedTime = LerpInt(m_recentElapsedTime, elapsedTime, (int64_t)3, (int64_t)10);

		//WarnTooLongElapsedTime();
		try
		{
			switch ( m_worker->GetState() )
			{
				case CNetClientWorker::IssueConnect:
					Heartbeat_IssueConnect();
					break;
				case CNetClientWorker::Connecting:
					Heartbeat_Connecting();
					break;
				case CNetClientWorker::JustConnected:
					Heartbeat_JustConnected();
					break;
				case CNetClientWorker::Connected:
					Heartbeat_Connected();
					break;
				case CNetClientWorker::Disconnecting:
					Heartbeat_Disconnecting();
					break;
				case CNetClientWorker::Disconnected:
					Heartbeat_Disconnected();
					break;
			}
		}
		catch ( std::bad_alloc &ex )
		{
			Exception e(ex);
			BadAllocException(e);
		}

		TcpAndUdp_DoForLongInterval();
	}

	void CNetClientImpl::Heartbeat_ConnectedCase()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		int64_t curTime = GetPreciseCurrentTimeMs();

		// 사용자가 너무 오랫동안 FrameMove를 호출 안하면, 경고를 보여준다.
		if ( m_lastFrameMoveInvokedTime > 0 &&
			curTime - m_lastFrameMoveInvokedTime > 30000 )
		{
			NTTNTRACE("**WARNING** HostID : %d CNetClient.FrameMove is not called in thirty seconds. Is this your intention?\n", (int)(GetLocalHostID()));

			if ( m_enableLog || m_settings.m_emergencyLogLineCount > 0 )
				Log(0, LogCategory_System, _PNT("**WARNING** CNetClient.FrameMove is not called in thirty seconds. Is this your intention?"));

			m_lastFrameMoveInvokedTime = -1;
		}

		int64_t interval = curTime - m_remoteServer->m_ToServerTcp->m_lastReceivedTime;

		if (curTime - m_checkTcpUnstable_timeToDo > 0)
		{
			// 여기가 CPU를 제법 먹는다.
			// 너무 조금 하면 안되고, 그렇다고 너무 자주 해도 안된다. 
			m_checkTcpUnstable_timeToDo = curTime + 700;

			// TCP 수신이 위험할 정도(time-out값의 60%)로 안 오는 경우
			// udp 통신을 줄이도록.
			bool isUnstable = IsTcpUnstable(interval);

			// 클라-서버간 UDP도 혼잡을 일으킬 수 있음
			if (m_remoteServer->m_ToServerUdp != NULL)
				m_remoteServer->m_ToServerUdp->SetTcpUnstable(curTime, isUnstable);

			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				CRemotePeer_C* remotePeer = LeanDynamicCastForRemotePeer(i->GetSecond());
				if (remotePeer && remotePeer->m_udpSocket != NULL)
					remotePeer->m_udpSocket->SetTcpUnstable(curTime, isUnstable);
			}
		}

		// 망 전환 감지 기능
		// LG G3 폰의 경우, wifi존을 벗어나서 local IP가 바뀌는데도 기존 TCP 소켓이 안 끊어진다. 그래서 이것이 필요.
		if (m_connectionParam.m_enableAutoConnectionRecovery == true
			&& m_autoConnectionRecoveryContext == NULL
			&& curTime - m_checkNextTransitionNetworkTimeMs >= 0)
		{
			// 망 전환이 일어났는지 체크 하고 전환이 되었다면 ACR 작업으로 변경 한다. 
			// (heartbeat 에서 체크하는 전환 감지 기능은 유니캐스트 주소가 기존의 서버와의 TCP 로컬 주소와 바뀌었는지로 체크)
			if (IsTransitionNetwork(true))
			{
				DisconnectOrStartAutoConnectionRecovery(*ErrorInfo::From(ErrorType_ConnectServerTimeout, HostID_Server));
			}

			m_checkNextTransitionNetworkTimeMs = curTime + 2000;
		}

		/* TCP 하트비트 전혀 없으면 Receive timeout 처리로 간주하여, 연결을 해제한다.

		UDP도 검사하는 이유: congestive collapse이지만 정작 클라는 피투피 통신을 잘 하고 있을 때가 있다.
		그러한 경우 TCP는 resend가 매우 느리게 진행되고 있겠지만 UDP는 4.3초마다 핑퐁이 오가므로 낮은 확률로라도 겨우 통신이
		되고 있을 수 있다. 그러한 경우에서 연결이 잔존하기 위해서 UDP 수신 여부도 검사한다.

		단, simple packet mode이면 TCP건 UDP건 검사를 안한다.
		왜냐하면, simple packet mode 사용자가 ping-pong을 구현하는 부담도 있고
		어차피 TCP RTO timeout 기능으로도 2분이내에는 잡으니까
		(물론 하나도 안 보내서 영원히 못 찾는 경우도 있겠지만 그건 무시하자...)
		simple packet mode는 UDP를 안쓰므로 어차피 검사 무용. */
		if ( IsSimplePacketMode() == false &&
			m_worker &&
			((curTime - m_remoteServer->m_ToServerTcp->m_lastReceivedTime) >= m_settings.m_defaultTimeoutTime) && // 핑퐁 타임아웃
			m_worker->GetState() == CNetClientWorker::Connected && // 연결 해제 과정 아직 안 갔음
			!m_autoConnectionRecoveryContext) // ACR 재접속도 안하고 있음
		{
			m_checkNextTransitionNetworkTimeMs = curTime + 2000;

			int64_t a = m_remoteServer->m_ToServerTcp->m_lastReceivedTime;

			if ( m_enableLog || m_settings.m_emergencyLogLineCount > 0 )
			{
				String msg = String::NewFormat(_PNT("TCP ping-pong time out: default timeout:%lf, current time:%lf, client:%d"),
											   m_settings.m_defaultTimeoutTime,
											   GetPreciseCurrentTimeMs(),
											   GetVolatileLocalHostID());

				Log(0, LogCategory_System, msg);
			}

			DisconnectOrStartAutoConnectionRecovery(*ErrorInfo::From(
				ErrorType_ConnectServerTimeout, 
				HostID_Server, 
				_PNT("Local detected no receive.")));
		}


		// NetClient 수가 1개에서 0개가 될 때 NetClientManager가 networker thread를 0개로 만듭니다.
		// 그런데 0개로 만들면서, 기존 실행중이던 networker thread의 메인 함수가 리턴하면서 종료하기도 전에 NetClientManager가 갖고 있던 
		//thread list에서는 이미 사라진 상태라서 나오는 assert fail입니다.
		//assert(IsCalledByWorkerThread());

		Heartbeat_EveryRemotePeer();

		SendServerHolePunchOnNeed();
		if ( IsSimplePacketMode() == false )
		{
			// simple packet mode가 비활성화일때만 ping pong 메시지를 주고 받는다.
			// 자세한 사항은 ProudNet 설계문서 참고.
			RequestServerTimeOnNeed();
			SpeedHackPingOnNeed();
			P2PPingOnNeed();
			FallbackServerUdpToTcpOnNeed();
			ReportP2PPeerPingOnNeed();
			ReportRealUdpCount();
		}
		AddUpnpTcpPortMappingOnNeed();

		CheckSendQueue();

#if defined(_WIN32)
		// Emergency log를 위해 CNetClientStats의 사본을 일정시간마다 갱신한다.
		UpdateNetClientStatClone();
#endif
	}

	void CNetClientImpl::Heartbeat_EveryRemotePeer()
	{
		LockMain_AssertIsLockedByCurrentThread();
		int64_t currTime = GetPreciseCurrentTimeMs();

		int p2pConnCnt = 0;
		for ( Position i = m_authedHostMap.GetStartPosition(); i != NULL; m_authedHostMap.GetNext(i) )
		{
			CRemotePeer_C *rp = LeanDynamicCastForRemotePeer(m_authedHostMap.GetValueAt(i));

			if ( rp != NULL )
			{
				if ( rp->m_garbaged )
					continue;
				// 			//add by rekfkno1 - 위에 있던 로직 합침. => CNetClientImpl::IssueSendOnNeed 로 옮기면서 주석화
				// 			if(rp->m_udpSocket != NULL)
				// 				rp->m_udpSocket->IssueSendOnNeed_IfPossible();

				rp->Heartbeat(currTime);

				if ( rp->m_p2pConnectionTrialContext != NULL )
					p2pConnCnt++;
			}
		}

		// 병렬 진행중인 p2p 연결 시도 횟수에 따라 인터벌을 조절한다.
		int p2pTrialUnderProgressCount = max(1, p2pConnCnt);

		m_P2PHolepunchInterval = CNetConfig::P2PHolepunchIntervalMs * p2pTrialUnderProgressCount;
		m_P2PConnectionTrialEndTime = CNetConfig::GetP2PHolepunchEndTimeMs() * p2pTrialUnderProgressCount;

		assert(m_P2PHolepunchInterval > 0);
		assert(m_P2PConnectionTrialEndTime > 0);
	}

	// user에게 처리권이 넘어가기 직전의 수신된 message를 처리한다.
	// 여기서 처리된 것 중 일부는 final message로 넘어가게 된다.
	bool CNetClientWorker::ProcessMessage_ProudNetLayer(CSuperSocket* socket, CReceivedMessage &receivedInfo)
	{

		// NOTE: udpSocket may be NULL if received from TCP socket
		m_owner->LockMain_AssertIsLockedByCurrentThread();

		CMessage &recvedMsg = receivedInfo.m_unsafeMessage;

		int orgReadOffset = recvedMsg.GetReadOffset();

		MessageType type;
		if ( recvedMsg.Read(type) == false )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		bool messageProcessed = false;
		switch ( (int)type )
		{
			case MessageType_ConnectServerTimedout:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_ConnectServerTimedout(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_NotifyStartupEnvironment:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_NotifyStartupEnvironment(socket, recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_NotifyServerConnectSuccess:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_NotifyServerConnectSuccess(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_NotifyProtocolVersionMismatch:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_NotifyProtocolVersionMismatch(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_NotifyServerDeniedConnection:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_NotifyServerDeniedConnection(recvedMsg);
				messageProcessed = true;
				break;

			case MessageType_NotifyAutoConnectionRecoverySuccess:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					m_owner->ProcessMessage_NotifyAutoConnectionRecoverySuccess(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_NotifyAutoConnectionRecoveryFailed:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					m_owner->ProcessMessage_NotifyAutoConnectionRecoveryFailed(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_RequestStartServerHolepunch:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_RequestStartServerHolepunch(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_ServerHolepunchAck:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_ServerHolepunchAck(receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_ServerHolepunchAck:
				if ( IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_PeerUdp_ServerHolepunchAck(receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_NotifyClientServerUdpMatched:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_NotifyClientServerUdpMatched(recvedMsg);
				messageProcessed = true;
				break;

			case MessageType_UnreliablePong:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_UnreliablePong(recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_ArbitraryTouch:
				// 아무것도 안한다. 그저 UDP packet received time을 갱신하기 위함.
				messageProcessed = true;
				break;

			case MessageType_ReliableRelay2:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_ReliableRelay2(socket, recvedMsg);
				messageProcessed = true;
				break;
			case MessageType_UnreliableRelay2:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_UnreliableRelay2(socket, receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_LingerDataFrame2:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_LingerDataFrame2(socket, receivedInfo);
				messageProcessed = true;
				break;

			case MessageType_ReliableUdp_Frame:
				ProcessMessage_ReliableUdp_Frame(socket, receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_PeerHolepunch:
				ProcessMessage_PeerUdp_PeerHolepunch(receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_PeerUdp_PeerHolepunchAck:
				ProcessMessage_PeerHolepunchAck(receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_P2PUnreliablePing:
				ProcessMessage_P2PUnreliablePing(socket, receivedInfo);
				messageProcessed = true;
				break;

			case MessageType_P2PUnreliablePong:
				ProcessMessage_P2PUnreliablePong(receivedInfo);
				messageProcessed = true;
				break;

				// 서버 브로드캐스팅 메세지를 피어간 릴레이를 통해서 부하 분산을 위해서 추가했다.
			case MessageType_S2CRoutedMulticast1:
				if ( !IsFromRemoteClientPeer(receivedInfo) )
					ProcessMessage_S2CRoutedMulticast1(socket, receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_S2CRoutedMulticast2:
				ProcessMessage_S2CRoutedMulticast2(socket, receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_Rmi:
				ProcessMessage_Rmi(receivedInfo, /*ref*/ messageProcessed);
				break;
			case MessageType_UserMessage:
				ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_UserMessage, /*ref*/ messageProcessed);
				break;
			case MessageType_Hla:
				ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_Hla, /*ref*/ messageProcessed);
				break;
			case MessageType_Encrypted_Reliable_EM_Secure:
			case MessageType_Encrypted_Reliable_EM_Fast:
			case MessageType_Encrypted_UnReliable_EM_Secure:
			case MessageType_Encrypted_UnReliable_EM_Fast:
			{
				CReceivedMessage decryptedReceivedMessage;
				if ( m_owner->ProcessMessage_Encrypted(type, receivedInfo, decryptedReceivedMessage.m_unsafeMessage) )
				{
					//decryptedReceivedMessage.m_actionTime = receivedInfo.m_actionTime;
					decryptedReceivedMessage.m_relayed = receivedInfo.m_relayed;
					decryptedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
					decryptedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
					decryptedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

					messageProcessed |= ProcessMessage_ProudNetLayer(socket, decryptedReceivedMessage);
				}
			}
			break;

			case MessageType_Compressed:
			{
				CReceivedMessage uncompressedReceivedMessage;
				if ( m_owner->ProcessMessage_Compressed(receivedInfo, uncompressedReceivedMessage.m_unsafeMessage) )
				{
					//uncompressedReceivedMessage.m_actionTime = receivedInfo.m_actionTime;
					uncompressedReceivedMessage.m_relayed = receivedInfo.m_relayed;
					uncompressedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
					uncompressedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
					uncompressedReceivedMessage.m_compressMode = receivedInfo.m_compressMode;
					// Send 할 때 압축 후 암호화를 한다. Recv 시에 복호화 후 압축을 푼다. 따라서 여기서 m_encryptMode 값을 셋팅해 주어야 압축정보를 제대로 받을 수 있다.
					uncompressedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

					messageProcessed |= ProcessMessage_ProudNetLayer(socket, uncompressedReceivedMessage);
				}
				break;
			}

			case MessageType_PolicyRequest:
				messageProcessed = true;
				break;
			case MessageType_P2PReliablePing:
				ProcessMessage_P2PReliablePing(receivedInfo);
				messageProcessed = true;
				break;
			case MessageType_P2PReliablePong:
				ProcessMessage_P2PReliablePong(receivedInfo);
				messageProcessed = true;
				break;
		}

		// 만약 잘못된 메시지가 도착한 것이면 이미 ProudNet 계층에서 처리한 것으로 간주하고
		// 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
		// 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
		int l1 = receivedInfo.GetReadOnlyMessage().GetLength();
		int l2 = receivedInfo.GetReadOnlyMessage().GetReadOffset();

		if ( messageProcessed == true && l1 != l2 && !MessageType_IsEncrypted(type) ) // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
		{
			messageProcessed = true;
			// 2009.11.26 add by ulelio : 에러가 난시점의 msg를 기록한다.

			String comment;
			comment.Format(_PNT("%s위치에서 MessageType:%d"), _PNT("NC.PNL"), (int)type);
			ByteArray lastReceivedMessage(recvedMsg.GetData(), recvedMsg.GetLength());
			m_owner->EnqueError(ErrorInfo::From(ErrorType_InvalidPacketFormat, receivedInfo.m_remoteHostID, comment, lastReceivedMessage));
		}

		//		AssureMessageReadOkToEnd(messageProcessed, type,receivedInfo);

		if ( messageProcessed == false )
		{
			recvedMsg.SetReadOffset(orgReadOffset);
			return false;
		}

		return true;
	}

	void CNetClientWorker::ProcessMessage_PeerUdp_PeerHolepunch(CReceivedMessage &ri)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();
		CP2PConnectionTrialContext::ProcessPeerHolepunch(m_owner, ri);
	}

	void CNetClientWorker::ProcessMessage_PeerHolepunchAck(CReceivedMessage &ri)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();
		CP2PConnectionTrialContext::ProcessPeerHolepunchAck(m_owner, ri);
	}

	void CNetClientWorker::ProcessMessage_ReliableUdp_Frame(CSuperSocket* udpSocket, CReceivedMessage &ri)
	{
		//puts("CNetClientWorker::ProcessMessage_ReliableUdp_Frame");

		// add to remote peer's reliable UDP receive queue by frame number
		m_owner->LockMain_AssertIsLockedByCurrentThread();

		// UDP addr를 근거로 remote peer를 찾는다.
		// GetPeerByUdpAddr는 홀펀칭이 아직 안된 remote peer는 찾지 못하므로 OK.
		CRemotePeer_C *rp = m_owner->GetPeerByUdpAddr(ri.m_remoteAddr_onlyUdp, false);
		if ( rp != NULL &&
			rp->m_ToPeerReliableUdp.m_failed == false )
		{
			// 사용하기전에 클리어 해준다.
			POOLED_ARRAY_LOCAL_VAR(CReceivedMessageList, extractedMessages);

			extractedMessages.Clear();
			ErrorType extractError;
			rp->m_ToPeerReliableUdp.EnqueReceivedFrameAndGetFlushedMessages(ri.m_unsafeMessage, extractedMessages, extractError);
			if ( extractError != ErrorType_Ok )
			{
				m_owner->EnqueError(ErrorInfo::From(extractError, rp->m_HostID, _PNT("Stream Extract Error at Reliable UDP")));
			}

			for ( CReceivedMessageList::iterator i0 = extractedMessages.begin(); i0 != extractedMessages.end(); i0++ )
			{
				CReceivedMessage &rii = *i0;
				rii.m_relayed = ri.m_relayed;
				//rii.m_actionTime = m_owner->GetAbsoluteTime();
				// 각 메시지에 대해서 재귀 호출을 한다. 이렇게 안하면 무한루프에 걸린다.
				assert(rii.m_unsafeMessage.GetReadOffset() == 0);
				ProcessMessage_ProudNetLayer(udpSocket, rii);
			}
		}
		else
		{
			/* symmetric NAT 홀펀칭을 하는 경우 등에서, 의도하지 않은 다른 peer로부터의 메시지 수신이 있을 수 있다.
			이런 경우, 즉 대응되지 않는 패킷인 경우라도 끝까지 다 읽은셈 쳐야 한다.
			안그러면 엄한데서 온 UDP frame message가 있는 경우 bad stream exception이 발생하기 때문이다. */
			ri.m_unsafeMessage.SkipRead(ri.m_unsafeMessage.GetLength() - ri.m_unsafeMessage.GetReadOffset());
		}
	}

	void CNetClientWorker::ProcessMessage_NotifyClientServerUdpMatched(CMessage &msg)
	{
		// set that TCP fallback nomore		
		msg.Read(m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchMagicNumber); // 한번 더 읽는다. 확인사살.
		m_owner->m_remoteServer->GetToServerUdpFallbackable()->SetRealUdpEnabled(true);

		// 로컬 이벤트
		LocalEvent e;
		e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
		e.m_type = LocalEventType_ServerUdpChanged;
		e.m_remoteHostID = HostID_Server;
		m_owner->EnqueLocalEvent(e, m_owner->m_remoteServer);

		// 로그
		if ( m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0 )
		{
			m_owner->Log(0, LogCategory_P2P, _PNT("Holepunch to server UDP successful."));
		}
	}

	void CNetClientWorker::ProcessMessage_ServerHolepunchAck(CReceivedMessage &ri)
	{
		// ignore unknown magic number
		CMessage &msg = ri.m_unsafeMessage;
		Guid magicNumber;
		AddrPort addrOfHereAtServer;
		if ( msg.Read(magicNumber) == false )
			return;
		if ( msg.Read(addrOfHereAtServer) == false )
			return;
		if ( magicNumber != m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchMagicNumber )
			return;
		if ( m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr != ri.m_remoteAddr_onlyUdp )
			return;

		assert(m_owner->Get_ToServerUdpSocketLocalAddr().m_port != 0);
		//assert(m_owner->Get_ToServerUdpSocketLocalAddr().m_port != 0xffff);

		// send my magic number & client local UDP address via TCP
		CMessage header; header.UseInternalBuffer();
		header.Write((char)MessageType_NotifyHolepunchSuccess);
		header.Write(m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchMagicNumber);
		header.Write(m_owner->Get_ToServerUdpSocketLocalAddr());
		header.Write(addrOfHereAtServer);
		CSendFragRefs sendData;
		sendData.Add(header);

		m_owner->m_remoteServer->m_ToServerUdp->m_localAddrAtServer = addrOfHereAtServer;

		m_owner->m_remoteServer->m_ToServerTcp->AddToSendQueueWithSplitterAndSignal_Copy(
			sendData,
			SendOpt(),
			m_owner->m_simplePacketMode);

		if ( m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0 )
		{
			m_owner->Log(0,
						 LogCategory_P2P,
						 String::NewFormat(_PNT("Message_ServerHolepunchAck. AddrOfHereAtServer=%s"),
						 addrOfHereAtServer.ToString().GetString()));
		}
	}

	// per-peer UDP socket의 외부 주소를 얻기 위한, 클라-서버 홀펀칭이 성공했음.
	void CNetClientWorker::ProcessMessage_PeerUdp_ServerHolepunchAck(CReceivedMessage &ri)
	{
		// #ifdef LOG_RESTORE_UDP
		// 		if(m_owner->m_logRestoreUdp)
		// 		{
		// 			NTTNTRACE("LOG_RESTORE_UDP: 서버와의 홀펀칭 ACK가 도착했음.\n");
		// 		}
		// #endif

		// ignore unknown magic number
		CMessage &msg = ri.m_unsafeMessage;

		Guid magicNumber;
		AddrPort addrOfHereAtServer;
		HostID peerID;

		if ( msg.Read(magicNumber) == false )
			return;
		if ( msg.Read(addrOfHereAtServer) == false )
			return;
		if ( msg.Read(peerID) == false )
			return;

		CRemotePeer_C* peer = m_owner->GetPeerByHostID_NOLOCK(peerID);
		if ( peer && !peer->m_garbaged && peer->m_p2pConnectionTrialContext )
		{
			peer->m_p2pConnectionTrialContext->ProcessMessage_PeerUdp_ServerHolepunchAck(
				ri,
				magicNumber,
				addrOfHereAtServer,
				peerID);
		}
	}

	// 서버로 부터의 브로드캐스팅 메세지를 클라이언트 단에서 릴레이하기 위해서 설정
	// 릴레이 해줄 피어가 처리한다.
	// MessageType_S2CRoutedMulticast1 를 처리하기 위한 프로세스를 수행한다.
	// hostIDArray를 얻어서 바로 Data를 Sending 해 준다.
	// 서버로 부터 넘어온 메세지를 피어간 릴레이를 하기 위해서 설정
	bool CNetClientWorker::ProcessMessage_S2CRoutedMulticast1(CSuperSocket *udpSocket, CReceivedMessage &ri)
	{
		CMessage &msg = ri.m_unsafeMessage;

		// 서버로부터 온 메시지가 아니면 무시한다.
		if ( ri.GetRemoteHostID() != HostID_Server )
			return false;

		MessagePriority priority;
		int64_t uniqueID;
		if ( !msg.Read(priority) ||
			!msg.ReadScalar(uniqueID) )
			return false;

		POOLED_ARRAY_LOCAL_VAR(HostIDArray, hostIDArray);
		if ( msg.Read(hostIDArray) == false )
		{
			return false;
		}

		// 내용을 읽는다.
		ByteArray payload;
		if ( msg.Read(payload) == false )
		{
			return false;
		}

		CMessage msg1; msg1.UseInternalBuffer();

		msg1.Write((char)MessageType_S2CRoutedMulticast2);
		msg1.Write(payload);

		SendOpt udpSendOpt(g_UnreliableSendForPN);
		udpSendOpt.m_priority = priority;
		udpSendOpt.m_uniqueID.m_value = uniqueID;

		// NOTE: route1 메시지 자체가 MTU를 넘는 경우가 있을 수 있다. 
		// 따라서 route2에서도 MTU 단위로 쪼개는 것이 필요할 수 있다. false로 세팅하지 말자.
		//udpSendOpt.m_INTERNAL_USE_fraggingOnNeed = false;

		/* peer가 relayed thru server 상태인 경우 다시 릴레이하면 효율성이 무시되어버린다.
		일단, 서버에서 direct P2P라는걸로 인식된 상태이므로 그걸 받아들이고 직빵 전송해야 한다. */
		for ( int i = 0; i < (int)hostIDArray.GetCount(); i++ )
		{
			if ( hostIDArray[i] != HostID_Server &&
				hostIDArray[i] != m_owner->GetVolatileLocalHostID() )
			{
				CRemotePeer_C *remotePeer = m_owner->GetPeerByHostID_NOLOCK(hostIDArray[i]);
				if ( remotePeer == NULL || remotePeer->m_garbaged || remotePeer->IsRelayedP2P() == true )
				{
					NTTNTRACE("S2C routed broadcast에서 도달 못하는 상황. RemotePeers: ");
					for ( AuthedHostMap::iterator iPeer = m_owner->m_authedHostMap.begin();
						 iPeer != m_owner->m_authedHostMap.end(); iPeer++ )
					{
						NTTNTRACE("%d ", iPeer->GetFirst());
					}
					NTTNTRACE("\n");
				}
				else
				{
					// 상대가 relay일 때 이는 무시되겠지만 그래도 상관없다. 어차피 unreliable이고 조만간 서버에서는 
					// 제대로 조정된 것으로 보내질 터이니.					
					if ( remotePeer->m_udpSocket )
					{
						remotePeer->m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(hostIDArray[i],
																						  FilterTag::CreateFilterTag(m_owner->GetVolatileLocalHostID(), hostIDArray[i]),
																						  remotePeer->m_P2PHolepunchedLocalToRemoteAddr,
																						  msg1,
																						  GetPreciseCurrentTimeMs(),
																						  udpSendOpt);
					}
				}
			}
		}

		// fake 처리 - 릴레이를 하는 peer에게 서버로 부터 온 메세지라고 인지시킨다.
		CReceivedMessage ri1;
		ri1.m_remoteHostID = HostID_Server;
		//ri1.m_actionTime = m_owner->GetAbsoluteTime();
		ri1.m_unsafeMessage.UseInternalBuffer();
		ri1.m_unsafeMessage.Write(&payload[0], (int)payload.GetCount());

		assert(ri1.m_unsafeMessage.GetReadOffset() == 0);
		ProcessMessage_ProudNetLayer(udpSocket, ri1);

		return true;
	}

	// 피어로 부터 온 서버 메세지를 처리하기 위한 메서드.
	// 피어로 부터 받은 서버로 부터의 릴레이드 메세지를 받는 피어가 처리한다.
	bool CNetClientWorker::ProcessMessage_S2CRoutedMulticast2(CSuperSocket *udpSocket, CReceivedMessage &ri)
	{
		CMessage &msg = ri.m_unsafeMessage;

		// Body를 읽는다.
		ByteArray byteArray;
		if ( msg.Read(byteArray) == false )
		{
			return false;
		}

		// fake 처리 - 릴레이를 하는 피어에게 서버로 부터 온 메세지라고 인지시킨다.
		CReceivedMessage ri1;
		ri1.m_remoteHostID = HostID_Server;
		//ri1.m_actionTime = m_owner->GetAbsoluteTime();
		ri1.m_unsafeMessage.UseInternalBuffer();
		ri1.m_unsafeMessage.Write(&byteArray[0], (int)byteArray.GetCount());
		ri1.m_relayed = true;

		assert(ri1.m_unsafeMessage.GetReadOffset() == 0);
		ProcessMessage_ProudNetLayer(udpSocket, ri1);

		return true;
	}

	void CNetClientWorker::ProcessMessage_RequestStartServerHolepunch(CMessage &msg)
	{
		// magic number
		msg.Read(m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchMagicNumber);

		assert(m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr.m_binaryAddress != 0xffffffff);

		// begin sending magic number to server via UDP, repeat
		m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchTimeMs = 0; // 완전 과거로 돌림. 즉, 바로 시행하라는 뜻이 됨.
		m_owner->m_remoteServer->GetToServerUdpFallbackable()->m_holepunchTrialCount = 0;
	}

	void CNetClientWorker::ProcessMessage_ConnectServerTimedout(CMessage &msg)
	{
		m_owner->EnqueueConnectFailEvent(ErrorType_ConnectServerTimeout,
			ErrorInfo::From(ErrorType_ConnectServerTimeout,
			HostID_Server,
			_PNT("Remote detected no receive.")));
		SetState(Disconnecting);
		//XXAACheck(m_owner);
	}

	void CNetClientWorker::ProcessMessage_NotifyStartupEnvironment(CSuperSocket* socket, CMessage &msg)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();

		ByteArray publicKeyBlob;
		ByteArray randomBlock;
		ByteArray fastEncryptRandomBlock;
		ByteArray encryptedAESKeyBlob;
		ByteArray encryptedFastKey;

		bool enableLog;
		CNetSettings settings;

		if ( m_owner->m_simplePacketMode == false )
		{
			// public key, server UDP port 주소를 얻는다.
			if ( msg.Read(enableLog) == false )
			{
				m_owner->EnqueueDisconnectionEvent(ErrorType_ProtocolVersionMismatch, 
					ErrorType_TCPConnectFailure, _PNT("NSE-R1"));
				SetState(Disconnecting);

				return;
			}

			if ( !msg.Read(settings) )
			{
				m_owner->EnqueueDisconnectionEvent(ErrorType_ProtocolVersionMismatch,
					ErrorType_TCPConnectFailure, _PNT("NSE-R2"));
				SetState(Disconnecting);
				return;
			}

			if ( !msg.Read(publicKeyBlob) )
			{
				m_owner->EnqueueDisconnectionEvent(ErrorType_ProtocolVersionMismatch,
					ErrorType_TCPConnectFailure, _PNT("NSE-R3"));
				SetState(Disconnecting);
				return;
			}
		}

		if ( msg.GetReadOffset() != msg.GetLength() )
		{
			m_owner->EnqueueDisconnectionEvent(ErrorType_ProtocolVersionMismatch,
				ErrorType_TCPConnectFailure, _PNT("NSE-R4"));
			SetState(Disconnecting);
			return;
		}

		// 이 메시지를 받은 TCP socket이 연결유지기능 재접속 후보인 경우
		// 메시지 처리를 하지 말자.
		// 이 메시지는 서버에 처음 연결하는 경우에만 핸들링 해야 한다.
		if ( m_owner->m_autoConnectionRecoveryContext != NULL
			&& m_owner->m_autoConnectionRecoveryContext->m_tcpSocket == socket )
		{
			return;
		}

		if ( m_owner->m_simplePacketMode == false )
		{
			// 서버 연결 시점 전에 이미 로그를 남겨야 하냐 여부도 갱신받아야 하므로.
			m_owner->m_enableLog = enableLog;
			m_owner->m_settings = settings;

			// 백업. 재접속시 서버에서 또 받지 않고 그냥 재사용하기 위해.
			m_owner->m_publicKeyBlob = publicKeyBlob;
		}

		m_owner->m_ReliablePing_Timer.SetIntervalMs(m_owner->GetReliablePingTimerIntervalMs());
		m_owner->m_ReliablePing_Timer.Reset(GetPreciseCurrentTimeMs());

		if ( m_owner->m_settings.m_enablePingTest )
			m_owner->m_enablePingTestEndTime = GetPreciseCurrentTimeMs();

		// nagle 알고리즘을 켜거나 끈다
		m_owner->m_remoteServer->m_ToServerTcp->SetEnableNagleAlgorithm(m_owner->m_settings.m_enableNagleAlgorithm);

		if ( m_owner->m_simplePacketMode == false && m_owner->m_settings.m_enableEncryptedMessaging == true ) //youngjun.ko
		{
#ifndef ENCRYPT_NONE
			if ( CCryptoRsa::CreateRandomBlock(randomBlock, m_owner->m_settings.m_encryptedMessageKeyLength) != true ||
				CCryptoAes::ExpandFrom(m_owner->m_selfP2PSessionKey.m_aesKey, randomBlock.GetData(), m_owner->m_settings.m_encryptedMessageKeyLength / 8) != true ||
				CCryptoRsa::CreateRandomBlock(fastEncryptRandomBlock, m_owner->m_settings.m_fastEncryptedMessageKeyLength) != true ||
				CCryptoFast::ExpandFrom(m_owner->m_selfP2PSessionKey.m_fastKey, fastEncryptRandomBlock.GetData(), m_owner->m_settings.m_fastEncryptedMessageKeyLength / 8) != true )
			{
				m_owner->EnqueueDisconnectionEvent(ErrorType_EncryptFail, ErrorType_TCPConnectFailure, _PNT("ENC1"));

				if ( m_owner->m_eventSink_NOCSLOCK )
				{
					m_owner->EnqueError(ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("Failed to create SessionKey.")));
				}

				SetState(Disconnecting);
				return;
			}

			// AES Key 는 공개키로 암호화하고, Fast Key는 AES Key로 암호화한다.
			if ( CCryptoRsa::CreateRandomBlock(randomBlock, m_owner->m_settings.m_encryptedMessageKeyLength) != true ||
				CCryptoAes::ExpandFrom(m_owner->m_toServerSessionKey.m_aesKey, randomBlock.GetData(), m_owner->m_settings.m_encryptedMessageKeyLength / 8) != true ||
				CCryptoRsa::EncryptSessionKeyByPublicKey(encryptedAESKeyBlob, randomBlock, publicKeyBlob) != true ||
				CCryptoRsa::CreateRandomBlock(fastEncryptRandomBlock, m_owner->m_settings.m_fastEncryptedMessageKeyLength) != true ||
				CCryptoFast::ExpandFrom(m_owner->m_toServerSessionKey.m_fastKey, fastEncryptRandomBlock.GetData(), m_owner->m_settings.m_fastEncryptedMessageKeyLength / 8) != true ||
				CCryptoAes::EncryptByteArray(m_owner->m_toServerSessionKey.m_aesKey, fastEncryptRandomBlock, encryptedFastKey) != true )
			{
				m_owner->EnqueueDisconnectionEvent(ErrorType_EncryptFail, ErrorType_TCPConnectFailure, _PNT("ENC2"));

				if ( m_owner->m_eventSink_NOCSLOCK )
				{
					m_owner->EnqueError(ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("Failed to create SessionKey.")));
				}

				SetState(Disconnecting);
				return;
			}
#endif
		}

		// ACR 재연결 과정에서 재사용됨
		m_owner->m_toServerSessionKey.m_aesKeyBlock = randomBlock;
		// 이건 재사용 안됨
		m_owner->m_toServerSessionKey.m_fastKeyBlock = fastEncryptRandomBlock;

		m_owner->m_firstTcpPingedTime = GetPreciseCurrentTimeMs();

		CMessage sendMsg;
		sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_RequestServerConnection);

		if ( m_owner->m_simplePacketMode == false )
		{
			int platform;
#ifdef __MARMALADE__
			{
				s3eDeviceOSID a = (s3eDeviceOSID)s3eDeviceGetInt(S3E_DEVICE_OS);
				switch (a)
				{
					case S3E_OS_ID_WINDOWS:
					case S3E_OS_ID_OSX:
					case S3E_OS_ID_LINUX:
					case S3E_OS_ID_PSP:
					case S3E_OS_ID_PS3:
					case S3E_OS_ID_X360:
					case S3E_OS_ID_WII:
						platform = RuntimePlatform_C;
						break;
					case S3E_OS_ID_IPHONE:
						platform = RuntimePlatform_NIPhone;
						break;
					case S3E_OS_ID_ANDROID:
						platform = RuntimePlatform_NAndroid;
						break;
					default:
						platform = RuntimePlatform_C;
						break;
				}
			}
#elif defined(_WIN32)
			switch ( GetProcessorType() )
			{
				case PROCESSOR_ARCHITECTURE_AMD64:
				case PROCESSOR_ARCHITECTURE_IA64:
				case PROCESSOR_ARCHITECTURE_INTEL:
					platform = RuntimePlatform_C;
					break;
				default:
					platform = RuntimePlatform_NIPhone; // TODO: 이게 아니라 Windows Mobile로 대체되어야 함. 언젠가 들어가야 함.
					break;
			}
#elif defined(__MACH__)// NOTE: __FreeBSD__ also has this.
			platform = RuntimePlatform_NIPhone; // TODO: osx와 ios를 구별 못함. 따라서 ios를 구별하기 위한 코드가 들어가야 함.
#elif defined(__linux__) || defined(__ORBIS__)
			platform = RuntimePlatform_NAndroid;
#else
			platform = ? ;
#endif

			m_owner->m_isProactorAsyncModel = (platform == RuntimePlatform_C);

			sendMsg.Write(platform);

			// ACR 기능
			sendMsg.Write(m_owner->m_enableAutoConnectionRecovery);

#ifndef ENCRYPT_NONE
			// youngjun.ko
			if (m_owner->m_settings.m_enableEncryptedMessaging == true) {
				sendMsg.Write(encryptedAESKeyBlob);
				sendMsg.Write(encryptedFastKey);
			}
#endif
			sendMsg.Write(m_owner->m_connectionParam.m_userData);
			sendMsg.Write(m_owner->m_connectionParam.m_protocolVersion);
			sendMsg.Write(m_owner->m_internalVersion);
		}

		m_owner->m_remoteServer->m_ToServerTcp->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_owner->m_simplePacketMode);
	}

	void CNetClientWorker::ProcessMessage_NotifyServerConnectSuccess(CMessage &msg)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();

		int64_t firstTcpLatency = GetPreciseCurrentTimeMs() - m_owner->m_firstTcpPingedTime;

		// 행여나 나올 수 없는 값이 나온 것을 교정하기 위함
		if ( firstTcpLatency > 1000 * 100 )
			firstTcpLatency = 0;

		m_owner->ServerTcpPing_UpdateValues((int)firstTcpLatency);

		ByteArrayPtr userData;
		userData.UseInternalBuffer();

		Guid serverInstanceGuid;
		HostID localHostID;
		NamedAddrPort localAddrAtServer;
		int C2SNextMessageID, S2CNextMessageID;

		// set local HostID: connect established! but TCP fallback mode yet
		if ( !msg.Read(localHostID) ||
			!msg.Read(userData) ||
			!msg.Read(localAddrAtServer) )
		{
			ProcessReadPacketFailed();
			return;
		}

		if ( !m_owner->IsSimplePacketMode() )
		{
			if ( !msg.Read(serverInstanceGuid) )
			{
				ProcessReadPacketFailed();
				return;
			}

			// 연결유지기능 AMR
			if ( !msg.Read(C2SNextMessageID)
				|| !msg.Read(S2CNextMessageID) )
			{
				ProcessReadPacketFailed();
				return;
			}
		}

		m_owner->m_loopbackHost->m_backupHostID = m_owner->m_loopbackHost->m_HostID = localHostID;

		if ( m_owner->m_enableAutoConnectionRecovery && !m_owner->IsSimplePacketMode() )
		{
			// 수신미보장 부분을 위한 messageID 동기화를 끝냈으니, 이제야 켜도 된다.
			m_owner->m_remoteServer->m_ToServerTcp->AcrMessageRecovery_Init(C2SNextMessageID, S2CNextMessageID);
		}

		// NetServer부터 발급받은 HostID로 설정
		m_owner->m_loopbackHost->m_HostID = localHostID;
		// CandidateHosts에서 LoopbackHost와 RemoteServer를 제거
		m_owner->Candidate_Remove(m_owner->m_loopbackHost);
		m_owner->Candidate_Remove(m_owner->m_remoteServer);
		// AuthedHosts에 LoopbackHost와 RemoteServer를 추가
		m_owner->m_authedHostMap.Add(localHostID, m_owner->m_loopbackHost);
		m_owner->m_authedHostMap.Add(HostID_Server, m_owner->m_remoteServer);

		if ( m_owner->m_remoteServer->m_ToServerTcp != NULL )
		{
			m_owner->m_remoteServer->m_ToServerTcp->m_localAddrAtServer = AddrPort::From(localAddrAtServer);
		}
		else
			assert(0);

		// connection hint까지 왔으면, TCP 연결을 add port mapping upnp를 걸기 위해, 여기서부터 미리 켜도록 한다.
		m_owner->StartupUpnpOnNeed();

		{
			CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
			// enque connect ok event
			LocalEvent e;
			e.m_type = LocalEventType_ConnectServerSuccess;
			e.m_userData = userData;
			e.m_remoteHostID = HostID_Server;
			e.m_remoteAddr = AddrPort::FromHostNamePort(m_owner->m_connectionParam.m_serverIP, m_owner->m_connectionParam.m_serverPort);
			m_owner->EnqueLocalEvent(e, m_owner->m_remoteServer);
			m_owner->m_serverInstanceGuid = serverInstanceGuid;

#ifdef VIZAGENT
			// VIZ
			if(m_owner->m_vizAgent)
			{
				CriticalSectionLock vizlock(m_owner->m_vizAgent->m_cs,true);
				CServerConnectionState ignore;
				m_owner->m_vizAgent->m_c2sProxy.NotifyCli_ConnectionState(HostID_Server, g_ReliableSendForPN, m_owner->GetServerConnectionState(ignore));
			}
#endif
		}
		if ( m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0 )
		{
			m_owner->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Connecting to server successful. serverIP=%s, serverPort=%d"), m_owner->m_connectionParam.m_serverIP.GetString(), m_owner->m_connectionParam.m_serverPort));
		}

#if defined(_WIN32)        
		// 클라이언트 실행 환경을 수집한다.
		if ( m_owner->GetVolatileLocalHostID() == 50 )
		{
			if ( CRandom::StaticGetInt() % 3 == 0 )
			{
				String txt;

				PNTCHAR module_file_name[_MAX_PATH];
				::GetModuleFileName(NULL, module_file_name, _COUNTOF(module_file_name));

				if ( m_owner->m_remoteServer->m_ToServerTcp )
				{
					txt.Format(_PNT("Usage: %s ## %s ## %s"),
							   m_owner->m_remoteServer->m_ToServerTcp->m_localAddrAtServer.ToString().GetString(),
							   m_owner->GetNatDeviceName().GetString(),
							   module_file_name);
					CErrorReporter_Indeed::Report(txt);
				}
				else
					assert(0);
			}
		}
#endif

	}

	void CNetClientWorker::ProcessMessage_NotifyProtocolVersionMismatch(CMessage &msg)
	{
		m_owner->EnqueueConnectFailEvent(ErrorType_ProtocolVersionMismatch, _PNT("NPVM1"));
		SetState(Disconnecting);

		//XXAACheck(m_owner);
	}

	void CNetClientWorker::ProcessMessage_NotifyServerDeniedConnection(CMessage &msg)
	{
		ByteArrayPtr reply;
		reply.UseInternalBuffer();
		msg.Read(reply);
		m_owner->EnqueueConnectFailEvent(ErrorType_NotifyServerDeniedConnection, _PNT("NPDC"),
			SocketErrorCode_Ok, reply);
		SetState(Disconnecting);

		//XXAACheck(m_owner);
	}

	void CNetClientWorker::ProcessMessage_UnreliableRelay2(CSuperSocket *socket, CReceivedMessage &receivedInfo)
	{
		CMessage& msg = receivedInfo.GetReadOnlyMessage();
		// 서버로부터 온 메시지가 아니면 무시한다.
		if ( receivedInfo.GetRemoteHostID() != HostID_Server )
			return;

		m_owner->LockMain_AssertIsLockedByCurrentThread();

		HostID senderHostID;
		int payloadLength;

		if ( !msg.Read(senderHostID) ||
			!msg.ReadScalar(payloadLength) )
		{
			return;
		}

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( payloadLength < 0 || payloadLength >= m_owner->m_settings.m_clientMessageMaxLength )
		{
			assert(0);
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		assert(msg.GetReadOffset() + payloadLength == msg.GetLength());
		CMessage payload;
		if ( !msg.ReadWithShareBuffer(payload, payloadLength) )
		{
			assert(0);
			return;
		}

		CRemotePeer_C* rp = m_owner->GetPeerByHostID_NOLOCK(senderHostID);
		if ( rp != NULL && !rp->m_garbaged )
		{
			CReceivedMessage ri;
			ri.m_unsafeMessage.UseInternalBuffer();
			ri.m_relayed = true;
			payload.CopyTo(ri.m_unsafeMessage); // internal buffer를 쓰게 해야 하므로 복사를 한다.
			ri.m_unsafeMessage.SetReadOffset(0);
			ri.m_remoteHostID = senderHostID;
			//ri.m_actionTime = m_owner->GetAbsoluteTime();

			// recursive call
			// 어차피 이 Rmi는 networker thread에서 호출되므로 안전하다.
			assert(ri.m_unsafeMessage.GetReadOffset() == 0);
			ProcessMessage_ProudNetLayer(socket, ri);
		}
	}

	// 타 피어가 릴레이로 전송한 P2P 메시지
	void CNetClientWorker::ProcessMessage_ReliableRelay2(CSuperSocket* socket, CMessage &msg)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();

		HostID senderHostID;
		int frameNumber;
		int contentLength;

		if ( !msg.Read(senderHostID) ||
			!msg.Read(frameNumber) ||
			!msg.ReadScalar(contentLength) )
		{
			return;
		}

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( contentLength < 0 || contentLength >= m_owner->m_settings.m_clientMessageMaxLength )
		{
			assert(0);
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		assert(msg.GetReadOffset() + contentLength == msg.GetLength());
		CMessage content;
		if ( !msg.ReadWithShareBuffer(content, contentLength) )
		{
			assert(0);
			return;
		}

		CRemotePeer_C* rp = m_owner->GetPeerByHostID_NOLOCK(senderHostID);
		if ( rp != NULL && !rp->m_garbaged && rp->m_ToPeerReliableUdp.m_failed == false )
		{
			ReliableUdpFrame longFrame;
			CRemotePeerReliableUdpHelper::BuildRelayed2LongDataFrame(frameNumber, content, longFrame);

			POOLED_ARRAY_LOCAL_VAR(CReceivedMessageList, extractedMessages);

			ErrorType extractError;
			rp->m_ToPeerReliableUdp.EnqueReceivedFrameAndGetFlushedMessages(longFrame, extractedMessages, extractError);
			if ( extractError != ErrorType_Ok )
			{
				m_owner->EnqueError(ErrorInfo::From(extractError, rp->m_HostID, _PNT("Stream Extract Error at Reliable UDP")));
			}

			for ( CReceivedMessageList::iterator i = extractedMessages.begin(); i != extractedMessages.end(); i++ )
			{
				CReceivedMessage &rii = *i;
				rii.m_relayed = true;
				//rii.m_actionTime = m_owner->GetAbsoluteTime();
				rii.m_unsafeMessage.SetReadOffset(0);
				rii.m_remoteHostID = senderHostID;

				// recursive call
				// 어차피 이 Rmi는 networker thread에서 호출되므로 안전하다.
				assert(rii.m_unsafeMessage.GetReadOffset() == 0);
				ProcessMessage_ProudNetLayer(socket, rii);
			}
		}
	}

	void CNetClientWorker::ProcessMessage_LingerDataFrame2(CSuperSocket* udpSocket, CReceivedMessage& rm)
	{
		// 서버에서 보낸 경우에만 제대로 처리한다.
		if ( rm.GetRemoteHostID() != HostID_Server )
			return;

		m_owner->LockMain_AssertIsLockedByCurrentThread();

		// 사용하지 않는 변수 삭제
		// HostID remote = rm.GetRemoteHostID();
		CMessage& msg = rm.GetReadOnlyMessage();

		HostID senderRemoteHostID;
		int frameNumber;
		int frameLength;

		if ( !msg.Read(senderRemoteHostID) ||
			!msg.Read(frameNumber) ||
			!msg.ReadScalar(frameLength) )
			return;

		// 잘못된 크기의 데이터가 와도 즐 친다.
		if ( frameLength < 0 || frameLength >= m_owner->m_settings.m_clientMessageMaxLength )
		{
			assert(0);
			return;
		}

		// 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
		assert(msg.GetReadOffset() + frameLength == msg.GetLength());
		CMessage frameData;
		if ( !msg.ReadWithShareBuffer(frameData, frameLength) )
		{
			assert(0);
			return;
		}

		// reliable UDP 프레임을 꺼내서 처리한다.
		CRemotePeer_C* rp = m_owner->GetPeerByHostID_NOLOCK(senderRemoteHostID);
		if ( rp != NULL && !rp->m_garbaged && rp->m_ToPeerReliableUdp.m_failed == false )
		{
			//VERBOSE2(frameNumber, rp);
			ReliableUdpFrame frame;
			frame.m_type = ReliableUdpFrameType_Data;
			frame.m_frameNumber = frameNumber;
			frame.m_data.UseInternalBuffer();
			frame.m_data.SetCount(frameData.GetLength());
			memcpy(frame.m_data.GetData(), frameData.GetData(), frameData.GetLength());

			POOLED_ARRAY_LOCAL_VAR(CReceivedMessageList, extractedMessages);
			
			ErrorType extractError;
			rp->m_ToPeerReliableUdp.EnqueReceivedFrameAndGetFlushedMessages(frame, extractedMessages, extractError);
			if ( extractError != ErrorType_Ok )
			{
				m_owner->EnqueError(ErrorInfo::From(extractError, rp->m_HostID, _PNT("Stream Extract Error at Reliable UDP")));
			}

			for ( CReceivedMessageList::iterator i=extractedMessages.begin(); i != extractedMessages.end(); i++ )
			{
				CReceivedMessage& ri = *i;
				ri.m_relayed = true;
				//ri.m_actionTime = m_owner->GetAbsoluteTime();
				// recursive call
				// 어차피 이 Rmi는 networker thread에서 호출되므로 안전하다.
				assert(ri.m_unsafeMessage.GetReadOffset() == 0);
				ProcessMessage_ProudNetLayer(udpSocket, ri);
			}
		}
	}

	// 서버에서 와야 하는 메시지인데 P2P로 도착한 경우
	// (혹은 아직 서버와의 연결 처리가 다 끝나지 않은 상태이면 HostID_None에서 오는 경우일테고 이때는 무시)
	bool CNetClientWorker::IsFromRemoteClientPeer(CReceivedMessage &receivedInfo)
	{
		return receivedInfo.GetRemoteHostID() != HostID_Server &&
			receivedInfo.GetRemoteHostID() != HostID_None;
	}


	bool CNetClientImpl::Main_IssueConnect(SocketErrorCode* outCode)
	{
		// 이미 non-block 설정된 socket이므로 아래 함수는 즉시 리턴할 것이다.		
		SocketErrorCode r;
	L1:
#if defined (_WIN32)
		// async connect
		r = m_remoteServer->m_ToServerTcp->IssueConnectEx(
			m_connectionParam.m_serverIP,
			m_connectionParam.m_serverPort);
#else
		// non-block connect
		r = m_remoteServer->m_ToServerTcp->SetNonBlockingAndConnect(
			m_connectionParam.m_serverIP,
			m_connectionParam.m_serverPort);
#endif

		if ( r == SocketErrorCode_Ok )
		{
			return true;
		}

		if ( r == SocketErrorCode_Intr )
			goto L1;

		// EAGAIN의 경우 Win32에서는 WSAEWOULDBLOCK이 대신 한다.
		// EAGAIN에 대해서 따로 처리 하지 않아도 된다.
		if ( r == SocketErrorCode_AlreadyIsConnected
			|| r == SocketErrorCode_AlreadyAttempting
			|| r == SocketErrorCode_InProgress
			|| r == SocketErrorCode_WouldBlock
			)
		{
			return true;
		}

		*outCode = r;
		return false;
	}

	// 에러가 났거나 타임아웃 상황
	void CNetClientImpl::Heartbeat_ConnectFailCase(SocketErrorCode code, const String& comment)
	{
		if ( m_worker->GetState() <= CNetClientWorker::Connecting )
		{
			// CloseSocket 후 Disconnecting mode로 전환
			//m_remoteServer->m_ToServerTcp->SetSocketVerboseFlag(true); // 이것을 설정하면 쓸데없이 잔뜩 로그 쌓임. 디버깅할때나 하자.

			// close, equeue notify event and stop thread
			EnqueueConnectFailEvent(ErrorType_TCPConnectFailure, comment, code);

			//GetVolatileLocalHostID() = HostID_None; // 서버와 연결이 해제됐으므로 초기화한다.
			m_worker->SetState(CNetClientWorker::Disconnecting);
		}
	}

#if defined(_WIN32)    
	void CNetClientImpl::IssueTcpFirstRecv()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		/*SocketErrorCode code = */m_remoteServer->m_ToServerTcp->IssueRecv(true);

		// 		if(code != SocketErrorCode_Ok)
		// 		{
		// 			EnqueueConnectFailEvent(ErrorType_TCPConnectFailure,code);
		// 			SetState(Disconnecting);
		// 		}
	}
#endif

	// 딱 한번만 실행되는 state다.
	void CNetClientImpl::Heartbeat_IssueConnect()
	{
		if ( m_enableLog || m_settings.m_emergencyLogLineCount > 0 )
		{
			Log(0, LogCategory_System, _PNT("Client NetWorker thread start"));
		}

		/* create TCP socket and TCP blocked connect to server socket

		winsock connect()는 TCP 연결 제한시간이 1초이다. 이를 API 딴에서 고칠 방법은 없다.
		따라서 TCP connect()를 수차례 반복후 안되면 즐쳐줘야 한다.
		통상적인 소켓은 1초지만 어떤 유저들은 이 값을 윈도 레지스트리를 통해 바꿔놓으므로 타임 기준으로 체크한다. */
		if ( SocketErrorCode_Ok != m_remoteServer->m_ToServerTcp->Bind() )
		{
			CriticalSectionLock mainLock(GetCriticalSection(), true);

			EnqueError(ErrorInfo::From(ErrorType_TCPConnectFailure, 
				HostID_None, _PNT("Cannot bind TCP socket to a local address!")));

			SocketErrorCode e;
#if !defined(_WIN32)
			e = (SocketErrorCode)errno;
#else
			e = (SocketErrorCode)WSAGetLastError();
#endif
			Heartbeat_ConnectFailCase(e, _PNT("Bind at IC"));

			return;
		}
		else
		{
			// bind가 성공했다. 일단 IP는 ANY이겠지만 port 번호값이라도 얻어두자.
			m_remoteServer->m_ToServerTcp->RefreshLocalAddr();
		}

		// 연결 시작 시각 기록
		m_worker->m_issueConnectStartTimeMs = GetPreciseCurrentTimeMs();

		//  소켓 기본 설정
		m_remoteServer->m_ToServerTcp->SetSocketVerboseFlag(false);

#if defined (_WIN32)
		// Main_IssueConnect에서 ConnectEx로 서버와 연결을 시도하기 때문에 IOCP에 해당 Socket을 등록합니다.
		// NOTE: 비 윈도에서는 issue connect 이후에 epoll add를 하고 있다. 아래 소스.
		m_netThreadPool->AssociateSocket(m_remoteServer->m_ToServerTcp->GetSocket());
#endif

		// 연결 시도를 건다. async or non-block으로.
		SocketErrorCode code;
		if ( Main_IssueConnect(&code) == false )
		{
			Heartbeat_ConnectFailCase(code, _PNT("IC: MI"));
			return;
		}

#if !defined (_WIN32)
		// NOTE: Socket 생성 후, (bind를 하더라도) Hang-up이 발생하여 Connect 호출 이후에 kqueue나 epoll에 Socket을 등록하는 것으로 변경하였습니다.
		// 비 윈도우 환경에서는 non-blocking Connect를 시도하기 때문에 Connect 이후에 kqeue나 epoll에 해당 Socket을 등록합니다.
		m_netThreadPool->AssociateSocket(m_remoteServer->m_ToServerTcp->GetSocket());
#endif

		// 연결 시도중 상태로.
		m_worker->SetState(CNetClientWorker::Connecting);
	}

	void CNetClientImpl::Heartbeat_Connecting()
	{
		int64_t currTimeMs = GetPreciseCurrentTimeMs();

		// 연결 시도(HostID를 받기까지의 메시징도 포함해서)중 상태가 너무 오래 지속되면 실패 처리
		if ( currTimeMs - m_worker->m_issueConnectStartTimeMs > CNetConfig::TcpSocketConnectTimeoutMs )
		{
			Heartbeat_ConnectFailCase(SocketErrorCode_Timeout, _PNT("HC: CFC"));
//		서버와 연결됐는데도 kqueue or epoll이 감지 못하는지 검사
//	auto e = m_remoteServer->m_ToServerTcp->m_fastSocket->Tcp_Send0ByteForConnectivityTest();
	//		printf("e=%d\n", e);//
		}
	}

	// 막 TCP 연결이 맺어진 직후.
	void CNetClientImpl::Heartbeat_JustConnected()
	{
		SetInitialTcpSocketParameters();

		m_worker->SetState(CNetClientWorker::Connected);

#if defined(_WIN32)
		// kqueue에서 처리되므로 따로 firstrecv issue를 걸필요가 없음.
		IssueTcpFirstRecv();
#endif
		GarbageTooOldRecyclableUdpSockets();
	}

	void CNetClientImpl::Heartbeat_Connected()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 성능 테스트를 하기 위해 이 함수만 추출했다.
		Heartbeat_Connected_AfterLock();
	}

	void CNetClientImpl::Heartbeat_Disconnecting()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		m_worker->m_DisconnectingModeHeartbeatCount++;

		DeleteUpnpTcpPortMappingOnNeed();

		// ACR을 막아버린다. 진행중인 것도 중단시킨다.
		// disconnecting mode가 되면 ACR이고 나발이고 없다.
		m_enableAutoConnectionRecovery = false;
		// RAII다!! 타 언어 포팅시 주의.
		m_autoConnectionRecoveryContext = CAutoConnectionRecoveryContextPtr();

		// send issue를 하고 있던 것을 모두 중지시킨다.
		//UnlinkSelf(); // Proud.CNetClientManager.m_sendIssuedInstanceQueue

		// 첫 1회인 경우 모든 소켓을 닫고 보유한 socket들을 garbage 처리한다.
		if ( m_worker->m_DisconnectingModeHeartbeatCount == 1 )
		{
			LockMain_AssertIsLockedByCurrentThread();

			// 첫 1회의 경우 ACR Remote Server 임시 객체들을 모두 제거 한다.
			AutoConnectionRecovery_GarbageEveryTempRemoteServerAndSocket();

			if ( m_enableLog || m_settings.m_emergencyLogLineCount > 0 )
				Log(0, LogCategory_Udp, _PNT("Heartbeat_Disconnecting, CloseSocketOnly called."));
		}

		// 반복적으로 호출하는 이유: 중간에 새로 들어오는 것들이 행여나 있으면 안되니까.
		GarbageAllHosts();

		AllClearRecycleToGarbage();

		//GarbageTooOldRecyclableUdpSockets();
		AssertIsLockedByCurrentThread();

		int unsafeDisconnectReason = 1;
		// async io가 모두 끝난게 확인될 떄까지 disconnecting mode 지속.
		// 어차피 async io는 이 스레드 하나에서만 이슈하기 떄문에 이 스레드 issue 없는 상태인 것을 확인하면 더 이상 async io가 진행중일 수 없다.

		// 모든 garbage socket,host가 collect된 상태이고, 시간도 꽤 지났고, 
		// 기타 종료 조건(DLL어쩌구 등)을 만족하면
		int64_t currtime = GetPreciseCurrentTimeMs();

		// 가비지, m_candidateHosts, m_authedHosts가 모두 비었거나
		if (CanDeleteNow())
		{
			/* 전부 garbage collected라 함은 어떠한 스레드도 this 및
			하부 객체를 더 이상 건드리지 않음을 의미.
			따라서 여기서 모든 데이터를 청소해도 된다.
			단, Disconnect()가 state is disconnected가 될 때까지 기다리는데서
			UnregisterReferrer도 해주어야 하므로
			UnregisterReferrer는 여기서는 안한다. */

			//assert(m_remotePeers.GetCount()==0);

			// FinalUserWorkItemList_RemoveReceivedMessagesOnly(); 이걸 하지 말자. 서버측에서 RMI직후추방시 클라에서는 수신RMI를 폴링해야하는데 그게 증발해버리니까. 대안을 찾자.
			// 이러면 disconnected 상황이 되어도 미수신 콜백을 유저가 폴링할 수 있다. 이래야 서버측에서 디스 직전 보낸 RMI를 클라가 처리할 수 있다.

			m_worker->SetState(CNetClientWorker::Disconnected);
		}
		else if ( !m_worker->m_DisconnectingModeWarned && currtime - m_worker->m_DisconnectingModeStartTime > 5 )
		{
#if defined(_WIN32)
			String txt;
			txt.Format(_PNT("Too long time elapsed since disconnecting mode! unsafeDisconnectReason=%d"), unsafeDisconnectReason);
			CErrorReporter_Indeed::Report(txt);
#endif
			m_worker->m_DisconnectingModeWarned = true;
		}
	}

	void CNetClientImpl::Heartbeat_Disconnected()
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		/* 아직 접속종료 완전히 안 끝났는데 사용자가 Connect를 호출했다면
		pended connect info를 넣었을 것이다. 그걸 비로소 여기서 쓴다. */
		if ( m_pendedConnectionParam != NULL )
		{
			Cleanup();
			Connect_Internal(*m_pendedConnectionParam);

			delete m_pendedConnectionParam;
			m_pendedConnectionParam = NULL;
		}
	}

	void CNetClientImpl::Heartbeat_Connected_AfterLock()
	{
		GarbageTooOldRecyclableUdpSockets();

		/* 예전에는 Heartbeat_ConnectedCase가 아래 while 문 안에 있었으나, 그게 아닌 듯 하다.
		아래는 더 이상 수신된 시그널이 없을 때까지 도는 루프니까. */
		Heartbeat_ConnectedCase();

		Heartbeat_DetectNatDeviceName();

		Heartbeat_AutoConnectionRecovery();
		//Heartbeat_AutoConnectionRecovery_ProcessMessageIDAck();

		int64_t currTime = GetPreciseCurrentTimeMs();
		int64_t shutdownIssuedTime = m_remoteServer->m_shutdownIssuedTime;
		// 서버와의 연결 해제 시작 상황이 된 후 서버와 오랫동안 연결이 안끊어지면 강제 스레드 종료를 한다.
		if ( shutdownIssuedTime > 0 && currTime - shutdownIssuedTime > m_worker->m_gracefulDisconnectTimeout )
		{
			m_worker->SetState(CNetClientWorker::Disconnecting);
			return;
		}
	}

	void CNetClientWorker::ProcessMessage_P2PUnreliablePing(CSuperSocket* socket, CReceivedMessage &ri)
	{
		int64_t clientLocalTime;
		if ( ri.GetReadOnlyMessage().Read(clientLocalTime) == false )
			return;

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		CRemotePeer_C *peer = m_owner->GetPeerByUdpAddr(ri.m_remoteAddr_onlyUdp, true);
		if ( peer == NULL || peer->m_HostID == HostID_Server )
			return;

		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_P2PUnreliablePong);
		sendMsg.Write(clientLocalTime);
		// 
		int64_t speed = 0;
		if ( socket )
			speed = socket->GetRecentReceiveSpeed(ri.GetRemoteAddr());

		int lossPercent;
		// server 와의 packet loss 율
		m_owner->GetUnreliableMessagingLossRatioPercent(HostID_Server, &lossPercent);
		sendMsg.Write(lossPercent);

		// peer 와의 packet loss 율
		m_owner->GetUnreliableMessagingLossRatioPercent(ri.GetRemoteHostID(), &lossPercent);
		sendMsg.Write(lossPercent);
		sendMsg.WriteScalar(speed);

		CSendFragRefs sd(sendMsg);
		peer->m_ToPeerUdp.SendWithSplitter_Copy(sd, SendOpt(MessagePriority_Ring0, true));
	}

	void CNetClientWorker::ProcessMessage_P2PUnreliablePong(CReceivedMessage &ri)
	{
		int64_t clientOldLocalTime;
		int64_t repliedReceivedSpeed;
		int CSLossPercent, C2CLossPercent = 0;

		if ( ri.GetReadOnlyMessage().Read(clientOldLocalTime) == false )
			return;

		if ( !ri.GetReadOnlyMessage().Read(CSLossPercent) )
			return;

		if ( !ri.GetReadOnlyMessage().Read(C2CLossPercent) )
			return;

		if ( !ri.GetReadOnlyMessage().ReadScalar(repliedReceivedSpeed) )
			return;

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		CRemotePeer_C *peer = m_owner->GetPeerByUdpAddr(ri.m_remoteAddr_onlyUdp, true);
		if ( peer == NULL || peer->m_HostID == HostID_Server )
			return;

		// modify by ulelio : 피어와의 핑만 계산한다.
		int64_t currTime = GetPreciseCurrentTimeMs();
		int newLag = int((currTime - clientOldLocalTime) / 2);

		peer->m_lastPingMs = newLag;
		peer->m_lastPingMs = max(peer->m_lastPingMs, 1);

		if ( peer->m_p2pPingChecked == false )
			peer->m_p2pPingChecked = true;

		if ( peer->m_recentPingMs > 0 )
			peer->m_recentPingMs = LerpInt(peer->m_recentPingMs, newLag, CNetConfig::LagLinearProgrammingFactorPercent, 100);
		else
			peer->m_recentPingMs = newLag;

		peer->m_recentPingMs = max(peer->m_recentPingMs, 1);

		peer->m_CSPacketLossPercent = CSLossPercent;
		//peer->m_unreliableRecentReceivedSpeed = repliedReceivedSpeed;

		// 		CUdpPacketFragBoard* fboard = peer->Get_ToPeerUdpSocket()->m_udpPacketFragBoard_needSendLock;
		// 		if (fboard)
		// 			fboard->SetReceiveSpeedAtReceiverSide(peer->m_P2PHolepunchedLocalToRemoteAddr, repliedReceivedSpeed, C2CLossPercent, GetPreciseCurrentTimeMs());

		peer->m_udpSocket->SetReceiveSpeedAtReceiverSide(peer->m_P2PHolepunchedLocalToRemoteAddr, repliedReceivedSpeed, C2CLossPercent, GetPreciseCurrentTimeMs());

		peer->m_lastDirectUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs();//이거 timer거 쓰면 안된다.절대!!! 
		peer->m_directUdpPacketReceiveCount++;

	}

	void CNetClientWorker::ProcessMessage_UnreliablePong(CMessage &msg)
	{
		int64_t clientOldLocalTime, serverLocalTime;
		int64_t speed = 0;
		int packetLossPercent = 0;

		if ( !msg.Read(clientOldLocalTime) )
			return;

		if ( !msg.Read(serverLocalTime) )
			return;

		if ( !msg.ReadScalar(speed) )
			return;
		if ( !msg.Read(packetLossPercent) )
			return;

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		// 새로 server lag을 계산한 후 클라이언트와의 시간차도 구한다.
		int64_t clientTime = GetPreciseCurrentTimeMs();
		int newLag = (int)((clientTime - clientOldLocalTime) / 2);
		m_owner->m_serverUdpLastPingMs = max(newLag, 1);

		m_owner->m_serverUdpPingChecked = true;

		if ( m_owner->m_serverUdpRecentPingMs == 0 )
			m_owner->m_serverUdpRecentPingMs = m_owner->m_serverUdpLastPingMs;
		else
			m_owner->m_serverUdpRecentPingMs = LerpInt(m_owner->m_serverUdpRecentPingMs, m_owner->m_serverUdpLastPingMs, (int)CNetConfig::LagLinearProgrammingFactorPercent, 100);


		int64_t serverTime = serverLocalTime + m_owner->m_serverUdpRecentPingMs;
		m_owner->m_serverTimeDiff = clientTime - serverTime;

		if ( m_owner->m_remoteServer->m_ToServerUdp != NULL )
			m_owner->m_remoteServer->m_ToServerUdp->SetReceiveSpeedAtReceiverSide(m_owner->m_remoteServer->GetServerUdpAddr(), speed, packetLossPercent, GetPreciseCurrentTimeMs());

		//m_owner->LogLastServerUdpPacketReceived(); 이미 UDP 패킷 수신 처리에서 했는데?

		//Console.WriteLine(String.Format("{0}: UnreliablePong!#2", m_owner->GetVolatileLocalHostID()));

		LocalEvent e;
		e.m_type = LocalEventType_SynchronizeServerTime;
		e.m_remoteHostID = HostID_Server;
		m_owner->EnqueLocalEvent(e, m_owner->m_remoteServer);

		//	NTTNTRACE("************************\n");
	}

	void CNetClientWorker::SetState(State newVal)
	{
		if ( newVal > /*>=*/ m_state_USE_FUNC ) // 뒤 상태로는 이동할 수 없어야 한다.
		{
			m_DisconnectingModeHeartbeatCount = 0; // 이것도 세팅해야!
			m_state_USE_FUNC = newVal;
			m_DisconnectingModeStartTime = GetPreciseCurrentTimeMs();
			m_DisconnectingModeWarned = false;
		}
	}

	void CNetClientWorker::ProcessMessage_Rmi(CReceivedMessage &receivedInfo, bool &refMessageProcessed)
	{
		receivedInfo.GetReadOnlyMessage().SetSimplePacketMode(m_owner->IsSimplePacketMode());
		CMessage& payload = receivedInfo.GetReadOnlyMessage();

		// 이 값은 MessageType_Rmi 다음의 offset이다.
		int orgReadOffset2 = payload.GetReadOffset();

		// hostTag 처리
		void* hostTag = m_owner->GetHostTag(receivedInfo.m_remoteHostID);

		// ProudNet layer의 RMI이면 아래 구문에서 true가 리턴되고 결국 user thread로 넘어가지 않는다.
		// 따라서 아래처럼 하면 된다.
		refMessageProcessed |= m_owner->m_s2cStub.ProcessReceivedMessage(receivedInfo, hostTag);
		if ( refMessageProcessed == false )
		{
			payload.SetReadOffset(orgReadOffset2);
			refMessageProcessed |= m_owner->m_c2cStub.ProcessReceivedMessage(receivedInfo, hostTag);
		}
		if ( refMessageProcessed == false )
		{
			// 유저 스레드에서 RMI를 처리하도록 enque한다.
			payload.SetReadOffset(orgReadOffset2);

			CReceivedMessage receivedInfo2;
			//receivedInfo2.m_actionTime = receivedInfo.m_actionTime;
			receivedInfo2.m_unsafeMessage.UseInternalBuffer();
			receivedInfo2.m_unsafeMessage.AppendByteArray(
				payload.GetData() + payload.GetReadOffset(),
				payload.GetLength() - payload.GetReadOffset());
			receivedInfo2.m_relayed = receivedInfo.m_relayed;
			receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
			receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
			receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
			receivedInfo2.m_compressMode = receivedInfo.m_compressMode;

			/* 주의: 이 메서드의 입력 변수로서의 데이터 버퍼는 스택 할당 데이터이어서는 안된다! */
			if ( receivedInfo2.m_unsafeMessage.m_msgBuffer.IsNull() )
				receivedInfo2.m_unsafeMessage.m_msgBuffer.UseInternalBuffer();

			m_owner->LockMain_AssertIsLockedByCurrentThread();

			if ( receivedInfo2.m_remoteHostID == HostID_Server )
			{
				m_owner->UserTaskQueue_Add(m_owner->m_remoteServer, receivedInfo2, UWI_RMI);
			}
			else if ( receivedInfo2.m_remoteHostID == m_owner->GetVolatileLocalHostID() )
			{
				m_owner->UserTaskQueue_Add(m_owner->m_loopbackHost, receivedInfo2, UWI_RMI);
			}
			else
			{
				// Non-PN RMI가 도착하면 P2P 홀펀칭JIT 조건이 된다.
				CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

				CRemotePeer_C* peer = m_owner->GetPeerByHostID_NOLOCK(receivedInfo2.m_remoteHostID);
				if ( peer != NULL && !peer->m_garbaged )
				{
					// main lock을 걸었지만 정작 peer가 이미 떠난 상태일 수 있는데, 이때는 그냥 쌓지말고 버린다.
					m_owner->UserTaskQueue_Add(peer, receivedInfo2, UWI_RMI);

					if (!peer->m_forceRelayP2P)
						peer->m_jitDirectP2PNeeded = true;

					//  2010.08.24 add by ulelio : 외부 rmi real p2p 인경우 해당 peer에게 받은 udp packet count를 센다.
					if ( !receivedInfo2.m_relayed )
						peer->m_receiveudpMessageSuccessCount++;
				}
			}
		}
	}

	void CNetClientImpl::Heartbeat_DetectNatDeviceName()
	{
		if ( !m_natDeviceNameDetected && HasServerConnection() && GetVolatileLocalHostID() != HostID_None )
		{
			String name = GetNatDeviceName();
			if ( name.GetLength() > 0 )
			{
				m_natDeviceNameDetected = true;
				m_c2sProxy.NotifyNatDeviceNameDetected(HostID_Server, g_ReliableSendForPN, name);
			}
		}
	}

	void CNetClientWorker::ProcessMessage_UserOrHlaMessage(CReceivedMessage &receivedInfo, FinalUserWorkItemType UWIType, bool &refMessageProcessed)
	{
		CMessage& payload = receivedInfo.GetReadOnlyMessage();

		// 유저 스레드에서 user or HLA message 를 처리하도록 enque한다.
		CReceivedMessage receivedInfo2;
		//receivedInfo2.m_actionTime = receivedInfo.m_actionTime;
		receivedInfo2.m_unsafeMessage.UseInternalBuffer();
		receivedInfo2.m_unsafeMessage.AppendByteArray(payload.GetData() + payload.GetReadOffset(), payload.GetLength() - payload.GetReadOffset());
		receivedInfo2.m_relayed = receivedInfo.m_relayed;
		receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
		receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
		receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
		receivedInfo2.m_compressMode = receivedInfo.m_compressMode;

		/* 주의: 이 메서드의 입력 변수로서의 데이터 버퍼는 스택 할당 데이터이어서는 안된다! */
		receivedInfo2.m_unsafeMessage.m_msgBuffer.MustInternalBuffer();

		m_owner->LockMain_AssertIsLockedByCurrentThread();

		if ( receivedInfo2.m_remoteHostID == HostID_Server )
		{
			m_owner->UserTaskQueue_Add(m_owner->m_remoteServer, receivedInfo2, UWIType);
		}
		else if ( receivedInfo2.m_remoteHostID == m_owner->GetLocalHostID() )
		{
			m_owner->UserTaskQueue_Add(m_owner->m_loopbackHost, receivedInfo2, UWIType);
		}
		else
		{
			// Non-PN RMI가 도착하면 P2P 홀펀칭JIT 조건이 된다.
			CriticalSectionLock lock(m_owner->GetCriticalSection(), true);
			CRemotePeer_C* peer = m_owner->GetPeerByHostID_NOLOCK(receivedInfo2.m_remoteHostID);

			if ( peer != NULL && !peer->m_garbaged )
			{
				// user message를 처리하는 것이면 peer는 무조건 있어야 한다.
				// 그러나, 클라가 종료중인 경우 peer가 garbaged이거나 
				// 이미 garbage collected일 것이다.
				// 이러한 경우 그냥 무시하는 것이 상책.
				m_owner->UserTaskQueue_Add(peer, receivedInfo2, UWIType);

				// 클라로부터 P2P 메시지가 온 것이므로
				// JIT P2P 홀펀칭 발동 시작해야.
				if ( !peer->m_forceRelayP2P )
					peer->m_jitDirectP2PNeeded = true;

				//  2010.08.24 add by ulelio : real udp인 경우 count를 한다.
				if ( !receivedInfo2.m_relayed )
					peer->m_receiveudpMessageSuccessCount++;
			}
		}
	}

	void CNetClientWorker::WarnTooLongElapsedTime()
	{
		if ( CNetConfig::EnableStarvationWarning )
		{
			int64_t recentAverageTime = m_owner->m_recentElapsedTime;

			// 10초나 hearbeat 이 안 돌면 개막장이지...유저 RMI는 별도 스레드에서 도는데 말이다.
			if ( recentAverageTime > 10000 && !m_owner->m_hasWarnedStarvation )
			{
				m_owner->m_hasWarnedStarvation = true;

				String str;
				str.Format(_PNT("Too long elapsed time in NetClient hearbeat thread! (%3.31f sec)"), double(recentAverageTime) / 1000);

				CriticalSectionLock lock(m_owner->GetCriticalSection(), true);
				if ( m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0 )
					m_owner->Log(0, LogCategory_System, str);

				m_owner->EnqueWarning(ErrorInfo::From(ErrorType_TooSlowHeartbeatWarning, m_owner->GetLocalHostID(), str));
			}
		}
	}

	void CNetClientWorker::ProcessMessage_P2PReliablePing(CReceivedMessage &ri)
	{
		int64_t clientLocalTime;
		if ( ri.GetReadOnlyMessage().Read(clientLocalTime) == false )
			return;

		double recentFrameRate;
		int recentPing;
		if ( ri.GetReadOnlyMessage().Read(recentFrameRate) == false )
			return;
		if ( ri.GetReadOnlyMessage().Read(recentPing) == false )
			return;

		{
			CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

			CRemotePeer_C* rp = m_owner->GetPeerByHostID_NOLOCK(ri.GetRemoteHostID());
			if ( rp != NULL && !rp->m_garbaged )
			{
				rp->m_recentFrameRate = recentFrameRate;

				/*m_peerToServerPingMs가 ReportServerTimeAndFrameRateAndPong이라는
				P2P RMI에서도 갱신됩니다. 둘 중 하나가 사라져야 하지 않나요? */
				// PnTest 빌드때문에 잠시 살려둔 것입니다. 타언어 작업 시 사라질 예정입니다.
				rp->m_peerToServerPingMs = recentPing;
			}
		}

		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_P2PReliablePong);
		sendMsg.Write(clientLocalTime);
		// 
		CApplicationHint hint;
		m_owner->GetApplicationHint(hint);
		sendMsg.Write(hint.m_recentFrameRate);
		sendMsg.Write(m_owner->GetServerTimeMs());

		CSendFragRefs sd(sendMsg);

		// ping 패킷이므로 Ring0로 보낸다.
		SendOpt opt(g_ReliableSendForPN);
		opt.m_priority = MessagePriority_Ring0;

		m_owner->Send_BroadcastLayer(sd, NULL, opt, &ri.m_remoteHostID, 1);
	}

	void CNetClientWorker::ProcessMessage_P2PReliablePong(CReceivedMessage &ri)
	{
		int64_t clientOldLocalTime, serverTime;
		double recentFrameRate;

		if ( !ri.GetReadOnlyMessage().Read(clientOldLocalTime) )
			return;

		if ( !ri.GetReadOnlyMessage().Read(recentFrameRate) )
			return;

		if ( !ri.GetReadOnlyMessage().Read(serverTime) )
			return;

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		CRemotePeer_C *peer = m_owner->GetPeerByHostID_NOLOCK(ri.m_remoteHostID);
		if ( peer == NULL || peer->m_HostID == HostID_Server )
			return;

		int64_t currTime = GetPreciseCurrentTimeMs();
		int newLag = (int)((currTime - clientOldLocalTime) / 2);

		peer->m_lastReliablePingMs = max(newLag, 1);

		if ( peer->m_p2pReliablePingChecked == false )
		{
			peer->m_p2pReliablePingChecked = true;
		}

		if ( peer->m_recentReliablePingMs > 0 )
		{
			peer->m_recentReliablePingMs = LerpInt(
				peer->m_recentReliablePingMs,
				peer->m_lastReliablePingMs,
				CNetConfig::LagLinearProgrammingFactorPercent, 100);
		}
		else
		{
			peer->m_recentReliablePingMs = peer->m_lastReliablePingMs;
		}

		peer->m_recentFrameRate = recentFrameRate;
		peer->m_indirectServerTimeDiff = currTime - (serverTime + peer->m_recentPingMs);

		// reliableudp도 어차피 udp를 사용하므로 해당값을 갱신
		peer->m_lastDirectUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs();
		peer->m_directUdpPacketReceiveCount++;
	}

	// To-server TCP socket이 연결이 성공했을 때 호출된다.
	// ACR 재접속 후보는 여기서 처리 안함.
	void CNetClientImpl::ProcessFirstToServerTcpConnectOk()
	{
		// 혹시 모르니, 이 lock을 함수 맨 위로 올립시다.
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 이제 로컬 IP주소도 얻자. 127.0.0.1 아니면 연결된 NIC의 주소가 나올 것이다.
		m_remoteServer->m_ToServerTcp->RefreshLocalAddr();
		if ( !m_remoteServer->m_ToServerTcp->GetLocalAddr().IsUnicastEndpoint() )
		{
			ErrorInfoPtr err = ErrorInfo::From(
				ErrorType_UnknownAddrPort,
				HostID_Server,
				_PNT("Unexpected: TCP-connected socket has no local address!"));
			EnqueError(err);
		}

		ValidSendReayList_Add(m_remoteServer->m_ToServerTcp);

		// 마무리. Heartbeat_JustConnected 참고.
		m_worker->SetState(CNetClientWorker::JustConnected);
	}

	void CNetClientWorker::ProcessReadPacketFailed(void)
	{
		m_owner->EnqueueConnectFailEvent(ErrorType_InvalidPacketFormat,
										 ErrorInfo::From(
										 ErrorType_ProtocolVersionMismatch,
										 HostID_Server,
										 _PNT("Bad format in NotifyServerConnectSuccess")));
		SetState(Disconnecting);
		return;
	}

}
