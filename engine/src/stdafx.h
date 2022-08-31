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
#pragma warning(error:4706) // if-assign을 에러로 낸다.
#pragma warning(error:4715) // "not all path returns value"는 에러급 오류다
#pragma warning(error:4150) // "deletion of pointer to incomplete type"는 에러급 오류다
#pragma warning(disable:4635) // 주석에 XML 어쩌고 경고
#endif

#if defined(_WIN32)
	#include <objbase.h>
	#include <WinSock2.h>
	#include <mswsock.h>
	#include <conio.h> // win32에만 있는 헤더파일
	//#include <comdef.h> // 서버에서만 쓰자. UE4 때문에 클라는 쓰면 안됨.
#else

#if defined(__MACH__)
// iOS 에서 일부 IPv6 기능 (RFC 문서) 을 사용하기 위해서는 아래와 같은 매크롤 사용자가 직접 입력 해야 한다.
#define __APPLE_USE_RFC_2292
#endif

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <algorithm>
#include <assert.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

// XCode gnu libstd++에서는 C++11지원하지 않기 때문에 tr1(boost)를 사용합니다.
// __GLIBCXX__ : gnu libstd++
#if defined(__MACH__) && defined(__GLIBCXX__)
#include <tr1/memory>
#else
#include <memory>
#endif

// _PN_BOOST 전처리기 선언은 ProudNet_ARM.vcxproj 프로젝트 속성에 정의되어 있습니다.
// ProudNet_ARM 프로젝트에는 boost가 사용되며, 사용되는 boost 버전의 경우 1.54버전 입니다.
// ndk stlport로 빌드할 때에도 boost를 사용합니다. (Proud/ndk/jni/Application.mk 파일에서 설정)
#if defined(_PN_BOOST)
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#endif //defined(_PN_BOOST)

#if (defined(__MACH__) && defined(__GLIBCXX__)) /*|| _MSC_VER < 1600 // VS2008 or older*/
using namespace std::tr1;
#endif

#if defined(_PN_BOOST)
using namespace boost;
#endif

using namespace std;

#if (defined(__MACH__) && defined(__GLIBCXX__)) || defined(_PN_BOOST)
#define nullptr 0
#endif

#ifdef USE_PARALLEL_FOR
	#if (_MSC_VER>=1400)
	#include <omp.h>
	#endif
#endif

#include "../include/OrbisIPv6.h"

//#include <cvmarkersobj.h>
//using namespace Concurrency::diagnostic;
