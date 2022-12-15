
// %csmethodmodifiers은 C++ 헤더 파일에서 선언된 함수들과 1:1 매칭되는 C#에서 사용가능한 함수들의 액세스 한정자이다.
// 아래 %typemap(cscode)에 정의되는 함수에 대한 액세스 한정자와 다르다.

%csmethodmodifiers Proud::CNetServer::Create "internal"
%csmethodmodifiers Proud::CNetServer::SendUserMessage "internal"
%csmethodmodifiers Proud::CNetServer::CreateP2PGroup "internal"
%csmethodmodifiers Proud::CNetServer::GetClientHostIDs "internal"
%csmethodmodifiers Proud::CNetServer::GetJoinedP2PGroups "internal"
%csmethodmodifiers Proud::CNetServer::GetClientInfo "internal"
%csmethodmodifiers Proud::CNetServer::AttachStub "internal"
%csmethodmodifiers Proud::CNetServer::AttachProxy "internal"
%csmethodmodifiers Proud::CNetServer::Start "internal"
%csmethodmodifiers Proud::CNetServer::GetP2PGroupInfo "internal"
%csmethodmodifiers Proud::CNetServer::GetMemberCountOfP2PGroup "internal"
%csmethodmodifiers Proud::CNetServer::GetTcpListenerLocalAddrs "internal"
%csmethodmodifiers Proud::CNetServer::SetMessageMaxLength "internal"
%csmethodmodifiers Proud::CNetServer::SetDefaultFallbackMethod "internal"
%csmethodmodifiers Proud::CNetServer::EnableLog "internal"
%csmethodmodifiers Proud::CNetServer::DisableLog "internal"
%csmethodmodifiers Proud::CNetServer::GetMostSuitableSuperPeerInGroup "internal"
%csmethodmodifiers Proud::CNetServer::GetInternalVersion "internal"
%csmethodmodifiers Proud::CNetServer::GetStats "internal"
%csmethodmodifiers Proud::CNetServer::GetP2PRecentPingMs(HostID HostA, HostID HostB) "internal"
%csmethodmodifiers Proud::CNetServer::GetLastUnreliablePingMs(Proud::HostID peerHostID) "internal"
%csmethodmodifiers Proud::CNetServer::GetRecentUnreliablePingMs(Proud::HostID peerHostID) "internal"
%csmethodmodifiers Proud::CNetServer::AllowEmptyP2PGroup "internal"
%csmethodmodifiers Proud::CNetServer::IsEmptyP2PGroupAllowed "internal"
%csmethodmodifiers Proud::CNetServer::SetHostTag "internal"

%csmethodmodifiers Proud::CNetServer::SetTimeoutTimeMs "internal"
%csmethodmodifiers Proud::CNetServer::SetDefaultTimeoutTimeMs "internal"
%csmethodmodifiers Proud::CNetServer::SetDefaultAutoConnectionRecoveryTimeoutTimeMs "internal"
%csmethodmodifiers Proud::CNetServer::SetAutoConnectionRecoveryTimeoutTimeMs "internal"
%csmethodmodifiers Proud::CNetServer::GetP2PConnectionStats "internal"


%typemap(cscode) Proud::CNetServer
%{
	internal static NativeNetServer Create(bool cMemoryOwn)
	{
		global::System.IntPtr cPtr = ProudNetServerPluginPINVOKE.NativeNetServer_Create();
		NativeNetServer ret = (cPtr == global::System.IntPtr.Zero) ? null : new NativeNetServer(cPtr, cMemoryOwn);
		return ret;
	}
%}

%extend Proud::CNetServer
{
	inline Proud::HostID CreateP2PGroup(Proud::HostIDArray* clientHostIDs, Proud::ByteArray* message)
	{
		assert(self);
		assert(clientHostIDs);
		return self->CreateP2PGroup(clientHostIDs->GetData(), clientHostIDs->GetCount(), (message != NULL ? *message : Proud::ByteArray()));
	}

	inline Proud::HostID CreateP2PGroup(Proud::ByteArray* message)
	{
		assert(self);
		return self->CreateP2PGroup(NULL, 0, (message != NULL ? *message : Proud::ByteArray()));
	}

	inline void GetClientHostIDs(Proud::HostIDArray* clientHostIDs)
	{
		assert(self);
		assert(clientHostIDs);

		int count = self->GetClientCount();
		clientHostIDs->SetCount(count);
		self->GetClientHostIDs(clientHostIDs->GetData(), count);
	}

	inline void GetJoinedP2PGroups(Proud::HostID clientHostID, Proud::HostIDArray* output)
	{
		assert(self);
		assert(output);

		self->GetJoinedP2PGroups(clientHostID, *output);
	}

	inline bool GetClientInfo(Proud::HostID clientHostID, Proud::CNetClientInfo* output)
	{
		assert(self);
		assert(output);
		return self->GetClientInfo(clientHostID, *output);

	}

	inline void AttachStub(void* nativeRmiStub) throw (Proud::Exception)
	{
		assert(self);
		assert(nativeRmiStub);

		Proud::IRmiStub* rmiStub = static_cast<Proud::IRmiStub*>(nativeRmiStub);
		self->AttachStub(rmiStub);
	}

	inline void AttachProxy(void* nativeRmiProxy) throw (Proud::Exception)
	{
		assert(self);
		assert(nativeRmiProxy);

		Proud::IRmiProxy* rmiProxy = static_cast<Proud::IRmiProxy*>(nativeRmiProxy);
		self->AttachProxy(rmiProxy);
	}

	inline void Start(Proud::CStartServerParameter &param) throw (Proud::Exception)
	{
		assert(self);
		self->Start(param);
	}

	inline void Stop() throw (Proud::Exception)
	{
		assert(self);
		self->Stop();
	}

	inline Proud::CP2PGroup GetP2PGroupInfo(Proud::HostID groupHostID)
	{
		assert(self);

		Proud::CP2PGroup info;
		self->GetP2PGroupInfo(groupHostID, info);
		return info;
	}

	inline int GetMemberCountOfP2PGroup(Proud::HostID groupHostID)
	{
		assert(self);

		Proud::CP2PGroup info;
		self->GetP2PGroupInfo(groupHostID, info);
		return info.m_members.GetCount();
	}

	inline Proud::CFastArray<Proud::AddrPort> GetTcpListenerLocalAddrs()
	{
		assert(self);

		Proud::CFastArray<Proud::AddrPort> output;
		self->GetTcpListenerLocalAddrs(output);
		return output;
	}

	inline void EnableLog(Proud::String logFileName) throw (Proud::Exception)
	{
		assert(self);
		self->EnableLog(logFileName.GetString());
	}

	inline Proud::CNetServerStats Proud::CNetServer::GetStats()
	{
		assert(self);
		Proud::CNetServerStats outVal;
		self->GetStats(outVal);
		return outVal;
	}

	inline int GetLastUnreliablePingMs(Proud::HostID peerHostID)
	{
		assert(self);
		return self->GetLastUnreliablePingMs(peerHostID);
	}

	inline int GetRecentUnreliablePingMs(Proud::HostID peerHostID)
	{
		assert(self);
		return self->GetRecentUnreliablePingMs(peerHostID);
	}

	inline bool GetP2PConnectionStats(Proud::HostID remoteHostID, Proud::CP2PConnectionStats* status) throw (Proud::Exception)
	{
		assert(self);
		return self->GetP2PConnectionStats(remoteHostID, *status);
	}

	inline bool GetP2PConnectionStats(Proud::HostID remoteA, Proud::HostID remoteB, Proud::CP2PPairConnectionStats* status) throw (Proud::Exception)
	{
		assert(self);
		return self->GetP2PConnectionStats(remoteA, remoteB, *status);
	}
}

// Swig 인터페이스를 사용하여 재 정의
%ignore Proud::CNetServer::Stop();

// Swig 인터페이스를 사용하여 재 정의
%ignore Proud::CNetServer::CreateP2PGroup(const HostID* clientHostIDs, int count, ByteArray message = ByteArray(), CP2PGroupOption &option = CP2PGroupOption::Default, HostID assignedHostID = HostID_None);
%ignore Proud::CNetServer::CreateP2PGroup(ByteArray message = ByteArray(), CP2PGroupOption &option = CP2PGroupOption::Default, HostID assignedHostID = HostID_None);
%ignore Proud::CNetServer::CreateP2PGroup(HostID memberID, ByteArray message = ByteArray(), CP2PGroupOption &option = CP2PGroupOption::Default, HostID assignedHostID = HostID_None);

%ignore Proud::CNetServer::GetStats(CNetServerStats &outVal);

// Swig 인터페이스를 사용하여 재 정의
%ignore Proud::CNetServer::GetClientHostIDs(HostID* output, int outputLength);

%ignore Proud::CNetServer::GetJoinedP2PGroups(HostID clientHostID, HostIDArray& output);

%ignore Proud::CNetServer::GetLastPingSec(HostID clientID, ErrorType* error = NULL);

%ignore Proud::CNetServer::GetLastUnreliablePingMs(HostID clientID, ErrorType* error = NULL);

%ignore Proud::CNetServer::GetRecentPingSec(HostID clientID, ErrorType* error = NULL);

%ignore Proud::CNetServer::GetRecentUnreliablePingMs(HostID clientID, ErrorType* error = NULL);

%ignore Proud::CNetServer::GetP2PGroupInfo(HostID groupHostID, CP2PGroup &output);

%ignore Proud::CNetServer::GetP2PGroups(CP2PGroups& output);

%ignore Proud::CNetServer::GetClientInfo(HostID clientHostID, CNetClientInfo& output);

%ignore Proud::CNetServer::GetPeerInfo(HostID clientHostID, CNetClientInfo& output);

//%ignore Proud::CNetServer::SetHostTag(HostID hostID, void* hostTag);

//%ignore Proud::CNetServer::JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray message = ByteArray());

//%ignore Proud::CNetServer::SetEventSink(INetServerEvent* eventSink);

%ignore Proud::CNetServer::Start(CStartServerParameter &param, ErrorInfoPtr& outError);

%ignore Proud::CNetServer::Start(CStartServerParameter &param);

%ignore Proud::CNetServer::GetRemoteIdentifiableLocalAddrs(CFastArray<NamedAddrPort> &output);

%ignore Proud::CNetServer::GetTcpListenerLocalAddrs(CFastArray<AddrPort> &output);

%ignore Proud::CNetServer::GetUdpListenerLocalAddrs(CFastArray<AddrPort> &output);

//%ignore Proud::CNetServer::SetDefaultFallbackMethod(FallbackMethod newValue);

%ignore Proud::CNetServer::EnableLog(const TCHAR* logFileName);

//%ignore Proud::CNetServer::SetDirectP2PStartCondition(DirectP2PStartCondition newVal);

//%ignore Proud::CNetServer::GetMostSuitableSuperPeerInGroup(HostID groupID, const CSuperPeerSelectionPolicy& policy = CSuperPeerSelectionPolicy::GetOrdinary(), const HostID* excludees = NULL, intptr_t excludeesLength = 0);

//%ignore Proud::CNetServer::GetMostSuitableSuperPeerInGroup(HostID groupID, const CSuperPeerSelectionPolicy& policy, HostID excludee);

%ignore Proud::CNetServer::GetSuitableSuperPeerRankListInGroup(HostID groupID, SuperPeerRating* ratings, int ratingsBufferCount, const CSuperPeerSelectionPolicy& policy = CSuperPeerSelectionPolicy::GetOrdinary(), const CFastArray<HostID> &excludees = CFastArray<HostID>());

%ignore Proud::CNetServer::GetUdpSocketAddrList(CFastArray<AddrPort> &output);

%ignore Proud::CNetServer::GetP2PConnectionStats(HostID remoteHostID,/*output */CP2PConnectionStats& status);

%ignore Proud::CNetServer::GetP2PConnectionStats(HostID remoteA, HostID remoteB, CP2PPairConnectionStats& status);

%ignore Proud::CNetServer::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output);

%ignore Proud::CNetServer::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output);

//%ignore Proud::CNetServer::SendUserMessage(const HostID* remotes, int remoteCount, RmiContext &rmiContext, uint8_t* payload, int payloadLength);

//%ignore Proud::CNetServer::SendUserMessage(HostID remote, RmiContext &rmiContext, uint8_t* payload, int payloadLength);

%ignore Proud::CNetServer::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent);

%ignore Proud::CNetServer::OnClientJoin;
%ignore Proud::CNetServer::OnClientLeave;
%ignore Proud::CNetServer::OnClientHackSuspected;
%ignore Proud::CNetServer::OnClientOffline;
%ignore Proud::CNetServer::OnClientOnline;
%ignore Proud::CNetServer::OnConnectionRequest;
%ignore Proud::CNetServer::OnP2PGroupRemoved;
%ignore Proud::CNetServer::OnUserWorkerThreadBegin;
%ignore Proud::CNetServer::OnUserWorkerThreadEnd;
%ignore Proud::CNetServer::OnP2PGroupJoinMemberAckComplete;

%ignore Proud::ToString(ConnectionState cs);
