
// C++ 클래스 또는 함수 이름을 변경합니다.

// rename class
%rename(NetConnectionParam) Proud::CNetConnectionParam;

%rename(ApplicationHint) Proud::CApplicationHint;

%rename(ClientWorkerInfo) Proud::CClientWorkerInfo;

%rename(DirectP2PInfo) Proud::CDirectP2PInfo;

%rename(DisconnectArgs) Proud::CDisconnectArgs;

%rename(FrameMoveResult) Proud::CFrameMoveResult;

%rename(NetCoreStats) Proud::CNetCoreStats;

%rename(NetClientStats) Proud::CNetClientStats;

%rename(NativeNetClient) Proud::CNetClient;

%rename(NetPeerInfo) Proud::CNetPeerInfo;

%rename(NativeNetUtil) Proud::CNetUtil;

%rename(RemoteOfflineEventArgs) Proud::CRemoteOfflineEventArgs;

%rename(RemoteOnlineEventArgs) Proud::CRemoteOnlineEventArgs;

%rename(ServerConnectionState) Proud::CServerConnectionState;

%rename(SocketInfo) Proud::CSocketInfo;

%rename(NativeRmiContext) Proud::RmiContext;

//%rename(NativeNamedAddrPort) Proud::NamedAddrPort;

%rename(NativeHostIDArray) Proud::HostIDArray;

%rename(NativeException) Proud::Exception;

// ikpil.choi 2017-01-10 : MessageSummary.cs 에 직접 구현해 사용하므로, Native는 없어도 됨
//%rename(NativeMessageSummary) Proud::MessageSummary;
//%rename(NativeAfterRmiSummary) Proud::AfterRmiSummary;
//%rename(NativeBeforeRmiSummary) Proud::BeforeRmiSummary;

%rename(NetClientInfo) Proud::CNetClientInfo;

%rename(NetConfig) Proud::CNetConfig;

%rename(NativeNetServer) Proud::CNetServer;

%rename(P2PConnectionStats) Proud::CP2PConnectionStats;

%rename(P2PPairConnectionStats) Proud::CP2PPairConnectionStats;

%rename(NativeReceivedMessage) Proud::CReceivedMessage;

%rename(StartServerParameter) Proud::CStartServerParameter;

%rename(StartServerParameterBase) Proud::CStartServerParameterBase;

%rename(SuperPeerSelectionPolicy) Proud::CSuperPeerSelectionPolicy;

%rename(SuperPeerSelectionPolicy) Proud::CSuperPeerSelectionPolicy;

%rename(NativeByteArray) Proud::ByteArray;

%rename(NetServerStats) Proud::CNetServerStats;

%rename(P2PGroup) Proud::CP2PGroup;

%rename(P2PGroupOption) Proud::CP2PGroupOption;

%rename(NativeThreadPool) Proud::CThreadPool;

%rename(UserWorkerThreadCallbackContext) Proud::CUserWorkerThreadCallbackContext;


#ifdef __NETSERVER__
%rename(NetServerConfig) Proud::CNetConfig;
#else
%rename(NetClientConfig) Proud::CNetConfig;
#endif

// rename AddrPort public member
%rename(binaryAddress) Proud::AddrPort::m_binaryAddress;
%rename(port) Proud::AddrPort::m_port;

// rename NamedAddrPort public member
%rename(addr) Proud::NamedAddrPort::m_addr;
%rename(port) Proud::NamedAddrPort::m_port;

// rename CApplicationHint public member
%rename(recentFrameRate) Proud::CApplicationHint::m_recentFrameRate;

// rename CClientWorkerInfo public member
%rename(isWorkerThreadNull) Proud::CClientWorkerInfo::m_isWorkerThreadNull;
%rename(connectCallCount) Proud::CClientWorkerInfo::m_connectCallCount;
%rename(disconnectCallCount) Proud::CClientWorkerInfo::m_disconnectCallCount;
%rename(connectionState) Proud::CClientWorkerInfo::m_connectionState;
%rename(finalWorkerItemCount) Proud::CClientWorkerInfo::m_finalWorkerItemCount;
%rename(currentTimeMs) Proud::CClientWorkerInfo::m_currentTimeMs;
%rename(peerCount) Proud::CClientWorkerInfo::m_peerCount;

// rename CDirectP2PInfo public member
%rename(localUdpSocketAddr) Proud::CDirectP2PInfo::m_localUdpSocketAddr;
%rename(localToRemoteAddr) Proud::CDirectP2PInfo::m_localToRemoteAddr;
%rename(remoteToLocalAddr) Proud::CDirectP2PInfo::m_remoteToLocalAddr;

// rename CDirectP2PInfo public member
%rename(gracefulDisconnectTimeoutMs) Proud::CDisconnectArgs::m_gracefulDisconnectTimeoutMs;
%rename(disconnectSleepIntervalMs) Proud::CDisconnectArgs::m_disconnectSleepIntervalMs;

// rename CNetConnectionParam public member
%rename(serverIP) Proud::CNetConnectionParam::m_serverIP;
%rename(serverPort) Proud::CNetConnectionParam::m_serverPort;
%rename(localUdpPortPool) Proud::CNetConnectionParam::m_localUdpPortPool;
%rename(protocolVersion) Proud::CNetConnectionParam::m_protocolVersion;
//%rename(userData) Proud::CNetConnectionParam::m_userData;
%rename(tunedNetworkerSendIntervalMs_TEST) Proud::CNetConnectionParam::m_tunedNetworkerSendIntervalMs_TEST;
%rename(simplePacketMode) Proud::CNetConnectionParam::m_simplePacketMode;
%rename(allowExceptionEvent) Proud::CNetConnectionParam::m_allowExceptionEvent;
%rename(clientAddrAtServer) Proud::CNetConnectionParam::m_clientAddrAtServer;
%rename(timerCallbackIntervalMs) Proud::CNetConnectionParam::m_timerCallbackIntervalMs;
%rename(timerCallbackParallelMaxCount) Proud::CNetConnectionParam::m_timerCallbackParallelMaxCount;
%rename(slowReliableP2P) Proud::CNetConnectionParam::m_slowReliableP2P;
%rename(enableAutoConnectionRecovery) Proud::CNetConnectionParam::m_enableAutoConnectionRecovery;
%rename(userWorkerThreadModel) Proud::CNetConnectionParam::m_userWorkerThreadModel;
%rename(netWorkerThreadModel) Proud::CNetConnectionParam::m_netWorkerThreadModel;

// ikpil.choi 2017-04-12 : synthesize 기능 제거에 따른 비활성화
//%rename(publicDomainName1) Proud::CNetConnectionParam::m_publicDomainName1;
//%rename(publicDomainName2) Proud::CNetConnectionParam::m_publicDomainName2;


// rename ErrorInfo public member
%rename(errorType) Proud::ErrorInfo::m_errorType;
%rename(detailType) Proud::ErrorInfo::m_detailType;
%rename(remote) Proud::ErrorInfo::m_remote;
%rename(comment) Proud::ErrorInfo::m_comment;
%rename(source) Proud::ErrorInfo::m_source;

// rename ErrorInfo public function
%rename(GetString) Proud::ErrorInfo::ToString;

// rename CFrameMoveResult public member
%rename(processedMessageCount) Proud::CFrameMoveResult::m_processedMessageCount;
%rename(processedEventCount) Proud::CFrameMoveResult::m_processedEventCount;

// rename RmiContext public member
%rename(relayed) Proud::RmiContext::m_relayed;
%rename(sentFrom) Proud::RmiContext::m_sentFrom;
%rename(unreliableS2CRoutedMulticastMaxCount) Proud::RmiContext::m_unreliableS2CRoutedMulticastMaxCount;
%rename(unreliableS2CRoutedMulticastMaxPingMs) Proud::RmiContext::m_unreliableS2CRoutedMulticastMaxPingMs;
%rename(maxDirectP2PMulticastCount) Proud::RmiContext::m_maxDirectP2PMulticastCount;
%rename(uniqueID) Proud::RmiContext::m_uniqueID;
%rename(priority) Proud::RmiContext::m_priority;
%rename(reliability) Proud::RmiContext::m_reliability;
%rename(enableLoopback) Proud::RmiContext::m_enableLoopback;
%rename(enableP2PJitTrigger) Proud::RmiContext::m_enableP2PJitTrigger;
%rename(allowRelaySend) Proud::RmiContext::m_allowRelaySend;
%rename(forceRelayThresholdRatio) Proud::RmiContext::m_forceRelayThresholdRatio;
%rename(INTERNAL_USE_isProudNetSpecificRmi) Proud::RmiContext::m_INTERNAL_USE_isProudNetSpecificRmi;
%rename(encryptMode) Proud::RmiContext::m_encryptMode;
%rename(compressMode) Proud::RmiContext::m_compressMode;
%rename(rmiID) Proud::RmiContext::m_rmiID;
//%rename(sendFailedRemotes) Proud::RmiContext::m_sendFailedRemotes;
%ignore Proud::RmiContext::m_sendFailedRemotes; // 지금은 긴급히 고객사에게 줘야 해서....나중에 rename 하고 제대로 살리도록 하자.
%rename(fillSendFailedRemotes) Proud::RmiContext::m_fillSendFailedRemotes;

// rename CNetClientStats public member
%rename(totalTcpReceiveBytes) Proud::CNetClientStats::m_totalTcpReceiveBytes;
%rename(totalTcpSendBytes) Proud::CNetClientStats::m_totalTcpSendBytes;
%rename(totalUdpSendCount) Proud::CNetClientStats::m_totalUdpSendCount;
%rename(totalUdpSendBytes) Proud::CNetClientStats::m_totalUdpSendBytes;
%rename(totalUdpReceiveCount) Proud::CNetClientStats::m_totalUdpReceiveCount;
%rename(totalUdpReceiveBytes) Proud::CNetClientStats::m_totalUdpReceiveBytes;
%rename(remotePeerCount) Proud::CNetClientStats::m_remotePeerCount;
%rename(serverUdpEnabled) Proud::CNetClientStats::m_serverUdpEnabled;
%rename(directP2PEnabledPeerCount) Proud::CNetClientStats::m_directP2PEnabledPeerCount;

// rename CNetPeerInfo public member
%rename(udpAddrFromServer) Proud::CNetPeerInfo::m_UdpAddrFromServer;
%rename(udpAddrInternal) Proud::CNetPeerInfo::m_UdpAddrInternal;
%rename(hostID) Proud::CNetPeerInfo::m_HostID;
%rename(relayedP2P) Proud::CNetPeerInfo::m_RelayedP2P;

%rename(joinedP2PGroups) Proud::CNetPeerInfo::m_joinedP2PGroups;

%rename(isBehindNat) Proud::CNetPeerInfo::m_isBehindNat;
%rename(recentPingMs) Proud::CNetPeerInfo::m_recentPingMs;
%rename(sendQueuedAmountInBytes) Proud::CNetPeerInfo::m_sendQueuedAmountInBytes;
%rename(directP2PPeerFrameRate) Proud::CNetPeerInfo::m_directP2PPeerFrameRate;
%rename(toRemotePeerSendUdpMessageTrialCount) Proud::CNetPeerInfo::m_toRemotePeerSendUdpMessageTrialCount;
%rename(toRemotePeerSendUdpMessageSuccessCount) Proud::CNetPeerInfo::m_toRemotePeerSendUdpMessageSuccessCount;
%rename(unreliableMessageReceiveSpeed) Proud::CNetPeerInfo::m_unreliableMessageReceiveSpeed;

// rename CRemoteOfflineEventArgs public member
%rename(remoteHostID) Proud::CRemoteOfflineEventArgs::m_remoteHostID;

// rename CRemoteOnlineEventArgs public member
%rename(remoteHostID) Proud::CRemoteOnlineEventArgs::m_remoteHostID;

// rename CServerConnectionState public member
%rename(realUdpEnabled) Proud::CServerConnectionState::m_realUdpEnabled;

// rename CServerConnectionState public member
%rename(realUdpEnabled) Proud::CServerConnectionState::m_realUdpEnabled;

// rename CReceivedMessage public member
%rename(remoteHostID) Proud::CReceivedMessage::m_remoteHostID;
%rename(remoteAddr_onlyUdp) Proud::CReceivedMessage::m_remoteAddr_onlyUdp;
%rename(relayed) Proud::CReceivedMessage::m_relayed;
%rename(messageID) Proud::CReceivedMessage::m_messageID;
%rename(hasMessageID) Proud::CReceivedMessage::m_hasMessageID;
%rename(encryptMode) Proud::CReceivedMessage::m_encryptMode;
%rename(compressMode) Proud::CReceivedMessage::m_compressMode;
%rename(compressMode) Proud::CReceivedMessage::m_compressMode;

// rename CNetClientInfo public member
%rename(tcpAddrFromServer) Proud::CNetClientInfo::m_TcpAddrFromServer;
%rename(udpAddrFromServer) Proud::CNetClientInfo::m_UdpAddrFromServer;
%rename(udpAddrInternal) Proud::CNetClientInfo::m_UdpAddrInternal;
%rename(hostID) Proud::CNetClientInfo::m_HostID;
%rename(relayedP2P) Proud::CNetClientInfo::m_RelayedP2P;
%rename(isBehindNat) Proud::CNetClientInfo::m_isBehindNat;
%rename(realUdpEnabled) Proud::CNetClientInfo::m_realUdpEnabled;
%rename(natDeviceName) Proud::CNetClientInfo::m_natDeviceName;
%rename(recentPingMs) Proud::CNetClientInfo::m_recentPingMs;
%rename(sendQueuedAmountInBytes) Proud::CNetClientInfo::m_sendQueuedAmountInBytes;
%rename(recentFrameRate) Proud::CNetClientInfo::m_recentFrameRate;
%rename(toServerSendUdpMessageTrialCount) Proud::CNetClientInfo::m_toServerSendUdpMessageTrialCount;
%rename(toServerSendUdpMessageSuccessCount) Proud::CNetClientInfo::m_toServerSendUdpMessageSuccessCount;
%rename(hostIDRecycleCount) Proud::CNetClientInfo::m_hostIDRecycleCount;
%rename(unreliableMessageRecentReceiveSpeed) Proud::CNetClientInfo::m_unreliableMessageRecentReceiveSpeed;
%rename(unreliableMessageRecentReceiveSpeed) Proud::CNetClientInfo::m_unreliableMessageRecentReceiveSpeed;

// rename CNetServerStats public member

%rename(p2pDirectConnectionPairCount) Proud::CNetServerStats::m_p2pDirectConnectionPairCount;
%rename(clientCount) Proud::CNetServerStats::m_clientCount;
%rename(realUdpEnabledClientCount) Proud::CNetServerStats::m_realUdpEnabledClientCount;
%rename(occupiedUdpPortCount) Proud::CNetServerStats::m_occupiedUdpPortCount;

// rename CStartServerParameter public member
%rename(tcpPorts) Proud::CStartServerParameter::m_tcpPorts;
%rename(udpPorts) Proud::CStartServerParameter::m_udpPorts;
%rename(udpAssignMode) Proud::CStartServerParameter::m_udpAssignMode;
%rename(enableIocp) Proud::CStartServerParameter::m_enableIocp;
%rename(upnpDetectNatDevice) Proud::CStartServerParameter::m_upnpDetectNatDevice;
%rename(upnpTcpAddPortMapping) Proud::CStartServerParameter::m_upnpTcpAddPortMapping;
%rename(usingOverBlockIcmpEnvironment) Proud::CStartServerParameter::m_usingOverBlockIcmpEnvironment;
%rename(clientEmergencyLogMaxLineCount) Proud::CStartServerParameter::m_clientEmergencyLogMaxLineCount;
%rename(enablePingTest) Proud::CStartServerParameter::m_enablePingTest;
%rename(ignoreFailedBindPort) Proud::CStartServerParameter::m_ignoreFailedBindPort;
%rename(failedBindPorts) Proud::CStartServerParameter::m_failedBindPorts;
%rename(tunedNetworkerSendIntervalMs_TEST) Proud::CStartServerParameter::m_tunedNetworkerSendIntervalMs_TEST;
%rename(simplePacketMode) Proud::CStartServerParameter::m_simplePacketMode;
%rename(hostIDGenerationPolicySimplePacketMode) Proud::CStartServerParameter::m_hostIDGenerationPolicy;

// rename CStartServerParameterBase public member
%rename(serverAddrAtClient) Proud::CStartServerParameterBase::m_serverAddrAtClient;
%rename(localNicAddr) Proud::CStartServerParameterBase::m_localNicAddr;
%rename(protocolVersion) Proud::CStartServerParameterBase::m_protocolVersion;
%rename(threadCount) Proud::CStartServerParameterBase::m_threadCount;
%rename(netWorkerThreadCount) Proud::CStartServerParameterBase::m_netWorkerThreadCount;
%rename(encryptedMessageKeyLength) Proud::CStartServerParameterBase::m_encryptedMessageKeyLength;
%rename(fastEncryptedMessageKeyLength) Proud::CStartServerParameterBase::m_fastEncryptedMessageKeyLength;
%rename(enableP2PEncryptedMessaging) Proud::CStartServerParameterBase::m_enableP2PEncryptedMessaging;
%rename(allowServerAsP2PGroupMember) Proud::CStartServerParameterBase::m_allowServerAsP2PGroupMember;
%rename(timerCallbackIntervalMs) Proud::CStartServerParameterBase::m_timerCallbackIntervalMs;
%rename(timerCallbackParallelMaxCount) Proud::CStartServerParameterBase::m_timerCallbackParallelMaxCount;
%rename(enableNagleAlgorithm) Proud::CStartServerParameterBase::m_enableNagleAlgorithm;
%rename(reservedHostIDFirstValue) Proud::CStartServerParameterBase::m_reservedHostIDFirstValue;
%rename(reservedHostIDCount) Proud::CStartServerParameterBase::m_reservedHostIDCount;
%rename(allowExceptionEvent) Proud::CStartServerParameterBase::m_allowExceptionEvent;
%rename(enableEncryptedMessaging) Proud::CStartServerParameterBase::m_enableEncryptedMessaging;


// rename SuperPeerRating public member
%rename(hostID) Proud::SuperPeerRating::m_hostID;
%rename(rating) Proud::SuperPeerRating::m_rating;
%rename(realUdpEnabled) Proud::SuperPeerRating::m_realUdpEnabled;
%rename(isBehindNat) Proud::SuperPeerRating::m_isBehindNat;
%rename(recentPingMs) Proud::SuperPeerRating::m_recentPingMs;
%rename(p2pGroupTotalRecentPingMs) Proud::SuperPeerRating::m_P2PGroupTotalRecentPingMs;
%rename(sendSpeed) Proud::SuperPeerRating::m_sendSpeed;
%rename(joinedTime) Proud::SuperPeerRating::m_JoinedTime;
%rename(frameRate) Proud::SuperPeerRating::m_frameRate;

// rename CNetCoreStats public member
%rename(totalTcpReceiveCount) Proud::CNetCoreStats::m_totalTcpReceiveCount;
%rename(totalTcpReceiveBytes ) Proud::CNetCoreStats::m_totalTcpReceiveBytes;
%rename(totalTcpSendCount) Proud::CNetCoreStats::m_totalTcpSendCount;
%rename(totalTcpSendBytes) Proud::CNetCoreStats::m_totalTcpSendBytes;
%rename(totalUdpSendCount) Proud::CNetCoreStats::m_totalUdpSendCount;
%rename(totalUdpSendBytes) Proud::CNetCoreStats::m_totalUdpSendBytes;
%rename(totalUdpReceiveCount) Proud::CNetCoreStats::m_totalUdpReceiveCount;
%rename(totalUdpReceiveBytes) Proud::CNetCoreStats::m_totalUdpReceiveBytes;
%rename(p2pConnectionPairCount) Proud::CNetCoreStats::m_p2pConnectionPairCount;

// rename CP2PGroup public member
%rename(groupHostID) Proud::CP2PGroup::m_groupHostID;
%rename(members) Proud::CP2PGroup::m_members;