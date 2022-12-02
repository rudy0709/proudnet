#include "stdafx.h"
#include "P2PPair.h"
#include "RemoteClient.h"

namespace Proud
{
	// 인자 peer의 반대쪽을 리턴한다.
	// peer가 잘못됐으면 NULL 리턴.
	shared_ptr<CRemoteClient_S> P2PConnectionState::GetOppositePeer(const shared_ptr<CRemoteClient_S>& peer) const
	{
		if (!peer)
			return nullptr;

		auto f(m_firstClient.lock());
		auto s(m_secondClient.lock());

		if (f == peer)
			return s;

		if (s == peer)
			return f;

		return nullptr;
	}
}