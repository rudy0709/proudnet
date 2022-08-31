/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/FastArrayPtr.h"
#include "../include/FastColl.h"
#if defined(_WIN32)
#endif

namespace Proud
{
	class CSendFragRefs;
	/** byte deque와 비슷하다. TCP 송수신 버퍼 등에서 활용할 수 있다.

	주요 특징
	- heap access를 최소화한다. 내부에서 일정 크기의 버퍼를 갖고 있어서 필요할 때마다 크기를 늘린다.
	- head,tail이 있는 queue이지만 circular 모양은 아니다.
	- 미리 잡아놓은 heap block이 모자라면 기존 데이터를 block의 앞단으로 땡긴다.
	- 내용의 전체의 데이터 포인터를 얻어도 이것들이 중간에 끊어지지 않고 끝까지 연결된다. */
	class CStreamQueue
	{
		int m_growBy;
		CFastArray<uint8_t, false, true, int> m_block; // ByteArray는 자체 메모리 풀을 쓰므로 위험
		int m_headIndex;
		int m_contentsLength;
	public:
		CStreamQueue(int growBy);
		~CStreamQueue(void);

		void PushBack_Copy(const uint8_t* data, int length);
		void PushBack_Copy(const CSendFragRefs& sendData);

		void Shrink();
		/** 내용물의 데이터 버퍼 포인터를 얻는다. 크기는 GetLength()이다.*/
		inline uint8_t* GetData()
		{
			return m_block.GetData() + m_headIndex;
		}
		/** 내용물의 데이터 크기를 얻는다. */
		inline int GetLength()
		{
			return m_contentsLength;
		}

		int PopFront(int length);

		inline int PopAll()
		{
			return PopFront(GetLength());
		}
	};
}
