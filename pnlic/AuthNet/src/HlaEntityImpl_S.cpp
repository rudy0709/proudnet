
#include "stdafx.h"
#ifdef USE_HLA
#include "HlaEntityImpl_S.h"
#include "HlaSessionHostImpl_S.h"
#include "HlaSpaceImpl_S.h"
#include "HlaCritSec.h"
#include "../include/hlahost_s.h"

namespace Proud
{
	void CHlaEntity_S::SetClassID_INTERNAL( HlaClassID val )
	{
		m_internal->m_classID = val;
	}

	void CHlaEntity_S::MoveToSpace( CHlaSpace_S* space )
	{
		m_internal->m_ownerSessionHost->EntityMoveToSpace(this, space);
	}

	CHlaEntity_S::CHlaEntity_S()
	{
		m_internal = new CHlaEntityInternal_S;
	}

	CHlaEntity_S::~CHlaEntity_S()
	{
		delete m_internal;
	}

	Proud::HlaEntityID CHlaEntity_S::GetInstanceID()
	{
		return m_internal->m_instanceID;
	}

	void CHlaEntity_S::SetDirty_INTERNAL()
	{
		if(!m_internal->m_isDirty)
		{
			m_internal->m_isDirty = true;

			CHlaCritSecLock lock(m_internal->m_ownerSessionHost->m_userDg, true);
			m_internal->m_iterInDirtyList = m_internal->m_ownerSessionHost->m_dirtyEntities.AddTail(this);
		}
	}

}

#endif