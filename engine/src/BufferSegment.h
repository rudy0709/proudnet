#pragma once

#include "../include/ByteArrayPtr.h"
#include "LowFragMemArray.h"
#include "../include/ClassBehavior.h"
#include "FavoriteLV.h"

namespace Proud
{
#ifndef _WIN32
	struct WSABUF  // fake win32
	{
		int len;	/* the length of the buffer */
		char *buf;	/* the pointer to the buffer */
	};
#endif

	class WSABUF_Array :public CFastArray<WSABUF, true, true, int> {};

	// WSABUF array의 wrap. 데이터 버퍼를 직접 소유하지 않고 외부의 것을 참조한다.
	class CFragmentedBuffer
	{
	private:
		NO_COPYABLE(CFragmentedBuffer)
	private:
		// segment들의 바이트 총합
		int m_totalLength;

	private:

		/* (예전에는 CLowFragMemArray<100, WSABUF, WSABUF&, true> 였지만,
		TCP send queue에서 빼올 때 100을 넘는 경우가 흔하며, 이때는 상당한 realloc & copy 로 인해 성능상 이익이 불투명.
		따라서 class object pool을 쓴다.
		*/
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
