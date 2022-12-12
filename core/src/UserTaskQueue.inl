/*
ProudNet v1.7


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/
#pragma once 



#include "CriticalSectImpl.h"
#include "../include/MilisecTimer.h"

namespace Proud
{
	// code profile 결과, 이들이 많이 먹으므로, inline화를 했다.
	// LTCG가 지원되지 않는 GCC 때문에.

	// task subject 즉 host를 user task queue에 추가한다.
	// postEvent: true이면 eventfd or PQCS를 수행. caller가 "더 이상 처리할 task subject가 없을 때까지 루프를 도는 경우"를 제외하고 항상 true다.
	inline void CUserTaskQueue::Push(const shared_ptr<CHostBase>& subject, const CFinalUserWorkItem& item)
	{
		assert(item.HasNetCoreHeart()); // 이거 세팅 안하고 큐에 넣으면 안되지.그러면 호스트 종료시에 사용자 콜백 처리 못할 수 있으니까.
		assert(subject);

		/* DOC-USER-TASK-QUEUE-USE-COUNT

		task subject의 use count는, 다음 조건에서 +1을 유지한다.
		- work ready list or working list 중 어딘가에 있으면.

		이를 위해, 다음과 같이 use count를 건드린다.

		push를 할 때 => work ready list에 추가되는 순간, +1 [1]

		pop을 할 때 => work ready list로부터 working list로 옮겨지므로, 가만히 있자.

		pop을 한 것에 대한 work가 끝나고 나면
		=> 다시 work ready list로 옮겨지는 경우, 가만히 있자.
		=> working list에서 사라지는 경우, -1.

		[1]: 이를 pop을 할 때 하려고 했으나, inc use count는 main lock을 전제로 한다.
		이 때문에 pop을 할 때 main lock을 한다는 과중한 부담이 발생한다.

		*/

		// lock user task queue itself.
		SpinLock lock(m_critSec, true);

		// work ready에도, working list에도 없으면, 추가하도록 한다.
		// 이미 working list or work ready list에 있으면 그냥 냅두도록 하자.
		if (subject->m_UserTaskQueueUseOnly_WorkState == CHostBase::NotInWorkList)
		{
			m_workReadyList.AddTail(subject);
			subject->m_UserTaskQueueUseOnly_WorkState = CHostBase::InWorkReadyList;
		}

		// work item을 추가.
		subject->m_UserTaskQueueUseOnly_finalUserWorkItemList.AddTail(item);

	}

	/* work item과 그것의 소유자 task subject를 꺼내온다. 꺼내옴과 동시에 task subject는 working state가 된다.
	worker thread에서 처리중이지 않으면서 처리해야 하는 이벤트를 갖고 있는 task subject와 그것의 task를 준다.

	리턴값: 꺼냈으면 true.

	output: 상기 값이 true일때만 유효하다. 주의: 상기 값이 flase이면, output에는 예전 값이 그대로 남아있을 수 있다.
	따라서 false일 때는 값을 사용하지 말 것. */
	inline bool CUserTaskQueue::Pop(CPoppedTask& output)
	{
		output.m_taskSubject.reset();

		// lock user task queue itself.
		SpinLock lock(m_critSec, true);

		// work ready list에서 하나 꺼낸 후, 그것을 working list에 추가한다.
		if (m_workReadyList.GetCount() == 0)
			return false;

		// work ready list로부터 working list로 옮긴다.
		// use count는 안 건드린다. DOC-USER-TASK-QUEUE-USE-COUNT 참고.
		shared_ptr<CHostBase> subject;
		m_workReadyList.RemoveHead(subject);
		assert(subject->m_UserTaskQueueUseOnly_WorkState == CHostBase::InWorkReadyList);

		if (!subject->UserTaskQueueUseOnly_PopFirstUserWorkItem(output.m_workItem))
		{
			// 꺼내지를 못했다. 그냥 바로 not working 상태로 냅두고 나가도록 하자.
			subject->m_UserTaskQueueUseOnly_WorkState = CHostBase::NotInWorkList;
			return false;
		}

		// 잘 꺼내왔다. working list에 추가하도록 하자.
		subject->m_UserTaskQueueUseOnly_WorkState = CHostBase::InWorkingList;
		subject->m_UserTaskQueueUseOnly_iterInWorkingList = m_workingList.AddTail(subject);


		// 출력물
		output.m_taskSubject = MOVE_OR_COPY(subject);
		output.m_owner = this;

		return true;
	}

	// 꺼내왔던 work item은 working state일텐데, 이를 reset한다.
	// CPoppedTask에 의해서 사용된다.
	inline void CUserTaskQueue::ResetTaskRunningFlag(const shared_ptr<CHostBase>& subject)
	{
		// user task queue 전용 mutex이므로, contention이 적다.
		SpinLock clk(m_critSec, true);

		// set as NOT working.
		assert(subject->m_UserTaskQueueUseOnly_WorkState == CHostBase::InWorkingList);


		if (subject->m_UserTaskQueueUseOnly_finalUserWorkItemList.GetCount() > 0)
		{
			// 아직 처리할 것이 더 남아있으면 working list에서 work ready list로 이동시킨다.
			// shared_ptr의 복사 비용이 비싸서 move를 하자.
			m_workingList.MoveToOtherListTail(subject->m_UserTaskQueueUseOnly_iterInWorkingList, m_workReadyList);
			subject->m_UserTaskQueueUseOnly_WorkState = CHostBase::InWorkReadyList;
		}
		else
		{
			// 처리할 것이 없으면 working list에서 제거한다.
			m_workingList.RemoveAt(subject->m_UserTaskQueueUseOnly_iterInWorkingList); // fast remove
			
			subject->m_UserTaskQueueUseOnly_WorkState = CHostBase::NotInWorkList;
		}
	}

	inline CUserTaskQueue::CPoppedTask::~CPoppedTask()
	{
		if (m_owner)
			m_owner->ResetTaskRunningFlag(m_taskSubject);
	}

	inline CUserTaskQueue::CPoppedTask::CPoppedTask()
	{
		m_owner = NULL;
		m_taskSubject.reset();
	}

}