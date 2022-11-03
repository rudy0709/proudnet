/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/Deque.h"
#include "../include/Enums.h"
//#include "TypedPtr.h"
#include "ISendDest_C.h"

namespace Proud
{
	class CP2PGroup;

	class CP2PGroup_C;
	typedef RefCount<CP2PGroup_C> CP2PGroupPtr_C;

	typedef CFastMap2<HostID, CP2PGroupPtr_C,int> P2PGroups_C;

	class CP2PGroup_C
	{
	public:
		/* 그룹 ID */
		HostID m_groupHostID;

		/* 그룹에 소속된 client peer들의 HostID */
		P2PGroupMembers_C m_members;

		CP2PGroup_C()
		{
			m_groupHostID = HostID_None;
		}
		void ToInfo(CP2PGroup &ret);
	};
}
