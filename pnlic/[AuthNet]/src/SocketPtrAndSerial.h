#pragma once 

#include "../include/FakeClrBase.h"
#include "SuperSocket.h"

namespace Proud
{
	// new socket의 주소가 최근 freed socket의 주소와 같은 경우
	// ABA problem이 발생할 여지를 없애기 위해 socket ptr 대신 사용되는 구조체
	class SocketPtrAndSerial
	{
	public:
		CSuperSocket* m_socket;
		intptr_t m_serialNum; // socket 내 serial number. CSuperSocket.m_serialNum과 동일.

		inline SocketPtrAndSerial()
		{
			m_socket = NULL;
			m_serialNum = 0;
		}

		// 이게 있으면 변수 생성하기가 훨씬 편리
		inline SocketPtrAndSerial(CSuperSocket* socket)
		{
			m_socket = socket;
			m_serialNum = socket->GetSerialNumber();
		}
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


}