

namespace Proud
{
	/* socket에 대응하는 remote를 찾는다.
	대응하는 것이 없으면 null 리턴.
	delete socket A => new socket B를 할 경우 CRT heap 특성상 같은 주소가 나올 수 있다.
	이러면 ABA problem이 나올 수 있지만, socket 내 serial number 값을 key로 추가 사용하기 때문에 그런 문제 없다.
	즉 ptr(A)=PTR(B)라 하더라도 내부 serial number가 다르므로 remote of A != remote of B가 보장된다. */
	inline shared_ptr<CHostBase> CNetCoreImpl::SocketToHostsMap_Get_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& recvAddrPort)
	{
		bool isWildcard; // IGNORED
		shared_ptr<CHostBase> ret;
		if (!SocketToHostsMap_TryGet_NOLOCK(socket, recvAddrPort, ret, isWildcard))
			return shared_ptr<CHostBase>();
		else
			return ret;
	}

	// code profile 후 추가.
	// outIsWildcard: socket에 대한 항목을 찾았는데, wildcard이면 즉 socket에 대응하는 host가 유일한 경우 true로 채워진다. 함수 리턴값이 true일 때만 유효하다.
	inline bool CNetCoreImpl::SocketToHostsMap_TryGet_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& recvAddrPort, shared_ptr<CHostBase>& output, bool& outIsWildcard)
	{
		AssertIsLockedByCurrentThread();

		SocketPtrAndSerial key(socket);

		CAddrPortToHostMap* value;
		if (!m_socketToHostsMap.TryGetValue(key, value))
			return false;

		if (!value->TryGetValue(recvAddrPort, output, outIsWildcard))
			return false;
		
		return true;
	}


}