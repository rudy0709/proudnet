#include "stdafx.h"
#include "UserTaskQueue.h"
#include "CriticalSectImpl.h"

namespace Proud
{
	CUserTaskQueue::CUserTaskQueue(IUserTaskQueueOwner *owner)
	{
		m_owner = owner;
	}

	CUserTaskQueue::~CUserTaskQueue()
	{
		SpinLock clk(m_critSec, true);

		// work ready list, working list에 있는 것들에 대해 dec use count를 한다.
		m_workReadyList.Clear();
		m_workingList.Clear();
	}
}
