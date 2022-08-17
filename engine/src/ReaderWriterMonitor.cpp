/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/ReaderWriterMonitor.h"
#include "../include/sysutil.h"

namespace Proud
{
#if defined(_WIN32)
	CReaderWriterMonitor_NORECURSE::CReaderWriterMonitor_NORECURSE(void):
		m_noReaderLocked(true,true),
		m_noWriterLocked(true,true)
	{
		m_readerLockCount = 0;
		m_writerLockCount = 0;
		m_owningThread = 0;
	}

	CReaderWriterMonitor_NORECURSE::~CReaderWriterMonitor_NORECURSE(void)
	{
	}

	int CReaderWriterMonitor_NORECURSE::ReaderLock()
	{
		while(1)
		{
			CriticalSectionLock lock(m_cs,true);
			// writer lock이 전혀 없을 때까지 대기한다.
			if(m_writerLockCount>0)
			{
				lock.Unlock();
				m_noWriterLocked.WaitOne();
			}
			else
			{
				m_readerLockCount++;
				m_noReaderLocked.Reset();
				return m_readerLockCount;
			}
		}
	}

	void CReaderWriterMonitor_NORECURSE::ReaderUnlock()
	{
		CriticalSectionLock lock(m_cs,true);
		m_readerLockCount--;
		if(m_readerLockCount <= 0)
		{
			m_noReaderLocked.Set();
		}
	}

	void CReaderWriterMonitor_NORECURSE::WriterLock()
	{
		while(1)
		{
			CriticalSectionLock lock(m_cs,true);
			// lock이 전혀 없을 때까지 대기한다.
			if(m_writerLockCount>0 || m_readerLockCount > 0)
			{
				Event* events[2];
				events[0] = &m_noReaderLocked;
				events[1] = &m_noWriterLocked;
				lock.Unlock();
				Event::WaitAll(events,2);
			}
			else
			{
				m_writerLockCount++;
				m_noWriterLocked.Reset();
				return;
			}
		}
	}

	void CReaderWriterMonitor_NORECURSE::WriterUnlock()
	{
		CriticalSectionLock lock(m_cs,true);
		m_writerLockCount--;
		if(m_writerLockCount <= 0)
		{
			m_noWriterLocked.Set();
		}
	}

	void CReaderWriterAccessChecker::AssertThreadID( eAccessMode eMode ) const
	{
		_pn_unused(eMode);
#ifdef _DEBUG
		assert(eAccessMode_None != eMode);
		eAccessMode oldMode = (eAccessMode)InterlockedExchange((LONG*)&m_AccessMode, eMode);

		uint32_t currThread = ::GetCurrentThreadId();
		uint32_t oldThread = InterlockedExchange((LONG*)&m_AccessThreadID,currThread);

		// read-write or write-read or write-write이면 오용이다!
		if (oldThread != ::GetCurrentThreadId())
		{
			if((oldMode == eAccessMode_Read && eMode == eAccessMode_Write) ||
				(oldMode == eAccessMode_Write && eMode == eAccessMode_Read) ||
				(oldMode == eAccessMode_Write && eMode == eAccessMode_Write) )
			{
				String showMessage;
				showMessage.Format(_PNT("Another thread %d was already accessing this so current thread %d cannot access it!\n"), oldThread, currThread);
				ShowUserMisuseError(showMessage.GetString());
			}
		}

#endif // _DEBUG

	}

	void CReaderWriterAccessChecker::ClearThreadID() const
	{
#ifdef _DEBUG
		InterlockedExchange((LONG*)&m_AccessMode,eAccessMode_None);
		InterlockedExchange((LONG*)&m_AccessThreadID,0);
#endif // _DEBUG
	}

	CReaderWriterAccessChecker::CReaderWriterAccessChecker()
	{
#ifdef _DEBUG
		m_AccessThreadID = 0;
		m_AccessMode = eAccessMode_None;
#endif // _DEBUG
	}
#endif
}
