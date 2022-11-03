#include "stdafx.h"
#include "PooledObjects.h"
#include "PooledObjectAsLocalVar.h"
#include "../include/ByteArrayPtr.h"

namespace Proud
{
	// pool singleton끼리도 의존관계가 있다. 예: CReceivedMessageList는 ByteArrayPtr를 의존한다.
	// 명시적으로 생성 순서를 정해야 하는 것들을 코딩이 드러워도 이렇게 따로 떼어냈다.
	// 이게 없으니 SimpleLan test에서 클라 종료시 이미 파괴된 singleton 참조로 인한 에러가 나 버림.
	// 게다가 CFavoritePooledObjects가 아래 멤버 변수를 직접 가질 수 없어서 (쌍방 참조 문제가 생겨)
	// 불가피하게 pimpl로 구현해 버렸다.
	class CFavoritePooledObjects_pimpl
	{
	public:
		CClassObjectPool<ByteArrayPtr::Tombstone>::Holder m_holder_ByteArrayPtr;
	};

	// 오랫동안 노는 object들을 제거한다.
	void CFavoritePooledObjects::ShrinkOnNeed()
	{
		CriticalSectionLock lock(m_critSec, true);// deadlock free. singleton 내 lock 후 이걸 lock 하는 일은 없으므로.
		for (RegisteredPoolSingletons::iterator i = m_registeredPoolSingletons.begin();
			i != m_registeredPoolSingletons.end(); i++)
		{
			ISingletonHolder* h = *i;
			h->GetSubstance()->ShrinkOnNeed();
		}
	}

	CFavoritePooledObjects::~CFavoritePooledObjects()
		{
		// 등록된 순서의 반대로 파괴되어야 한다. 그들끼리도 의존하는 관계가 있기 때문이다. 가령 ByteArrayPtr의 tombstone.
		while (m_registeredPoolSingletons.GetCount() > 0)
		{
			m_registeredPoolSingletons.RemoveTailNoReturn();
		}

		delete m_pimpl;
		}

	CFavoritePooledObjects::CFavoritePooledObjects()
		{
		m_pimpl = new CFavoritePooledObjects_pimpl;
	}


	// 	void CFavoritePooledObjects::Unregister(IClassObjectPool* singletonHolder)
	// 	{
	// 		CriticalSectionLock lock(m_critSec, true);
	// 		m_registeredPoolSingletons.RemoveKey(singletonHolder);
	// 	}

	// 	CFavoritePooledObjects::~CFavoritePooledObjects()
	// 	{
	// 		CriticalSectionLock lock(m_critSec, true);// deadlock free. singleton 내 lock 후 이걸 lock 하는 일은 없으므로.
	// 
	// 		for (CFastList2<IPoolSingletonholder*, int>::iterator i = m_singletonHolders.begin();
	// 			i != m_singletonHolders.end(); i++)
	// 		{
	// 			IPoolSingletonholder* holder = *i;
	// 			delete holder;
	// 		}
	// 	}


}
