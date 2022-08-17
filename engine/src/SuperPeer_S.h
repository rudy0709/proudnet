#pragma once 

#include "../include/NetConfig.h"

namespace Proud
{
	using namespace std;

	class CRemoteClient_S;
	class CP2PGroup_S;

	class SuperPeerCandidate
	{
	public:
		shared_ptr<CRemoteClient_S> m_peer;
		shared_ptr<CP2PGroup_S> m_group;
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
		static double GetSuperPeerRatingC(const shared_ptr<CRemoteClient_S>& rc, shared_ptr<CP2PGroup_S> group);
	};

	class SuperPeerCandidateArray :public CFastArray<SuperPeerCandidate> {};

}