/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

//////////////////////////////////////////////////////////////////////////
// IOCP 즉 proactor에서만 유효한 모듈

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
//#include "NetClient.h"
//#include "ServerSocketPool.h"
//#include "RemotePeer.h"
//#include "TCPLayer.h"
//#include "Epoll_Reactor.h"
#include "NetCore.h"
#include "ReportError.h"
#include "Epoll_Reactor.h"


// Send Queue를 확인하기 위한 Test 용도로 사용하는 변수
bool Proud::CSuperSocket::m_test_isSendQueueLock = false;


namespace Proud
{

#if defined (_WIN32)
	void CSuperSocket::OnSocketIoCompletion(CIoEventStatus &comp)
	{
		CUseCounter counter(*this);

		switch ( comp.m_type )
		{
			case IoEventType_Send:
				ProcessOverlappedSendCompletion(comp);
				break;
			case IoEventType_Receive:
				ProcessOverlappedReceiveCompletion(comp);
				break;
		}
	}


	// overlapped i/o send completion이 발생한 것을 처리한다. iocp 전용.
	void CSuperSocket::ProcessOverlappedSendCompletion(CIoEventStatus &comp)
	{
		m_owner->AssertIsNotLockedByCurrentThread();
		// 반드시 이것이 있어야 OnMessageReceived 안에서 main lock 시 데드락이 안 생긴다. 
		// lock oreder가 main then socket이니까.
		Proud::AssertIsNotLockedByCurrentThread(m_cs);

		CriticalSectionLock socketLock(m_cs, true);

		/* completion 결과가 EINTR or WSAECANCELLED인 경우
		TTL 복원 하지 말고 앞서 보내기 issue했던 내용 자체를 가지고 즉시 재 issue해야 한다.
		WSAEINTR: 내부 문제였기 때문이다.
		WSAECANCELLED: worker thread를 줄이는 경우 발생할 수 있으니까.
		TCP의 경우: 에러가 온 경우 completed bytes가 0일 것이다. 따라서 통상 재시도 그냥 하면 됨. 소켓 닫지 말고!
		udp의 경우:앞서 issue 했던 내용 자체는 m_sendIssuedFragment에 아직 있을 것이다. 그것을 그대로 쓰자. (PopFragmentOrFullPacket을 호출해서 기존 가졌던 것을 겹쳐쓰지 말것)
		상기는 ProcessRecvCompletion에서도 마찬가지로 고치자. */

		if ( m_socketType == SocketType_Udp )
		{
			if ( comp.m_errorCode != SocketErrorCode_Intr && comp.m_errorCode != SocketErrorCode_Cancelled )
			{
				// 앞서 TTL을 바꾼 적이 있다면 여기서 교체해야.
				m_fastSocket->RestoreTtlOnCompletion();
			}
		}


		bool issueSendOnNeed = false;
		SocketErrorCode e;
		ProcessType nextProc = GetNextProcessType_AfterSend(comp, e);

		socketLock.Unlock();
		AssertIsNotLockedByCurrentThread(m_cs);

		switch ( nextProc )
		{
			case ProcessType_OnMessageSent:
			{
				m_owner->OnMessageSent(comp.m_completedDataLength, m_socketType);
				issueSendOnNeed = true;
				break;
			}
			case ProcessType_OnConnectSuccess:
			{
				m_owner->OnConnectSuccess(this);
				issueSendOnNeed = true;
				break;
			}
			case ProcessType_OnConnectFail:
			{
				m_owner->OnConnectFail(this, e);
				break;
			}
			case ProcessType_CloseSocketAndProcessDisconnecting:
			{
				assert(m_socketType == SocketType_Tcp);
				if ( CloseSocketOnly() )
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo,
											   IoEventType_Send,
											   comp.m_completedDataLength,
											   comp.m_errorCode);
					m_owner->ProcessDisconnecting(this, errorInfo);
				}
				break;
			}
			default:
			{
				assert(nextProc == ProcessType_None);
				break;
			}
		}

		int32_t p = AtomicCompareAndSwap32(1, 0, &m_sendIssued);
		assert(p == 1);

		if ( issueSendOnNeed )
		{
			// 이 루틴이 실행중일떄는 아직 owner가 건재함을 전제한다.
			socketLock.Lock();
			IssueSendOnNeedAndUnlock(true, socketLock);
			assert(socketLock.IsLocked() == false);
		}
	}

	// overlapped i/o receive completion이 발생한 것을 처리한다. iocp 전용.
	void CSuperSocket::ProcessOverlappedReceiveCompletion(CIoEventStatus &comp)
	{
		m_owner->AssertIsNotLockedByCurrentThread();
		// 반드시 이것이 있어야 OnMessageReceived 안에서 main lock 시 데드락이 안 생긴다. 
		// lock oreder가 main then socket이니까.
		Proud::AssertIsNotLockedByCurrentThread(m_cs);

		CriticalSectionLock socketLock(m_cs, true);

		if (AtomicCompareAndSwap32(1, 0, &m_recvIssued) != 1)
		{
			// 이미 다 처리되어서 끝난 놈인데 completion 신호가 온다고라고라? 버그다!
			assert(0);
		}


		ProcessType nextProc = GetNextProcessType_AfterRecv(comp);
		switch (nextProc)
		{
		case ProcessType_OnMessageReceived:
		{
			RefreshLastReceivedTime();

			POOLED_ARRAY_LOCAL_VAR(CReceivedMessageList, msgList);

			ErrorInfoPtr errorinfo;
			ProcessType p = ExtractMessage(comp, msgList, errorinfo);

			socketLock.Unlock();
			// 이것이 확인 안된채로 아래 main lock이 내장된 콜백에 돌입하면 데드락 생김
			AssertIsNotLockedByCurrentThread(GetCriticalSection());

			if (p == ProcessType_OnMessageReceived)
			{
				assert(errorinfo == NULL);
				m_owner->OnMessageReceived(comp.m_completedDataLength,
					m_socketType,
					msgList,
					this);
			}
			else if (p == ProcessType_CloseSocketAndProcessDisconnecting)
			{
				assert(errorinfo != NULL);
				assert(m_socketType == SocketType_Tcp);
				if (CloseSocketOnly())
				{
					m_owner->ProcessDisconnecting(this, *errorinfo);
				}
			}
			else if (p == ProcessType_EnqueWarning)
			{
				assert(errorinfo != NULL);
				m_owner->EnqueWarning(errorinfo);
			}
			else if (p == ProcessType_EnquePacketDefragWarning)
			{
				assert(errorinfo != NULL);
				m_owner->EnquePacketDefragWarning(this, comp.m_receivedFrom, errorinfo->m_comment);
			}
			else
			{
				assert(p == ProcessType_None);
			}

			IssueRecv();
		}
		break;
		case ProcessType_OnAccept:
		{
			assert(m_isListeningSocket);
			assert(m_socketType == SocketType_Tcp);

			// accepted socket을 가져온다. 그리고 issue-accept를 다음에 건다.
			// 이렇게 해야 socket unlock을 1회만 할 수 있으니까.
			CSuperSocket* acceptedSocket = m_acceptedSocket;
			AddrPort acceptedSocket_tcpLocalAddr;
			AddrPort acceptedSocket_tcpRemoteAddr;

			if (acceptedSocket)
			{
				acceptedSocket_tcpLocalAddr = m_acceptedSocket_tcpLocalAddr;
				acceptedSocket_tcpRemoteAddr = acceptedSocket->m_fastSocket->GetPeerName();
			}
			m_acceptedSocket = NULL; // detach

			// 이제 issue-accept을 건다.
			SocketErrorCode e = IssueAccept();
			if (e != SocketErrorCode_Ok &&
				e != SocketErrorCode_IoPending)
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				if (CloseSocketOnly())
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Receive, 0, e);
					m_owner->ProcessDisconnecting(this, errorInfo);
				}
			}
			else
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				// 위 issue-accept 결과는 성공하건 실패하건,
				// accept한 소켓이 있으면 처리는 해야.
				if (acceptedSocket)
				{
					m_owner->OnAccepted(acceptedSocket,
						acceptedSocket_tcpLocalAddr, acceptedSocket_tcpRemoteAddr);
				}
			}
		}
		break;
		case ProcessType_CloseSocketAndProcessDisconnecting:
		{
			socketLock.Unlock();
			AssertIsNotLockedByCurrentThread(m_cs);

			assert(m_socketType == SocketType_Tcp);
			if (CloseSocketOnly())
			{
				ErrorInfo errorInfo;
				BuildDisconnectedErrorInfo(errorInfo,
					IoEventType_Receive,
					comp.m_completedDataLength,
					comp.m_errorCode);
				m_owner->ProcessDisconnecting(this, errorInfo);

			}
		}
		break;
		default:
		{
			assert(nextProc == ProcessType_None);
		}
		break;
		}
	}

	/* issue send on need의 본체 함수.
	왜 unlock and claaback이 있어도 안전한지?
	- 기존 IssueSendOnNeed의 콜러들을 확인해 봤는데 해당 함수가 리턴 된 이후부터는 할 일이 없고 즉시 락을 풀었습니다.
	- 함수가 끝날 때 락을 풀어야 하는 경우가 있어서 일관성을 위함.

	이 함수는 LowContextSwitchLoop에서도 불려지기 때문에
	인자로 CriticalSectionLock을 받을 수 밖에 없다.
	이 함수는 리턴 직전에 socketLock을 해제한다. */
	void CSuperSocket::IssueSendOnNeedAndUnlock(bool calledBySendCompletion, CriticalSectionLock& socketLock)
	{
		AssertIsLockedByCurrentThread(m_cs);
		assert(socketLock.GetRecursionCount() == 1);
		int32_t p;

		// 소켓이 건재하고 송신 미이슈 상태에 한해
		if ( m_stopIoRequested )
		{
			AtomicCompareAndSwap32(0, 2, &m_sendIssued);
			goto ret;
		}

		// garbaged에 들어갔으면 send를 하지 말아야 한다. i/o는 살아있지만 송신은 안하고 수신은 다 받는 족족 버려야 한다.
		if (m_timeToGarbage != 0) 
			goto ret;

		p = AtomicCompareAndSwap32(0, 1, &m_sendIssued);

		// 아직 issue가 진행중이면 또 issue하면 안되므로
		if ( p != 0 )
			goto ret;

		ProcessType nextProc;
		SocketErrorCode e = IssueSendOnNeed_internal(calledBySendCompletion, nextProc);
		if ( e != SocketErrorCode_Ok )
		{
			if ( nextProc == ProcessType_CloseSocketAndProcessDisconnecting )
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				assert(m_socketType == SocketType_Tcp);
				if ( CloseSocketOnly() )
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Send, 0, e);
					m_owner->ProcessDisconnecting(this, errorInfo);

				}
			}
			else
			{
				assert(nextProc == ProcessType_None);
			}

			// 원상복구
			p = AtomicCompareAndSwap32(1, 0, &m_sendIssued);
			assert(p == 1);
		}
		else
		{
			assert(nextProc == ProcessType_None);
		}

	ret:
		// CriticalSectionLock 객체는 Lock 횟수보다 Unlock을 많이 해도 안전하다.
		socketLock.Unlock();
		assert(socketLock.GetRecursionCount() == 0);
		AssertIsNotLockedByCurrentThread(m_cs);
	}

	SocketErrorCode CSuperSocket::IssueSendOnNeed_internal(bool calledBySendCompletion, ProcessType& outNextProcess)
	{

		// Send Queue에 쌓인 량을 테스트하기 위해 추가된 내용이다.
		// ※ 경고 : 아래 소스는 테스트 용이므로 다른 용도로는 사용하지 말 것.
		if (Test_IsSendQueueLock())
		{
			return SocketErrorCode_Error;
		}

		//메인 Lock가 걸리면 안된다.여기서 하는일은 main lock이 관여하는 것이 아니므로.
		m_owner->LockMain_AssertIsNotLockedByCurrentThread();
		AssertIsLockedByCurrentThread(m_cs);

		outNextProcess = ProcessType_None;
		int64_t currTime = GetPreciseCurrentTimeMs();

		// 랜선이 끊어진 상황은 실제 전송자체를 막아버려야 시뮬레이션이 정확함
		if (m_turnOffSendAndReceive)
		{
			return SocketErrorCode_Error;
		}

		int bufferLength;
		if ( m_socketType == SocketType_Tcp )
		{
			// overlapped send를 위한 WSABUF array이다. WSABUF array 자체는 WSASend 호출 직후 폐기되어도 상관없다.
			// WSABUF array가 가리키는 데이터만 훼손되지 않으면 된다.
			CFragmentedBuffer sendBuf;

			// 보낼 스트림을 꺼낸다.
			// 상한선이 넘어도 문제 없다. 어차피 버퍼는 safe하고 TCP socket이 가득 차면 어차피 completion만 늦을 뿐이니까.
			{
				CriticalSectionLock sendQueueLock(m_sendQueueCS, true);
				m_sendQueue_needSendLock->PeekSendBuf(sendBuf); 
				bufferLength = sendBuf.GetLength();
				if ( bufferLength <= 0 )
				{
					return SocketErrorCode_Error;
				}
			}
			// send queue critsec이 비보호 상태지만, 타 스레드에서 중간에 push를 할 지언정 pop or delete를 하지는 않는다.
			// 따라서 이 구간에서 sendBuf를 다루는 것은 안전.

			// 이 함수 안에서 EINTR시 iocall 재시도하고 있음.
			SocketErrorCode r = m_fastSocket->IssueSend_NoCopy(sendBuf);

			// 송신 실패(ERROR_IO_PENDING은 이미 제외됐음)가 오면 소켓을 닫는다=>TCP issue recv or recv completion에서 소켓 닫힘이 감지되며, OnLeaveServer로 이어질 것임.
			if ( r != SocketErrorCode_Ok )
			{
				outNextProcess = ProcessType_CloseSocketAndProcessDisconnecting;
				return r;
			}
		}
		else
		{
			// UDP case

			// 			if (???) // 혼잡제어로 인해 지금은 보낼 때가 아니라면 <= R2.2로부터 병합할 때 여기 넣을 것!
			// 			{
			// 				AtomicCompareAndSwap32(1, 0, &m_sendIssued);
			// 				return;
			// 			}

			{
				CriticalSectionLock sendQueueLock(m_sendQueueCS, true);

				// 아무거나 하나 찾아서 보내도록 한다.
				// NOTE: non-block send와 달리, m_sendIssuedFragment에 뭔가가 아직 들어있어도 
				// 그것을 그냥 오버라이트 해버린다.
				if ( !(m_udpPacketFragBoard_needSendLock->PopAnySendQueueFilledOneWithCoalesce(*m_sendIssuedFragment, currTime, calledBySendCompletion)
					&& m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() > 0) )
				{
					// send brake 에 의해 송신큐가 안비었어도 못 보낸 경우가 있다.
					// 이러한 경우에는 send ready list에 다시 넣어야 한다.
					// CNetworkerLocalVars.sendIssuedPool로부터 꺼내온 임시 저장소에서 루프를 도는 것이기 때문에 AddOrSet을 호출한다고 해서 무한루프에 빠지지는 않으니 걱정말자.
					if ( m_udpPacketFragBoard_needSendLock->IsLastPopFragmentSurpressed_ValidAfterPopFragment() ) // 최근에 IssueSend를 호출했을 때, 보낼 패킷은 있었으나 송신이 제한된 것인지를 리턴한다. UDP의 경우 m_lastPopFragmentToSendSurpressed 를 리턴하는 함수이다.
					{
						m_owner->SendReadyList_Add(this, false);
						//m_owner->m_sendReadyList->AddOrSet(this);
					}

					return SocketErrorCode_Error;
				}

				if ( m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() == 0 )
				{
					return SocketErrorCode_Error;
				}
			}

			bufferLength = m_sendIssuedFragment->m_sendFragFrag.GetLength();

			// 이 함수 안에서 EINTR시 iocall 재시도하고 있음.
			SocketErrorCode se = m_fastSocket->IssueSendTo_NoCopy_TempTtl(m_sendIssuedFragment->m_sendFragFrag, m_sendIssuedFragment->m_sendTo, m_sendIssuedFragment->m_ttl);

			if ( se != SocketErrorCode_Ok )
			{
				// UDP는 보내기 실패해도 소켓 닫으면 안됨
				return SocketErrorCode_Error;
			}
		}

		// 성공
		//m_lastSendIssueWarningTime = 0;
		//m_lastSendIssuedTime = currTime;
		m_totalIssueSendBytes += bufferLength;
		m_totalIssueSendCount++;

		return SocketErrorCode_Ok;
	}

	// 수신을 걸 수 있으면 걸되, 에러 상황이면 중단 이벤트를 수행.
	void CSuperSocket::IssueRecv(bool firstRecv/*=false*/)
	{
		AssertIsNotLockedByCurrentThread(m_cs);

		if ( !firstRecv )
		{
			m_owner->AssertIsNotLockedByCurrentThread();
		}

		// 이미 닫힌 소켓이면 issue를 하지 않는다. 
		// 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다. 메모리도 긁으면서.
		if ( m_stopIoRequested )
		{
			int32_t p = AtomicCompareAndSwap32(0, 2, &m_recvIssued);
			assert(p == 0 || p == 2);
			return;
		}

		int32_t p = AtomicCompareAndSwap32(0, 1, &m_recvIssued);
		assert(p == 0); // 동일한 소켓에 대하여 복수의 쓰레드에서 이슈하지 않는다.

		// 기존 behavior를 유지하기 위해 남긴 if문.
		if ( p != 0 )
			return;

		CriticalSectionLock socketLock(m_cs, true);
		AssertIsLockedByCurrentThread(m_cs);

		// 랜선이 끊어진 상황을 시뮬.
		// 아예 issue 자체를 걸지 말도록 한다.
		// 재연결이 되면 새로운 소켓이 되므로 안전하다.
		if (m_turnOffSendAndReceive)
		{
			AtomicCompareAndSwap32(1, 0, &m_recvIssued); // 원상복구
			// 소켓은 close 시키지 않는다.
			return;
		}

		SocketErrorCode se;
		if ( m_socketType == SocketType_Tcp )
		{
			// 타임스탬핑
			se = m_fastSocket->IssueRecv(CNetConfig::TcpIssueRecvLength);
		}
		else
		{
			assert(m_socketType == SocketType_Udp);
			se = m_fastSocket->IssueRecvFrom(CNetConfig::UdpIssueRecvLength);
		}
		m_totalIssueReceiveCount++;
		if (!m_firstIssueReceiveTime)
			m_firstIssueReceiveTime = GetPreciseCurrentTimeMs();

		if ( se != SocketErrorCode_Ok )
		{
			if ( m_socketType == SocketType_Tcp )
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				if ( CloseSocketOnly() )
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Receive, 0, se);
					m_owner->ProcessDisconnecting(this, errorInfo);
				}
			}
			AtomicCompareAndSwap32(1, 0, &m_recvIssued); // 원상복구
			return;
		}

		//m_lastRecvissueWarningTime = 0;

	}

	// 내부: AcceptEx를 위한 new socket을 준비한 후에(있으면 그거 쓰고) AcceptEx를 호출한다.
	SocketErrorCode CSuperSocket::IssueAccept()
	{
		CriticalSectionLock sockLock(GetCriticalSection(), true);

		m_isListeningSocket = true;

		// 이미 닫힌 소켓이면 issue를 하지 않는다. 
		// 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다. 메모리도 긁으면서.
		if ( m_stopIoRequested )
		{
			int32_t p = AtomicCompareAndSwap32(0, 2, &m_recvIssued);
			assert(p == 0 || p == 2);
			return SocketErrorCode_Ok;
		}

		int32_t p = AtomicCompareAndSwap32(0, 1, &m_recvIssued);
		assert(p == 0); // 동일한 소켓에 대하여 복수의 쓰레드에서 이슈하지 않는다.

		// 기존 behavior를 유지하기 위해 남긴 if문.
		if ( p != 0 )
			return SocketErrorCode_Error;

		SocketErrorCode se = SocketErrorCode_Ok;

		// accept 받을 예비 소켓을 준비(없으면)
		if ( m_acceptCandidateSocket == NULL )
			m_acceptCandidateSocket = new CFastSocket(SocketType_Tcp);

	retry:
		// 이제야 만들어졌다. AcceptEx를 건다.
		SocketErrorCode e = m_fastSocket->AcceptEx(m_acceptCandidateSocket);
		switch ( e )
		{
		case SocketErrorCode_Ok:
		case SocketErrorCode_IoPending:	// iopending도 성공
			//assert(m_owner->m_TcpListenSocket->AcceptExIssued());
			// 제대로 걸었다.
			return e;
			//case SocketErrorCode_ConnectResetByRemote:			
		default:
			/* PATCH : 이렇게 하면 잘 되는지 체크해야 한다.
			* listen Socket을 의도적으로 닫지 않았는데 AcceptEx()가 EINVAL을 리턴하는 경우 재시도 하면 된다.
			* 단 확실히 하기 위해 bind(), listen()을 재 호출 한다(에러 값 무시).
			*/
			if (CNetConfig::ListenSocket_RetryOnInvalidArgError && !m_fastSocket->IsClosed())
			{
				SocketErrorCode bindErrorCode = m_fastSocket->Bind(m_fastSocket->GetBindAddress());
				if (bindErrorCode != SocketErrorCode_Ok && bindErrorCode != SocketErrorCode_InvalidArgument)
				{
					/* 운 없게도 그사이에 다른 곳에서 점유를 해버릴 수도 있다.
					이때는 소켓이 닫힌채로 콜러에게 알려주자.
					NOTE: bind가 이미 성공적으로 되어 있는데 또 bind를 호출할 경우,
					EINVAL(10022)가 리턴된다. 이때는 skip이 옳음.
					*/
					return bindErrorCode;
				}

				m_fastSocket->Listen();
				goto retry;

			}
			else
			{
				// 서버를 종료하는 상황이라서 여기에 왔을 수 있다. 해당 listen socket을 safe하게 닫도록 하자.
				// listen socket의 에러 핸들링은 Proud.ISuperSocketDelegate.ProcessDisconnecting에서 한다.
				///CloseSocketAndUnlockAndCallback(superSocketLock, IoEventType_Receive, 0, e);
				AtomicCompareAndSwap32(1, 0, &m_recvIssued); // 원상복구
				return e;
			}

			return SocketErrorCode_Ok;
		}
	}

	SocketErrorCode CSuperSocket::IssueConnectEx(String hostAddr, int hostPort)
	{
		SocketErrorCode r;

		// sendIsseud를 ConnectEx issued로도 같이 사용. 이래도 안전. epoll도 이러거덩.
		if ( AtomicCompareAndSwap32(0, 1, &m_sendIssued) == 0 )
		{
			assert(!m_isListeningSocket); // 중복 호출 금지
			m_isConnectingSocket = true;

		L1:
			r = m_fastSocket->ConnectEx(hostAddr, hostPort);
			// EINTR은 재시도해야. driver bug로 인해서 이런게 뜰 수 있으니.
			if ( r == SocketErrorCode_Intr )
				goto L1;

			if ( r != SocketErrorCode_IoPending && r != SocketErrorCode_Ok )
			{
				AtomicCompareAndSwap32(1, 0, &m_sendIssued);
				return r;
			}
			return SocketErrorCode_Ok;
		}

		// IssueConnectEx는 Atomic에 실패해서는 안되는 상황.
		throw Exception("AtomicCompareAndSwap32 in CSuperSocket::IssueConnectEx function was failed.");

		return SocketErrorCode_Error;
	}

#endif // _WIN32
}
