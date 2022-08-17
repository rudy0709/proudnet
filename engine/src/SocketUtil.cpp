/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/FastArray.h"
#include "../include/PNString.h"

#include <errno.h>
#include <new>

#if defined(__ORBIS__)

#include <net.h>
#include <libnetctl.h>

#elif defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>

#else

// 리눅스와 맥은 ifaddrs 사용
#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__MACH__)
#include <ifaddrs.h>
#endif

#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>


#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#endif 

#include "../include/ProudNetCommon.h"
#include "SocketUtil.h"
#include "ReportError.h"
#include "SendMessage.h"
#include "BufferSegment.h"
#include "FakeWinsock.h"
#include "NumUtil.h"

#define MAXIFCOUNT 30

// 마멀레이드에서는 INET6_ADDRSTRLEN가 정의되어 있지 않습니다. 현재 PN에서 INET6_ADDRSTRLEN를 쓰는 곳은 이곳뿐입니다.
// 값은 ws2ipdef.h에 정의되어 있는 값을 그대로 사용합니다.
#if !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN 65
#endif

// MSG_NOSIGNAL does not exists on OS X
#if defined(__APPLE__) || defined(__MACH__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif
#endif

// 마멀레이드에서는 INET6_ADDRSTRLEN가 정의되어 있지 않습니다. 현재 PN에서 INET6_ADDRSTRLEN를 쓰는 곳은 이곳뿐입니다.
// 값은 ws2ipdef.h에 정의되어 있는 값을 그대로 사용합니다.
#if !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN 65
#endif

namespace Proud
{
#if defined(_WIN32)

	// inet_pton, inet_ntop 등의 함수들은 xp 지원을 하지 않는다.
	// Windows XP를 위해 직접 제작한 대체 함수이다.
	int pn_inet_pton(int af, const char* src, void* dst)
	{
		struct sockaddr_storage ss;
		// ikpil.choi : 2016-11-03, memory safe function
		//ZeroMemory(&ss, sizeof(ss));
		memset_s(&ss, sizeof(ss), 0, sizeof(ss));

		int size = sizeof(ss);
#pragma warning(push)
#pragma warning(disable: 4996) // Deprecation
		if (WSAStringToAddressA((char*)src, af, nullptr, (struct sockaddr *)&ss, &size) == 0)
#pragma warning(pop)
		{
			switch (af)
			{
			case AF_INET:
				*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
				break;
			case AF_INET6:
				*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
				break;
			}

			return 1;
		}

		return 0;
	}

	const char* pn_inet_ntop(int af, const void* src, char* dst, socklen_t size)
	{
		struct sockaddr_storage ss;
		unsigned long s = size;

		// ikpil.choi : 2016-11-03, memory safe function
		//ZeroMemory(&ss, sizeof(ss));
		memset_s(&ss, sizeof(ss), 0, sizeof(ss));


		ss.ss_family = static_cast<ADDRESS_FAMILY>(af);

		switch (af)
		{
		case AF_INET:
			((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
			break;
		case AF_INET6:
			((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
			break;
		default:
			return nullptr;
		}

#pragma warning(push)
#pragma warning(disable: 4996) // Deprecation
		return (WSAAddressToStringA((struct sockaddr *)&ss, sizeof(ss), nullptr, dst, &s) == 0) ? dst : nullptr;
#pragma warning(pop)

	}
#endif

	/* 성공시 0. else, errno.
	이 함수는 SocketErrorCode_AddressNotAvailable를 리턴하는 경우도 있다. 자세한건 코드 참고.*/
	int BindSocket(int af, SOCKET hSocket, const AddrPort& addrPort)
	{
		int r = 0;

		ExtendSockAddr sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));

		ErrorInfo err;

		// 현재 프라우드넷은 무조건 IPv6 with dual-stack 이지만 그렇다고 Native V4 address 로 bind 는 안됨
		// V6 소켓은 V4 주소로 바인딩 하더라도 무조건 sockaddr_in6 로 바인딩 해야 됨
		switch (af)
		{
		case AF_INET:
		{
			if (!addrPort.ToNativeV4(sockAddr, err))
			{
				// socket은 ipv4인데 bind할 주소는 ipv6이네? 어라?
				return SocketErrorCode_AddressNotAvailable;
			}
			// 참고: IPv4 socket으로 만들어진 경우에는 u.v4를 사용할 것. sockAddr 구조체 자체가 아니라.
			// ex) sizeof(sockaddr.u.v4) // 이런 식으로 사용할 것
			r = ::bind(hSocket, (const struct sockaddr *)&sockAddr.u.v4, sizeof(sockAddr.u.v4));
		}
		break;

		case AF_INET6:
		{
			addrPort.ToNativeV6(sockAddr);
			r = ::bind(hSocket, (const struct sockaddr *)&sockAddr.u.v6, sizeof(sockAddr));
		}
		break;

		default:
			throw Exception(_PNT("Socket is not inet protocol!"));
			break;
		}



		if (r == 0)
			return 0;
		else
		{
#ifdef _WIN32
			return WSAGetLastError();
#else
			return errno;
#endif
		}
	}

	SocketErrorCode Socket_Listen(SOCKET hSocket)
	{
#ifdef _WIN32
		const int backLog = SOMAXCONN;
#else
		const int backLog = 64; // linux에서는 최대 갯수가 제한
#endif
		// backlog는 너무 작으면 동시다발로 접속이 들어오는 클라이언트를 처리 못한다. 가령 스트레스테스트 클라이언트가 메롱한다.
		static_assert(backLog >= 64, "wrong listen backlog."); // windows에서 잘못된 winsock header include를 하는걸 방지하기 위해.
		
		int r = ::listen(hSocket, backLog);
		if (r == 0)
			return SocketErrorCode_Ok;
		else
			return (SocketErrorCode)WSAGetLastError();
	}

	// 주의: 이 함수는 SocketErrorCode_AddressNotAvailable를 리턴하는 경우도 있다.
	SocketErrorCode Socket_Connect(int af, SOCKET hSocket, const AddrPort& hostAddr)
	{
		int r = 0;
		ExtendSockAddr sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		ErrorInfo err;

		switch (af)
		{
		case AF_INET:
			if (!hostAddr.ToNativeV4(sockAddr, err))
			{
				// socket은 ipv4인데 주소는 ipv6이네? 어라?
				return SocketErrorCode_AddressNotAvailable;
			}
			r = connect(hSocket, (const struct sockaddr *)&sockAddr, sizeof(sockAddr.u.v4));
			break;

		case AF_INET6:
			hostAddr.ToNativeV6(sockAddr);
			r = connect(hSocket, (const struct sockaddr *)&sockAddr, sizeof(sockAddr));
			break;
		}

		if (r == 0)
			return SocketErrorCode_Ok;
		else
			return (SocketErrorCode)WSAGetLastError();
	}


#if defined __ORBIS__

	// PS4에서는 DNS lookup을 위한 리소스를 할당을 해두어야 한다. 이를 미리 해두고 지속적으로 재사용하자.
	CriticalSection g_PS4_DNSLookup_critSec;
	int g_PS4_NetLibraryMemoryID = -1;
	SceNetId g_PS4_ResolverID = -1;

	/* 지우지 말것. 나중에 언젠가 쓰이겠지.
	ret = sceNetResolverDestroy(g_PS4_ResolverID);
	if (ret < 0)
	{
	printf("sceNetResolverDestroy() failed (0x%x errno=%d)\n",
	ret, sce_net_errno);
	goto failed;
	}
	ret = sceNetPoolDestroy(g_PS4_NetLibraryMemoryID);
	if (ret < 0)
	{
	printf("sceNetPoolDestroy() failed (0x%x errno=%d)\n",
	ret, sce_net_errno);
	goto failed;
	}

	*/

	uint32_t DnsForwardLookup_IPv4_PS4(String name)
	{
		// 아래 리소스 할당 작업 때문에 이렇게 글로벌하게 잠금한다.
		CriticalSectionLock lock(g_PS4_DNSLookup_critSec, true);

		SceNetInAddr addr;
		int ret;

		// 첫 1회에 한해 리소스를 할당한다.
		// DNS lookup 전에 이게 필요함.

		if (g_PS4_NetLibraryMemoryID == -1)
		{
			ret = sceNetPoolCreate(__FUNCTION__, 4 * 1024, 0);
			if (ret < 0)
			{
				// NTTNTRACE
				// ps4 toolKit-console 혹은 VS output-console 어느쪽을 선호할지.
				printf("sceNetPoolCreate() failed (0x%x errno=%d)\n",
					ret, sce_net_errno);
				goto failed;
			}

			g_PS4_NetLibraryMemoryID = ret;
		}

		if (g_PS4_ResolverID == -1)
		{
			ret = sceNetResolverCreate("resolver", g_PS4_NetLibraryMemoryID, 0);
			if (ret < 0)
			{
				printf("sceNetResolverCreate() failed (0x%x errno=%d)\n",
					ret, sce_net_errno);
				goto failed;
			}
			g_PS4_ResolverID = ret;
		}

		// 본격 찾기.
		// 함수 이름과 달리, 블러킹 작동. 즉 이름 찾기가 완료될 때까지 리턴 안한다.
		ret = sceNetResolverStartNtoa(g_PS4_ResolverID, StringT2A(name).GetString(), &addr, 0, 0, 0);
		if (ret < 0)
		{
			printf("sceNetResolverStartNtoa() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}

		return addr.s_addr;

	failed:
		return 0;
	}
#endif 

	/** host 이름, 가령 www.xxx.com을 입력하면 거기에 대응하는 IP address를 리턴한다. */
	uint32_t DnsForwardLookup_IPv4(String name)
	{
#if defined __ORBIS__
		return DnsForwardLookup_IPv4_PS4(name);
#else // PS4를 제외한 모두
		// Get host adresses
		struct hostent * pHost;

#if !defined(__linux__)
#pragma warning(push)
#pragma warning(disable: 4996) // Deprecation
		pHost = gethostbyname(StringT2A(name).GetString());
#pragma warning(pop)
#else
		// 리눅스에선 gethostbyname 이 thread unsafe 하다.
		// 따라서 아래와 같은 방식으로 변경
		ByteArray tmp;
		int herr, hres;
		struct hostent hostbuf;

		tmp.resize(1024);
		while ((hres = gethostbyname_r(StringT2A(name).GetString(), &hostbuf,
			(char*)tmp.GetData(), tmp.GetCount(), &pHost, &herr)) == ERANGE)
		{
			tmp.resize(tmp.GetCount() * 2);
		}
#endif
		// 네트워크를 사용할 수 없을 경우, gethostbyname에서 NULL을 반환합니다.
		if (pHost == NULL)
		{
			return 0;
		}

		// 기본 코드에선 첫번째 address 만 가져 오도록 되어 있으므로
		// 가독성을 위하여 아래와 같이 수정
		const char* const h_address(pHost->h_addr_list[0]);
		if (h_address == NULL)
		{
			return 0;
		}
		if (pHost->h_length != 4)
		{
			return 0;
		}

		union
		{
			struct
			{
				uint8_t net;
				uint8_t host;
				uint8_t lh;
				uint8_t impno;
			};
			uint32_t s_address;
		} address_in;

		address_in.net = h_address[0];
		address_in.host = h_address[1];
		address_in.lh = h_address[2];
		address_in.impno = h_address[3];

		return address_in.s_address;
#endif // PS를 제외한 모두
	}

	SocketErrorCode ResolveAddress(const String& remoteHost, uint16_t remotePort, const String& /*publicDomainName1*/, const String& /*publicDomainName2*/, AddrPort &outAddrPort, String& outErrorText)
	{
		/*
		이 함수는 블럭킹 함수이다. 내부에서 getaddrinfo 를 호출하기 때문이다.
		"블러킹 걸리는 DNS lookup 부분 해결하기.pptx" 참고
		*/

		AddrInfo addrInfo;

		Tstringstream sstream;

		// 서버의 IP 주소를 얻는다.
		SocketErrorCode e = DnsForwardLookupAndGetPrimaryAddress(remoteHost.GetString(), remotePort, addrInfo);
		if (e != SocketErrorCode_Ok)
		{
			sstream << _PNT("DNS lookup failure. error code=") << e;
			outErrorText = sstream.str().c_str();
			return e;
		}

		outAddrPort.FromNative(addrInfo.m_sockAddr);

		// ikpil.choi 2017-04-10 : synthesize 기능 제거, ios의 경우 FQDN 으로 강제로 규칙 변경(대표님 말씀). 혹시 몰라 아래 코드 주석으로 남김
		return SocketErrorCode_Ok;

		//if (false == IsIPv4Literal(remoteHost))
		//{
		//	return SocketErrorCode_Ok;
		//}

		///*
		//ikpil.choi 2017-04-06 : ipv4 literal 이므로, synthesize 를 수행해야 하는데,
		//publicDomainName1, publicDomainName2 하나라도 없는 경우, synthesize를 보장할 수 없다.
		//*/
		//if (true == publicDomainName1.IsEmpty() || true == publicDomainName2.IsEmpty())
		//{
		//	sstream << _PNT("Given address is not FQDN. You have not given publicDomainName1 and 2.\n");
		//	sstream << _PNT("We recommend using FQDN as server address string.\n");
		//	sstream << _PNT("If you can't fill out publicDomainName1 and 2 for workaround, but it is not recommended for full coverage.\n");

		//	outErrorText = sstream.str().c_str();

		//	// 보장할 수 없는 것이지 실패는 아니다.
		//	return SocketErrorCode_Ok;
		//}

		///*
		// 사용자가 입력한 2개의 IPv4 only인 FQDN로부터 IPv4 주소를 얻는다.
		// 만약 로컬 기기가 IPv6 only에 있다면 즉 NAT64 뒤에 있다면
		// 서로 다른 두 FQDN은 공통분모 즉 pref64가 붙어있는 주소를 줄 것이다.
		// 여기서 pref64를 추출한다.

		// 사용자가 입력한 서버 주소 문자열이 ipv4 literal (11.22.33.44) 형식이면,
		// NAT64 뒤에 있는 클라 기기인 경우, 서버의 주소를 ::ffff:11.22.33.44로 얻는다.
		// 그러나 이 주소는 NAT64에 있는 클라 기기에서는 사용할 수 없는 주소다.
		// 필요한 주소는 수동으로 얻어내는 synthesize 된 주소이어야 한다. 
		//*/

		//// IPv4 to IPv6 synthesize를 시작.
		//uint32_t targetV4Addr = 0;
		//if (outAddrPort.GetIPv4Address(&targetV4Addr) == false)
		//{
		//	sstream << _PNT("Unexpected at DNS lookup. - targetV4Addr : ") << targetV4Addr;
		//	outErrorText = sstream.str().c_str();
		//	return SocketErrorCode_Error;
		//}

		//// Step 1. NetConnectionParam 으로 받았던 publicDomainName 값들을 DnsLookup 을 함
		//AddrInfo publicDomain1AddrInfo, publicDomain2AddrInfo;
		//SocketErrorCode e1 = DnsForwardLookupAndGetPrimaryAddress(publicDomainName1.GetString(), 80, publicDomain1AddrInfo);
		//SocketErrorCode e2 = DnsForwardLookupAndGetPrimaryAddress(publicDomainName2.GetString(), 80, publicDomain2AddrInfo);

		//if (e1 != SocketErrorCode_Ok || e2 != SocketErrorCode_Ok)
		//{
		//	// DNS lookup이 실패할 경우, 에러 처리를 하지 않고, 단지 synthesize만 안할 뿐이다. 폐쇄망의 경우 때문이다.
		//	sstream << _PNT("IP synthesize failed. e1=") << e1 << _PNT(", e2=") << e2;
		//	outErrorText = sstream.str().c_str();
		//	return SocketErrorCode_Ok;
		//}

		//AddrPort publicDomain1AddrPort, publicDomain2AddrPort;
		//publicDomain1AddrPort.FromNative(publicDomain1AddrInfo.m_sockAddr);
		//publicDomain2AddrPort.FromNative(publicDomain2AddrInfo.m_sockAddr);


		//// NAT64 뒤에 있는데 사용자는 IPv4 문자열을 사용하고 있으므로, 경고사항을 알려 준다.
		//// 밑의 코드들이 공통으로 사용하는 문구이므로, 여기서 미리 문구를 만들어 둔다.
		//sstream << _PNT("You are using an IPv4 literal(e.g. 11.22.33.44) as server address.\n");
		//sstream << _PNT("Though you also give Public Domain name for working around this,\n");
		//sstream << _PNT("it is recommended to depend on NAT64 and DNS64. \n");
		//sstream << _PNT("In short, you should provide FQDN host name to your server (e.g. host1.myservice.com) \n");
		//sstream << _PNT("or give an IPv6 address to your server.\n");


		//// Step 2. DnsForwardLookup 을 통하여 받은 주소 정보 값을 RFC 6052 에서 정의하고 있는 Well-Known Prefix 값과 똑같은지 비교
		//uint8_t wellknownPrefix[PN_IPV6_ADDR_LENGTH] = { 0x00, 0x64, 0xFF, 0x9B, 0x00, };

		//if (memcmp(publicDomain1AddrPort.m_addr.v6Byte, wellknownPrefix, 4) == 0 &&
		//	memcmp(publicDomain2AddrPort.m_addr.v6Byte, wellknownPrefix, 4) == 0)
		//{
		//	// 만약 여기로 왔다면 로컬 기기는 NAT64 뒤에 있는 환경이고 그 NAT64 는 Well-Known Prefix 인 64:FF9B 을 사용하는 NAT64 이다.
		//	outAddrPort.Synthesize(wellknownPrefix, sizeof(wellknownPrefix), targetV4Addr);
		//	outErrorText = sstream.str().c_str();
		//	return SocketErrorCode_Ok;
		//}

		///*
		//Step 2-1. RFC 6052 에서 정의하고 있는 Network-Specific Prefix 비교를 해본다.

		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|PL| 0-------------32--40--48--56--64--72--80--88--96--104---------|
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|32|     prefix    |v4(32)         | u | suffix                    |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|40|     prefix        |v4(24)     | u |(8)| suffix                |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|48|     prefix            |v4(16) | u | (16)  | suffix            |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|56|     prefix                |(8)| u |  v4(24)   | suffix        |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|64|     prefix                    | u |   v4(32)      | suffix    |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
		//|96|     prefix                                    |    v4(32)     |
		//+--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

		//현재는 96비트를 prefix 로 쓰는 상황만 검증 해본다.

		//출처: https://tools.ietf.org/html/rfc6052
		//*/
		//if (false == publicDomain1AddrPort.IsIPv4MappedIPv6Addr() && false == publicDomain2AddrPort.IsIPv4MappedIPv6Addr())
		//{
		//	// 앞단 12바이트가 동일한지 체크한다. 
		//	// TODO: 아직은 위 주석의 96bit prefix만 지원한다. 나머지 prefix type도 지원하게 하려면 여기를 손대야 할거다.
		//	if (memcmp(publicDomain1AddrPort.m_addr.v6Byte, publicDomain2AddrPort.m_addr.v6Byte, 12) == 0)
		//	{
		//		outAddrPort.Synthesize(publicDomain1AddrPort.m_addr.v6Byte, 12, targetV4Addr);
		//		outErrorText = sstream.str().c_str();
		//		return SocketErrorCode_Ok;
		//	}
		//}

		//return SocketErrorCode_Ok;
	}

	/* 
	hostname:port가 가리키는 주소 중 대표 주소를 얻는다.
	우선순위는 getaddrinfo 로 얻은 것들 중 
	1. ipv4 or ipv6 중 제일 처음 걸리는 것
	2. 1번에서 못얻었다면, 제일 처음 것
	성공하면 Ok를, 실패하면 socket error code가 리턴된다. 
	*/
	SocketErrorCode DnsForwardLookupAndGetPrimaryAddress(const PNTCHAR* hostName, uint16_t port, AddrInfo& out)
	{
		CFastArray<AddrInfo> addrs;
		SocketErrorCode ret = DnsForwardLookup(hostName, port, addrs);
		if (ret != SocketErrorCode_Ok)
		{
			return ret;
		}

		if (addrs.GetCount() <= 0)
		{
			return SocketErrorCode_Error;
		}

		// 순수 IPv6 주소[1]를 우선 선택한다. 없으면 배열의 맨 앞의것을 선택한다.
		// [1]: NAT64 주소이나 실제 IPv6 주소이다.
		// ::ffff.xx.xx.xx.xx 즉 IPv4 mapped IPv6 address가 아닌 주소다.
		// NAT64 뒤에 있고, IPv4 and 6가 모두 물려있는 클라가, IPv4 and 6가 모두 있는 서버에 접속할 때
		// IPv4 주소를 얻을 경우 운 없으면 IPv4 mapped IPv6 address를 얻으면 곤란하다.
		// (위의 내용은 퇴역)

		// 기존 해당 함수의 behavior 에서는 non-loopback ipv6 주소를 선택 하도록 되어 있었다.
		// 하지만 인도네시아 일부 통신사에서 IPv6 주소를 가져왔음에도 불구하고 해당 주소로 연결이 되지 않는 이슈가 발생
		// 원인은 해당 IPv6 주소가 RFC에서도 정의되어 있지 않은 이상한 주소이여서 그렇다.
		// 따라서 현재 네트워크 인프라 상황에선 IPv4가 아직 많이 쓰이니 우선적으로 IPv4 주소를 선택하도록 변경 하였다.
		// 애플 IPv4 Reject에 걸릴 일도 없다. (왜냐하면 애플에서는 서버로의 접속 자체를 IPv4 literal 로 쓰지 말자라는 것이기 때문)
		for (int32_t i = 0; i < addrs.GetCount(); i++)
		{
			AddrInfo &rAddrInfo = addrs[i];

			AddrPort addrPort;
			addrPort.FromNative(rAddrInfo.m_sockAddr);

			if (false == addrPort.IsUnicastEndpoint())
				continue;

			// ikpil.choi 2017-04-06 : IPv4 이거나 IPv6 라면, i ~ count 까지 순환하여 제일 처음 걸리는 것을 반환한다.
			if (AF_INET == rAddrInfo.m_sockAddr.u.s.sa_family || AF_INET6 == rAddrInfo.m_sockAddr.u.s.sa_family)
			{
				out = rAddrInfo;
				return SocketErrorCode_Ok;
			}
		}

		// ikpil.choi 2017-04-06 : ipv4 or ipv6가 없는 상태이므로, 오류인것 같은데, 코드 리뷰때 한번 봐야 한다.
		out = addrs[0];

		return SocketErrorCode_Ok;
	}

	// hostName:port가 가리키는 FQDN(ex: a.b.com) 혹은 IP literal(ex: 11.22.33.44 or xx:xx:xx::xx:xx) 형식의 문자열로부터
	// IP v4 or v6 주소를 얻는다.
	// 포트값도 인자로 받는 이유는, getaddrinfo가 그것까지 요구하니까.
	Proud::SocketErrorCode DnsForwardLookup(const PNTCHAR* hostName, uint16_t port, CFastArray<AddrInfo>& outParam)
	{
#if defined __ORBIS__
		// PS4는 IPv4밖에 지원 안한다. 따라서 IPv4 API를 쓴다.
		uint32_t addr = DnsForwardLookup_IPv4(hostName);

		if (addr == 0)
			return (SocketErrorCode)SocketErrorCode_Error;

		outParam.Clear();

		AddrInfo info;

		// 구조체 짜맞추기
		info.m_sockType = SOCK_STREAM;
		info.m_protocol = IPPROTO_TCP;

		memset(&info.m_sockAddr.u, 0, sizeof(info.m_sockAddr.u));

		info.m_sockAddr.u.v4.sin_family = AF_INET;
		info.m_sockAddr.u.v4.sin_addr.s_addr = addr;
		info.m_sockAddr.u.v4.sin_port = htons(port); // 포트값은 이렇게 해야

		// PS4는 IP 주소가 하나 뿐이다.
		outParam.Add(info);

		return SocketErrorCode_Ok;
#else
		SocketInitializer::GetSharedPtr();// WSAStartup 을 유도

		outParam.Clear();

		int32_t ret = 0;

		addrinfo* result = NULL;
		addrinfo hints;
		addrinfo* ptr = NULL;
		memset(&hints, 0, sizeof(addrinfo));

		// ipv4, v6 주소 모두 얻겠다는 심산.
		// NOTE: 이걸 UNSPEC이 아닌 ipv6를 넣으면, 출력되는 포트값이 0이 나와버린다.
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		stringstream sstream;
		sstream << port;

		// win32에서 WSAHOST_NOT_FOUND는 여타 OS에서EAI_NONAME이다.
		uint64_t t0 = GetPreciseCurrentTimeMs();
		int trialCount = 0;
		while (true)
		{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // Deprecation
#endif
			ret = getaddrinfo(StringT2A(hostName).GetString(), sstream.str().c_str(), &hints, &result);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

			switch (ret)
			{
			case EAI_NONAME: // The name does not resolve for the supplied parameters or the pNodeName and pServiceName parameters were not provided.
				// dns cache 목록에 hostName이 없는 경우에 EAI_NONAME 발생할 수 있습니다.
				// EAI_NONAME이 반환된 경우에는 이미 getaddrinfo이 한번 수행이 됨으로 인해서 dns cache 목록이 갱신이 되는 것으로 확인하였습니다.
				// 그렇기 때문에, 이럴 경우에는 getaddrinfo를 한번 더 수행하면 수초안에 바로 주소 정보를 가져 올 수 있습니다.
				// 그래도 최소 3회는 재시도를 하자. 한번도 재시도를 안하면 곤란하다.
				if (GetPreciseCurrentTimeMs() - t0 > 500 && trialCount > 3)
					return (SocketErrorCode)WSAGetLastError();
				trialCount++;
				Sleep(10);
				break;
			case 0: // 성공
				goto L1;
			default: // 실패
				return (SocketErrorCode)WSAGetLastError();
			}
		}
	L1:

		AddrInfo info;
		// 얻은 주소는 하나 이상일 수 있다. 루프를 돌며 채우자.
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_addrlen <= sizeof(sockaddr_in6))
			{
				info.m_protocol = ptr->ai_protocol;
				info.m_sockType = ptr->ai_socktype;

				if (ptr->ai_addrlen <= sizeof(info.m_sockAddr))
				{
					// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
					//memcpy(&info.m_sockAddr, ptr->ai_addr, ptr->ai_addrlen);
					memcpy_s(&info.m_sockAddr, sizeof(info.m_sockAddr), ptr->ai_addr, ptr->ai_addrlen);

					outParam.Add(info);
				}
			}
		}

		freeaddrinfo(result);

		return SocketErrorCode_Ok;
#endif
	}

	// String 형태의 주소를 IPv4 literal 주소(예: "11.22.33.44")인지 판별한다.
	// IPv4 literal이면 true를 리턴.
	bool IsIPv4Literal(String str)
	{
		// ascii 로 변환하는 이유는 c runtime function 인 iswdigit 의 UTF-16, UTF-32 지원 여부를 모르니
		// 안전하게 ascii 표로 숫자를 체킹하는 isdigit 를 사용하기 위함이다.
		StringA asciiStr = StringT2A(str.GetString());
		int pos = 0;
		while (true)
		{
			StringA value = asciiStr.Tokenize(".", pos);
			if (value.IsEmpty())
			{
				break;
			}

			const char* strPointer = value.GetString();
			for (int i = 0; i < value.GetLength(); i++)
			{
				if (AnsiStrTraits::IsDigit(strPointer[i]) == false)
				{
					return false;
				}
			}
		}

		return true;
	}

	// IP literal을 binary addr로 바꾼다.
		// 주의: addr에는 DN 말고 IP literal만 넣을 것! DNS lookup 못 하니까.
	uint32_t InetAddrV4(const char* addr)
	{
		struct in_addr sa;
		sa.s_addr = INADDR_NONE;
		/* 값을 초기화 해 주지 않으면 inet_pton 실패시 쓰레기 값이 그대로 반환 된다.
		*/

		pn_inet_pton(AF_INET, addr, &sa);
		return sa.s_addr;
	}

	// IP literal을 binary addr로 바꾼다.
		// 주의: addr에는 DN 말고 IP literal만 넣을 것! DNS lookup 못 하니까.
	in6_addr InetAddrV6(const char* addr)
	{
		struct in6_addr sa;
		memset(&sa, 0, sizeof(in6_addr));

		/* 값을 초기화 해 주지 않으면 inet_pton 실패시 쓰레기 값이 그대로 반환 된다.
		*/

		pn_inet_pton(AF_INET6, addr, &sa);
		return sa;
	}

	Proud::String InetNtopV4(const in_addr& sa)
	{
		char cAddr[INET6_ADDRSTRLEN * WCHAR_LENGTH]; // 혹시 모르므로 2배 더. 괜히 메모리 그으면 곤란하니.

		pn_inet_ntop(AF_INET,
			(void*)&sa,
			cAddr,
			INET6_ADDRSTRLEN * WCHAR_LENGTH);
		String ret(StringA2T(cAddr));

		return ret;
	}

	Proud::String InetNtopV6(const in6_addr& sa)
	{
		char cAddr[INET6_ADDRSTRLEN * WCHAR_LENGTH * 2]; // 혹시 모르므로 2배 더. 괜히 메모리 그으면 곤란하니.

		pn_inet_ntop(AF_INET6,
			(void*)&sa,
			cAddr,
			INET6_ADDRSTRLEN * WCHAR_LENGTH);
		String ret(StringA2T(cAddr));

		return ret;
	}

	// local address들을 얻어서 리턴한다.
	// cache된 값이 아니므로 소요 시간이 길다.
	void GetLocalIPAddresses(CFastArray<String>& output)
	{
		output.Clear();

#if defined __ORBIS__
		SceNetCtlInfo info;
		const int ret = sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_ADDRESS, &info);
		if (ret < 0)
		{
			printf("sceNetPoolCreate() failed (0x%x errno=%d)\n", ret, sce_net_errno);
			assert(0);
			return;
		}

		output.Add(StringA2T(info.ip_address));

#elif (defined(__linux__) && !defined(__ANDROID__)) || defined(__MACH__)
		struct ifaddrs * ifAddrStruct = NULL;
		struct ifaddrs * ifa = NULL;
		void * tmpAddrPtr = NULL;

		getifaddrs(&ifAddrStruct);

		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
		{
			if (!ifa->ifa_addr)
			{
				continue;
			}
			if (ifa->ifa_addr->sa_family == AF_INET)
			{ // check it is IP4
				// is a valid IP4 Address
				tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

				output.Add(StringA2T(addressBuffer));
			}
			else if (ifa->ifa_addr->sa_family == AF_INET6)
			{	// check it is IP6
				// is a valid IP6 Address
				tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
				char addressBuffer[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			}
		}
		if (ifAddrStruct != NULL)
			freeifaddrs(ifAddrStruct);

#elif defined(__ANDROID__)
		ifconf ifcfg;
		int fd = socket(AF_INET, SOCK_DGRAM, 0);

		ifcfg.ifc_len = sizeof(struct ifreq) * MAXIFCOUNT;
		ifcfg.ifc_buf = (char*)malloc(ifcfg.ifc_len);

		int ret = ioctl(fd, SIOCGIFCONF, (char *)&ifcfg);
		close(fd);

		if (ret >= 0)
		{
			struct ifreq *ifr = ifcfg.ifc_req;
			int ifcount = ifcfg.ifc_len / sizeof(struct ifreq);

			for (int loop = 0; loop < ifcount; loop++)
			{
				output.Add(StringA2T(inet_ntoa(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr)));
				ifr++;
			}
		}

		free(ifcfg.ifc_buf);

#else
		char szHostName[128];
		if (gethostname(szHostName, 128) == 0)
		{
			// Get host adresses
			struct hostent * pHost;
			int i;

#pragma warning(push)
#pragma warning(disable: 4996) // Deprecation
			pHost = gethostbyname(szHostName);
#pragma warning(pop)

			for (i = 0; pHost != NULL && pHost->h_addr_list[i] != NULL; i++)
			{
				String str;
				int j;

				for (j = 0; j < pHost->h_length; j++)
				{
					String addr;

					if (j > 0)
						str += _PNT(".");

					addr.Format(_PNT("%u"), (unsigned int)((unsigned char*)pHost->h_addr_list[i])[j]);
					str += addr;
				}
				// str now contains one local IP address - do whatever you want to do with it (probably add it to a list)
				output.Add(str);
			}
		}
#endif
	}

	Proud::AddrPort Socket_GetSockName(SOCKET hSocket)
	{
		ExtendSockAddr addr;
#ifndef __ORBIS__
		int addrLen = sizeof(addr);
#else
		int addrLen = sizeof(addr.u.v4);
#endif
		if (::getsockname(hSocket, (sockaddr*)&addr, (socklen_t *)&addrLen) != 0)
		{
			return AddrPort::Unassigned;
		}
		else
		{
			AddrPort ret;

			if (addr.u.s.sa_family == AF_INET)
			{
				ret.FromNativeV4(addr.u.v4);
			}
			else if (addr.u.s.sa_family == AF_INET6)
			{
				ret.FromNativeV6(addr.u.v6);
			}

			return ret;
		}
	}

	void AssertCloseSocketWillReturnImmediately(SOCKET s)
	{
		struct linger lin;
		size_t lingerLen = sizeof(lin);

		if (getsockopt(
			s,
			SOL_SOCKET,
			SO_LINGER,
#if !defined(_WIN32)
			(void*)&lin,
			(socklen_t*)&lingerLen)
#else
			(char*)&lin,
			(int*)&lingerLen)
#endif
			== 0)
		{
			if (lin.l_onoff && lin.l_linger)
			{
				CErrorReporter_Indeed::Report(_PNT("FATAL: Socket which has behavior of some waits in closesocket() has been detected!"));
			}
		}
	}

#ifndef _WIN32
	void FragmentedBufferToMsgHdr(CFragmentedBuffer& sendBuffer, CLowFragMemArray<IOV_MAX, iovec> &sendBuffer2, msghdr *hdr)
	{
		int count = sendBuffer.GetSegmentCount();

		sendBuffer2.SetCount(count);

		memset(hdr, 0, sizeof(msghdr));
		hdr->msg_iovlen = count;
		hdr->msg_iov = sendBuffer2.GetData();

		for (int i = 0; i < count; i++) {
			hdr->msg_iov[i].iov_base = sendBuffer.Array()[i].buf;
			hdr->msg_iov[i].iov_len = sendBuffer.Array()[i].len;
		}
	}
#endif

	// WSASend 혹은 sendmsg를 호출한다.
	// Caller에서 Intr 에러를 처리한다.
	int send_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags)
	{
#ifdef _WIN32
		{
			int completedLength = 0;
			int r = ::WSASend(socket, sendBuffer.GetSegmentPtr(), sendBuffer.GetSegmentCount(), (LPDWORD)&completedLength, flags, NULL, NULL);
			if (r < 0)
				completedLength = r;
			return r;
		}
#else

		{
			// 비 윈도에서는 sendmsg가 segment data를 보낼 수 있게 해준다.
			CLowFragMemArray<IOV_MAX, iovec> sendBuffer2;
			msghdr hdr;
			FragmentedBufferToMsgHdr(sendBuffer, sendBuffer2, &hdr);

			// Requests not to send SIGPIPE on errors on stream oriented sockets when the other end breaks the connection. The EPIPE error is still returned.
			// http://linux.die.net/man/2/sendmsg
			flags |= MSG_NOSIGNAL;
			return ::sendmsg(socket, &hdr, flags);
		}



		/*RMI 3003 버그 잡으려면,
		위 구문을 이렇게 교체해서 테스트를 해야.

				CMessage tempMsg;
				tempMsg.UseInternalBuffer();


				// 혹시 이건 버그의 원인일라나? 펄백 해보자 3003!!!
				for(int i=0;i<sendBuffer.GetSegmentCount();i++)
				{
					WSABUF x = sendBuffer.m_buffer[i];
					tempMsg.Write((uint8_t*)x.buf, x.len);
				}

				return ::send(socket, tempMsg.GetData(), tempMsg.GetLength(), flags);


		*/







#endif
	}

	// WSASend의 segment buffer를 송신하는 것을 흉내낸다.
	// Caller에서 Intr 에러를 처리한다.
	int sendto_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen)
	{
#ifdef _WIN32
		{
			int completedLength = 0;
			int r = ::WSASendTo(socket, sendBuffer.GetSegmentPtr(), sendBuffer.GetSegmentCount(), (LPDWORD)&completedLength, flags, sendTo, sendToLen, NULL, NULL);
			if (r < 0)
				completedLength = r;
			return r;
		}
#else
		{
			// 비 윈도에서는 sendmsg가 segment data를 보낼 수 있게 해준다.
			CLowFragMemArray<IOV_MAX, iovec> sendBuffer2;
			msghdr hdr;
			FragmentedBufferToMsgHdr(sendBuffer, sendBuffer2, &hdr);
			hdr.msg_name = (sockaddr*)sendTo;
			hdr.msg_namelen = sendToLen;

			// Requests not to send SIGPIPE on errors on stream oriented sockets when the other end breaks the connection. The EPIPE error is still returned.
			// http://linux.die.net/man/2/sendmsg
			flags |= MSG_NOSIGNAL;
			return ::sendmsg(socket, &hdr, flags);
		}
#endif
	}

	SocketErrorCode Socket_SetBlocking(SOCKET hSocket, bool isBlockingMode)
	{
#if defined(__ORBIS__)
		{
			int argp = isBlockingMode ? 0 : 1;
			if (sceNetSetsockopt(hSocket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &argp, sizeof(argp)) == 0)
				return SocketErrorCode_Ok;
			else
				return (SocketErrorCode)errno;
		}
#elif defined(_WIN32)
		{
			u_long argp = isBlockingMode ? 0 : 1;
			if (::ioctlsocket(hSocket, FIONBIO, &argp) == 0)
				return SocketErrorCode_Ok;
			else
				return (SocketErrorCode)WSAGetLastError();
		}
#else
		{
			// unix에서는 fcntl을 쓰라고 권유하고 있다. 그러나, 
			// fcntl()를 사용하면 GCC ARM에서 nonblock 임에도 불구하고 조금씩 block현상이 생김
			// 그리고 추가적으로 fcntl()을 사용시 connect가 안되는 현상이 발생 
			// fcntl() -> ioctl()로  수정함 modify by kdh
			//int flag = fcntl(m_socket, F_GETFL, 0);
			u_long argp = isBlockingMode ? 0 : 1;
			int flag = 1;
			flag = ::ioctl(hSocket, FIONBIO, &argp);
			if (flag == 0)
				return SocketErrorCode_Ok;
			else
				return (SocketErrorCode)errno;
		}

		//  		int newFlag;
		//  		if(isBlockingMode)
		//  			newFlag = flag & ~O_NONBLOCK;
		//  		else
		//  			newFlag = flag | O_NONBLOCK;
		//  
		//          int ret = fcntl(m_socket, F_SETFL, newFlag );
		//  	
		//          if(ret == -1)
		//          {
		//  			assert(0); // 있어서는 안되는 상황
		//              //PostSocketWarning(errno, _PNT("SetBlockingMode : fcntl set error"));
		//  			PostSocketWarning(errno, _PNT("SetBlockingMode : ioctl get error"));
		//          }
#endif
	}

#ifndef _WIN32
	SocketErrorCode Socket_GetBlocking(SOCKET hSocket, bool* outBlocking)
	{
		// TODO: fcntl이 아니라 위에처럼 ioctl을 써야 하지 않나? 일관성!

		int flag = fcntl(hSocket, F_GETFL, 0);
		if (flag == -1)
		{
			return (SocketErrorCode)errno;
		}

		*outBlocking = (flag & O_NONBLOCK) ? false : true;

		return SocketErrorCode_Ok;
	}
#endif



#if 0 // 이 함수는 아직 쓸일이 없다. 따라서 막았다.
	// 잘 작동하는 코드이지만 여기로 그냥 옮겨왔다. 쓰려면 마무리하고 쓰자.

	// PS4는 socketpair()가 없지만, 
	// 직접 두 socket을 만들면 된다.
	// 어차피 클라에서만 작동하는 OS이므로 아주 짧은 순간에 다른 TCP connection이
	// 그것도 localhost에 있는 것으로부터 연결 들어올 가능성은 희박하다.
	// 실패하면 throw한다. 
	void FakeCreateSocketPair(int* socketPairArray)
	{
		// TODO: 매우 짧은 순간에 두 socket을 만들어 서로 연결시키므로 빈도는 매우 작겠지만,
		// 해커가 두 연결이 맺어지기 전에 연결을 해버릴 가능성이 있을 수는 있다.
		// 게다가 PF_LOCAL에 비해서 훨씬 느리다.
		// 따라서 추후 PF_LOCAL로 바꾸고, PF_LOCAL을 못 쓰는 platform에서는
		// bind 주소로 any:0이 아니라 localhost:0으로 바꾸자.

		int listenSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (listenSocket == -1)
		{
			throw Exception("socketpair creation failure 1: errno:%d", errno);
		}

		if (BindSocketIpv4(listenSocket, 0, 0) != 0)
		{
			throw Exception("socketpair creation failure 2: errno:%d", errno);
		}

		socketPairArray[1] = socket(AF_INET, SOCK_STREAM, 0);

		if (socketPairArray[1] == -1)
		{
			throw Exception("socketpair creation failure 3: errno:%d", errno);
		}

		EventFDs_SetNonBlocking(); // 위 socket을 non-block으로 설정한다.

		if (BindSocketIpv4(m_eventFDs[1], 0, 0) != 0)
		{
			throw Exception("socketpair creation failure 4: errno:%d", errno);
		}

		SocketErrorCode err = Socket_Listen(listenSocket);

		if (err != SocketErrorCode_Ok)
		{
			throw Exception("socketpair creation failure 5: errno:%d", err);
		}

		// 한쪽이 나머지에 접속을 한다.
		AddrPort a = Socket_GetSockName(listenSocket);

		err = Socket_Connect(m_eventFDs[1], a);

		if ((err != SocketErrorCode_Ok) && err != (CFastSocket::IsWouldBlockError(err)))
		{
			throw Exception("socketpair creation failure 6: errno:%d", err);
		}

		while (1)
		{
			socketPairArray[0] = ::accept(listenSocket, 0, 0);

			if (socketPairArray[0] > 0)
			{
				::close(listenSocket);
				break;
			}

			Proud::Sleep(1);
		}

	}
#endif
}