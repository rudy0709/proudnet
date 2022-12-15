
%csmethodmodifiers Proud::CNetClient::Create "internal"
%csmethodmodifiers Proud::CNetClient::SendUserMessage "internal"
%csmethodmodifiers Proud::CNetClient::GetLastUnreliablePingMs "internal"
%csmethodmodifiers Proud::CNetClient::GetLastUnreliablePingSec "internal"
%csmethodmodifiers Proud::CNetClient::GetLastReliablePingMs "internal"
%csmethodmodifiers Proud::CNetClient::GetLastReliablePingSec "internal"
%csmethodmodifiers Proud::CNetClient::GetRecentReliablePingMs "internal"
%csmethodmodifiers Proud::CNetClient::GetRecentReliablePingSec "internal"
%csmethodmodifiers Proud::CNetClient::GetServerConnectionState "internal"
%csmethodmodifiers Proud::CNetClient::GetRecentUnreliablePingMs "internal"
%csmethodmodifiers Proud::CNetClient::GetDirectP2PInfo "internal"
%csmethodmodifiers Proud::CNetClient::ForceP2PRelay "internal"
%csmethodmodifiers Proud::CNetClient::Disconnect "internal"
%csmethodmodifiers Proud::CNetClient::SetDefaultTimeoutTimeMs "internal"
%csmethodmodifiers Proud::CNetClient::Connect "internal"
%csmethodmodifiers Proud::CNetClient::GetPublicAddress "internal"
%csmethodmodifiers Proud::CNetClient::DisconnectAsync "internal"

%typemap(cscode) Proud::CNetClient
%{
	  // cMemoryOwn 변수는 C# GC에서 managed 객체를 삭제할 때 연결되어 있는 native 객체를 삭제 유/무를 설정할 수 있습니다.
	  internal static NativeNetClient Create(bool cMemoryOwn) {
		global::System.IntPtr cPtr = ProudNetClientPluginPINVOKE.NativeNetClient_Create();
		NativeNetClient ret = (cPtr == global::System.IntPtr.Zero) ? null : new NativeNetClient(cPtr, cMemoryOwn);
		return ret;
	  }
%}

%extend Proud::CNetClient
{
	inline void HolsterMoreCallbackUntilNextFrameMove()
	{
		assert(self);
		self->HolsterMoreCallbackUntilNextFrameMove();
	}

	inline void DisconnectAsync() throw (Proud::Exception)
	{
		assert(self);
		Proud::CDisconnectArgs args;
		self->DisconnectAsync(args);
	}

	inline bool Connect(Proud::CNetConnectionParam* param) throw (Proud::Exception)
	{
		assert(self);
		assert(param);
		return self->Connect(*param);
	}

	inline bool Connect(Proud::CNetConnectionParam* param, Proud::ErrorInfo* outError)
	{
		assert(self);
		assert(param);
		assert(outError);

		Proud::ErrorInfoPtr errorInfo;

		if (false == self->Connect(*param, errorInfo))
		{
			if (outError != NULL)
			{
				*outError = *errorInfo;
			}
			return false;
		}

		return true;
	}

	inline void FrameMove() throw (Proud::Exception)
	{
		assert(self);
		self->FrameMove();
	}

	inline void FrameMove(int maxWaitTime,  Proud::CFrameMoveResult* result) throw (Proud::Exception)
	{
		assert(self);
		self->FrameMove(maxWaitTime, result);
	}

	inline Proud::HostIDArray GetGroupMembers(Proud::HostID groupHostID)
	{
		assert(self);
		Proud::HostIDArray hostIDArray;
		self->GetGroupMembers(groupHostID, hostIDArray);
		return hostIDArray;
	}

	inline Proud::HostIDArray GetLocalJoinedP2PGroups()
	{
		assert(self);
		Proud::HostIDArray hostIDArray;
		self->GetLocalJoinedP2PGroups(hostIDArray);
		return hostIDArray;
	}

	inline Proud::CNetClientStats GetStats()
	{
		assert(self);
		Proud::CNetClientStats status;
		self->GetStats(status);
		return status;
	}

	inline Proud::CNetPeerInfo GetPeerInfo(Proud::HostID peerHostID)
	{
		assert(self);
		Proud::CNetPeerInfo info;
		self->GetPeerInfo(peerHostID, info);
		return info;
	}

	inline Proud::CDirectP2PInfo InvalidateUdpSocket(Proud::HostID peerID)
	{
		assert(self);
		Proud::CDirectP2PInfo directP2PInfo;
		self->InvalidateUdpSocket(peerID, directP2PInfo);
		return directP2PInfo;
	}

	inline Proud::ReliableUdpHostStats GetPeerReliableUdpStats(Proud::HostID peerID)
	{
		assert(self);
		Proud::ReliableUdpHostStats output;
		self->GetPeerReliableUdpStats(peerID, output);
		return output;
	}

	inline bool IsLocalHostBehindNat()
	{
		assert(self);
		bool ret = false;
		self->IsLocalHostBehindNat(ret);
		return ret;
	}

	inline Proud::CSocketInfo GetSocketInfo(Proud::HostID remoteHostID)
	{
		assert(self);
		Proud::CSocketInfo output;
		self->GetSocketInfo(remoteHostID, output);
		return output;
	}

	inline int GetUnreliableMessagingLossRatioPercent(Proud::HostID remotePeerID)
	{
		assert(self);
		int outputPercent = 0;
		self->GetUnreliableMessagingLossRatioPercent(remotePeerID, &outputPercent);
		return outputPercent;
	}

	inline Proud::ErrorType GetUnreliableMessagingLossRatioPercentErrorType(Proud::HostID remotePeerID)
	{
		assert(self);
		int outputPercent = 0;
		return self->GetUnreliableMessagingLossRatioPercent(remotePeerID, &outputPercent);
	}

	inline void SetEventSink(void* obj) throw (Proud::Exception)
	{
		assert(self);
		assert(obj);
		Proud::INetClientEvent* eventSink = static_cast<Proud::INetClientEvent*>(obj);

		//assert(dynamic_cast<CNetClientEventWrap*>(eventSink));

		self->SetEventSink(eventSink);
	}

	inline void AttachProxy(void* obj) throw (Proud::Exception)
	{
		assert(self);
		assert(obj);
		Proud::IRmiProxy* proxy = static_cast<Proud::IRmiProxy*>(obj);

		//assert(dynamic_cast<CRmiProxyWrap*>(proxy));

		self->AttachProxy(proxy);
	}

	inline void AttachStub(void* obj) throw (Proud::Exception)
	{
		assert(self);
		assert(obj);
		Proud::IRmiStub* stub = static_cast<Proud::IRmiStub*>(obj);

		//assert(dynamic_cast<CRmiStubWrap*>(stub));
		self->AttachStub(stub);
	}

	inline int GetLastUnreliablePingMs(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetLastUnreliablePingMs(remoteHostID);
	}

	inline double GetLastUnreliablePingSec(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetLastUnreliablePingSec(remoteHostID);
	}

	inline int GetLastReliablePingMs(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetLastReliablePingMs(remoteHostID);
	}

	inline double GetLastReliablePingSec(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetLastReliablePingSec(remoteHostID);
	}

	inline int GetRecentReliablePingMs(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetRecentReliablePingMs(remoteHostID);
	}

	inline double GetRecentReliablePingSec(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetRecentReliablePingSec(remoteHostID);
	}

	inline int GetRecentUnreliablePingMs(Proud::HostID remoteHostID)
	{
		assert(self);
		return self->GetRecentUnreliablePingMs(remoteHostID);
	}
}

%ignore Proud::ToString(ConnectionState cs);

//%ignore Proud::CNetClient::Create;

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::Connect(const CNetConnectionParam &connectionInfo);
%ignore Proud::CNetClient::Connect(const CNetConnectionParam &connectionInfo, ErrorInfoPtr &outError);

// 필요할 때...
//%ignore Proud::CNetClient::Disconnect(const CDisconnectArgs &args);
%ignore Proud::CNetClient::Disconnect(ErrorInfoPtr& outError);
%ignore Proud::CNetClient::Disconnect(const CDisconnectArgs &args, ErrorInfoPtr& outError);

// 필요할 때...
%ignore Proud::CNetClient::DisconnectAsync();
%ignore Proud::CNetClient::DisconnectAsync(const CDisconnectArgs &agrs);
%ignore Proud::CNetClient::DisconnectAsync(const CDisconnectArgs &args, ErrorInfoPtr& outError);
%ignore Proud::CNetClient::DisconnectAsync(ErrorInfoPtr& outError);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::FrameMove(int maxWaitTime = 0, CFrameMoveResult* outResult = NULL);
%ignore Proud::CNetClient::FrameMove(int maxWaitTime, CFrameMoveResult* outResult, ErrorInfoPtr& outError);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetGroupMembers(HostID groupHostID, HostIDArray &output);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetLocalJoinedP2PGroups(HostIDArray &output);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetStats(CNetClientStats &outVal);

//%ignore Proud::CNetClient::GetLocalUdpSocketAddr(HostID remotePeerID);

//%ignore Proud::CNetClient::GetDirectP2PInfo(HostID remotePeerID, CDirectP2PInfo &outInfo);

//%ignore Proud::CNetClient::GetServerAddrPort();

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetPeerInfo(HostID peerHostID, CNetPeerInfo& output);

//%ignore Proud::CNetClient::SetHostTag(HostID hostID, void* hostTag);

//%ignore Proud::CNetClient::GetServerConnectionState(CServerConnectionState &output);

//%ignore Proud::CNetClient::GetWorkerState(CClientWorkerInfo &output);

%ignore Proud::CNetClient::SetEventSink(INetClientEvent* eventSink);

// 필요할 때...
%ignore Proud::CNetClient::GetLastUnreliablePingSec(HostID remoteHostID, ErrorType* error = NULL);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetLastUnreliablePingMs(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetLastReliablePingMs(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetLastReliablePingSec(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetRecentUnreliablePingSec(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetRecentUnreliablePingMs(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetRecentReliablePingMs(HostID remoteHostID, ErrorType* error = NULL);

// 필요할 때...
%ignore Proud::CNetClient::GetRecentReliablePingSec(HostID remoteHostID, ErrorType* error = NULL);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::InvalidateUdpSocket(HostID peerID, CDirectP2PInfo &outDirectP2PInfo );

//%ignore Proud::CNetClient::TEST_SetPacketTruncatePercent(Proud::HostType hostType, int percent);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetPeerReliableUdpStats(HostID peerID,ReliableUdpHostStats &output);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::IsLocalHostBehindNat(bool &output);

//%ignore Proud::CNetClient::GetPublicAddress();

// 필요할 때...
%ignore Proud::CNetClient::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output);

// 필요할 때...
%ignore Proud::CNetClient::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetSocketInfo(HostID remoteHostID,CSocketInfo& output);

// 필요할 때...
//%ignore Proud::CNetClient::SetApplicationHint(const CApplicationHint &hint);

// 필요할 때...
%ignore Proud::CNetClient::SendUserMessage(const HostID* remotes, int remoteCount, RmiContext &rmiContext, uint8_t* payload, int payloadLength);

// 필요할 때...
//%ignore Proud::CNetClient::SendUserMessage(HostID remote, RmiContext &rmiContext, uint8_t* payload, int payloadLength);

//%ignore Proud::CNetClient::GetTcpLocalAddr();

//%ignore Proud::CNetClient::GetUdpLocalAddr();

// 필요할 때...
%ignore Proud::CNetClient::TEST_GetTestStats(CTestStats2 &output);

// Swig 인터페이스 파일에서 값으로 리턴하도록 함수 재 정의
%ignore Proud::CNetClient::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent);

%ignore Proud::CNetClient::GetWorkerState(CClientWorkerInfo &output);

%ignore Proud::CNetClient::GetEventSink();

//%csmethodmodifiers Proud::CNetClient::Create "private"

%ignore Proud::CNetClient::OnJoinServerComplete;
%ignore Proud::CNetClient::OnLeaveServer;
%ignore Proud::CNetClient::OnP2PMemberJoin;
%ignore Proud::CNetClient::OnP2PMemberLeave;
%ignore Proud::CNetClient::OnChangeP2PRelayState;
%ignore Proud::CNetClient::OnChangeServerUdpState;
%ignore Proud::CNetClient::OnP2PMemberOffline;
%ignore Proud::CNetClient::OnP2PMemberOnline;
%ignore Proud::CNetClient::OnServerOffline;
%ignore Proud::CNetClient::OnServerOnline;
%ignore Proud::CNetClient::OnSynchronizeServerTime;
