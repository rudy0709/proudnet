#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#ifdef WIN32
#include <process.h>
#include <tchar.h>
#include <fstream>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#endif

#include "../PNLicenseSDK/PNSecure.h"
#include "PNTypes.h"

namespace Proud
{
    // success: 프로세스 실행이 성공 혹은 실패
    // hSpawnedProcess: 실행했던 프로세스
    // 사용 예: PNLic_OnSpawnComplete
	typedef void(*OnSpawnCompleteHandler)(bool success, HANDLE hSpawnedProcess);
}
