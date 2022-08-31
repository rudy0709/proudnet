﻿/* ProudNet

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
#include "SuperSocket.h"
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

		// epoll or kqueue 생성
#if defined (__linux__)
		m_epfd = epoll_create(MaxPollCount);
#elif defined (__MACH__) || defined(__FreeBSD__)
		m_epfd = kqueue();
#endif
		if (m_epfd == -1)
		{
			Tstringstream part;
			part << "I/O event poll creation failure: errno:" << errno;
			throw Exception(part.str().c_str());
		}


	}


	CIoReactorEventNotifier::~CIoReactorEventNotifier()
	{
		if (m_epfd != -1)
		{
			close(m_epfd);
		}
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
		//		bool r1 = fd_is_valid(lpEpollParam->m_nIdent);
		//		bool r2 = fd_is_valid(m_epfd);
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
		if (ret < 0)
		{
			// 2016.3.25 linux Server 종료 시 정상종료가 안되는 문제로 인해 주석 처리(Simple Sample을 기준으로 테스트 결과 주석 처리해도 정상 동작함)
			/*StringA text = StringA::NewFormat("IoEventReactor::SetEvent failed. errno:%d\n", errno);
			assert(0);
			throw Exception(text);*/
		}
	}

#endif // epoll or kqueue가 있는 OS에 대해


	// 모아진 이벤트를 커널 이벤트 큐에 등록하고 모아진 이벤트들을 처리한다.
	// CompletionPort::GetQueuedCompletionStatusEx와 사용법과 주의사항이 동일하므로 거기 설명 꼭 참고하라.
	void CIoReactorEventNotifier::Poll(CIoEventStatusList &ret, uint32_t maxWaitTimeMs)
	{
		ret.Clear();

		if (m_epollWorkCount_Timer.IsTimeToDo(GetPreciseCurrentTimeMs()))
		{
			m_epollWorkCountPerSec = 0;
		}

#if defined(__MACH__) || defined(__FreeBSD__) || defined(__linux__)
		// epoll or kqueue인 경우. 내부는 서로 대동소이.
		Poll_epollOrKqueue(ret, maxWaitTimeMs);
#else
		// simple poll인 경우. eventfd가 없으니 최대한 짧은 주기로 폴링.
		Poll_simplePoll(ret, 1);
#endif
	}

	// socket을 epoll or kqueue에 add한다.
	// events: 어떤 event에 대해서 io event를 받을 것인지 지정. level or edge trigger 여부도 같이.
	void CIoReactorEventNotifier::AssociateSocket(const shared_ptr<CSuperSocket>& socket, bool isEdgeTrigger)
	{
		// 행여나 방어 코드. weak_ptr의 암묵적 캐스팅 후에 행여나.
		if (!socket)
		{
			assert(0);
			return;
		}

		IoEventParam EpollParam;

		CriticalSectionLock lock(m_cs, true);

		EpollParam.m_lpUdata = socket.get();

		// EPOLLPRI : OOB 데이터 존재할 경우
		// EPOLLRDHUP 커널 버전 2.6.17 이후에 가능. 연결이 종료되거나 Half-close 가 진행된 상황.
		// 엣지트리거에서 테스트 케이스 작성할때 유용하다는데 아직까지 쓿일은 없을듯. 보류.
#ifdef __linux__
		EpollParam.m_nFD = socket->m_fastSocket->m_socket;
		EpollParam.m_nOperation = EPOLL_CTL_ADD;
		EpollParam.m_nOption = EPOLLIN | EPOLLOUT;
		if (isEdgeTrigger)
			EpollParam.m_nOption |= EPOLLET;
		SetEvent(&EpollParam);
#endif // __linux__

#if defined(__MACH__) || defined(__FreeBSD__)
		EpollParam.m_nFD = socket->m_fastSocket->m_socket;
		EpollParam.m_nFilter = EVFILT_READ;
		EpollParam.m_nFlags = EV_ADD;
		if (isEdgeTrigger)
			EpollParam.m_nFlags |= EV_CLEAR;
		SetEvent(&EpollParam);
		// kqueue는, write에 대해서도 추가로 kqueue에 등록해 주어야.
		EpollParam.m_nFilter = EVFILT_WRITE;
		SetEvent(&EpollParam);
#endif
		if (socket->m_associatedIoQueue_accessByAssociatedSocketsOnly == this)
			return;

		if (socket->m_associatedIoQueue_accessByAssociatedSocketsOnly != NULL)
			throw Exception(_PNT("이미 다른 ioqueue에 assoc되어있음!"));

		socket->m_associatedIoQueue_accessByAssociatedSocketsOnly = this;

		// 값 세팅
		socket->m_isLevelTrigger = !isEdgeTrigger;

		m_associatedSockets.Add(socket, socket);
	}

#if defined (__linux__) || defined(__MACH__) || defined(__FreeBSD__)
	// 이미 epoll에 add된 socket의 event io 발생 조건을 edge trigger로 바꾼다.
	void CIoReactorEventNotifier::AssociatedSocket_ChangeToEdgeTrigger(const shared_ptr<CSuperSocket>& socket)
	{
		IoEventParam EpollParam;

		CriticalSectionLock lock(m_cs, true);

		if (!m_associatedSockets.ContainsKey(socket))
			throw Exception(_PNT("먼저 socket을 AssociateSocket해야 함!"));

		EpollParam.m_lpUdata = socket.get();

		// EPOLLPRI : OOB 데이터 존재할 경우
		// EPOLLRDHUP 커널 버전 2.6.17 이후에 가능. 연결이 종료되거나 Half-close 가 진행된 상황. 엣지트리거에서 테스트 케이스 작성할때 유용하다는데 아직까지 쓿일은 없을듯 보류
#ifdef __linux__
		EpollParam.m_nFD = socket->m_fastSocket->m_socket;
		EpollParam.m_nOperation = EPOLL_CTL_MOD;
		EpollParam.m_nOption = EPOLLIN | EPOLLOUT | EPOLLET;
		SetEvent(&EpollParam);
#endif // __linux__

#if defined(__MACH__) || defined(__FreeBSD__)
		/* Adds the event to the kqueue.  Re-adding an existing event
		will modify the parameters of the original event, and not
		result in a duplicate entry[1].  Adding an event automati-cally automatically
		cally enables it, unless overridden by the EV_DISABLE flag.
		라고 하지만, 실제로는 delete 후 add를 해주어야 level to edge trigger 전환이 된다.
		*/
		// delete
		EpollParam.m_nFD = socket->m_fastSocket->m_socket;
		EpollParam.m_nFilter = EVFILT_READ;
		EpollParam.m_nFlags = EV_DELETE;
		SetEvent(&EpollParam);

		// write에 대해서도 추가로 kqueue modify를 해 주어야.
		EpollParam.m_nFilter = EVFILT_WRITE;
		SetEvent(&EpollParam);

		// 그리고 add
		EpollParam.m_nFD = socket->m_fastSocket->m_socket;
		EpollParam.m_nFilter = EVFILT_READ;
		EpollParam.m_nFlags = EV_ADD | EV_CLEAR;  // [1]
		SetEvent(&EpollParam);

		// write에 대해서도 추가로 kqueue modify를 해 주어야.
		EpollParam.m_nFilter = EVFILT_WRITE;
		SetEvent(&EpollParam);
#endif

		// 이제 Level Trigger가 아니므로 false로 변경.
		socket->m_isLevelTrigger = false;
	}
#endif

	/* epoll에서 socket을 제거한다.
	associated socket list 구조체에서도 제거한다.
	epoll 자체에서도 delete를 해서 더 이상 이벤트가 안오게 한다.

	참고: 이 함수를, epoll_wait를 하는 스레드에서 실행하건
	외 스레드에서 실행하건, 이미 remove했음에도 불구하고 이벤트가
	오는 경우가 있다.
	그래서 그것을 valid check하기 위해 associated socket list가 따로 있다.
	따라서, 이 함수는 아무 스레드에서나 호출해도 된다. */
	bool CIoReactorEventNotifier::RemoveSocket(const SocketPtrAndSerial& key)
	{
		CriticalSectionLock lock(m_cs, true);

		weak_ptr<CSuperSocket> socket_weak;
		if (m_associatedSockets.TryGetValue(key, socket_weak)) // 유효한 socket 객체를 찾으면
		{
			shared_ptr<CSuperSocket> socket(socket_weak.lock()); // 유효한 socket 객체를 찾으면
			if (socket)
			{
				// epoll 커널 객체에서의 제거
				IoEventParam EpollParam;
#ifdef __linux__
				EpollParam.m_nFD = socket->m_fastSocket->m_socket;
				EpollParam.m_nOperation = EPOLL_CTL_DEL;
				EpollParam.m_nOption = EPOLLIN | EPOLLOUT;
				SetEvent(&EpollParam);
#endif // __linux__

#if defined(__MACH__) || defined(__FreeBSD__)
				EpollParam.m_nFD = socket->m_fastSocket->m_socket;
				EpollParam.m_nFilter = EVFILT_READ;
				EpollParam.m_nFlags = EV_DELETE;
				SetEvent(&EpollParam);

				// write에 대해서도 추가로 kqueue modify를 해 주어야.
				EpollParam.m_nFilter = EVFILT_WRITE;
				SetEvent(&EpollParam);
#endif
			}
		}

		// 유효하건 무효하건 내부 데이터 자체 목록에서는 제거해야.
		return m_associatedSockets.RemoveSocketByKey(key);
	}

#if defined(__MACH__) || defined(__FreeBSD__) || defined(__linux__)
	// caller 주석 참고
	void CIoReactorEventNotifier::Poll_epollOrKqueue(CIoEventStatusList &ret, uint32_t maxWaitTimeMs)
	{
		ret.Clear();

		// wait를 본격 기다린다. 이 기간은 device time임에 주의.
#if defined(__linux__)
		epoll_event polledEvents[MaxPollCount];
		int eventCount = ::epoll_wait(m_epfd, polledEvents, MaxPollCount, maxWaitTimeMs);

#elif defined(__MACH__) || defined (__FreeBSD__)
		struct kevent polledEvents[MaxPollCount];

		timespec timeoutSpec;
		timeoutSpec.tv_sec = maxWaitTimeMs / 1000;
		timeoutSpec.tv_nsec = (maxWaitTimeMs % 1000) * 1000 * 1000;

		int eventCount = ::kevent(m_epfd, NULL, 0, polledEvents, MaxPollCount, &timeoutSpec);
#endif

		// 여기서부터 lock을 걸어도 된다. syscall이 딱히 있는 것도 아니고 CPU time뿐이니까.
		// 사소한데 리스크 두지 말자구.
		CriticalSectionLock lock(m_cs, true);

		weak_ptr<CSuperSocket> spi;

		// 각 이벤트를 event status list에 담자.
		for (int i = 0; i < eventCount; i++)
		{
			CIoEventStatus status;
			CSuperSocket* spi0;
#if defined(__linux__)
			spi0 = (CSuperSocket*)polledEvents[i].data.ptr;
#elif defined(__MACH__) || defined (__FreeBSD__)
			spi0 = (CSuperSocket*)polledEvents[i].udata;
#endif

			// validation check (spi0 => spi)
			shared_ptr<CSuperSocket> socket;
			if(m_associatedSockets.TryGetValue(spi0, spi)
				&& (socket = spi.lock()) != nullptr) // 이미 사라진 socket이 아니어야 한다.
			{
				// 특정 socket에 대한 이벤트를 받았다.

				// socket이 level trigger이면 edge trigger로 재등록
				// 단, TCP connecting socket이면 예외. 자세한 것은 #iOS_ENOTCONN_issue 에.
				// TCP connected이면 이 이벤트가 반복해서 올 것이다. 이벤트 누락 문제는 자연 해결됨.
				if (socket->m_isLevelTrigger && !socket->m_isConnectingSocket)
				{
#if defined(__linux__) || defined(__MACH__) || defined(__FreeBSD__)
					AssociatedSocket_ChangeToEdgeTrigger(socket);
#endif
				}

				// socket io event이다. 걸맞게 채우자.
				// 다행히 epoll,kqueue는 대동소이하다.
				status.m_type = IoEventType_ReferFlags;
				status.m_superSocket = socket;
				status.m_errorCode = SocketErrorCode_Ok;
				status.m_completedDataLength = 0;
				status.m_eventFlags = 0;

#if defined(__linux__)
				uint32_t eventFlags = polledEvents[i].events;
				if (eventFlags & EPOLLIN) status.m_eventFlags |= IoFlags_In;
				if (eventFlags & EPOLLOUT) status.m_eventFlags |= IoFlags_Out;
				if (eventFlags & EPOLLERR) status.m_eventFlags |= IoFlags_Error;
				if (eventFlags & EPOLLHUP) status.m_eventFlags |= IoFlags_Hangup;
#elif defined(__MACH__) || defined(__FreeBSD__)
				uint32_t eventFlags = polledEvents[i].filter;
				// EVFILT_READ,EVFILT_WRITE는 -1,-2,...값이다. 즉 bit mask가 아니다. 따라서 위에처럼 하면 안되고 아래처럼 해야 한다.
				// 타 kqueue 사용예시도 이런식으로 짜져있다.
				if (eventFlags == EVFILT_READ) status.m_eventFlags |= IoFlags_In;
				if (eventFlags == EVFILT_WRITE) status.m_eventFlags |= IoFlags_Out;
				if (eventFlags == EV_ERROR) status.m_eventFlags |= IoFlags_Error;
				if (eventFlags == EV_EOF) status.m_eventFlags |= IoFlags_Hangup;
#else
#endif

				// 이제는 eventfd같은 것을 안 쓴다. 따라서 socket event가 아니면 poll list에 넣지 말자.
				ret.Add(status);
			}
		}
	}
#else

	// epoll or iocp 사용 불가능 플랫폼용.
	// Q : 1024개를 훌쩍 넘을 수 있지 않나요?
	// A : simple poll 모델을 쓰는 NetClient가 설마 1024개씩이나 P2P 연결을 하는 경우는 없다고 봅시다.
	void CIoReactorEventNotifier::Poll_simplePoll(CIoEventStatusList &ret, uint32_t maxWaitTimeMs)
	{
		/* 각 socket을 돌면서 send or recv available을 체크해서 나머지를 처리한다.*/
		{
			CriticalSectionLock lock(m_cs, true);

			// 메모리 할당해제 반복을 절약
			m_tempPollFdList.SetCapacity(m_associatedSockets.GetCount() + 1);
			m_tempPollFdList.SetCount(0);

			// 감시할 socket들을 목록으로 만들어 poll()에 던질 준비를.
			// 각 remote 및 udp socket에 대해 루프를 돌아도 되나 kqueue방식과 동일하게 가려면 이렇게 해야.
			for (AssociatedSockets::iterator i = m_associatedSockets.begin(); i != m_associatedSockets.end(); i++)
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
				pfd.events = POLLIN;
				pfd.revents = 0;

				m_tempPollFdList.Add(pfd);
			}

			// custom event를 감지하는 loopback socket도 추가.

			UNTESTED_YET; // 아래 코드는 아직 테스트 안되었다. 테스트하자!
			{
				pollfd pfd;
				pfd.fd = GetReadOnlyEventFD();
				// POLLOUT은 검사 안함.
				// edge trigger도 아니기 때문에 항상 signalled로 나올 것이므로 불필요.
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
			for (AssociatedSockets::iterator i = m_associatedSockets.begin();
				i != m_associatedSockets.end(); i++)
			{
				status.m_superSocket = i->GetSecond();
				ret.Add(status);
			}

			// custom value event가 있건 없건 일단 추가한다.
			// 그래야 위의 주석처럼 custom event queue에서 꺼내 처리하는 루틴이 작동하니까.
			ForceAddCustomEvent(ret);
		}
	}

#endif // (defined(__MACH__) || defined(__linux__))

#endif // non _WIN32
}
