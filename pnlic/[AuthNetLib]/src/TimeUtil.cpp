/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#ifndef _WIN32
    #include <sys/time.h>
#endif

#include "../include/TimeUtil.h"
#include "GlobalTimerThread.h"

namespace Proud
{
#ifndef _WIN32

	// 시스템 현재 시간
	void TimeGetAbsolute(timespec* ts)
	{
// 		timeval tv;
// 		gettimeofday(&tv, NULL);
// 
// 		ts->tv_sec = tv.tv_sec;
// 		ts->tv_nsec = (tv.tv_usec % 1000) * 1000;

		CGlobalTimerThread::Instance().GetAbsoluteTimeSpec(ts);
	}

	// ts에 밀리초를 더한다.
	void TimeAddMilisec(timespec* ts, uint32_t milisec)
	{
		time_t sec = milisec / 1000;
		int nanosec = (milisec % 1000) * 1000000;
		ts->tv_sec += sec;
		ts->tv_nsec += nanosec;

		// carry (참고: http://www.geonius.com/software/source/libgpl/ts_util.c )
		if(ts->tv_nsec >= 1000000000L)
		{
			ts->tv_sec++;
			ts->tv_nsec -= 1000000000L;
		}
	}
#endif // WIN32
}
