#if defined(_WIN32)

#include "stdafx.h"
#include "WinTime.h"

bool CWinTime::FileTimeToLocalTm(FILETIME fileTime, tm *pTm)
{
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&fileTime, &sysTime);

	CTime transTime(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	transTime.GetLocalTm((struct tm*)pTm);
	return true;
}

bool CWinTime::FileTimeToGmtTm(FILETIME fileTime, tm *pTm)
{
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&fileTime, &sysTime);

	CTime transTime(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	transTime.GetGmtTm((struct tm*)pTm);
	return true;
}

#endif // _WIN32
