/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "SendFragRefs.h"
#include "StreamQueue.h"
#include "SendFragRefs.h"

namespace Proud
{

	CStreamQueue::CStreamQueue(int growBy)
	{
		assert(growBy > 0);
		m_growBy = growBy;
		m_headIndex = 0;
		m_contentsLength = 0;

		m_block.SetGrowPolicy(GrowPolicy_HighSpeed);
	}

	CStreamQueue::~CStreamQueue(void)
	{
	}

	/** 데이터를 추가한다.
	데이터를 추가한 후 유저는 GetData()를 다시 호출할 필요가 있다. 바뀔 수 있으므로. */
	void CStreamQueue::PushBack_Copy(const uint8_t* data, int length)
	{
		// 블럭 크기가 초과되지 않는 경우 그냥 추가한다.
		if (m_headIndex + m_contentsLength + length < m_block.GetCount())
		{			
#if defined(_WIN32)
			BoundCheckInt32(m_headIndex+m_contentsLength,__FUNCTION__);
#endif
			UnsafeFastMemcpy(&m_block[m_headIndex+m_contentsLength], data, length);
			m_contentsLength += length;
		}
		else
		{
			// 추가하려 해도 블럭 크기를 초과하면 일단 앞으로 땡긴다.
			if (m_headIndex > 0 && m_block.GetCount() > 0)
			{
				Shrink();

			}
			// 블럭 크기가 초과되고 앞으로 땡기더라도 공간이 모자란 경우 블럭 크기를 재할당한다.
			if (m_contentsLength + length > m_block.GetCount())
			{
#if defined(_WIN32)
				BoundCheckInt32(m_contentsLength + length + m_growBy,__FUNCTION__);
#endif
				m_block.SetCount(m_contentsLength + length + m_growBy);
			}
			// 복사하기
#if defined(_WIN32)
			BoundCheckInt32(m_contentsLength,__FUNCTION__);
#endif
			UnsafeFastMemcpy(&m_block[m_contentsLength], data, length);
			m_contentsLength += length;
		}
	}

	void CStreamQueue::PushBack_Copy( const CSendFragRefs& sendData )
	{
		for (int i = 0;i < sendData.GetFragmentCount();i++)
		{
			if (sendData[i].GetData() != NULL)
			{
				PushBack_Copy(sendData[i].GetData(), sendData[i].GetLength());
			}
		}
	}
	/** head를 지정한 갯수만큼 pop한다.
	\return 실제로 pop이 된 갯수 */
	int CStreamQueue::PopFront( int length )
	{
		// head를 뒤로 민다.
		length = PNMIN(length, m_contentsLength);
		m_headIndex += length;
		m_contentsLength -= length;
		// 만약 거의 빈 상태가 되면 head를 맨 앞으로 땡긴다. 이러면 성능이 좋아지니까.
		if (m_contentsLength <= m_growBy / 64)
			Shrink();
		return length;
	}

	/** head를 맨 앞으로 땡긴다. */
	void CStreamQueue::Shrink()
	{
#if defined(_WIN32)
		BoundCheckInt32(m_headIndex,__FUNCTION__);
#endif
		if (m_contentsLength > 0)
			memmove(&m_block[0], &m_block[m_headIndex], m_contentsLength);
		m_headIndex = 0;
	}
}