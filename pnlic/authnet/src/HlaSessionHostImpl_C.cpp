#include "stdafx.h"
// xcode는 STL을 cpp에서만 include 가능 하므로
#include <new>
#include <stack>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <iosfwd>

#include "../include/HlaHost_C.h"
#include "../include/ReceivedMessage.h"
#include "HlaSessionHostImpl_C.h"
#include "HlaCritSec.h"
#include "NetUtil.h"
#include "HlaEntityImpl_C.h"
#if !defined(_WIN32)
    #include <new>
#endif

namespace Proud
{
	extern const PNTCHAR* AsyncCallbackMayOccurErrorText;
	const PNTCHAR* DuplicatedHlaEntityClassIDErrorText = _PNT("Duplicated Entity Class ID is found. Review HLA declaration in .hla files.");
	const PNTCHAR* HlaUninitErrorText = _PNT("HlaSetDelegate must be called prior to using any HLA feature!");

	CHlaSessionHostImpl_C::CHlaSessionHostImpl_C(IHlaSessionHostImplDelegate_C* dg)
	{
		m_implDg = dg;
		m_userDg = NULL;
	}

	CHlaSessionHostImpl_C::~CHlaSessionHostImpl_C(void)
	{
	}

	void CHlaSessionHostImpl_C::HlaAttachEntityTypes( CHlaEntityManagerBase_C* entityManager )
	{
		CHlaCritSecLock lock(m_userDg, true);

		if(entityManager->m_ownerHost != this && entityManager->m_ownerHost != NULL)
			ThrowInvalidArgumentException();

		if(m_implDg->AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);

		for (int ii = 0;ii < (int)m_entityManagerList_NOCSLOCK.GetCount();ii++)
		{
			CHlaEntityManagerBase_C *i = m_entityManagerList_NOCSLOCK[ii];

			if (IsCombinationEmpty<HlaClassID>(i->GetFirstEntityClassID(), (i->GetLastEntityClassID() + 1),
				entityManager->GetFirstEntityClassID(), (entityManager->GetLastEntityClassID() + 1)) == false)
			{
				throw Exception(DuplicatedHlaEntityClassIDErrorText);
			}
		}

		entityManager->m_ownerHost = this;
		m_entityManagerList_NOCSLOCK.Add(entityManager);
	}

	void CHlaSessionHostImpl_C::HlaFrameMove()
	{
		// TODO: 만들자
	}

	void CHlaSessionHostImpl_C::ProcessMessage( CReceivedMessage &receivedMessage )
	{
		CHlaCritSecLock lock(m_userDg, true);

		CMessage& msg = receivedMessage.m_unsafeMessage;
		uint64_t temp;
		HlaMessageType type;
		if(!msg.ReadScalar(temp))
			return;

		type = (HlaMessageType)temp;

		switch(type)
		{
            case HlaMessageType_Appear:
                ProcessMessageType_Appear(receivedMessage);
                break;
            case HlaMessageType_Disappear:
                ProcessMessageType_Disappear(receivedMessage);
                break;
            case HlaMessageType_NotifyChange:
                ProcessMessageType_NotifyChange(receivedMessage);
                break;
            default:
                break;
		}
	}

	void CHlaSessionHostImpl_C::ProcessMessageType_Appear( CReceivedMessage & receivedMessage )
	{
		CMessage& msg = receivedMessage.m_unsafeMessage;

		HlaEntityID instanceID;
		HlaClassID classID;

		uint64_t temp;
		if (!msg.ReadScalar(instanceID))
			return;

		if (!msg.ReadScalar(temp))
			return;

		classID = (HlaClassID)temp;

		CHlaEntity_C* e = HlaCreateEntity(classID, instanceID);
		if(e == NULL)
			return;

		try
		{
			e->DeserializeAppear(msg);
			m_userDg->HlaOnEntityAppear(e);
		}
		catch(Exception &)
		{
			// >> 에서 실패했을 터. 개체 생성을 취소해야.
			UnregisterEntity(e->m_internal->m_instanceID);
			delete e;
		}
	}

	void CHlaSessionHostImpl_C::ProcessMessageType_Disappear( CReceivedMessage & receivedMessage )
	{
		CMessage& msg = receivedMessage.m_unsafeMessage;

		HlaEntityID instanceID;

		if (!msg.ReadScalar(instanceID))
			return;
		
		CHlaEntity_C* e;
		if(m_entities.TryGetValue(instanceID, e))
		{
			UnregisterEntity(instanceID);
			m_userDg->HlaOnEntityDisappear(e);
		}
	}

	void CHlaSessionHostImpl_C::ProcessMessageType_NotifyChange( CReceivedMessage & receivedMessage )
	{
		CMessage& msg = receivedMessage.m_unsafeMessage;

		HlaEntityID instanceID;

		if (!msg.ReadScalar(instanceID))
			return;

		CHlaEntity_C* e;
		if(!m_entities.TryGetValue(instanceID, e))
			return;

		try
		{
			e->DeserializeNotifyChange(msg);
		}
		catch(Exception &)
		{
			// >> 에서 실패했을 터. 그냥 무시를 해야.
		}
	}

	// classID에 대응하는 객체가 없으면 NULL을 리턴함
	// 사용자가 직접 콜 불가
	CHlaEntity_C* CHlaSessionHostImpl_C::HlaCreateEntity( HlaClassID classID, HlaEntityID entityID )
	{
		CHlaCritSecLock lock(m_userDg, true);

		CHlaEntity_C* entity = NULL;
		for(int i=0;i<m_entityManagerList_NOCSLOCK.GetCount();i++)
		{
			entity = m_entityManagerList_NOCSLOCK[i]->CreateEntityByClassID(classID);

			if(entity != NULL)
				break;
		}

		if(entity == NULL)
			return NULL;

		entity->m_internal->m_instanceID = entityID;
		entity->m_internal->m_ownerSessionHost = this;
		m_entities.Add(entity->m_internal->m_instanceID, entity);

		return entity;
	}

	// 사용자가 직접 콜 불가
	void CHlaSessionHostImpl_C::UnregisterEntity( HlaEntityID entityID )
	{
		CHlaCritSecLock lock(m_userDg, true);

		CHlaEntity_C* e;
		if(m_entities.TryGetValue(entityID, e))
		{
			m_entities.Remove(entityID);
						
			/* entity를 지우는 것은 HlaOnEntityDisappear에서 사용자의 몫! 
			따라서 delete e; 같은걸 안한다.
			(C# 버전이면 굳이 지울 필요는 없지만, HLA는 성능에도 민감하고, 
			DB cache와 달리 값 억세스가 잦으므로, copy 비용이 매우 큰 RefCount를 안 쓴다.) */
		}
	}

	/* 내부 청소.
	소유하고 있는 entity들 자체를 파괴하지는 않는다. 파괴는 유저의 몫이다.
	m_entityManagerList_NOCSLOCK는 냅둔다. 유저가 재사용하는 경우를 위해서다. */
	void CHlaSessionHostImpl_C::Clear()
	{
		CHlaCritSecLock lock(m_userDg, true);

		// entity들이 일괄 파괴되므로, 유저에게 알려줘야 한다. 그래야 파괴해주지.
		while(!m_entities.IsEmpty())
		{
            CHlaEntities_C::iterator i = m_entities.begin();
			CHlaEntity_C* e = i.GetSecond();
			UnregisterEntity(e->m_internal->m_instanceID);
			m_userDg->HlaOnEntityDisappear(e);
		}
	}

	void CHlaSessionHostImpl_C::HlaSetDelegate( IHlaDelegate_C* dg )
	{
		m_userDg = dg;
	}

}
