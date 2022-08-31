#include "stdafx.h"
#include "HlaEntityImpl_C.h"
#if !defined(_WIN32)
	#include <new>
#endif

namespace Proud
{
	CHlaEntity_C::CHlaEntity_C()
	{
		m_internal = new CHlaEntityInternal_C;
	}

	CHlaEntity_C::~CHlaEntity_C()
	{
		delete m_internal;
	}


	Proud::HlaEntityID CHlaEntity_C::GetInstanceID()
	{
		return m_internal->m_instanceID;
	}

	void CHlaEntity_C::SetDirty_INTERNAL()
	{
#if defined(_WIN32)
		DebugBreak();
#endif
	}

	void CHlaEntity_C::SetClassID_INTERNAL( HlaClassID val )
	{
		m_internal->m_classID = val;
	}
}
