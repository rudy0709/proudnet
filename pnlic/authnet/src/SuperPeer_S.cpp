/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <typeinfo>
#include "../include/INetServerEvent.h"
#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/P2PGroup.h"
#include "SendFragRefs.h"
#include "../include/sysutil.h"
#include "../include/ErrorInfo.h"
#include "../include/MessageSummary.h"

#include "RemoteClient.h"
#include "NetServer.h"
#include "STLUtil.h"
#include "RmiContextImpl.h"
#include "quicksort.h"

namespace Proud
{

	class SuperPeerCandidate
	{
	public:
		CRemoteClient_S* m_peer;
		CP2PGroup_S* m_group;
		int64_t m_joinedTime;

		inline bool operator==(const SuperPeerCandidate& rhs)
		{
			return GetSuperPeerRatingC(m_peer, m_group) == GetSuperPeerRatingC(rhs.m_peer, m_group);
		}

		inline bool operator<(const SuperPeerCandidate& rhs)
		{
			return GetSuperPeerRatingC(m_peer, m_group) > GetSuperPeerRatingC(rhs.m_peer, m_group);
		}
	private:
		inline static double GetSuperPeerRatingC(CRemoteClient_S* rc, CP2PGroup_S* group)
		{
			if (group->m_orderedSuperPeerSuitables.GetCount() > 0 && group->m_orderedSuperPeerSuitables[0].m_hostID == rc->m_HostID)
				return rc->m_SuperPeerRating + CNetConfig::SuperPeerSelectionPremiumMs;
			else
				return rc->m_SuperPeerRating;
		}
	};

	// 수퍼피어로서 역할을 가장 잘 수행할 놈을 고른다.
	// 이미 지정된 경우 웬만하면 바꾸지 않는다.
	void CNetServerImpl::P2PGroup_RefreshMostSuperPeerSuitableClientID(CP2PGroup_S* group)
	{
		AssertIsLockedByCurrentThread();

		int64_t currTime = GetCachedServerTimeMs();

		if (group->m_members.size() == 0 || !group->m_superPeerSelectionPolicyIsValid)
		{
			group->m_orderedSuperPeerSuitables.Clear();
			//	NTTNTRACE("m_orderedSuperPeerSuitables = nil\n");
		}
		else
		{
			// 판단 기준: real UDP가 쓰이고 있는지? 공유기 없이 사용중인 클라인지?
			for (P2PGroupMembers_S::iterator i = group->m_members.begin(); i != group->m_members.end(); i++)
			{
				P2PGroupMember_S rc0 = i->second;
				//CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(rc0.m_ptr);
				CRemoteClient_S* rc = NULL;
				try
				{
					rc = dynamic_cast<CRemoteClient_S*>(rc0.m_ptr);
				}
				catch (std::bad_cast& e)
				{
					CFakeWin32::OutputDebugStringT(StringA2T(e.what()));
					CauseAccessViolation();
				}

				// 그룹 멤버로서의 서버는 평가 대상에서 제외한다.
				if (rc == NULL)
					continue;

				/* 수퍼피어가 아닐 당시의 계산이 된 적이 있고 현 수퍼피어이면 계산을 하지 않는다.
				이유: 수퍼피어가 되면 트래픽이 확 늘어남=>평가 점수가 급락하게 됨=>다른 피어에 비해서 평가 점수가 크게 하락할 수 있음
				*/
				if (rc->m_SuperPeerRating != 0
					&& group->m_orderedSuperPeerSuitables.GetCount() > 0
					&& group->m_orderedSuperPeerSuitables[0].m_hostID == rc->m_HostID
					&& rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
				{
					continue;
				}

				// 0점부터 합산해 올라간다.
				rc->m_SuperPeerRating = 0;

				// 클섭 UDP 홀펀칭이 되어 있으면 일단 높은 기본 점수 깔고 들어감
				// 반대로, 클섭 UDP 홀펀칭이 안되어 있으면 크게 불리해짐.
				if (rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
				{
					rc->m_SuperPeerRating += group->m_superPeerSelectionPolicy.m_realUdpWeight;//10000
				}

				// 공유기 없이 리얼 IP이면 득점
				if (!rc->IsBehindNat())
				{
					rc->m_SuperPeerRating += group->m_superPeerSelectionPolicy.m_noNatDeviceWeight; //500;
				}

				// latency가 길수록 크게 감점 (1초 이상이면 심각한 수준이므로 200점 감점)
				double recentPing = (double)rc->m_recentPingMs / 1000;
				rc->m_SuperPeerRating -= recentPing * group->m_superPeerSelectionPolicy.m_serverLagWeight; //200;

				// 사용자가 입력한 프레임 속도만큼 가산점 처리한다. 40~60 프레임 * 4 = 240 점 정도
				rc->m_SuperPeerRating += rc->m_lastApplicationHint.m_recentFrameRate * group->m_superPeerSelectionPolicy.m_frameRateWeight;

				/* 피어 평균 핑만큼 감점 처리한다.
				(제아무리 서버와의 핑이 좋아도 타 피어와의 핑이 나쁘면 심각한 수준인 경우도 있고 아닌 경우도 있다.
				디폴트는 피어 갯수당 0점. >0이면 JIT 홀펀칭이 무의미하므로.) */
				double p2pGroupTotalRecentPing = (double)rc->GetP2PGroupTotalRecentPing(group->m_groupHostID) / 1000;
				rc->m_SuperPeerRating -= PNMAX(0, p2pGroupTotalRecentPing) * group->m_superPeerSelectionPolicy.m_peerLagWeight;

				// 트래픽 속도만큼 가산점 처리 (VDSL 수준(100Mbps) 이면 매우 높은 수준이므로 400점 가산점)
				rc->m_SuperPeerRating += group->m_superPeerSelectionPolicy.m_sendSpeedWeight * rc->m_sendSpeed * (1024 * 1024 * 10);//400

				// P2P 그룹에 들어온지 얼마 안됐으면 왕창 감점한다.
				if (group->m_superPeerSelectionPolicy.m_excludeNewJoineeDurationTimeMs > 0 &&
					currTime - rc0.m_joinedTime < group->m_superPeerSelectionPolicy.m_excludeNewJoineeDurationTimeMs)
				{
					rc->m_SuperPeerRating = -9e+30;
				}
			}

			// 정렬한다.
			POOLED_ARRAY_LOCAL_VAR(CFastArray<SuperPeerCandidate>, memberList);

			for (P2PGroupMembers_S::iterator i = group->m_members.begin(); i != group->m_members.end(); i++)
			{
				//CRemoteClient_S* iAsRC = dynamic_cast<CRemoteClient_S*>(i->second.m_ptr);
				CRemoteClient_S* iAsRC = NULL;
				try
				{
					iAsRC = dynamic_cast<CRemoteClient_S*>(i->second.m_ptr);
				}
				catch (std::bad_cast& e)
				{
					CFakeWin32::OutputDebugStringT(StringA2T(e.what()));
					CauseAccessViolation();
				}
				if (iAsRC)
				{
					SuperPeerCandidate item;
					item.m_peer = iAsRC;
					item.m_group = group;
					item.m_joinedTime = (currTime - i->second.m_joinedTime);
					memberList.Add(item);
				}
			}
			QuickSort(memberList.GetData(), memberList.GetCount());

			group->m_orderedSuperPeerSuitables.SetCount(memberList.GetCount());
			for (int i = 0; i < memberList.GetCount(); i++)
			{
				group->m_orderedSuperPeerSuitables[i].m_hostID = memberList[i].m_peer->m_HostID;
				group->m_orderedSuperPeerSuitables[i].m_rating = memberList[i].m_peer->m_SuperPeerRating;
				group->m_orderedSuperPeerSuitables[i].m_realUdpEnabled = memberList[i].m_peer->m_ToClientUdp_Fallbackable.m_realUdpEnabled;
				group->m_orderedSuperPeerSuitables[i].m_isBehindNat = memberList[i].m_peer->IsBehindNat();
				group->m_orderedSuperPeerSuitables[i].m_recentPingMs = memberList[i].m_peer->m_recentPingMs;
				group->m_orderedSuperPeerSuitables[i].m_P2PGroupTotalRecentPingMs = memberList[i].m_peer->GetP2PGroupTotalRecentPing(group->m_groupHostID);
				group->m_orderedSuperPeerSuitables[i].m_sendSpeed = memberList[i].m_peer->m_sendSpeed;
				group->m_orderedSuperPeerSuitables[i].m_JoinedTime = memberList[i].m_joinedTime;
				group->m_orderedSuperPeerSuitables[i].m_frameRate = memberList[i].m_peer->m_lastApplicationHint.m_recentFrameRate;
			}
			/*#ifdef _DEBUG
			{
			StringA txt,s;
			for(int i=0;i<group->m_orderedSuperPeerSuitables.GetCount();i++)
			{
			s.Format("%d ", group->m_orderedSuperPeerSuitables[i]);

			txt += s;
			}

			NTTNTRACE("m_orderedSuperPeerSuitables = %s\n",txt);
			}

			#endif*/
		}
	}

	void CNetServerImpl::P2PGroup_RefreshMostSuperPeerSuitableClientID(CRemoteClient_S* rc)
	{
		AssertIsLockedByCurrentThread();

		// 연계된 모든 p2p group에 대해 refresh를 시행
		for (JoinedP2PGroups_S::iterator i = rc->m_joinedP2PGroups.begin(); i != rc->m_joinedP2PGroups.end(); i++)
		{
			CP2PGroup_S* grp = i->GetSecond().m_groupPtr;
			P2PGroup_RefreshMostSuperPeerSuitableClientID(grp);
		}
	}

	HostID CNetServerImpl::GetMostSuitableSuperPeerInGroup(
		HostID groupID,
		const CSuperPeerSelectionPolicy& policy /*=CSuperPeerSelectionPolicy::GetOrdinary()*/,
		const HostID* excludees /*=NULL*/,
		intptr_t excludeesLength /*=0*/)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CP2PGroup_S* group = GetP2PGroupByHostID_NOLOCK(groupID);
		if (group)
		{
			TouchSuitableSuperPeerCalcRequest(group, policy);

			// 종전 얻은 결과를 리턴
			for (int i = 0; i<(int)group->m_orderedSuperPeerSuitables.GetCount(); i++)
			{
				if (!ArrayHasItem(excludees, excludeesLength, group->m_orderedSuperPeerSuitables[i].m_hostID))
					return group->m_orderedSuperPeerSuitables[i].m_hostID;
			}
		}
		return HostID_None;
	}

	Proud::HostID CNetServerImpl::GetMostSuitableSuperPeerInGroup(
		HostID groupID,
		const CSuperPeerSelectionPolicy& policy,
		HostID excludee)
	{
		CFastArray<HostID> ex;
		if (excludee != HostID_None)
			ex.Add(excludee);

		return GetMostSuitableSuperPeerInGroup(groupID, policy, ex.GetData(), ex.GetCount());
	}

	void CNetServerImpl::ElectSuperPeer()
	{
		//	try	
		{
			CriticalSectionLock lock(GetCriticalSection(), true);
			CHECK_CRITICALSECTION_DEADLOCK(this);

			for (P2PGroups_S::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); i++)
			{
				CP2PGroup_S* group = i->GetSecond();
				P2PGroup_RefreshMostSuperPeerSuitableClientID(group);
			}
		}
		// 		catch (...)사용자 정의 루틴을 콜 하는 곳이 없으므로 주석화
		// 		{
		// 			if(m_logWriter)
		// 			{				
		// 				m_logWriter->WriteLine(TID_System, _PNT("FATAL ** Unhandled Exception at PurgeTooOldUnmatureClient!"));
		// 			}
		// 			throw;
		// 		}
	}

	int CNetServerImpl::GetSuitableSuperPeerRankListInGroup(HostID groupID, SuperPeerRating* ratings, int ratingsBufferCount, const CSuperPeerSelectionPolicy& policy, const CFastArray<HostID> &excludees)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		CP2PGroup_S* group = GetP2PGroupByHostID_NOLOCK(groupID);
		if (group)
		{
			TouchSuitableSuperPeerCalcRequest(group, policy);

			int retCnt = 0;

			// 종전 얻은 결과를 리턴. 단, 예외는 제끼고.
			for (int i = 0; i < (int)group->m_orderedSuperPeerSuitables.GetCount(); i++)
			{
				if (retCnt < ratingsBufferCount && !ArrayHasItem(excludees, group->m_orderedSuperPeerSuitables[i].m_hostID))
				{
					ratings[retCnt] = group->m_orderedSuperPeerSuitables[i];
					retCnt++;
				}
			}

			return retCnt;
		}

		return 0;
	}

	// P2P group 하나에 대해, 모든 클라들끼리 일정 시간마다 수퍼피어 선정을 위한 검사 기능이 작동하는 기능을 활성화한다.
	void CNetServerImpl::TouchSuitableSuperPeerCalcRequest(CP2PGroup_S* group, const CSuperPeerSelectionPolicy &policy)
	{
		int64_t currTime = GetCachedServerTimeMs();

		// 클라들에게 통신량 측정을 지시. 필요시에만.
		for (P2PGroupMembers_S::iterator i = group->m_members.begin(); i != group->m_members.end(); i++)
		{
			CRemoteClient_S* rc = NULL;
			try
			{
				rc = dynamic_cast<CRemoteClient_S*>(i->second.m_ptr);
			}
			catch (std::bad_cast& e)
			{
				// 절대 이러한 상황으로 올 일이 없어야 한다. 
				CFakeWin32::OutputDebugStringT(StringA2T(e.what()));
				CauseAccessViolation();
			}
			// 클라에게 송신 속도를 측정하라는 명령을 보낸다. 단, 쿨타임 있음.

			// TODO: 이거 없애도 될 듯. Set_lastRequestMeasureSendSpeedTime 변수까지 통째로.
			if (rc &&  // NOTE: RemoteServer이면 null일 수 있다.
				(rc->m_safeTimes.Get_lastRequestMeasureSendSpeedTime() == 0
				|| currTime - rc->m_safeTimes.Get_lastRequestMeasureSendSpeedTime() > CNetConfig::MeasureClientSendSpeedIntervalMs)
				)
			{
				rc->m_safeTimes.Set_lastRequestMeasureSendSpeedTime(currTime);

				m_s2cProxy.RequestMeasureSendSpeed(rc->m_HostID, g_ReliableSendForPN,
					policy.m_sendSpeedWeight > 0);
			}
		}

		// 새 policy를 갱신
		group->m_superPeerSelectionPolicy = policy;
		group->m_superPeerSelectionPolicyIsValid = true;

		// 일단 부정확하더라도 수퍼피어 목록은 넣어야 최초 콜 결과가 HostID_None이 리턴되는 불상사는 없다.
		if (group->m_orderedSuperPeerSuitables.GetCount() == 0 && group->m_members.size() > 0)
		{
			P2PGroup_RefreshMostSuperPeerSuitableClientID(group);
		}
	}

	CSuperPeerSelectionPolicy::CSuperPeerSelectionPolicy()
	{
		// 반드시 0만 채워야 한다! GetNull 때문
		m_realUdpWeight = 0;
		m_noNatDeviceWeight = 0;
		m_serverLagWeight = 0;
		m_peerLagWeight = 0;
		m_sendSpeedWeight = 0;
		m_excludeNewJoineeDurationTimeMs = 0;
		m_frameRateWeight = 0;
	}

	CSuperPeerSelectionPolicy CSuperPeerSelectionPolicy::GetOrdinary()
	{
		CSuperPeerSelectionPolicy ret;
		ret.m_realUdpWeight = 10000;
		ret.m_noNatDeviceWeight = 500;
		ret.m_serverLagWeight = 150;
		ret.m_peerLagWeight = 50; // 게임방 멤버가 4명 이상인 경우 서버와의 핑보다 피어와의 핑의 가중치가 상회함을 의미한다.
		ret.m_sendSpeedWeight = 0; // 송신 속도 측정 기능에 문제가 있는 라우터를 피하기 위해 0으로 설정되어 있다.
		ret.m_excludeNewJoineeDurationTimeMs = 5000; // 통상적으로 이정도는 잡아주어야 잦은 수퍼피어 변경을 막음
		ret.m_frameRateWeight = 4; // serverLagWeight보다 약간더 우선순위가 크다. ( 최고 60프레임정도로 보았을때 240 )

		return ret;
	}

	CSuperPeerSelectionPolicy CSuperPeerSelectionPolicy::GetNull()
	{
		CSuperPeerSelectionPolicy ret;

		return ret;
	}

}