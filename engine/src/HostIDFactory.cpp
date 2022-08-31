#include "stdafx.h"
#include "HostIDFactory.h"


namespace Proud
{
	CHostIDFactory::CHostIDFactory(int64_t issueValidTime, const bool isHostIDRecycle)
		: m_isHostIDRecycle(isHostIDRecycle)
		, m_issueValidTime(issueValidTime)
	{
		m_isCycledIssueHostID = false;
		m_reservedStartHostID = HostID_None;
		m_reservedHostIDCount = 0;
		m_lastIssuedHostID = HostID_Last;
	}
	CHostIDFactory::~CHostIDFactory()
	{
		m_lastIssuedHostID = HostID_Last;
		m_droppedHostIDList.Clear();
		m_issuedHostIDMap.Clear();
	}

	// assignedHostID=none이면 예약범위 외 임의 값을 할당해주며,
	// 그렇지 않은 경우 assignedHostID가 지정한 값으로 할당해준다.
	// 할당 실패시 none을 리턴한다.
	HostID CHostIDFactory::Create(HostID assignedHostID /*=HostID_None*/)
	{
		int64_t curTime = GetPreciseCurrentTimeMs();
		if (assignedHostID != HostID_None)
		{
			if (assignedHostID < (int)m_reservedStartHostID
				|| assignedHostID >= (int)m_reservedStartHostID + m_reservedHostIDCount)
			{
				return HostID_None;
			}

			CFastMap2<HostID, CHostIDNode, int>::iterator iter = m_issuedHostIDMap.find(assignedHostID);
			if (iter != m_issuedHostIDMap.end())
				return HostID_None;

			CHostIDNode hostIDNode;
			hostIDNode.m_hostID = assignedHostID;
			m_issuedHostIDMap.Add(assignedHostID, hostIDNode);

			return assignedHostID;
		}

		// HostID Recycle 기능이 켜져 있을때만 dropped list를 검사함
		if (m_isHostIDRecycle)
		{
			// free 항목이 있으며 일정시간이 지난것만 재사용.
			if (m_droppedHostIDList.GetCount() > 0)
			{
				CHostIDNode hostIDNode = m_droppedHostIDList.GetHead();
				if ((curTime - hostIDNode.m_dropTime) > m_issueValidTime)
				{
					hostIDNode.m_recycleCount++;
					hostIDNode.m_dropTime = 0;

					m_issuedHostIDMap.Add(hostIDNode.m_hostID, hostIDNode);
					m_droppedHostIDList.RemoveHead();

					return hostIDNode.m_hostID;
				}
			}
		}

		// 항목이 없거나 있더라도 오래된 항목이 없으므로 새로 발급.
		CHostIDNode hostIDNode;
		hostIDNode.m_hostID = ExploreNewHostID();
		m_issuedHostIDMap.Add(hostIDNode.m_hostID, hostIDNode);

		return hostIDNode.m_hostID;
	}

	void CHostIDFactory::Drop(HostID dropID)
	{
		// Drop 자체가 free list에 넣는 행위를 하므로 다음 Create시에 free list가 없어서 매번 새로 생성함
		// 해당 코드는 INT_MAX를 넘어설수 없다는 가정하에 쓰면 좋은 코드
		// 하지만 무결성이 깨짐
		// 이유 - m_issuedHostIDMap 자체를 검사할 필요가 없음
		//		if (m_isHostIDRecycle == false)
		//			return;
		int64_t curTime = GetPreciseCurrentTimeMs();

		//assigned된 HostID인지 판단.
		CFastMap2<HostID, CHostIDNode, int>::iterator iter = m_issuedHostIDMap.find(dropID);
		if (iter != m_issuedHostIDMap.end())
		{
			// HostID Recycle 기능이 켜져 있을때만 m_droppedHostIDList에 넣음
			if (m_isHostIDRecycle)
			{
				CHostIDNode hostIDNode = iter->GetSecond();
				hostIDNode.m_dropTime = curTime;
				m_droppedHostIDList.AddTail(hostIDNode); // 씬삥을 맨 뒤로. 맨 앞은 가장 오래전에 drop된 항목.
			}

				m_issuedHostIDMap.erase(iter);
			}
		}

	// 재사용 가능한 hostID가 없으므로 완전히 새로운 값을 찾는다.
	Proud::HostID CHostIDFactory::ExploreNewHostID()
	{
		while (true)
		{
		m_lastIssuedHostID = (HostID)((int)m_lastIssuedHostID + 1);

		if (m_reservedStartHostID == m_lastIssuedHostID)
		{
			m_lastIssuedHostID = (HostID)((int)m_lastIssuedHostID + m_reservedHostIDCount);
		}

		// 사실,여기까지 갈 일은 없다. HostID가 오랫동안 안 쓰인 것이 재사용되기 때문에 int overflow는 애당초 가지도 않는다.
		if (m_lastIssuedHostID == HostID_None)
			{
				// 한바퀴를 돌았을때 체크 변수를 true로 설정
				// HostID Recycle Off일때 사용
				m_isCycledIssueHostID = true;
			m_lastIssuedHostID = (HostID)((int)HostID_Last + 1);
			}

			// 해당 코드는 HostID 발급에 대한 무결성 코드
			// HostID 재사용 기능 Off일때
			// m_droppedHostIDList를 전혀 사용하지 않으므로 현재 발급된(m_issuedHostIDMap) HostID만 검사하고 새로운것을 발급하면 됨
			if (m_isCycledIssueHostID)
			{
				if (m_issuedHostIDMap.ContainsKey(m_lastIssuedHostID) == true)
				{
					continue;
				}
			}

			break;
		}

		return m_lastIssuedHostID;
	}

	// hostID가 재사용된 횟수를 얻는다.
	uint32_t CHostIDFactory::GetRecycleCount(HostID hostID)
	{
		CFastMap2<HostID, CHostIDNode, int>::iterator itr = m_issuedHostIDMap.find(hostID);
		if (itr != m_issuedHostIDMap.end())
		{
			return itr->GetSecond().m_recycleCount;
		}
		return 0;
	}

	void CHostIDFactory::SetReservedList(HostID reservedStartHostID, int reservedHostIDCount)
	{
		m_reservedStartHostID = reservedStartHostID;
		m_reservedHostIDCount = reservedHostIDCount;
	}

	// 최근에 반환된 것이면 true를 리턴한다.
	// 주의: 느리다!
	bool CHostIDFactory::IsDroppedRecently(HostID hostID)
	{
		int64_t curTime = GetPreciseCurrentTimeMs();

		for (CFastList2<CHostIDNode, int>::iterator i = m_droppedHostIDList.begin();
			i != m_droppedHostIDList.end();i++)
		{
			CHostIDNode& hostIDNode = *i;
			if (hostIDNode.m_hostID == hostID
				&& curTime - hostIDNode.m_dropTime < m_issueValidTime)
				return true;
		}

		return false;
	}
}
