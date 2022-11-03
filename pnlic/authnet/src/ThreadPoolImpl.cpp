/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/CoInit.h"
//
#include "SuperSocket.h"
#include "ThreadPoolImpl.h"
#include "ReusableLocalVar.h"
#include "Epoll_Reactor.h"
#include "ioevent.h"
#include "STLUtil.h"
#include "RunOnScopeOut.h"
//#include "LowContextSwitchingLoop.h"

#if defined(__MACH__)
#include <unistd.h>
#endif

#ifdef GET_SEND_SYSCALL_TOTAL_COUNT
#include "../UtilSrc/DebugUtil/DebugCommon.h"
#endif

namespace Proud
{
	CThreadPool *CThreadPool::Create(IThreadPoolEvent* eventSink, int threadCount)
	{
		// 여기서 이걸 꼭 해주어야 한다. 안그러면 handover 함수 안에서 
		// 아직 준비되지 않은 eventfd or socketpair를 건드리는 문제가 생긴다.

		// NOTE: iocp나 epoll 등 생성이 실패할 수도 있는 변수들 때문에, ctor가 아닌 Create함수가 따로 있는 것임.
		CThreadPoolImpl* obj = new CThreadPoolImpl();

		CriticalSectionLock lock(obj->m_cs, true);

		obj->m_zeroThread = CWorkerThreadPtr(new CWorkerThread);

		// iocp,epoll,kqueue or simple poll
#if defined (_WIN32)
		// IOCP 준비.
		obj->m_completionPort.Attach(new CompletionPort());
#else
		// zero thread의 ionotifier를 세팅하기
		obj->m_zeroThread->m_ioNotifier.Attach(new CIoReactorEventNotifier());
#endif
		obj->SetEventSink(eventSink);

		// 최초의 스레드 수를 설정한다.
		// 즉시 스레드를 생성하게 만들자. 
		// 여기서는, 초기 명세와 달리 SetDesiredThreadCount를 호출하지 말고 여기서 즉시 스레드를 생성하게 합시다.
		// NetCore모듈의 빠른 활성화를 위함.
		obj->SetDesiredThreadCount(threadCount);

		// 이 함수가 리턴되는 즉시 스레드 수가 threadCount만큼 만들어지는건지요?
		// 별도의 thread가 생성되지 않으면 함수내부에서 즉시 스레드를 생성합니다.

		return obj;
	}

	CThreadPoolImpl::CThreadPoolImpl()
	{
		m_referrers.SetOptimalLoad_BestLookup();

		m_eventSink_NOCSLOCK = NULL;
		m_stopping = false;
		m_chooseIndex = 0;
		m_desiredThreadCount = 0;

		m_depend_CFavoritePooledObjects = CFavoritePooledObjects::GetSharedPtr();
	}

	// 파괴자. 스레드풀 모든 스레드가 파괴될 때까지 대기.
	// referrer가 없어야 한다. 안그러면 파괴자에서 예외 던져진다.
	CThreadPoolImpl::~CThreadPoolImpl()
	{
		AssertIsNotLockedByCurrentThread();
		{
			CriticalSectionLock clk(m_cs, true);

			if (m_referrers.GetCount() > 0)
				throw Exception("You MUST delete every thread pool referrer (NetClient, NetServer, ...) before deleting thread pool object!");

			// 모든 스레드 종료를 건다.
			// SetDesiredThreadCount 내부에서 m_stopping이 true이면 무시되도록 설정되어 있음. 따라서 m_stopping을 건들기 전에 먼저 이것을 해야.
			SetDesiredThreadCount(0);
			m_stopping = true; // 아래 구간동안 no lock 상태로 대기 타기 때문에, 다른 스레드에서 스레드를 올리는 짓을 못하게 하기 위해
		}

		// 모든 스레드가 사라질 때까지 대기
		while (1)
		{
			{
				CriticalSectionLock lock(m_cs, true);

				if (m_garbagedThreads.IsEmpty())
					break;
			}
			Sleep(10);
		}

		// 안해도 됨. 파괴자니까.
		// #ifdef _WIN32
		// 		// 이제 모든 스레드들이 종료했다.
		// 		// 안전하게 모든 ioqueue를 제거
		// 		m_completionPort.Free();
		// #endif
		// 		m_customValueEventQueue.Clear();
	}

	void CThreadPoolImpl::StaticWorkerThread_Main(void* ctx)
	{
		CWorkerThread* workerThread = (CWorkerThread*)ctx;
		CThreadPoolImpl* ownerThreadPool = (CThreadPoolImpl*)workerThread->m_ownerThreadPool;

		ownerThreadPool->WorkerThread_Main(workerThread);

		/* epoll_wait의 결과값에 socket A의 이벤트를 가진 2개 이상의 epoll은 없다.
		스레드3에서 A를 처리한 후, 더 이상 A를 액세스하지 않는다는 전제하에,
		thread 4의 epoll에 add A를 한다.
		그 순간, 스레드 4에서 A에 대한 이벤트를 처리하면서 A를 즉시 액세스를 할 수 있다.
		그러나 이미 스레드3은 A를 더 이상 액세스하지 않음이 보장되어 있기 때문에,
		두 스레드에서 A를 액세스하는 일은 없다. */
	}

	void CThreadPoolImpl::WorkerThread_Main(CWorkerThread* thisThread)
	{
		// 스레드가 종료하기 직전 실행되는 루틴.
		/* 안타깝지만 IOT에서는 컴파일러가 람다식을 처리 못한다.
		따라서 아래 구문 대신 RunOnScopeOut2를 구현함.

		RunOnScopeOut dd(([this, thisThread]()
		{
			// 여기까지 오게 되면, ThreadPool은 garbage list에는 this를 갖고 있는 상태다.
			// 따라서 지우도록 하자.
			CriticalSectionLock lock(m_cs, true);
#if !defined (_WIN32)
			CWorkerThreadPtr destWorkerThread = GetWorkerThread_NOLOCK(CThreadPoolImpl::ChooseAny);
			// 종료되는 Thread가 가지고 있는 Socket과 Eventfd를 다른 Thread로 넘겨줍니다.
			HandOverSocketAndEvent_NOLOCK(thisThread, destWorkerThread);
#endif
			m_garbagedThreads.RemoveAt(thisThread->m_threadPoolIter);
		}));
		*/

		class RunOnScopeOut2
		{
			bool m_run;
		public:
			CThreadPoolImpl* m_this;
			CWorkerThread* m_thisThread;

			void Run()
			{
				if (!m_run)
				{
					m_run = true;

					// 여기까지 오게 되면, ThreadPool은 garbage list에는 this를 갖고 있는 상태다.
					// 따라서 지우도록 하자.
					CriticalSectionLock lock(m_this->m_cs, true);
#if !defined (_WIN32)
					CWorkerThreadPtr destWorkerThread = m_this->GetWorkerThread_NOLOCK(CThreadPoolImpl::ChooseAny);
					// 종료되는 Thread가 가지고 있는 Socket과 Eventfd를 다른 Thread로 넘겨줍니다.
					m_this->HandOverSocketAndEvent_NOLOCK(m_thisThread, destWorkerThread);
#endif
					m_this->m_garbagedThreads.RemoveAt(m_thisThread->m_threadPoolIter);
				}
			}

			RunOnScopeOut2() : m_run(false), m_this(NULL), m_thisThread(NULL)
			{}		

			~RunOnScopeOut2() {
				Run();
			}
		};
		
		RunOnScopeOut2 dd;
		dd.m_this = this;
		dd.m_thisThread = thisThread;

#if defined (_WIN32)
		// device time에서 awake를 하는 경우 일시적으로 최우선으로 처리를 해주도록 한다. 적은 context switch를 위한 것이다.
		SetThreadPriorityBoost(GetCurrentThread(), FALSE); // MSDN 왈, FALSE가 boost를 켠다고.
		CCoInitializer coi;		// 이것은 스레드 단위다. 프로세스 단위가 아니고. 유저 코드에 의해 쓰일 것이므로 호출 불가피.
#endif
		AssertIsNotLockedByCurrentThread();
		if (m_eventSink_NOCSLOCK != NULL)
		{
			m_eventSink_NOCSLOCK->OnThreadBegin();
		}

		while (!thisThread->m_stopThisThread)
		{
#if defined(_WIN32)
			// 프로세스 종료중이면 아무것도 하지 않는다.
			if (Thread::m_dllProcessDetached_INTERNAL)
				break;
#endif
			ProcessAllEvents(thisThread, NULL, CNetConfig::WaitCompletionTimeoutMs);
		}

		// 이 스레드가 죽을 때 다른 스레드에게 소켓과 이벤트를 이양하는 과정을 하지는 않는다. 그냥 죽는다.


		// 스레드가 종료됐다. 이 스레드가 가진 eventfd가 가진 값+1을 가져와, 
		// 나머지 잔존 스레드 중 하나에게 던져주도록 하자.
		// 만약 이 스레드가 최후의 스레드인 경우 던져짐을 받는 것은 m_zeroThread에게 던져주어야.

		// 위 구현을 합시다. m_zeroThread와 thisThread의 ioNotifier의 eventfd로부터 값 가져와 새것에 대해 write를 해주어야 할 것임.
		// #ifndef _WIN32
		// 		{
		// 			CriticalSectionLock lock(m_cs, true);
		// 
		// 			CWorkerThreadPtr workerThread = GetWorkerThread(thisThread);
		// 			AddToWorkerThreadWithSocketAndEventfd(thisThread, workerThread);
		// 		}
		// #endif
		AssertIsNotLockedByCurrentThread();

		if (m_eventSink_NOCSLOCK != NULL)
		{
			m_eventSink_NOCSLOCK->OnThreadEnd();
		}

	}

	void CThreadPoolImpl::SetEventSink(IThreadPoolEvent* eventSink)
	{
		// 이미 count 가 8
		// 		if (m_workerThreadPool.GetCount() > 0)
		// 			throw Exception("Already async callback may occur! threadpool start should have not been done before here!");

	//	AssertIsNotLockedByCurrentThread();
		m_eventSink_NOCSLOCK = eventSink;
	}

	// IThreadReferrer를 thread pool에 등록한다.
	// custom value event가 콜백될 것이다.
	bool CThreadPoolImpl::RegisterReferrer(IThreadReferrer* referrer)
	{
		CriticalSectionLock clk(m_cs, true);

		if (m_referrers.ContainsKey((intptr_t)referrer))
			return false;

		CReferrerStatusPtr referrerUseInfo = CReferrerStatusPtr(new CReferrerStatus);

		m_referrers.Add((intptr_t)referrer, referrerUseInfo);

		return true;
	}

	/* referrer를 등록 해제한다.
	이 함수가 리턴한 직후부터는 thread pool에서 이 referrer에 대한 custom value event는 전혀 진행중이 아님이 보장된다.
	따라서 이 함수를 호출한 스레드는 이 함수 리턴 직후 바로 delete referrer를 해도 된다.
	그 댓가로 이 함수는 referrer를 다루는 thread가 전혀 없을 때까지 블러킹된다. */
	void CThreadPoolImpl::UnregisterReferrer(IThreadReferrer* referrer)
	{
		const int64_t TimeOut = 30 * 1000;
		int64_t t = GetPreciseCurrentTimeMs();

		do
		{
			{
				CReferrerStatusPtr referrerUseInfo;

				CriticalSectionLock clk(m_cs, true);

				// 이미 지워져있는데 이 함수를 호출하면 안되지!
				if (!m_referrers.TryGetValue((intptr_t)referrer, referrerUseInfo))
				{
					assert(0);
					return;
				}

				// UnregisterReferrer 요청을 받았음을 설정합니다.
				referrerUseInfo->m_unregisterRequested = true;

				// 작동중인 스레드 중에 referrer의 event를 처리하고 있는 worker thread가 것이 하나도 없는 것을 확인하면,
				if (referrerUseInfo->m_useCount == 0)
				{
					m_referrers.Remove((intptr_t)referrer);

					// custom event queue에서도 referrer를 위한 event들을 모두 제거한다. (그래야 이후 것이 모두 안 수행됨)
					for (CFastList2<CCustomValueEvent,int>::iterator itr = m_customValueEventQueue.begin();
						itr != m_customValueEventQueue.end();)
					{
						CCustomValueEvent& v = *itr;
						if (v.m_referrer == referrer)
						{
							itr = m_customValueEventQueue.erase(itr);
						}
						else
							++itr;
					}

					return;
				}
			}

			// 조건 불만족. 조금 있다 다시 재시도.
			Sleep(10);
		} while (GetPreciseCurrentTimeMs() - t < TimeOut);

		throw Exception("UnregisterReferrer TimeOut!!");
	}

	// 현재 스레드가 이 스레드풀 내 있는지?
	bool CThreadPoolImpl::ContainsCurrentThread()
	{
		CriticalSectionLock clk(m_cs, true);

		if (m_workerThreads.IsEmpty())
			return false;

#ifdef _WIN32
		uint32_t tid = GetCurrentThreadId();
#else
		pthread_t tid = pthread_self();
#endif

		for (CFastList2<CWorkerThreadPtr, int>::iterator itr = m_workerThreads.begin();
			itr != m_workerThreads.end(); ++itr)
		{
			CWorkerThreadPtr workerThread = *itr;

#ifdef  _WIN32
			if (workerThread->m_thread->ID == tid)
#else
			if (workerThread->m_thread->GetHandle() == tid)
#endif
				return true;
		}

		return false;
	}

	// 각 스레드의 정보를 리턴한다.
	void CThreadPoolImpl::GetThreadInfos(CFastArray<CThreadInfo>& output)
	{
		CriticalSectionLock clk(m_cs, true);

		if (m_workerThreads.IsEmpty())
			return;

		CThreadInfo info;
		for (CFastList2<CWorkerThreadPtr, int>::iterator itr = m_workerThreads.begin();
			itr != m_workerThreads.end(); ++itr)
		{
			CWorkerThreadPtr workerThread = *itr;
#if defined (_WIN32)
			info.m_threadID = workerThread->m_thread->ID;
#endif
			info.m_threadHandle = workerThread->m_thread->GetHandle();

			output.Add(info);
		}
	}

	// 스레드를 1개 추가.
	void CThreadPoolImpl::CreateWorkerThread(int threadCount)
	{
		CriticalSectionLock lock(m_cs, true);

		for (int i = 0; i < threadCount; i++)
		{
			CWorkerThreadPtr thread = CWorkerThreadPtr(new CWorkerThread);

			thread->m_ownerThreadPool = this;

			/* SetThreadCount가 스레드 수를 줄일 경우,
			스레드가 줄어드는 동안 블러킹이 일어나면 안된다.
			그래서 needJoin=false로 설정된다. */
			thread->m_thread = ThreadPtr(new Thread(StaticWorkerThread_Main, (CWorkerThread*)thread, false));
#ifndef _WIN32
			// 스레드용 epoll을 생성
			thread->m_ioNotifier.Attach(new CIoReactorEventNotifier);

			// 스레드가 전혀 없는 상태에서 최초의 스레드를 만드는 경우
			// zero thread 즉 유저가 수동으로 Process()를 호출해야만 작동하는 스레드의 내부 정보를 최초의 스레드로 옮겨버린다.
			// zero thread는 이때부터는 무용해짐.
			if (m_workerThreads.IsEmpty())
			{
				HandOverSocketAndEvent_NOLOCK(m_zeroThread, thread);
			}
#endif
			// 스레드 리스트 업데이트
			thread->m_threadPoolIter = m_workerThreads.AddTail(thread);

			// 스레드 시작
			thread->m_thread->Start();
		}
	}

	/* 매개변수인 current worker thread를 제외하고 종료 중인 아닌 worker thread를 반환한다.
	thread pool의 스레드가 전혀 없으면 zero thread를 반환한다. */
	CWorkerThreadPtr CThreadPoolImpl::GetWorkerThread_NOLOCK(Choose choose)
	{
		// 이 함수의 콜러에서부터 이미 lock this 상태이어야 합니다. 
		// 인자 혹은 리턴값이 내부 객체의 뭔가를 가리키는 경우에는 애당초 콜러에서부터 잠금 상태 유지 해야 합니다. 
		// 안하면 데이터 레이스임. 멀티스레드 프로그래밍의 기본입니다. 기억하세요.
		AssertIsLockedByCurrentThread();

		// WorkerThread가 더이상 존재하지 않는다면 ZeroThread를 반환한다.
		if (m_workerThreads.IsEmpty())
			return m_zeroThread;

		CWorkerThreadPtr output = m_zeroThread;

		switch (choose)
		{
		case Proud::CThreadPoolImpl::ChooseAny:
#ifdef _WIN32
			// Windows 에서는 WorkerThread를 반환한다.
			// 이해가 안되는데 설명 부탁해요. 첫번째 thread를 무작정 반환하네요?
			// ==> Windows의 경우, 하나의 CompletionPort로 Socket들을 관리하기에 어떤 Thread를 반환하더라도 큰 의미가 없다고 판단하여 첫번째 Thread를 무조건 반환하도록하였습니다.
			return m_workerThreads.GetHead();
#else
			{
				// Windows가 아닌 환경에서는 (+1 MOD N) 방식으로 선출합니다.
				m_chooseIndex = (m_chooseIndex + 1) % m_workerThreads.GetCount();

				// (+1 MOD N) 방식으로 선출된 Index를 가진 WorkerThread의 Position을 구합니다.
				Proud::Position position = m_workerThreads.FindIndex(m_chooseIndex);
				// Index를 통해 찾은 Position으로 WorkerThread를 구합니다.
				output = m_workerThreads.GetAt(position);
			}
#endif
			break;

#ifndef _WIN32
		case Proud::CThreadPoolImpl::ChooseHavingFewSockets:
		{
			int minSocketCount = INT_MAX;
			// PN_FOREACH를 사용할 경우 CFastList2<CWorkerThreadPtr, int>의 ,(콤마)가 다음 매개변수로 인식되는 문제로 원래상태로 원복하였습니다.
			for (CFastList2<CWorkerThreadPtr, int>::iterator itr = m_workerThreads.begin();
				itr != m_workerThreads.end(); ++itr)
			{
				CWorkerThreadPtr workerThread = *itr;

				int socketCount = workerThread->m_ioNotifier->m_associatedSockets.GetCount();

				// 가장 Socket을 적게 소유한 WorkerThread를 찾습니다.
				if (minSocketCount > socketCount)
				{
					minSocketCount = socketCount;
					output = workerThread;
				}
			}
		}
		break;

		case Proud::CThreadPoolImpl::ChooseIdleOrAny:
		{
			int minEpollWorkCountPerSec = INT_MAX;
			// ChooseIdleOrAny를 이용하는 콜러는 매우 성능에 민감한 함수입니다.
			// 최대한 최적화되어 작동할 수 있게 해주시기 바랍니다. 스레드 수만큼 루프를 도는 것도 무겁긴 하지만 일단은 그렇게 가도 됩니다.

			// PN_FOREACH를 사용할 경우 CFastList2<CWorkerThreadPtr, int>의 ,(콤마)가 다음 매개변수로 인식되는 문제로 원래상태로 원복하였습니다.
			for (CFastList2<CWorkerThreadPtr, int>::iterator itr = m_workerThreads.begin();
				itr != m_workerThreads.end(); ++itr)
			{
				CWorkerThreadPtr workerThread = *itr;

				int epollWorkCountPerSec = workerThread->m_ioNotifier->m_epollWorkCountPerSec;

				// 초당 가장 적게 처리한 WorkerThread를 찾습니다.
				if (minEpollWorkCountPerSec > epollWorkCountPerSec)
				{
					minEpollWorkCountPerSec = epollWorkCountPerSec;
					output = workerThread;
				}
			}
		}
		break;
#endif
		default:
			assert(0);
			break;
		}

		return output;
	}

#ifndef _WIN32
	//	void CThreadPoolImpl::WriteEvnetfdToWorkerThreadHavingSocket(CSuperSocket* socket)
	// 	{
	// 		CriticalSectionLock clk(m_cs, true);
	// 
	// 		if (m_workerThreadPool.IsEmpty())
	// 		{
	// 			CriticalSectionLock lock(m_zeroThread->m_ioNotifier->m_cs, true);
	// 
	// 			if (m_zeroThread->m_ioNotifier->m_associatedSockets.ContainsKey((intptr_t)socket))
	// 			{
	// 				m_zeroThread->m_ioNotifier->RemoveSocket(socket);
	// 				return;
	// 			}
	// 		}
	// 
	// 		for (CFastList2<CWorkerThreadPtr, int>::iterator itr = m_workerThreadPool.begin();
	// 			itr != m_workerThreadPool.end(); itr++)
	// 		{
	// 			CWorkerThreadPtr workerThread = *itr;
	// 
	// 			CriticalSectionLock lock(m_zeroThread->m_ioNotifier->m_cs, true);
	// 
	// 			if (workerThread->m_ioNotifier->m_associatedSockets.Co﻿ntainsKey((intptr_t)socket))
	// 			{
	// 				workerThread->m_ioNotifier->RemoveSocket(socket);
	// 				return;
	// 			}
	// 		}
	// 
	// 		// 아래까지 왔다는 것은 socket을 가지는 WorkerThread가 없을 경우
	// 		throw Exception("Any WorkerThread hasn't this socket!");
	// 	}

	/* src가 가진 socket과 eventfd를 새로운 worker thread에 등록한다.
	ThreadPool의 스레드 수를 줄일 때 사용된다. */
	void CThreadPoolImpl::HandOverSocketAndEvent_NOLOCK(CWorkerThread* src, CWorkerThread* dest)
	{
		AssertIsLockedByCurrentThread();

		int64_t value = 0;

		// src가 가진 event 누적량을 dest로 모두 옮긴다.
		
		int ret = read(src->m_ioNotifier->GetReadOnlyEventFD(), &value, sizeof(value));
		if (ret == -1)
		{
			// read(2) proceeds as described above) or fails with the error EAGAIN if the file descriptor has been made nonblocking.
			int e = errno;
			if (e != EAGAIN && e != EWOULDBLOCK)
			{
				NTTNTRACE("event fd read failed. errno=%d", e);
				assert(0);
			}
		}
		else  // 정상적으로 read를 하였을 경우 destWorkerThread에 write.
		{
			write(dest->m_ioNotifier->GetWriteOnlyEventFD(), &value, sizeof(value));
		}

		{
			// dead lock 방지를 위해 src,dest를 ptr 값이 작은 순으로 잠근다.
			CriticalSection* cs1 = &src->m_ioNotifier->m_cs;
			CriticalSection* cs2 = &dest->m_ioNotifier->m_cs;
			if (cs2>cs1)
				swap(cs1, cs2);

			CriticalSectionLock lock1(*cs1, true);
			CriticalSectionLock lock2(*cs2, true);

			// PN_FOREACH를 사용할 경우 CFastMap2<CFastSocket*, CFastSocket*, int>의 ,(콤마)가 다음 매개변수로 인식되는 문제로 원래상태로 원복하였습니다.
			for (CFastMap2<CFastSocket*, CFastSocket*, int>::iterator itr = src->m_ioNotifier->m_associatedSockets.begin(); itr != src->m_ioNotifier->m_associatedSockets.end(); ++itr)
			{
				// socket을 또다른 thread에 assoc하는 경우
				// 일시적으로 두 스레드에서 동시에 avail io가 떠버릴 수 있다.
				// 이거 어떻게 해결한다?
				// ==> Thread가 종료되기 전(더 이상의 I/O Event를 처리할 수 없는 시점)에 Socket을 이양하도록 변경하였습니다.
				CFastSocket* sock = (CFastSocket*)itr->GetSecond();

				// LevelTrigger
				sock->m_isLevelTrigger = true;
				sock->m_associatedIoQueue = NULL;

				// 인수인계받은 worker thread는 처음에는 level trigger로 받는다.
				// 그래야 과거 worker thread가 가졌던 epoll의 이벤트를 분명하게 처리하니까.
				dest->m_ioNotifier->AssociateSocket(sock, false);
			}
			src->m_ioNotifier->m_associatedSockets.Clear();
		}
	}

#endif // !_WIN32

	// 스레드의 수를 리턴한다. zero thread mode이면 0을 리턴한다.
	int CThreadPoolImpl::GetThreadCount()
	{
		return m_workerThreads.GetCount();
	}

	// socket을 이 thread pool에 add한다.
	// iocp, epoll 모두 공통으로 작동한다.
	// iocp의 경우 단일 iocp에 add되고, epoll의 각 스레드 중 가장 적은 수의 socket이 있는 곳의 epoll에 add된다.
	void CThreadPoolImpl::AssociateSocket(CFastSocket* socket)
	{
#ifdef _WIN32
		// 내부 lock를 쓰지는 않는다.
		m_completionPort->AssociateSocket(socket);
#else
		// 리눅스에서는 각 스레드가 가진 IoEventReactor 중 가장 적은 소켓을 가진 놈에게 Proud.CIoEventReactor.AssociateSocket를 하게 바뀌어야.
		// 각 epoll은 m_workerThreadPool에 들어있다.
		/* 2014.07.04 : seungchul.lee
		* Q: SetDesiredThreadCount에 의해 종료되는 WorkerThread에 AssociateSocket을 해버린다면?
		* A: GetWorkerThread 함수에서는 종료 중(m_stopThisThread=ture)인 WorkerThread는 호출하지 않는다. */
		CriticalSectionLock lock(m_cs, true);

		// 가장 적은 수의 소켓을 가진 스레드에게 붙인다.
		CWorkerThreadPtr workerThread = GetWorkerThread_NOLOCK(ChooseHavingFewSockets);
		workerThread->m_ioNotifier->AssociateSocket(socket, true);
#endif 
	}

	// 1개의 custom event를 꺼낸다.
	// referrer에는 custom event를 처리할 주체의 ptr가 채워진다.
	// customValue에는 꺼낸 이벤트가 채워진다.
	// 꺼내는데 성공하면 true, 꺼낼게 없으면 false.
	bool CThreadPoolImpl::PopCustomValueEvent(IThreadReferrer** referrer, CustomValueEvent& customValue)
	{
		CCustomValueEvent e;

		CriticalSectionLock lock(m_cs, true);

		if (m_customValueEventQueue.IsEmpty())
			return false;

		e = m_customValueEventQueue.RemoveHead();

		*referrer = e.m_referrer;
		customValue = e.m_customValue;

		return true;
	}

	// custom value를 큐에 넣고 eventfd에 신호를 던진다.
	// iocp PQCS와 같은 역할.
	// post가 성공하려면 referrer가 이미 register되어 있어야 한다.
	bool CThreadPoolImpl::PostCustomValueEvent(IThreadReferrer* referrer, CustomValueEvent customValue)
	{
		// engine/main에 보면, referrer를 찾을 뿐만 아니라 UseReferrer() 체크도 하고 있습니다.
		// 그러나 여기에는 없습니다.
		// 어느 것이 맞는지 확인 바랍니다.
		// 해당 구현에 대해서는 진혁씨가 압니다.

		// ==> engine/main의 경우, PostCompletionStatus를 호출하는 기준은 CustomValueEvent와 Socket Accept, First Recv등에 대한 상황에서도 사용이 됩니다.
		// 하지만, engine/dev003의 PostCustomValueEvent는 오로지 CustomValueEvent에서만 사용이 됩니다.
		// CustomValueEvent의 경우 PQCS가 여러번 호출되더라도 한 번 호출되면 while문을 돌면서 그동안 쌓여있던 Queue에서 처리하게 됩니다.
		// 그렇기 때문에 PQCS 시점에서 UseReferrer를 확인하는 것이 아닌, Event를 호출하기 직전에 확인하도록 구현하였습니다.


		// 우선, key와 custom value 값을 m_customValueQueue에 넣자. (critsec 보호 잊지 말 것)
		CriticalSectionLock lock(m_cs, true);

		// 등록된 referrer에 한해서만 enque & post된다.
		// 즉 미등록이면 post를 하지 말고 false 리턴
		//이렇게 수정하자.
		// m_referrers를 CFastMap2로 바꾸시기 바랍니다. O(n)검색을 매 post마다 하면 엄청 느림.
		if (!m_referrers.ContainsKey((intptr_t)referrer))
		{
			//그리고 caller들을 모두 찾아, post가 실패해도 ignore해도 되는 것들이 아니면
			// post 실패시 throw exception해야 한다.
			// Heartbeat, SendEnqueued 같은 것은 상관없지만, 1회만 전달되는 것은 에러 처리해야.
			return false;
		}

		CCustomValueEvent e;
		e.m_referrer = referrer;
		e.m_customValue = customValue;

		m_customValueEventQueue.AddTail(e);

		// code profile 결과 DoUserTask에 너무 많은 시간을 쓴다.
		// code profile 결과 이 부분이 무시못하는 허당(정작 할 일이 없음) 99%다.
		// 따라서 최초 추가할 때만 PQCS or write event를 수행.
		if (CNetConfig::DefensiveCustomValueEvent || m_customValueEventQueue.GetCount() == 1)
		{
#if !defined(_WIN32) // ios,linux뿐만 아니라 PS4,Marmalade를 모두 포함한다.

			/* custom event를 처리할 적당한 스레드를 고르고
			그 스레드가 즉시 깰 수 있게 write to eventfd을 한다.
			놀고 있는 스레드를 우선으로 선택하되 노는 것이 없으면 아무거나 고른다.
			초당 처리 횟수가 가장 적은 것을 고르는 것은 문제가 있다.
			각 처리 단위당 소요 시간이 많아서 초당 처리 횟수가 느린 놈일 수도 있기 때문이다.
			*/
			CWorkerThreadPtr workerThread = GetWorkerThread_NOLOCK(ChooseIdleOrAny);

			int64_t value = 1;
			write(workerThread->m_ioNotifier->GetWriteOnlyEventFD(), &value, sizeof(value));
			//eventfd_write(workerThread->m_ioNotifier->m_eventFD, 1);
#else
			// iocp: 그리고 그냥 iocp에 signal post.
			m_completionPort->PostCompletionStatus();
#endif
		}
		return true;
	}

	void CThreadPoolImpl::Process(int waitTimeMs)
	{
		Process(NULL, waitTimeMs);
	}

	// 사용자가 관리하는 스레드에서 직접 thread pool의 뭔가를 처리할 경우
	// zero thread일때만 내부 작동을 한다. 
	// 그 외에는 이미 다른 스레드에서 처리중이므로 아무것도 안한다.
	void CThreadPoolImpl::Process(CWorkResult* workResult, int waitTimeMs)
	{
		// 동시에 이 함수를 호출해도 직렬화를 시킨다.
		CriticalSectionLock lock(m_zeroThreadCritSec, true);

		// 작동중인 스레드가 있으면 이 함수는 아무것도 하지 말아야. 왜냐하면 zero thread를 위한 것이 아무것도 없으므로.		
		// 작동중인 스레드가 없으면 첫번째 스레드를 위한 처리를 한다.
		if (!m_workerThreads.IsEmpty())
			return;

		ProcessAllEvents(m_zeroThread, workResult, waitTimeMs);
	}

	// 목표로 하는 스레드의 수를 정한다.
	// 이 함수는 즉시 리턴하고, 조금 있다가 스레드의 수가 늘거나 준다.
	void CThreadPoolImpl::SetDesiredThreadCount(int threadCount)
	{
		// 현재 스레드 수 조정중이 아니면서 스레드 수가 늘어나는 상황이면
		// 이양 과정 없이 이 함수가 리턴하는 순간 모든 스레드가 즉시 실행되게 합시다.
		// 맨 처음 thread pool 객체를 생성 후 최초의 SetDesiredThreadCount를 호출한 경우 즉시 스레드가 생겨주어야 하기 때문임.
		// 안그러면 서버 시작시 클라들 접속 무섭게 들어오기 시작할 때 늑장 대응으로 서버 장애 생김.

		// 입력값 검사
		if (threadCount < 0 || threadCount >= 512)
			throw Exception("Invalid thread count %d!", threadCount);

		// 여기서 이걸 꼭 해주어야 한다. 안그러면 handover 함수 안에서 
		// 아직 준비되지 않은 eventfd or socketpair를 건드리는 문제가 생긴다.
		CriticalSectionLock lock(m_cs, true);

		// 스레드 풀이 종료중이면 무시해야.
		if (m_stopping)
			return;

		// 이전에 설정한 desiredThreadCount와 동일하면 무시!
		if (m_desiredThreadCount == threadCount)
			return;

		// 현재의 WorkerThreadPool의 갯수를 가져온다.
		int currentThreadCount = m_workerThreads.GetCount();

		// 모자란 스레드 수만큼 더 만든다.
		if (threadCount > currentThreadCount)
		{
			// 아래의 함수에서는 WorkerThread를 즉시 생성하여 실행한다.
			CreateWorkerThread(threadCount - currentThreadCount);
		}
		else if (threadCount < currentThreadCount)
		{
			// 잉여 스레드를 제거한다.

			// 필요한 수 만큼 중단 요청
			for (int i = 0; i < currentThreadCount - threadCount; i++)
			{
				// 아무 스레드나 고른다. 
				// 처리량이 가벼운 스레드냐 무거운 스레드냐를 고르는 것이 별로 의미가 없다.
				// 균등 조건에서 아무거나 랜덤으로 고르자.
				CWorkerThreadPtr workerThread = GetWorkerThread_NOLOCK(ChooseAny);

				// 해당 스레드에게 종료 요청을 한다.
				workerThread->m_stopThisThread = true;

				// 생성 시에 ThreadPool의 Iter로 해당 객체를 제거한다.
				m_workerThreads.RemoveAt(workerThread->m_threadPoolIter);
				// GarbagedThreadPool에 추가와 함께 추가된 Iter로 교체한다.
				workerThread->m_threadPoolIter = m_garbagedThreads.AddTail(workerThread);
			}
		}
		m_desiredThreadCount = threadCount;
	}

	// iocp or epoll에 누적되어 있는 이벤트를 모두 꺼내 처리한다.
	// workResult에 실행 결과가 채워진다.
	// waitTimeMs만큼 기다린다. 0이면 안 기다리고 지금 당장 처리가능한 이벤트만 처리한다.
	void CThreadPoolImpl::ProcessAllEvents(CWorkerThread* workerThread, CWorkResult* workResult, int waitTimeMs)
	{
		if (waitTimeMs != 0)
		{
			AssertIsNotLockedByCurrentThread();
		}

		POOLED_ARRAY_LOCAL_VAR(CIoEventStatusArray, polledEvents);

		// NOTE: PS4,Marmalade도 이제 timer event나 custom event가 발생할 때를 위한 socket pair를 쓴다. 
		// 따라서 waitTimeMs가 20 미만으로 짧을 필요가 없다.

		// iocp or epoll로부터 socket io event나 custom event를 꺼내온다.
		// NOTE: custom event는 epoll에 엮인 eventfd나 iocp PQCS를 통해 꺼내짐.
#if defined (_WIN32)
		m_completionPort->GetQueuedCompletionStatusEx(polledEvents, MaxPollCount, waitTimeMs);
#else
		// thread pool 안에서 실행되는 경우: 대응 CWorkerThread의 epoll로부터 pop.
		// CThreadPool.Process()를 사용자가 호출한 경우: m_zeroThread의 epoll로부터 pop.
		// 위 내용을 구현합시다. 
		workerThread->m_ioNotifier->Poll(polledEvents, MaxPollCount, waitTimeMs);
#endif

		int dequeuedCount = polledEvents.GetCount();
		for (int i = 0; i < dequeuedCount; ++i)
		{
			if (polledEvents[i].m_type == IoEventType_ReferCustomValue)
			{
				// 적은 thread context switch와 syscall를 위해 한큐에 모두 처리해야 하고]
				// eventfd가 가진 count가 부정확할 수 있으므로(게다가 semaphore-like가 아니면 아예 1이고)
				// 루프를 돌아서 실제 쌓인 이벤트만 처리한다.

#ifndef _WIN32 // iocp를 쓰지 않는 OS에 대해
				{
					// eventfd에 쌓인 event를 처리하는 것이므로, 그것을 한번 비워줘야.
					// 꺼낸 결과는 그냥 무시한다. 어차피 아래에서 루프 돌면서 처리하며 semaphore-like eventfd가 아니니까.
					// worker thread 갯수를 줄일 때 eventfd의 카운트가 무의미해지므로 이렇게 해야 합니다.
					int64_t value = 0;
					int ret = read(workerThread->m_ioNotifier->GetReadOnlyEventFD(), &value, sizeof(value));
				}
#endif

				/* custom event를 처리.
				반드시 루프를 돌아야 한다.
				각 워커 스레드의 eventfd는 non-semaphore like이니까.
				설령 semaphore like였다 하더라도 카운트가 정확하지 않다.
				스레드 수 감소시 custom event도 이양하는 과정 때문이다. */
				while (true)
				{
					IThreadReferrer* poppedThreadReferrer = NULL;
					CustomValueEvent poppedCustomValue;

					{
						CriticalSectionLock lock(m_cs, true);

						if (!PopCustomValueEvent(&poppedThreadReferrer, poppedCustomValue))
						{
							// 더 이상 꺼낼 이벤트가 없다. 루프 끝.
							break;
						}

						// custom event queue에서 꺼내서 처리하되 '작동중'임을 마킹한다.
						// 그래야 UnregisterReferrer()에서 자신에 대한 custom event가 미처리중임을 확인 후 제거한다.
						if (!IncreaseReferrerUseCount(poppedThreadReferrer))
						{
							// invalid referrer이면 pop한 것을 버리고 다음 pop을 시작한다.
							continue;
						}
					}

					// 여기 왔을 때는 thread pool lock을 하지 않은 상태이어야.
					AssertIsNotLockedByCurrentThread();

					poppedThreadReferrer->OnCustomValueEvent(workResult, poppedCustomValue);
#ifndef _WIN32
					// 아래의 변수는 GetWorkerThread_NOLOCK(ChooseIdleOrAny)의 산출을 위해 사용됩니다.
					++workerThread->m_ioNotifier->m_epollWorkCountPerSec;
#endif
					// 처리 끝.
					DecreaseReferrerUseCount(poppedThreadReferrer);
				}
			}
			else // socket io event case
			{
				Proud::CIoEventStatus& event = polledEvents[i];
#if defined (_WIN32)				
				CSuperSocket* socket = (CSuperSocket*)event.m_fastSocket->GetIoEventContext();

				//진단 정보 추가
				if (event.m_type == IoEventType_Send)
				{
					socket->m_totalSendEventCount++;
#ifdef GET_SEND_SYSCALL_TOTAL_COUNT
					AtomicIncrement32(&DebugCommon::m_socketSendEventCount);
#endif          
				}
				else
				{
					socket->m_totalReceiveEventCount++;

					if (socket->m_firstReceiveEventTime == 0)
					{
						socket->m_firstReceiveEventTime = GetPreciseCurrentTimeMs();
						int64_t diff = socket->m_firstReceiveEventTime - socket->m_firstIssueReceiveTime;

						CSuperSocket::m_firstReceiveDelay_TotalValue += diff;
						CSuperSocket::m_firstReceiveDelay_TotalCount++;
						if (diff > 10000)
						{
							int a = 0;
						}
						CSuperSocket::m_firstReceiveDelay_MaxValue = PNMAX(CSuperSocket::m_firstReceiveDelay_MaxValue, diff);
						CSuperSocket::m_firstReceiveDelay_MinValue = PNMIN(CSuperSocket::m_firstReceiveDelay_MinValue, diff);

					}
				}

				socket->OnSocketIoCompletion(event);
#else
				{
					// 매 이벤트 각각마다 ioNotifier를 lock 건다. 각 thread+epll이 가진 socket list를 액세스해야 하니까.
					CriticalSectionLock lock(workerThread->m_ioNotifier->m_cs, true);

					// 이제, ioNotier lock을 한 상태이다.
					// GarbageSocket에서 CIoReactorEventNotifier.RemoveSocket를 한 후에야
					// m_garbages.Add를 한다. (RemoveSocket도 내부적으로 ioNotifier lock을 한다)
					// 따라서 여기에서 valid socket ptr 즉 아래 Contains true이라고 나온 이상 
					// 아직 m_garbages에 들어가기 전임을 의미한다.
					// 따라서 delete safe 단계에 전혀 가지 않은 상태이다.
					// 따라서 안전하게 socket ptr을 액세스해도 된다.
					// 하지만 OnSocketIoAvail을 호출하기 전에 반드시 ioNotier unlock을 해야 하는데
					// 이때 socket ptr은 돌연 타 스레드에 의해 delete될 수 있는 위험이 있다.
					// 이를 방지하기 위해 unlock을 하기 전에 m_ioState=1로 바꿔놓는다.
					if (workerThread->m_ioNotifier->m_associatedSockets.ContainsKey(event.m_fastSocket))
					{
						assert(event.m_fastSocket->m_associatedIoQueue == workerThread->m_ioNotifier);

						CSuperSocket* superSocket = (CSuperSocket*)event.m_fastSocket->GetIoEventContext();

						if (AtomicCompareAndSwap32(0, 1, &superSocket->m_ioState) == 0)
						{
							// 반드시 unlock해야 한다. 
							// GarbageSocket에서 main lock->ioNotifer lock을 하는 구간이 있기 때문이다.
							lock.Unlock();

							// unlock을 딱 하는 순간 다른 스레드가 RemoveSocket를 호출해릴 수 있지만 무관.
							// 이미 m_ioState는 1이기 때문에 socket은 도중에 delete되지 않음이 보장된다.
							event.m_fastSocket->GetIoEventContext()->OnSocketIoAvail(event); // process socket event

							// 아래의 변수는 GetWorkerThread_NOLOCK(ChooseIdleOrAny)의 산출을 위해 사용됩니다.
							++workerThread->m_ioNotifier->m_epollWorkCountPerSec;

							AtomicCompareAndSwap32(1, 0, &superSocket->m_ioState);
						}
					}
				}
#endif 
			}
		}
	}

	// referrer의 이벤트가 처리중인 스레드가 하나 증가함을 표식
	// referrer가 invalid하면 false를 리턴.
	bool CThreadPoolImpl::IncreaseReferrerUseCount(IThreadReferrer* referrer)
	{
		CriticalSectionLock lock(m_cs, true);

		CReferrerStatusPtr referrerUseInfo;

		if (!m_referrers.TryGetValue((intptr_t)referrer, referrerUseInfo))
			return false;

		// Unregister 요청을 받은 상태라면 더 이상 처리하지 않습니다.
		if (referrerUseInfo->m_unregisterRequested)
			return false;

		// m_cs lock 후 건드리는 것이므로 atomic op 불필요
		++referrerUseInfo->m_useCount;

		// 증가된 Referrer의 Count의 값은 0이하의 값이 되어서는 안된다. 
		assert(referrerUseInfo->m_useCount > 0);

		return true;
	}

	// 위 함수의 반대
	void CThreadPoolImpl::DecreaseReferrerUseCount(IThreadReferrer* referrer)
	{
		CriticalSectionLock lock(m_cs, true);

		CReferrerStatusPtr referrerUseInfo;

		if (!m_referrers.TryGetValue((intptr_t)referrer, referrerUseInfo))
		{
			assert(0);
			return;
		}

		--referrerUseInfo->m_useCount;
		// 감소된 Referrer의 Count의 값은 0미만의 값이 되어서는 안된다. 
		assert(referrerUseInfo->m_useCount >= 0);
	}

	CThreadPool::~CThreadPool() {}
}