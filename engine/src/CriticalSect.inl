#pragma once 

#include "../include/BasicTypes.h"

namespace Proud
{

	inline void AssertIsLockedByCurrentThread(const CriticalSection &cs)
	{
		_pn_unused(cs);

#ifdef _DEBUG
#ifdef PN_LOCK_OWNER_SHOWN
		assert(IsCriticalSectionLockedByCurrentThread(cs));
#endif
#endif 
	}

	inline void AssertIsNotLockedByCurrentThread(const CriticalSection &cs)
	{
		_pn_unused(cs);

#ifdef _DEBUG
#ifdef PN_LOCK_OWNER_SHOWN
		assert(!IsCriticalSectionLockedByCurrentThread(cs));
#endif
#endif 
	}

}
