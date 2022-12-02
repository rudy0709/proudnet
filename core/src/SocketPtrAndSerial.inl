#pragma once 

#include "SuperSocket.h"

namespace Proud
{
	/* 이게 있으면 변수 생성하기가 훨씬 편리.
	Q: shared_ptr<CSuperSocket>이 왜 아닌지?
	A: iocp or epoll에서 받은 user ptr 값이 CSuperSocket의 ptr 값인 경우가 있다.
	이때는 shared_ptr<CSuperSocket> 객체를 얻어올 방법이 없으므로, 이렇게 한다.
	*/
	inline SocketPtrAndSerial::SocketPtrAndSerial(void* socket)
	{
		m_socket = socket;
		CSuperSocket* socket2 = (CSuperSocket*)socket;
		m_serialNum = socket2->GetSerialNumber();
	}

	inline SocketPtrAndSerial::SocketPtrAndSerial(const shared_ptr<CSuperSocket>& socket)
	{
		m_socket = socket.get();
		m_serialNum = socket->GetSerialNumber();
	}

	// associated sockets에서 key가 가리키는 socket 항목 제거. key가 가리키는 socket의 owner 값도 정리해 버린다.
	// iocp or epoll case 공통 용도.
	// 제거를 했으면 true 리턴.
	inline bool AssociatedSockets::RemoveSocketByKey(const SocketPtrAndSerial& key)
	{
		weak_ptr<CSuperSocket> socket_weak;
		if (TryGetValue(key, socket_weak))
		{
			Remove(key); // 일단 지우고
			shared_ptr<CSuperSocket> socket(socket_weak.lock());
			if (socket)
			{
				// 관계 끊기
				socket->m_associatedIoQueue_accessByAssociatedSocketsOnly = NULL;
			}
			return true;
		}
		return false;
	}

}