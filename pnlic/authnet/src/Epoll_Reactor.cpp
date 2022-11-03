/* ProudNet
 
 이 프로그램의 저작권은 넷텐션에게 있습니다.
 이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
 계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
 무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.
 
 ** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
 */

#include "stdafx.h"
#include "ioevent.h"
#include "../include/BasicTypes.h"
#include "../include/Exception.h"
//#include "../include/NetConfig.h"
#include "../include/sysutil.h"
#include "../include/MilisecTimer.h"
#include "FastSocket.h"
#include "Epoll_Reactor.h"
#include <new>
#include "SocketUtil.h"

#if defined(__linux__)
	#include <sys/eventfd.h>

#elif defined(__MACH__) || defined(__FreeBSD__ )
    #include <unistd.h>
    #include <fcntl.h>
#endif

#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#endif


namespace Proud
{
#ifndef _WIN32
	const int EpollWakeupIntervalMs = 1000;

	CIoReactorEventNotifier::CIoReactorEventNotifier()
    {
		// 초기화
		m_epollWorkCountPerSec = 0;
		m_epollWorkCount_Timer.SetIntervalMs(EpollWakeupIntervalMs);

        IoEventParam eventParam;

#if defined (__linux__)
		m_eventFD=-1;
#else
		m_eventFDs[0]=-1;
		m_eventFDs[1]=-1;
#endif

		// epoll or kqueue 생성
#if defined (__linux__)
		m_epfd = epoll_create(MaxPollCount);
#elif defined (__MACH__) || defined(__FreeBSD__) 
		m_epfd = kqueue();
#endif
		if (m_epfd == -1)
			throw Exception("I/O event poll creation failure: errno:%d", errno);

#if defined (__linux__)
		{
			//eventfd도 생성을 하자.
			/* 2014.07.03 : seungchul.lee
			 * eventfd에 EFD_NONBLOCK이 추가된 이유
			 * If the eventfd counter is zero at the time of the call to read(2), then the call either blocks until the counter
			 * becomes nonzero (at which time, the read(2) proceeds as described above) or fails with the error EAGAIN if the file descriptor has been made nonblocking.
			 http://man7.org/linux/man-pages/man2/eventfd.2.html
			 */
			m_eventFD = eventfd(0, EFD_NONBLOCK);
			if (m_eventFD == -1)
			{
				// On success, eventfd() returns a new eventfd file descriptor.  Onerror, -1 is returned and errno is set to indicate the error.
				throw Exception("event fd creation failure: return -1. errno:%d", errno);
			}

			eventParam.m_nFD = m_eventFD;
			eventParam.m_nOperation = EPOLL_CTL_ADD;
			eventParam.m_nOption = EPOLLIN | EPOLLET;
			// 그리고 eventfd는 epoll에 add되어야 한다. add할 때 user ptr is null로.
			eventParam.m_lpUdata = NULL;

			SetEvent(&eventParam);
		}

#else // eventfd가 없는 OS 들에 대해

		{
#if !defined(__ORBIS__) && !defined(__MARMALADE__) // socketpair()를 쓸 수 있는 OS에 대해

			// mac 에는 eventfd 에 준하는것이 없다.
			// 따라서 socketpair 를 사용하여 소켓 쌍을 만든 후, 하나만 kqueue 에 등록하고
			// 나머지는 write 용도로 사용한다.
			// 이렇게 이용하면 custom value event 처리가 가능
			// by. hyeonmin.yoon
			if(socketpair(PF_LOCAL, SOCK_STREAM, 0, m_eventFDs) == -1)
			{
				throw Exception("socketpair creation failure: return -1. errno:%d", errno);
			}
			EventFDs_SetNonBlocking();

#else // socketpair()가 없는 PS4, Marmalade 등등에 대해

			{
				// Marmalade나 PS4는 socketpair()가 없지만, 
				// 직접 두 socket을 만들면 된다.
				// 어차피 클라에서만 작동하는 OS이므로 아주 짧은 순간에 다른 TCP connection이
				// 그것도 localhost에 있는 것으로부터 연결 들어올 가능성은 희박하다.


				// TODO: 매우 짧은 순간에 두 socket을 만들어 서로 연결시키므로 빈도는 매우 작겠지만,
				// 해커가 두 연결이 맺어지기 전에 연결을 해버릴 가능성이 있을 수는 있다.
				// 게다가 PF_LOCAL에 비해서 훨씬 느리다.
				// 따라서 추후 PF_LOCAL로 바꾸고, PF_LOCAL을 못 쓰는 platform에서는
				// bind 주소로 any:0이 아니라 localhost:0으로 바꾸자.

				int listenSocket = socket(AF_INET, SOCK_STREAM, 0);

				if (listenSocket == -1)
				{
					throw Exception("socketpair creation failure 1: errno:%d", errno);
				}

				if (BindSocketIpv4(listenSocket, 0, 0) != 0)
				{
					throw Exception("socketpair creation failure 2: errno:%d", errno);
				}
					
				m_eventFDs[1] = socket(AF_INET, SOCK_STREAM, 0);

				if (m_eventFDs[1] == -1)
				{
					throw Exception("socketpair creation failure 3: errno:%d", errno);
				}

				EventFDs_SetNonBlocking();

				if(BindSocketIpv4(m_eventFDs[1], 0, 0) != 0)
				{
					throw Exception("socketpair creation failure 4: errno:%d", errno);
				}

				SocketErrorCode err = Socket_Listen(listenSocket);

				if(err!=SocketErrorCode_Ok)
				{
					throw Exception("socketpair creation failure 5: errno:%d", err);
				}

				// 한쪽이 나머지에 접속을 한다.
				AddrPort a = Socket_GetSockName(listenSocket);

				err = Socket_Connect(m_eventFDs[1], a);

				if((err != SocketErrorCode_Ok) && err != (CFastSocket::IsWouldBlockError(err)))
				{
					throw Exception("socketpair creation failure 6: errno:%d", err);
				}

				while (1)
				{
					m_eventFDs[0] = ::accept(listenSocket, 0, 0);

					if (m_eventFDs[0] > 0)
					{
						::close(listenSocket);
						break;
					}

					Proud::Sleep(1);
				}

				EventFDs_SetNonBlocking();
			}

#endif // socketpair()가 없는 PS4, Marmalade 등등에 대해

			// 이제 socket pair가 준비되었다. socketpair()가 지원되건 안되건.

#ifndef __MARMALADE__ // simple poll밖에 못쓰는 OS를 제외하고

			// 첫번째 socket을 이벤트 노티를 받을 디스크립터로 지정
			// EV_CLEAR 옵션을 넣어야 엣지 트리거로 작동한다.
			eventParam.m_nFD = m_eventFDs[0];
			eventParam.m_nFilter = EVFILT_READ;
			eventParam.m_nFlags = EV_ADD | EV_CLEAR;
			eventParam.m_lpUdata = NULL;
			// kqueue epoll edge trigger equivalent 구글링하면 뭔가 나올지도.
			// http://www.kegel.com/c10k.html#nb.kqueue 

			// read하는 쪽의 socket에 대해서 read event만 등록한다.
			SetEvent(&eventParam);
#endif
		}

#endif // eventfd 대신 socket pair를 쓰는 OS에 대해

    }
    
	CIoReactorEventNotifier::~CIoReactorEventNotifier()
    {
		if (m_epfd != -1)
		{
			close(m_epfd);
		}

#if defined(__linux__)
		// eventfd를 제거.
		if (m_eventFD != -1)
		{
			//eventfd도 close해야.
			close(m_eventFD);
		}
#else
		// socketpair를 제거.
        if (m_eventFDs[0] != -1)
        {
            //eventfd도 close해야.
            close(m_eventFDs[0]);
        }
        if (m_eventFDs[1] != -1)
        {
            //eventfd도 close해야.
            close(m_eventFDs[1]);
        }
#endif
    }
    
#if defined (__linux__) || defined (__MACH__) || defined(__FreeBSD__) // epoll or kqueue가 있는 OS에 대해
	
	//  이벤트를 하나 등록한다./*int nFD, int nOperation, int nOption, void* pUdata*/
	void CIoReactorEventNotifier::SetEvent(IoEventParam *lpParam)
    {
		IoEventParam *lpEpollParam = lpParam;
		if (!lpEpollParam)
		{
			assert(0);
			throw Exception("No IoReactorEvent::SetEvent(null)!");
		}

// #ifdef _DEBUG
// 		bool r1 = fd_is_valid(lpEpollParam->m_nIdent);
// 		bool r2 = fd_is_valid(m_epfd);
// #endif
        
        CriticalSectionLock lock(m_cs, true);

#if defined (__linux__)
        struct epoll_event evt;
        evt.events = lpEpollParam->m_nOption;
        evt.data.ptr = lpEpollParam->m_lpUdata;
#elif defined (__MACH__) || defined(__FreeBSD__)
		struct kevent change; 
		change.ident = lpEpollParam->m_nFD;
		change.filter = lpEpollParam->m_nFilter;
		change.flags = lpEpollParam->m_nFlags;
		change.fflags = 0;
		change.data = 0;
		change.udata = lpEpollParam->m_lpUdata;
#endif 

		int ret = 0;
		// kevent에 등록
#if defined (__linux__)
        ret = ::epoll_ctl(m_epfd, lpEpollParam->m_nOperation, lpEpollParam->m_nFD, &evt);
#elif defined (__MACH__) || defined(__FreeBSD__)
		ret = ::kevent(m_epfd, &change, 1, NULL, 0, NULL);
#endif 
        if(ret < 0)
		{
			StringA text = StringA::NewFormat("IoEventReactor::SetEvent failed. errno:%d\n", errno);
			assert(0);
			throw Exception(text);
		}
    }

#endif // epoll or kqueue가 있는 OS에 대해


	// 모아진 이벤트를 커널 이벤트 큐에 등록하고 모아진 이벤트들을 처리한다.
	void CIoReactorEventNotifier::Poll(CIoEventStatusArray &ret, int maxPollCount, uint32_t maxWaitTimeMs)
	{
		
		
		



		if (m_epollWorkCount_Timer.IsTimeToDo(GetPreciseCurrentTimeMs()))
		{
			m_epollWorkCountPerSec = 0;
		}

#if defined(__MACH__) || defined(__FreeBSD__) || defined(__linux__)
		// epoll or kqueue인 경우. 내부는 서로 대동소이.
		Poll_epollOrKqueue(ret,maxPollCount,maxWaitTimeMs);
#else
		// simple poll인 경우. eventfd가 없으니 최대한 짧은 주기로 폴링.
		Poll_simplePoll(ret,maxPollCount, 1);
#endif
	}

	// socket을 epoll or kqueue에 add한다.
	// events: 어떤 event에 대해서 io event를 받을 것인지 지정. level or edge trigger 여부도 같이.
	void CIoReactorEventNotifier::AssociateSocket( CFastSocket* socket, bool isEdgeTrigger )
	{
		IoEventParam EpollParam;

		CriticalSectionLock lock(m_cs, true);

		EpollParam.m_lpUdata = socket;

		// EPOLLPRI : OOB 데이터 존재할 경우
		// EPOLLRDHUP 커널 버전 2.6.17 이후에 가능. 연결이 종료되거나 Half-close 가 진행된 상황.
		// 엣지트리거에서 테스트 케이스 작성할때 유용하다는데 아직까지 쓿일은 없을듯. 보류.
#ifdef __linux__
		EpollParam.m_nFD = socket->m_socket;
		EpollParam.m_nOperation = EPOLL_CTL_ADD;
		EpollParam.m_nOption = EPOLLIN|EPOLLOUT;
		if(isEdgeTrigger)
			EpollParam.m_nOption |= EPOLLET;
		SetEvent(&EpollParam);
#endif // __linux__

#if defined(__MACH__) || defined(__FreeBSD__)
		EpollParam.m_nFD = socket->m_socket;
		EpollParam.m_nFilter = EVFILT_READ;
		EpollParam.m_nFlags = EV_ADD;
		if(isEdgeTrigger)
			EpollParam.m_nFlags |= EV_CLEAR;	
		SetEvent(&EpollParam);
		// kqueue는, write에 대해서도 추가로 kqueue에 등록해 주어야.
		EpollParam.m_nFilter = EVFILT_WRITE;
		SetEvent(&EpollParam);
#endif
		if(socket->m_associatedIoQueue == this)
			return;

		if(socket->m_associatedIoQueue != NULL)
			throw Exception(_PNT("이미 다른 ioqueue에 assoc되어있음!"));

		if (socket->GetIoEventContext() == NULL)
			throw Exception(_PNT("먼저 socket의 completion context를 세팅해야 함!"));

		// marmalade 버전에서는 딱히 ioqueue라는게 없다. 그냥 grand kernel event noti 방식이니까.
		// 그래서 바로 아래와 같이 링크드 리스트에 연계만 시키면 쫑.
		socket->m_associatedIoQueue = this;
		m_associatedSockets.Add(socket, socket);
	}

#if defined (__linux__) || defined(__MACH__) || defined(__FreeBSD__)
	// 이미 epoll에 add된 socket의 event io 발생 조건을 edge trigger로 바꾼다.
	void CIoReactorEventNotifier::AssociatedSocket_ChangeToEdgeTrigger(CFastSocket* socket)
    {
		IoEventParam EpollParam;

		CriticalSectionLock lock(m_cs, true);

		if (!m_associatedSockets.ContainsKey(socket))
			throw Exception(_PNT("먼저 socket을 AssociateSocket해야 함!"));

		EpollParam.m_lpUdata = socket->GetIoEventContext();

		// EPOLLPRI : OOB 데이터 존재할 경우
		// EPOLLRDHUP 커널 버전 2.6.17 이후에 가능. 연결이 종료되거나 Half-close 가 진행된 상황. 엣지트리거에서 테스트 케이스 작성할때 유용하다는데 아직까지 쓿일은 없을듯 보류
#ifdef __linux__
		EpollParam.m_nFD = socket->m_socket;
		EpollParam.m_nOperation = EPOLL_CTL_MOD;
		EpollParam.m_nOption = EPOLLIN|EPOLLOUT|EPOLLET;
		SetEvent(&EpollParam);
#endif // __linux__

#if defined(__MACH__) || defined(__FreeBSD__)
		EpollParam.m_nFD = socket->m_socket;
		EpollParam.m_nFilter = EVFILT_READ;
		EpollParam.m_nFlags = EV_ADD|EV_CLEAR;
		SetEvent(&EpollParam);

		// write에 대해서도 추가로 kqueue modify를 해 주어야.
		EpollParam.m_nFilter = EVFILT_WRITE;
		SetEvent(&EpollParam);
#endif 

		// 이제 Level Trigger가 아니므로 false로 변경.
		socket->m_isLevelTrigger = false;
	}
#endif

	void CIoReactorEventNotifier::RemoveSocket(CFastSocket* socket)
	{
		CriticalSectionLock lock(m_cs, true);
		m_associatedSockets.RemoveKey(socket);
	}

#if defined(__MACH__) || defined(__FreeBSD__) || defined(__linux__)
	// caller 주석 참고
	void CIoReactorEventNotifier::Poll_epollOrKqueue(CIoEventStatusArray &ret, int maxPollCount, uint32_t maxWaitTimeMs)
	{
		maxPollCount = min(maxPollCount, MaxPollCount);
		maxPollCount = max(0, maxPollCount);


L1:
		int eventCount = 0;

#if defined(__linux__)
		// wait를 본격 기다린다. 이 기간은 device time임에 주의.
		epoll_event polledEvents[MaxPollCount];	
		eventCount = ::epoll_wait(m_epfd, polledEvents, MaxPollCount, maxWaitTimeMs);

#elif defined(__MACH__) || defined (__FreeBSD__)
		struct kevent polledEvents[MaxPollCount];
		timespec timeoutSpec;
		timeoutSpec.tv_sec = maxWaitTimeMs / 1000;
		timeoutSpec.tv_nsec = (maxWaitTimeMs % 1000) * 1000 * 1000;
        eventCount = ::kevent(m_epfd, NULL, 0, polledEvents, MaxPollCount, &timeoutSpec);
#endif
		// wait 끝. 
		ret.SetCount(0);

		if (eventCount < 0)
        {
			// EINTR이면 중간에 signal 등이 왔다는 뜻이다. 
			// socket send or recv의 경우 그냥 무시하면 되지만 
			// epoll-wait의 경우 다음에 또 시도해도 되는 것이므로 안전하게 루프를 그냥 나가자.
			if (errno != EINTR)
			{
				NTTNTRACE("epoll or kqueue wait is failed. errno:%d", errno);
				assert(0);
			}
			return;
        }

		// 여기서부터 lock을 걸어도 된다. syscall이 딱히 있는 것도 아니고 CPU time뿐이니까.
		// 사소한데 리스크 두지 말자구.
		CriticalSectionLock lock(m_cs, true);
		CFastSocket* spi;

		// 각 이벤트를 event status list에 담자.
		for (int i = 0; i < eventCount; i++)
        {
			CIoEventStatus status;
#if defined(__linux__)
			spi = (CFastSocket*)polledEvents[i].data.ptr;
			int fd = polledEvents[i].data.fd;
#elif defined(__MACH__) || defined (__FreeBSD__)
			spi = (CFastSocket*)polledEvents[i].udata;
#endif 

			if (spi != NULL)
            {
				// 특정 socket에 대한 이벤트를 받았다.
				// validation check
				if (!m_associatedSockets.ContainsKey(spi))
					continue;

				// socket이 level trigger이면 edge trigger로 재등록
				if (spi->m_isLevelTrigger) 
				{
#if defined(__linux__) || defined(__MACH__) || defined(__FreeBSD__)
					AssociatedSocket_ChangeToEdgeTrigger(spi);
#endif
				}
					
				// socket io event이다. 걸맞게 채우자.
				// 다행히 epoll,kqueue는 대동소이하다.
				status.m_type = IoEventType_ReferFlags;
				status.m_fastSocket = spi;
				status.m_errorCode = SocketErrorCode_Ok;
				status.m_completedDataLength = 0;
				status.m_eventFlags = 0;

#if defined(__linux__)
				uint32_t eventFlags = polledEvents[i].events;
				if(eventFlags & EPOLLIN) status.m_eventFlags |= IoFlags_In;
				if(eventFlags & EPOLLOUT) status.m_eventFlags |= IoFlags_Out;
				if(eventFlags & EPOLLERR) status.m_eventFlags |= IoFlags_Error;
				if(eventFlags & EPOLLHUP) status.m_eventFlags |= IoFlags_Hangup;
#elif defined(__MACH__) || defined(__FreeBSD__)
				uint32_t eventFlags = polledEvents[i].filter;
				if (eventFlags & EVFILT_READ) status.m_eventFlags |= IoFlags_In;
				if (eventFlags & EVFILT_WRITE) status.m_eventFlags |= IoFlags_Out;
				if (eventFlags & EV_ERROR) status.m_eventFlags |= IoFlags_Error;
				if (eventFlags & EV_EOF) status.m_eventFlags |= IoFlags_Hangup;
#else
#endif
			}
			else
			{
				// 아무 socket과도 연관없는 이벤트가 왔다. custom value라던지.
				// 즉 eventfd에 +1을 해서 여기에 신호가 왔다.
				// custom event를 저장하도록 하자.
				status.m_type = IoEventType_ReferCustomValue;

				status.m_fastSocket = NULL;
				status.m_errorCode = SocketErrorCode_Ok;
				status.m_completedDataLength = 0;
				status.m_eventFlags = 0;
            }
			ret.Add(status); // 생성자 함수 외에서는 손대지 않은 나머지 인자들도 모두 복사하기 위해.
        }
    }
#else
	void CIoReactorEventNotifier::Poll_simplePoll(CIoEventStatusArray &ret, int maxPollCount, uint32_t maxWaitTimeMs)
	{
		/* 각 socket을 돌면서 send or recv available을 체크해서 나머지를 처리한다.
		참고: marmalade 자체는 모든 socket api가 non block 방식. 즉 즉시 리턴이 보장되며 대기를 해야 할 경우
		무조건 would block error가 발생한다. */
		{
			CriticalSectionLock lock(m_cs, true);

			// 메모리 할당해제 반복을 절약
			m_tempPollFdList.SetCapacity(m_associatedSockets.GetCount() + 1);
			m_tempPollFdList.SetCount(0);

			// 감시할 socket들을 목록으로 만들어 poll()에 던질 준비를.
			// 각 remote 및 udp socket에 대해 루프를 돌아도 되나 kqueue방식과 동일하게 가려면 이렇게 해야. 
			for(CFastMap2<CFastSocket*, CFastSocket*, int>::iterator i = m_associatedSockets.begin();i!=m_associatedSockets.end();i++)
			{
				/* 각 socket에 대해서 루프를 돌면서 io를 시행하기 위한 미리 수집.
				event 핸들러 루틴 안에서 socket close를 하면 ioqueue와의 assoc이 상실해버림.
				따라서 iter를 그냥 돌면 m_associatedSockets와의 iteration이 상실할 경우 댕글링으로 이어짐.
				따라서 미리 list들을 모두 모아놓고 그것을 처리하도록 해야 안전. */
				CFastSocket* s = i->GetSecond();

				pollfd pfd;
				pfd.fd = s->m_socket;
				// POLLOUT은 검사 안함. 
				// edge trigger도 아니기 때문에 항상 signalled로 나올 것이므로 불필요. 
				// 게다가, marmalade 리얼폰에서는 POLLOUT이 나와야 하는 상황에서도 안나오는 버그가 있다!
				pfd.events = POLLIN; 
				pfd.revents = 0;

				m_tempPollFdList.Add(pfd);
			}

			// custom event를 감지하는 loopback socket도 추가.
			@@@;// 아직 테스트 안되었다. 테스트하자!
			{
				pollfd pfd;
				pfd.fd = GetReadOnlyEventFD();
				// POLLOUT은 검사 안함. 
				// edge trigger도 아니기 때문에 항상 signalled로 나올 것이므로 불필요. 
				// 게다가, marmalade 리얼폰에서는 POLLOUT이 나와야 하는 상황에서도 안나오는 버그가 있다!
				pfd.events = POLLIN; 
				pfd.revents = 0;

				m_tempPollFdList.Add()
			}




			// 아래부터 device time. 따라서 unlock.
		}

		// 이제 목록을 근거로 io available이 된 socket들이 있을 때까지 기다린다.
		// 여기서는 unlock 상태이어야 함!
		::poll(m_tempPollFdList.GetData(), m_tempPollFdList.GetCount(), maxWaitTimeMs); // poll은 milisec timeout.

		ret.SetCount(0);
		{
			// device time 끝. 잠금해도 된다.
			CriticalSectionLock lock(m_cs, true);

			// simple poll에서는 eventfd도 없고, 각 socket에 대해서 signal이 왔는지 
			// 확인하는 것도 정상적으로 되지 않는다.
			// 따라서 모든 socket에 대해 non block op를 모두 시도해야 하고 
			// eventfd 여부와 상관없이 custom value queue도 체크해야 한다.
			// 따라서 여기서는 모든 socket과 eventfd에 대해 넣어야 한다.
			// 비효율적이지만 불가피.
			CIoEventStatus status;
			status.m_type = IoEventType_ReferFlags;
			status.m_eventFlags = IoFlags_In | IoFlags_Out;
			// NOTE: m_tempPollSocketList를 사용하지 않는다.
			// unlock-lock 구간 사이에서 타 스레드에 의해 목록 자체에 변화가 있을 수 있으니.
			for (CFastMap2<CFastSocket*, CFastSocket*, int>::iterator i = m_associatedSockets.begin();
				i != m_associatedSockets.end(); i++)
			{
				status.m_fastSocket = i->GetSecond();
				ret.Add(status);
			}

			// custom value event가 있건 없건 일단 추가한다. 
			// 그래야 위의 주석처럼 custom event queue에서 꺼내 처리하는 루틴이 작동하니까.
			status.m_fastSocket = NULL;
			status.m_type = IoEventType_ReferCustomValue;
			ret.Add(status);
		}

	}


#endif // (defined(__MACH__) || defined(__linux__))


#ifndef __linux__ // socket pair를 쓰는 OS에 대해
	// socket pair를 non-blocking으로.
	void CIoReactorEventNotifier::EventFDs_SetNonBlocking()
	{
		// non-block 소켓으로 지정
		for (int i = 0; i < 2; i++)
		{
			int flag = fcntl(m_eventFDs[i], F_GETFL, 0);
			fcntl(m_eventFDs[i], F_SETFL, flag | O_NONBLOCK);
		}
	}

#endif


#endif // non _WIN32
}
