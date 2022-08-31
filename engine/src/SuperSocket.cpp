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

#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/ReceivedMessage.h"
#include "SendFragRefs.h"

#include "FastSocket.h"
#include "IntraTracer.h"
#include "NetCore.h"
#include "ReportError.h"
#include "Epoll_Reactor.h"
#ifdef SUPERSOCKET_DETECT_HANG_ON_GARBAGE
#include "../UtilSrc/DebugUtil/DebugCommon.cpp"
#endif

#ifdef GET_SEND_SYSCALL_TOTAL_COUNT
#include "../UtilSrc/DebugUtil/DebugCommon.h"
#include "../UtilSrc/DebugUtil/DebugCommon.cpp"
#endif

#include "../include/SocketEnum.h"

#include "MessagePrivateImpl.h"

using namespace std;

namespace Proud
{
	// super socket들은 생성될 때마다 unique한 값이 하나씩 증가하면서 할당된다. socket to remote map에서 ABA problem을 막기 위해서임.
	intptr_t g_superSocketLastSerial = 0;

	CSuperSocket::CSuperSocket(CNetCoreImpl *owner, SocketType socketType/*, CThreadPoolImpl* ownerThreadPool, AddrPort remoteAddr*/)
		: m_tcpOnlyLastSentTime(0)
		, m_totalIssueSendCount(0)
		, m_totalIssueReceiveCount(0)
		, m_firstIssueReceiveTime(0)
		, m_firstReceiveEventTime(0)
		, m_totalSendEventCount(0)
		, m_totalReceiveEventCount(0)
		, m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList(NULL)
		, m_ACCESSED_ONLY_BY_SendReadySockets_Owner(NULL)
		, m_warnTooLongGarbage(false)
	{
		static_assert(sizeof(IoState) == sizeof(int32_t), "FATAL! IoState size is incompatible with this compiler.");
		m_turnOffSendAndReceive = false;
		m_forceImmediateSend = false;

#if defined(_WIN32)
		m_sendIssued = IoState_NotWorking;
		m_recvIssued = IoState_NotWorking;
#else
		//m_ioState = IoState_NotWorking;
#endif
		m_requestStopIoTime = 0;

		m_acceptCandidateSocket.reset();
		m_acceptedSocket = shared_ptr<CSuperSocket>();
		m_stopIoRequested_USE_FUNCTION = IoState_False;
		m_associatedIoQueue_accessByAssociatedSocketsOnly = NULL;
		m_totalIssueSendBytes = 0;
		m_lastReceivedTime = GetPreciseCurrentTimeMs();
		m_socketType = socketType;
		m_timeToGarbage = 0;

#ifndef _WIN32
		m_isLevelTrigger = false;
#endif

		m_owner = owner;

		//		m_ownerThreadPool = ownerThreadPool;
		m_serialNumber = AtomicIncrementPtr(&g_superSocketLastSerial);
		m_localAddr_USE_FUNCTION = m_localAddrAtServer = AddrPort::Unassigned;

		//m_lastRecvissueWarningTime = 0;
		//m_lastSendIssueWarningTime = 0;

		m_userCalledDisconnectFunction = false;
		m_isListeningSocket = false;
		m_isConnectingSocket = false;

		//owner->LockMain_AssertIsLockedByCurrentThread();

		if ( socketType == SocketType_Tcp )
		{
			m_recvStream.Attach(new CStreamQueue(CNetConfig::StreamGrowBy));
			m_sendQueue_needSendLock.Attach(new CTcpSendQueue);
		}
		else
		{
			m_udpPacketFragBoard_needSendLock.Attach(new CUdpPacketFragBoard(this));
			m_sendIssuedFragment.Attach(new CUdpPacketFragBoardOutput);
			m_udpPacketFragBoard_needSendLock->InitHashTableForClient();
			//assert(m_owner->m_unsafeHeap != NULL);
			m_udpPacketDefragBoard.Attach(new CUdpPacketDefragBoard(this));

#ifdef CHECK_BUF_SIZE
			int sendBufLength, recvBufLength;
			m_fastSocket->GetSendBufferSize(&sendBufLength);
			m_fastSocket->GetRecvBufferSize(&recvBufLength);
#endif

#ifdef USE_DisableUdpConnResetReport
			m_fastSocket->DisableUdpConnResetReport();
#endif // USE_DisableUdpConnResetReport
		}
		m_packetTruncatePercent = 0;
		//owner->AssociateSocket(m_socket); -- 호출 금지! TCP connect 과정은 Heartbeat_Connecting에서 epoll or iocp를 쓰지 않고 nonblock poll을 하기 때문이다.

		// socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
		// recv buffer 제거는 백해무익이므로 즐
#ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
		m_fastSocket->SetSendBufferSize(0);
#endif // ALLOW_ZERO_COPY_SEND


		// recv buffer를 크게 잡을수록 OS 부하가 커진다. 특별한 경우가 아니면 이것은 건드리지 않는 것이 좋다.
		//m_socket->SetRecvBufferSize(CNetConfig::ClientTcpRecvBufferLength);

		m_dropSendAndReceive = false;
	}

	SuperSocketCreateResult CSuperSocket::New(CNetCoreImpl* owner, const shared_ptr<CFastSocket>& fastSocket, SocketType socketType)
	{
		shared_ptr<CSuperSocket> superSocket(new CSuperSocket(owner, socketType));

		superSocket->m_fastSocket = fastSocket;

#ifndef _WIN32 // iocp를 쓰지 않는 모든 플랫폼
		superSocket->m_fastSocket->SetBlockingMode(false);
#endif

#if !defined(SOL_SOCKET)
#	error You must include socket header file for correct compilation of the functions below.
#endif

#if defined(SO_NOSIGPIPE)
		// 소켓의 SIG_PIPE 에러가 안나도록 해준다.
		// OS 막론하고 SIGNAL이 발생하면 안된다.
		superSocket->m_fastSocket->SetNoSigPipe(true);
#endif

		if (socketType == SocketType_Tcp)
		{
#ifdef CHECK_BUF_SIZE
			int sendBufLength, recvBufLength;
			superSocket->m_fastSocket->GetSendBufferSize(&sendBufLength);
			superSocket->m_fastSocket->GetRecvBufferSize(&recvBufLength);
#endif // CHECK_BUF_SIZE
			// 아직은 SetTcpDefaultBehavior_Client와 동일하게 사용되고 있음.
			SetTcpDefaultBehavior_Server(fastSocket);
		}
		else
		{
#ifdef USE_DisableUdpConnResetReport
			superSocket->m_fastSocket->DisableUdpConnResetReport();
#endif // USE_DisableUdpConnResetReport

			// socket이 재사용 되지 못하게 명시적으로 막는다.
			// 이것을 안했을때, 홀펀칭 정보 재사용시 DotNet에서 gabage에 있는 소켓과 새로 bind하는 소켓이 port가 겹치는 문제있었음.

			// NOTE: Naddic Games 포트 바인드 실패 문제 해결과 관련하여 아래의 내용은 주석처리하였습니다.
			//superSocket->m_socket->SetSocketReuseAddress(false);

#ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
			m_fastSocket->SetSendBufferSize(0);
#endif // ALLOW_ZERO_COPY_SEND

			// NetClient의 UdpSocket에 한해서 처리한다.
			// NetServer는 별도로 UdpAssignMode에 따라서 별도로 직접 처리해주어야 한다!!!
			if (owner->GetLocalHostID() != HostID_Server)
				SetUdpDefaultBehavior_Client(fastSocket);
		}

		SuperSocketCreateResult ret;
		ret.socket = superSocket;
		return ret;
	}

	SuperSocketCreateResult CSuperSocket::New(CNetCoreImpl* owner, SocketType socketType)
	{
		SocketCreateResult r0 = CFastSocket::Create(socketType);
		if (!r0.socket)
		{
			SuperSocketCreateResult r;
			r.errorText = r0.errorText;
			return r;
		}
		else
		{
			return New(owner, r0.socket, socketType);
		}
	}

	//	// 이미 닫혀버린 socket을 다시 연다. => 제거. RestoreSocket 대신 CreateSocket(TcpSocketAddr)로 해결가능할 것이므로.
	//	// UDP 전용.
	//	bool CSuperSocket::RestoreSocket(AddrPort tcpLocalAddr/*m_main->Get_ToServerTcp()->m_localAddr가 여기 들어가야!*/)
	//	{
	//		//assert(m_ownerPeer != NULL);
	//		if (m_socketType != SocketType_Udp)
	//		{
	//			assert(0);
	//			throw Exception("Wrong CSuperSocket::RestoreSocket call!");
	//		}
	//
	//		m_socket->Restore(false);
	//		StopIoAckedToReset();
	//
	// #ifdef USE_DisableUdpConnResetReport
	//		m_socket->DisableUdpConnResetReport();
	// #endif // USE_DisableUdpConnResetReport
	//
	//		// 서버와 연결되었던 TCP 소켓의 로컬 주소를 얻어서 그걸로 바인딩한다. 안그러면 로컬 NIC가 둘 이상이며 하나가 사내 네트워크 전용 따위인 경우 통신 불량이 생기기도 함.
	//		AddrPort udpLocalAddr = tcpLocalAddr;
	//		if (udpLocalAddr.m_binaryAddress == 0 || udpLocalAddr.m_binaryAddress == 0xffffffff)
	//		{
	// #if defined(_WIN32)
	//			String text;
	//			text.Format(_PNT("FATAL: RestoreSocket - It should already have been TCP-Connected prior to the Creation of the UDP Socket but %s Appeared!"), udpLocalAddr.ToString());
	//			CErrorReporter_Indeed::Report(text);
	// #endif
	//			m_socket->CloseSocketOnly();
	//			m_stopIoRequested = true;
	//			return false;
	//		}
	//		udpLocalAddr.m_port = 0;
	//
	// #ifndef _WIN32 // ios,...
	//		m_socket->SetBlockingMode(false);
	// #endif // _WIN32
	//
	//		if (m_socket->Bind(udpLocalAddr) != true)
	//		{
	// #if defined(_WIN32)
	//			String text;
	//			text.Format(_PNT("RestoreSocket - Failed to Bind the UDP Local Address to %s."), udpLocalAddr.ToString());
	//			CErrorReporter_Indeed::Report(text);
	// #endif
	//			m_socket->CloseSocketOnly();
	//			m_stopIoRequested = true;
	//			return false;
	//		}
	//
	//		if (RefreshLocalAddr() != true)
	//		{
	// #if defined(_WIN32)
	//			String text;
	//			text.Format(_PNT("The Result of the getsockname after Binding the UDP Local Address to %s wackily appears to be %s."), udpLocalAddr.ToString(), GetLocalAddr().ToString());
	//			CErrorReporter_Indeed::Report(text);
	// #endif
	//
	//			m_socket->CloseSocketOnly();
	//			m_stopIoRequested = true;
	//			return false;
	//		}
	//
	//		// IOCP혹은 KqueueContext에 역는다.
	//		m_socket->SetCompletionContext(this);
	//
	// #if defined(_WIN32)
	//		m_main->LockMain_AssertIsLockedByCurrentThread();
	// #endif
	//
	// #if defined(_WIN32)
	//		m_main->m_manager->m_completionPort->AssociateSocket(m_socket);
	// #else
	//		m_main->m_manager->m_reactor->AssociateSocket(m_socket);
	// #endif
	//
	//		// socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
	//		// send buffer 없애기는 coalsecse, throttling을 위해 필수다.
	//		// recv buffer 제거는 백해무익이므로 즐
	// #ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
	//		m_socket->SetSendBufferSize(0);
	// #endif // ALLOW_ZERO_COPY_SEND
	//		SetUdpDefaultBehavior_Client(m_socket);
	//
	//		// recv buffer를 크게 잡을수록 OS 부하가 커진다. 특별한 경우가 아니면 이것은 건드리지 않는 것이 좋다.
	//		//m_socket->SetRecvBufferSize(CNetConfig::ClientUdpRecvBufferLength);
	//
	//		return true;
	//	}

	/* 더 이상 i/o 가 걸릴 수 없는 상태로 만들기를 시작한다.
	이 함수 리턴 직후 this는 delete is safe는 아직 아니다.
	close socket은 안한다.
	closed socket handle값이 즉시 재사용되는 사고를 막기 위해서 close socket은 최후의 스레드가 하는 것이 원칙이다.

	iocp 환경에서: i/o state가 모두 2가 되어야 delete is safe가 된다.
	epoll 환경에서: this를 처리하는 networker thread에서 직접 close and delete한다.

	리턴값 : 최초로 close를 시도하는 경우 true 반환.

	주의: 이 함수가 잘 작동하려면 SuperSocket에 대한 overlapped i/o 시도하는 모든 루틴에서
	사전에 m_stopIoRequested를 검사해야 한다.

	Q: 왜 close socket을 직접 하지 않고 이렇게 만들어서, DoGarbageCollect에서야 비로소 close socket을 하나요?
	A: 과거에는 그렇게 구현되어 있었습니다. 그러나 리눅스에서, 막 닫은 socket fd값과 동일한 socket fd값이
	재사용되는 경우가 있습니다. 이를 해결하려면 더 이상 어떤 스레드도 socket fd를 액세스하지 않는다는게
	보장된 후에 close를 해야 합니다. 이거 때문에 case 37 fail 등 여러 문제를 겪었습니다. */
	bool CSuperSocket::RequestStopIo()
	{
		bool firstStop = AtomicCompareAndSwapEnum<IoState,int32_t>(IoState_False, IoState_True, &m_stopIoRequested_USE_FUNCTION) == IoState_False;

		if (firstStop)
		{
#if defined(_WIN32)
			// udp socket 닫은 내역을 기록한다.
			if ( m_socketType == SocketType_Udp && m_owner->m_settings.m_emergencyLogLineCount > 0 )
			{
				String text = _PNT("Called RequestStopIo().");
				m_owner->AddEmergencyLogList(1, LogCategory_Udp, text);
			}
#endif

			/* win32에서는 이 함수는 socket close도 합니다.
			win32에서는 이 함수는 socket close도 하기 때문에, overlapped io가 이미 진행중이었다면,
			completion이 반드시 오게 됩니다.
			따라서 아래 Working=>NotWorking 변화가 없더라도 괜찮습니다. */
			if (m_fastSocket)
			{
#ifdef _DEBUG
				assert(!m_fastSocket->IsClosedOrClosing());
#endif
				m_fastSocket->RequestStopIo_CloseOnWin32();
#ifdef _WIN32
				assert(m_fastSocket->IsClosedOrClosing());
#endif
			}

			// iocp or epoll과의 연결을 해제한다.
			if (m_associatedIoQueue_accessByAssociatedSocketsOnly != NULL)
			{
				//CriticalSectionLock lock(m_associatedIoQueue->m_cs, true);
				//m_associatedSockets.RemoveAt(m_ioQueueIter);

				// Linux에서 해당 Socket을 가지고 있는 IoNotifier가 SetDesiredThreadCount에 의해 다른 스레드로 Socket을 복사하는 중이라면
				// RemoveSocket이 정상적으로 처리되지 않을 것으로 보입니다.
				/* => RemoveSocket은 내부적으로 Proud.CThreadPoolImpl.m_cs를 lock합니다.
				그리고 SetDesiredThreadCount도 같은 lock을 하고 있고, 그 안에서 모자란 스레드를 생성하고 초과하는 스레드를 중단하라는 명령과 함께 소켓 이양 처리를 모두 완료한 후 리턴하는 것으로 압니다.
				그렇다면 아래 함수가 실행되는 동안에는 죽어야 할 스레드에서 찾는 헛짓거리를 할 가능성은 없다고 생각합니다.

				그리고 오늘 회의에서, SetDesiredThreadCount가 리턴하기 전에 죽을 스레드로부터 소켓들을 모두 이양하지 않는다고 했는데, 그러면 안됩니다.
				level trigger로 옮겨지게 한 이유는 이양한 후에 그것들의 이벤트가 언젠가는 반드시 발생하게 하려고입니다.
				이 때문에 SetDesiredThreadCount에서 리턴하게 전에 죽을 스레드로부터 소켓들을 모두 이양하게 바꾸세요. 그러면 위의 문제 해결이 성립됩니다.

				*/
				m_associatedIoQueue_accessByAssociatedSocketsOnly->RemoveSocket(shared_from_this());

				m_associatedIoQueue_accessByAssociatedSocketsOnly = NULL;
			}

			m_requestStopIoTime = GetPreciseCurrentTimeMs();
		}

		return firstStop;
	}

	CSuperSocket::~CSuperSocket()
	{
#if defined(_WIN32)
		//m_owner->LockMain_AssertIsLockedByCurrentThread(); garbaged인 경우 m_owner가 무의미하므로

		// i/o completion이 아직 안 끝났는데 여기 오면, OS 커널이 메모리를 그어버린다!
		assert(m_sendIssued != IoState_Working);
		assert(m_recvIssued != IoState_Working);
#endif
		// 행여나 실수로 아직 iocp or epoll로부터의 remove socket을 안했다면,
		// 여기서 정리하게 요청 보내기.
		if (m_associatedIoQueue_accessByAssociatedSocketsOnly)
		{
			bool removed = m_associatedIoQueue_accessByAssociatedSocketsOnly->RemoveSocket(SocketPtrAndSerial(this));
			_pn_unused(removed);
			assert(removed); // 등록되지 않았는데 여기 올 수 있나?
		}

		m_udpPacketDefragBoard.Free();
		m_sendIssuedFragment.Free();
		m_udpPacketFragBoard_needSendLock.Free();
		m_sendQueue_needSendLock.Free();
		m_recvStream.Free();

		// 이미 recvIssued=0 or 2. 즉 acceptex overlapped 상태가 아님.
		// 따라서 안전하게 파괴해도 된다.
		m_acceptCandidateSocket = shared_ptr<CFastSocket>();
		m_acceptedSocket = shared_ptr<CSuperSocket>();
	}

	void CSuperSocket::DoForLongInterval(int64_t currTime, uint32_t &queueLength)
	{
		/*아래 루틴은 send queue lock만을 요구합니다. 그리고 m_socketType은 객체 존속 시간 동안 바뀌는 일이 없습니다.
			따라서 m_cs를 lock할 필요는 없이 그냥 send queue lock만 해도 됩니다.
			m_cs lock은 syscall을 겸하기 때문에 매우 느립니다. 따라서 빼는 것이 성능상 유리합니다.
			수정 및 본 주석을 남겨 주시기 바랍니다. */

		/*
		* 결과로 Queue에 쌓여 있는 종 량을 리턴합니다.
		*/
		CriticalSectionLock sendLock(m_sendQueueCS, true);

		if ( m_socketType == SocketType_Tcp )
		{
			m_sendQueue_needSendLock->DoForLongInterval();

			int len = GetTcpSendQueueLength();

			if (len < 0)
				queueLength = 0;
			else
				queueLength = (uint32_t)len;
		}
		else if (GetSocketType() == SocketType_WebSocket) {}
		else
		{
			// NOTE: 여기서는 CUdpPacketFragBoard.SetCoalesceInterval를 호출 안한다.
			// 이미 사용자가 CSuperSocket.SetCoalesceInterval을 호출했어야 한다.
			m_udpPacketFragBoard_needSendLock->DoForLongInterval(currTime);

			// 수신 큐에 대해서도 처리하기
			if ( m_udpPacketDefragBoard )
				m_udpPacketDefragBoard->DoForLongInterval(currTime);

			queueLength = GetUdpSendQueueLength();
		}
	}

	// TCP 송신이 한가한 remote에게 TCP 송신을 더미라도 보낸다.
	// 만약 remote가 랜선뽑기 상황이면 이것을 통해 보낸 시간의 20초 이내에 OS에 의해 TCP retransmission timeout으로 인한 단선이 유도된다.
	// 이게 없으면 영원한 단선 미감지가 발생한다.
	void CSuperSocket::SendDummyOnTcpOnTooLongUnsending(int64_t currTime)
	{
		// 웹소켓에 대해서는 더미패킷을 주는 것도 받는 것도 하면 안된다.
		if(SocketType_WebSocket == m_socketType)
		{
			return;
		}

		if (currTime - m_tcpOnlyLastSentTime > CNetConfig::WaitForSendingDummyPacketIntervalMs)
		{
			CMessage ignorant;
			ignorant.UseInternalBuffer();
			Message_Write(ignorant, MessageType_Ignore);

			SendOpt opt;
			opt.m_enableLoopback = false;
			opt.m_priority = MessagePriority_Ring0;

			AddToSendQueueWithSplitterAndSignal_Copy(shared_from_this(), CSendFragRefs(ignorant), opt, false);
		}
	}

	void CSuperSocket::DoForShortInterval(int64_t currTime)
	{
		CriticalSectionLock sendLock(m_sendQueueCS, true);

		if ( m_socketType == SocketType_Udp )
		{
			m_udpPacketFragBoard_needSendLock->DoForShortInterval(currTime);
		}
	}

	void CSuperSocket::ResetPacketFragState()
	{
		CriticalSectionLock lock(m_cs, true);

#if defined(_WIN32)
		assert(m_recvIssued != IoState_Working);
		assert(m_sendIssued != IoState_Working);
#else
		//assert(m_ioState != IoState_Working);
#endif
		m_sendIssuedFragment.Free();
		m_udpPacketFragBoard_needSendLock.Free();
		m_udpPacketDefragBoard.Free();

		m_sendIssuedFragment.Attach(new CUdpPacketFragBoardOutput);
		m_udpPacketFragBoard_needSendLock.Attach(new CUdpPacketFragBoard(this));
		m_udpPacketFragBoard_needSendLock->InitHashTableForClient();

		//assert(m_main->m_unsafeHeap != NULL);
		m_udpPacketDefragBoard.Attach(new CUdpPacketDefragBoard(this));

		//m_sendIssuedFragment->ResetForReuse();
		//m_udpPacketFragBoard->Clear();
		//if(m_udpPacketDefragBoard)
		//m_udpPacketDefragBoard->Clear();
	}

	void CSuperSocket::SetEnableNagleAlgorithm(bool enableNagleAlgorithm)
	{
		m_fastSocket->EnableNagleAlgorithm(enableNagleAlgorithm);
	}

	bool CSuperSocket::RefreshLocalAddr()
	{
		if ( m_socketType == SocketType_Tcp )
		{
			AddrPort a1 = m_fastSocket->GetSockName();
//			AddrPort a2 = AddrPort::GetOneLocalAddress();// 서버와 연결이 아직 안되어있으면 0.0.0.0 등 식별불가 IP가 나와야 정상.
//
//			// 설정된 IP주소가 없으면, 로컬 IP 중 한개를 가져 옵니다.
//			if ( (a1.m_binaryAddress == 0) && (a2.m_binaryAddress != 0) )
//			{
//				a1.m_binaryAddress = a2.m_binaryAddress;
//			}

			m_localAddr_USE_FUNCTION = a1;
		}
		else
		{
			m_localAddr_USE_FUNCTION = m_fastSocket->GetSockName();
		}

		return true;
		//		if (m_socketType == SocketType_Tcp)
		//		{
		//			AddrPort a1 = m_fastSocket->GetSockName();
		//			AddrPort a2 = AddrPort::GetOneLocalAddress();// 서버와 연결이 아직 안되어있으면 0.0.0.0 등 식별불가 IP가 나와야 정상.
		//			if (a1.m_binaryAddress == 0 && a2.m_binaryAddress != 0)
		//				a1.m_binaryAddress = a2.m_binaryAddress;
		//
		//			m_localAddr_USE_FUNCTION = a1;
		//
		//			return true;
		//		}
		//		else
		//		{
		//			m_localAddr_USE_FUNCTION = m_fastSocket->GetSockName();
		//			if (m_localAddr_USE_FUNCTION.m_binaryAddress == 0 || m_localAddr_USE_FUNCTION.m_binaryAddress == 0xffffffff)
		//			{
		//#if defined(_WIN32)
		//				// 이미 TCP와 연결된 상태에서 UDP소켓을 만들지 않으면 UDP소켓이 통신 어려운 NIC를 통해 상대에게 쏘고 상대는 거기서 인식된 주소로 홀펀칭하면 안습 상황이 생길 수 있다. 가령 게임회사 사내 랜카드 둘 이상 컴에서.
		//				String text;
		//				text.Format(_PNT("FATAL: The IP Address of UDP Socket from the Creation of the Client is the unbind address (%s)!"), m_localAddr_USE_FUNCTION.ToString());
		//				CErrorReporter_Indeed::Report(text); // nothrow
		//#endif
		//				return false;
		//			}
		//			return true;
		//		}
	}

	bool CSuperSocket::StopIoRequested()
	{
		return m_stopIoRequested_USE_FUNCTION == IoState_True;
	}

	/* iocp: overlapped io를 더 이상 않겠다는 선언 (0->2 atomic change)이 되어 있으면 true를 리턴한다.
	epoll: io event를 더 이상 처리할 수 없다는 선언 (0->2 atomic change)이 되어 있으면 true를 리턴한다. */
	bool CSuperSocket::StopIoAcked()
	{
		if (m_stopIoRequested_USE_FUNCTION != IoState_True)
			return false;

#ifndef _WIN32
		/* garbage socket 선언한지 10초가 됐는데도, 'socket을 액세스하는 최후의 스레드의 일이 마침'
		이 아직도 안 끝난다면(물론 그런 버그가 있어서는 안되지만), 모바일이나 PS4처럼 소켓 갯수 제한되는 곳에서는
		장시간 구동 중 소켓 생성 실패가 발생할 수 있다.
		이를 예방하기 위해서, garbage socket 후 장시간 안 없어지는 socket은 강제로 close를 해버린다.
		자세한 것은 m_requestStopIoTime 변수 설명 참고. */
		if (GetPreciseCurrentTimeMs() - m_requestStopIoTime > 10000)
		{
			m_fastSocket->CloseSocketOnly();
		}

		// non win32에서는 NetCoreHeart의 사라짐을 검사하지, StopIoAcked 리턴값을 검사하지 않는다.
		return true;
#else
// Working이 아닌 이상 NoMoreWorkGuaranteed로 변경해 버린다.
// 모두가 다 바뀌면 비로소 this를 없애도 된다.
		int p1 = AtomicCompareAndSwapEnum<IoState,int32_t>(IoState_NotWorking, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
		int p2 = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_NoMoreWorkGuaranteed, &m_sendIssued);

		bool acked = (p1 == IoState_NotWorking || p1 == IoState_NoMoreWorkGuaranteed)
			&& (p2 == IoState_NotWorking || p2 == IoState_NoMoreWorkGuaranteed);

		// garbage 시킨 소켓이 너무 오랜동안 기다려도 처리되지 못하고 있다면
		// 오류 로그를 출력합니다.
		if (GetPreciseCurrentTimeMs() - m_requestStopIoTime > 6000 && !acked && !m_warnTooLongGarbage)
		{
			m_warnTooLongGarbage = true;
			Tstringstream ss;
			ss << "SuperSocket garbage work takes too long time! SendIssued=" << m_sendIssued << ", RecvIssued=" << m_recvIssued << ", StopIoRequested=" << m_stopIoRequested_USE_FUNCTION;
			ss << ", g_msgSizeErrorCount=" << g_msgSizeErrorCount;
			ss << ", g_intrErrorCount=" << g_intrErrorCount;
			ss << ", g_netResetErrorCount=" << g_netResetErrorCount;
			ss << ", g_connResetErrorCount=" << g_connResetErrorCount;
			m_owner->EnqueWarning(ErrorInfo::From(ErrorType_Unexpected, HostID_None, ss.str()));
		}

		return acked;
#endif // win32 case
	}

	// 앞서 overlapped i/o completion 혹은 non-block syscall 후의 결과값을 가지고 나머지 처리를 한 후,
	// caller가 무슨 행동을 해야 하는지를 알려주는 함수.
	CSuperSocket::ProcessType CSuperSocket::GetNextProcessType_AfterRecv(const CIoEventStatus& comp)
	{
		Proud::AssertIsLockedByCurrentThread(m_cs);

		ProcessType ret = ProcessType_None;

		if (m_isListeningSocket)
		{
			AddrPort tcpLocalAddr, tcpRemoteAddr;

			// 이 socket은 accept를 전담하는 경우.

			// AcceptEx가 completion with ok인 경우, 수신 첫 데이터는 반드시 local and remote address이다.
			// 따라서 수신 바이트가 0인 경우는 없겠지만 방어 코딩을 위해 0도 체크.
			// 예외: WSAEINTR or EINTR 다루기.pptx

			// AcceptEx에서 두 addr외의 다른 것을 받지 않기 때문에 0을 리턴하면 accept ok라는 뜻.
			if (comp.m_completedDataLength < 0 && comp.m_errorCode != SocketErrorCode_Intr)
			{
				// 서버를 종료하는 상황이라서 여기에 왔을 수 있다. 해당 listen socket을 safe하게 닫도록 하자.
				ret = ProcessType_CloseSocketAndProcessDisconnecting;
				goto listen_end;
			}

			// accept 처리를 마무리.
			ret = ProcessType_OnAccept;
			if (!m_acceptCandidateSocket)
			{
				// 처리할 소켓이 없어도 ret는 상동. issue-accept은 해야 하니까.
				goto listen_end;
			}
			m_acceptCandidateSocket->FinalizeAcceptEx(m_fastSocket, tcpLocalAddr, tcpRemoteAddr);

			if (tcpRemoteAddr.IsUnicastEndpoint())
			{
				// accept 성공. issue-accept 하기 전에 OnAccepted를 호출되게 하자.
				// m_acceptedSocket을 세팅함으로.
				shared_ptr<CSuperSocket> superSocket;

				SuperSocketCreateResult r = CSuperSocket::New(m_owner, m_acceptCandidateSocket, SocketType_Tcp);
				if (!r.socket)
				{
					// accept이 실패했다. 딱히 할 것은 없고, 그냥 계속 진행한다.
					m_acceptCandidateSocket = shared_ptr<CFastSocket>();
				}
				superSocket = r.socket;

				// 처리 성공. m_acceptedSocket로 옮긴다.
				m_acceptedSocket = superSocket;
				m_acceptedSocket_tcpLocalAddr = tcpLocalAddr;
				m_acceptedSocket_tcpRemoteAddr = tcpRemoteAddr;

				m_acceptCandidateSocket = shared_ptr<CFastSocket>();
			}
			else
			{
				// accept이 실패했다. 딱히 할 것은 없고, 그냥 계속 진행한다.
				m_acceptCandidateSocket = shared_ptr<CFastSocket>();
			}
		listen_end:
			;
		}
		else
		{
			ret = ProcessType_OnMessageReceived;
			if ( m_socketType == SocketType_Tcp )
			{
				// TCP로 receive를 하고 있던 경우.

				// 수신의 경우 0바이트 수신했음 혹은 음수바이트 수신했음이면 연결에 문제가 발생한 것이므로 디스해야 한다.
				// 예외: WSAEINTR or EINTR 다루기.pptx
				// 2016.3.7 linux 서버에 window 클라이언트가 접속 후 클라이언트의 종료 시 linux 서버에 OnClientLeave 콜백이 호출되지 않는 문제 수정
				// 서버에서 Disconnet 메시지를 받은 후 errorno 104에 해당하는 ECONNRESET가 발생하는 점을 이용하여 수정
				if (comp.m_completedDataLength == 0 ||  // TCP 종료
					(comp.m_completedDataLength < 0 && comp.m_errorCode != SocketErrorCode_Intr)  // TCP 강제 종료
					)
				{
#if defined (_WIN32) // Linux에는 SocketErrorCode_Cancelled의 값이 없으며 이미 SocketErrorCode_Intr에 대해 검사를 수행
					if ( comp.m_errorCode != SocketErrorCode_Intr && comp.m_errorCode != SocketErrorCode_Cancelled )
#endif // _WIN32
					{
						ret = ProcessType_CloseSocketAndProcessDisconnecting;
					}

				}
				else
				{
				}
			}
			else
			{
				// UDP로 receive를 하고 있던 경우.
				// 하지만, UDP에서는 따로 disconnect 관련 처리할 것이 없다.
				assert(m_socketType == SocketType_Udp);
			}
		}

		return ret;
	}

	// >0 바이트를 수신했을 때의 처리.
	// epoll or iocp case를 모두 처리할 수 있다.
	// message 추출=>socket unlock=>caller가 해야 할 명령 가령 OnMessageReceived callback을
	// 실행하던지 등을 지령.
	// (직접 콜백을 여기서 수행 안함. per-socket lock 상태에서 여기가 실행될 수 있으므로.)
	CSuperSocket::ProcessType CSuperSocket::ExtractMessage(
		const CIoEventStatus &comp,
		CReceivedMessageList& outMsgList,
		ErrorInfoPtr& outErrInfo)
	{
		AssertIsLockedByCurrentThread(m_cs);

		ProcessType ret = ProcessType_None;
		int64_t absTime = GetPreciseCurrentTimeMs();

		// 받은 UDP packet을 assemble한다.
		// note: assembled packet은 1개 이상의 message를 가지고 있다.
		// coalesce가 되어있어 1개 이상이다.

		// NOTE: CAssembledPacket 소멸자에서 DefraggingPacket를 정리하기 때문에 Extract 작업이 완료되기 전까지 소멸이 되면 안됩니다.
		CAssembledPacket assembledPacket;

		// TCP or UDP로 받은 메시지. 여기서 message들을 추출해야.
		uint8_t* receivedData = NULL; // TCP의 경우 항상 set, UDP의 경우 assembled packet인 경우에만 set
		int receivedDataLength = 0;
		AddrPort remoteAddr_onlyUDP;

		if ( m_socketType == SocketType_Tcp )
		{
			// 받은 데이터를 수신 스트림에 이어붙인다.
			m_recvStream->PushBack_Copy(m_fastSocket->GetRecvBufferPtr(), comp.m_completedDataLength);
			receivedData = m_recvStream->GetData();
			receivedDataLength = m_recvStream->GetLength();
		}
		else
		{
			if ( m_dropSendAndReceive == false && comp.m_completedDataLength > 0 )
			{
				/* Q: volatile한 sender hostID를 얻어올 경우, filter tag가 막을 패킷을 못 막지 않나요?
				A: 괜찮다. sender hostID를 변화시키는 다른 스레드가 없고 (데이터 수신 핸들러에서만 변화하므로)
				데이터 수신 핸들러는 1개 스레드에서만 처리하기 때문이다.
				(iocp: issue recv를 동시에 한 스레드만 하므로, epoll: 한 스레드만 한 소켓에 대한 수신 이벤트 처리하므로)
				Q: 타이머나 다른 sender로 인해서 hostID가 돌발 바뀌는 것에 대해서는 문제되지 않을까?
				A: sender로부터 온 데이터를 처리하는 돌연 타 sender나 타이머에 의해 sender의 hostID가 바뀌는 것은
				무시해도 된다. 어차피 다른 sender의 수신 처리나 타이머로 인한 hostID 돌발 변경 사건은 이 사건
				즉 sender로부터 온 데이터를 처리하는 것과는 시간 독립적인 사건이기 때문이다.

				게다가, message extract 과정은 긴 CPU time이다. 이를 per-socket lock으로 처리하면 어차피 이미 긴
				syscall과 섞이기 때문에 병렬 처리률도 높아져서 성능상 이익이다. */

				// Non-block recv 가 끝났을때 세팅 하도록 변경
// #if !defined (_WIN32)
//				comp.m_receivedFrom.FromNative(m_fastSocket->GetRecvedFrom());
// #endif

				String errorOut;
				AssembledPacketError assembled;
				{
					CriticalSectionLock sendQueueLock(m_sendQueueCS, true);

					assembled = m_udpPacketDefragBoard->PushFragmentAndPopAssembledPacket(
						m_fastSocket->GetRecvBufferPtr(),
						comp.m_completedDataLength,
						comp.m_receivedFrom,
						ReceivedAddrPortToVolatileHostIDMap_Get(comp.m_receivedFrom),
						absTime,
						assembledPacket,
						errorOut);
				}

				if ( assembled == AssembledPacketError_Ok )
				{
					// full packet이 뽑혀져 나왔다. 아래 extract하기 위한 곳에 넣도록 하자.
					receivedData = assembledPacket.GetData();
					receivedDataLength = assembledPacket.GetLength();
					remoteAddr_onlyUDP = assembledPacket.m_senderAddr;
				}
				else if ( assembled == AssembledPacketError_Error )
				{
					outErrInfo = ErrorInfoPtr(new ErrorInfo);
					outErrInfo->m_comment = errorOut;
					ret = ProcessType_EnquePacketDefragWarning;

					assert(receivedData == NULL);
				}
			}
		}

		if ( receivedData != NULL )
		{
			/* socket lock을 unlock 안하고 extract message처리를 하는 이유
			1. 방어적 코딩
			2. per-socket lock은 syscall을 하는 구간이다. 따라서 어차피 이미 시간 오래 먹었기에
			굳이 extract message처리를 unlock안해도 큰 차이 안난다. */

			ErrorType extractError;
			CTcpLayerMessageExtractor extractor;
			extractor.m_recvStream = receivedData;
			extractor.m_recvStreamCount = receivedDataLength;
			extractor.m_extractedMessageAddTarget = &outMsgList;
			extractor.m_senderHostID = HostID_None;
			extractor.m_messageMaxLength = m_owner->GetMessageMaxLength();
			extractor.m_remoteAddr_onlyUdp = comp.m_receivedFrom;
			//extractor.m_unsafeHeap = unsafeHeap;

			int addedCount = extractor.Extract(extractError, m_owner->IsSimplePacketMode());

			if ( addedCount < 0 ) // 스트림이 깨지면 음수
			{
				// 서버와의 TCP 연결에 문제가 있다! TCP의 경우 한번 스트림 일관성이 깨지면 복구가 어렵다.
				// 따라서 연결을 끊도록 한다.
				if ( m_socketType == SocketType_Tcp )
				{
					outErrInfo = ErrorInfoPtr(new ErrorInfo);
					outErrInfo->m_errorType = extractError;
					outErrInfo->m_comment = _PNT("Received stream from TCP server became inconsistent!");
					ret = ProcessType_CloseSocketAndProcessDisconnecting;
				}
				else
				{
					// 잘못된 스트림 데이터이다. UDP는 제3자 해커로부터의 메시지가 오는 경우도 있으므로
					// 저쪽과의 연결을 끊지 않되, 그냥 조용히 수신된 메시지들을 폐기해야 한다.
					// 그리고 Warning을 남겨주자.
					outErrInfo = ErrorInfo::From(extractError,
												 m_owner->GetLocalHostID(),
												 _PNT("Received datagram from UDP became inconsistent!"));
					ret = ProcessType_EnqueWarning;
				}
			}
			else
			{
				// 성공적으로 받아졌다.
				if ( m_socketType == SocketType_Tcp )
				{
					// TCP의 경우 수신큐에서 제거하도록 하자.
					m_recvStream->PopFront(extractor.m_outLastSuccessOffset);
				}
				else
				{
					// UDP의 경우 조립된 full packet을 꺼내면서 이미 defrag board에서 제거되었다.
				}
				ret = ProcessType_OnMessageReceived;
			}
		}

		return ret;
	}

	// socket I/O 중에 난 에러 상황을 갖고 ErrorInfo 객체를 만든다.
	// send or recv 호출이었냐? 리턴된 값은? errno는? 에 따라 내용을 채운다.
	void CSuperSocket::BuildDisconnectedErrorInfo(ErrorInfo& output,
		IoEventType eventType,
		int ioLength,
		SocketErrorCode errorCode)
	{
		output.m_socketError = errorCode;
		if (m_userCalledDisconnectFunction)
		{
			output.m_errorType = ErrorType_DisconnectFromLocal;
			output.m_detailType = ErrorType_TCPConnectFailure;
			output.m_comment = _PNT("TCP graceful disconnect, NetClient.Disconnect() or NetServer.CloseConnection() has been called.");
		}
		//		else if (StopIoRequested()) 이거를 검사하지 말자! caller가 RequestStopIo를 호출한 후 이것을 호출하기 때문이다. 이걸 검사해 버리면 아래 if구문들이 모두 무시된다.
		//		{
		//			output.m_errorType = ErrorType_DisconnectFromLocal;
		//			output.m_detailType = ErrorType_TCPConnectFailure;
		//			output.m_comment = _PNT("We stopped socket I/O already.");
		//		}
		else if (eventType == IoEventType_Receive && ioLength == 0)
		{
			output.m_errorType = ErrorType_DisconnectFromRemote;
			output.m_detailType = ErrorType_TCPConnectFailure;
			output.m_comment = _PNT("TCP graceful disconnect.");
		}
		else
		{
			output.m_errorType = ErrorType_DisconnectFromRemote;
			output.m_detailType = ErrorType_TCPConnectFailure;
			output.m_comment.Format(_PNT("Event type=%d, I/O length=%d, error code=%d"), eventType, ioLength, errorCode);
		}
	}

	Proud::AddrPort CSuperSocket::GetLocalAddr()
	{
		if ( m_localAddr_USE_FUNCTION == AddrPort::Unassigned && m_fastSocket.get() != NULL )
		{
			RefreshLocalAddr();
		}
		return m_localAddr_USE_FUNCTION;
		//return AddrPort::Unassigned;
	}

	// destAddr로의 보내는 패킷에 대해 coalesce interval을 지정한다.
	void CSuperSocket::SetCoalesceInteraval(const AddrPort &destAddr, int coalesceIntervalMs)
	{
		// 이 한줄 때문에 추가됨.
		CriticalSectionLock lock(m_sendQueueCS, true);

		m_udpPacketFragBoard_needSendLock->SetCoalesceInterval(destAddr, coalesceIntervalMs);
	}

	// CanDeleteNow 디버그용. 문제 해결을 해도 냅두자. 미증유 발견 위해.
	void CSuperSocket::CanDeleteNow_DumpStatus()
	{
		cout << "SS begin\n";
		cout << "   T=" << m_socketType << endl;
#ifdef _WIN32
		// 이것이 영원히 working이면, GQCS가 해당하는게 안왔거나, 왔지만 합당한 처리를 못했거나다.
		cout << "   SI flag=" << m_sendIssued << endl;
		cout << "   RI flag=" << m_recvIssued << endl;
#endif
		cout << "SS end\n";

	}

	// recvfrom으로 받은 주소를 입력하면 그 송신자의 hostID를 리턴한다.
	// filter tag 연산에서 쓰임.
	Proud::HostID CSuperSocket::ReceivedAddrPortToVolatileHostIDMap_Get(const AddrPort& senderAddr)
	{
		HostID remoteHostID;
		if ( m_receivedAddrPortToVolatileHostIDMap.TryGetValue(senderAddr, remoteHostID) )
			return remoteHostID;

		return HostID_None;
	}

	/* 이 함수를 호출하는 곳은, 상대방 즉 송신자로부터 UDP datagram을 받았을 때
		즉 recvfrom의 인자로 받은 주소가 가리키는 송신자가 알려진 HostID를 갖고 있을 때 map에 설정하는 목적으로 쓰여야 합니다.
		따라서 이 함수는 m_UdpAddrPortToRemoteClientIndex에 add or set을 하는 모든 곳에서 호출해야 합니다.
		기존에 이 함수를 호출하고 있는 곳들을 찾아 제거 후 위 수정 바랍니다.
		*/

	// 송신자 쪽이 HostID를 할당받았으면 이걸 호출해서 송신자로부터 오는 패킷의 filtertag 유효성을 검사가 시작될 수 있게 해야.
	// senderAddr: 송신자의 주소 즉 recvfrom의 인자로 받은 주소다. UDP socket의 local addr가 들어갈 수 없음에 주의할 것.
	void CSuperSocket::ReceivedAddrPortToVolatileHostIDMap_Set(const AddrPort& senderAddr, HostID remoteHostID)
	{
		assert(senderAddr.IsUnicastEndpoint());

		m_receivedAddrPortToVolatileHostIDMap[senderAddr] = remoteHostID; // add or set
	}

	/* 이 함수를 호출하는 곳은, 상대방 즉 송신자로부터 UDP datagram을 받았을 때
		즉 recvfrom의 인자로 받은 주소가 가리키는 송신자가 알려진 HostID를 갖고 있을 때 map에 설정하는 목적으로 쓰여야 합니다.
		따라서 이 함수는 m_UdpAddrPortToRemoteClientIndex에 remove을 하는 모든 곳에서 호출해야 합니다.
		기존에 이 함수를 호출하고 있는 곳들을 찾아 제거 후 위 수정 바랍니다.
		*/

	// 송신자 쪽이 HostID를 잃었으면 즉 나갔으면 이걸 호출해서 송신자로부터 오는 패킷의 filtertag 유효성 검사가 중단되게 해야.
	// senderAddr: 송신자의 주소 즉 recvfrom의 인자로 받은 주소다. UDP socket의 local addr가 들어갈 수 없음에 주의할 것.
	void CSuperSocket::ReceivedAddrPortToVolatileHostIDMap_Remove(const AddrPort& senderAddr)
	{
		//assert(senderAddr.m_binaryAddress != 0xffffffff);
		//assert(senderAddr.m_binaryAddress != 0);

		m_receivedAddrPortToVolatileHostIDMap.Remove(senderAddr);
	}

	// remote client의, 외부 인터넷에서도 인식 가능한 TCP 연결 주소를 리턴한다.
	//	Proud::NamedAddrPort CSuperSocket::GetRemoteIdentifiableLocalAddr(CHostBase* rc)
	//	{
	//		GetCriticalSection().AssertIsNotLockedByCurrentThread();//mainlock만으로 진행이 가능할듯.
	//
	//		/* 이 함수 자체는 SuperSocket의 멤버로 두지 맙시다.
	//			대신, CNetServer.GetRemoteIdentifiableLocalAddr(rc) 형식으로 만들어, 아래 루틴이 들어가게 합시다.
	//			위에 함수 설명 추가했음.
	//			*/
	//		NamedAddrPort ret = NamedAddrPort::From(m_socket->GetSockName());
	//		ret.OverwriteHostNameIfExists(rc->m_tcpLayer->m_socket->GetSockName().ToDottedIP());  // TCP 연결을 수용한 소켓의 주소. 가장 연결 성공률이 낮다. NIC가 두개 이상 있어도 TCP 연결을 받은 주소가 UDP 연결도 받을 수 있는 확률이 크므로 OK.
	//		ret.OverwriteHostNameIfExists(m_owner->m_localNicAddr); // 만약 NIC가 두개 이상 있는 서버이며 NIC 주소가 지정되어 있다면 지정된 NIC 주소를 우선 선택한다.
	//		ret.OverwriteHostNameIfExists(m_owner->m_serverAddrAlias); // 만약 서버용 공인 주소가 따로 있으면 그것을 우선해서 쓰도록 한다.
	//
	//		if (!ret.IsUnicastEndpoint())
	//		{
	//			//assert(0); // 드물지만 있더라. 어쨌거나 어설션 즐
	//			ret.m_addr = StringW2A(CNetUtil::GetOneLocalAddress());
	//		}
	//
	//		return ret;
	//	}



	// 이 함수는 제거합시다. 즉 이번 버전에서는 이 함수를 필요로 하는 기능이 모두 제거되어야 합니다.
	// 물론 MessageType_RequestReceiveSpeedAtReceiverSide_NoRelay 정의 자체는 하위호환성을 위해 제거하면 안되고요.
	// 씨드나인 버전 준 후에 R2.2의 혼잡제어 기능 구현을 여기로 가져옴으로 이 기능을 대체합시다.
	//	void CSuperSocket::RequestReceiveSpeedAtReceiverSide_NoRelay(AddrPort dest)
	//	{
	//		// 소켓이 공유되고 있으므로 적절한 dest를 찾아서 쏨
	//		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
	//		Message_Write(sendMsg, MessageType_RequestReceiveSpeedAtReceiverSide_NoRelay);
	//
	//		// 서버측에서의 UDP 수신 속도를 요청한다.
	//		AddToSendQueueWithSplitterAndSignal_Copy(HostID_None,
	//			FilterTag::CreateFilterTag(HostID_Server, HostID_None),
	//			dest,
	//			sendMsg,
	//			SendOpt(MessagePriority_Ring1, true));
	//	}

	int CSuperSocket::GetMessageMaxLength()
	{
		// 주의: main lock 걸지 말것. caller가 per-socket lock을 걸었으므로.
		AssertIsNotLockedByCurrentThread(m_owner->GetCriticalSection());
		return m_owner->GetMessageMaxLength();
	}

	uint32_t CSuperSocket::GetUdpSendQueueLength()
	{
		return m_udpPacketFragBoard_needSendLock->GetPacketQueueTotalLength();
	}

	int CSuperSocket::GetUnreliableMessagingLossRatioPercent(AddrPort& senderPort)
	{
		return m_udpPacketDefragBoard->GetUnreliableMessagingLossRatioPercent(senderPort);
	}

	void CSuperSocket::SetSocketBlockingMode(bool flag)
	{
		m_fastSocket->SetBlockingMode(flag);
	}

	Proud::AddrPort CSuperSocket::GetSocketName()
	{
		return m_fastSocket->GetSockName();
	}

	// called by io state changer and io state stop ack checker.
	CriticalSection* CSuperSocket::GetAssociatedIoQueueCritSec()
	{
		if (!m_associatedIoQueue_accessByAssociatedSocketsOnly)
			return NULL;
		return &m_associatedIoQueue_accessByAssociatedSocketsOnly->m_cs;
	}

	SocketErrorCode CSuperSocket::Bind()
	{
		return Bind(0);
	}

	SocketErrorCode CSuperSocket::Bind(int port)
	{
#ifdef __ORBIS__
		// PS4에서는 IPv4 API밖에 없어서. 즉 ::1을 인식 못해서 이렇게 한다.
		return Bind(AddrPort::FromIPPort(AF_INET, _PNT(""), port));
#else
		// dual stack socket이다. 그냥 여기서 무조건 IPv6 socket을 만들자.
		return Bind(AddrPort::FromIPPort(AF_INET6, _PNT(""), (uint16_t)port));
#endif
	}

	SocketErrorCode CSuperSocket::Bind(const PNTCHAR* addr, int port)
	{
		AddrPort addrPort;
		SocketErrorCode ret = SocketErrorCode_Error;

		if (AddrPort::FromHostNamePort(&addrPort, ret, addr, (uint16_t)port) == false)
		{
			return ret;
		}

		return Bind(addrPort);
	}

	SocketErrorCode CSuperSocket::Bind(AddrPort localAddr)
	{
		SocketErrorCode socketErrorCode = SocketErrorCode_Ok;

		socketErrorCode = m_fastSocket->Bind(localAddr);
		if ( socketErrorCode != SocketErrorCode_Ok )
		{
			return socketErrorCode;
		}

		if (RefreshLocalAddr() != true)
		{
			return SocketErrorCode_Error;
		}

		return SocketErrorCode_Ok;
	}

	void CSuperSocket::SetSocketVerboseFlag(bool flag)
	{
		m_fastSocket->SetVerboseFlag(flag);
	}

	void CSuperSocket::UdpPacketFragBoard_Remove(AddrPort addrPort)
	{
		CriticalSectionLock sendLock(m_sendQueueCS, true);

		if ( m_udpPacketFragBoard_needSendLock )
			m_udpPacketFragBoard_needSendLock->Remove(addrPort);
	}

	void CSuperSocket::UdpPacketDefragBoard_Remove(AddrPort addrPort)
	{
		if ( m_udpPacketDefragBoard )
			m_udpPacketDefragBoard->Remove(addrPort);
	}

	int64_t CSuperSocket::GetRecentReceiveSpeed(AddrPort src)
	{
		return m_udpPacketDefragBoard->GetRecentReceiveSpeed(src);
	}

	void CSuperSocket::SetReceiveSpeedAtReceiverSide(AddrPort dest, int64_t speed, int packetLossPercent, int64_t curTime)
	{
		CriticalSectionLock sendLock(m_sendQueueCS, true);

		if ( m_udpPacketFragBoard_needSendLock )
			m_udpPacketFragBoard_needSendLock->SetReceiveSpeedAtReceiverSide(dest, speed, packetLossPercent, curTime);
	}

	void CSuperSocket::SetTcpUnstable(int64_t curTime, bool unstable)
	{
		CriticalSectionLock sendLock(m_sendQueueCS, true);

		m_udpPacketFragBoard_needSendLock->SetTcpUnstable(curTime, unstable);
	}

	void CSuperSocket::MustTcpSocket()
	{
		// 이 overloaded function는 TCP 전용이다.
		if ( m_socketType != SocketType_Tcp )
		{
			throw Exception("AddToSendQueueWithSplitterAndSignal_Copy: wrong TCP function call.");
		}
	}

	void CSuperSocket::RefreshLastReceivedTime()
	{
		// 랜선 플러그 아웃 상황을 연출 하기 위해선 received time을 갱신하면 안됨
		if (m_turnOffSendAndReceive)
		{
			return;
		}

		m_lastReceivedTime = GetPreciseCurrentTimeMs();
	}

	int64_t CSuperSocket::m_firstReceiveDelay_TotalValue = 0;
	int64_t CSuperSocket::m_firstReceiveDelay_TotalCount = 0;
	int64_t CSuperSocket::m_firstReceiveDelay_MaxValue = 0;
	int64_t CSuperSocket::m_firstReceiveDelay_MinValue = 1000000000;
}
