/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

ProudNet

此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。

ProudNet
このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

**注意　：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

#include "../include/FastList.h"
#include "../include/FastMap.h"
#include "RCPair.h"
#include "ISendDest_C.h"
#include "NetCore.h"
#include "P2PPair.h"
#include "IVizAgentDg.h"
#include "P2PGroup_S.h"
#include "Relayer.h"
#include "HostIDFactory.h"
#include "RemoteClient.h"
#include "LoopbackHost.h"
#include "LeanDynamicCast.h"
#include "HlaSessionHostImpl_S.h"
#include "FinalUserWorkItem.h"
#include "LocalEvent.h"
#include "TimeAlarm.h"


using namespace Proud;
#include "NetC2S_stub.h"
#include "NetS2C_proxy.h"
#include "CachedTimes.h"
#include "SendDestInfo.h"
#include "P2PPair_S.h"

namespace Proud
{
	class CListenerThread_S;
	class P2PGroup_MemberJoin_AckKey;
	class MessageSummary;

#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	class CNetServerImpl :
		public CNetCoreImpl, 
#ifdef USE_HLA
		public IHlaSessionHostImplDelegate_S,
#endif
		public Proud::CNetServer,
		public CP2PGroupMemberBase_S,
		public IVizAgentDg,
		public IThreadPoolEvent
	{
		friend class CSuperSocket;

		// 소스코드 여기저기서 main lock이라 말하는 것은 바로 이것이다.
		CriticalSection m_cs;

		// Start/Stop 함수가 여러 스레드에서 동시에 호출되더라도 단일화하기 위한 목적
		CriticalSection m_startStopCS;		

#ifndef PROUDSRCCOPY
		void* m_lic;
#endif
		CGlobalTimerThread::PtrType m_globalTimer;

	public:
		virtual CriticalSection& GetCriticalSection() PN_OVERRIDE{return m_cs; }

		//CCachedTimes m_cachedTimes;

		inline int64_t GetCachedServerTimeMs()
		{
			return GetPreciseCurrentTimeMs();
			//return m_cachedTimes.GetTimeMs();
		}

		// 파라메터로 받은 서버의 로컬 IP
		// 서버가 공유기나 L4 스위치 뒤에 있는 경우 유용
		String m_serverAddrAlias;

		// 서버가 사용할 NIC. 어떤 회사는 여러 NIC 중 하나를 지정해서 그것으로만 게임 클라와의 통신을 한다.
		// 비어있으면, 모든 NIC을 사용하겠음을 의미한다. 
		String m_localNicAddr;

		// 엔진 버전
		int m_internalVersion;

//		uint32_t m_timerCallbackInterval;
//		void*  m_timerCallbackContext;
		
		// 이 서버가 실행되는 환경에서, ICMP 패킷이 도착하는 호스트로부터의 통신을 완전 차단하는
		// 환경이 적용되어 있는지?
		// false이면 안전하게 TTL 을 수정한 패킷 교환이 가능

		bool m_usingOverBlockIcmpEnvironment;

		// 이 객체 인스턴스의 GUID. 이 값은 전세계 어느 컴퓨터에서도 겹치지 않는 값이어야 한다.
		// 같은 호스트에서 여러개의 서버를 실행시 p2p 홀펀칭 시도중 hostID, peer addr만 갖고는 어느 서버와 연결된 것에 대한 것인지
		// 구별이 불가능하므로 이 기능을 쓰는 것이다.
		Guid m_instanceGuid;

		// P2P 통신중인 클라이언트끼리의 리스트(직접,간접 포함)
		// 루프백 P2P (클라A-클라A)도 포함
		CP2PPairList m_p2pConnectionPairList;

		// UDP 주소->클라 객체 인덱스

		// UDP 수신시 고속 검색을 위해 필요하다.
// 		class CUdpAddrPortToRemoteClientIndex : public CFastMap2<AddrPort, CRemoteClient_S*,int>
// 		{
// 		public:
// 			CUdpAddrPortToRemoteClientIndex();
// 		};
// 		CUdpAddrPortToRemoteClientIndex m_UdpAddrPortToRemoteClientIndex;

		struct P2PGroupSubset_S
		{
			HostID m_groupID;
			HostIDArray m_excludeeHostIDList;
		};

		// TCP listening을 하고 있는 중인지?
		// false이면 더 이상의 연결을 받는 처리를 안한다.
		bool m_listening;

		//이 변수들과 변수를 사용하는 부분을 모두 CNetCoreImpl로 옮기자. 같은 이름의 변수가 NS,NC,LS,LC에 있는데 마찬가지로.
// 		PROUDNET_VOLATILE_ALIGN bool m_netThreadPoolUnregistered;
// 		PROUDNET_VOLATILE_ALIGN bool m_userThreadPoolUnregistered;
		//여기까지.

		Guid m_protocolVersion;

		// 서버에 접속되어 있는 클라들의 P2P 그룹 목록
		P2PGroups_S m_P2PGroups;
		
		// 접속 들어오는 client의 hostID를 발급해주는 역할
		CHeldPtr<CHostIDFactory> m_HostIDFactory;

		CLoopbackHost* m_loopbackHost;

		class C2SStub : public ProudC2S::Stub
		{
		public:
			CNetServerImpl *m_owner;
			
			DECRMI_ProudC2S_P2PGroup_MemberJoin_Ack;
			DECRMI_ProudC2S_NotifyP2PHolepunchSuccess;
			DECRMI_ProudC2S_P2P_NotifyDirectP2PDisconnected;
			DECRMI_ProudC2S_NotifyUdpToTcpFallbackByClient;
			DECRMI_ProudC2S_ReliablePing;
			DECRMI_ProudC2S_ShutdownTcp;
			DECRMI_ProudC2S_NotifyLog;
			DECRMI_ProudC2S_NotifyLogHolepunchFreqFail;
			DECRMI_ProudC2S_NotifyNatDeviceName;
			DECRMI_ProudC2S_NotifyJitDirectP2PTriggered;
			DECRMI_ProudC2S_NotifyNatDeviceNameDetected;
			DECRMI_ProudC2S_NotifySendSpeed;	
			DECRMI_ProudC2S_ReportP2PPeerPing;
			DECRMI_ProudC2S_C2S_RequestCreateUdpSocket;
			DECRMI_ProudC2S_C2S_CreateUdpSocketAck;
			DECRMI_ProudC2S_ReportC2CUdpMessageCount;
			DECRMI_ProudC2S_ReportC2SUdpMessageTrialCount;
		};
		class Proxy : public ProudS2C::Proxy
		{
		};

		C2SStub m_c2sStub;
		Proxy m_s2cProxy;
	public:
		CNetServerStats m_stats;
		
		// OnMessageSend는 겨우 stats update만 한다.
		// 이것때문에 main lock 걸면 오버다.
		// 그래서 이것이 따로 있다.
		CriticalSection m_statsCritSec;
		
		// remote client들이 공유하는 UDP socket들. strong pointer(직접 소유)다.
		// static assigned mode를 위함.
		// NOTE: per-remote UDP socket은 Proud.CRemoteClient_S.m_ownedUdpSocket가 소유한다.
		CFastArray<CSuperSocketPtr> m_staticAssignedUdpSockets;
		
		CFastMap2<AddrPort,CSuperSocketPtr,int> m_localAddrToUdpSocketMap; // UDP socket의 local 주소 -> 소켓 객체

		// CSuperSocket를 쓰는 이유: issue state 등을 갖고 있는 완전체니까. 그리고 accept pend socket을 갖고 있으니까.
		// TCP listen socket을 여러개 할수 있도록 수정함.
		CFastArray<CSuperSocketPtr> m_TcpListenSockets;

		//CHeldPtr<CMilisecTimer> m_timer;
		//CHeldPtr<CompletionPort> m_completionPort;
		//CHeldPtr<CompletionPort> m_TcpAcceptCompletionPort;

		// Proud.CNetConfig.EveryRemoteIssueSendOnNeedIntervalMs이지만 테스트를 위해 값을 수정할 경우를 위해 멤버로 떼놨다.
		int m_everyRemoteIssueSendOnNeedIntervalMs;

		/*NOTE: send ready list, task subject list는 
		각 main 즉 IThreadReferrer가 갖고 있어도 되겠다.
		어차피 custom value event는 main 단위로 가고, epoll에서
		여러 스레드가 동시에 같은 socket에 대해서 non block send를 해도 되니까.
		*/

		/* IssueSend를 걸어야 하는 것들
		send queue에 뭔가가 들어가는 순간, issue send = F인 것들이 여기에 등록된다.
		issue send = T가 되는 순간 여기서 빠진다.
		UDP 소켓을 굉장히 많이 둔 경우 모든 UDP socket에 대해 루프를 돌면 비효율적이므로 이것이 필요 */
// 		CSendReadyList* m_sendReadyList; 
// 		//이 변수와 이 변수를 다루는 함수들 (SendReadyList_Add,
// 		//SendReadyList에 대한 생성/파괴)을 CNetCoreImpl로 옮기자.

		
		/* 위 m_sendReadyList는 main lock 없이 접근된다. 
		따라서 m_sendReadyList로부터 얻은 IHostObject는 무효할 수 있다.
		이것의 유효성 검사는 main lock 상태에서 해야 한다. 그것을 위한 목록이다.
		이 목록에는 TCP socket들, UDP socket들이 들어간다. per-remote UDP socket, per-remote TCP socket, static assigned UDP socket들이 들어간다. 즉 인덱스인 셈이다. 
		m_sendReadyList에 있더라도 이 목록에 없으면 무효다.
		이 목록에 있는 객체들은 실제로 파괴되기 직전까지 항상 들어있는다. 즉 send ready 조건을 만족할 때만 들어가는 것이 아님.
		이 목록에 접근할 때는 main lock일 때만 해야 한다.
		*/
		// 이 변수는 CSuperSocket*를 key로 가져야 한다. 
		//CFastMap2<CSuperSocket*, int, int> m_validSendReadyCandidates;
		
		// 루프백 메시지용 키
		CSessionKey m_selfSessionKey;
		CryptCount m_selfEncryptCount, m_selfDecryptCount;

		// aes 키를 교환하기 위한 rsa key
		CCryptoRsaKey m_selfXchgKey;

		// 공개키는 미리 blob 으로 변환해둔다.
		ByteArray m_publicKeyBlob;

		PROUDNET_VOLATILE_ALIGN int32_t m_heartbeatWorking;
	
		void Heartbeat();
		void Heartbeat_One();

		CTimeAlarm m_PurgeTooOldUnmatureClient_Timer;
		CTimeAlarm m_PurgeTooOldAddMemberAckItem_Timer;
		//m_disposeGarbagedHosts_Timer,m_DisconnectRemotePeerOnTimeout_Timer,m_acceptedPeerdispose_Timer,m_lanRemotePeerHeartBeat_Timer는 NetCore로 옮겨도 될 것 같죠?
		// 당연히, 이들 객체를 초기화하는 부분들도 netcore로 옮기셔야.
		//CTimeAlarm m_disposeGarbagedHosts_Timer;
		CTimeAlarm m_electSuperPeer_Timer;
		CTimeAlarm m_removeTooOldUdpSendPacketQueue_Timer;
		//CTimeAlarm m_DisconnectRemoteClientOnTimeout_Timer;
		CTimeAlarm m_removeTooOldRecyclePair_Timer;
		CTimeAlarm m_GarbageTooOldRecyclableUdpSockets_Timer;

//		PROUDNET_VOLATILE_ALIGN LONG m_issueSendOnNeedWorking;

//		void EveryRemote_IssueSendOnNeed(CNetworkerLocalVars& localVars);
#if defined (_WIN32)
		void StaticAssignedUdpSockets_IssueFirstRecv();
		void TcpListenSockets_IssueFirstAccept();
#endif // _WIN32

		void ShowThreadExceptionAndPurgeClient(CRemoteClient_S* client, const PNTCHAR* where, const PNTCHAR* reason);

		void ProcessMessageOrMoveToFinalRecvQueue( SocketType socktype, CSuperSocket* socket, CReceivedMessageList &extractedMessages) PN_OVERRIDE;
		bool ProcessMessage_ProudNetLayer( CriticalSectionLock* mainLocker, CReceivedMessage &receivedInfo, CRemoteClient_S *rc , CSuperSocket* udpSocket);
		bool ProcessMessage_FromUnknownClient( AddrPort from, CMessage& recvedMsg, CSuperSocket *udpSocket );

		void IoCompletion_ProcessMessage_ServerHolepunch( CMessage &msg, AddrPort from, CSuperSocket *udpSocket );
		void IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch( CMessage &msg, AddrPort from, CSuperSocket *udpSocket );
		void IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc);
		void IoCompletion_ProcessMessage_UnreliablePing( CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_SpeedHackDetectorPing( CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_UnreliableRelay1( CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc);
		void IoCompletion_ProcessMessage_ReliableRelay1( CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_LingerDataFrame1( CriticalSectionLock* mainLocker, CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_NotifyHolepunchSuccess( CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess( CMessage &msg, CRemoteClient_S *rc );
		void IoCompletion_ProcessMessage_Rmi( CReceivedMessage &receivedInfo, bool messageProcessed, CRemoteClient_S * rc, bool isRealUdp );
		void IoCompletion_ProcessMessage_UserOrHlaMessage( CReceivedMessage &receivedInfo, FinalUserWorkItemType UWIType, bool messageProcessed, CRemoteClient_S * rc, bool isRealUdp );
		void IoCompletion_MulticastUnreliableRelay2_AndUnlock( CriticalSectionLock* mainLocker, const HostIDArray &relayDest, HostID relaySenderHostID, CMessage &payload, MessagePriority priority, int64_t uniqueID );

		// 해당 메시지가 와도 아무것도 하지않는다.
		//void IoCompletion_ProcessMessage_PolicyRequest(CRemoteClient_S *rc);

		void NotifyProtocolVersionMismatch( CRemoteClient_S * rc );
		//add by rekfkno1 - END

		virtual void OnThreadBegin() PN_OVERRIDE;
		virtual void OnThreadEnd() PN_OVERRIDE;
		//add by rekfkno1 - END

		void SetEventSink(INetServerEvent *eventSink);

#ifdef SUPPORTS_LAMBDA_EXPRESSION
		void Set_OnClientJoin(OnClientJoin_ParamType* handler);
		void Set_OnClientLeave(std::function<void(CNetClientInfo *, ErrorInfo *, const ByteArray&)> handler);
		
		std::shared_ptr<LambdaBase_Param1<void,CNetClientInfo*>> m_event_OnClientJoin;
		std::function<void(CNetClientInfo *, ErrorInfo *, const ByteArray&)> m_event_OnClientLeave;
#endif // SUPPORTS_LAMBDA_EXPRESSION

		void Start(CStartServerParameter &param);
		void Start(CStartServerParameter &param, ErrorInfoPtr& outError);

		void Stop();

	private:
		void StaticAssignedUdpSockets_Clear();

		// Caller(CNetServerImpl::Start함수)에서 outError를 셋팅하므로 불필요.
		SocketErrorCode CreateTcpListenSocketsAndInit(CFastArray<int> tcpPorts);// , ErrorInfoPtr &outError);
		SocketErrorCode CreateAndInitUdpSockets(ServerUdpAssignMode udpAssignMode,
												CFastArray<int> udpPorts, 
												CFastArray<int> &failedBindPorts, 
												CFastArray<CSuperSocketPtr>& outCreatedUdpSocket);// , ErrorInfoPtr &outError);

		virtual bool AsyncCallbackMayOccur();
		void BadAllocException(Exception& ex);
	public:

		Random m_random;


		/** clientHostID가 가리키는 peer가 참여하고 있는 P2P group 리스트를 얻는다. */
		bool GetJoinedP2PGroups(HostID clientHostID, HostIDArray &output);

		CRemoteClient_S* GetRemoteClientBySocket_NOLOCK(CSuperSocket* socket, const AddrPort& remoteAddr);

		void PurgeTooOldAddMemberAckItem();

		void PurgeTooOldUnmatureClient();

		CSuperSocketPtr GetAnyUdpSocket();
		void RemoteClient_New_ToClientUdpSocket(CRemoteClient_S* rc);

		void GetP2PGroups(CP2PGroups &ret);
		int GetP2PGroupCount();
		
		void OnSocketWarning(CSuperSocket *s, String msg);

		////////////////////////////
		// CNetCoreImpl 구현

		void OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/);
		void OnMessageReceived(int doneBytes, SocketType socketType, CReceivedMessageList& messages, CSuperSocket* socket);
		
		void ProcessDisconnecting(CSuperSocket* socket, const ErrorInfo& errorInfo) PN_OVERRIDE;

		void OnAccepted(CSuperSocket* socket, AddrPort tcpLocalAddr, AddrPort tcpRemoteAddr) PN_OVERRIDE;

		void OnConnectSuccess(CSuperSocket* socket) PN_OVERRIDE;
		void OnConnectFail(CSuperSocket* socket, SocketErrorCode code) PN_OVERRIDE;

		//아래 함수도 CNetCoreImpl로 옮기자.
		//void SendReadyList_Add(CSuperSocket* socket);
		
		virtual void OnHostGarbaged(CHostBase* remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode);
		virtual bool OnHostGarbageCollected(CHostBase *remote) PN_OVERRIDE;
		virtual void OnSocketGarbageCollected(CSuperSocket* socket) PN_OVERRIDE;

		virtual void AddEmergencyLogList(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction = _PNT(""), int logLine = 0) {}
		//////////////////////////////////////////////////////////////////////////
		// IThreadReferrer 구현
		void OnCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue) PN_OVERRIDE;

		bool Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult) PN_OVERRIDE;

		bool IsValidHostID(HostID id);
		bool IsValidHostID_NOLOCK( HostID id );

		void EnqueueClientLeaveEvent( CRemoteClient_S *rc, ErrorType errorType, ErrorType detailType, const ByteArray& comment ,SocketErrorCode socketErrorCode);
		virtual void ProcessOneLocalEvent(LocalEvent& e, CHostBase* subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult) PN_OVERRIDE;

		CNetServerImpl();
		~CNetServerImpl();
	private:
		CHostBase* GetTaskSubjectByHostID_NOLOCK(HostID subjectHostID);
	public:
		inline void AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}

		inline void AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}
	private:
#ifdef _DEBUG
		void CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname);
#endif // _DEBUG
	public:
		INetCoreEvent *GetEventSink_NOCSLOCK() PN_OVERRIDE;

		CSessionKey *GetCryptSessionKey(HostID remote, String &errorOut, bool& outEnqueError);

		bool NextEncryptCount(HostID remote, CryptCount &output);
		void PrevEncryptCount(HostID remote);
		bool GetExpectedDecryptCount(HostID remote, CryptCount &output);
		bool NextDecryptCount(HostID remote);

		virtual bool Send(const CSendFragRefs &sendData,
			const SendOpt& sendContext,
			const HostID *sendTo, int numberOfsendTo ,int &compressedPayloadLength) PN_OVERRIDE;
	public:
		bool Send_BroadcastLayer(const CSendFragRefs& payload, const CSendFragRefs* encryptedPayload, const SendOpt& sendContext,const HostID* sendTo, int numberOfsendTo);

		void ConvertGroupToIndividualsAndUnion(int numberOfsendTo, const HostID * sendTo, HostIDArray &sendDestList);

		virtual bool CloseConnection(HostID clientHostID);
		virtual void CloseEveryConnection();
		void RequestAutoPrune(CRemoteClient_S* rc);

		virtual bool SetHostTag(HostID hostID, void* hostTag);
		void* GetHostTag_NOLOCK(HostID hostID);

//		CRemoteClient_S *GetRemoteClientByHostID_NOLOCK(HostID clientHostID);
		CRemoteClient_S *GetAuthedClientByHostID_NOLOCK(HostID clientHostID);
		
		int GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL);
		int GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL);
		virtual int GetP2PRecentPingMs(HostID HostA,HostID HostB) PN_OVERRIDE;

		bool DestroyP2PGroup(HostID groupHostID);

		bool LeaveP2PGroup(HostID memberHostID, HostID groupHostID);
		HostID CreateP2PGroup( const HostID* clientHostIDList0, int count, ByteArray customField,CP2PGroupOption &option,HostID assignedHostID);
		bool JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray customField);

		void EnqueAddMemberAckCompleteEvent( HostID groupHostID, HostID addedMemberHostID ,ErrorType result);
	private:
		int m_joinP2PGroupKeyGen;

		bool JoinP2PGroup_Internal(HostID newMemberHostID, HostID groupHostID, ByteArray customField, int joinP2PGroupKeyGen);
		void AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent( CP2PGroup_S* group,HostID memberHostID , ErrorType reason);

	public:
		void P2PGroup_CheckConsistency();
		void P2PGroup_RefreshMostSuperPeerSuitableClientID(CP2PGroup_S* group);
		void P2PGroup_RefreshMostSuperPeerSuitableClientID(CRemoteClient_S* rc);
		void DestroyEmptyP2PGroups();

		int GetClientHostIDs(HostID* ret, int count);

		bool GetClientInfo(HostID clientHostID, CNetClientInfo &ret);

		CP2PGroupPtr_S GetP2PGroupByHostID_NOLOCK(HostID groupHostID);
		bool GetP2PGroupInfo(HostID groupHostID, CP2PGroup &output);
		void ConvertAndAppendP2PGroupToPeerList(HostID sendTo, HostIDArray &sendTo2);

		CHostBase *GetSendDestByHostID_NOLOCK(HostID peerHostID);

		HostID GetHostID() PN_OVERRIDE
		{
			return HostID_Server;
		}

		bool GetP2PConnectionStats(HostID remoteHostID,CP2PConnectionStats& status);
		bool GetP2PConnectionStats(HostID remoteA, HostID remoteB, CP2PPairConnectionStats& status);

	//	bool IsFinalReceiveQueueEmpty();

		//void RemoteClient_RemoveFromCollections( CRemoteClient_S * rc );
		void EnqueErrorOnIssueAcceptFailed(SocketErrorCode e);
		void EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient();
		void HardDisconnect_AutoPruneGoesTooLongClient(CRemoteClient_S* rc);

		void ElectSuperPeer();

		int64_t GetTimeMs() PN_OVERRIDE;


		String DumpGroupStatus();

		int GetClientCount();
		void GetStats(CNetServerStats &outVal);

		inline void AttachProxy(IRmiProxy *proxy)
		{
			CNetCoreImpl::AttachProxy(proxy);
		}
		inline void AttachStub(IRmiStub *stub)
		{
			CNetCoreImpl::AttachStub(stub);
		}
		inline void DetachProxy(IRmiProxy *proxy)
		{
			CNetCoreImpl::DetachProxy(proxy);
		}
		inline void DetachStub(IRmiStub *stub)
		{
			CNetCoreImpl::DetachStub(stub);
		}
		
#ifdef USE_HLA
		void HlaFrameMove();
		void HlaAttachEntityTypes(CHlaEntityManagerBase_S* entityManager);
		void HlaSetDelegate(IHlaDelegate_S* dg);
		CHlaSpace_S* HlaCreateSpace();
		CHlaEntity_S* HlaCreateEntity(HlaClassID classID);
		void HlaDestroySpace(CHlaSpace_S* space);		
		void HlaDestroyEntity(CHlaEntity_S* Entity);
		void HlaViewSpace(HostID viewerID, CHlaSpace_S* space, double levelOfDetail);
		void HlaUnviewSpace(HostID viewerID, CHlaSpace_S* space);

	private:
		CHeldPtr<CHlaSessionHostImpl_S> m_hlaSessionHostImpl;
#endif
	public:

		void ShowError_NOCSLOCK( ErrorInfoPtr errorInfo );
		void ShowWarning_NOCSLOCK( ErrorInfoPtr errorInfo);

		inline void ShowNotImplementedRmiWarning(const PNTCHAR* RMIName)
		{
			CNetCoreImpl::ShowNotImplementedRmiWarning(RMIName);
		}

		inline void PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName)
		{
			CNetCoreImpl::PostCheckReadMessage( msg, RMIName);
		}

		void GetRemoteIdentifiableLocalAddrs(CFastArray<NamedAddrPort> &output);
		// remote client의, 외부 인터넷에서도 인식 가능한 TCP 연결 주소를 리턴한다.
		NamedAddrPort GetRemoteIdentifiableLocalAddr(CRemoteClient_S* rc);

		void SetDefaultTimeoutTimeMs(int newValInMs);
		void SetDefaultTimeoutTimeSec(double newValInSec);

//		void SetTimerCallbackIntervalMs(int newVal);

		bool SetDirectP2PStartCondition(DirectP2PStartCondition newVal);

		// 너무 오랫동안 쓰이지 않은 UDP 송신 큐를 찾아서 제거한다.
		void DoForLongInterval();
	
	public:

		//void DisconnectRemoteClientOnTimeout();
		void Heartbeat_EveryRemoteClient();

		void GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output);
		void GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output);
	private:
		void FallbackServerUdpToTcpOnNeed( CRemoteClient_S * rc, int64_t currTime );
		void ArbitraryUdpTouchOnNeed( CRemoteClient_S * rc, int64_t currTime );
		void RefreshSendQueuedAmountStat(CRemoteClient_S *rc);
	public:
		//void PruneTooOldDefragBoardOnNeed();
		void SecondChanceFallbackUdpToTcp(CRemoteClient_S* rc);

		virtual void GetTcpListenerLocalAddrs(CFastArray<AddrPort> &output);
		virtual void GetUdpListenerLocalAddrs(CFastArray<AddrPort> &output);
		
		// 정렬을 하기위해 P2PConnectionState 과 배열의 index와 directP2P Count로 구성된 구조체 리스트를 만든다.
		struct ConnectionInfo
		{
			P2PConnectionState* state;
			int hostIndex0, hostIndex1;
			int connectCount0,connectCount1;

			void Clear()
			{
				state = NULL; hostIndex0 = hostIndex1 = 0;
				connectCount0 = 0; connectCount1 = 0;
			}
			inline bool operator < (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs < rhs.state->m_recentPingMs;
			}
			inline bool operator <= (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs <= rhs.state->m_recentPingMs;
			}
			inline bool operator >(const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs > rhs.state->m_recentPingMs;
			}
			inline bool operator >= (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs >= rhs.state->m_recentPingMs;
			}
		};
	private:
		CFastArray<ConnectionInfo> m_connectionInfoList;
		CFastArray<int, false, true> m_routerIndexList;
	public:
		void RemoteClient_RequestStartServerHolepunch_OnFirst( CRemoteClient_S* rc );
		void RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed( CRemoteClient_S* rc);

		void MakeP2PRouteLinks(SendDestInfoArray &tgt, int unreliableS2CRoutedMulticastMaxCount, int64_t routedSendMaxPing);

		void Convert_NOLOCK(SendDestInfoArray& dest, HostIDArray& src);

		virtual void SetDefaultFallbackMethod(FallbackMethod value);

		CHeldPtr<CLogWriter> m_logWriter;

		virtual void EnableLog(const PNTCHAR* logFileName);
		virtual void DisableLog();

//		void EnqueueHackSuspectEvent(CRemoteClient_S* rc,const char* statement,HackType hackType);
		void EnqueueP2PGroupRemoveEvent( HostID groupHostID );

// 		double GetSpeedHackLongDetectMinInterval();
// 		double GetSpeedHackLongDeviationRatio();
// 		double GetSpeedHackDeviationThreshold();
// 		int GetSpeedHackDetectorSeriesLength();

		int m_SpeedHackDetectorReckRatio; // 0~100

		virtual void SetSpeedHackDetectorReckRatioPercent(int newValue);
		virtual bool EnableSpeedHackDetector(HostID clientID, bool enable);

		virtual void SetMessageMaxLength(int value_s, int value_c);

		virtual bool IsConnectedClient(HostID clientHostID);

		virtual void TEST_SomeUnitTests();

		void TestUdpConnReset();
#if defined (_WIN32)
		// $$$; == > 본 작업이 완료되면 반드시 #ifdef를 제거해주시기 바랍니다.
		// dev003에서 흘러온 것임. 승철씨 선에서 해결되어야. 
		void TestReaderWriterMonitor();
#endif
		virtual void SetMaxDirectP2PConnectionCount(HostID clientID, int newVal);
		virtual HostID GetMostSuitableSuperPeerInGroup(HostID groupID, const CSuperPeerSelectionPolicy& policy, const HostID* excludees, intptr_t excludeesLength);
		virtual HostID GetMostSuitableSuperPeerInGroup( HostID groupID, const CSuperPeerSelectionPolicy& policy, HostID excludee );

		virtual int GetSuitableSuperPeerRankListInGroup(HostID groupID, SuperPeerRating* ratings, int ratingsBufferCount, const CSuperPeerSelectionPolicy& policy, const CFastArray<HostID> &excludees);
	private:
		void TouchSuitableSuperPeerCalcRequest( CP2PGroup_S* group, const CSuperPeerSelectionPolicy &policy );
	public:

		virtual void GetUdpSocketAddrList(CFastArray<AddrPort> &output);

		virtual int GetInternalVersion();

		void TestAddrPort();
		void TestStringW();
		void TestStringA();
		void TestFastArray();

		void EnqueueUnitTestFailEvent(String msg);
		virtual void EnqueError(ErrorInfoPtr info);
		virtual void EnqueWarning(ErrorInfoPtr info);

		void EnqueClientJoinApproveDetermine(CRemoteClient_S *rc, const ByteArray& request);
		void ProcessOnClientJoinApproved(CRemoteClient_S *rc, const ByteArray &response);
		void ProcessOnClientJoinRejected(CRemoteClient_S *rc, const ByteArray &response);
	private:
		// true시 빈 P2P 그룹도 허용함

		bool m_allowEmptyP2PGroup;
		bool m_startCreateP2PGroup;
	public:
		virtual void AllowEmptyP2PGroup(bool enable);
	private:
		//void TestReliableUdpFrame();
		void TestSendBrake();

		String GetConfigString();
	public:
		void EnquePacketDefragWarning(CSuperSocket* superSocket, AddrPort sender, const PNTCHAR* text);

	private:
		int GetMessageMaxLength();
		virtual HostID GetVolatileLocalHostID() const PN_OVERRIDE;
		virtual HostID GetLocalHostID() PN_OVERRIDE;
		virtual bool IsNagleAlgorithmEnabled();

		//double m_lastPruneTooOldDefragBoardTime;

		int m_freqFailLogMostRank;
		String m_freqFailLogMostRankText;
		int64_t m_lastHolepunchFreqFailLoggedTime;
		PROUDNET_VOLATILE_ALIGN int32_t m_fregFailNeed;
		
	public:		
		// 종료시에만 설정됨
		// 2010.07.20 delete by ulelio :  Stop()이 return하기 전까지는 이벤트 콜백이 있어야하므로 삭제한다.
		//PROUDNET_VOLATILE_ALIGN bool m_shutdowning;

		bool SendUserMessage(const HostID* remotes, int remoteCount, const RmiContext &rmiContext, uint8_t* payload, int payloadLength)
		{
			return CNetCoreImpl::SendUserMessage(remotes,remoteCount,rmiContext,payload,payloadLength);
		}

		virtual ErrorType GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent);

		virtual ErrorType SetCoalesceIntervalMs(HostID remote, int intervalMs);
		virtual ErrorType SetCoalesceIntervalToAuto(HostID remote);

	public:
		void LogFreqFailOnNeed();
		void LogFreqFailNow();
	
		virtual void TEST_SetOverSendSuspectingThresholdInBytes(int newVal);
		virtual void TEST_SetTestping(HostID hostID0, HostID hostID1, int ping);

		bool m_forceDenyTcpConnection_TEST;
		virtual void TEST_ForceDenyTcpConnection();

		ServerUdpAssignMode m_udpAssignMode;
		
#ifdef VIZAGENT
		void EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey) PN_OVERRIDE;
#endif
	private:
		virtual CNetClientImpl* QueryNetClient() { return NULL; }
		virtual CNetServerImpl* QueryNetServer() { return this; }

#ifdef VIZAGENT
		virtual void Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext);
		virtual void Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString);
#endif
		void Unregister( CRemoteClient_S * rc );

	public:
		virtual void LockMain_AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}
		virtual void LockMain_AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}
		virtual bool IsSimplePacketMode() { return m_simplePacketMode; }

		//////////////////////////////////////////////////////////////////////////
		// 연결 유지 기능
		void RemoteClient_GarbageOrGoOffline(
			CRemoteClient_S *rc,
			ErrorType errorType,
			ErrorType detailType,
			const ByteArray& comment,
			const PNTCHAR* where,
			SocketErrorCode socketErrorCode);

		void FallbackP2PToRelay(CRemoteClient_S * rc, ErrorType reason);
		void FallbackP2PToRelay(CRemoteClient_S * rc, P2PConnectionState* state, ErrorType reason);
		void IoCompletion_ProcessMessage_RequestAutoConnectionRecovery(CMessage &msg, CRemoteClient_S *rc);
		void NotifyAutoConnectionRecoveryFailed(CRemoteClient_S * rc, ErrorType reason);
		void RemoteClient_CleanupUdpSocket(CRemoteClient_S* rc, bool garbageSocketNow);
		void RemoteClient_FallbackUdpSocketAndCleanup(CRemoteClient_S* rc, bool garbageSocketNow);
		void RemoteClient_ChangeToAcrWaitingMode(CRemoteClient_S *rc, ErrorType errorType, ErrorType detailType, const ByteArray& comment, const PNTCHAR* where, SocketErrorCode socketErrorCode);
		void Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(CRemoteClient_S* rc);
		void Heartbeat_AcrSendMessageIDAckOnNeed(CRemoteClient_S* rc);
	};

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}
