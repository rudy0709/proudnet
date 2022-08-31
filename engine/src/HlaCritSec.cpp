#include "stdafx.h"
#include "../include/HlaDelagate_Common.h"
#include "HlaCritSec.h"
#include "../include/Exception.h"

namespace Proud
{
	CHlaCritSecLock::CHlaCritSecLock( IHlaDelegate_Common* lockee, bool initLock /*= false*/ )
	{
		m_lockee = lockee;
		m_isLocked = false;

		if(initLock)
			Lock();
	}

	CHlaCritSecLock::~CHlaCritSecLock()
	{
		Unlock();
	}

	void CHlaCritSecLock::Lock()
	{
		if(m_lockee == NULL)
			throw Exception("HlaSetDelegate must be called prior to any HLA commands!");

		if(!m_isLocked)
		{
			m_isLocked = true;
			m_lockee->HlaOnLockCriticalSection();
		}
	}

	void CHlaCritSecLock::Unlock()
	{
		if(m_isLocked)
		{
			m_isLocked = false;
			m_lockee->HlaOnUnlockCriticalSection();
		}
	}
}
