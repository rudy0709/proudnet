#pragma once

#if defined(_WIN32)

#include "PNTime.h"
#include <windows.h>
#include <atltime.h>

class CWinTime
{
public:
	CWinTime() {};
	~CWinTime() {};

	bool FileTimeToLocalTm(FILETIME fileTime, tm *pTm);
	bool FileTimeToGmtTm(FILETIME fileTime, tm *pTm);
};

#endif // _WIN32
