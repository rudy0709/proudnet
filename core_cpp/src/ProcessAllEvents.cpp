#include "stdafx.h"
#include "ThreadPoolImpl.h"
#include "ScopeExit_CAS.h"

namespace Proud
{
	/* iocp or epoll에 누적되어 있는 이벤트를 모두 꺼내 처리한다.
	workResult에 실행 결과가 채워진다.
	이 함수는 내부적으로 아무런 이벤트가 없을 때까지 반복한다. 따라서 콜러는 이것을 실행한 결과를 굳이 따로 검사할 필요가 없다.

	서버에서 syscall의 횟수를 줄이기 위해, 200us 정도의 인터벌마다 event를 처리하고, 모두 넌블러킹으로 처리한다.
	자세한 것은 optimize PQCS and GQCS.pptx에.

	selectedReferrer: 
	null이면 모든 referrer에 대해 issue send on need and heartbeat을 한다. 
	not null이면 지정한 것 하나의 referrer에 대해 한다.

	enableUserCallback가 false이면 pop은 하지만 custom event에 대한 이벤트 콜백은 하지 않는다. */
	void CThreadPoolImpl::ProcessAllEvents(
		IThreadReferrer* selectedReferrer,
		CWorkerThread* workerThread, 
		CWorkResult* workResult,
		int maxWaitTimeMs, 
		const ThreadPoolProcessParam& param)
	{
		/* 주의: 만약 이 함수를 추후 waitable 성질을 갖게 수정할 경우, 다음을 주의할 것.

		얻어온 것이 있지만 timeOut 값 이하의 시간이 소요되었다.
		한번 더 루프를 돌 때는 timeOut=0으로 즉 넌블러킹으로 추가적 poll을 해야 한다.
		MaxPollCount만큼만 poll을 하기 때문이다.

		*/

		AssertIsNotLockedByCurrentThread();

		while (true)  // GQCS or epoll에서의 pop을 한 것이 있으면, 대기 없이 한번 더 한다. 그리고 더 이상 나오는게 없으면 본 함수는 리턴한다.
		{
			CWorkResult midResult1;
			/* 각 thread referrer에 대해 heartbeat 및 issue send on need를 한다.
			예전에는 IThreadPoolReferrer R에 대한 custom event가 들어가 있는 경우에만
			처리를 했으나, CustomValueEvent_SendIssued, CustomValueEvent_Heartbeat가
			매우 많은 경우 PQCS,GQCS,send,recv등의 호출이 너무 많아짐으로 인해
			서버 CPU사용량이 크게 증가했다.
			게다가 게임 개발에서는 1ms 이하의 처리 딜레이는 문제가 없다.
			차라리 1ms의 처리 딜레이를 희생시키고, syscall의 횟수를 줄이는 것이 훨씬 효과적이다.
			따라서 여기서 issue send on need와 heartbeat 처리를 모두 수행한다. */
			bool holstered = false;
			bool processedAtLeastOneIssueSend = ThreadReferrer_Process(selectedReferrer, param, &midResult1, holstered);
			workResult->Accumulate(midResult1);

			CWorkResult midResult2;

			// 위와 같은 이유로, 각 IThreadRefferer의 custom value event를 여기서 한 큐에 처리한다.
			ProcessCustomValueEvents(param, &midResult2);
			workResult->Accumulate(midResult2);

			// 한개라도 처리를 했는지?
			bool processedAtLeastOneCustomValueEvent =
				midResult1.m_processedEventCount > 0 || midResult1.m_processedMessageCount > 0
				|| midResult2.m_processedEventCount > 0 || midResult2.m_processedMessageCount > 0;

			// RunAsync 쌓인 함수콜백을 여기서 실행한다.
			ProcessRunAsyncWorkItems(workerThread);

			/* 위 루틴이 예전에는 while 바깥에 있었다. 그러나, 이렇게 하니 다음 문제가 발생해서, 안으로 옮겼다.
			- socket i/o 처리를 할 것이 계속 들어오면, 딜레이 없이 계속 반복 처리를 하고 있어야 하는데,
			위 처리들은 정작 방치된 후 200us (윈도에서는 실제로는 >1000us) 가 지나서야 뒤늦게 버린다.
			*/

			POOLED_LOCAL_VAR(CIoEventStatusList, polledEvents);

			bool processAtLeastOneSocketEvent = false; // 1개 이상의 socket event를 poll and process 했는가?

			int64_t gqcsWaitTimeMs = GetPreciseCurrentTimeMs();

			// 1024개 넘는 event가 쌓여있는 경우 분할해서 처리를 하되,
			// 분할 구간 사이에서는 EveryThreadReferrer_Process를 생략한다. 시간이 안 지났는데 또 하는 것은 괜한 시간 낭비다.
			while (true)
			{
				polledEvents.ClearAndKeepCapacity(); // 이전 루프에서 재사용되니까 필요

				// NOTE: PS4도 이제 timer event나 custom event가 발생할 때를 위한 socket pair를 쓴다.
				// 따라서 waitTimeMs가 20 미만으로 짧을 필요가 없다.

				// iocp or epoll로부터 socket io event나 custom event를 꺼내온다.
				// NOTE: custom event는 epoll에 엮인 eventfd나 iocp PQCS를 통해 꺼내짐.
#if defined (_WIN32)
				m_completionPort->GetQueuedCompletionStatusEx(polledEvents, maxWaitTimeMs > 0 ? 1 : 0);
#else
				// thread pool 안에서 실행되는 경우: 대응 CWorkerThread의 epoll로부터 pop.
				// CThreadPool.Process()를 사용자가 호출한 경우: m_zeroThread의 epoll로부터 pop.
				// 위 내용을 구현합시다.
				// #UNIX_POST_EVENT 만약 아래 [1]을 1 이외의 다른 값으로 바꾸게 되면, #UNIX_POST_EVENT 에서 지정한 부분의 반응속도가 하락하게 된다.
				// 만약 이 값을 바꿔야 한다면, #UNIX_POST_EVENT 부분을 모두 손대서 epoll or kqueue에다가 post event를 할 수 있도록 만들도록 하자. epollfd_write 등을 써야 하겠다.
				// 다만 이것을 적용한 후에는 eventfd_write 등 자체가 부하를 먹기 때문에 발생하는 성능 하락이 있는지를 StressTest 하도록 하자.
				workerThread->m_ioNotifier->Poll(polledEvents, maxWaitTimeMs > 0 ? 1/*[1]*/ : 0);
#endif
				if (polledEvents.GetCount() <= 0)
					break;

				Process_SocketEvents_(polledEvents, workerThread);
				processAtLeastOneSocketEvent = true;

				if (maxWaitTimeMs > 0) // 블러킹 처리이면
				{
					// 위의 GQCS가 device time이 중간에 있었다면 루프를 빠져나가자.
					if (GetPreciseCurrentTimeMs() - gqcsWaitTimeMs >= 1)
						break;
				}
			}

			/* custom event를 한개도 처리 못하고, socket event도 한개도 없고,
			issue send도 더 이상 할 것이 없으면, 루프를 빠져나간다.
			만약 이들이 한개라도 처리되면, 더 처리할게 있는데도 불구하고 빠져나가서,
			다음 thread context switch에서 처리하게 하면, 큰 딜레이[1]가 발생한다.
			이는 쓸데없는 RTT 증가를 일으킨다.
			[1]: Windows Server의 경우 50ms정도, Windows 8의 경우 5ms정도. */
			// 사용자가 Holster를 해도 루프를 빠져나간다.
			if ((!processAtLeastOneSocketEvent
				&& !processedAtLeastOneCustomValueEvent
				&& !processedAtLeastOneIssueSend) || holstered)
			{
				/* Q: holstered is true이면 루프를 나가는 이 코드. 안전할까요? 만약 ThreadPool이 여러 IThreadReferrer를 처리하고 있으면 어쩌려고요?
				A: 사용자는 NetClient.FrameMove가 일을 중단하고 바로 빠져나오게 하는 것이 목적입니다. 사용자는 이를 위해 user worker thread is single을 설정합니다.
				그러면 NetClient의 user worker thread pool은 zero thread가 되고 이 zero thread pool에 추가된 IThreadReferrer는 1개의 NetClient 뿐입니다.
				이러한 경우 말고는 holster is true가 될 일이 없습니다. 따라서 안전합니다. */
				break;
			}

		}
	}

	// polled socket event들을 처리한다.
	void CThreadPoolImpl::Process_SocketEvents_(CIoEventStatusList &polledEvents, CWorkerThread* workerThread)
	{
		_pn_unused(workerThread);

		for (CIoEventStatusList::iterator i = polledEvents.begin(); i != polledEvents.end(); i++)
		{
			CIoEventStatus& event = *i;

#if defined (_WIN32)
			const shared_ptr<CSuperSocket>& socket = event.m_superSocket;

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
						//int a = 0;
					}
					CSuperSocket::m_firstReceiveDelay_MaxValue = PNMAX(CSuperSocket::m_firstReceiveDelay_MaxValue, diff);
					CSuperSocket::m_firstReceiveDelay_MinValue = PNMIN(CSuperSocket::m_firstReceiveDelay_MinValue, diff);
				}
			}

			socket->OnSocketIoCompletion((void*)&socket, event);
#else
			{
				// supersocket에 대한 이벤트 처리를 수행한다.
				// shared_ptr이므로 비파괴 보장되고 있다.
				// 그리고 이 함수 안에서도 m_accessHeart에 대한 weak to shared ptr access를 시도한다. 
				// 따라서 역시 안전.
				event.m_superSocket->OnSocketIoAvail((void*)&event.m_superSocket, event); // process socket event

				// 아래의 변수는 GetWorkerThread_NOLOCK(ChooseIdleOrAny)의 산출을 위해 사용됩니다.
				++workerThread->m_ioNotifier->m_epollWorkCountPerSec;
			}
#endif

		}
	}


}
