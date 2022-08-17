#include "stdafx.h"
#include "SendReadyList.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// 코딩시 주의사항: critsec가 non-recursive이다. 재귀 호출 없도록 할 것.

	int CSendReadySockets::GetCount() const
	{
		SpinLock lock(m_critSec,true);
		return m_sendReadySockets.GetCount();
	}

	CSendReadySockets::CSendReadySockets()
	{
	}

	// UDP socket or TCP remote object를 목록에 추가하되, 이미 있으면 추가를 하지 않는다.
	void CSendReadySockets::AddOrSet(const shared_ptr<CSuperSocket>& socket)
	{
		SpinLock lock(m_critSec, true);

		if (!socket)
		{
			assert(0);
			return;
		}

		// Add 함수는, 이미 있으면 그냥 냅둔다. 없으면 추가한다.
		if (socket->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList == NULL)
		{
			socket->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList =
				m_sendReadySockets.AddTail(socket);
			socket->m_ACCESSED_ONLY_BY_SendReadySockets_Owner = this;
		}
		else
		{
			// validation check
			if (socket->m_ACCESSED_ONLY_BY_SendReadySockets_Owner != this)
				throw Exception("Wrong SendReadyList.Add!");

			// do nothing
		}


	}

	void CSendReadySockets::Remove(const shared_ptr<CSuperSocket>& socket)
	{
		SpinLock lock(m_critSec,true);

		if (socket->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList)
		{
			if(socket->m_ACCESSED_ONLY_BY_SendReadySockets_Owner != this)
				throw Exception("Wrong SendReadyList.Remove!");

			m_sendReadySockets.RemoveAt(socket->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList);
			socket->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList = NULL;
			socket->m_ACCESSED_ONLY_BY_SendReadySockets_Owner = NULL;
		}
	}

	// send ready socket들을 모두 뽑는다.
	// 예전에는 하나씩 뽑았으나 concurrency visualizer 측정 결과 contention이 너무 심해서(lock convoy) 한 큐에 뽑음.
	// weak_ptr to shared_ptr이므로 lock 실패시 꺼낸 후 그냥 버린다.
	void CSendReadySockets::PopKeys(CSuperSocketArray& output)
	{
		SpinLock lock(m_critSec,true);

		// 미리 크기 할당
		output.SetCount(m_sendReadySockets.GetCount());

		int c = 0;
		for (SendReadySocketList::iterator i = m_sendReadySockets.begin();
			i != m_sendReadySockets.end(); i++)
		{
			// shared_ptr의 높은 복사 비용 때문에 바로 shared_ptr 객체의 주소 위치에 기록한다.
			// C++11을 지원하면 std::move를, 미지원시 복사를.
			shared_ptr<CSuperSocket>& dest = output[c];

			// weak_ptr to shared_ptr
			dest = (*i).lock();

			// 다음. 단, lock이 성공했다면.
			if (dest)
			{
				c++;

				// 위에가 성공하든 실패하든 어쨌건 꺼내왔으므로
				dest->m_ACCESSED_ONLY_BY_SendReadySockets_Owner = NULL;
				dest->m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList = NULL;
			}
		}

		// 실제 꺼내온 크기는 더 적을 수 있다. resize하자.
		output.SetCount(c);

		// 다 꺼내주었으니 청소
		m_sendReadySockets.Clear();
	}
}
