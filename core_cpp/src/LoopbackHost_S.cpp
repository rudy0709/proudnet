#include "stdafx.h"
#include "LoopbackHost_S.h"
#include "NetServer.h"

namespace Proud
{

	CriticalSection& CLoopbackHost_S::GetOwnerCriticalSection()
	{
		return m_owner->GetCriticalSection();
	}

	CLoopbackHost_S::CLoopbackHost_S(CNetServerImpl* owner)
	{
		m_owner = owner;
		m_HostID = HostID_Server;
	}

	CLoopbackHost_S::~CLoopbackHost_S()
	{
		// 여기서 owner를 접근하지 말 것. 
		// PopUserTask 후 RAII로 인해 no main lock 상태에서 여기가 호출될 수 있다.
		// 정리해야 할게 있으면 여기가 아니라 OnHostGarbageCollected or OnHostGarbaged에 구현할 것.
	}

	HostID CLoopbackHost_S::GetHostID()
	{
		return m_HostID;
	}

}