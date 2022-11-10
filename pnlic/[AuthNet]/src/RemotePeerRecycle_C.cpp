#include "stdafx.h"
#include "NetClient.h"

namespace Proud
{
	// authed에서 recycled로 옮긴다.
	void CNetClientImpl::RemotePeerRecycles_Add(CRemotePeer_C* peer)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (peer->m_HostID == HostID_None
			|| peer->m_HostID == HostID_Server)
		{
			assert(0);
			return;
		}

		// recycle list에 이미 있으면 생략
		if (m_remotePeerRecycles.ContainsKey(peer->m_HostID))
			return;

		// recycle list에 추가
		m_remotePeerRecycles.Add(peer->m_HostID, peer);

		// recycle에 들어간 시간 설정
		// +10초의 이유: 서버에서 '재사용 가능'하다고 판단했을 때 이를 클라에게 보내는데 걸리는 최악 시간보다는 길어야 하니까.
		peer->m_recycleExpirationTime = GetPreciseCurrentTimeMs() + CNetConfig::RecyclePairReuseTimeMs + CNetConfig::TcpSocketConnectTimeoutMs;
		peer->m_recycled = true;

		// 상태 변수 설정. 이건 pop하는 함수에서도 동일하다.
		peer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
	}

	// 위 함수의 반대.
	CRemotePeer_C* CNetClientImpl::RemotePeerRecycles_Pop(HostID peerHostID)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		
		// recycle list에 없으면 생략
		CRemotePeer_C* peer;
		if (!m_remotePeerRecycles.TryGetValue(peerHostID, peer))
			return NULL;

		int64_t currTime = GetPreciseCurrentTimeMs();

		// recycle list에서 제거
		m_remotePeerRecycles.RemoveKey(peerHostID);
		
		// 상태 변수 설정.
		peer->m_recycleExpirationTime = 0;
		peer->m_recycled = false;
		// 이걸 해주어야 재사용 하자마자 P2P를 잃어버리지 않는다.
		peer->m_lastDirectUdpPacketReceivedTimeMs = currTime;

		peer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

		return peer;
	}

	void CNetClientImpl::RemotePeerRecycles_GarbageTooOld()
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		for (HostIDToRemotePeerMap::iterator i = m_remotePeerRecycles.begin(); i != m_remotePeerRecycles.end(); i++)
		{
			CRemotePeer_C* peer = i->GetSecond();
			// remote peer 재사용 대기중인 것들 중 너무 오래된 것들 폐기
			// 즉, 연관된 P2P group이 없어, inactive 상태가 된 remote peer가, 
			// 수십초동안 재사용되지 못하면, 그냥 버린다.
			// +10초를 하는 이유: 서버에서는 P2P pair 재사용 시한 + 레이턴시보다는 충분히 큰 시간만큼 여기서는 꼭 재사용 가능해야 하니까.
			if (peer->m_recycled && currTime - peer->m_recycleExpirationTime > 0)
			{
				GarbageHost(peer, ErrorType_NoP2PGroupRelation, ErrorType_NoP2PGroupRelation, ByteArray(), NULL, SocketErrorCode_Ok);
			}
		}
	}
}