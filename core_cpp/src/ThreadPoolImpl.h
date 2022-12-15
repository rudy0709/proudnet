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
#include "PooledObjects_C.h"

namespace Proud
{
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

    using namespace std;

    class CThreadPoolImpl;

    class ThreadPoolProcessParam
    {
    public:
        // 평소에는 true이지만, false이면 PN 사용자 정의 함수 콜백을 하지 않는다.
        // NetClient or NetServer의 delete 수행중 thread pool을 정리하기 위해 블러킹 되는 동안 user callback이 일어나는 경우
        // 사용자가 실수로 또 Disconnect or Stop 을 호출하는 경우를 막기 위함.
        bool m_enableUserCallback;

        ThreadPoolProcessParam()
        {
            m_enableUserCallback = true;
        }

    };


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
        void Accumulate(CWorkResult& midResult);
    };

    class RunAsyncWork
    {
    public:
        RunAsyncFunction m_function;
        void* m_context;
    };

    // CThreadPoolImpl의 각 스레드
    class CWorkerThread
    {
    public:
        CThreadPoolImpl* m_ownerThreadPool;

        // 스레드 핸들
        ThreadPtr m_thread;

        // true이면 이 스레드는 곧 종료한다.
        // SetDesiredThreadCount에 의해 세팅되기도 함.
        volatile bool m_stopThisThread;

        CWorkerThread()
        {
            m_ownerThreadPool = NULL;
            m_stopThisThread = false;
        }
    private:
        // ThreadPool.RunAsyncWork에 의해 쌓인 것들
        CFastList2<RunAsyncWork, intptr_t> m_runAsyncWorkList;
        _Guarded_by_(m_runAsyncWorkList) mutable CriticalSection m_runAsyncWorkListCritSec;
    public:
        void RunAsyncWorkQueue_PushBack(RunAsyncFunction function, void* context);
        bool RunAsyncWorkQueue_PopFront(RunAsyncWork& outRunAsyncWork);
        bool RunAsyncWorkQueue_IsEmpty() const;

#ifndef _WIN32
        // epoll은 여러 스레드에서 같은 fd에 대해서 io avail를 처리하는 것이 문제가 많다.
        // 따라서 부득이하게 각 스레드가 epoll을 가진다.
        CHeldPtr<CIoReactorEventNotifier> m_ioNotifier;
#endif
    };
    typedef RefCount<CWorkerThread> CWorkerThreadPtr;

    /* "Fix NetCore dangle while Socket IO event.pptx"의,
    “NetCore의 모든 socket이 더 이상 X를 액세스하지 않는다“를 확인시켜주는 용도의 객체 X.
    NetCore의 delete is safe를 체크하기 위한 용도로 쓰인다.
    X가 존재하면 “NetCore의 socket 중 최소 하나가 X를 액세스하고 있다”를 의미한다.
    NetCore와 SuperSocket들은 X를 참조한다.
    NetCore는 소유를, X들은 약한 참조를 한다. */
    class  CReferrerHeart
    {
    public:
        CReferrerHeart() {}
        ~CReferrerHeart() {}
    };

    /* thread pool에 등록되는 객체.
    thread pool에 사용자가 post한 custom event를 처리하는 주체이기도 하다.
    thread pool이 다루는 socket들을 소유하기도 한다. NC,NS,LC,LS등이 이것을 상속받는 이유.

    스레드풀 보다 오래 살 수 없다. thread pool이 파괴될 때 이 객체가 아직 등록되어 있으면 안된다.
    1개 referrer는 여러 thread pool에 종속될 수 있다. (예: user worker thread, net worker thread) */
    class  IThreadReferrer
    {
    private:
        // 특정 쓰레드가 참조하는 이 객체에 대하여 Dangle pointer access 를 방지하기 위한 변수들
        CriticalSection m_cs;

        // 이건 로컬호스트 모듈이 시작할 때와 끝낼 때 뺴고는 액세스하지 말자.
        // 대신 m_accessHeart를 액세스하자.
        shared_ptr<CReferrerHeart> m_referrerHeart;

        /* ReferrerHeart가 소멸해야 하는 상황이고, 더 이상 아무도 액세스하고 있지 않는지를 검사하는 용도로 쓰인다.
        자세한 것은 Fix NetCore dangle while Socket IO event.pptx에 있다.
        Disconnect or Stop을 호출한 스레드 외 다른 스레드에서
        이걸 lock 하지 못하면, NetCore가 파괴중이므로, 더 이상 뭔가를 해서는 안된다. */
        weak_ptr<CReferrerHeart>   m_accessHeart;

    public:
        // 쓰레드풀의 쓰레드들로부터 Access 가 가능함을 마킹 한다.
        // 이때부터 이 객체는 임의의 쓰레드에서 랜덤한 때에 액세스 된다.
        // NetCore가 작동을 시작할 때 호출하자.
        void AllowAccess()
        {
            CriticalSectionLock lock(m_cs, true);
            if (m_accessHeart.lock().get() != nullptr)
            {
                return;
            }

            m_referrerHeart.reset(new CReferrerHeart());
            m_accessHeart = m_referrerHeart;
        }

        // 더 이상 쓰레드들로부터 Access 를 하지 말라는 마킹을 한다.
        // NetCore의 파생 클래스에서, 작동을 멈추는 기능을 완료할 때 이걸 호출하자.
        void StopAccess()
        {
            CriticalSectionLock lock(m_cs, true);
            m_referrerHeart = shared_ptr<CReferrerHeart>();
        }

        /* 이 객체를 다른 곳에서 액세스 할 수 있다는 상황을 만든다.
        예로 A 라는 객체에서 이 객체의 GetReferrerHeart 를 콜하여 shared_ptr 데이터를 가져오면
        이때부터 이 객체는 "임의의 곳에서 액세스 되고 있는 객체" 의 상황이 된다.
        따라서 쓰레드풀은 그 임의의 곳에서의 소유권을 포기하지 않는 한, 이 객체의 Access 는 안전함을 보장한다.

        NetCore를 액세스해도 되는지 체크해서,
        액세스할 수 있으면 성공적으로 lock을 한다.
        콜러는, 이걸 호출 후 output is not null일때만 NetCore를 액세스하자.
        만약 이게 null이면 NetCore는 파괴중이므로 액세스하지 말아야 한다. */
        void TryGetReferrerHeart(shared_ptr<CReferrerHeart>& output)
        {
            output = m_accessHeart.lock();
        }


    public:
        // referrer 접근을 위한 friend 선언
        friend class CThreadPoolImpl;

    public:

        // 마지막으로 heartbeat을 시행한 시간.
        // 너무 자주 heartbeat call을 안하기 위해.
        int64_t m_lastHeartbeatTimeMs;

        // 마지막으로 issue send 를 시행한 시간.
        // 너무 자주 call을 안하기 위해.
        volatile int64_t m_lastIssueSendOnNeedTimeMs;

        /* heartbeat이나 issue send on need 등에 의해 사용중인 카운트 값.
        EveryThreadReferrer_HeartbeatAndIssueSendOnNeed에 의해 액세스 중이면 +1이 되고,
        액세스가 끝나면 -1이 된다.

        이 값은 CThreadPoolImpl.m_cs에 대한 lock 즉 T lock이 된 때에만 증가하고,
        감소는 T lock 없이도 한다.
        =0 check 즉 safe to delete를 검사하는 것도 T lock이 된 때에만 한다.

        이렇게 하면, 안전해진다.

        CThreadPoolImpl 객체 T가 있고 T가 가진 IThreadReferrer R이 있다고 치자.

        안전하지 않은 케이스는 다음과 같다.

        1. T를 액세스 중인데 갑자기 T가 다른 스레드의 UnregisterReferrer에 의해 등록 해제되는 경우
        2. UnregisterReferrer에서 지우려고 하는데 갑자기 T가 다른 스레드에 의해 액세스 되는 경우

        값 증가는 T를 액세스할 때이고, =0 체크는 T를 delete할 때에 하는 일이다.
        그런데 이 두 작업이 lock T 상태에서만 한다. 따라서 두 과정이 동시에 일어날 수 없다.
        값 감소는 돌발로 일어날 수 있지만, 이는 위 문제를 일으키는데 영향을 안 준다.

        따라서 안전하다.
        */
        //volatile int32_t m_useCount;

        // Multi-Threaded 모델시 networker thread의 비병렬 실행을 보장해준다.
        volatile int32_t m_processingNetworkerThreadWork;

        // 매 1ms 혹은 그 이하마다 호출되는 함수
        virtual void ProcessHeartbeat() = 0;

        // 각 remote에 대해서 issue send를 한다. 단, 보낼 데이터가 있으면 보낸다.
        // returns: true이면 최소 1회의 issue send syscall을 했음.
        virtual bool EveryRemote_IssueSendOnNeed(int64_t currTime) = 0;

        // 사용자 정의 코드를 실행한다.
        // returns: true이면 최소 1회의 user work를 했음을 의미한다.
        virtual void DoUserTask(const ThreadPoolProcessParam& param, CWorkResult* workResult, bool& holstered) = 0;

        /* custom event 콜백 루틴. 사용자가 구현해야 함.
        일정 시간마다 혹은 send issued without coalesce 등 다양한 상황에서 호출된다.

        사용자가 CThreadPoolImpl.Post를 호출하면
        thread pool의 worker thread 중 하나가 이 함수를 콜백한다.

        IThreadReferrer가 CThreadPoolImpl를 등록된 동안에만 콜백된다.
        미등록된 경우 위 Post()는 무시된다.

        workResult: 실행 결과를 채운다.
        customValue: 발생한 이벤트의 종류. user ptr 등이 안 들어가는 이유는, epoll이 그렇게까지 제공되지 않기 때문이다. */
        virtual void OnCustomValueEvent(const ThreadPoolProcessParam& param, CWorkResult* workResult, CustomValueEvent customValue) = 0;

        IThreadReferrer() :
            m_lastHeartbeatTimeMs(0),
            m_lastIssueSendOnNeedTimeMs(0),
            m_processingNetworkerThreadWork(0)
        {
        }

        virtual ~IThreadReferrer();

        // 사용자 마음대로 채우자.
        // PNTest에서 건드린다.
        CFastArray<String> m_debugInfo;

        PROUD_API void PrintDebugInfo(int indent);
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

        주의: Zero thread mode용 함수인 Process()에서만 사용할 것! */
        CriticalSection m_zeroThreadCritSec_XXX;

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
        CFastMap2<CWorkerThread*, CWorkerThreadPtr, int> m_workerThreads;

        // 곧 죽을 worker thread들
        CFastMap2<CWorkerThread*, CWorkerThreadPtr, int> m_garbagedThreads;

        /* worker thread를 전혀 갖고 있지 않은, 즉 zero thread mode일때
        사용자가 Process()를 호출한다.
        Process() 또한 eventfd나 epoll등을 요구하는데
        아래는 그것을 담기 위한 역할을 한다. */
        CWorkerThreadPtr m_zeroThread;

    private:
        // thread pool에 특화된 이벤트 콜백
        IThreadPoolEvent* m_eventSink_NOCSLOCK;

        // 각 referrer의 상태
        class CReferrerStatus
        {
        public:
            // 현재 이 referrer에 대한 이벤트가 실행중인 스레드의 수.
            // 사용 중이면 +1, 사용이 완료되면 -1
            // m_unregisterRequested=true이면 더 이상 처리하지 않습니다.
            int32_t m_customActionUseCount;

            // Unregister 요청을 받으면 True로 설정합니다.
            bool m_unregisterRequested;

            // true이면, thread pool에서 event poll을 할 때, heartbeat과
            // issue send on need를 한다.
            bool m_asNetworkerThread;

            // true이면, 상기 상황에서, DoUserTask 즉 user work item을
            // 처리한다.
            bool m_asUserWorkerThread;

            // ThreadPoolImpl이 이것을 액세스하기 전에 
            // lock을 얻기를 시도한다. 얻어짐이 성공할 때만
            // 이 객체를 액세스한다.
            // 그렇게 댕글링을 예방한다.
            weak_ptr<CReferrerHeart>	m_accessHeart;

            CReferrerStatus()
                : m_customActionUseCount(0)
                , m_unregisterRequested(false)
                , m_asNetworkerThread(false)
                , m_asUserWorkerThread(false)
            {
            }
        };

        typedef RefCount<CReferrerStatus> CReferrerStatusPtr;

        // 이 thread pool에 의존되고 있는 처리 객체들. 가령 NC,NS,LC,LS다.
        typedef CFastMap2<intptr_t, CReferrerStatusPtr, int> Refferers;
        Refferers m_referrers;

        // 사용자가 원하는 스레드 수
        // 금방 thread 수는 이 값에 도래하게 된다.
        volatile int m_desiredThreadCount;

        // GetWorkerThread_NOLOCK(ChooseAny)에서 산출을 위한 Index 값
        int m_chooseIndex;

        // 종료 과정에서, SetDesiredThreadCount가 작동 안하게 하기 위함
        bool m_stopping;


    public:
        CThreadPoolImpl();
        ~CThreadPoolImpl();

    private:
        static void StaticWorkerThread_Main(void* ctx);
        void WorkerThread_Main(CWorkerThread* thisThread);

    public:
        virtual void SetDesiredThreadCount(int threadCount) PN_OVERRIDE;
        PROUD_API int GetThreadCount();

        virtual void Process(int waitTimeMs) PN_OVERRIDE;

        void ProcessButDropUserCallback(IThreadReferrer* referrer, int waitTimeMs);
    private:
        void ZeroThreadPool_Process(IThreadReferrer* referrer, CWorkResult* workResult, int waitTimeMs, const ThreadPoolProcessParam& param);
    public:
        void Process(IThreadReferrer* referrer, CWorkResult* workResult, int waitTimeMs);
        void SetEventSink(IThreadPoolEvent* eventSink);

        inline void AssertIsLockedByCurrentThread()
        {
            Proud::AssertIsLockedByCurrentThread(m_cs);
        }

        inline void AssertIsNotLockedByCurrentThread()
        {
            Proud::AssertIsNotLockedByCurrentThread(m_cs);
        }

        PROUD_API bool ContainsCurrentThread();

        PROUD_API void GetThreadInfos(CFastArray<CThreadInfo>& output);

        PROUD_API void RegisterReferrer(IThreadReferrer* referrer, bool doNetworkThreadTask);
        PROUD_API void UnregisterReferrer(IThreadReferrer* referrer);

    private:
        void ProcessAllEvents(
            IThreadReferrer* referrer,
            CWorkerThread* workerThread,
            CWorkResult* workResult,
            int maxWaitTimeMs,
            const ThreadPoolProcessParam& param);

        void Process_SocketEvents_(CIoEventStatusList& polledEvents, CWorkerThread* workerThread);

        bool ThreadReferrer_Process(IThreadReferrer* selectedReferrer, const ThreadPoolProcessParam& param, CWorkResult* workResult, bool& holstered);

        bool PopCustomValueEvent(IThreadReferrer** referrer, CustomValueEvent& customValue);

        bool IncreaseReferrerUseCount(IThreadReferrer* referrer);
    public: // finally{}의 대체재에서 호출되어야 하므로 public
        void DecreaseReferrerUseCount(IThreadReferrer* referrer);
    private:
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

        PROUD_API void AssociateSocket(const shared_ptr<CSuperSocket>& socket, bool isEdgeTrigger);

        PROUD_API bool PostCustomValueEvent(IThreadReferrer* referrer, CustomValueEvent customValue);
        void ProcessCustomValueEvents(const ThreadPoolProcessParam& param, CWorkResult* workResult);

#if !defined (_WIN32)
        //void WriteEvnetfdToWorkerThreadHavingSocket(CSuperSocket* socket);
        void HandOverSocketAndEvent_NOLOCK(CWorkerThread* src, CWorkerThread* dest);
#endif

        virtual void RunAsync(RunAsyncType type, RunAsyncFunction func, void* context) PN_OVERRIDE;
        void ProcessRunAsyncWorkItems(CWorkerThread* workerThread);
        CWorkerThread* GetWorkerThreadRandomly();
    };

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}
