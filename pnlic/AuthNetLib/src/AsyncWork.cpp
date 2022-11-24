#include "stdafx.h"
#include "AsyncWork.h"

namespace Proud 
{
	CAsyncWork::CAsyncWork( CAsyncWorksOwner* owner)
	{
		m_owner = owner;
		AtomicIncrement32(&m_owner->m_asyncWorkCount);
	}

	CAsyncWork::~CAsyncWork()
	{
		AtomicDecrement32(&m_owner->m_asyncWorkCount);
	}


	CAsyncWorksOwner::CAsyncWorksOwner()
	{
		m_asyncWorkCount = 0;
	}

	bool CAsyncWorksOwner::IsFinished()
	{
		return m_asyncWorkCount == 0;
	}

}