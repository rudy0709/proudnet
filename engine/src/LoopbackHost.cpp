#include "stdafx.h"
#include "LoopbackHost.h"
#include "RemoteBase.h"
#include "NetClient.h"

namespace Proud
{
	CLoopbackHost_C::CLoopbackHost_C(CNetClientImpl* owner, HostID hostID)
	{
		m_owner = owner;
		m_HostID = hostID;
		m_backupHostID = HostID_None;
	}

	CLoopbackHost_C::~CLoopbackHost_C()
	{
		// 여기서 owner를 접근하지 말 것.
		// PopUserTask 후 RAII로 인해 no main lock 상태에서 여기가 호출될 수 있다.
		// 정리해야 할게 있으면 여기가 아니라 OnHostGarbageCollected or OnHostGarbaged에 구현할 것.
	}

	HostID CLoopbackHost_C::GetHostID()
	{
		return m_HostID;
	}

	int64_t CLoopbackHost_C::GetIndirectServerTimeDiff()
	{
		return ((CNetClientImpl*)m_owner)->m_serverTimeDiff;
	}

	CriticalSection& CLoopbackHost_C::GetOwnerCriticalSection()
	{
		return m_owner->GetCriticalSection();
	}
}
