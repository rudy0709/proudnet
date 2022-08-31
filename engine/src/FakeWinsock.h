#pragma once

#if !defined(_WIN32)

#include "../include/BasicTypes.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
// #include "sys/ioctl.h"

#else

#include "iocp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"mswsock.lib")

#endif

#ifdef __MACH__
#include <sys/fcntl.h>
#include <sys/event.h>
#endif

#ifdef __ANDROID__
#include <fcntl.h>
#endif

#if !defined(_WIN32)

#include <errno.h>

#define WSAENOTCONN ENOTCONN

namespace Proud
{
	inline uint32_t WSAGetLastError() { return errno; }
}

#endif
