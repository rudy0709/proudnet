#include "stdafx.h"
#include "UserTaskQueue.h"
#include "CriticalSectImpl.h"

namespace Proud
{
	// task subject 즉 host를 user task queue에 추가한다.
	// postEvent: true이면 eventfd or PQCS를 수행. caller가 "더 이상 처리할 task subject가 없을 때까지 루프를 도는 경우"를 제외하고 항상 true다.
	void CUserTaskQueue::AddTaskSubject(CHostBase* subject, bool postEvent)
	{
		AssertIsLockedByCurrentThread(m_owner->GetCriticalSection());

		if (subject->m_taskSubjectNode.GetListOwner() == NULL && !subject->IsTaskRunning())
		{
			m_taskSubjects.PushBack(&subject->m_taskSubjectNode);
		}

		if (postEvent)
		{
			m_owner->PostEvent();
		}
		//m_eventSemaphore.Release();   // 이게 있어야 바로 대기중인 스레드를 깨운다.
	}

	// worker thread에서 처리중이지 않으면서 처리해야 하는 이벤트를 갖고 있는 task subject와 그것의 task를 준다.
	// 리턴값: 꺼냈으면 true
	bool CUserTaskQueue::PopAnyTaskNotRunningAndMarkAsRunning(CFinalUserWorkItem& output, CHostBase** outSubject)
	{
		// 목록에서 하나 빼낸 후 큐에서 제거한다.
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		/* 원래는 set이었지만 많이 쌓인 경우 HostID가 뒤에 있는 항목들은 굶어버리는 문제가
		있는바, deque로 바꾸었음 */
		while (m_taskSubjects.GetCount() > 0)
		{
			CHostBase *subject = m_taskSubjects.GetFirst()->m_owner;
			m_taskSubjects.Erase(&subject->m_taskSubjectNode);
			//subject->m_taskSubjectNode.UnlinkSelf();

			/* subject가 없거나, subject의 task queue가 이미 비어있거나, subject의 task running ON이면 스킵한다.
			만약 워커 스레드에 의해 처리중이라 스킵하는 경우 큐에 있던 것은 소실되지만
			워커 스레드에서 완료 후 주체의 전용 수신큐가 안 빈걸로 확인되면 다시 큐에 채워넣으므로 OK */
			if (!subject->IsFinalReceiveQueueEmpty() &&
				!subject->m_userTaskRunning &&
				subject->PopFirstUserWorkItem(output) /* &&
													  m_owner->IsValidHostID_NOLOCK(output.m_unsafeMessage.m_remoteHostID) || m_owner->m_loopbackHost == subject)*/)
													  // 위의 주석 제거 후 안전한지 확인 필요!
													  // => 배현직이 제거하라고 했으므로 안전합니다.
			{
				// 한 스레드에 의해 점유중임을 마킹한다.
				subject->m_userTaskRunning = true;
				*outSubject = subject;
				return true;
			}
		}

		return false;
	}

	// no CS lock 상태에서도 호출되므로 ITaskSubject 객체를 직접 파라메터로 받아오지 않음
	void CUserTaskQueue::SetTaskRunningFlag(CHostBase* subject, bool val)
	{
		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		subject->m_userTaskRunning = val;

		if (val == false)
		{
			// 아직 처리할 것이 더 남아있으면 메인 큐에 자신을 추가시킨다.
			if (subject->IsFinalReceiveQueueEmpty() == false)
			{
				AddTaskSubject(subject, false);
			}
		}
	}

	CUserTaskQueue::CUserTaskQueue(IUserTaskQueueOwner *owner)
		//m_eventSemaphore(0, INT_MAX)
	{
		m_owner = owner;
	}

}