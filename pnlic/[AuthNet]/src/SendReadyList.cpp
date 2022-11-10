#include "stdafx.h"
#include "SendReadyList.h"

namespace Proud
{
	int CSendReadySockets::GetCount()
	{
		CriticalSectionLock lock(m_critSec,true);
		return m_sendReadySockets.GetCount();
	}

	CSendReadySockets::CSendReadySockets()
	{
		// 성능보다 빠른 검사가 우선이므로
		m_sendReadySockets.SetOptimalLoad_BestLookup();
	}

	// UDP socket or TCP remote object를 목록에 추가하되, 이미 있으면 추가를 하지 않는다.
	// 빈 목록에 추가하는 것이면 true를 리턴한다. 
	bool CSendReadySockets::AddOrSet(CSuperSocket* socket)
	{
		CriticalSectionLock lock(m_critSec, true);
		if (m_sendReadySockets.ContainsKey((intptr_t)socket))
			return false;

		m_sendReadySockets.Add((intptr_t)socket, socket);
		return m_sendReadySockets.GetCount() == 1;
	}

	void CSendReadySockets::Remove(CSuperSocket* socket)
	{
		CriticalSectionLock lock(m_critSec,true);
		m_sendReadySockets.Remove((intptr_t)socket);
	}

	// 목록의 첫번째 것을 pop 후 true를 리턴하거나 없으면 false를 리턴
	bool CSendReadySockets::PopKey(CSuperSocket** outSocket)
	{
		CriticalSectionLock lock(m_critSec,true);

		if (!m_sendReadySockets.IsEmpty())
		{
			CFastMap2<intptr_t, CSuperSocket*, int>::iterator head = m_sendReadySockets.begin();
			*outSocket = head->GetSecond();
			m_sendReadySockets.erase(head);
			return true;
		}
		else
		{
			return false;
		}
	}
}
