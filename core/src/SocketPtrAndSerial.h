#pragma once 

#include "../include/FakeClrBase.h"
#include "FastMap2.h"

namespace Proud
{
	using namespace std;
	
	class CSuperSocket; 

	// new socket의 주소가 최근 freed socket의 주소와 같은 경우
	// ABA problem이 발생할 여지를 없애기 위해 socket ptr 대신 사용되는 구조체
	class SocketPtrAndSerial
	{
	public:
		void* m_socket; // shared_ptr<CSuperSocket>이 가진 포인터 값.
		intptr_t m_serialNum; // socket 내 serial number. CSuperSocket.m_serialNum과 동일.

		inline SocketPtrAndSerial() 
			: m_socket (NULL)
			, m_serialNum(0)
		{
		}

		inline SocketPtrAndSerial(void* socket);
		inline SocketPtrAndSerial(const shared_ptr<CSuperSocket>& socket);
	};

	class SocketPtrAndSerialTraits
	{
	public:
		typedef const SocketPtrAndSerial& INARGTYPE;
		typedef SocketPtrAndSerial& OUTARGTYPE;

		inline static uint32_t Hash(const SocketPtrAndSerial& element) throw()
		{
			return CPNElementTraits<uintptr_t>::Hash(uintptr_t(element.m_socket) ^ uintptr_t(element.m_serialNum));
		}

		inline static bool CompareElements(const SocketPtrAndSerial& element1,
			const SocketPtrAndSerial& element2)
		{
			return (element1.m_socket == element2.m_socket)
				&& (element1.m_serialNum == element2.m_serialNum);
		}

		// 			inline static int CompareElementsOrdered(const SocketToHostMapPair& element1, const SocketToHostMapPair& element2)
		// 			{
		//
		// 			}
	};

	/* iocp or epoll에 add된 socket들. 
	weak_ptr인 이유는, 더 이상 쓰일 필요가 없는 socket인데 버그로 인해 iocp or epoll에서의 remove를 깜박한 경우,
	계속해서 socket fd가 잔존하게 되는데, 이때 PS4나 안드로이드같은 리소스 협소 환경에서 자원 고갈이 날 수 있기 때문이다.
	*/
	class AssociatedSockets :public CFastMap2<SocketPtrAndSerial, weak_ptr<CSuperSocket>, int, SocketPtrAndSerialTraits>
	{
	public:
		bool RemoveSocketByKey(const SocketPtrAndSerial& key);
		
	};

}

#include "SocketPtrAndSerial.inl"