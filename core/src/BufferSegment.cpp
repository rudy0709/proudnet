#include "stdafx.h"
#include "BufferSegment.h"
#include "FavoriteLV.h"
#include "PooledObjects_C.h"

namespace Proud 
{
	// 조립한 후의 사본을 리턴한다.
	// WSASendTo가 없는 비 윈도에서는 어쩔 수 없이 조립된 사본을 얻는 과정이 필요하다.
	// maxLength: 조립 최대 크기
	// segmentStartIndex: 몇번째 segment부터 조립을 시작해야 하는지
	// outUsedSegmentIndex: 몇개의 segment가 조립에 동원됐는지(0일 수 있음)
	Proud::ByteArrayPtr CFragmentedBuffer::AssembleUpTo( int maxLength, int segmentStartIndex, int* outUsedSegmentCount )
	{
		int usedSegmentCount = 0; // 성능업 위해 별도 변수에서 +1연산을 반복 수행

		ByteArrayPtr r;
		r.UseInternalBuffer();
		r.SetCapacity(maxLength);
		r.SetCount(0);

		for (int i = segmentStartIndex; i<Array().GetCount(); i++)
		{
			char* buf = Array()[i].buf;
			int len = Array()[i].len;

			if(maxLength < len)
				break;

			r.AddRange((const uint8_t*)buf, len);
			maxLength -= len;
			usedSegmentCount++;
		}

		*outUsedSegmentCount = usedSegmentCount;
		return r;
	}

	CFragmentedBuffer::CFragmentedBuffer() :m_totalLength(0)
	{
		m_buffer = CClassObjectPool<WSABUF_Array>::GetUnsafeRef().NewOrRecycle();
		m_buffer->SetCount(0);
	}

	CFragmentedBuffer::~CFragmentedBuffer()
	{
		CClassObjectPool<WSABUF_Array>::GetUnsafeRef().Drop(m_buffer);
	}

	// 조립한 후의 사본을 리턴한다.
	// WSASendTo가 없는 비 윈도에서는 어쩔 수 없이 조립된 사본을 얻는 과정이 필요하다.
	ByteArrayPtr CFragmentedBuffer::Assemble()
	{
		ByteArrayPtr r;
		r.UseInternalBuffer();
		r.SetCapacity(m_totalLength);
		r.SetCount(0);
		for (int i = 0; i<Array().GetCount(); i++)
		{
			r.AddRange((const uint8_t*)Array()[i].buf, Array()[i].len);
		}

		return r;
	}

	
}