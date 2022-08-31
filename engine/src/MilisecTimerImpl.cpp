#include "stdafx.h"
#include "../include/SysTime.h"
#include "../include/BasicTypes.h"
#include "../include/pndefine.h"
#include "GlobalTimerThread.h"
#include "MilisecTimerImpl.h"

#include "../include/BasicTypes.h"

#ifndef PROUDNET_H_LIB_SIGNATURE
#	error You must include this for using SUPPORTS_CPP11 macro!
#else
#if defined(SUPPORTS_CPP11)
#include <chrono>
#endif
#endif

#ifndef PN_USE_CHRONO
#error PN_USE_CHRONO is not defined!
#endif

#ifndef PN_USE_CHRONO
#if (_MSC_VER>=1800) || (__cplusplus > 199711L)
#define PN_USE_CHRONO 1
#else
#define PN_USE_CHRONO 0
#endif // (_MSC_VER>=1800) || (__cplusplus > 199711L)
#endif // PN_USE_CHRONO

#if (PN_USE_CHRONO == 1)
#include <chrono>
#endif


namespace Proud
{
	//--------------------------------------------------------------------------------------
	CMilisecTimerImpl::CMilisecTimerImpl()
	{
		m_bTimerStopped = true;

		m_llStopTime = 0;
		m_llLastElapsedTime = 0;
		m_llBaseTime = 0;

	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Reset()
	{
		// Get either the current time or the stop time
		int64_t qwTime;
		if (m_llStopTime != 0)
			qwTime = m_llStopTime;
		else
			qwTime = GetPreciseCurrentTimeMs();

		m_llBaseTime = qwTime;
		m_llLastElapsedTime = qwTime;
		m_llStopTime = 0;
		m_bTimerStopped = false;
	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Start()
	{
		// Get the current time
		int64_t qwTime = GetPreciseCurrentTimeMs();

		if (m_bTimerStopped)
			m_llBaseTime += qwTime - m_llStopTime;
		m_llStopTime = 0;
		m_llLastElapsedTime = qwTime;
		m_bTimerStopped = false;
	}


	//--------------------------------------------------------------------------------------
	void CMilisecTimerImpl::Stop()
	{
		if (!m_bTimerStopped)
		{
			// Get either the current time or the stop time
			int64_t qwTime;
			if (m_llStopTime != 0)
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
		if (m_llStopTime != 0)
			return m_llStopTime;
		else
			return GetPreciseCurrentTimeMs() - m_llBaseTime;
	}


	//--------------------------------------------------------------------------------------
	int64_t CMilisecTimerImpl::GetElapsedTimeMs()
	{
		// Get either the current time or the stop time
		int64_t qwTime;
		if (m_llStopTime != 0)
			qwTime = m_llStopTime;
		else
			qwTime = GetPreciseCurrentTimeMs();

		int64_t elapsedTime = qwTime - m_llLastElapsedTime;
		if (0 != elapsedTime)
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

#if (PN_USE_CHRONO==1)
	// #FAST_REACTION  C++11을 지원할 경우 이걸 쓰도록 한다. 이게 훨씬 성능이 좋다.
	// 예전에는 함수 안에 local static var였으나, 리눅스에서 DLL로 사용되는 경우 이것이 크래시를 일으키는 것도 있고,
	// gcc에서는 local static var의 JIT creation으로 인한 속도저하로 인해,
	// 전역 변수로 뺐다.
	static auto g_GetPreciseCurrentTimeMs_firstTime = chrono::high_resolution_clock::now();
#endif

	int64_t GetPreciseCurrentTimeMs()
	{
#if (PN_USE_CHRONO==1)
		return chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - g_GetPreciseCurrentTimeMs_firstTime).count();
#else

// sizeof(vodi*) != 4 요런걸로 바꾸지 말자!!!!!! 괜히 풀패키징만 실패한다.
#if !defined __LP64__ && !defined _WIN64		// 32bit에서는 int64를 값을 가져올 때 중간에 잘라먹을 수 있다. 하지만 x64는 그런거 없다.
			SpinLock lock(CGlobalTimerThread::m_measuredTimeMs_32bitOnly, true);
#endif
			return CGlobalTimerThread::m_measuredTimeMs;
#endif  // PN_USE_CHRONO

	}

	int64_t GetEpochTimeMs()
	{
#if (PN_USE_CHRONO==1)
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
		throw Exception(__FUNCTION__" requires newer compiler.");
#endif
	}
}
