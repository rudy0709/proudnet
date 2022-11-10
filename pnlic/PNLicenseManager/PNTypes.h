#pragma once



#if defined(__linux__)
#include <unistd.h>
#include "LicenseType.h"
#else
#include <windows.h>
#include "LicenseType.h"
#endif

// 주의: 여기다 authnet이나 proudnet 헤더파일 포함 금지! 이것을 include하는 곳에서 하던지!


#if defined(__linux__)
typedef pid_t HANDLE;
#else
typedef unsigned long DWORD;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#if defined(__linux__)
#define INVALID_HANDLE_VALUE ((HANDLE) -1)
#endif

#include "Defines.h"


