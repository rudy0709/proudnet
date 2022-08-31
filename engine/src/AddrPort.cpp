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

#include "../include/CriticalSect.h"
#include "../include/AddrPort.h"
#include "../include/ErrorInfo.h"
#include "SocketUtil.h"
#include "FastSocket.h"


namespace Proud
{
	AddrPort::AddrPort()
	{
		memset_s(&m_addr, sizeof(m_addr), 0xFF, PN_IPV6_ADDR_LENGTH);
		m_port = 0xFFFF;
	}

	// output에다 ipv4 주소를 채운다. this가 ipv6 주소를 가진 경우 false를 리턴하면, errorInfo는 뭔가로 채워진다.
	// this가 ipv4 주소를 갖고 있으면 output이 채워지고 true를 리턴한다. errorInfo에는 아무것도 안 채워진다.
	bool AddrPort::ToNativeV4(ExtendSockAddr& output, ErrorInfo& outErrorInfo) const
	{
		outErrorInfo.m_errorType = ErrorType_Ok;

		// 이 클래스는 ipv4 or v6 상관없이 공용으로 사용된다.
		// 따라서, IPv6 주소지만, any or loopback이면 IPv4에 해당하는 주소로 변환하여 리턴한다.
		if (Is0000Address())
		{
			output.u.v4.sin_port = htons(m_port);
			output.u.v4.sin_addr.s_addr = 0;
			output.u.v4.sin_family = AF_INET;

			return true;
		}
		if (IsFFFFAddress())
		{
			output.u.v4.sin_port = htons(m_port);
			output.u.v4.sin_addr.s_addr = (uint32_t)-1;
			output.u.v4.sin_family = AF_INET;

			return true;
		}
		if (IsLocalhostAddress())
		{
			output.u.v4.sin_port = htons(m_port);
			output.u.v4.sin_addr.s_addr = InetAddrV4("127.0.0.1"); // localhost가 아님! FQDN을 검색하면 device time 생기니까.
			output.u.v4.sin_family = AF_INET;

			return true;
		}

		if (IsIPv4MappedIPv6Addr() == false)
		{
			// 변환 실패. errorInfo를 채운다.
			Proud::String a = AddrPort::ToString();
			Tstringstream ss;
			ss << a.GetString() << _PNT(" is not an IPv4 address!");

			// 에러를 출력하도록 한다.
			outErrorInfo.m_errorType = ErrorType_UnknownAddrPort;
			outErrorInfo.m_comment = ss.str().c_str();

			return false;
		}

		// 성공
		output.u.v4.sin_port = htons(m_port);
		output.u.v4.sin_addr.s_addr = m_addr.v4;
		output.u.v4.sin_family = AF_INET;

		return true;
	}

	void AddrPort::ToNativeV6(ExtendSockAddr& out) const
	{
		out.u.v6.sin6_port = htons(m_port);

		// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		//memcpy(&out.u.v6.sin6_addr, m_addr.v6Byte, sizeof(in6_addr));
		memcpy_s(&out.u.v6.sin6_addr, sizeof(out.u.v6.sin6_addr), m_addr.v6Byte, sizeof(in6_addr));
		out.u.v6.sin6_family = AF_INET6;
	}

	void AddrPort::FromNativeV4(const sockaddr_in& in)
	{
		const sockaddr_in& sa = in;
		SetIPv4MappedIPv6Address(sa.sin_addr.s_addr);
		m_port = ntohs(sa.sin_port);
	}

	void AddrPort::FromNativeV6(const sockaddr_in6& in)
	{
		SetIPv6Address(in.sin6_addr);
		m_port = ntohs(in.sin6_port);
	}

	void AddrPort::FromNative(const ExtendSockAddr& in)
	{
		if (in.u.s.sa_family == AF_INET)
		{
			FromNativeV4(in.u.v4);
		}
		else
		{
			FromNativeV6(in.u.v6);
		}
	}

	String AddrPort::ToString() const
	{
		String a2;

		if (IsIPv4MappedIPv6Addr() == true)
		{
			ExtendSockAddr native;
			ErrorInfo err;
			if(ToNativeV4(native, err))
			{
			a2.Format(_PNT("%s:%d"), InetNtopV4(native.u.v4.sin_addr).GetString(), m_port);
		}
		else
		{
				a2 = _PNT("Error: ") + err.m_comment;
			}
		}
		else
		{
			ExtendSockAddr native;
			ToNativeV6(native);
			a2.Format(_PNT("%s:%d"), InetNtopV6(native.u.v6.sin6_addr).GetString(), m_port);
		}

		return a2;
	}

	String AddrPort::IPToString() const
	{
		String a2;

		if (IsIPv4MappedIPv6Addr() == true)
		{
			ExtendSockAddr native;
			ErrorInfo err;
			if(ToNativeV4(native, err))
			a2 = InetNtopV4(native.u.v4.sin_addr);
			else
				a2 = _PNT("Error: ") + err.m_comment;

		}
		else
		{
			ExtendSockAddr native;
			ToNativeV6(native);
			a2 = InetNtopV6(native.u.v6.sin6_addr);
		}

		return a2;
	}

	Proud::AddrPort AddrPort::FromIPPortV4(const String& ipAddress, uint16_t port)
	{
		AddrPort out;
		// Q: DnsForwardLookup을 안 쓰고요?
		// A: 239.255.255.0 같은 broadcast address는 DNS lookup에서 나오지 않습니다.
		out.SetIPv4MappedIPv6Address(InetAddrV4(StringT2A(ipAddress.GetString()).GetString()));
		out.m_port = port;

		return out;
	}

	Proud::AddrPort AddrPort::FromIPPortV6(const String& ipAddress, uint16_t port)
	{
		AddrPort out;
		out.SetIPv6Address(InetAddrV6(StringT2A(ipAddress.GetString()).GetString()));
		out.m_port = port;

		return out;
	}

	Proud::AddrPort AddrPort::FromIPPort(const int32_t& af, const String& ipAddress, uint16_t port)
	{
		if (ipAddress.IsEmpty())
		{
			return AddrPort::FromAnyIPPort(af, port);
		}
		else if (af == AF_INET)
		{
			return FromIPPortV4(ipAddress, port);
		}
		else if(af == AF_INET6)
		{
			return FromIPPortV6(ipAddress, port);
		}

		throw Exception(_PNT("AddrPort supported AF_INET or AF_INET6 only!"));
	}

	Proud::AddrPort AddrPort::FromAnyIPPort(const int32_t& af, uint16_t port)
	{
		AddrPort out;
		if (af == AF_INET)
		{
			out.SetIPv4MappedIPv6Address(0);
		}
		else if(af == AF_INET6)
		{
			memset(out.m_addr.v6Byte, 0, sizeof(out.m_addr));
		}
		else
		{
			throw Exception(_PNT("AddrPort supported AF_INET or AF_INET6 only!"));
		}

		out.m_port = port;

		return out;
	}

	bool AddrPort::FromHostNamePort(AddrPort* outAddrPort, SocketErrorCode& errorCode, const String& hostName, uint16_t port)
	{
		if (outAddrPort == nullptr)
		{
			return false;
		}

		CFastArray<AddrInfo> ipAddressList;

		SocketErrorCode ret = Proud::DnsForwardLookup(hostName.GetString(), port, ipAddressList);

		if ((ret != SocketErrorCode_Ok ) || (ipAddressList.GetCount() <= 0))
		{
			errorCode = ret;
			return false;
		}

		outAddrPort->FromNative(ipAddressList[0].m_sockAddr);

		return true;
	}

	AddrPort AddrPort::From(const NamedAddrPort& src)
	{
		// FQDN or IPv4 or 6 literal 형태의 문자열을 받으면, 이를 변형하도록 한다.
		// 가급적이면 이 함수는 안 쓰는 쪽이 좋다.
		AddrInfo addrInfo;
		Proud::DnsForwardLookupAndGetPrimaryAddress(src.m_addr.GetString(), src.m_port, addrInfo);

		AddrPort addrPort;
		addrPort.FromNative(addrInfo.m_sockAddr);
		addrPort.m_port = src.m_port;

		return addrPort;
	}


	AddrPort AddrPort::Unassigned;

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
		ret.m_addr = src.IPToString();
		ret.m_port = src.m_port;

		return ret;
	}

	String NamedAddrPort::ToString() const
	{
		String fmt;
		fmt.Format(_PNT("%s:%d"),m_addr.GetString(),m_port);
		return fmt;
	}

	/// 브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?
	bool AddrPort::IsUnicastEndpoint() const
	{
		if(m_port == 0 /*|| m_port == 0xffff*/)
			return false;

		if (Is0000Address() || IsFFFFAddress())
			return false;

		return true;
	}

	bool AddrPort::IsAnyOrUnicastEndpoint() const
	{
		if (m_port == 0 /*|| m_port == 0xffff*/)
			return false;

		if (IsIPv4MappedIPv6Addr() == true)
		{
			uint32_t binaryAddr = 0;
			GetIPv4Address(&binaryAddr);
			if (binaryAddr == 0xffffffff)
				return false;
		}
		else
		{
			static uint8_t max[16] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

			if (memcmp(m_addr.v6Byte, max, sizeof(m_addr)) == 0)
			{
				return false;
			}
		}

		return true;
	}

// ipv4 v6 상관없이, 0xff로 이루어진 주소인가?
	bool AddrPort::IsFFFFAddress() const
	{
		// for ipv4
		if (IsIPv4MappedIPv6Addr())
			return m_addr.v4 == 0xffffffff;

		// for ipv6
		const static uint8_t max[16] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		return memcmp(m_addr.v6Byte, max, sizeof(m_addr)) == 0;
	}

// ipv4 or v6 상관없이, 0x00로 이루어졌나?
// ::ffff:0.0.0.0도 true로 간주한다.
	bool AddrPort::Is0000Address() const
	{
		// check for ipv4
		if (IsIPv4MappedIPv6Addr())
		{
			return m_addr.v4 == 0;
		}

		// for ipv6
		const static uint8_t zero[16] = { 0x00 };

		return memcmp(m_addr.v6Byte, zero, sizeof(m_addr)) == 0;
	}

	// ipv4 or v6 상관없이, localhost 주소인가?
	// ipv4이면 127.0.0.1인가? v6이면 ::1인가?
	bool AddrPort::IsLocalhostAddress() const
	{
		// ipv6 주소인 경우 ::1. ipv4 주소인 경우 127.0.0.1.
		if (IsIPv4MappedIPv6Addr())
		{
			bool ret = m_addr.v4 == InetAddrV4("127.0.0.1"); // localhost가 아님! FQDN을 검색하면 device time 생기니까.
			return ret;
		}
		else
		{
			in6_addr x = InetAddrV6("::1"); // localhost가 아님! FQDN을 검색하면 device time 생기니까.
			assert(sizeof(x) == PN_IPV6_ADDR_LENGTH);
			return memcmp(m_addr.v6Byte, &x, PN_IPV6_ADDR_LENGTH);
		}
	}

	bool AddrPort::IsIPv4MappedIPv6Addr() const
	{
// PS4에서는 IPv4 socket만 사용 가능하므로
#ifndef __ORBIS__
		if (m_addr.v6Byte[0] == 0x00 &&
			m_addr.v6Byte[1] == 0x00 &&
			m_addr.v6Byte[2] == 0x00 &&
			m_addr.v6Byte[3] == 0x00 &&
			m_addr.v6Byte[4] == 0x00 &&
			m_addr.v6Byte[5] == 0x00 &&
			m_addr.v6Byte[6] == 0x00 &&
			m_addr.v6Byte[7] == 0x00 &&
			m_addr.v6Byte[8] == 0x00 &&
			m_addr.v6Byte[9] == 0x00 &&
			m_addr.v6Byte[10] == 0xFF &&
			m_addr.v6Byte[11] == 0xFF)
		{
			return true;
		}

		return false;
#else
		return true;
#endif
	}

	void AddrPort::SetIPv4MappedIPv6Address(uint32_t ipv4Address)
	{
		//memset(&m_addr, 0, sizeof(m_addr));
		memset_s(&m_addr, sizeof(m_addr), 0, sizeof(m_addr));

		m_addr.v6Short[5] = 0xFFFF;
		m_addr.v4 = ipv4Address;
	}

	void AddrPort::SetIPv6Address(const in6_addr& addr)
	{
		// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		//memset(&m_addr, 0, sizeof(m_addr));
		//memcpy(m_addr.v6Byte, &addr, sizeof(in6_addr));
		memset_s(&m_addr, sizeof(m_addr), 0, sizeof(m_addr));
		memcpy_s(m_addr.v6Byte, sizeof(m_addr.v6Byte), &addr, sizeof(in6_addr));
	}

	void AddrPort::SetIPv6Address(const uint8_t* src, const size_t length)
	{
		// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		//memset(&m_addr, 0, sizeof(m_addr));
		//memcpy(m_addr.v6Byte, src, length);
		memset_s(&m_addr, sizeof(m_addr), 0, sizeof(m_addr));
		memcpy_s(m_addr.v6Byte, sizeof(m_addr.v6Byte), src, length);
	}

	// pref를 주소 앞단에 채우고, ipv4 주소를 뒷단에 채운다.
	void AddrPort::Synthesize(const uint8_t* pref, const size_t prefLength, const uint32_t v4BinaryAddress)
	{
		// 해당 함수는 가변길이의 pref64 를 synthesize 하기 위한 함수이므로 아래의 validation 체크는 무의미 합니다.
//		if (prefLength > PN_IPV6_ADDR_LENGTH)
//			throw Exception("Invalid parameter at AddrPort.Synthesize!");

		// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		//memset(&m_addr, 0, sizeof(m_addr));
		//memcpy(m_addr.v6Byte, pref, prefLength);
		memset_s(&m_addr, sizeof(m_addr), 0, sizeof(m_addr));
		memcpy_s(m_addr.v6Byte, sizeof(m_addr.v6Byte), pref, prefLength);
		m_addr.v4 = v4BinaryAddress;
	}

	bool AddrPort::GetIPv4Address(uint32_t* outIPv4Address) const
	{
		if (IsIPv4MappedIPv6Addr() == false)
		{
			return false;
		}

		*outIPv4Address = m_addr.v4;

		return true;
	}

	/// 브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?
	bool NamedAddrPort::IsUnicastEndpoint()
	{
		m_addr = m_addr.Trim();

		if(m_port == 0 /*|| m_port == 0xffff*/)
			return false;

		if (CNetUtil::IsAddressUnspecified(m_addr)
			|| CNetUtil::IsAddressAny(m_addr))
		{
			return false;
		}

		return true;
	}

	void NamedAddrPort::OverwriteHostNameIfExists(const String &hostName)
	{
		if(!CNetUtil::IsAddressUnspecified(hostName))
		{
			m_addr = hostName;
		}
	}

	void CNetUtil::GetLocalIPAddresses(CFastArray<String> &output)
	{
		SocketInitializer::GetSharedPtr(); // WSAStartup이 호출되게 함.

		output.Clear();

		Proud::GetLocalIPAddresses(output);
	}

	int CNetUtil::GetIPVersionFromString(const String& rhs)
	{
		if (rhs.Find(_PNT(".")) != -1)
		{
			return AF_INET;
		}
		else if (rhs.Find(_PNT(":")) != -1)
		{
			return AF_INET6;
		}

		return -1;
	}

	bool CNetUtil::IsAddressAny(const String& address)
	{
		return address.Compare(_PNT("255.255.255.255")) == 0
			|| address.CompareNoCase(_PNT("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")) == 0;
	}

	bool CNetUtil::IsAddressUnspecified(const String& address)
	{
		return address.Compare(_PNT("0.0.0.0")) == 0
			|| address.Compare(_PNT("::")) == 0
			|| address.IsEmpty();
	}

	bool CNetUtil::IsAddressPhysical(const String& address)
	{
		return !CNetUtil::IsAddressAny(address)
			&& !CNetUtil::IsAddressUnspecified(address)
			&& !CNetUtil::IsAddressLoopback(address);
	}

	bool CNetUtil::IsAddressLoopback(const String& address)
	{
		return address.Compare(_PNT("127.0.0.1")) == 0
			|| address.Compare(_PNT("::1")) == 0
			|| address.Compare(_PNT("localhost")) == 0;
	}

#if defined(_WIN32)
	ErrorType CNetUtil::EnableTcpOffload(bool enable)
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
