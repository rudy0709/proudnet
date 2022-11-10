/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

//////////////////////////////////////////////////////////////////////////
// epoll,kqueue,simple poll 즉 reactor에서만 유효한 모듈


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
#include "PooledObjects.h"
#include "RunOnScopeOut.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

namespace Proud
{
#if !defined(_WIN32)
	// epoll같은 reactor model에서, non block socket으로 이미 설정된 socket에 대한 send syscall을 수행한다.
	// 다 처리하고 나면 socket lock을 해제한다. 
	// caller가 멀티캐스트를 위해 try-lock을 성공시킨 상태에서 호출되므로 거기에 본 함수가 맞춰 움직인다.
	// comp: 여기에 실행 결과가 채워진다.
	// calledByIoEvent: true이면 caller가 i/o event에서라는 뜻. 
	//    이럴때는 coalesce를 할 때가 아직 안되었더라도 송신큐에서 꺼내서 바로 처리한다.
	// return: true이면 더 이상 할게 없음 즉 EWOULDBLOCK이나 error라는 상황이다. 
	//    false이면 아직 더 할일이 있으므로 caller는 이 함수를 또 호출해야 한다.
	bool CSuperSocket::NonBlockSendAndUnlock(bool calledByIoEvent, CriticalSectionLock& socketLock, CIoEventStatus &comp)
	{
		assert(socketLock.GetRecursionCount() == 1); // lock은 이미 lock 건 상태로 오는 것이므로!
		AssertIsLockedByCurrentThread(m_cs);

#if !defined(__GNUC__) // GCC ARM을 QNX 에뮬레이터에서 돌리면 여기서 왜 실패하지?
		// nonblock으로 작동하려면 넌블럭 설정이 선결되어야 함.
		assert(!m_fastSocket->GetBlockingMode());
#endif

		AssertIsLockedByCurrentThread(m_cs);

		comp.m_completedDataLength = 0;
		
		bool finished; // no more work to do?
#ifdef _DEBUG
		bool udpSendTried=false;
#endif
		/* NOTE: IOT 툴체인에서는 c++11을 못쓴다. 따라서 RunOnScopeOut을 못 쓴다.
		아래 주석은 미래를 위해 보관하자.
		아래 주석을 살리게 되면 아래 RunOnScopeOut1을 지우고 아래 구문으로 바꿔버리자.

		// 함수가 리턴할 때 반드시 unlock을 해야 하므로.
		RunOnScopeOut socketUnlocker([&](){ 
			socketLock.Unlock(); 
			AssertIsNotLockedByCurrentThread(m_cs); 
		});
		*/
		class RunOnScopeOut1
		{
			bool m_run;
		public:
			CriticalSectionLock* m_socketLock;
			CriticalSection* m_cs;
			CSuperSocket* m_this;

			RunOnScopeOut1()
			{
				m_run=false;
			}

			void Run()
			{
				if(!m_run)
				{
					m_socketLock->Unlock();
					AssertIsNotLockedByCurrentThread(*m_cs); 
					m_run=true;
				}
			}

			~RunOnScopeOut1() { Run(); }
		};

		RunOnScopeOut1 socketUnlocker;
		socketUnlocker.m_cs=&m_cs;
		socketUnlocker.m_socketLock=&socketLock;

		// 이미 닫힌 소켓이면 루프를 나가자.
		if (m_fastSocket->IsClosed())
		{
			return true;
		}

		if (m_isConnectingSocket)
		{
			// select(),poll()은 플랫폼마다 사용법과 behavior가 서로 다르고, 일부 플랫폼은 그나마 제대로 얻지도 못한다.
			// 따라서 zero-send를 통해 연결이 진행중인지 연결이 성사됐는지 체크함으로 일관화하자.

			// 어차피 아래 ProcessSendOrConnectEvent 에서 ConnectExComplete를 호출합니다.
			// 따라서 여기에서는 그냥 아무것도 안하는 것이 맞을 것 같아 여기는 텅 비움.
			// 그래도 Linux와 Windows Connect 테스트 결과에서는 특별한 문제는 없는 것으로 확인됨.
		}
		else if (m_socketType == SocketType_Tcp)
		{
			CFragmentedBuffer sendBuf;

			// PeekSendBuf() 안 NormalizePacketQueue 에서 length 가 구해져서 순서 변경.
			// 보낼 양을 정한다.
			{
				CriticalSectionLock sendLock(m_sendQueueCS, true);

				// Windows가 아닌 시스템에서 IOV_MAX개수 이상의 fragment를 
				// sendmsg에서 한번에 처리 불가능하기 때문에 한번에 뽑는 개수를 제한한다.
#if defined(__MARMALADE__)
				m_sendQueue_needSendLock->PeekSendBuf(sendBuf);
#else
				m_sendQueue_needSendLock->PeekSendBuf(sendBuf, IOV_MAX - 1);

				assert(sendBuf.Array().GetCount() < IOV_MAX);
#endif
				// Q: 위 peek만 했는데요, non-block send 성공한 만큼 pop은 어디서 하나요?
				// A: 아래 GetNextProcessType_AfterSend 입니다.

				int brakedSendAmount = m_sendQueue_needSendLock->GetLength(); // TCP는 혼잡 제어가 내장되어 있어서 송신량 제어가 불필요
				if (brakedSendAmount == 0)
				{ 
					// 꺼낼게 없다. 그냥 나가도록 하자.
					// 콜러에게도 '다 완료됐음'을 노티하자.
					return true;
				}
			}

			// send queue critsec이 비보호 상태지만, 타 스레드에서 중간에 push를 할 지언정 pop or delete를 하지는 않는다.
			// 따라서 이 구간에서 sendBuf를 다루는 것은 안전.

			/* send()가 에러를 낼 때까지는 socket buffer에 계속 넣을 수 있음을 의미.
			따라서 더 이상 넣을 것이 없거나 send()가 에러를 낼 때까지 반복해야.
			would block이 뜬 경우, socket send buffer가 빈공간이 부족해서 그런 것이나, 조만간 send buffer 공간이 늘어날때마다 event가 발생할 것임.
			따라서 그때 재시도하면 됨. */

			do
			{
				// TCP임! bufferLength와 doneLength는 서로 다른 값일 수 있다는걸 간과하면 안됨!
				comp.m_errorCode = m_fastSocket->Send(sendBuf, &comp.m_completedDataLength);
				// EINTR은 driver에 의해 발생할 수도 있으므로 이러한 경우 재시도해야.
			} while (comp.m_errorCode == SocketErrorCode_Intr);

			// ENOTCONN는 edge trigger를 안 쓰는 경우, 아직 TCP 연결을 맺기 전의 socket의 경우 나는 에러다. 따라서 무시해줘야.
			if (comp.m_errorCode == EWOULDBLOCK || comp.m_errorCode == ENOTCONN || comp.m_errorCode == EAGAIN)
			{
				return true;
			}

			// 송신 실패(EAGAIN 제외)가 오면 소켓을 닫는다=>TCP issue recv or recv completion에서 소켓 닫힘이 감지되며, OnLeaveServer로 이어질 것임.
			if (comp.m_errorCode != SocketErrorCode_Ok) // EAGAIN 검사 안하면 연결이 멀쩡한데도 디스나는 사태로 이어짐.
			{
				String text;
				text.Format(_PNT("non-blocked send failed. Error:%d\n"), comp.m_errorCode);

				if(!m_fastSocket->IsClosed())
				{
					NTTNTRACE(StringT2A(text));
				}
				//m_owner->OnSocketWarning(m_socket, text);
				socketUnlocker.Run();
				AssertIsNotLockedByCurrentThread(m_cs);

				assert(m_socketType == SocketType_Tcp);
				if ( CloseSocketOnly() )
				{
					ErrorInfo errorInfo;
					BuildDisconnectedErrorInfo(errorInfo,
						IoEventType_Send, 
						comp.m_completedDataLength, 
						comp.m_errorCode, 
						text);
					m_owner->ProcessDisconnecting(this, errorInfo);
				}

				// 에러 났으니까 caller는 또 이걸 호출해야 하는 상황 아니다.
				return true;
			}
		}// else if (m_socketType == SocketType_Tcp)
		else
		{
			// UDP case
			{
				CriticalSectionLock sendLock(m_sendQueueCS, true);
				// UDP socket 안의 송신버퍼 잔여량을 아직 모른다. 따라서 일단 non-block send를 때려본다. 
				// UDP의 경우 송신되는게 all or nothing이다. 따라서 EAGAIN(=EWOULDBLOCK)이 뜨면 하나도 보내지지 않았다는 뜻.
				// 이럴 때를 위해 보내려는 데이터는 계속 갖고 있다가 제대로 보내지는게 확인되면 그때서야 보낼 데이터를 다음 것으로 교체해야 한다.
				if (m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() == 0)
				{
#ifdef PN_LOCK_OWNER_SHOWN
					AssertSendQueueAccessOk();
#endif
					bool r = m_udpPacketFragBoard_needSendLock->PopAnySendQueueFilledOneWithCoalesce(*m_sendIssuedFragment, GetPreciseCurrentTimeMs(), calledByIoEvent);
					if (!r)
					{
						// 위에서 못 가져왔는데 여기가 채워져 있으면 안됨. 검사하자.
						assert(m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() == 0);

						// 더 이상 할 일이 없으므로
						return true;
					}
				}

				// 더이상 보낼게 없으면 중지
				if (m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() == 0)
				{
					// 더 이상 할 일이 없으므로
					return true;
				}

#if !defined(__MARMALADE__)
				// sendmsg IOV_MAX 제한 검사
				assert(m_sendIssuedFragment->m_sendFragFrag.GetSegmentCount() < IOV_MAX);
#endif
			}

			comp.m_errorCode = m_fastSocket->SendTo_TempTtl(m_sendIssuedFragment->m_sendFragFrag, 
				m_sendIssuedFragment->m_sendTo, 
				m_sendIssuedFragment->m_ttl, 
				&comp.m_completedDataLength);

			// 더 이상 socket에서 송신큐에 담을 수 없거나 기타 에러가 있을 때 중지
			if (comp.m_errorCode != SocketErrorCode_Ok) // would block도 포함
			{
				// 더 이상 할 일이 없으므로
				return true;
			}

			{
				// 성공적으로 UDP socket의 send buffer에 넣었다. 
				// 따라서, 비워버린다. 그래야 다음 루프 턴에서 보낼 UDP 패킷을 새로 조립하니까. [1]
				CriticalSectionLock sendlock(m_sendQueueCS, true);
				m_sendIssuedFragment->m_sendFragFrag.Array().SetCount(0);
#ifdef _DEBUG
				udpSendTried = true;
#endif
			}
		}//else

		// send 완료.
		SocketErrorCode e;
		ProcessType nextProc = GetNextProcessType_AfterSend(comp, e);

		// per-socket unlock 후 유저 콜백 수행.
		socketUnlocker.Run();
		AssertIsNotLockedByCurrentThread(m_cs);

		switch ( nextProc )
		{
		case ProcessType_OnMessageSent:
		{
			m_owner->OnMessageSent(comp.m_completedDataLength, m_socketType);
			// 성공적으로 송신했다. caller는 다시 이 함수를 호출할 필요가 있다.
			// 따라서 not return yet.
			break;
		}
		case ProcessType_OnConnectSuccess:
		{
			m_owner->OnConnectSuccess(this);
			// TCP 연결 성공. 이 함수를 반복 호출할게 아니다. 따라서 true를 리턴.
			// 더 이상 할 일이 없으므로
			break;
		}
		case ProcessType_OnConnectFail:
		{
			m_owner->OnConnectFail(this, e);
			// 에러니까
			// 더 이상 할 일이 없으므로
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
			// 에러니까
			// 더 이상 할 일이 없으므로
			return true;
		}
		break;
		default:
		{
			assert(nextProc == ProcessType_None);
		}
		break;
		}

		if ( comp.m_completedDataLength > 0 )
		{
			// 위[1]을 안 지나갔는데 여기를 오는 상황이 있을리 없다.	
#ifdef _DEBUG
			assert(m_socketType==SocketType_Tcp || udpSendTried);
#endif
			m_totalIssueSendBytes += comp.m_completedDataLength;
		}

		// caller는 더 해야 할 일이 있을 수 있다. EWOULDBLOCK 등이 리턴된 것이 아니므로.
		return false; 
	}

	void CSuperSocket::NonBlockRecvAndProcessUntilWouldBlock(CIoEventStatus &comp)
	{
		AssertIsNotLockedByCurrentThread(m_cs);

#if !defined(__GNUC__) // GCC ARM을 QNX 에뮬레이터에서 돌리면 여기서 왜 실패하지?
		assert(!m_fastSocket->GetBlockingMode());
#endif

		while (true)
		{
			AssertIsNotLockedByCurrentThread(m_cs);
			CriticalSectionLock socketLock(m_cs, true);

			// 이미 닫힌 소켓이면 루프를 나가자.
			if (m_fastSocket->IsClosed())
				break;

			if (m_timeToGarbage != 0)
				break;

			if (m_isListeningSocket)
			{
				if (m_acceptCandidateSocket == NULL)
				{
					// 리눅스에서는 accept socket을 바로 할당받는다.
					m_acceptCandidateSocket = m_fastSocket->Accept(comp.m_errorCode);
				}
			}
			else if (m_socketType == SocketType_Tcp)
			{
				comp.m_errorCode = m_fastSocket->Recv(CNetConfig::TcpIssueRecvLength);
			}
			else
			{
				comp.m_errorCode = m_fastSocket->RecvFrom(CNetConfig::UdpIssueRecvLength);
			}

			// socket send queue에 공간이 부족. 나중에 재시도하자. edge trigger이므로 다시 기회가 올 것이다.
			// ENOTCONN는 edge trigger를 안 쓰는 경우, 아직 TCP 연결을 맺기 전의 socket의 경우 나는 에러다. 따라서 무시해줘야.
			if (comp.m_errorCode == EWOULDBLOCK || comp.m_errorCode == ENOTCONN || comp.m_errorCode == EAGAIN)
				break;

			if (comp.m_errorCode != SocketErrorCode_Ok) // would block도 포함
				break;

			// 위 Recv or RecvFrom은 마치고 나면 실제 받은 크기만큼으로 recv buffer를 세팅하므로.
			comp.m_completedDataLength = m_fastSocket->GetRecvBufferLength();

/*//3003!!
			if(comp.m_completedDataLength)
				m_receiveCount++;*/

			ProcessType nextProc = GetNextProcessType_AfterRecv(comp);
			switch (nextProc)
			{
			case ProcessType_OnAccept:
			{
				// accepted socket을 가져온다. 그리고 issue-accept를 다음에 건다.
				// 이렇게 해야 socket unlock을 1회만 할 수 있으니까.
				CSuperSocket* acceptedSocket = m_acceptedSocket;
				AddrPort acceptedSocket_tcpLocalAddr;
				AddrPort acceptedSocket_tcpRemoteAddr;

				if (acceptedSocket)
				{
					acceptedSocket_tcpLocalAddr = m_acceptedSocket_tcpLocalAddr;
					acceptedSocket_tcpRemoteAddr = m_acceptedSocket_tcpRemoteAddr;
				}
				m_acceptedSocket = NULL; // detach

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
			break;
			case ProcessType_OnMessageReceived:
			{
				RefreshLastReceivedTime();

				comp.m_receivedFrom.FromNative(m_fastSocket->GetRecvedFrom());

				POOLED_ARRAY_LOCAL_VAR(CReceivedMessageList, msgList);
				ErrorInfoPtr errorinfo;

				ProcessType p = ExtractMessage(comp, msgList, errorinfo);

				socketLock.Unlock();
				// 이것이 확인 안된채로 아래 main lock이 내장된 콜백에 돌입하면 데드락 생김
				AssertIsNotLockedByCurrentThread(GetCriticalSection());

				if ( p == ProcessType_OnMessageReceived )
				{
					assert(errorinfo == NULL);
				/*	3003!! int rmi3003Index = Array_GetMatchIndex(
						m_fastSocket->GetRecvBufferPtr(),
						m_fastSocket->GetRecvBufferLength(),
						Rmi3003Patterns::Instance);*/

					m_owner->OnMessageReceived(comp.m_completedDataLength,
						m_socketType,
						msgList,
						this);
				}
				else if ( p == ProcessType_CloseSocketAndProcessDisconnecting )
				{
					assert(errorinfo != NULL);
					assert(m_socketType == SocketType_Tcp);
					if ( CloseSocketOnly() )
					{
						m_owner->ProcessDisconnecting(this, *errorinfo);
					}
				}
				else if ( p == ProcessType_EnqueWarning )
				{
					assert(errorinfo != NULL);
					m_owner->EnqueWarning(errorinfo);
				}
				else if ( p == ProcessType_EnquePacketDefragWarning )
				{
					assert(errorinfo != NULL);
					m_owner->EnquePacketDefragWarning(this, comp.m_receivedFrom, errorinfo->m_comment);
				}
				else
				{
					assert(p == ProcessType_None);
				}

			}
			break;
			case ProcessType_CloseSocketAndProcessDisconnecting:
			{
				// NOTE: EWOULDBLOCK인 경우 여기 안오고 저 위의 break 문으로 나가게 됨.
				socketLock.Unlock();
				AssertIsNotLockedByCurrentThread(m_cs);

				assert(m_socketType == SocketType_Tcp);
				if ( CloseSocketOnly() )
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
	}

	void CSuperSocket::OnSocketIoAvail(CIoEventStatus &comp)
	{
		CUseCounter counter(*this);

		// 		/* 블러킹 없는 수신 및 수신 데이터 처리를 한다. 받은 데이터가 없다면 그냥 무시.
		// 		이 함수는 io available signal이 오면 콜됨.
		// 		io call <-> event wait 횟수를 줄이기 위해,
		// 		이 함수 안에서 would block error가 날 때까지 recv or recvfrom를 반복해야. */
		// 		virtual void NonBlockRecvAndProcessUntilWouldBlock() = 0;
		// 
		// 		/* 보낼게 있다면, 블러킹 없는 송신을 한다.
		// 		이 함수는 io available signal이 오면 콜됨.
		// 		io call <-> event wait 횟수를 줄이기 위해, 특히 marmalade에서는 heartbeat에서만 io call을 할 기회가 없기 때문에,
		// 		이 함수 안에서 would block error가 날 때까지 send를 반복해야. */
		// 		virtual void NonBlockSendUntilWouldBlock() = 0;

		if ((comp.m_eventFlags & IoFlags_In)
			|| (comp.m_eventFlags & IoFlags_Hangup)
			|| (comp.m_eventFlags & IoFlags_Error))
		{
			// 받을 수 있는 용량 최대한 recv를 건다. (event로 받은 data는 무시함. 잠깐 사이에 더 늘어난게 있을터이니.)
			// error or end flag에 대해서도 이 안에서 처리해서 close socket으로 유도된 것이다.
			NonBlockRecvAndProcessUntilWouldBlock(comp);
		}

		if (comp.m_eventFlags & IoFlags_Out)
		{
			while(true)
			{
				CriticalSectionLock socketLock(m_cs, true); // 첫번째 lock 상태.
				// NOTE: 아래 함수가 socketLock을 unlock한다.
				bool finished = NonBlockSendAndUnlock(true, socketLock, comp);

				if(finished)
					break;
			}
		}
	}

	// non-block socket으로 mode change 후 connect를 수행한다.
	SocketErrorCode CSuperSocket::SetNonBlockingAndConnect(const String& hostName, uint16_t serverPort)
	{
		assert(!m_isConnectingSocket); // 중복 호출하면 안됨
		m_fastSocket->SetBlockingMode(false);
		m_isConnectingSocket = true;
		
		return m_fastSocket->Connect(hostName, serverPort);
	}

#endif 

}
