/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "ThreadPoolImpl.h"

namespace Proud
{
	void CThreadPoolImpl::RunAsync(RunAsyncType type, RunAsyncFunction func, void* context)
	{
		// 유지보수시 주의: 이 함수는 ProudNet 2에서 매우 자주 호출한다. 즉 성능에 민감하다. 따라서 lambda 식 버전이 아닌 이 함수는 있어야 하며 람다식 버전에 의한 성능 하락이 있어서는 안된다.

		// 개요: 위키'ThreadPool의 RunAsync 기능' 참고
		CriticalSectionLock lock(m_cs, true);
		if (type == RunAsyncType_Single) // 한 스레드만
		{
			// 아무 스레드를 하나 고른다.
			CWorkerThread* workerThread = GetWorkerThreadRandomly();
			workerThread->RunAsyncWorkQueue_PushBack(func, context);
#ifdef _WIN32
			// IOCP에게 깨우게 시킨다.
			// 이걸로 다른 스레드가 깨더라도, 위 고른 스레드는 CPU 타임이라는 뜻이므로, 조만간 이 일을 잘 처리할 것이다.
			m_completionPort->PostCompletionStatus();
#else
			// #UNIX_POST_EVENT 해당 스레드를 깨운다.
			//workerThread->m_ioNotifier->PostEvent();
#endif
		}
		else	// 모든 스레드가 동시에 같은 일을 처리
		{
			// 각 스레드에게 일을 넣는다.
			if (m_workerThreads.GetCount() == 0)
			{
				m_zeroThread->RunAsyncWorkQueue_PushBack(func, context);
			}
			else
			{
				for (auto i =m_workerThreads.begin();i!= m_workerThreads.end();i++)
				{
					i.GetSecond()->RunAsyncWorkQueue_PushBack(func, context);
				}
			}

			// 일단 다 넣고 시그널을 울리자. 다 넣기 전에 시그널을 울리면 뒷북 문제가 있을 수 있다.

#ifdef _WIN32
			// 스레드 수만큼 IOCP를 꺠운다.
			// 이걸로 다른 스레드가 깨더라도, 위 고른 스레드는 CPU 타임이라는 뜻이므로, 조만간 이 일을 잘 처리할 것이다.
			for (int i = 0; i < m_workerThreads.GetCount(); i++)
			{
				// IOCP에게 깨우게 시킨다.
				// 각 스레드가 다 깨어나야 하니 스레드 수만큼 한다.
				m_completionPort->PostCompletionStatus();
			}
#else
			// #UNIX_POST_EVENT
			// 각 스레드를 깨운다.
			//for (auto i : m_workerThreads)
			//{//
			//	i.GetSecond()->m_ioNotifier.PostEvent();
			//}
#endif
		}
	}

	// ThreadPool.RunAsync에 의해 추가된 RunAsync 항목에 대한 유저콜백 함수를 실행한다.
	// 참고: 위키문서 'ThreadPool의 RunAsync 기능'
	void CThreadPoolImpl::ProcessRunAsyncWorkItems(CWorkerThread* workerThread)
	{
		/* 이 스레드 전용으로 가진 작업 큐에서 꺼내서 처리한다.
		Q: 루프 돌면 다른 스레드에서 기아화 안해요?
		A: context switch 횟수가 적어져서 더 좋고요, 이 스레드가 다 처리하지 못할 만큼 많은 작업이면 알아서 다른 스레드가 일을 가져갑니다. 즉 지나칠 정도로 기아화 안해요.
		*/
		RunAsyncWork runAsyncWork;
		while (workerThread->RunAsyncWorkQueue_PopFront(runAsyncWork))
		{
			// 함수를 실행
			runAsyncWork.m_function(runAsyncWork.m_context);
		}
	}

	// 랜덤으로 스레드 하나를 꺼내준다.
	CWorkerThread* CThreadPoolImpl::GetWorkerThreadRandomly()
	{
		// 없으면 이거라도
		if (m_workerThreads.GetCount() == 0)
			return m_zeroThread;

		// 난수값
		static int num = 0;
		num += (int)GetPreciseCurrentTimeMs();
		int num2 = num % m_workerThreads.GetCount();

		// CFastMap은 iter가 빠르다.
		auto it = m_workerThreads.begin();

		for (int i = 0; i < num2; i++)
		{
			it++;
		}

		return it.GetSecond();
	}

	// 이 스레드의 RunAsync 작업항목에 추가한다.
	void CWorkerThread::RunAsyncWorkQueue_PushBack(RunAsyncFunction function, void* context)
	{
		CriticalSectionLock lock(m_runAsyncWorkListCritSec, true);
		RunAsyncWork a;
		a.m_context = context;
		a.m_function = function;
		m_runAsyncWorkList.AddTail(a);
	}

	bool CWorkerThread::RunAsyncWorkQueue_PopFront(RunAsyncWork& outRunAsyncWork)
	{
		CriticalSectionLock lock(m_runAsyncWorkListCritSec, true);
		if (m_runAsyncWorkList.IsEmpty())
			return false;
		m_runAsyncWorkList.GetHead(&outRunAsyncWork);
		m_runAsyncWorkList.RemoveHeadNoReturn();
		return true;
	}


	bool CWorkerThread::RunAsyncWorkQueue_IsEmpty() const
	{
		CriticalSectionLock lock(m_runAsyncWorkListCritSec, true);
		return m_runAsyncWorkList.IsEmpty();
	}
}
