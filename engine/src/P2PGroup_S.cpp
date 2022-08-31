/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "P2PGroup_S.h"
#include "../include/P2PGroup.h"
#include "RemoteClient.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// CP2PGroup_S

	void CP2PGroup_S::ToInfo( CP2PGroup &ret )
	{
		/*for (Position i = m_members.GetStartPosition(); i;)
		{
			HostID m = m_members.GetKeyAt(i);
			ret.m_members.insert(m);
			m_members.GetNext(i);
		}*/

		for(P2PGroupMembers_S::iterator it=m_members.begin(); it!=m_members.end(); it++)
		{
			HostID m = it->first;
			ret.m_members.Add(m);
		}

		ret.m_groupHostID = m_groupHostID;
	}

	Proud::CP2PGroupPtr CP2PGroup_S::ToInfo()
	{
		CP2PGroupPtr ret(new CP2PGroup);
		ToInfo(*ret);
		return ret;
	}


	CP2PGroup_S::CP2PGroup_S()
	{
		m_groupHostID = HostID_None;
		m_superPeerSelectionPolicy = CSuperPeerSelectionPolicy::GetNull();
		m_superPeerSelectionPolicyIsValid = false;
	}

	bool CP2PGroup_S::CAddMemberAckWaiterList::AckWaitingItemExists( HostID joiningMemberHostID, uint32_t eventID )
	{
		for (int i = 0; i < (int)GetCount(); i++)
		{
			AddMemberAckWaiter a = (*this)[i];
			if (a.m_joiningMemberHostID == joiningMemberHostID &&
				a.m_eventID == eventID)
			{
				return true;
			}
		}
		return false;
	}

	bool CP2PGroup_S::CAddMemberAckWaiterList::AckWaitingItemExists( HostID joiningMemberHostID)
	{
		for (int i = 0; i < (int)GetCount(); i++)
		{
			AddMemberAckWaiter a = (*this)[i];
			if (a.m_joiningMemberHostID == joiningMemberHostID )
			{
				return true;
			}
		}
		return false;
	}

	// 두 멤버의, eventID에 대한 항목을 지운다.
	// 발견 후 지우면 true를 리턴한다.
	bool CP2PGroup_S::CAddMemberAckWaiterList::RemoveEqualItem(
		HostID oldMemberHostID, HostID newMemberHostID, uint32_t eventID )
	{
		for (int i = 0; i < (int)GetCount(); i++)
		{
			AddMemberAckWaiter& a = (*this)[i];
			if (a.m_oldMemberHostID == oldMemberHostID &&
				a.m_joiningMemberHostID == newMemberHostID &&
				a.m_eventID == eventID)
			{
				RemoveAt(i);
				return true;
			}
		}
		return false;
	}
}
