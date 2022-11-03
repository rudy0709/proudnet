#include "stdafx.h"
#include "P2PPair_S.h"
#include "RemoteClient.h"
#include "NetServer.h"


namespace Proud
{
	// 직접 P2P 연결이 되어있는가?
	P2PConnectionStatePtr CP2PPairList::GetPair(HostID a, HostID b)
	{
		P2PConnectionStatePtr ret;
		RCPairMap::iterator ignore;

		if (GetPair(a, b, ret, ignore))
		{
			return ret;
		}

		return P2PConnectionStatePtr();
	}

	// a-b간 연결 정보 객체를 얻는다. 객체 자체와, 그 객체를 erase 용으로 쓸 수 있는 iterator를 모두 리턴한다.
	// a-b로 얻든 b-a로 얻든 같은 결과가 리턴된다.
	bool CP2PPairList::GetPair(HostID a, HostID b, P2PConnectionStatePtr& out1, RCPairMap::iterator &out2)
	{
		if (a > b)
			swap(a, b);
		RCPair p;
		p.m_first = a;
		p.m_second = b;

		RCPairMap::iterator i = m_activePairs.find(p);
		if (i == m_activePairs.end())
			return false;

		out1 = i->GetSecond();
		out2 = i;
		return true;
	}

	// 재활용도 불가능하게 아예 지워버린다.
	void CP2PPairList::RemovePairOfAnySide(CRemoteClient_S *client)
	{
		RCPair key;
		//modify by rekfkno1 - 최적화를 위하여 필요한것만 for을 돌도록 바꿉니다.
		for (CRemoteClient_S::P2PConnectionPairs::iterator itr = client->m_p2pConnectionPairs.begin();
			itr != client->m_p2pConnectionPairs.end();)
		{
			P2PConnectionStatePtr val = *itr;
			key.m_first = val->m_firstClient->m_HostID;
			key.m_second = val->m_secondClient->m_HostID;

			m_activePairs.Remove(key);
			m_recyclablePairs.Remove(key);//없을수도 있으나, 그냥 return false;하므로 remove를 쓴다.

			if (val->m_firstClient != val->m_secondClient)
			{
				if (val->m_firstClient == client)
					val->m_secondClient->m_p2pConnectionPairs.Remove(val);
				else if (val->m_secondClient == client)
					val->m_firstClient->m_p2pConnectionPairs.Remove(val);
			}

			itr = client->m_p2pConnectionPairs.erase(itr);
		}

		/*for (RCPairMap::iterator i = m_activePairs.begin();i != m_activePairs.end();)
		{
		#if 1 // HostID가 미지정되면(그럴리는 없겠지만) 어쩔라구
		P2PConnectionStatePtr val = i->second;
		if(val->m_firstClient == client || val->m_secondClient == client)
		{
		#ifdef TRACE_NEW_DISPOSE_ROUTINE
		NTTNTRACE("%d-%d Pair Remove...\n",(int)val->m_firstClient->m_HostID,(int)val->m_secondClient->m_HostID);
		#endif // TRACE_NEW_DISPOSE_ROUTINE
		val->m_firstClient->m_p2pConnectionPairs.Remove(val);
		val->m_secondClient->m_p2pConnectionPairs.Remove(val);
		i = m_activePairs.erase(i);
		}
		else
		i++;
		#else
		const RCPair& p = i->first;
		if (p.m_first == client->m_HostID || p.m_second == client->m_HostID)
		{
		P2PConnectionStatePtr val = i->second;
		val->m_firstClient->m_p2pConnectionPairs.Remove(val);
		val->m_secondClient->m_p2pConnectionPairs.Remove(val);

		i = m_activePairs.erase(i);
		}
		else
		i++;
		#endif
		}

		//modify by rekfkno1 - 완전히 서버를 떠나는 상황이므로,
		//재사용 리스트에서도 삭제토록 하자.
		for (RCPairMap::iterator i = m_recyclablePairs.begin();i != m_recyclablePairs.end();)
		{
		P2PConnectionStatePtr val = i->second;
		if(val->m_firstClient == client || val->m_secondClient == client)
		{
		// 아마도 이미 다 해제된 후로 들어가니까 안해도 될거다.
		//val->m_firstClient->m_p2pConnectionPairs.Remove(val);
		//val->m_secondClient->m_p2pConnectionPairs.Remove(val);
		i = m_recyclablePairs.erase(i);
		}
		else
		i++;
		}*/
	}

	void CP2PPairList::AddPair(CRemoteClient_S *aPtr, CRemoteClient_S *bPtr, P2PConnectionStatePtr state)
	{
		HostID a = aPtr->m_HostID;
		HostID b = bPtr->m_HostID;
		// 서버, 잘못된 아이디, 서로 같은 아이디는 추가 불가능. P2P 직빵연결된 것들의 인덱스니까.
		//assert(a != b);  loopback도 pair에 들어갈 수 있으므로 이건 체크하지 말자.
		assert(a != HostID_Server);
		assert(a != HostID_None);
		assert(b != HostID_Server);
		assert(b != HostID_None);

		if (a > b)
		{
			swap(a, b);
			swap(aPtr, bPtr);
		}

		RCPair p;
		p.m_first = a;
		p.m_second = b;

		state->m_firstClient = aPtr;
		state->m_secondClient = bPtr;

		m_activePairs.Add(p, state);
		//	NTTNTRACE("CP2PPairList.GetCount()=%d\n",m_list.GetCount());
	}

	// a,b간 p2p 연결 중복 수를 하나 줄인다. 중복 횟수가 0이 되면 p2p간 연결 관계가 없음을 의미하므로
	// pair list에서 제거한다. 제거되면, 릴레이 상태 마저도 없다는 뜻.
	// (NOTE: a,b가 서로 다른 p2p그룹 안에 들어있는 경우 중복 수가 2 이상이 될 수 있다.)
	void CP2PPairList::ReleasePair(CNetServerImpl* owner, CRemoteClient_S *a, CRemoteClient_S *b)
	{
		P2PConnectionStatePtr pair;
		RCPairMap::iterator iPair;
		if (GetPair(a->m_HostID, b->m_HostID, pair, iPair))
		{
			pair->m_dupCount--;
			if (pair->m_dupCount == 0)
			{
				pair->m_firstClient->m_p2pConnectionPairs.Remove(pair);
				pair->m_secondClient->m_p2pConnectionPairs.Remove(pair);

				// 홀펀칭 정보나 세션키를 재사용하기 위해, 
				// 완전히 없애지 않고 일단 recyclable pair로 일정 시간 보관하도록 한다.
				// 서로간 홀펀칭을 아직 안했더라도, 클라간의 P2P 세션키를 또 재발급하지 말고 재사용하기 위해, 굳이 보관을 한다.
				// (홀펀칭 재사용을 아직 해야 하는 상황이므로 이건 안함 => pair->SetRelayed(true); )
				RecyclePair_Add(pair->m_firstClient, pair->m_secondClient, pair);

				// 자 이제 제거.
				m_activePairs.erase(iPair);  // 이게 여기 있어야 한다. 위에 있으면 안되고.

				if (owner->m_logWriter)
				{
					String txt;
					txt.Format(_PNT("[Server] P2P Connection Pairs between Client %d and Client %d Deleted"), a->m_HostID, b->m_HostID);
					owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, txt);
				}
			}
		}
	}

	// 두 피어간 연결 상태를 제거하기 전에, 홀펀칭 재사용 기능을 위해 일정 시간 보관하도록 한다.
	void CP2PPairList::RecyclePair_Add(CRemoteClient_S *aPtr, CRemoteClient_S *bPtr, P2PConnectionStatePtr state)
	{
		HostID a = aPtr->m_HostID;
		HostID b = bPtr->m_HostID;
		// 서버, 잘못된 아이디, 서로 같은 아이디는 추가 불가능. P2P 직빵연결된 것들의 인덱스니까.
		//assert(a != b);  loopback도 pair에 들어갈 수 있으므로 이건 체크하지 말자.
		assert(a != HostID_Server);
		assert(a != HostID_None);
		assert(b != HostID_Server);
		assert(b != HostID_None);

		if (a > b)
		{
			swap(a, b);
			swap(aPtr, bPtr);
		}

		RCPair p;
		p.m_first = a;
		p.m_second = b;

		state->m_firstClient = aPtr;
		state->m_secondClient = bPtr;
		state->m_releaseTimeMs = GetPreciseCurrentTimeMs();

		m_recyclablePairs.Add(p, state);
	}

	// 너무 오래된 것들을 지워버린다.
	void CP2PPairList::RecyclePair_RemoveTooOld(int64_t absTime)
	{
		for (RCPairMap::iterator i = m_recyclablePairs.begin(); i != m_recyclablePairs.end();)
		{
			P2PConnectionStatePtr val = i->GetSecond();
			assert(val->m_releaseTimeMs > 0);

			if (val->m_releaseTimeMs 
				&& absTime - val->m_releaseTimeMs > CNetConfig::RecyclePairReuseTimeMs)
			{
				// 아마도 이미 다 해제된 후로 들어가니까 안해도 될거다.
				//val->m_firstClient->m_p2pConnectionPairs.Remove(val);
				//val->m_secondClient->m_p2pConnectionPairs.Remove(val);
				i = m_recyclablePairs.erase(i);
			}
			else
				i++;
		}
	}

	void CP2PPairList::Clear()
	{
		m_activePairs.Clear();
		m_recyclablePairs.Clear();
	}

	// recycle P2P pair에서 꺼내서 재사용하도록 한다.
	P2PConnectionStatePtr CP2PPairList::RecyclePair_Pop(HostID a, HostID b)
	{
		P2PConnectionStatePtr ret;
		RCPairMap::iterator iter;

		if (!RecyclePair_Get(a, b, ret, iter))
			return P2PConnectionStatePtr();

		m_recyclablePairs.erase(iter);

		int64_t currTime = GetPreciseCurrentTimeMs();
		// 꺼내왔지만 너무 오래됐으면 역시 버린다.
		// Q: 이게 필요해요? 
		// A: 몇 초마다 오래된 것을 지우지만 재수없게도 그 몇 초가 오차로 발생할 수 있으므로.
		if (ret->m_releaseTimeMs 
			&& currTime - ret->m_releaseTimeMs > CNetConfig::RecyclePairReuseTimeMs)
		{
			return P2PConnectionStatePtr();
		}

		// 꺼냈으니까 리셋해야지.
		ret->m_releaseTimeMs = 0;

		// 성공
		return ret;
	}

	// 두 클라끼리의 P2P 연결 정보를 recycle에서 찾는다.
	// 꺼내오는 것은 아님.
	bool CP2PPairList::RecyclePair_Get(HostID a, HostID b, P2PConnectionStatePtr& out1, RCPairMap::iterator& out2)
	{
		if (a > b)
			swap(a, b);

		RCPair p;
		p.m_first = a;
		p.m_second = b;

		RCPairMap::iterator i = m_recyclablePairs.find(p);
		if (i == m_recyclablePairs.end())
			return false;

		out1 = i->GetSecond();
		out2 = i;
		return true;
	}


}