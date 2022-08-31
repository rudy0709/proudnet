#include "stdafx.h"
#include "../include/TimerThread.h"

namespace Proud
{
#ifdef _WIN32
	void CTimerThread::OuterThreadProc(void* ctx)
	{
		CTimerThread* owner = (CTimerThread*)ctx;
		while(!owner->m_stopThread)
		{
			owner->m_proc(owner->m_procCtx);
			owner->m_tickEvent.WaitOne();
		}
	}

	CTimerThread::CTimerThread(Thread::ThreadProc threadProc, uint32_t interval, void *ctx):m_timer(m_tickEvent.m_event,interval, 0),m_thread(OuterThreadProc,this)
	{
		m_stopThread = false;
		m_proc = threadProc;
		m_procCtx = ctx;
		m_useComModel = false;
	}


	void CTimerThread::Start()
	{
		CriticalSectionLock lock(m_startStopMethodLock,true);
		m_stopThread = false;
		m_thread.m_useComModel = m_useComModel;
		m_thread.Start();
	}

	CTimerThread::~CTimerThread()
	{
		Stop();
	}

	void CTimerThread::Stop()
	{
		CriticalSectionLock lock(m_startStopMethodLock,true);
		m_stopThread = true;
		m_thread.Join();
	}
#endif
}
