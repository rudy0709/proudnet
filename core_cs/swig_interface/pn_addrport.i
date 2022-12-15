%csmethodmodifiers Proud::AddrPort::GetNativeIPAddr() "internal"

%ignore Proud::AddrPort::ToNative;
%ignore Proud::AddrPort::FromNative;
%ignore Proud::AddrPort::ToString;
%ignore Proud::AddrPort::ExtendAddr;

%ignore Proud::AppendTextOut(String& a, AddrPort &b);
%ignore Proud::AppendTextOut(String& a, NamedAddrPort &b);

%ignore Proud::NamedAddrPort::ToString;

// IPV6 Interface
%ignore Proud::AddrPort::m_addr;
%ignore Proud::AddrPort::ExtendAddr::__dummy;
%ignore Proud::AddrPort::ExtendAddr::v4;
%ignore Proud::AddrPort::ExtendAddr::v6;

%ignore Proud::AddrPort::SetIPv6Address;
%ignore Proud::AddrPort::GetIPv4Address;
%ignore Proud::AddrPort::ToNativeV4;
%ignore Proud::AddrPort::ToNativeV6;
%ignore Proud::AddrPort::FromNativeV4;
%ignore Proud::AddrPort::FromNativeV6;
%ignore Proud::AddrPort::FromHostNamePort;


%typemap(cscode) Proud::AddrPort
%{
	public System.Net.IPAddress GetIPAddr()
	{
		return Nettention.Proud.ConvertToCSharp.ConvertNativeIPAddrToManagedIPAddr(GetNativeIPAddr());
	}
%}

%extend Proud::AddrPort
{
	inline void* GetNativeIPAddr()
	{
	    assert(self);
		return &(self->m_addr.v6Byte);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CNetUtil

%csmethodmodifiers Proud::CNetUtil::GetIPVersionFromString "internal"
%csmethodmodifiers Proud::CNetUtil::IsAddressAny "internal"
%csmethodmodifiers Proud::CNetUtil::IsAddressUnspecified "internal"
%csmethodmodifiers Proud::CNetUtil::IsAddressPhysical "internal"
%csmethodmodifiers Proud::CNetUtil::IsAddressLoopback "internal"

%csmethodmodifiers Proud::CNetUtil::LocalIPAddresses_New "internal"
%csmethodmodifiers Proud::CNetUtil::LocalIPAddresses_Delete "internal"
%csmethodmodifiers Proud::CNetUtil::GetLocalIPAddress "internal"

%ignore Proud::CNetUtil::GetLocalIPAddresses;

%typemap(cscode) Proud::CNetUtil
%{
%}

%extend Proud::CNetUtil
{
	inline static void* LocalIPAddresses_New()
	{
		using namespace Proud;

		CFastArray<String>* output = new CFastArray<String>();
		CNetUtil::GetLocalIPAddresses(*output);
		return (void*) output;
	}

	inline static void LocalIPAddresses_Delete(void* ipAddresses)
	{
		assert(ipAddresses);

		using namespace Proud;
		delete (CFastArray<String>*)ipAddresses;
	}

	inline static int GetLocalIPAddresseCount(void* p) throw (Proud::Exception)
	{
		if (p == NULL)
		{
			return 0;
		}

		using namespace Proud;

		CFastArray<String>* ipAddresses = (CFastArray<String>*)p;
		return ipAddresses->GetCount();
	}

	inline static Proud::String GetLocalIPAddress(void* p, int index) throw (Proud::Exception)
	{
		if (p == NULL)
		{
			return Proud::String();
		}

		using namespace Proud;
		CFastArray<String>* ipAddresses = (CFastArray<String>*)p;
		return (*ipAddresses)[index];
	}
}