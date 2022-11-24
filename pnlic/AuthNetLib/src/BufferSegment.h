#pragma once 

#include "../include/ByteArrayPtr.h"
#include "LowFragMemArray.h"
#include "../include/ClassBehavior.h"

namespace Proud
{
#ifndef _WIN32
	struct WSABUF  // fake win32
	{
		int len;     /* the length of the buffer */
		char *buf; /* the pointer to the buffer */
	};
#endif

	// WSABUF array의 wrap. 데이터 버퍼를 직접 소유하지 않고 외부의 것을 참조한다.
	class CFragmentedBuffer
	{
	private: 
		NO_COPYABLE(CFragmentedBuffer)
	private:
		// segment들의 바이트 총합
		int m_totalLength;

	public:
		typedef CFastArray<WSABUF, true, true, int> WSABUF_Array;
	private:
		/* (예전에는 CLowFragMemArray<100, WSABUF, WSABUF&, true> 였지만, 
		TCP send queue에서 빼올 때 100을 넘는 경우가 흔하며, 이때는 상당한 realloc & copy 로 인해 성능상 이익이 불투명.
		다행히 linux, win32 모두 heap access가 매우 빨라졌다. 
		win32는 low fragment heap이라는 기능이 자체 내장되었고
		linux는 contention이 발생하면 heap pool을 생성해서 쓰는 방식이기 때문에
		code profile을 해서 딱히 느리다고 나오지 않는한 just use new keyword가 답이다.
		하지만 배열 크기가 증가하는 cost를 줄이기 위해 minimum capacity를 키우는 방식[1]으로 사용하도록 하자. */
		WSABUF_Array* m_buffer;
		

	public:
		inline WSABUF_Array& Array() { return *m_buffer; }
		inline const WSABUF_Array& Array() const { return *m_buffer; }

		CFragmentedBuffer();
		~CFragmentedBuffer();

		int GetLength()
		{
			//int ret = 0;
			/*for(int i=0;i<(int)m_buffer.Count;i++)
			{
				ret += m_buffer[i].len;
			}*/
			return m_totalLength;
		}

		void Clear()
		{
			Array().Clear();
			m_totalLength = 0;
		}

		void Add(uint8_t* buf, int length)
		{
			WSABUF e;
			e.buf = (char*)buf;
			e.len = length;
			m_totalLength += length;
			Array().Add(e);
		}

		inline WSABUF* GetSegmentPtr()
		{
			return Array().GetData();
		}

		inline int GetSegmentCount()
		{
			return Array().GetCount();
		}

		ByteArrayPtr Assemble();
		ByteArrayPtr AssembleUpTo(int maxLength, int segmentStartIndex, int* outUsedSegmentCount);
	};
}