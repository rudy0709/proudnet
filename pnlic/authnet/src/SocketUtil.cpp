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

#elif defined(__MACH__)
#include <ifaddrs.h>

#define pn_getifaddrs getifaddrs
#define pn_freeifaddrs freeifaddrs

#else

#include <sys/uio.h> // marmalade빌드시 iovec때문에 요구됨
#include <sys/socket.h>
#include <unistd.h>
#ifdef __ANDROID__
#include "ifaddrs.h"  // 자체제작 버전 사용
#endif

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

#ifdef _MSC_VER
#pragma warning(disable:4996) // gethostbyname은 freebsd 등에서 아직 쓰인다. 언젠가는 이 경고 비활성화도 없애야 하겠지만.
#endif

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
	/* 성공시 0. else, errno */
	int BindSocketIpv4( SOCKET hSocket, uint32_t binaryAddress, uint32_t nSocketPort )
	{
		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));

		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = binaryAddress;

		sockAddr.sin_port = htons((u_short)nSocketPort);

		int r = ::bind(hSocket, (const struct sockaddr *)&sockAddr, sizeof(sockAddr));
		if(r == 0)
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
		int backLog;
#ifdef _WIN32
		backLog = SOMAXCONN;
#else
		backLog = 64; // linux에서는 최대 갯수가 제한
#endif
		int r = ::listen(hSocket, backLog);
		if (r == 0)
			return SocketErrorCode_Ok;
		else
			return (SocketErrorCode)WSAGetLastError();
	}

	SocketErrorCode Socket_Connect(SOCKET hSocket, const String &hostAddressName, int hostPort)
	{
		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		if (hostAddressName.GetLength() > 0)
			sockAddr.sin_addr.s_addr = Proud::DnsForwardLookup(hostAddressName);
		else	
		{
			// 주소를 입력 안하고 포트만 넣었다면 localhost를 넣도록 하자.
			sockAddr.sin_addr.s_addr = Proud::InetAddrV4("127.0.0.1");
		}

		// 포트
		sockAddr.sin_port = htons((u_short)hostPort);

		int r = connect(hSocket, (const struct sockaddr *)&sockAddr, sizeof(sockAddr));
		if (r == 0)
			return SocketErrorCode_Ok;
		else
			return (SocketErrorCode)WSAGetLastError();
	}

	SocketErrorCode Socket_Connect(SOCKET hSocket, const AddrPort& hostAddr)
	{
		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		if (hostAddr.m_binaryAddress != 0)
		{
			sockAddr.sin_addr.s_addr = hostAddr.m_binaryAddress;
		}
		else
		{
			// 주소를 입력 안했으면 localhost를 넣자.
			sockAddr.sin_addr.s_addr = Proud::InetAddrV4("127.0.0.1");
		}

		// 포트
		sockAddr.sin_port = htons((u_short)hostAddr.m_port);

		int r= connect(hSocket, (const struct sockaddr *)&sockAddr, sizeof(sockAddr));
		if (r == 0)
			return SocketErrorCode_Ok;
		else
			return (SocketErrorCode)WSAGetLastError();
	}

#if defined(_WIN32)    
	bool Winsock2Startup()
	{
		uint16_t wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD( 2, 2 );
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 )
		{
			// Tell the user that we couldn't find a usable
			// WinSock DLL.
			return 0;
		}
		// Confirm that the WinSock DLL supports 2.2.
		// Note that if the DLL supports versions greater
		// than 2.2 in addition to 2.2, it will still return
		// 2.2 in wVersion since that is the version we
		// requested.
		if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
		{
			// Tell the user that we couldn't find a usable
			// WinSock DLL.
			WSACleanup( );
			return 0;
		}

		return 1;
	}
#endif

	/** host 이름, 가령 www.xxx.com을 입력하면 거기에 대응하는 IP address를 리턴한다. */
	uint32_t DnsForwardLookup(String name)
	{
#if defined __ORBIS__
		SceNetId rid = -1;
		SceNetInAddr addr;
		int memid = -1;
		int ret;

		ret = sceNetPoolCreate(__FUNCTION__, 4 * 1024, 0);
		if (ret < 0)
		{
			// NTTNTRACE
			// ps4 toolKit-console 혹은 VS output-console 어느쪽을 선호할지.
			printf("sceNetPoolCreate() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}
		memid = ret;
		ret = sceNetResolverCreate("resolver", memid, 0);
		if (ret < 0)
		{
			printf("sceNetResolverCreate() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}
		rid = ret;
		ret = sceNetResolverStartNtoa(rid, StringT2A(name).GetString(), &addr, 0, 0, 0);
		if (ret < 0)
		{
			printf("sceNetResolverStartNtoa() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}

		ret = sceNetResolverDestroy(rid);
		if (ret < 0)
		{
			printf("sceNetResolverDestroy() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}
		ret = sceNetPoolDestroy(memid);
		if (ret < 0)
		{
			printf("sceNetPoolDestroy() failed (0x%x errno=%d)\n",
				ret, sce_net_errno);
			goto failed;
		}
		return addr.s_addr;

	failed:
		sceNetResolverDestroy(rid);
		sceNetPoolDestroy(memid);

		return 0;
#else // PS4를 제외한 모두
		// Get host adresses
		struct hostent * pHost;

#if !defined(__linux__)
		pHost = gethostbyname(StringT2A(name));
#else
		// 리눅스에선 gethostbyname 이 thread unsafe 하다.
		// 따라서 아래와 같은 방식으로 변경
		ByteArray tmp;
		int herr, hres;
		struct hostent hostbuf;

		tmp.resize(1024);
		while ( (hres = gethostbyname_r(StringT2A(name), &hostbuf, 
			(char*)tmp.GetData(), tmp.GetCount(), &pHost, &herr) ) == ERANGE) 
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

	uint32_t InetAddrV4(const char* addr)
	{
		struct in_addr sa;
		sa.s_addr = INADDR_NONE;
		/* 값을 초기화 해 주지 않으면 inet_pton 실패시 쓰레기 값이 그대로 반환 된다.
		*/

		inet_pton(AF_INET, addr, &sa);
		return sa.s_addr;
	}


	String InetNtopV4(in_addr& addr)
	{
		char cAddr[INET6_ADDRSTRLEN * WCHAR_LENGTH];
#if defined(_WIN32)
		inet_ntop(AF_INET,
			(PVOID)&addr.S_un.S_addr,
			cAddr,
			INET6_ADDRSTRLEN * WCHAR_LENGTH);
		String ret(StringA2T(cAddr));
#else
		inet_ntop(AF_INET,
			&addr.s_addr,
			cAddr,
			INET6_ADDRSTRLEN * WCHAR_LENGTH);
		String ret(StringA2T(cAddr));
#endif		

		return ret;
	}

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

#elif defined __MARMALADE__
		output.Add(StringA2T(s3eSocketGetString(S3E_SOCKET_HOSTNAME)));

#elif defined(__linux__) && !defined(__ANDROID__)
		ifconf ifcfg;
		int fd = socket(AF_INET, SOCK_DGRAM, 0);

		ifcfg.ifc_len = sizeof(struct ifreq) * MAXIFCOUNT;
		ifcfg.ifc_buf = (__caddr_t)malloc(ifcfg.ifc_len);

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

#elif defined(__ANDROID__) || defined(__MACH__)
		struct ifaddrs * ifAddrStruct = NULL;
		struct ifaddrs * ifa = NULL;
		void * tmpAddrPtr = NULL;

		pn_getifaddrs(&ifAddrStruct);

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
			// 			else if (ifa->ifa_addr->sa_family == AF_INET6)
			// 			{ // check it is IP6
			// 				// is a valid IP6 Address
			// 				tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
			// 				char addressBuffer[INET6_ADDRSTRLEN];
			// 				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			// 				//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
			// 			}
			}
		if (ifAddrStruct != NULL)
			pn_freeifaddrs(ifAddrStruct);

#else
		char szHostName[128];
		if (gethostname(szHostName, 128) == 0)
		{
			// Get host adresses
			struct hostent * pHost;
			int i;

			pHost = gethostbyname(szHostName);

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
		sockaddr_in o;
		int olen = sizeof(o);
		if (::getsockname(hSocket, (sockaddr*)&o, (socklen_t *)&olen) != 0)
			return AddrPort::Unassigned;
		else
		{
			AddrPort ret;
			ret.m_binaryAddress = o.sin_addr.s_addr;
			ret.m_port = ntohs(o.sin_port);

			return ret;
		}
	}

	void AssertCloseSocketWillReturnImmediately(SOCKET s)
	{
#ifdef __MARMALADE__
		return;
#else
		struct linger lin;
		size_t lingerLen = sizeof(lin);

		if(getsockopt(
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
			if(lin.l_onoff && lin.l_linger)
			{
				CErrorReporter_Indeed::Report(_PNT("FATAL: Socket which has behavior of some waits in closesocket() has been detected!"));
			}
		}
#endif
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
			hdr->msg_iov[i].iov_len  = sendBuffer.Array()[i].len;
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
			if(r < 0)
				completedLength = r;
			return r;
		}
 #elif __MARMALADE__
		return send_segmented_data_without_sengmsg(socket, sendBuffer, flags);
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
	int sendto_segmented_data( SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen )
	{
#ifdef _WIN32
		{
			int completedLength = 0;
			int r = ::WSASendTo(socket, sendBuffer.GetSegmentPtr(), sendBuffer.GetSegmentCount(), (LPDWORD)&completedLength, flags, sendTo, sendToLen, NULL, NULL);
			if(r < 0)
				completedLength = r;
			return r;
		}
 #elif __MARMALADE__
		return sendto_segmented_data_without_sendmsg(socket, sendBuffer, flags, sendTo, sendToLen);
#else
		{	
			// 비 윈도에서는 sendmsg가 segment data를 보낼 수 있게 해준다.
			CLowFragMemArray<IOV_MAX, iovec> sendBuffer2;
			msghdr hdr;
			FragmentedBufferToMsgHdr(sendBuffer, sendBuffer2, &hdr);
			hdr.msg_name = (sockaddr*)sendTo; // non-const로의 타입캐스팅은 marmalade 대문
			hdr.msg_namelen = sendToLen;

			// Requests not to send SIGPIPE on errors on stream oriented sockets when the other end breaks the connection. The EPIPE error is still returned.
			// http://linux.die.net/man/2/sendmsg
			flags |= MSG_NOSIGNAL;
			return ::sendmsg(socket, &hdr, flags);
		}
#endif
	}

}