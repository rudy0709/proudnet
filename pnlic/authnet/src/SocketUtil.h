#pragma once

#include "../include/PNString.h"

#include <errno.h>
// POSIX 지원 플랫폼에서 표준 유닉스 헤더파일 포함
#if defined(_WIN32)
#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__) || defined(__MACH__)
#include <unistd.h>
#include <fcntl.h> // fcntl
#if !defined(__ORBIS__)
#include <sys/ioctl.h> // FIONBIO
#endif
#if !defined(__linux__) && !defined(__MACH__)
#include <net.h>
#endif
#include "Epoll_Reactor.h"
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

#if defined (__MARMALADE__)
/* 2014.06.30 : seungchul.lee
* Marmalade 7.0 에는 INADDR_NONE 정의되어 있지 않아서 추가 */
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#endif

#include <limits.h> // IOV_MAX
#ifndef IOV_MAX
#define IOV_MAX 1024 // android나 win32에서는 이게 없어서 강제로 세팅.
#endif

namespace Proud
{
	uint32_t DnsForwardLookup(String name);

	uint32_t InetAddrV4(const char* addr);
	String InetNtopV4(in_addr& addr);

	void GetLocalIPAddresses(CFastArray<String>& output);
	AddrPort Socket_GetSockName(SOCKET hSocket);

	int BindSocketIpv4(SOCKET hSocket, uint32_t binaryAddress, uint32_t nSocketPort);
	
	SocketErrorCode Socket_Listen(SOCKET hSocket);
	SocketErrorCode Socket_Connect(SOCKET hSocket, const String &hostAddressName, int hostPort);
	SocketErrorCode Socket_Connect(SOCKET hSocket, const AddrPort& hostAddr);

	void AssertCloseSocketWillReturnImmediately(SOCKET s);
#if defined(_WIN32)
	bool Winsock2Startup();
#endif
}

