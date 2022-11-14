#include "stdafx.h"
#include "P2PPair.h"

namespace Proud
{
	// 인자 peer의 반대쪽을 리턴한다.
	// peer가 잘못됐으면 NULL 리턴.
	CRemoteClient_S* P2PConnectionState::GetOppositePeer(CRemoteClient_S* peer)
	{
		if (m_firstClient == peer)
			return m_secondClient;
		if (m_secondClient == peer)
			return m_firstClient;
		return NULL;
	}
}