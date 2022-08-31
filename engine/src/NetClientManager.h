#pragma once
#include "FavoriteLV.h"
#include "../include/IRmiHost.h"
#include "ThreadPoolImpl.h"
#include "IntraTracer.h"
#include "GlobalTimerThread.h"
#include "Upnp.h"


namespace Proud
{

	// CNetClient 인스턴스 관리자, Heartbeat 수행 스레드, io completion 수행 스레드 풀, collected garbages
	class CNetClientManager :
		/* public ISendSpeedMeasurerDg,*/
		public DllSingleton<CNetClientManager>
	{
	public:
		// 이 객체가 직접 가진 멤버들만 보호한다.
		// 가령 garbage list 등을 보호함.
		// NC 각각 내부는 보호하지 않는다.
		// 따라서, NetClient의 critSec을 잠금한 상태에서 이것을 잠그지 말 것. 데드락 일어남.
		CriticalSection m_critSec;
	private:
#if defined(_WIN32)
		CTracer::PtrType m_usingTracer;
#endif

		//CRefCountHeap::PtrType m_dependRefCountHeap;
		//CByteArrayPtrManager::PtrType m_pooledObjects;	// ByteArrayPtr의 manager가 먼저 파괴되지 못하게 함
		CGlobalTimerThread::PtrType m_globalTimer;			// GetPreciseCurrentTime 때문에

		//		CByteArrayHolderAlloc::PtrType m_dependsByteArrayAllocator;
		//		CFastHeapForArray::PtrType m_dependsFastArrayAllocator;
	public:
		void OnCustomValueEvent(const ThreadPoolProcessParam& param, CWorkResult* workResult, CustomValueEvent customValue);

//		void Heartbeat_EveryNetClient();

//		void DoGarbageFree();
		//void OnCompletionPortWarning(CompletionPort *port, const PNTCHAR* msg);

		//void ProcessOldGarbages();
//		void ProcessIoEvent(CNetworkerLocalVars &lv, uint32_t waitTime);

		// NC들을 위한 thread pool.
		// 지금은 1개의 스레드만 가지지만, 추후 NC stress test를 위해 여러 스레드를 가지게 하자.
		// NC의 수가 0이면 이 thread pool도 없어지게 하는 것이 visual leak detector에 의한 거짓 누수도 막음.
	public:

		//volatile bool m_stopThread;

	//private:

		//int64_t m_lastGarbageFreeTime;
	private:
		//static void StaticWorkerProc(void* ctx);
		//void WorkerProc();

		void ShowThreadUnexpectedExit(const PNTCHAR* where, const PNTCHAR* reason);

	public:
		//volatile bool m_useNetworkerThread;

		// NetClient의 networker thread를 multi thread pool로 지정할 경우 여기가 액세슨된다.
		RefCount<CThreadPoolImpl> m_netWorkerMultiThreadPool;

		// NetClient의 networker thread를 zero thread pool로 지정할 경우 여기가 액세스된다.
		RefCount<CThreadPoolImpl> m_netWorkerZeroThreadPool;

		// networker thread를 사용자가 비활성화했을 때, (UseNetworkerThread OFF시) 그 동면을 멈춰야 할 경우 이 세마포어가 건드려진다.
		// pthread 호환성을 위해 Proud.Event 대신 이것이 쓰임.
		//Semaphore m_stopHibernateSemaphore;

	private:

		// 예전엔 Event 객체로 두어서 mmtimer가 event set 후 Wait(0) 방식이었다. 그러나 이것보다는 bool var로 가는게 더 신뢰적. SetEvent를 MMCallback에서 실행하는 것 보다는 변수 세팅이 더 빠르니까.
		// heartbeat 신호가 쌓인 바가 있으면 1 그렇지 않으면 0
		//int32_t m_heartbeated;

		static void StaticThreadProc(void* ctx);
		void ThreadProc();
		volatile bool m_stopThread;
		Thread m_workerThread;

		// #if !defined(_WIN32)
		//		pthread_t m_workerThreadID;
		// #else
		//		uint32_t m_workerThreadID;
		// #endif
	public:
		int m_disconnectInvokeCount;

		// 활성화중인 NC의 갯수들
		volatile int32_t m_instanceCount;
#if defined(_WIN32)
		CUpnp m_upnp;
#endif
		void AdjustThreadCount();

		//
		// #if defined(_WIN32)
		//		// CompletionPort class는 fake mode를 지원하므로 safe.
		//		CAutoPtr<CompletionPort> m_completionPort;
		// #else	// Android/IOS
		//		CHeldPtr<CIoEventReactor>	m_reactor;
		// #endif

		CNetClientManager();
		virtual ~CNetClientManager();

		//void GetThreadInfos(CFastArray<CThreadInfo>& output);

// #if !defined(_WIN32)
//		inline pthread_t GetWorkerThreadID()
//		{
//			return m_workerThreadID;
//		}
// #else
//		inline uint32_t GetWorkerThreadID()
//		{
//			return m_workerThreadID;
//		}
// #endif
//
// #if defined(_WIN32)
//		inline HANDLE GetWorkerThreadHandle()
//		{
//			return m_workerThread->Handle;
//		}
// #endif

		//		int64_t GetCachedTime();
		//		inline int64_t GetCachedElapsedTime()
		//		{
		//			return GetPreciseCurrentTimeMs();
		//		}

		//void SendReadyInstances_Add(CNetClientImpl* inst);

		void RequestMeasureSendSpeed(bool enable);

#ifdef _WIN32
#pragma push_macro("new")
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif
	};

	DECLARE_DLL_SINGLETON(PROUD_API, CNetClientManager);
}
