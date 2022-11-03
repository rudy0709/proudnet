/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/CriticalSect.h"
#include "../include/Deque.h"
#include "../include/IRmiHost.h"
#include "../include/ThreadPool.h"
#include "CustomValueEventQueue.h"
#include "Epoll_Reactor.h"
#include "FastMap2.h"
#include "FastSocket.h"
#include "IOCP.h"
#include "CriticalSectImpl.h"

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	class CThreadPoolImpl;

	// 내부에서 메시지와 이벤트가 얼마나 처리 되었는지의 결과입니다.
	// FrameMove의 CFrameMoveResult에 전달됩니다.
	class CWorkResult
	{
	public:
		uint32_t m_processedEventCount;
		uint32_t m_processedMessageCount;

		CWorkResult() :
			m_processedEventCount(0),
			m_processedMessageCount(0)
		{

		}
	};

	// CThreadPoolImpl의 각 스레드
	class CWorkerThread
	{
	public:
		CThreadPoolImpl* m_ownerThreadPool;
		Proud::Position m_threadPoolIter;

		// 스레드 핸들
		ThreadPtr m_thread;

		// true이면 이 스레드는 곧 종료한다.
		// SetDesiredThreadCount에 의해 세팅되기도 함.
		PROUDNET_VOLATILE_ALIGN bool m_stopThisThread;

		CWorkerThread()
		{
			m_ownerThreadPool = NULL;
			m_threadPoolIter  = NULL;
			m_stopThisThread  = false;
		}

#ifndef _WIN32
		// epoll은 여러 스레드에서 같은 fd에 대해서 io avail를 처리하는 것이 문제가 많다.
		// 따라서 부득이하게 각 스레드가 epoll을 가진다.
		CHeldPtr<CIoReactorEventNotifier> m_ioNotifier;
#endif
	};
	typedef RefCount<CWorkerThread> CWorkerThreadPtr;

	/* thread pool에 등록되는 객체.
	thread pool에 사용자가 post한 custom event를 처리하는 주체이기도 하다.
	thread pool이 다루는 socket들을 소유하기도 한다. NC,NS,LC,LS등이 이것을 상속받는 이유.
	
	스레드풀 보다 오래 살 수 없다. thread pool이 파괴될 때 이 객체가 아직 등록되어 있으면 안된다.
	1개 referrer는 여러 thread pool에 종속될 수 있다. (예: user worker thread, net worker thread) */
	class IThreadReferrer
	{
	public:
		/* custom event 콜백 루틴. 사용자가 구현해야 함.
		일정 시간마다 혹은 send issued without coalesce 등 다양한 상황에서 호출된다.

		사용자가 CThreadPoolImpl.Post를 호출하면
		thread pool의 worker thread 중 하나가 이 함수를 콜백한다.

		IThreadReferrer가 CThreadPoolImpl를 등록된 동안에만 콜백된다. 
		미등록된 경우 위 Post()는 무시된다. 
		
		workResult: 실행 결과를 채운다.
		customValue: 발생한 이벤트의 종류. user ptr 등이 안 들어가는 이유는, epoll이 그렇게까지 제공되지 않기 때문이다. */
		virtual void OnCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue) = 0;
	};

	class CThreadPoolImpl :public CThreadPool
	{
	public:
		/* thread list, custom event queue를 보호할 뿐만 아니라
		각 referrer의 is-working 도 보호한다.
		Q: is-working 까지 lock 보호 대상이면 매우 느릴텐데?
		A: 어차피 worker proc은 루프를 돌때 custom event에서 꺼내기 위해 pop을 해야 한다. 이때 lock은 불가피.
		그때 한꺼번에 다루면 된다.
		Q: 그래도 worker thread의 루프 빈도가 매우 높은데 매번 lock 하면 느리지 않은가?
		A: custom event를 다룰 때만 lock하는 과정이 있다. socket io event를 다루는 구간에서는 lock을 안한다.
		*/
		CriticalSection m_cs;

	private:
		/* zero thread에서 Process()를 여러 스레드에서 동시 호출하더라도
		단 1개의 스레드만이 작동하게 한다. 
		Process()에서만 사용할 것! */
		CriticalSection m_zeroThreadCritSec;

		/* custom value event queue. 
		custom event의 처리 주체는 IThreadReferrer만으로 제한된다.
		m_referrers에 등록되지 않은 것은 절대 이 queue에 들어있지 않는다. (referrer 제거시 이 queue에서도 모두 제거됨)
		최소한의 thread context switch를 위해, eventfd나 epoll처럼 linux 특성상 불가피한 것을 제외하고
		모두 이 클래스의 멤버로 둔다. thread별 데이터는 최소한으로 둔다.
		이로 인해, heartbeat에 의해 non block send가 수행되는 것과 send avail event에 의해 non block send가 수행되는 것이
		동시에 서로 다른 스레드에서도 발생하겠지만, 괜찮다. 
		pop then non block send를 하는 과정이 per-socket critsec으로 보호 후 수행되기 때문이다.

		PQCS 안에 이것을 의미하는 값을 넣는 것이 더 나이스하긴 하지만, 
		IThreadReferrer가 중도 사라지는 경우 iocp에 이미 들어간 IThreadReferrer용 custom value event를 중도 제거할 수 없어서
		이것을 따로 두는 것이다. 
    
    예전에는 Deque였으나 CFastList2가 성능이 더 빠름.
    */
		CFastList2<CCustomValueEvent, int> m_customValueEventQueue;

	public:
#ifdef _WIN32
		// 윈도 버전에서는 iocp를 쓰지만 리눅스에서는 각 스레드가 epoll을 가진다.
		// 단, custom value를 post or PQCS하는 경우에는 각 스레드 중 노는 스레드 하나가 깨어나야 하므로
		// (그래야 더 적은 thread context switch를 함) 
		// 이를 처리하기 위한 객체를 가진다.
		CHeldPtr<CompletionPort> m_completionPort;
#endif
		// worker thread들
		CFastList2<CWorkerThreadPtr, int> m_workerThreads;

		// 곧 죽을 worker thread들
		CFastList2<CWorkerThreadPtr, int> m_garbagedThreads;

		/* worker thread를 전혀 갖고 있지 않은, 즉 zero thread mode일때
		사용자가 Process()를 호출한다. 
		Process() 또한 eventfd나 epoll등을 요구하는데
		아래는 그것을 담기 위한 역할을 한다. */
		CWorkerThreadPtr m_zeroThread;

	private:
		// thread pool에 특화된 이벤트 콜백
		IThreadPoolEvent *m_eventSink_NOCSLOCK;

		// 각 referrer의 상태
		class CReferrerStatus
		{
		public:
			// 현재 이 referrer에 대한 이벤트가 실행중인 스레드의 수.
			// 사용 중이면 +1, 사용이 완료되면 -1
			// m_unregisterRequested=true이면 더 이상 처리하지 않습니다.
			int32_t m_useCount;

			// Unregister 요청을 받으면 True로 설정합니다.
			bool m_unregisterRequested;
			
			CReferrerStatus() :
				m_useCount(0),
				m_unregisterRequested(false) 
			{
			}
		};

		typedef RefCount<CReferrerStatus> CReferrerStatusPtr;

		// 이 thread pool에 의존되고 있는 처리 객체들. 가령 NC,NS,LC,LS다.
		CFastMap2<intptr_t, CReferrerStatusPtr, int> m_referrers;

		// 사용자가 원하는 스레드 수
		// 금방 thread 수는 이 값에 도래하게 된다.
		PROUDNET_VOLATILE_ALIGN int m_desiredThreadCount;

		// GetWorkerThread_NOLOCK(ChooseAny)에서 산출을 위한 Index 값
		int m_chooseIndex;

		// 종료 과정에서, SetDesiredThreadCount가 작동 안하게 하기 위함
		bool m_stopping;

		// 멤버 함수 중에 pooled object를 쓰는데, pool이 먼저 파괴되면 안되므로.
		CFavoritePooledObjects::PtrType m_depend_CFavoritePooledObjects;

	public:
		CThreadPoolImpl();
		~CThreadPoolImpl();

	private:
		static void StaticWorkerThread_Main(void* ctx);
		void WorkerThread_Main(CWorkerThread* thisThread);

	public:
		virtual void SetDesiredThreadCount(int threadCount) PN_OVERRIDE;
		int GetThreadCount();

		virtual void Process(int waitTimeMs) PN_OVERRIDE;
		void Process(CWorkResult* workResult, int waitTimeMs);
		void SetEventSink(IThreadPoolEvent* eventSink);
		
		inline void AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(m_cs);
		}

		inline void AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(m_cs);
		}

		bool ContainsCurrentThread();

		void GetThreadInfos(CFastArray<CThreadInfo>& output);

		bool RegisterReferrer(IThreadReferrer* referrer);
		void UnregisterReferrer(IThreadReferrer* referrer);

	private:
		void ProcessAllEvents(CWorkerThread* thread, CWorkResult* workResult, int waitTime);

		bool PopCustomValueEvent(IThreadReferrer** referrer, CustomValueEvent& customValue);
		
		bool IncreaseReferrerUseCount(IThreadReferrer* referrer);
		void DecreaseReferrerUseCount(IThreadReferrer* referrer);

		void CreateWorkerThread(int threadCount);

	public:
		enum Choose 
		{
			ChooseAny,  // 아무거나 (+1 MOD N) 선택
#if !defined (_WIN32)
			ChooseHavingFewSockets,  // 가장 적은 소켓을 가진 스레드
			ChooseIdleOrAny  // 놀고 있는 스레드를 고르되 노는 것이 없으면 아무거나 선택
#endif
		};
		CWorkerThreadPtr GetWorkerThread_NOLOCK(Choose choose);

		void AssociateSocket(CFastSocket* socket);

		bool PostCustomValueEvent(IThreadReferrer* referrer, CustomValueEvent customValue);

#if !defined (_WIN32)
		//void WriteEvnetfdToWorkerThreadHavingSocket(CSuperSocket* socket);
		void HandOverSocketAndEvent_NOLOCK(CWorkerThread* src, CWorkerThread* dest);
#endif
	};

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}
