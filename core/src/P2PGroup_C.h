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
#include "../include/HostIDArray.h"
//#include "TypedPtr.h"
#include "ISendDest_C.h"

namespace Proud
{
	using namespace std;
	
	class CP2PGroup;

	class CP2PGroup_C;
	typedef shared_ptr<CP2PGroup_C> CP2PGroupPtr_C;

	typedef CFastMap2<HostID, CP2PGroupPtr_C, int> P2PGroups_C;

	class CP2PGroup_C
	{
	public:
		/* 그룹 ID */
		HostID m_groupHostID;

		/* 그룹에 소속된 client peer들의 HostID */
		typedef CFastMap2<HostID, weak_ptr<IP2PGroupMember>, int> P2PGroupMembers;
		P2PGroupMembers m_members;

		CP2PGroup_C()
		{
			m_groupHostID = HostID_None;
		}
		void ToInfo(CP2PGroup &ret);
	};

	// 2011.09.21 add by ulelio : 패킷 최적화를 위해 unreliable relay를 할때 모든 hostid를 보내는것이 아니라 directp2p한 양이 적을경우
	// 서버에 direct된 hostId를 보내 최적화한다. ( 부분 집합이므로 subset이란 단어사용 )
	class P2PGroupSubset_C
	{
	public:
		HostIDArray m_excludeeHostIDList;	// 릴레이에서 제외되야할 hostid list
	};
}
