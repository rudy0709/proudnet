
%csmethodmodifiers Proud::CNetPeerInfo::m_joinedP2PGroups "private"

%typemap(cscode) Proud::CNetPeerInfo
%{
	public HostIDArray GetJoinedP2PGroups()
	{
		int count = joinedP2PGroups.GetCount();
		if (count == 0)
		{
			return null;
		}

		HostIDArray idArray = new HostIDArray();

		for (int i = 0; i < count; ++i )
		{
			idArray.Add(joinedP2PGroups.At(i));
		}

		return idArray;
	}
%}

%extend Proud::CNetPeerInfo
{

}

//%ignore Proud::CNetPeerInfo::m_joinedP2PGroups;
