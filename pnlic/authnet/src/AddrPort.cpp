/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>

#if defined(__linux__)
#define MAXIFCOUNT 30

#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif

#if defined(__ANDROID__)
#include "ifaddrs.h"
#elif defined(__MACH__)
#include <ifaddrs.h>

#define pn_getifaddrs  getifaddrs
#define pn_freeifaddrs freeifaddrs
#endif

#include "../include/CriticalSect.h"
#include "../include/AddrPort.h"
#include "SocketUtil.h"
#include "FastSocket.h"

#define _COUNTOF(array) (sizeof(array)/sizeof(array[0]))

namespace Proud
{

	void AddrPort::ToNative( sockaddr_in& out )
	{
		out.sin_port = htons( m_port );
		out.sin_addr.s_addr = m_binaryAddress;
		out.sin_family = AF_INET;
	}

	void AddrPort::FromNative( const sockaddr_in& in )
	{
		const sockaddr_in& sa = in;
		m_binaryAddress = sa.sin_addr.s_addr;
		m_port = ntohs( sa.sin_port );
	}

	// AddrPort.GetOneLocalAddress에서 사용됨. AddrPort.GetOneLocalAddress 자체가 잘 안쓰이므로 이 클래스는 자체도 잘 안쓰임.
	class CLocalIPPreserve:public CSingleton<CLocalIPPreserve>
	{
		CriticalSection cs;
		SocketInitializer::Holder m_dep;
	public:
		AddrPort ret;

		inline CLocalIPPreserve()
		{
			String a = CNetUtil::GetOneLocalAddress();
			int b = DnsForwardLookup(a);
			ret.m_binaryAddress = b;
			assert(ret.m_binaryAddress != 0);
			ret.m_port = htons(0);
		}
	};

	Proud::AddrPort AddrPort::GetOneLocalAddress()
	{
		// inet_addr 등은 매우 느린 메서드다. 어차피 로컬 IP는 그렇게 자주 변하는건 아니니까 처음 1회만 새 값을 얻도록 한다.
		return CLocalIPPreserve::Instance().ret;
	}

	AddrPort AddrPort::GetIPAddress(String hostName)
	{
		int b = Proud::DnsForwardLookup(hostName);

		AddrPort ret2;
		ret2.m_binaryAddress = b;
		ret2.m_port = 0; // htons(0) == 0;

		return ret2;
	}

	String AddrPort::ToString() const
	{
		in_addr a;
#if !defined(_WIN32)
		a.s_addr = m_binaryAddress;
#else
		a.S_un.S_addr = m_binaryAddress;
#endif
		String a2;
		a2.Format(_PNT("%s:%d"), InetNtopV4(a).GetString(), m_port);
		return a2;
	}

	String AddrPort::IPToString() const
	{
		in_addr a;
#if !defined(_WIN32)
		a.s_addr = m_binaryAddress;
#else
		a.S_un.S_addr = m_binaryAddress;
#endif
		String a2;
		a2 = InetNtopV4(a);
		return a2;
	}

	Proud::AddrPort AddrPort::FromIPPort(const String& ipAddress, uint16_t port)
	{
		AddrPort out;
		// Q: DnsForwardLookup을 안 쓰고요?
		// A: 239.255.255.0 같은 broadcast address는 DNS lookup에서 나오지 않습니다.
		out.m_binaryAddress = InetAddrV4(StringT2A(ipAddress));
		out.m_port = port;

		return out;
	}

	Proud::AddrPort AddrPort::FromHostNamePort(const String& hostName, uint16_t port)
	{
		AddrPort out;
		out.m_binaryAddress = Proud::DnsForwardLookup(hostName);
		out.m_port = port;

		return out;
	}


	String AddrPort::ToDottedIP() const
	{
		in_addr in;
		in.s_addr = m_binaryAddress;
		return InetNtopV4(in);
	}

	AddrPort AddrPort::From(const NamedAddrPort& src)
	{
		AddrPort ret;
		ret.m_binaryAddress = Proud::DnsForwardLookup(src.m_addr);
		ret.m_port = src.m_port;
		return ret;
	}


	AddrPort AddrPort::Unassigned(0xffffffff, 0xffff);

	NamedAddrPort NamedAddrPort::Unassigned = NamedAddrPort::FromAddrPort(_PNT(""), 0xffff);

	NamedAddrPort NamedAddrPort::FromAddrPort(String addr, uint16_t port)
	{
		NamedAddrPort ret;
		ret.m_addr = addr;
		ret.m_port = port;

		return ret;
	}

	NamedAddrPort NamedAddrPort::From(const AddrPort &src)
	{
		NamedAddrPort ret;
		ret.m_addr = src.ToDottedIP();
		ret.m_port = src.m_port;

		return ret;
	}

	String NamedAddrPort::ToString() const
	{
		String fmt;
		fmt.Format(_PNT("%s:%d"),m_addr.GetString(),m_port);
		return fmt;
	}

	/// 로컬 컴퓨터를 가리키는 IP인가?
	bool AddrPort::IsLocalIP()
	{
		return (m_binaryAddress == InetAddrV4("127.0.0.1"));
	}

	/// 브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?
	bool AddrPort::IsUnicastEndpoint()
	{
		if(m_port == 0 /*|| m_port == 0xffff*/)
			return false;
		if(m_binaryAddress == 0 || m_binaryAddress == 0xffffffff)
			return false;
		return true;
	}

	/// 로컬 컴퓨터를 가리키는 IP인가?
	bool NamedAddrPort::IsLocalIP()
	{
		m_addr = m_addr.Trim();
		return m_addr == _PNT("127.0.0.1") || m_addr == _PNT("localhost");
	}

	/// 브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?
	bool NamedAddrPort::IsUnicastEndpoint()
	{
		m_addr = m_addr.Trim();

		if(m_port == 0 /*|| m_port == 0xffff*/)
			return false;

		if (m_addr == _PNT("") || m_addr == _PNT("0.0.0.0") || m_addr == _PNT("255.255.255.255"))
			return false;
		return true;
	}

	void NamedAddrPort::OverwriteHostNameIfExists( String hostName )
	{
		if(hostName.GetLength() > 0 && hostName != _PNT("0.0.0.0"))
		{
			m_addr = hostName;
		}
	}

	String CNetUtil::GetOneLocalAddress()
	{
		CFastArray<String> ip_addresses;
		GetLocalIPAddresses(ip_addresses);
		int cnt = ip_addresses.GetCount();
		if (cnt > 0) {
			int i;
			for (i = 0; i < cnt; i++)
			{
				if (ip_addresses[i].Compare(_PNT("127.0.0.1")) != 0)
				{
					return ip_addresses[i];
				}
			}

			if (i == cnt)
				return ip_addresses[0];
		}
		return _PNT("");
	}

	void CNetUtil::GetLocalIPAddresses(CFastArray<String> &output)
	{
		SocketInitializer::Instance();

		output.Clear();

		Proud::GetLocalIPAddresses(output);
	}
    
#if defined(_WIN32)
	ErrorType CNetUtil::EnableTcpOffload( bool enable )
	{
		// Q: ATL을 왜 안써요? 
		// A: UE4에서는 ATL을 쓰지 못하기 때문입니다. 물론 쓸 수 있게는 할 수 있지만 지금 PN은 멀티 플랫폼 라이브러리이므로 가급적 피해야.
		const HKEY keyParent(HKEY_LOCAL_MACHINE);
		// tcp offload를 설정하는 레지스트리 위치
		const PNTCHAR keyName[] = _PNT("SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters");
		const PNTCHAR valueName[] = _PNT("DisableTaskOffload");

		const uint32_t on(enable == true ? 0 : 1);

		HKEY keyCreated(NULL);
		DWORD disposition;
		const LSTATUS statusCreateKey(::RegCreateKeyEx(keyParent, keyName, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &keyCreated, &disposition));
		if (statusCreateKey != ERROR_SUCCESS)
		{
			if (statusCreateKey == ERROR_ACCESS_DENIED)
			{
				return ErrorType_PermissionDenied;
			}
			else
			{
				return ErrorType_Unexpected;
			}
		}

		const LSTATUS statusSetValue(::RegSetValueEx(keyCreated, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&on), sizeof(DWORD)));
		if (statusSetValue != ERROR_SUCCESS)
		{
			if (statusSetValue == ERROR_ACCESS_DENIED)
			{
				return ErrorType_PermissionDenied;
			}
			else
			{
				return ErrorType_Unexpected;
			}
		}

		if (keyCreated != NULL)
		{
			const LSTATUS statusCloseKey(::RegCloseKey(keyCreated));
			if (statusCloseKey != ERROR_SUCCESS)
			{
				if (statusCloseKey == ERROR_ACCESS_DENIED)
					return ErrorType_PermissionDenied;
			}
			else
			{
				return ErrorType_Unexpected;
			}
		}

		return ErrorType_Ok;
	}
#endif
}
