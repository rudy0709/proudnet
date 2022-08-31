#pragma once

#include "../include/Enums.h"
#include "../include/FastMap.h"
#include "../include/FastList.h"

#include "FastMap2.h"
#include "FastList2.h"
#include "../include/MilisecTimer.h"

namespace Proud
{
	// HostID를 발급하는 역할을 한다.
	// doc의 명세서 참고.
	class CHostIDFactory
	{
	private:
		class CHostIDNode
		{
		public:
			HostID m_hostID;
			// 재사용된 횟수.
			uint32_t m_recycleCount;
			// host ID pool에 떨군 시간. 충분히 오래 되어야 재사용이 허가된다. OS가 port 번호를 다루는 것 처럼.
			int64_t m_dropTime;

			inline CHostIDNode()
			{
				m_hostID = HostID_None;
				m_recycleCount = 0;
				m_dropTime = 0;
			}
		};

		const bool m_isHostIDRecycle;

		// HostID Recycle Off일때 쓰임
		// 첫 한바퀴때는 m_issuedHostIDMap을 검사할 필요가 없으므로 해당 변수를 만듬
		// HostID가 INT_MAX를 넘어서서 HostID_None으로 돌아왔을때 true로 변하여
		// 새로 발급할때마다 m_issuedHostIDMap을 검사함
		// best effort를 위한 변수
		bool m_isCycledIssueHostID;

		// 이 영역에 있는 HostID는 할당 대상에서 제외된다. 사용자가 수동으로 할당할 때 쓰는 영역이라.
		HostID m_reservedStartHostID;
		int m_reservedHostIDCount;

		// 휴지통에 들어간 값은 일정 시간 동안 재사용 금지. 이것은 그 금지 최소 기간이다.
		int64_t m_issueValidTime;

		// 가장 마지막에 발급한 값
		HostID m_lastIssuedHostID;

		// 재사용 대기중인 것들.
		// CHostIDNode의 구조체가 단순하고 작으므로 byval.
		CFastList2<CHostIDNode, int> m_droppedHostIDList;
		// 발급된 HostID
		// CHostIDNode의 구조체가 단순하고 작으므로 byval.
		CFastMap2<HostID, CHostIDNode, int> m_issuedHostIDMap;

		HostID ExploreNewHostID();

		// FastList2가 복사대입 불가능한 객체이므로 GCC에서 컴파일 오류 발생.
		CHostIDFactory(const CHostIDFactory&);
		CHostIDFactory& operator=(const CHostIDFactory&);

	public:
		PROUDSRV_API CHostIDFactory(int64_t issueValidTime, const bool isHostIDRecycle = true);
		PROUDSRV_API virtual ~CHostIDFactory();

		PROUDSRV_API HostID Create(HostID assignedHostID = HostID_None);
		PROUDSRV_API void Drop(HostID dropID);
		PROUDSRV_API uint32_t GetRecycleCount(HostID hostID);
		PROUDSRV_API void SetReservedList(HostID reservedStartHostID, int reservedHostIDCount = true);

		PROUDSRV_API bool IsDroppedRecently(HostID hostID);
	};
}
