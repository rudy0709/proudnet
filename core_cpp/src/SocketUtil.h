#pragma once

#include "../include/PNString.h"
#include "../include/AddrPort.h"

#include <errno.h>
// POSIX 지원 플랫폼에서 표준 유닉스 헤더파일 포함
#if defined(_WIN32)
#include <WS2tcpip.h>
#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__) || defined(__MACH__)
#include <unistd.h>
#include <fcntl.h> // fcntl
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#if !defined(__ORBIS__)
#include <sys/ioctl.h> // FIONBIO
#endif
#if !defined(__linux__) && !defined(__MACH__)
#include <net.h>
#endif

#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

#include <limits.h> // IOV_MAX
#ifndef IOV_MAX
#define IOV_MAX 1024 // android나 win32에서는 이게 없어서 강제로 세팅.
#endif

namespace Proud
{
#if defined(_WIN32)
	int pn_inet_pton(int af, const char* src, void* dst);
	const char* pn_inet_ntop(int af, const void* src, char* dst, socklen_t size);
#else
	#define pn_inet_pton inet_pton
	#define pn_inet_ntop inet_ntop
#endif

	// 해당 함수는 비권장 API
	uint32_t DnsForwardLookup_IPv4(String name);

	/* socket API의 sockaddr 인자로 이것이 들어간다.
	ipv4,6 모두에 대해 값을 저장할 수 있는 union 객체.

	Q: AddrPort에 있는 것과 중복 아닌가요?
	A: AddrPort에 있는 것은, 순수 ipv4 or ipv6 주소값이고, 그건 platform independent합니다. 
	sockaddr_xx 내부 구조는 platform dependent하므로 신뢰할 수 없음. */
	struct ExtendSockAddr
	{
		// 절대 이 구조체내에 변수 혹은 virtual 함수를 집어넣지 마십시오.
		// by. hyeonmin.yoon

		// 참고: IPv4 socket으로 만들어진 경우에는 u.v4를 사용할 것.sockAddr 구조체 자체가 아니라.
		// ex) sizeof(sockaddr.u.v4) // 이런 식으로 사용할 것

		union 
		{
			sockaddr s;
			sockaddr_in v4;
			sockaddr_in6 v6;
		} u;
	};

	// socket API의 addrinfo 객체로부터 가져오는 값.
	struct AddrInfo
	{
		int32_t m_sockType;
		int32_t m_protocol;

		// ipv4 or 6 address
		ExtendSockAddr m_sockAddr;
	};

	SocketErrorCode DnsForwardLookup(const PNTCHAR* hostName, uint16_t port, CFastArray<AddrInfo>& outParam);
	SocketErrorCode DnsForwardLookupAndGetPrimaryAddress(const PNTCHAR* hostName, uint16_t port, AddrInfo& out);
	SocketErrorCode ResolveAddress(const String& remoteHost, uint16_t remotePort, const String& publicDomainName1, const String& publicDomainName2, AddrPort &outAddrPort, String& outErrorText);
	

	bool IsIPv4Literal(String str);

	uint32_t InetAddrV4(const char* addr);
	in6_addr InetAddrV6(const char* addr);

	Proud::String InetNtopV4(const in_addr& sa);
	Proud::String InetNtopV6(const in6_addr& sa);

	PROUD_API void GetLocalIPAddresses(CFastArray<String>& output);

	AddrPort Socket_GetSockName(SOCKET hSocket);

	int BindSocket(int af, SOCKET hSocket, const AddrPort& addrPort);

	SocketErrorCode Socket_Listen(SOCKET hSocket);
	SocketErrorCode Socket_Connect(int af, SOCKET hSocket, const AddrPort& hostAddr);
	SocketErrorCode Socket_SetBlocking(SOCKET hSocket, bool isBlockingMode);
#ifndef _WIN32
	SocketErrorCode Socket_GetBlocking(SOCKET hSocket, bool* outBlocking);
#endif

	void AssertCloseSocketWillReturnImmediately(SOCKET s);

   
}

