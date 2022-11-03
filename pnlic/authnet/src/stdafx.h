// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#	ifdef _WIN64
#		define _WIN32_WINNT 0x0600 // Interlocked op 64는 vista부터 지원하므로. WinXP 64bit user는 쿨하게 잊어버리자.
#	else 
#		define _WIN32_WINNT 0x0600 // win vista 미만 유저는 쿨하게 잊어버리자.
#	endif
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#if defined(_MSC_VER)
#pragma warning(error:4715) // "not all path returns value"는 에러급 오류다
#endif

#if defined(__MARMALADE__)
	#include <s3eSocket.h>
#else
#endif

#if defined(_WIN32)
	#include <objbase.h>
	#include <WinSock2.h>
    #include <mswsock.h>
	#include <conio.h> // win32에만 있는 헤더파일
	//#include <comdef.h> // 서버에서만 쓰자. UE4 때문에 클라는 쓰면 안됨.
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <algorithm>
#include <assert.h>
#include <map>
#include <set>
#include <vector>
#include <string>

using namespace std;

#ifdef USE_PARALLEL_FOR
	#if (_MSC_VER>=1400)
	#include <omp.h>
	#endif
#endif

#define SAFE_DELETE(x) { if(x) { delete x; x = 0; } }
