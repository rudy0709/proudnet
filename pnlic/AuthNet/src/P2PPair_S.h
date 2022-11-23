#pragma once

#include "P2PPair.h"
#include "FastMap2.h"
#include "RCPair.h"

namespace Proud
{
	typedef CFastMap2<RCPair, P2PConnectionStatePtr, int, RCPairTraits> RCPairMap;

	// P2P 연결 상태 맵. key: 두 클라의 hostID (A,B = B,A임에 주의)
	// 직접 P2P 통신중이 아니더라도 등록되어 있다.
	class CP2PPairList
	{
	public:
		// 릴레이건 직접이건, P2P 연결이 활성화된 것들
		RCPairMap m_activePairs;

		// 비활성되어있지만 다시 재사용될 가능성이 있는 것들, 즉 최근 일정시간까지는 활성화되어있던 것들
		RCPairMap m_recyclablePairs;

		P2PConnectionStatePtr GetPair(HostID a, HostID b);
		bool GetPair(HostID a, HostID b, P2PConnectionStatePtr& out1, RCPairMap::iterator &out2);

		void RemovePairOfAnySide(CRemoteClient_S *client);
		void AddPair(CRemoteClient_S *aPtr, CRemoteClient_S *bPtr, P2PConnectionStatePtr state);
		void ReleasePair(CNetServerImpl* owner, CRemoteClient_S* a, CRemoteClient_S* b);

		void RecyclePair_Add(CRemoteClient_S *aPtr, CRemoteClient_S *bPtr, P2PConnectionStatePtr state);
		void RecyclePair_RemoveTooOld(int64_t absTime);

		P2PConnectionStatePtr RecyclePair_Pop(HostID a, HostID b);
		bool RecyclePair_Get(HostID a, HostID b, P2PConnectionStatePtr& out1, RCPairMap::iterator& out2);

		void Clear();
	};

}