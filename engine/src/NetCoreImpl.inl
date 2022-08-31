#pragma once

namespace Proud
{
	// HostID에 해당하는 것을 authed host list에서 찾는다.
	inline shared_ptr<CHostBase> CNetCoreImpl::AuthedHostMap_Get(HostID hostID)
	{
		AssertIsLockedByCurrentThread();

		shared_ptr<CHostBase> rc;
		if (m_authedHostMap.TryGetValue(hostID, rc))
			return rc;
		else
			return shared_ptr<CHostBase>();
	}

	// code profile 후 추가됨
	inline bool CNetCoreImpl::AuthedHostMap_TryGet(HostID hostID, shared_ptr<CHostBase>& output)
	{
		AssertIsLockedByCurrentThread();

		return m_authedHostMap.TryGetValue(hostID, output);
	}
}
