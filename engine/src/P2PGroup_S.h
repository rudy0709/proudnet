/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

//#include "TypedPtr.h"
#include "../include/FakeClr.h"
#include "../include/NetServerInterface.h"
#include "../include/Enums.h"
#include "../include/P2PGroup.h"
#include "../include/NetServerInterface.h"
#include "FastMap2.h"

namespace Proud
{
	using namespace std;

	class CP2PGroup;
	class CRemoteClient_S;
	class CP2PGroup_S;
	typedef shared_ptr<CP2PGroup_S> CP2PGroupPtr_S;

	typedef CFastMap2<HostID, CP2PGroupPtr_S,int> P2PGroups_S;

	struct JoinedP2PGroupInfo
	{
		shared_ptr<CP2PGroup_S> m_groupPtr;

		inline JoinedP2PGroupInfo()
		{
		}
	};

	typedef CFastMap2<HostID, JoinedP2PGroupInfo,int> JoinedP2PGroups_S;
	// 서버 또는 RC가 이 객체를 상속받는다.
	class CP2PGroupMemberBase_S
	{
	public:
		/* 이 P2P 멤버가 참여하고 있던 P2P 그룹들 */
		JoinedP2PGroups_S m_joinedP2PGroups;

		virtual HostID GetHostID() = 0;
		virtual ~CP2PGroupMemberBase_S() {}
	};

	struct P2PGroupMember_S
	{
		int64_t m_joinedTime;
		weak_ptr<CP2PGroupMemberBase_S> m_ptr;
	};


	// 클라에서 unreliable relay가 왔을때 G,~a,~b,~c로부터 차집합을 얻는 과정이 있다.
	// 거기서 두 집합이 정렬되어 있으면 merge sort와 비슷한 방법으로 빨리 차집합을 얻을 수 있다.
	// 그래서 이 map은 ordered이어야 하고, 그것이 std.map을 쓰는 이유다.
	typedef std::map<HostID, P2PGroupMember_S> P2PGroupMembers_S;


	// 서버상의 1개의 P2P 그룹 객체
	class CP2PGroup_S
	{
	public:
		// 클라에게 P2P join 이벤트를 던진 후 그것에 대한 ack가 올 때까지 기다리는 상태 객체
		class AddMemberAckWaiter
		{
		public:
			HostID m_joiningMemberHostID;
			HostID m_oldMemberHostID; // 이 피어에서 새 멤버에 대한 OnP2PMemberJoin 콜백이 있어야 한다.
			uint32_t m_eventID;
			int64_t m_eventTime; // 너무 오랫동안 보유되면 제거하기 위함
		};

		/* 그룹 ID */
		HostID m_groupHostID;

		/* 그룹에 소속된 client peer들의 HostID */
		P2PGroupMembers_S m_members;

		/* P2P 그룹이 super peer를 놓고 통신할 경우 가장 적합하다고 추정되는 순으로 정렬된 HostID 리스트 */
		CFastArray<SuperPeerRating> m_orderedSuperPeerSuitables;

		// 이것이 켜져 있으면 일정 시간마다 수퍼피어 선정 계산을 한다.
		bool m_superPeerSelectionPolicyIsValid;

		CSuperPeerSelectionPolicy m_superPeerSelectionPolicy;

		// P2PGroup add ack가 도착할 때까지 기다리고 있는 것들의 목록
		class CAddMemberAckWaiterList : public CFastArray<AddMemberAckWaiter>
		{
		public:
			bool RemoveEqualItem(HostID oldMemberHostID, HostID newMemberHostID, uint32_t eventID);
			bool AckWaitingItemExists(HostID joiningMemberHostID, uint32_t eventID);
			bool AckWaitingItemExists(HostID joiningMemberHostID);
		};
		CAddMemberAckWaiterList m_addMemberAckWaiters;

		// Proud.CNetServerImpl.CreateP2PGroup의 파라메터로부터 전파받음
		CP2PGroupOption m_option;

		void ToInfo(CP2PGroup &ret);
		CP2PGroupPtr ToInfo();

		PROUDSRV_API CP2PGroup_S();

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif
	};

	// "P2P group의 모든 멤버 중에서 '특정 멤버들'을 제외하겠음"을 의미하는 데이터
	struct P2PGroupSubset_S
	{
		HostID m_groupID;  // P2P 그룹 X
		HostIDArray m_excludeeHostIDList; // X에서 제외될 멤버들
	};

	class P2PGroupSubsetList :public CFastArray<P2PGroupSubset_S> {};
}
