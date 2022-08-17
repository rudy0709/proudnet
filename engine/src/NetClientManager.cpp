#include "stdafx.h"
#include "NetClientManager.h"
#include "ThreadPoolImpl.h"
//#include "FavoriteLV.h"
//#include "CustomValueEventQueue.h"

namespace Proud
{

	void CNetClientManager::StaticThreadProc(void* ctx)
	{
		CNetClientManager* man = (CNetClientManager*)ctx;
		man->ThreadProc();
	}

	CNetClientManager::CNetClientManager()
		: m_stopThread(false)
		, m_workerThread(StaticThreadProc, this, true)

#if defined(_WIN32)
		, m_upnp(this)
#endif
	{
		// static CPool g_pool이 Manager보다 먼저 깨지는 현상을 방지하기 위해서 수정 modify by kdh
		m_globalTimer = CGlobalTimerThread::GetSharedPtr();

#if defined(_WIN32)
		m_usingTracer = CTracer::GetSharedPtr();
#endif

		//m_workerThreadID = 0;
		m_disconnectInvokeCount = 0;

		m_instanceCount = 0;

		// MultiThread 용 NetWorker 쓰레드풀이라도 기본 0개로 작동하다가
		// 어느 특정 NetClient 인스턴스에서 MultiThreaded 옵션이 있다면
		// 그때 쓰레드를 늘린다.
		m_netWorkerMultiThreadPool = RefCount<CThreadPoolImpl>((CThreadPoolImpl*)CThreadPool::Create(NULL, 0)); // 스레드 0개부터 시작. NC가 늘어나면 그때 같이 늘어난다.
		m_netWorkerZeroThreadPool = RefCount<CThreadPoolImpl>((CThreadPoolImpl*)CThreadPool::Create(NULL, 0));

		//m_netWorkerThread->RegisterReferrer(this);

		m_workerThread.Start();

	}

	CNetClientManager::~CNetClientManager()
	{

#if defined(_WIN32)
		m_upnp.IssueTermination();
		m_upnp.WaitUntilNoIssueGuaranteed();
#endif

		/*if(m_sendSpeedMeasurer)
		{
		m_sendSpeedMeasurer->IssueTermination();
		m_sendSpeedMeasurer->WaitUntilNoIssueGuaranteed();
		}*/

		//m_stopThread = true;
		//m_stopHibernateSemaphore.Release();

		//왜 주석으로 막았는지를 기재 바랍니다. 여기가 실행중일 때는 NC 인스턴스가 하나도 없음이 보장되는데
		// 굳이 Join을 안해야 하는 이유가 뭔지가 설명이 필요합니다.

		/* ==>	1. 사용자가 호출하는 ThreadPoolImpl.SetDesiredThreadCount 함수는 non-blocking 함수입니다.
				2. Windows와는 달리 Linux에서 별도의 속성없이 생성한 스레드는 반드시 pthread_join 함수를 호출해주어야만 메모리 해제가 이루어지게 됩니다.

				위의 1번을 만족시키기 위해서는 2번의 pthread_join을 하지 않고도 자동으로 메모리를 해제주는 기능이 필요합니다.

				다행히 Linux에서는 이러한 기능을 제공하고 있으며, 이 기능을 사용하게되면 더 이상 Join을 할 수 없게되버립니다.
				다시 말해 생성된 스레드는 반드시 메인 스레드가 종료되기 이전에 종료됨을 보장을 해주어야합니다.

				이러한 이유로 ThreadPoolImpl 소멸자에서는 생성한 스레드들이 모두 종료되기 전까지는 Blocking되도록 구현 되어있습니다.
				그렇기 때문에 아래의 Join은 해주시지 않으셔도 됩니다.
		*/
		//m_netWorkerThread->Join(); // 스레드 종료를 대기

		m_stopThread = true;
		m_workerThread.Join();

#ifdef _WIN32
//		m_upnp.Free();
// #else
// 		m_reactor.Free();
#endif

		//m_netWorkerThread->UnregisterReferrer(this);
	}


	void CNetClientManager::ShowThreadUnexpectedExit(const PNTCHAR* where, const PNTCHAR* reason)
	{
		String txt;
		txt.Format(_PNT("(%s): unexpected thread exit with (%s)"), where, reason);
		ShowUserMisuseError(txt.GetString());
	}

	// upnp 처리, objpool shrink 등을 처리한다.
	// NOTE: 각 NC에 대한 처리는 각 NC의 동명 함수에서 처리한다.
	void CNetClientManager::ThreadProc()
	{
		// NOTE:각 NC에 대한 heartbeat은 Proud.CNetCoreImpl.m_periodicPoster_Heartbeat에서 하고 있음므로 여기서는 신경 끄자.

		while (!m_stopThread)
		{
			CriticalSectionLock lock(m_critSec, true);

#if defined(_WIN32)
			//if (m_upnp)
			//{
			//	m_upnp->Heartbeat();
			//}
#endif

			// 모바일 플랫폼에서 돌아가는 넷클라는 메모리에도 민감. 따라서 이걸 콜 하자.
			// (서버는 어차피 메모리도 남아돌고 이거 체크하기 위해 per-rc lock을 거는 것도 짜증. 따라서 안하는 것이 나을지도.)
			PooledObjects_ShrinkOnNeed_ClientDll();

			//CalcAverageElapsedTime();
			//DoGarbageFree();

			AdjustThreadCount();

			// 100 정도면, 지나친 thread context switch도 아니면서 사용자 체험도 떨어뜨리지 않는다.
			// 예전에는 10이었으나, CClassObjectPool의 액세스 량이 상당하기 때문에, 100으로 상향.
			Sleep(100);
		}
	}
	// 	void CNetClientManager::SendReadyInstances_Add( CNetClientImpl* inst)
	// 	{
	// 		CriticalSectionLock	lock(m_cs,true);
	//
	// 		// 리스트에 아직 미등록임에 한해 등록.
	// 		if(!inst->GetListOwner())
	// 		{
	// 			m_sendReadyInstances.PushBack(inst);
	// 		}
	// 	}

// 	void CNetClientManager::DoGarbageFree()
// 	{
// 		//지우자
// 	}

	//void CNetClientManager::OnCompletionPortWarning(CompletionPort* /*port*/, const PNTCHAR* /*msg*/)
	//{
	//	//Console.WriteLine(msg);
	//}

	// NC의 갯수에 따라 스레드 수를 조정한다.
	// use networker thread OFF인 경우나 NC가 없으면 스레드 수를 0으로 간다.
	// 매우 자주 호출되고 있다.
	void CNetClientManager::AdjustThreadCount()
	{
		// 지금은 critsec이 모든 NC를 일제히 잠그므로 스레드 수를 늘릴 필요가 없지만
		// 나중에는 각 NC마다 critsec을 가지게 한 후 NC 개수에 따라 스레드 수를 늘려야 할거다.
		CriticalSectionLock lock(m_critSec, true);
		int num = 1;
		
		if (m_instanceCount > 0)	// NetWorkerThreadModel을 Multi로 설정한 클라이언트가 하나라도 있으면
		{

#ifdef KERNEL_RESOURCE_SHORTAGE
			// 모바일과 PS4에서는 더미 클라를 띄우더라도 프로세스 당 스레드 수가 많이 협소하므로 적게 쓰도록 하자.
			num = 1;
#else
			num = GetNoofProcessors();
#endif
			// NetClient 객체 중 한개라도 NetWorkerThreadModel 을 MultiThreaded 로 지정하였을 때
			// 그때 코어 갯수만큼 쓰레드를 늘린다.
		}

		m_netWorkerMultiThreadPool->SetDesiredThreadCount(num);
	}

// 	void CNetClientManager::GetThreadInfos(CFastArray<CThreadInfo>& output)
// 	{
// 		m_netWorkerThread->GetThreadInfos(output);
// 	}

}
