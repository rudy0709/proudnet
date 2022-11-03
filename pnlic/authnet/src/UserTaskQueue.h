#pragma once 

#include "../include/CriticalSect.h"
#include "../include/Enums.h"
#include "../include/ListNode.h"
#include "RemoteBase.h"

namespace Proud
{
	class CHostBase;

	// user task queue 즉 task subject list를 가지는 객체.
	class IUserTaskQueueOwner
	{
	public:
		virtual CriticalSection& GetCriticalSection() = 0;
		virtual CHostBase* GetTaskSubjectByHostID_NOLOCK(HostID hostID) = 0;
		virtual bool IsValidHostID_NOLOCK(HostID hostID) = 0;

		// 새 task subject가 queue에 추가되면 호출되는 함수
		// 가령 thread pool에 signal을 던진다.
		virtual void PostEvent() = 0;
	};

	/* 스레드 풀: UDP, TCP 공용 worker thread를 하나 두되 이를 IOCP로 처리한다.
	담당 소켓 개수가 많으므로. 단 스레드는 CPU 개수만큼만 둬도 충분하다.
	Worker thread에서 user routine을 직접 실행하지 않고 이 또한 스레드 풀로 던져서 처리하게 만든다.
	이렇게 해서 유저가 어떤 루틴을 만들건 기본 worker thread가 굶지 않게 함을 보장한다. */
	class CUserTaskQueue
	{
		// 이 user task를 가지는 소유주. 가령 넷서버,랜클라,랜서버 본체다.
		IUserTaskQueueOwner *m_owner;

		/* task subject list.
		각 task subject는 task list를 가진다.
		동시에 서로 다른 task subject의 user callback은 허락되지만, 같은 task subject에 대해서는 그러면 안된다.
		그렇게 하기 위한 데이터 구조이다.
		*/
		CListNode<CHostBase::CNode>::CListOwner m_taskSubjects;
	public:
		CUserTaskQueue(IUserTaskQueueOwner *owner);

		void AddTaskSubject(CHostBase* subject, bool postEvent);

		bool PopAnyTaskNotRunningAndMarkAsRunning(CFinalUserWorkItem& output, CHostBase** outSubject);

		void SetTaskRunningFlag(CHostBase* subject, bool val);
	};

}