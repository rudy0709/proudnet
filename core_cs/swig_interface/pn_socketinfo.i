

%typemap(cscode) Proud::CSocketInfo
%{
%}

%extend Proud::CSocketInfo
{
	inline uint32_t GetTCPSocket()
	{
		assert(self);
		return (uint32_t)self->m_tcpSocket;
	}

	inline uint32_t GetUDPSocket()
	{
		assert(self);
		return (uint32_t)self->m_udpSocket;
	}
}

%ignore Proud::CSocketInfo::m_tcpSocket;
%ignore Proud::CSocketInfo::m_udpSocket;