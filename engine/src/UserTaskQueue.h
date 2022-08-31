#pragma once

#include "../include/CriticalSect.h"
#include "../include/Enums.h"
#include "../include/ListNode.h"
#include "RemoteBase.h"
#include "SpinLock.h"

namespace Proud
{
	class CHostBase;

	// user task queue 즉 task subject list를 가지는 객체.
	class IUserTaskQueueOwner
	{
	public:
		/* hostID가 가리키는 host 객체의 주소를 리턴한다.
		caller는 이미 this에 대해 critsec lock을 한 상태이어야 한다. 즉 main lock 상태이어야 한다.
		왜냐하면 이 함수가 값을 리턴한 객체 자체도 main lock이 유지된 상태로 다뤄지기 때문이다.

		파생 클래스에서 이 함수를 impl하는 예시는 다음과 같다.

		NetClient의 경우, 자기자신의 hostID이면 CLoopbackHost_C 객체.
		HostID_Server이면, CRemoteServer_C 객체.
		P2P peer이면, CRemotePeer_C 객체.
		NetServer의 경우, HostID_Server이면 CLoopbackHost_S 객체.

		NetServer의 경우, HostID_Server => CLoopbackHost_S 객체.
		클라 ID이면 => CRemoteClient_S 객체.

		CUS or CUC의 경우, CUC HostID 값에 따라
		ClusterRemoteServer, ClusterRemotePeer, ClusterRemoteClient, ClusterLoopback 객체 계열 중 하나를 리턴한다. */
		virtual shared_ptr<CHostBase> GetTaskSubjectByHostID_NOLOCK(HostID hostID) = 0;

		/* hostID가 유효한 객체인지 알려준다. CHostBase 객체 뿐만 아니라 P2P group 등 유효한 HostID이면 true를 리턴해야 한다.

		Q: GetTaskSubjectByHostID_NOLOCK가 이미 있는데 굳이 이게 또 있어야 해요?
		A: NetServer의 경우, P2P group HostID도 유효한 값입니다. GetTaskSubjectByHostID_NOLOCK는 P2P group에 대해서는 못 찾습니다.
		따라서 이 함수가 별도로 있습니다. */
		virtual bool IsValidHostID_NOLOCK(HostID hostID) = 0;

	};

	/* 스레드 풀: UDP, TCP 공용 worker thread를 하나 두되 이를 IOCP로 처리한다.
	담당 소켓 개수가 많으므로. 단 스레드는 CPU 개수만큼만 둬도 충분하다.
	Worker thread에서 user routine을 직접 실행하지 않고 이 또한 스레드 풀로 던져서 처리하게 만든다.
	이렇게 해서 유저가 어떤 루틴을 만들건 기본 worker thread가 굶지 않게 함을 보장한다. */
	class CUserTaskQueue
	{
	public:
		/* pop()에 의해 뽑혀나온 work item과 task subject.
		task subject는 working 상태이고, use count+1 상태이다.
		이 객체가 파괴되면 working 상태는 not working 상태가 된다.
		그리고 use count+1은 해제된다.
		하지만 아직 user work item이 있으면 work ready 상태로 돌아가고
		use count+1은 그대로 유지된다. */
		class CPoppedTask
		{
		public:
			shared_ptr<CHostBase> m_taskSubject;
			CFinalUserWorkItem m_workItem;
			CUserTaskQueue* m_owner;

			inline CPoppedTask();
			inline ~CPoppedTask();
		};

	private:
		// 이 user task를 가지는 소유주. 가령 넷서버,랜클라,랜서버 본체다.
		IUserTaskQueueOwner *m_owner;

		/* user work item을 갖고 있으며, 아직 working state가 아닌 task subject들이다.
		동시에 서로 다른 task subject의 user callback은 허락되지만, 같은 task subject에 대해서는 그러면 안된다.
		그렇게 하기 위한 데이터 구조이다. */
		CFastList2<shared_ptr<CHostBase>, int> m_workReadyList;

		// 어딘가에서 popped task에 대한 처리를 하고 있는 상태인 것들.
		// Proud.CHostBase.m_UserTaskQueueUseOnly_InWorkingList와 일관성을 유지한다.
		// task subject는 work ready list와 working list 중 하나에만 들어간다. 즉 교집합은 비어있다.
		CFastList2<shared_ptr<CHostBase>, int> m_workingList;

	public:
		SpinMutex m_critSec;

		CUserTaskQueue(IUserTaskQueueOwner *owner);
		~CUserTaskQueue();

		inline void Push(const shared_ptr<CHostBase>& subject, const CFinalUserWorkItem& item);
		inline bool Pop(CPoppedTask& output);

		PROUD_API bool DoesHostHaveUserWorkItem(CHostBase* host);

		inline void ResetTaskRunningFlag(const shared_ptr<CHostBase>& subject);
	};
}

#include "UserTaskQueue.inl"
