
#include "stdafx.h"
#ifdef USE_HLA
#include "../include/HlaSpace_S.h"
#include "../include/Message.h"
#include "../include/MilisecTimer.h"
#include "../include/ReceivedMessage.h"
#include "SendFragRefs.h"
#include "../include/hlahost_s.h"
#include "HlaCritSec.h"
#include "HlaEntityImpl_S.h"
#include "HlaSessionHostImpl_S.h"
#include "HlaSpaceImpl_S.h"
//#include "NetCore.h"
#include "NetUtil.h"
#include "RmiContextImpl.h"
#include "enumimpl.h"
#include "SendOpt.h"

namespace Proud
{
	extern const PNTCHAR* AsyncCallbackMayOccurErrorText;
	extern const PNTCHAR* DuplicatedHlaEntityClassIDErrorText;

	CHlaSessionHostImpl_S::CHlaSessionHostImpl_S(IHlaSessionHostImplDelegate_S* dg)
		:m_entityIDGen(60)
	{
		m_implDg = dg;
		m_userDg = NULL;
	}

	CHlaSessionHostImpl_S::~CHlaSessionHostImpl_S(void)
	{
	}

	void CHlaSessionHostImpl_S::HlaAttachEntityTypes( CHlaEntityManagerBase_S* entityManager )
	{
		CHlaCritSecLock lock(m_userDg, true);

		if(entityManager->m_ownerHost != this && entityManager->m_ownerHost != NULL)
			ThrowInvalidArgumentException();

		if(m_implDg->AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);

		for (int ii = 0;ii < (int)m_entityManagerList_NOCSLOCK.GetCount();ii++)
		{
			CHlaEntityManagerBase_S *i = m_entityManagerList_NOCSLOCK[ii];

			if (IsCombinationEmpty<HlaClassID>(i->GetFirstEntityClassID(), (i->GetLastEntityClassID() + 1),
				entityManager->GetFirstEntityClassID(), (entityManager->GetLastEntityClassID() + 1)) == false)
			{
				throw Exception(DuplicatedHlaEntityClassIDErrorText);
			}
		}

		entityManager->m_ownerHost = this;
		m_entityManagerList_NOCSLOCK.Add(entityManager);
	}

	void CHlaSessionHostImpl_S::DestroyViewer( IHlaViewer_S * rc )
	{
		CHlaCritSecLock lock(m_userDg, true);

		HostID viewerID = rc->GetHostID();

		// viewer가 종속된 모든 space들로부터 탈출
		CFastSet<CHlaSpace_S*>& spaces= rc->GetViewingSpaces();

		while(!spaces.IsEmpty())
		{
			HlaUnviewSpace(viewerID, spaces.front());
		}

		m_viewers.RemoveKey(rc->GetHostID());
	}

	void CHlaSessionHostImpl_S::CreateViewer( IHlaViewer_S * rc )
	{
		CHlaCritSecLock lock(m_userDg, true);

		if(m_viewers.ContainsKey(rc->GetHostID()))
		{
			ThrowInvalidArgumentException();
		}
		m_viewers.Add(rc->GetHostID(), rc);
	}

	CHlaSpace_S* CHlaSessionHostImpl_S::HlaCreateSpace()
	{
		CHlaCritSecLock lock(m_userDg, true);

		CHlaSpace_S* space = new CHlaSpace_S;
		m_spaces.Add(space,0);

		return space;
	}

	void CHlaSessionHostImpl_S::HlaDestroySpace(CHlaSpace_S* space)
	{
		CHlaCritSecLock lock(m_userDg, true);

		// entity들 탈출
		while(!space->m_internal->m_entities.IsEmpty())
		{
			CHlaEntity_S* entity = space->m_internal->m_entities.front();
			entity->MoveToSpace(NULL);
		}

		// viewer들 탈출
		while(!space->m_internal->m_viewers.empty())
		{
			HostID viewerID = space->m_internal->m_viewers.begin()->first;
			HlaUnviewSpace(viewerID, space);
		}

		m_spaces.Remove(space);

		delete space;
	}

	// classID에 대응하는 객체가 없으면 NULL을 리턴함
	CHlaEntity_S* CHlaSessionHostImpl_S::HlaCreateEntity(HlaClassID classID)
	{
		CHlaCritSecLock lock(m_userDg, true);

		CHlaEntity_S* entity = NULL;
		for(int i=0;i<m_entityManagerList_NOCSLOCK.GetCount();i++)
		{
			entity = m_entityManagerList_NOCSLOCK[i]->CreateEntityByClassID(classID);

			if(entity != NULL)
				break;
		}

		if(entity == NULL)
			return NULL;

		entity->m_internal->m_instanceID = m_entityIDGen.Create(m_implDg->GetTimeMs());
		entity->m_internal->m_ownerSessionHost = this;
		m_entities.Add(entity->m_internal->m_instanceID, entity);

		return entity;
	}

	void CHlaSessionHostImpl_S::HlaDestroyEntity(CHlaEntity_S* entity)
	{
		CHlaCritSecLock lock(m_userDg, true);

		// 종속 space로부터 탈출
		entity->MoveToSpace(NULL);

		m_entities.Remove(entity->m_internal->m_instanceID);
		m_entityIDGen.Drop(m_implDg->GetTimeMs(), entity->m_internal->m_instanceID);

		if(entity->m_internal->m_isDirty)
		{
			m_dirtyEntities.RemoveAt(entity->m_internal->m_iterInDirtyList);
		}

		delete entity;
	}

	void CHlaSessionHostImpl_S::HlaViewSpace( HostID viewerID, CHlaSpace_S* space, double levelOfDetail )
	{
		CHlaCritSecLock lock(m_userDg, true);

		// 못찾으면 즐
		IHlaViewer_S* viewer;
		if(!m_viewers.TryGetValue(viewerID, viewer))
			return;

		// 이미 view 중이면 즐
		if(space->m_internal->m_viewers.find(viewerID) != space->m_internal->m_viewers.end())
			return;

		// space 내 entity들의 등장을 노티
		// TODO: 수신자는 동일한데 메시지는 여럿. 헤더를 뭉칠까? 그래봤자 3바이트인데.

		CFastSet<CHlaEntity_S*>::iterator iter = space->m_internal->m_entities.begin();
		for(;iter != space->m_internal->m_entities.end();++iter)
		{
			SendAppearMessage(&viewerID, 1, *iter);
		}

		// view 쌍방 등록
		space->m_internal->m_viewers.insert(map<HostID, IHlaViewer_S*>::value_type(viewerID, viewer));
		viewer->GetViewingSpaces().Add(space);
	}

	void CHlaSessionHostImpl_S::HlaUnviewSpace( HostID viewerID, CHlaSpace_S* space )
	{
		CHlaCritSecLock lock(m_userDg, true);

		// 못찾으면 즐
		IHlaViewer_S* viewer;
		if(!m_viewers.TryGetValue(viewerID, viewer))
			return;

		// 이미 unview 중이면 즐
		map<HostID, IHlaViewer_S*>::iterator iViewer = space->m_internal->m_viewers.find(viewerID);
		if(iViewer == space->m_internal->m_viewers.end())
			return;

		// space 내 entity들의 소멸을 노티
		// TODO: 수신자는 동일한데 메시지는 여럿. 헤더를 뭉칠까? 그래봤자 3바이트인데.
		CFastSet<CHlaEntity_S*>::iterator iter = space->m_internal->m_entities.begin();
		for(; iter != space->m_internal->m_entities.end(); ++iter)
		{
			SendDisappearMessage(&viewerID, 1, (*iter)->m_internal->m_instanceID);
		}

		// view 쌍방 등록
		space->m_internal->m_viewers.erase(iViewer);
		viewer->GetViewingSpaces().Remove(space);
	}

	void CHlaSessionHostImpl_S::EntityMoveToSpace( CHlaEntity_S* entity, CHlaSpace_S* space )
	{
		// 이미 lock 되어 있어야 하나, 그래도.
		CHlaCritSecLock lock(m_userDg, true);

		CHlaSpace_S* oldSpace = entity->m_internal->m_enteredSpace;

		// 빈 뷰어 객체들
		static map<HostID, IHlaViewer_S*> nilViewers;

		/* 기 space에서 빠져 나오면서 unview되는 viewer들이 있을 것이다. 그들에게 disappear를 쏜다.
		새 space로 들어가면서 view되는 viewer들이 있을 것이다. 그들에게 appear를 쏜다.
		단, disappear 직후 appear를 받으면 곤란하므로 그들에게는 아무것도 보내지 않는다.
		*/
		const map<HostID, IHlaViewer_S*>& oldViewers = oldSpace != NULL ? oldSpace->m_internal->m_viewers : nilViewers;
		const map<HostID, IHlaViewer_S*>& newViewers = space != NULL ? space->m_internal->m_viewers : nilViewers;

		map<HostID, IHlaViewer_S*>::const_iterator ov1 = oldViewers.begin();
		map<HostID, IHlaViewer_S*>::const_iterator ov2 = newViewers.begin();

		POOLED_LOCAL_VAR(HostIDArray, appearSendTo);
		POOLED_LOCAL_VAR(HostIDArray, disappearSendTo);

		size_t capacity = PNMAX(oldViewers.size(), newViewers.size());
		appearSendTo.SetMinCapacity(capacity);
		disappearSendTo.SetMinCapacity(capacity);

		while(1)
		{
			// 양쪽 다 루프 다 돌았으면 루프 쫑
			if(ov1 == oldViewers.end() && ov2 == newViewers.end())
				break;

			if(ov1 == oldViewers.end()) // 좌항이 끝이면 우항 나머지들은 등장을 노티해야
			{
				appearSendTo.Add(ov2->first);
				ov2++;
				continue;
			}
			else if(ov2 == newViewers.end()) // 반대
			{
				disappearSendTo.Add(ov1->first);
				ov1++;
				continue;
			}

			HostID ov1h = ov1->first;
			HostID ov2h = ov2->first;

			if(ov1h == ov2h) // 두 값이 같으면 appear,disappear가 동시에 가는 셈이나 다름없으므로 아무것도 보내지 말아야
			{
				ov1++;
				ov2++;
			}
			else if(ov1h < ov2h)	// 우항이 크면 좌항은 우항에 없으므로 좌항에 대해 사라져야 함을 노티해야
			{
				disappearSendTo.Add(ov1h);
				ov1++;
			}
			else	// 반대
			{
				appearSendTo.Add(ov2h);
				ov2++;
			}
		}

		// 멀티캐스트
		SendAppearMessage(appearSendTo.GetData(), appearSendTo.GetCount(), entity);
		SendDisappearMessage(disappearSendTo.GetData(), disappearSendTo.GetCount(), entity->m_internal->m_instanceID);

		// 새 값 갱신
		if(oldSpace != NULL)
			oldSpace->m_internal->m_entities.Remove(entity);
		if(space != NULL)
			space->m_internal->m_entities.Add(entity);

		entity->m_internal->m_enteredSpace = space;

	}

	void CHlaSessionHostImpl_S::SendAppearMessage( HostID* sendTo, int sendToCount, CHlaEntity_S* entity )
	{
		// SerializeAppear 호출 자체를 절약
		if(sendToCount == 0)
			return;

		// entity 내용 전체를 메시지로
		CMessage payload;
		payload.UseInternalBuffer();

		entity->SerializeAppear(payload);

		// 헤더 만들기
		CSmallStackAllocMessage header;
		Message_Write(header, MessageType_Hla);
		header.WriteScalar(HlaMessageType_Appear);
		header.WriteScalar(entity->m_internal->m_instanceID);
		header.WriteScalar(entity->m_internal->m_classID);

		CSendFragRefs fragRefs;
		fragRefs.Add(header);
		fragRefs.Add(payload.GetData(),payload.GetLength());

		// 멀티캐스트
		int outCompressedPayloadLength = 0;
		m_implDg->Send(fragRefs,SendOpt(g_ReliableSendForPN), sendTo, sendToCount, outCompressedPayloadLength);
	}

	void CHlaSessionHostImpl_S::SendDisappearMessage( HostID* sendTo, int sendToCount, HlaEntityID entityID )
	{
		if(sendToCount == 0)
			return;

		// 헤더 만들기
		CSmallStackAllocMessage header;
		Message_Write(header, MessageType_Hla);
		header.WriteScalar(HlaMessageType_Disappear);
		header.WriteScalar(entityID);

		CSendFragRefs fragRefs;
		fragRefs.Add(header);

		// 멀티캐스트
		int outCompressedPayloadLength = 0;
		m_implDg->Send(fragRefs,SendOpt(g_ReliableSendForPN), sendTo, sendToCount, outCompressedPayloadLength);
	}
	template<typename mapType, typename arrayType>
	void KeysToArray(const mapType& from, arrayType& to)
	{
		to.SetCount(from.size());

		int index = 0;
		for(mapType::const_iterator i=from.begin(); i!=from.end(); ++i)
		{
			to[index] = i->first;
			index++;
		}
	}

	void CHlaSessionHostImpl_S::HlaFrameMove()
	{
		CMessage entityBody;
		entityBody.UseInternalBuffer();

		POOLED_LOCAL_VAR(HostIDArray, sendTo);

		// 변경 사항이 있는 entity들을 모아서 그들의 viewer들에게 변화를 노티한다.
		while(m_dirtyEntities.GetCount() > 0)
		{
			CHlaEntity_S* e = m_dirtyEntities.GetHead();

			CHlaSpace_S* s = e->m_internal->m_enteredSpace;
			if(s != NULL && !s->m_internal->m_viewers.empty())
			{
				KeysToArray(s->m_internal->m_viewers, sendTo);

				entityBody.SetReadOffset(0);
				entityBody.SetLength(0);

				e->SerializeNotifyChange(entityBody);

				// 헤더 만들기
				CSmallStackAllocMessage header;
				Message_Write(header, MessageType_Hla);
				header.WriteScalar(HlaMessageType_NotifyChange);
				header.WriteScalar(e->m_internal->m_instanceID);

				CSendFragRefs fragRefs;
				fragRefs.Add(header);
				fragRefs.Add(entityBody.GetData(),entityBody.GetLength());

				// 멀티캐스트
				int outCompressedPayloadLength = 0;
				m_implDg->Send(fragRefs,SendOpt(g_ReliableSendForPN), sendTo.GetData(), sendTo.GetCount(), outCompressedPayloadLength);
			}

			// 보냈으니 클리어해야
			// TODO: cool time 안된거 처리하는 기능. viewer마다의 LOD.
			assert(e->m_internal->m_isDirty);
			e->m_internal->m_isDirty = false;
			m_dirtyEntities.RemoveHead();
		}
	}

	void CHlaSessionHostImpl_S::ProcessMessage( CReceivedMessage& receivedMessage )
	{
		DebugBreak();
	}

	void CHlaSessionHostImpl_S::HlaSetDelegate( IHlaDelegate_S* dg )
	{
		m_userDg = dg;
	}

}

#endif // USE_
