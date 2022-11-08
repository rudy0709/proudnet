#include "stdafx.h"
#include "PNTime.h"

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
/*
UTC �ð������� ������ ����ü �����͸� time_t�� ��ȯ���ִ� mktime() ��ü �Լ��Դϴ�. 
����Ÿ���� ��� mktime() �Լ��� ��ȯ�� ���� ������, UTCŸ���� ��� �̸� ��ȯ���� �Լ��� �����ϴ�. �׷��� ���� �Լ��Դϴ�.
*/
time_t mkUTCTime(SYSTEMTIME_t const& utcTimeInfo)
{
	time_t curTime;
	struct tm * timeinfo;

	time(&curTime);
	timeinfo = gmtime(&curTime);
	time_t utcMkTime = mktime(timeinfo);
	timeinfo = localtime(&curTime);
	time_t localMkTime = mktime(timeinfo);

	const int UTC_DT = int ((difftime(localMkTime, utcMkTime) / 3600));
	
	time_t utcTime;
	struct tm timeInfo = { 0 };
	timeInfo.tm_sec = utcTimeInfo.sec;
	timeInfo.tm_min = utcTimeInfo.min;
	timeInfo.tm_hour = utcTimeInfo.hour;
	timeInfo.tm_mday = utcTimeInfo.mday;
	timeInfo.tm_mon = utcTimeInfo.mon;
	timeInfo.tm_year = utcTimeInfo.year;
	timeInfo.tm_isdst = utcTimeInfo.isdst;

	utcTime = ::mktime(&timeInfo) + UTC_DT * 3600;

	return utcTime;
}