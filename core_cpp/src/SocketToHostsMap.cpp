#include "stdafx.h"
#include "RemoteBase.h"
#include "NetCore.h"


namespace Proud
{
	// 특정 socket으로 도착하는 메시지는 특정 단일 remote만을 위한 것이다.
	void CNetCoreImpl::SocketToHostsMap_SetForAnyAddr(const shared_ptr<CSuperSocket>& socket, const shared_ptr<CHostBase>& remote)
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

	void CNetCoreImpl::SocketToHostsMap_SetForAddr(const shared_ptr<CSuperSocket>& socket, const AddrPort& recvAddrPort, const shared_ptr<CHostBase>& remote)
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

	void CNetCoreImpl::SocketToHostsMap_RemoveForAnyAddr(const shared_ptr<CSuperSocket>& socket)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (socket != nullptr)
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

	void CNetCoreImpl::SocketToHostsMap_RemoveForAddr(const shared_ptr<CSuperSocket>& socket, const AddrPort& recvAddrPort)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (socket != nullptr)
		{
			SocketPtrAndSerial key(socket);

			CAddrPortToHostMap* value;
			if (m_socketToHostsMap.TryGetValue(key, value))
			{
				value->Remove(recvAddrPort);

				if (value->IsEmpty())
				{
					delete value;
					m_socketToHostsMap.RemoveKey(key);
				}
			}
		}
		SocketToHostsMap_AssertConsist();
	}

	void CNetCoreImpl::SocketToHostsMap_AssertConsist()
	{
		// TODO: 2개 이상의 host를 가리키는 것이 없는지 체크하자.
	}


}