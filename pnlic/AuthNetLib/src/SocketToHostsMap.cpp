#include "stdafx.h"
#include "RemoteBase.h"
#include "NetCore.h"


namespace Proud
{
	/* socket에 대응하는 remote를 찾는다.
	대응하는 것이 없으면 null 리턴.
	delete socket A => new socket B를 할 경우 CRT heap 특성상 같은 주소가 나올 수 있다.
	이러면 ABA problem이 나올 수 있지만, socket 내 serial number 값을 key로 추가 사용하기 때문에 그런 문제 없다.
	즉 ptr(A)=PTR(B)라 하더라도 내부 serial number가 다르므로 remote of A != remote of B가 보장된다. */
	CHostBase* CNetCoreImpl::SocketToHostsMap_Get_NOLOCK(CSuperSocket* socket, const AddrPort& recvAddrPort)
	{
		AssertIsLockedByCurrentThread();

		SocketPtrAndSerial key(socket);

		CAddrPortToHostMap* value;
		if (m_socketToHostsMap.TryGetValue(key, value))
		{
			return value->Find(recvAddrPort);
		}

		return NULL;
	}

	void CNetCoreImpl::SocketToHostsMap_SetForAnyAddr(CSuperSocket* socket, CHostBase* remote)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		SocketPtrAndSerial key(socket);

		// 한편, 아래처럼 이미 있는 경우 에러를 내지 말고 조용히 교체해버리게 수정하세요.
		CAddrPortToHostMap* value;
		if (!m_socketToHostsMap.TryGetValue(key, value))
		{
			value = new CAddrPortToHostMap;
			m_socketToHostsMap.Add(key, value);
		}
		value->SetWildcard(remote);

		SocketToHostsMap_AssertConsist();
	}

	void CNetCoreImpl::SocketToHostsMap_SetForAddr(CSuperSocket* socket, const AddrPort& recvAddrPort, CHostBase* remote)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		SocketPtrAndSerial key(socket);

		CAddrPortToHostMap* value;
		if (!m_socketToHostsMap.TryGetValue(key, value))
		{
			value = new CAddrPortToHostMap;
			m_socketToHostsMap.Add(key, value);
		}
		value->Add(recvAddrPort, remote);
		SocketToHostsMap_AssertConsist();

	}

	void CNetCoreImpl::SocketToHostsMap_RemoveForAnyAddr(CSuperSocket* socket)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (socket != NULL)
		{
			SocketPtrAndSerial key(socket);
			SocketsToHostsMap::iterator i= m_socketToHostsMap.find(key);
			if (i != m_socketToHostsMap.end())
			{
				delete i.GetSecond();
				m_socketToHostsMap.erase(i);
			}
		}
		SocketToHostsMap_AssertConsist();
	}

	void CNetCoreImpl::SocketToHostsMap_RemoveForAddr(CSuperSocket* socket, const AddrPort& recvAddrPort)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (socket != NULL)
		{
			SocketPtrAndSerial key(socket);

			CAddrPortToHostMap* value;
			if (m_socketToHostsMap.TryGetValue(key, value))
			{
				value->Remove(recvAddrPort);
			}
		}
		SocketToHostsMap_AssertConsist();
	}

	void CNetCoreImpl::SocketToHostsMap_AssertConsist()
	{
		// TODO: 2개 이상의 host를 가리키는 것이 없는지 체크하자.
	}


}