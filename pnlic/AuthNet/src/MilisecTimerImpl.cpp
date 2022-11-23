#include "stdafx.h"
#include "MilisecTimerImpl.h"
#include "../include/SysTime.h"
#include "GlobalTimerThread.h"
#include "../include/BasicTypes.h"

namespace Proud
{
	//--------------------------------------------------------------------------------------
	CMilisecTimerImpl::CMilisecTimerImpl()
	{
		m_bTimerStopped     = true;

		m_llStopTime        = 0;
		m_llLastElapsedTime = 0;
		m_llBaseTime        = 0;

	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Reset()
	{
		// Get either the current time or the stop time
		int64_t qwTime;
		if ( m_llStopTime != 0 )
			qwTime = m_llStopTime;
		else
			qwTime = GetPreciseCurrentTimeMs();

		m_llBaseTime        = qwTime;
		m_llLastElapsedTime = qwTime;
		m_llStopTime        = 0;
		m_bTimerStopped     = false;
	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Start()
	{
		// Get the current time
		int64_t qwTime = GetPreciseCurrentTimeMs();

		if ( m_bTimerStopped )
			m_llBaseTime += qwTime - m_llStopTime;
		m_llStopTime = 0;
		m_llLastElapsedTime = qwTime;
		m_bTimerStopped = false;
	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Stop()
	{
		if ( !m_bTimerStopped )
		{
			// Get either the current time or the stop time
			int64_t qwTime;
			if ( m_llStopTime != 0 )
				qwTime = m_llStopTime;
			else
				qwTime = GetPreciseCurrentTimeMs();

			m_llStopTime = qwTime;
			m_llLastElapsedTime = qwTime;
			m_bTimerStopped = true;
		}
	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Advance()
	{
		m_llStopTime += 100;
	}


	/*double CMilisecTimerImpl::GetAbsoluteTime()
	{
	if ( !m_bUsingQPF )
	return -1.0;

	// Get either the current time or the stop time
	int64_t qwTime;
	if ( m_llStopTime != 0 )
	qwTime.QuadPart = m_llStopTime;
	else
	QueryPerformanceCounter( &qwTime );

	double fTime = qwTime.QuadPart / (double) m_llQPFTicksPerSec;

	return fTime;
	}*/


	//--------------------------------------------------------------------------------------
	int64_t CMilisecTimerImpl::GetTimeMs()
	{
		// Get either the current time or the stop time
		if ( m_llStopTime != 0 )
			return m_llStopTime;
		else
			return GetPreciseCurrentTimeMs() - m_llBaseTime;
	}


	//--------------------------------------------------------------------------------------
	int64_t CMilisecTimerImpl::GetElapsedTimeMs()
	{
		// Get either the current time or the stop time
		int64_t qwTime;
		if ( m_llStopTime != 0 )
			qwTime = m_llStopTime;
		else
			qwTime = GetPreciseCurrentTimeMs();

		int64_t elapsedTime = qwTime - m_llLastElapsedTime;
		if(0 != elapsedTime)
		{
			m_llLastElapsedTime = qwTime;
		}
		return elapsedTime;
	}


	//--------------------------------------------------------------------------------------
	bool CMilisecTimerImpl::IsStopped()
	{
		return m_bTimerStopped;
	}

	CMilisecTimer* CMilisecTimer::New()
	{
		return new CMilisecTimerImpl();
	}

	int64_t GetPreciseCurrentTimeMs()
	{
		return CGlobalTimerThread::Instance().GetCachedTime();
	}

}
