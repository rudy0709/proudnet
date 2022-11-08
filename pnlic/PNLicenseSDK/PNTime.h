#pragma once

#include "stdafx.h"
#include <time.h>
#include <sys/stat.h>
#include <stdint.h>

#ifdef _MSC_VER
#define ATTRIBUTE_ALIGN(n) __declspec(align(n))
#else
#define ATTRIBUTE_ALIGN(n) __attribute__ ((aligned(n)))
#endif

// ������� ���������� struct tm ����ü ����� �ٸ��� ������
// ������ Time ����ü�� �����Ͽ� ���
ATTRIBUTE_ALIGN(4) struct SYSTEMTIME_t
{
	int sec;     /* seconds after the minute - [0,59] */
	int min;     /* minutes after the hour - [0,59] */
	int hour;    /* hours since midnight - [0,23] */
	int mday;    /* day of the month - [1,31] */
	int mon;     /* months since January - [0,11] */
	int year;    /* years since 1900 */
	int isdst;   /* daylight savings time flag */
};

time_t mkUTCTime(SYSTEMTIME_t const& utcTimeInfo);
