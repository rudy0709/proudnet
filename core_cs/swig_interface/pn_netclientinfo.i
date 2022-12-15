
%extend Proud::CNetClientInfo
{
	inline void* GetHostTag()
	{
		assert(self);
		return self->m_hostTag;
	}
}

%ignore Proud::CNetClientInfo::m_joinedP2PGroups;

%ignore Proud::CNetClientInfo::m_hostTag;