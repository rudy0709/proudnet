#include "stdafx.h"
#include "../include/PnTime.h"
#include "../include/sysutil.h"

#if defined(_WIN32)
#include <oledb.h>
#include <math.h>
#endif

namespace Proud
{
	int timespec_get_pn(::timespec *tp, int const base)
	{
#if defined(_WIN32) && 1900 > _MSC_VER
		// http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
		if (NULL == tp)
			return -1;

		// 1601 년 1 월 1 일 (UTC) 이후 100 나노초 간격의 수를 나타내는 64 비트 값을 포함합니다.
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);

		// ikpil.choi 2017-02-16 : 형변환시 문제 생길 수 있어, 안전한 변환방법으로 전환
		LARGE_INTEGER large;
		large.HighPart = ft.dwHighDateTime;
		large.LowPart = ft.dwLowDateTime;

		long long hecto = large.QuadPart - 116444736000000000LL;

		tp->tv_sec = static_cast<time_t>(hecto / 10000000LL);
		tp->tv_nsec = static_cast<long>((hecto % 10000000LL) * 100LL);

		return 0;

#elif defined(_WIN32) && 1900 <= _MSC_VER
		// ikpil.choi 2017-02-20 : MSVC 2015 이상이면 timespec_get 으로 대응
		return timespec_get(tp, base);

#else
		// ikpil.choi 2017-02-20 : 그 외는 clock_gettime 으로 대응(XCODE, linux, ndk, 플스 등등)
		// http://en.cppreference.com/w/c/chrono/timespec_get
		if (NULL == tp)
			return -1;

		return clock_gettime(CLOCK_REALTIME, tp);
#endif
	}

#if defined(_WIN32)
	// ikpil.choi 2017-02-16 : C++11 을 못쓰는 환경을 위한 cross platform time 함수 정의(N2590)
	errno_t localtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		// local time
		return localtime_s(pTm, pTime);
	}

	errno_t gmtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		// utc time
		return gmtime_s(pTm, pTime);
	}

#elif defined(__ORBIS__)
	// ikpil.choi 2017-02-17 : ORBIS 환경에선 localtime_s 가 존재하며, 윈도우랑 함수 파라미터가 서로 다르므로, 재정의
	errno_t localtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		localtime_s(pTime, pTm);
		return 0;
	}

	errno_t gmtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		gmtime_s(pTime, pTm);
		return 0;
	}

#else
	errno_t localtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		localtime_r(pTime, pTm);
		return 0;
	}

	errno_t gmtime_pn(const ::time_t* pTime, ::tm* pTm)
	{
		gmtime_r(pTime, pTm);
		return 0;
	}
#endif

	long long timespec_to_nanosecond(const ::timespec& ts)
	{
		return static_cast<long long>(ts.tv_sec) * 1000000000LL + static_cast<long long>(ts.tv_nsec);
	}

	int nanosecond_to_timespec(const long long& ns, timespec *tp)
	{
		if (NULL == tp)
			return -1;

		tp->tv_sec = static_cast<time_t>(ns / 1000000000LL); // signed

		if (ns >= 0)
		{
			tp->tv_nsec = static_cast<long>(ns % 1000000000LL);
		}
		else
		{
			tp->tv_nsec = -static_cast<long>((-ns) % 1000000000LL);
		}

		return 0;
	}

	long long timespec_to_millisecond(const ::timespec& ts)
	{
		return static_cast<long long>(ts.tv_sec) * 1000LL + static_cast<long long>(ts.tv_nsec / 1000000LL);
	}

	int millisecond_to_timespec(const long long& ms, timespec *tp)
	{
		if (NULL == tp)
			return -1;

		tp->tv_sec = static_cast<time_t>(ms / 1000LL); // signed

		if (ms >= 0)
		{
			tp->tv_nsec = static_cast<long>((ms % 1000LL) * 1000000LL);
		}
		else
		{
			tp->tv_nsec = -static_cast<long>(((-ms) % 1000LL) * 1000000LL);
		}

		return 0;
	}

	long long GetCurrentTimeNanoSecond()
	{
		timespec ts;
		timespec_get_pn(&ts, TIME_UTC);

		return timespec_to_nanosecond(ts);
	}
}
