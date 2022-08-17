#include "stdafx.h"
#include "../include/Message.h"
//#include "../include/SynchEntity.h"
//#include "../include/SynchViewport.h"
#include "../include/hlaruntime.h"
#include "../include/ptr.h"
#include <algorithm>

namespace Proud
{
	CHlaRuntime_S::CHlaRuntime_S(IHlaRuntimeDelegate_S* dg)
	{
		m_dg = dg;
		m_instanceIDFactory = HlaInstanceID_None;
	}

	CHlaRuntime_S::~CHlaRuntime_S(void)
	{
		if (m_synchViewports.size() > 0)
			throw Exception(L"All SynchViewports must have been unregistered before destruction of HLA runtime!");
		if (m_synchEntities.size() > 0)
			throw Exception(L"All SynchEntities must have been unregistered before destruction of HLA runtime!");
	}

	HlaInstanceID CHlaRuntime_S::CreateInstanceID()
	{
		m_instanceIDFactory++;
		return m_instanceIDFactory;
	}

	/** send the changes to clients */
	void CHlaRuntime_S::FrameMove( double elapsedTime )
	{
		for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
		{
			CMessage msg;			// TODO: reuse this object's buffer to reduce memory fragmentation and increase performance!

			SynchEntity_S* entity = i->second;
			if (entity->HasChanges())
			{
				Protocol protocol = Protocol_Undefined;

				msg << HlaMessageType_ShowState;
				msg << entity->GetInstanceID();

				entity->GatherTheChangeToMessage(msg, protocol);
				entity->ClearTheChanges();

				vector<HostID> sendToList;	// TODO: consider memory fragmentation here!
				entity->GetViewportOwnerHostList(sendToList);

				Send_INTERNAL(sendToList, protocol, msg);
			}
		}
	}

	void CHlaRuntime_S::Check_OneSynchEntity_EveryViewport( SynchEntity_S *entity )
	{
		CMessage appearMsg;
		CMessage disappearMsg;

		for (SynchViewports_S::iterator i = m_synchViewports.begin();i != m_synchViewports.end();i++)
		{
			SynchViewport_S* viewport = i->second;
			Check_OneSynchEntity_OneViewport(entity, viewport, appearMsg, disappearMsg);
		}
	}

	/** remove one synch viewport from the containers and broadcast it */
	void CHlaRuntime_S::RemoveOneSynchViewport_INTERNAL( SynchViewport_S* viewport )
	{
		CMessage disappearMsg;
		for (SynchViewport_S::SynchEntityList::iterator i = viewport->m_tangibles_INTERNAL.begin();i != viewport->m_tangibles_INTERNAL.end();i++)
		{
			SynchEntity_S* entity = *i;
			entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
			m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
			entity->m_beholders_INTERNAL.erase(viewport);
		}
		viewport->m_tangibles_INTERNAL.clear();
	}

	/** remove one synch from the containers and broadcast it */
	void CHlaRuntime_S::RemoveOneSynchEntity_INTERNAL( SynchEntity_S* entity )
	{
		CMessage disappearMsg;
		for (SynchEntity_S::SynchViewportList::iterator i = entity->m_beholders_INTERNAL.begin();i != entity->m_beholders_INTERNAL.end();i++)
		{
			SynchViewport_S* viewport = *i;
			entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
			m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
			viewport->m_tangibles_INTERNAL.erase(entity);
		}
		entity->m_beholders_INTERNAL.clear();
	}

	void CHlaRuntime_S::Check_OneSynchEntity_OneViewport( SynchEntity_S * entity, SynchViewport_S* viewport, CMessage appearMsg, CMessage disappearMsg )
	{
		if (m_dg->IsOneSynchEntityTangibleToOneViewport(entity, viewport) == true)
		{
			// send the appearance event to those who are newly involving that entity
			// and associate them. however, just skip if there's no need doing it.
			if (viewport->AddTangible_INTERNAL(entity) == true)
			{
				entity->CreateAppearMessageOnNeed_INTERNAL(appearMsg);
				m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, appearMsg);
				entity->m_beholders_INTERNAL.insert(viewport);
			}
		}
		else
		{
			// vice versa of the above
			if (viewport->RemoveTangible_INTERNAL(entity) == true)
			{
				entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
				m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
				entity->m_beholders_INTERNAL.erase(viewport);
			}
		}
	}

	IHlaRuntimeDelegate_S::~IHlaRuntimeDelegate_S()
	{

	}

	void CHlaRuntime_S::Check_EverySynchEntity_OneViewport(SynchViewport_S *viewport)
	{
		for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
		{
			// this variables must be initialized here because each synchentity generates a different message.
			CMessage appearMsg;
			CMessage disappearMsg;

			SynchEntity_S* entity = i->second;
			Check_OneSynchEntity_OneViewport(entity, viewport, appearMsg, disappearMsg);
		}
	}

	/** N:N check */
	void CHlaRuntime_S::Check_EverySynchEntity_EveryViewport()
	{
		for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
		{
			SynchEntity_S* entity = i->second;
			Check_OneSynchEntity_EveryViewport(entity);
		}
	}

	// broadcast messages
	void CHlaRuntime_S::Send_INTERNAL( vector<HostID> sendTo, Protocol protocol, CMessage &msg )
	{
		for (vector<HostID>::iterator i = sendTo.begin();i != sendTo.end();i++)
		{
			m_dg->Send(*i, protocol, msg);
		}
	}
	/** process received messages.

	\param msg The received message. Note that the user should set the reading point(cursor) where HLA protocol layer can identify */
	void CHlaRuntime_C::ProcessReceivedMessage( CMessage& msg )
	{
		HlaMessageType type;
		if (msg.Read(type) == false)
			return;

		switch (type)
		{
		case HlaMessageType_Appear:
			{
				HlaInstanceID instanceID;
				HlaClassID classID;
				if (msg.Read(instanceID) && msg.Read(classID))
				{
					RefCount<SynchEntity_C> entity(CreateSynchEntityByClassID(classID, instanceID));
					if (entity != NULL)
					{
						entity->UpdateFromMessage(msg);
						entity->m_instanceID_INTERNAL = instanceID;
						m_synchEntities.insert(SynchEntities_C::value_type(entity->GetInstanceID(), entity));
					}
					m_lastProcessMessageReport.Format(L"Appear: ClassID=%d,InstanceID=%d", classID, instanceID);
				}
			}
			break;
		case HlaMessageType_Disappear:
			{
				HlaInstanceID instanceID;
				if (msg.Read(instanceID))
				{
					m_synchEntities.erase(instanceID);
				}
				m_lastProcessMessageReport.Format(L"Disppear: InstanceID=%d", instanceID);
			}
			break;
		case HlaMessageType_ShowState:
			{
				HlaInstanceID instanceID;
				if (msg.Read(instanceID))
				{
					SynchEntity_C* entity = GetSynchEntityByID(instanceID);
					if (entity != NULL)
					{
						entity->UpdateFromMessage(msg);
					}
				}
				m_lastProcessMessageReport.Format(L"ShowState: InstanceID=%d", instanceID);
			}
			break;
		}

	}

	SynchEntity_C* CHlaRuntime_C::CreateSynchEntityByClassID( HlaClassID classID , HlaInstanceID instanceID)
	{
		for (DgList::iterator i = m_dgList.begin();i != m_dgList.end();i++)
		{
			SynchEntity_C* ret = (*i)->CreateSynchEntityByClassID(classID);
			if (ret != NULL)
			{
				ret->m_instanceID_INTERNAL = instanceID;
				return ret;
			}

		}

		return NULL;
	}

	SynchEntity_C* CHlaRuntime_C::GetSynchEntityByID( HlaInstanceID instanceID )
	{
		SynchEntities_C::iterator i = m_synchEntities.find(instanceID);
		if (i == m_synchEntities.end())
			return NULL;
		else
			return i->second;
	}

	void CHlaRuntime_C::AddDelegate( IHlaRuntimeDelegate_C *dg )
	{
		if (std::find(m_dgList.begin(), m_dgList.end(), dg) != m_dgList.end())
			throw Exception(L"Duplicated HLA runtime delegate cannot be added!");
		m_dgList.push_back(dg);
	}

	String CHlaRuntime_C::GetLastProcessMessageReport()
	{
		return m_lastProcessMessageReport;
	}

	void CHlaRuntime_C::Clear()
	{
		m_synchEntities.clear();
	}

	IHlaRuntimeDelegate_C::~IHlaRuntimeDelegate_C()
	{

	}

	Protocol CombineProtocol( Protocol a, Protocol b )
	{
		return (Protocol)max((int)a, (int)b);
	}
}
==== THEIRS HlaRuntime.cpp#10
﻿
==== YOURS HlaRuntime.cpp
﻿#include "stdafx.h"
#include <new>
#include "../include/Message.h"
#include "../include/SynchEntity.h"
#include "../include/SynchViewport.h"
#include "../include/hlaruntime.h"
#include "../include/ptr.h"

#ifndef __GNUC__
#include <algorithm>
#endif

namespace Proud
{
#ifndef __GNUC__
CHlaRuntime_S::CHlaRuntime_S(IHlaRuntimeDelegate_S* dg)
{
	m_dg = dg;
	m_instanceIDFactory = HlaInstanceID_None;
}

CHlaRuntime_S::~CHlaRuntime_S(void)
{
	if (m_synchViewports.size() > 0)
		throw Exception(L"All SynchViewports must have been unregistered before destruction of HLA runtime!");
	if (m_synchEntities.size() > 0)
		throw Exception(L"All SynchEntities must have been unregistered before destruction of HLA runtime!");
}

HlaInstanceID CHlaRuntime_S::CreateInstanceID()
{
	m_instanceIDFactory++;
	return m_instanceIDFactory;
}

/** send the changes to clients */
void CHlaRuntime_S::FrameMove( double elapsedTime )
{
	for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
	{
		CMessage msg;			// TODO: reuse this object's buffer to reduce memory fragmentation and increase performance!

		SynchEntity_S* entity = i->second;
		if (entity->HasChanges())
		{
			Protocol protocol = Protocol_Undefined;

			msg << HlaMessageType_ShowState;
			msg << entity->GetInstanceID();

			entity->GatherTheChangeToMessage(msg, protocol);
			entity->ClearTheChanges();

			vector<HostID> sendToList;	// TODO: consider memory fragmentation here!
			entity->GetViewportOwnerHostList(sendToList);

			Send_INTERNAL(sendToList, protocol, msg);
		}
	}
}

void CHlaRuntime_S::Check_OneSynchEntity_EveryViewport( SynchEntity_S *entity )
{
	CMessage appearMsg;
	CMessage disappearMsg;

	for (SynchViewports_S::iterator i = m_synchViewports.begin();i != m_synchViewports.end();i++)
	{
		SynchViewport_S* viewport = i->second;
		Check_OneSynchEntity_OneViewport(entity, viewport, appearMsg, disappearMsg);
	}
}

/** remove one synch viewport from the containers and broadcast it */
void CHlaRuntime_S::RemoveOneSynchViewport_INTERNAL( SynchViewport_S* viewport )
{
	CMessage disappearMsg;
	for (SynchViewport_S::SynchEntityList::iterator i = viewport->m_tangibles_INTERNAL.begin();i != viewport->m_tangibles_INTERNAL.end();i++)
	{
		SynchEntity_S* entity = *i;
		entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
		m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
		entity->m_beholders_INTERNAL.erase(viewport);
	}
	viewport->m_tangibles_INTERNAL.clear();
}

/** remove one synch from the containers and broadcast it */
void CHlaRuntime_S::RemoveOneSynchEntity_INTERNAL( SynchEntity_S* entity )
{
	CMessage disappearMsg;
	for (SynchEntity_S::SynchViewportList::iterator i = entity->m_beholders_INTERNAL.begin();i != entity->m_beholders_INTERNAL.end();i++)
	{
		SynchViewport_S* viewport = *i;
		entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
		m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
		viewport->m_tangibles_INTERNAL.erase(entity);
	}
	entity->m_beholders_INTERNAL.clear();
}

void CHlaRuntime_S::Check_OneSynchEntity_OneViewport( SynchEntity_S * entity, SynchViewport_S* viewport, CMessage appearMsg, CMessage disappearMsg )
{
	if (m_dg->IsOneSynchEntityTangibleToOneViewport(entity, viewport) == true)
	{
		// send the appearance event to those who are newly involving that entity
		// and associate them. however, just skip if there's no need doing it.
		if (viewport->AddTangible_INTERNAL(entity) == true)
		{
			entity->CreateAppearMessageOnNeed_INTERNAL(appearMsg);
			m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, appearMsg);
			entity->m_beholders_INTERNAL.insert(viewport);
		}
	}
	else
	{
		// vice versa of the above
		if (viewport->RemoveTangible_INTERNAL(entity) == true)
		{
			entity->CreateDisappearMessageOnNeed_INTERNAL(disappearMsg);
			m_dg->Send(viewport->GetHostID(), Protocol_ReliableSend, disappearMsg);
			entity->m_beholders_INTERNAL.erase(viewport);
		}
	}
}

IHlaRuntimeDelegate_S::~IHlaRuntimeDelegate_S()
{

}

void CHlaRuntime_S::Check_EverySynchEntity_OneViewport(SynchViewport_S *viewport)
{
	for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
	{
		// this variables must be initialized here because each synchentity generates a different message.
		CMessage appearMsg;
		CMessage disappearMsg;

		SynchEntity_S* entity = i->second;
		Check_OneSynchEntity_OneViewport(entity, viewport, appearMsg, disappearMsg);
	}
}

/** N:N check */
void CHlaRuntime_S::Check_EverySynchEntity_EveryViewport()
{
	for (SynchEntities_S::iterator i = m_synchEntities.begin();i != m_synchEntities.end();i++)
	{
		SynchEntity_S* entity = i->second;
		Check_OneSynchEntity_EveryViewport(entity);
	}
}

// broadcast messages
void CHlaRuntime_S::Send_INTERNAL( vector<HostID> sendTo, Protocol protocol, CMessage &msg )
{
	for (vector<HostID>::iterator i = sendTo.begin();i != sendTo.end();i++)
	{
		m_dg->Send(*i, protocol, msg);
	}
}
/** process received messages.

\param msg The received message. Note that the user should set the reading point(cursor) where HLA protocol layer can identify */
void CHlaRuntime_C::ProcessReceivedMessage( CMessage& msg )
{
	HlaMessageType type;
	if (msg.Read(type) == false)
		return;

	switch (type)
	{
	case HlaMessageType_Appear:
	{
		HlaInstanceID instanceID;
		HlaClassID classID;
		if (msg.Read(instanceID) && msg.Read(classID))
		{
			RefCount<SynchEntity_C> entity(CreateSynchEntityByClassID(classID, instanceID));
			if (entity != NULL)
			{
				entity->UpdateFromMessage(msg);
				entity->m_instanceID_INTERNAL = instanceID;
				m_synchEntities.insert(SynchEntities_C::value_type(entity->GetInstanceID(), entity));
			}
			m_lastProcessMessageReport.Format(L"Appear: ClassID=%d,InstanceID=%d", classID, instanceID);
		}
	}
	break;
	case HlaMessageType_Disappear:
	{
		HlaInstanceID instanceID;
		if (msg.Read(instanceID))
		{
			m_synchEntities.erase(instanceID);
		}
		m_lastProcessMessageReport.Format(L"Disppear: InstanceID=%d", instanceID);
	}
	break;
	case HlaMessageType_ShowState:
	{
		HlaInstanceID instanceID;
		if (msg.Read(instanceID))
		{
			SynchEntity_C* entity = GetSynchEntityByID(instanceID);
			if (entity != NULL)
			{
				entity->UpdateFromMessage(msg);
			}
		}
		m_lastProcessMessageReport.Format(L"ShowState: InstanceID=%d", instanceID);
	}
	break;
	}

}

SynchEntity_C* CHlaRuntime_C::CreateSynchEntityByClassID( HlaClassID classID , HlaInstanceID instanceID)
{
	for (DgList::iterator i = m_dgList.begin();i != m_dgList.end();i++)
	{
		SynchEntity_C* ret = (*i)->CreateSynchEntityByClassID(classID);
		if (ret != NULL)
		{
			ret->m_instanceID_INTERNAL = instanceID;
			return ret;
		}

	}

	return NULL;
}

SynchEntity_C* CHlaRuntime_C::GetSynchEntityByID( HlaInstanceID instanceID )
{
	SynchEntities_C::iterator i = m_synchEntities.find(instanceID);
	if (i == m_synchEntities.end())
		return NULL;
	else
		return i->second;
}

void CHlaRuntime_C::AddDelegate( IHlaRuntimeDelegate_C *dg )
{
	if (std::find(m_dgList.begin(), m_dgList.end(), dg) != m_dgList.end())
		throw Exception(L"Duplicated HLA runtime delegate cannot be added!");
	m_dgList.push_back(dg);
}

String CHlaRuntime_C::GetLastProcessMessageReport()
{
	return m_lastProcessMessageReport;
}

void CHlaRuntime_C::Clear()
{
	m_synchEntities.clear();
}

IHlaRuntimeDelegate_C::~IHlaRuntimeDelegate_C()
{

}

Protocol CombineProtocol( Protocol a, Protocol b )
{
	return (Protocol)max((int)a, (int)b);
}
#endif
}<<<<
