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
#include "ReceivedMessageList.h"


// Send Queue를 확인하기 위한 Test 용도로 사용하는 변수
bool Proud::CSuperSocket::m_test_isSendQueueLock = false;


namespace Proud
{

#if defined (_WIN32)
	void CSuperSocket::OnSocketIoCompletion(
		void* param_shared_from_this_ptr,
		CIoEventStatus &comp)
	{
		const shared_ptr<CSuperSocket>& param_shared_from_this = *((const shared_ptr<CSuperSocket>*)param_shared_from_this_ptr);
		assert(param_shared_from_this.get() == this);

		switch (comp.m_type)
		{
		case IoEventType_Send:
			ProcessOverlappedSendCompletion(param_shared_from_this, comp);
			break;
		case IoEventType_Receive:
			ProcessOverlappedReceiveCompletion(param_shared_from_this, comp);
			break;
		}
	}


	// overlapped i/o send completion이 발생한 것을 처리한다. iocp 전용.
	void CSuperSocket::ProcessOverlappedSendCompletion(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		CIoEventStatus &comp)
	{
		assert(param_shared_from_this.get() == this);

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

		// true이면, next issue send call을 수행한다.
		bool issueSendOnNeed = false;

		// true이면, TCP circuit이 끊어졌다는 뜻. 그러면 next issue syscall이 무용지물이다.
		bool shutdownTcpSocket = false;

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
			case ProcessType_OnConnectStillInProgress:				
			{
				assert(false); // 윈도에서는 여기 올 일이 없다!
				break;
			}
			case ProcessType_OnConnectSuccess:
			{
				m_owner->OnConnectSuccess(param_shared_from_this);
				issueSendOnNeed = true;
				break;
			}
			case ProcessType_OnConnectFail:
			{
				m_owner->OnConnectFail(param_shared_from_this, e);
				break;
			}
			case ProcessType_CloseSocketAndProcessDisconnecting:
			{
				assert(m_socketType == SocketType_Tcp);
				if (RequestStopIo())
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo,
											   IoEventType_Send,
											   comp.m_completedDataLength,
											   comp.m_errorCode);
					m_owner->ProcessDisconnecting(param_shared_from_this, errorInfo);
				}
				shutdownTcpSocket = true;

				break;
			}
			default:
			{
				assert(nextProc == ProcessType_None);
				break;
			}
		}

		if ( issueSendOnNeed )
		{
			// 이 루틴이 실행중일떄는 아직 owner가 건재함을 전제한다.
			socketLock.Lock();
			IssueSendOnNeedAndUnlock(param_shared_from_this, true, socketLock);
			assert(socketLock.IsLocked() == false);
		}
		else
		{
			if(!shutdownTcpSocket)
			{
				// 송신 과정이 실패했다던지, 보낼 데이터가 없으면, issue send를 안 건다.
				// 이때는 not working으로 전환해야 한다.
				int32_t p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_sendIssued);
				_pn_unused(p);
				assert(p == IoState_Working);
			}
			else
			{
				// TCP circuit shutdown 상황이다.
				// next issue는 반드시 실패하므로 여기서 차단한다.
				// 이게 없으면 SuperSocket의 m_sendIssued or m_recvIssued가 영원히 Working 상태로 남는 버그가 있다.
				int32_t p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_sendIssued);
				_pn_unused(p);
				assert(p == IoState_Working);
			}
		}
	}

	// overlapped i/o receive completion이 발생한 것을 처리한다. iocp 전용.
	void CSuperSocket::ProcessOverlappedReceiveCompletion(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		CIoEventStatus &comp)
	{
		assert(param_shared_from_this.get() == this);

		m_owner->AssertIsNotLockedByCurrentThread();
		// 반드시 이것이 있어야 OnMessageReceived 안에서 main lock 시 데드락이 안 생긴다.
		// lock oreder가 main then socket이니까.
		Proud::AssertIsNotLockedByCurrentThread(m_cs);

		CriticalSectionLock socketLock(m_cs, true);

		assert(m_recvIssued == IoState_Working);

		ProcessType nextProc = GetNextProcessType_AfterRecv(comp);
		switch (nextProc)
		{
		case ProcessType_OnMessageReceived:
		{
			RefreshLastReceivedTime();

			POOLED_LOCAL_VAR(CReceivedMessageList, msgList);

			ErrorInfoPtr errorinfo;
			ProcessType p = ExtractMessage(comp, msgList, errorinfo);

			socketLock.Unlock();
			// 이것이 확인 안된채로 아래 main lock이 내장된 콜백에 돌입하면 데드락 생김
			AssertIsNotLockedByCurrentThread(GetCriticalSection());

			if (p == ProcessType_OnMessageReceived)
			{
				assert(errorinfo == NULL);
				m_owner->OnMessageReceived(comp.m_completedDataLength,
					msgList,
					param_shared_from_this);
			}
			else if (p == ProcessType_CloseSocketAndProcessDisconnecting)
			{
				// extract 결과, 데이터가 깨져있다. 즉 더 이상 수신 처리를 할 수 없다.
				// TCP shutdown과 준하는 처리를 하자.

				assert(errorinfo != NULL);
				assert(m_socketType == SocketType_Tcp);
				if (RequestStopIo())
				{
					m_owner->ProcessDisconnecting(param_shared_from_this, *errorinfo);
				}
				
				// 이 상태에서는 더 이상 issue-recv를 해봤자 즉시 GQCS with failure가 뜬다. 그럼 또 issue recv 걸고 삽질 반복.
				// 그냥 여기서 정리해 버리자.
				// 게다가 이러한 경우가 왜 발생하는지 모르곘지만 이를 막기 위해서도 여기서 이걸 한다.
				// TCP circuit shutdown => issue-recv returns 0 length => GQCS returns 0 length => ... => issue-recv returns 0 length => close socket => (GQCS returns 0 length가 안옴(!) )
				// 위 if 구문에 넣지 말 것! remote쪽에서만 TCP close를 한 경우도 처리해야 하니까.
				// 이렇게 해 놓으면 아래 IssueRecv()는 skip된다.
				// **이게 없으면 SuperSocket의 m_sendIssued or m_recvIssued가 영원히 Working 상태로 남는 버그가 있다.**
				AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_recvIssued);

			}
			else if (p == ProcessType_EnqueWarning)
			{
				assert(errorinfo != NULL);
				m_owner->EnqueWarning(errorinfo);
			}
			else if (p == ProcessType_EnquePacketDefragWarning)
			{
				assert(errorinfo != NULL);
				m_owner->EnquePacketDefragWarning(param_shared_from_this, comp.m_receivedFrom, errorinfo->m_comment.GetString());
			}
			else
			{
				assert(p == ProcessType_None);
			}

			IssueRecv(param_shared_from_this);
		}
		break;
		case ProcessType_OnAccept:
		{
			assert(m_isListeningSocket);
			assert(m_socketType == SocketType_Tcp);

			// accepted socket을 가져온다. 그리고 issue-accept를 다음에 건다.
			// 이렇게 해야 socket unlock을 1회만 할 수 있으니까.
			shared_ptr<CSuperSocket> acceptedSocket = m_acceptedSocket;
			AddrPort acceptedSocket_tcpLocalAddr;
			AddrPort acceptedSocket_tcpRemoteAddr;

			if (acceptedSocket)
			{
				acceptedSocket_tcpLocalAddr = m_acceptedSocket_tcpLocalAddr;
				acceptedSocket_tcpRemoteAddr = acceptedSocket->m_fastSocket->GetPeerName();
			}
			m_acceptedSocket.reset(); // detach

			// 이제 issue-accept을 건다.
			SocketErrorCode e = IssueAccept(true);
			if (e != SocketErrorCode_Ok &&
				e != SocketErrorCode_IoPending)
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				if (RequestStopIo())
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Receive, 0, e);
					m_owner->ProcessDisconnecting(param_shared_from_this, errorInfo);
				}

//				int32_t p = AtomicCompareAndSwap32(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
//				assert(p == IoState_Working);
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

			if (RequestStopIo())
			{
				ErrorInfo errorInfo;
				BuildDisconnectedErrorInfo(errorInfo,
					IoEventType_Receive,
					comp.m_completedDataLength,
					comp.m_errorCode);
				m_owner->ProcessDisconnecting(param_shared_from_this, errorInfo);
			}

			// 이 상태에서는 더 이상 issue-recv를 해봤자 즉시 GQCS with failure가 뜬다. 그럼 또 issue recv 걸고 삽질 반복.
			// 그냥 여기서 정리해 버리자.
			// 게다가 이러한 경우가 왜 발생하는지 모르곘지만 이를 막기 위해서도 여기서 이걸 한다.
			// TCP circuit shutdown => issue-recv returns 0 length => GQCS returns 0 length => ... => issue-recv returns 0 length => close socket => (GQCS returns 0 length가 안옴(!) )
			// 위 if 구문에 넣지 말 것! remote쪽에서만 TCP close를 한 경우도 처리해야 하니까.
			// 이게 없으면 TCP 연결 해제 상황에서 m_sendIssued or m_recvIssued가 영원히 Working으로 남는 버그가 있다.
			AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
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
	void CSuperSocket::IssueSendOnNeedAndUnlock(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		bool calledBySendCompletion, CriticalSectionLock& socketLock)
	{
		assert(param_shared_from_this.get() == this);

		AssertIsLockedByCurrentThread(m_cs);
		assert(socketLock.GetRecursionCount() == 1);
		int32_t p;
		SocketErrorCode e;

		// 소켓이 건재하고 송신 미이슈 상태에 한해
		if (StopIoRequested())
		{
			if(calledBySendCompletion)
				AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_sendIssued);
			else
				AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_NoMoreWorkGuaranteed, &m_sendIssued);

			goto ret;
		}

		// garbaged에 들어갔으면 send를 하지 말아야 한다. i/o는 살아있지만 송신은 안하고 수신은 다 받는 족족 버려야 한다.
		if (m_timeToGarbage != 0)
		{
			goto ret;
		}

		if (calledBySendCompletion)
		{
			// completion 상황을 처리중이면 working 상태이다.
			assert(m_sendIssued == IoState_Working);
		}
		else
		{
			// timer or AddToSendQueue에 의한 호출이다.
			p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_Working, &m_sendIssued);

			if (p != IoState_NotWorking)
			{
				// 아직 issue가 진행중이면 또 issue하면 안되므로
				goto ret;
			}
		}

		ProcessType nextProc;
		e = IssueSendOnNeed_internal(param_shared_from_this, calledBySendCompletion, nextProc);

		if ( e != SocketErrorCode_Ok )
		{
			if ( nextProc == ProcessType_CloseSocketAndProcessDisconnecting )
			{
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				assert(m_socketType == SocketType_Tcp);
				if ( RequestStopIo() )
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Send, 0, e);
					m_owner->ProcessDisconnecting(param_shared_from_this, errorInfo);
				}
			}
			else
			{
				assert(nextProc == ProcessType_None);
			}

			// 원상복구
			if (nextProc == ProcessType_CloseSocketAndProcessDisconnecting)
			{
				p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_sendIssued);
				assert(p == IoState_Working);
			}
			else
			{
				p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_sendIssued);
				assert(p == IoState_Working);
			}
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

	SocketErrorCode CSuperSocket::IssueSendOnNeed_internal(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		bool calledBySendCompletion, ProcessType& outNextProcess)
	{
		assert(param_shared_from_this.get() == this);

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
						m_owner->SendReadyList_Add(param_shared_from_this, m_forceImmediateSend);
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
	void CSuperSocket::IssueRecv(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		bool firstRecv/*=false*/)
	{
		assert(param_shared_from_this.get() == this);

		AssertIsNotLockedByCurrentThread(m_cs);

		if ( !firstRecv )
		{
			m_owner->AssertIsNotLockedByCurrentThread();
		}

		// 이미 닫힌 소켓이면 issue를 하지 않는다.
		// 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다. 메모리도 긁으면서.
		if (StopIoRequested())
		{
			if(!firstRecv)
			{
				int32_t p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
				_pn_unused(p);
				assert(p == IoState_Working || p == IoState_NoMoreWorkGuaranteed);
			}
			else
			{
				int32_t p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
				_pn_unused(p);
				assert(p == IoState_Working || p == IoState_NoMoreWorkGuaranteed);
			}

			return;
		}

		if (firstRecv)
		{
			/* Q: caller에서 not working 체크 안하나요?
			A: recv의 경우 working <-> not working 변화를 하지 않고, 항상 working 상태를 유지한 채로 issue recv <-> recv completion 순환을 합니다.
			만약 recv가 중간에 실패하면 그제서야 다른 것으로 바뀌게 되어 있습니다.
			방어 코딩 취지로, not working 상태가 되는 순간을 완전히 없앴습니다. 
			그래야 socket을 그만 쓰는 상황 직전(StopIoAcked)까지 socket을 지우는 일이 전혀 없게 하니까요.  */

			// 기존 behavior를 유지하기 위해 남긴 if문.
			if (AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_Working, &m_recvIssued) != IoState_NotWorking)
			{
				return;
			}
		}
		else
		{
			// completion 중이므로 기존에 이미 working이어야.
			assert(m_recvIssued == IoState_Working);
		}

		CriticalSectionLock socketLock(m_cs, true);
		AssertIsLockedByCurrentThread(m_cs);

		// 랜선이 끊어진 상황을 시뮬.
		// 아예 issue 자체를 걸지 말도록 한다.
		// 재연결이 되면 새로운 소켓이 되므로 안전하다.
		if (m_turnOffSendAndReceive)
		{
			AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_recvIssued); // 원상복구


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

				if (RequestStopIo())
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo, IoEventType_Receive, 0, se);
					m_owner->ProcessDisconnecting(param_shared_from_this, errorInfo);
				}
			}
			AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_recvIssued); // 원상복구
		}
		else // IssueRecv syscall 성공.
		{
		}

		//m_lastRecvissueWarningTime = 0;

	}

	// 내부: AcceptEx를 위한 new socket을 준비한 후에(있으면 그거 쓰고) AcceptEx를 호출한다.
	SocketErrorCode CSuperSocket::IssueAccept(bool calledBySendCompletion)
	{
		CriticalSectionLock sockLock(GetCriticalSection(), true);

		m_isListeningSocket = true;

		// 이미 닫힌 소켓이면 issue를 하지 않는다.
		// 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다. 메모리도 긁으면서.
		if (StopIoRequested())
		{
			int32_t p;
			if (calledBySendCompletion)
			{
				p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
				assert(p == IoState_Working);
			}
			else
			{
				p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_NoMoreWorkGuaranteed, &m_recvIssued);
				assert(p == IoState_NotWorking || p == IoState_NoMoreWorkGuaranteed);
			}

			return SocketErrorCode_Ok;
		}

		if(!calledBySendCompletion)
		{
			int32_t p = AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_Working, &m_recvIssued);

			// 이미 issue accept을 했으면 건너뛴다.
			if (p != IoState_NotWorking)
			{
				assert(0);
				return SocketErrorCode_Error;
			}
		}
		else
		{
			// completion 핸들링중 콜백이다. 아직 working을 유지하고 있어야 한다.
			assert(m_recvIssued == IoState_Working);
		}

		//SocketErrorCode se = SocketErrorCode_Ok;

		// accept 받을 예비 소켓을 준비(없으면)
		if (!m_acceptCandidateSocket)
		{
			SocketCreateResult r = CFastSocket::Create(SocketType_Tcp);
			if(r.socket)
				m_acceptCandidateSocket = r.socket;
			else
			{
				return SocketErrorCode_Error;
			}
		}

	retry:
		// 이제야 만들어졌다. AcceptEx를 건다.
		SocketErrorCode e = m_fastSocket->AcceptEx(m_acceptCandidateSocket);
		switch (e)
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
			if (CNetConfig::ListenSocket_RetryOnInvalidArgError && !StopIoRequested())
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
				AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_recvIssued); // 원상복구
				return e;
			}

			return SocketErrorCode_Ok;
		}
	}

	SocketErrorCode CSuperSocket::IssueConnectEx(AddrPort hostAddrPort)
	{
		SocketErrorCode r;

		// sendIsseud를 ConnectEx issued로도 같이 사용. 이래도 안전. epoll도 이러거덩.
		if (AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_NotWorking, IoState_Working, &m_sendIssued) == IoState_NotWorking)
		{
			assert(!m_isListeningSocket); // 중복 호출 금지
			m_isConnectingSocket = true;

		L1:
			r = m_fastSocket->ConnectEx(hostAddrPort);
			// EINTR은 재시도해야. driver bug로 인해서 이런게 뜰 수 있으니.
			if ( r == SocketErrorCode_Intr )
				goto L1;

			if ( r != SocketErrorCode_IoPending && r != SocketErrorCode_Ok )
			{
				AtomicCompareAndSwapEnum<IoState, int32_t>(IoState_Working, IoState_NotWorking, &m_sendIssued);
				return r;
			}
			return SocketErrorCode_Ok;
		}

		// IssueConnectEx는 Atomic에 실패해서는 안되는 상황.
		throw Exception("AtomicCompareAndSwap32 in CSuperSocket::IssueConnectEx function was failed.");
	}

#endif // _WIN32
}
