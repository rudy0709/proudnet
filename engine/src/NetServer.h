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
#include "../include/Enums.h"
#include "RCPair.h"
#include "ISendDest_C.h"
#include "NetCore.h"
#include "P2PPair.h"
#include "IVizAgentDg.h"
#include "P2PGroup_S.h"
#include "Relayer.h"
#include "HostIDFactory.h"
#include "RemoteClient.h"
#include "LoopbackHost_S.h"
#include "LeanDynamicCast.h"
#include "HlaSessionHostImpl_S.h"
#include "FinalUserWorkItem.h"
#include "LocalEvent.h"
#include "TimeAlarm.h"
#include "marshaler-private.h"
#include "CachedTimes.h"
#include "SendDestInfo.h"
#include "P2PPair_S.h"
#include "LambdaEventHandlerImpl.h"
#include "CollUtil.h"
#include "PooledObjects_S.h"


#ifdef SUPPORTS_WEBGL
#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif
#include "server_ws.hpp"
#include "server_wss.hpp"
#endif

using namespace Proud;
#include "NetC2S_stub.h"
#include "NetS2C_proxy.h"

#define ASSERT_OR_HACKED(x) { if(!(x)) { EnqueueHackSuspectEvent(rc,#x,HackType_PacketRig); } }


namespace Proud
{
	class CListenerThread_S;
	class P2PGroup_MemberJoin_AckKey;
	class MessageSummary;

#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

	class CNetServerImpl :
		public CNetCoreImpl,
#ifdef USE_HLA
		public IHlaSessionHostImplDelegate_S,
#endif
		public Proud::CNetServer,
		public IVizAgentDg,
		public IThreadPoolEvent
	{
		friend class CSuperSocket;

		// 소스코드 여기저기서 main lock이라 말하는 것은 바로 이것이다.
		CriticalSection m_cs;

		// Start/Stop 함수가 여러 스레드에서 동시에 호출되더라도 단일화하기 위한 목적
		CriticalSection m_startStopCS;

		// 싱글톤 파괴 순서 정렬
		CGlobalTimerThread::PtrType m_globalTimer;

		// 서버쪽에서 ACR을 켜고 끌 수 있는 플래그. 기본값은 true.
		bool m_enableAutoConnectionRecoveryOnServer;

	public:
		virtual CriticalSection& GetCriticalSection() PN_OVERRIDE { return m_cs; }

		//CCachedTimes m_cachedTimes;

		// 이제 이 함수는 쓰지 말자. GetPreciseCurrentTimeMs로 대체하자.
		int64_t GetCachedServerTimeMs()
		{
			return GetPreciseCurrentTimeMs();
			//return m_cachedTimes.GetTimeMs();
		}

		// 파라메터로 받은 로컬 IP.
		// 즉 외부주소 혹은 public 주소이다.
		// 서버가 공유기나 L4 스위치 뒤에 있는 경우 유용.
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
		//		class CUdpAddrPortToRemoteClientIndex : public CFastMap2<AddrPort, shared_ptr<CRemoteClient_S>,int>
		//		{
		//		public:
		//			CUdpAddrPortToRemoteClientIndex();
		//		};
		//		CUdpAddrPortToRemoteClientIndex m_UdpAddrPortToRemoteClientIndex;

		// TCP listening을 하고 있는 중인지?
		// false이면 더 이상의 연결을 받는 처리를 안한다.
		bool m_listening;

		//이 변수들과 변수를 사용하는 부분을 모두 CNetCoreImpl로 옮기자. 같은 이름의 변수가 NS,NC,LS,LC에 있는데 마찬가지로.
		//		volatile bool m_netThreadPoolUnregistered;
		//		volatile bool m_userThreadPoolUnregistered;
		//여기까지.

		Guid m_protocolVersion;

		// 서버에 접속되어 있는 클라들의 P2P 그룹 목록
		P2PGroups_S m_P2PGroups;

		// 접속 들어오는 client의 hostID를 발급해주는 역할
		CHeldPtr<CHostIDFactory> m_HostIDFactory;

		int64_t m_HostIDRecycleAllowTimeMs;			// HostID Factory에서 같은 값을 재사용하는 cool time. Proud.CNetConfig.HostIDRecycleAllowTimeMs를 디폴트로 쓰지만 PNTest에 의해 다르게 가는 경우도 있다.

		// NetServer의 경우 Start ~ Stop 범위에서만 LoopbackHost 사용
		shared_ptr<CLoopbackHost_S> m_loopbackHost;

#ifdef SUPPORTS_WEBGL
		// Websocket server가 사용하는 스레드.
		// 현재 버전은 Networker thread와 통합되어 있지 않다. 그게 더 낫지만.
		// 추후에 websocket server의 i/o event와 기존 i/o event poller (iocp or epoll)이 서로 연결될 수 있게 만들면, 그때는 이걸 없애도록 하자.
		std::thread m_webSocketServerThread;

		// websocket start parameter.
		CStartServerParameter::WebSocketParam m_webSocketParam;

		// websocket server instance.
		// 아래 둘 중 하나만이 실제로 사용됩니다.
		shared_ptr<WsServer> m_webSocketServer;
		shared_ptr<WssServer> m_webSocketServerSecured;

		template<typename ConnectionT, typename MessageT, typename EndPointT>
		void DefineWebSocketCallbacks(EndPointT& endpoint, std::map<shared_ptr<ConnectionT>, shared_ptr<CRemoteClient_S>>& connectionMap);

		void NotifyStartupEnvironment(shared_ptr<CRemoteClient_S> rc);

		// 아래 map을 보호한다.
		// 주의: lock order는 main lock 다음이다. 반대 order로 lock 하는 일 없도록 하자.
		CriticalSection m_webSocketConnToClientMapCritSec;

		// websocket object smartptr이 key이고, value는 remote client에 대한 SuperSocket이다.
		std::map<shared_ptr<WsServer::Connection>, shared_ptr<CRemoteClient_S>> m_WebSocketConnToClientMap;
		std::map<shared_ptr<WssServer::Connection>, shared_ptr<CRemoteClient_S>> m_WebSocketSecuredConnToClientMap;
#endif

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
			DECRMI_ProudC2S_RoundTripLatencyPing;
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
		CSuperSocketArray m_staticAssignedUdpSockets;

		//CFastMap2<AddrPort,shared_ptr<CSuperSocket>,int> m_localAddrToUdpSocketMap; // UDP socket의 local 주소 -> 소켓 객체

																					  // CSuperSocket를 쓰는 이유: issue state 등을 갖고 있는 완전체니까. 그리고 accept pend socket을 갖고 있으니까.
																					  // TCP listen socket을 여러개 할수 있도록 수정함.
		CSuperSocketArray m_TcpListenSockets;

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
		//		CSendReadyList* m_sendReadyList;
		//		//이 변수와 이 변수를 다루는 함수들 (SendReadyList_Add,
		//		//SendReadyList에 대한 생성/파괴)을 CNetCoreImpl로 옮기자.


		/* 위 m_sendReadyList는 main lock 없이 접근된다.
		따라서 m_sendReadyList로부터 얻은 IHostObject는 무효할 수 있다.
		이것의 유효성 검사는 main lock 상태에서 해야 한다. 그것을 위한 목록이다.
		이 목록에는 TCP socket들, UDP socket들이 들어간다. per-remote UDP socket, per-remote TCP socket, static assigned UDP socket들이 들어간다. 즉 인덱스인 셈이다.
		m_sendReadyList에 있더라도 이 목록에 없으면 무효다.
		이 목록에 있는 객체들은 실제로 파괴되기 직전까지 항상 들어있는다. 즉 send ready 조건을 만족할 때만 들어가는 것이 아님.
		이 목록에 접근할 때는 main lock일 때만 해야 한다.
		*/
		// 이 변수는 shared_ptr<CSuperSocket> 를 key로 가져야 한다.
		//CFastMap2<shared_ptr<CSuperSocket> , int, int> m_validSendReadyCandidates;

		// 루프백 메시지용 키
		shared_ptr<CSessionKey> m_selfSessionKey;
		CryptCount m_selfEncryptCount, m_selfDecryptCount;

		// aes 키를 교환하기 위한 rsa key
		CCryptoRsaKey m_selfXchgKey;

		// 공개키는 미리 blob 으로 변환해둔다.
		ByteArray m_publicKeyBlob;

		void ProcessHeartbeat() PN_OVERRIDE;

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

		//		volatile LONG m_issueSendOnNeedWorking;

		//		bool EveryRemote_IssueSendOnNeed(CNetworkerLocalVars& localVars);
#if defined (_WIN32)
		void StaticAssignedUdpSockets_IssueFirstRecv();
		void TcpListenSockets_IssueFirstAccept();
#endif // _WIN32

		void ShowThreadExceptionAndPurgeClient(shared_ptr<CRemoteClient_S> client, const PNTCHAR* where, const PNTCHAR* reason);

		void ProcessMessageOrMoveToFinalRecvQueue(
			const shared_ptr<CSuperSocket>& socket,
			CReceivedMessageList &extractedMessages) PN_OVERRIDE;
		bool ProcessMessage_ProudNetLayer(
			CReceivedMessage &receivedInfo,
			const shared_ptr<CRemoteClient_S>& rc,
			const shared_ptr<CSuperSocket>& socket);
		bool ProcessMessage_FromUnknownClient( AddrPort from, CMessage& recvedMsg, const shared_ptr<CSuperSocket>& udpSocket );

		void IoCompletion_ProcessMessage_ServerHolepunch( CMessage &msg, AddrPort from, const shared_ptr<CSuperSocket>& udpSocket );
		void IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch( CMessage &msg, AddrPort from, const shared_ptr<CSuperSocket>& udpSocket );
		void IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(
			CMessage &msg,
			const shared_ptr<CRemoteClient_S>& rc);
		void IoCompletion_ProcessMessage_UnreliablePing( CMessage &msg, const shared_ptr<CRemoteClient_S>& rc );
		void IoCompletion_ProcessMessage_SpeedHackDetectorPing( CMessage &msg, const shared_ptr<CRemoteClient_S>& rc );
		void IoCompletion_ProcessMessage_UnreliableRelay1(
			CMessage &msg,
			const shared_ptr<CRemoteClient_S>& rc);
		void IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(
			CMessage &msg,
			const shared_ptr<CRemoteClient_S>& rc);
		void IoCompletion_ProcessMessage_ReliableRelay1(
			CMessage &msg,
			const shared_ptr<CRemoteClient_S>& rc);
		void IoCompletion_ProcessMessage_LingerDataFrame1( CMessage &msg, const shared_ptr<CRemoteClient_S>& rc );
		void IoCompletion_ProcessMessage_NotifyHolepunchSuccess( CMessage &msg, const shared_ptr<CRemoteClient_S>& rc );
		void IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess( CMessage &msg, const shared_ptr<CRemoteClient_S>& rc );
		void IoCompletion_ProcessMessage_Rmi(
			CReceivedMessage &receivedInfo,
			bool messageProcessed,
			const shared_ptr<CRemoteClient_S>& rc,
			const shared_ptr<CSuperSocket>& socket);
		void IoCompletion_ProcessMessage_UserOrHlaMessage(
			CReceivedMessage &receivedInfo,
			FinalUserWorkItemType UWIType,
			bool messageProcessed,
			const shared_ptr<CRemoteClient_S>& rc,
			const shared_ptr<CSuperSocket>& socket);
		void IoCompletion_MulticastUnreliableRelay2(
			CriticalSectionLock *mainLock,
			const HostIDArray &relayDest,
			HostID relaySenderHostID,
			EncryptMode& encMode,
			CMessage &payload,
			MessagePriority priority,
			int64_t uniqueID,
			bool fragmentOnNeed);

		DecryptResult GetDecryptedPayloadByRemoteClient(
			const shared_ptr<CRemoteClient_S>& rc,
			const EncryptMode& encMode,
			CMessage& msg,
			CMessage& decryptedOutput);

		// 해당 메시지가 와도 아무것도 하지않는다.
		//void IoCompletion_ProcessMessage_PolicyRequest(const shared_ptr<CRemoteClient_S>& rc);

		void NotifyProtocolVersionMismatch( const shared_ptr<CRemoteClient_S>& rc);
		//add by rekfkno1 - END

		void NotifyLicenseMismatch(const shared_ptr<CRemoteClient_S>& rc);

		virtual void OnThreadBegin() PN_OVERRIDE;
		virtual void OnThreadEnd() PN_OVERRIDE;
		//add by rekfkno1 - END

		void SetEventSink(INetServerEvent *eventSink) PN_OVERRIDE;

	public:
		void Start(CStartServerParameter &param) PN_OVERRIDE;
		void Start(CStartServerParameter &param, ErrorInfoPtr& outError) PN_OVERRIDE;

	private:
#ifdef SUPPORTS_WEBGL
		void Start_WebSocketServer(CStartServerParameter &param);
#endif
	public:
		void Stop() PN_OVERRIDE;
	private:
		void Stop_Internal(bool destructorCalls);
	private:
		void StopAsync();

	private:
		void StaticAssignedUdpSockets_Clear(bool startFunctionCalls);

		struct CreateSocketFailInfo
		{
			int m_arrayIndex; // TCP or UDP port 배열 중 몇번째에서 실패인지
			int m_portNum; // 시도하려고 했던 포트 넘버
		};

		SocketErrorCode CreateTcpListenSocketsAndInit(CFastArray<int> tcpPorts, CreateSocketFailInfo& outFailInfo);
		SocketErrorCode CreateAndInitUdpSockets(ServerUdpAssignMode udpAssignMode,
			CFastArray<int> udpPorts,
			CFastArray<int> &failedBindPorts,
			CSuperSocketArray& outCreatedUdpSocket,
			CreateSocketFailInfo& outFailInfo);

		virtual bool AsyncCallbackMayOccur() PN_OVERRIDE;
		void BadAllocException(Exception& ex) PN_OVERRIDE;
	public:

		Random m_random;


		/** clientHostID가 가리키는 peer가 참여하고 있는 P2P group 리스트를 얻는다. */
		bool GetJoinedP2PGroups(HostID clientHostID, HostIDArray &output) PN_OVERRIDE;

		shared_ptr<CRemoteClient_S> GetRemoteClientBySocket_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& remoteAddr);
		bool TryGetRemoteClientBySocket_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& remoteAddr, shared_ptr<CRemoteClient_S>& output);

		void PurgeTooOldAddMemberAckItem();

		void PurgeTooOldUnmatureClient();

		shared_ptr<CSuperSocket> GetAnyUdpSocket();
		void RemoteClient_New_ToClientUdpSocket(const shared_ptr<CRemoteClient_S>& rc);

		void GetP2PGroups(CP2PGroups &ret) PN_OVERRIDE;
		int GetP2PGroupCount() PN_OVERRIDE;

		void OnSocketWarning(shared_ptr<CSuperSocket> s, String msg);

		////////////////////////////
		// CNetCoreImpl 구현

		void OnMessageSent(int doneBytes, SocketType socketType/*, const shared_ptr<CSuperSocket>& socket*/) PN_OVERRIDE;
		void OnMessageReceived(int doneBytes, CReceivedMessageList& messages, const shared_ptr<CSuperSocket>& socket) PN_OVERRIDE;

		void ProcessDisconnecting(const shared_ptr<CSuperSocket>& socket, const ErrorInfo& errorInfo) PN_OVERRIDE;

		void OnAccepted(const shared_ptr<CSuperSocket>& socket, AddrPort tcpLocalAddr, AddrPort tcpRemoteAddr) PN_OVERRIDE;

		void OnConnectSuccess(const shared_ptr<CSuperSocket>& socket) PN_OVERRIDE;
		void OnConnectFail(const shared_ptr<CSuperSocket>& socket, SocketErrorCode code) PN_OVERRIDE;

		//아래 함수도 CNetCoreImpl로 옮기자.
		//void SendReadyList_Add(const shared_ptr<CSuperSocket>& socket);

		virtual void OnHostGarbaged(const shared_ptr<CHostBase>& remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode) PN_OVERRIDE;
		virtual void OnHostGarbageCollected(const shared_ptr<CHostBase>& remote) PN_OVERRIDE;
		virtual void OnSocketGarbageCollected(const shared_ptr<CSuperSocket>& socket) PN_OVERRIDE;

		virtual void AddEmergencyLogList(int /*logLevel*/, LogCategory /*logCategory*/, const String& /*logMessage*/, const String& /*logFunction = _PNT("")*/, int /*logLine = 0*/) {}
		//////////////////////////////////////////////////////////////////////////
		// IThreadReferrer 구현
		void OnCustomValueEvent(const ThreadPoolProcessParam& param, CWorkResult* workResult, CustomValueEvent customValue) PN_OVERRIDE;

		bool Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult) PN_OVERRIDE;

		bool IsValidHostID(HostID id) PN_OVERRIDE;
		bool IsValidHostID_NOLOCK(HostID id) PN_OVERRIDE;

		void EnqueueClientLeaveEvent(
			const shared_ptr<CRemoteClient_S>& rc,
			ErrorType errorType,
			ErrorType detailtype,
			const ByteArray& comment,
			SocketErrorCode socketErrorCode);
		virtual void ProcessOneLocalEvent(LocalEvent& e, const shared_ptr<CHostBase>& subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult) PN_OVERRIDE;

		CNetServerImpl();
		~CNetServerImpl();
	private:
		shared_ptr<CHostBase> GetTaskSubjectByHostID_NOLOCK(HostID subjectHostID) PN_OVERRIDE;
	public:
		void AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}

		void AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}
	private:
#ifdef _DEBUG
		void CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname) PN_OVERRIDE;
#endif // _DEBUG
	public:
		INetCoreEvent *GetEventSink_NOCSLOCK() PN_OVERRIDE;

		bool TryGetCryptSessionKey(HostID remote, shared_ptr<CSessionKey>& output, String &errorOut, LogLevel& outLogLevel) PN_OVERRIDE;

		bool NextEncryptCount(HostID remote, CryptCount &output) PN_OVERRIDE;
		void PrevEncryptCount(HostID remote) PN_OVERRIDE;
		bool GetExpectedDecryptCount(HostID remote, CryptCount &output) PN_OVERRIDE;
		bool NextDecryptCount(HostID remote) PN_OVERRIDE;

		virtual bool Send(const CSendFragRefs &sendData,
			const SendOpt& sendContext,
			const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength) PN_OVERRIDE;
	public:
		bool Send_BroadcastLayer(
			const CSendFragRefs& payload,
			const CSendFragRefs* encryptedPayload,
			const SendOpt& sendContext,
			const HostID *sendTo, int numberOfsendTo) PN_OVERRIDE;

		// P2P group => HostID list.
		// HostID => 그대로 냅둠
		// 다 한 후에, 동일 값을 병합하는 과정도 거침.
		// 속도에 민감해서 여기로 옮김.
		void ExpandHostIDArray(int numberOfsendTo, const HostID * sendTo, HostIDArray &sendDestList) PN_OVERRIDE
		{
			// source의 각 항목 A를 convert한 결과 B를 채우되, not-empty B를 2번 이상 채우면, union work를 해야 한다
			// union work는 sort등 부하가 크고, 대부분의 게임의 경우 P2P 그룹이나 수신자 하나만 넣기 때문에, 안할 수 있으면 안하는게 좋다.
			bool needUnion = false;
			int addTimes = 0;
			for (int i = 0; i < numberOfsendTo; ++i)
			{
				if (HostID_None != sendTo[i])
				{
					int addedCount = ExpandHostIDArray_Append(sendTo[i], sendDestList);
					if (addedCount > 0)
					{
						addTimes++;
					}
					if (addTimes >= 2)
						needUnion = true;
				}
			}

			//needUnion = true;// 버그때문에 잠시 막음

			if(needUnion)
			{
				// 이 구문은 아래 p2p relay 정리 전에 필요할 듯
				UnionDuplicates<HostIDArray, HostID, int>(sendDestList);
			}
		}

		virtual bool CloseConnection(HostID clientHostID) PN_OVERRIDE;
		virtual void CloseEveryConnection() PN_OVERRIDE;
		void RequestAutoPrune(const shared_ptr<CRemoteClient_S>& rc);

		virtual bool SetHostTag(HostID hostID, void* hostTag) PN_OVERRIDE;
		void* GetHostTag_NOLOCK(HostID hostID);

		//		shared_ptr<CRemoteClient_S> GetRemoteClientByHostID_NOLOCK(HostID clientHostID);
		shared_ptr<CRemoteClient_S> GetAuthedClientByHostID_NOLOCK(HostID clientHostID);
		bool TryGetAuthedClientByHostID(HostID clientHostID, shared_ptr<CRemoteClient_S>& output);

		int GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL) PN_OVERRIDE;
		int GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL) PN_OVERRIDE;
		virtual int GetP2PRecentPingMs(HostID HostA,HostID HostB) PN_OVERRIDE;

		bool DestroyP2PGroup(HostID groupHostID) PN_OVERRIDE;

		bool LeaveP2PGroup(HostID memberHostID, HostID groupHostID) PN_OVERRIDE;
		HostID CreateP2PGroup( const HostID* clientHostIDList0, int count, ByteArray message,CP2PGroupOption &option,HostID assignedHostID) PN_OVERRIDE;
		bool JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray message) PN_OVERRIDE;

		void EnqueAddMemberAckCompleteEvent( HostID groupHostID, HostID addedMemberHostID ,ErrorType result);
	private:
		int m_joinP2PGroupKeyGen;

		bool JoinP2PGroup_Internal(HostID newMemberHostID, HostID groupHostID, ByteArray message, int joinP2PGroupKeyGen);
		void AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent( shared_ptr<CP2PGroup_S> group,HostID memberHostID , ErrorType reason);

	public:
		void P2PGroup_CheckConsistency();
		void P2PGroup_RefreshMostSuperPeerSuitableClientID(shared_ptr<CP2PGroup_S> group);
		void P2PGroup_RefreshMostSuperPeerSuitableClientID(const shared_ptr<CRemoteClient_S>& rc);
		void DestroyEmptyP2PGroups() PN_OVERRIDE;

		int GetClientHostIDs(HostID* ret, int count) PN_OVERRIDE;

		bool GetClientInfo(HostID clientHostID, CNetClientInfo &ret) PN_OVERRIDE;

		CP2PGroupPtr_S GetP2PGroupByHostID_NOLOCK(HostID groupHostID)
		{
			AssertIsLockedByCurrentThread(); // 1회라도 CS 접근을 막고자

											 //CriticalSectionLock clk(GetCriticalSection(), true);
			CP2PGroupPtr_S ret;
			if (m_P2PGroups.TryGetValue(groupHostID, ret))
				return ret;
			else
				return shared_ptr<CP2PGroup_S>();
		}

		bool TryGetP2PGroupByHostID_NOLOCK(HostID groupHostID, CP2PGroupPtr_S& output)
		{
			AssertIsLockedByCurrentThread(); // 1회라도 CS 접근을 막고자

			return m_P2PGroups.TryGetValue(groupHostID, output);
		}

		bool GetP2PGroupInfo(HostID groupHostID, CP2PGroup &output) PN_OVERRIDE;

		// P2P 그룹을 host id list로 변환 후 배열에 이어붙인다.
		// garbaged인 것들도 일단은 포함된다.
		// return: sendTo2에 add한 횟수. convert를 안한다면 항상 1이다.
		int ExpandHostIDArray_Append(HostID sendTo, HostIDArray &sendTo2)
		{
			AssertIsLockedByCurrentThread();
			// convert sendTo group to remote hosts

			// ConvertP2PGroupToPeerList는 한번만 호출되는 것이 아니다.
			// sendTo2.SetSize(0);

			shared_ptr<CP2PGroup_S> g;
			if(!TryGetP2PGroupByHostID_NOLOCK(sendTo, g))
			{
				sendTo2.Add(sendTo);
				return 1;
			}
			else
			{
				for (P2PGroupMembers_S::iterator i = g->m_members.begin(); i != g->m_members.end(); i++)
				{
					HostID memberID = i->first;
					sendTo2.Add(memberID);
				}
				return (int)g->m_members.size();
			}
		}

		shared_ptr<CHostBase> GetSendDestByHostID_NOLOCK(HostID peerHostID);
		bool TryGetSendDestByHostID_NOLOCK(HostID peerHostID, shared_ptr<CHostBase>& output);

		bool GetP2PConnectionStats(HostID remoteHostID,CP2PConnectionStats& status) PN_OVERRIDE;
		bool GetP2PConnectionStats(HostID remoteA, HostID remoteB, CP2PPairConnectionStats& status) PN_OVERRIDE;

		//	bool IsFinalReceiveQueueEmpty();

		//void RemoteClient_RemoveFromCollections( const shared_ptr<CRemoteClient_S>& rc);
		void EnqueErrorOnIssueAcceptFailed(SocketErrorCode e);
		void EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient();
		void HardDisconnect_AutoPruneGoesTooLongClient(const shared_ptr<CRemoteClient_S>& rc);

		void ElectSuperPeer();

		int64_t GetTimeMs() PN_OVERRIDE;

		String DumpGroupStatus() PN_OVERRIDE;

		int GetClientCount() PN_OVERRIDE;
		void GetStats(CNetServerStats &outVal) PN_OVERRIDE;

		void AttachProxy(IRmiProxy *proxy) PN_OVERRIDE
		{
			CNetCoreImpl::AttachProxy(proxy);
		}
		void AttachStub(IRmiStub *stub) PN_OVERRIDE
		{
			CNetCoreImpl::AttachStub(stub);
		}
		void DetachProxy(IRmiProxy *proxy) PN_OVERRIDE
		{
			CNetCoreImpl::DetachProxy(proxy);
		}
		void DetachStub(IRmiStub *stub) PN_OVERRIDE
		{
			CNetCoreImpl::DetachStub(stub);
		}
		void SetTag(void* value) PN_OVERRIDE
		{ CNetCoreImpl::SetTag(value); }

		void* GetTag() PN_OVERRIDE
		{ return CNetCoreImpl::GetTag(); }

		bool RunAsyncInternal(HostID taskOwner, LambdaBase_Param0<void>* func) PN_OVERRIDE
		{ return CNetCoreImpl::RunAsyncInternal(taskOwner, func); }

		virtual HasCoreEventFunctionObjects& Get_HasCoreEventFunctionObjects() PN_OVERRIDE
		{ return *this; }

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

		void ShowWarning_NOCSLOCK(ErrorInfoPtr errorInfo);

		virtual void ShowError_NOCSLOCK(ErrorInfoPtr errorInfo) PN_OVERRIDE;
		virtual void ShowNotImplementedRmiWarning(const PNTCHAR* RMIName) PN_OVERRIDE;
		virtual void PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName) PN_OVERRIDE;

		void GetRemoteIdentifiableLocalAddrs(CFastArray<NamedAddrPort> &output) PN_OVERRIDE;
		// remote client의, 외부 인터넷에서도 인식 가능한 TCP 연결 주소를 리턴한다.
		NamedAddrPort GetRemoteIdentifiableLocalAddr(const shared_ptr<CRemoteClient_S>& rc);

		void SetDefaultTimeoutTimeMs(int newValInMs) PN_OVERRIDE;
		void SetTimeoutTimeMs(HostID host, int newValInMs) PN_OVERRIDE;

		SocketResult GetAllSocketsLastReceivedTimeOutState(int64_t currTime, const shared_ptr<CRemoteClient_S>& rc);

		void SetDefaultAutoConnectionRecoveryTimeoutTimeMs(int newValInMs) PN_OVERRIDE;
		void SetAutoConnectionRecoveryTimeoutTimeMs(HostID host, int newValInMs) PN_OVERRIDE;

		//		void SetTimerCallbackIntervalMs(int newVal);

		bool SetDirectP2PStartCondition(DirectP2PStartCondition newVal) PN_OVERRIDE;

		// 너무 오랫동안 쓰이지 않은 UDP 송신 큐를 찾아서 제거한다.
		void DoForLongInterval();

	public:

		//void DisconnectRemoteClientOnTimeout();
		void Heartbeat_EveryRemoteClient();

		void GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output) PN_OVERRIDE;
		void GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output) PN_OVERRIDE;
	private:
		void FallbackServerUdpToTcpOnNeed( const shared_ptr<CRemoteClient_S>& rc, int64_t currTime );
		void ArbitraryUdpTouchOnNeed( const shared_ptr<CRemoteClient_S>& rc, int64_t currTime );
		void RefreshSendQueuedAmountStat(const shared_ptr<CRemoteClient_S>& rc);
	public:
		//void PruneTooOldDefragBoardOnNeed();
		void SecondChanceFallbackUdpToTcp(const shared_ptr<CRemoteClient_S>& rc);

		virtual void GetTcpListenerLocalAddrs(CFastArray<AddrPort> &output) PN_OVERRIDE;
		virtual void GetUdpListenerLocalAddrs(CFastArray<AddrPort> &output) PN_OVERRIDE;

		// 정렬을 하기위해 P2PConnectionState 과 배열의 index와 directP2P Count로 구성된 구조체 리스트를 만든다.
		struct ConnectionInfo
		{
			shared_ptr<P2PConnectionState> state;
			int hostIndex0, hostIndex1;
			int connectCount0,connectCount1;

			void Clear()
			{
				state = NULL; hostIndex0 = hostIndex1 = 0;
				connectCount0 = 0; connectCount1 = 0;
			}
			bool operator < (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs < rhs.state->m_recentPingMs;
			}
			bool operator <= (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs <= rhs.state->m_recentPingMs;
			}
			bool operator >(const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs > rhs.state->m_recentPingMs;
			}
			bool operator >= (const ConnectionInfo& rhs) const
			{
				return state->m_recentPingMs >= rhs.state->m_recentPingMs;
			}
		};
	private:
		CFastArray<ConnectionInfo> m_connectionInfoList;
		CFastArray<int, false, true> m_routerIndexList;
	public:
		void RemoteClient_RequestStartServerHolepunch_OnFirst( const shared_ptr<CRemoteClient_S>& rc);
		void RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(shared_ptr<CRemoteClient_S>& rc);

		void MakeP2PRouteLinks(SendDestInfoArray &tgt, int unreliableS2CRoutedMulticastMaxCount, int64_t routedSendMaxPing);

		void HostIDArrayToHostPtrArray_NOLOCK(SendDestInfoArray& dest, HostIDArray& src, const SendOpt& sendOpt);

		virtual void SetDefaultFallbackMethod(FallbackMethod value) PN_OVERRIDE;

		shared_ptr<CLogWriter> m_logWriter;

		virtual void EnableLog(const PNTCHAR* logFileName) PN_OVERRIDE;
		virtual void DisableLog() PN_OVERRIDE;

		//		void EnqueueHackSuspectEvent(const shared_ptr<CRemoteClient_S>& rc,const char* statement,HackType hackType);
		void EnqueueP2PGroupRemoveEvent( HostID groupHostID );

		//		double GetSpeedHackLongDetectMinInterval();
		//		double GetSpeedHackLongDeviationRatio();
		//		double GetSpeedHackDeviationThreshold();
		//		int GetSpeedHackDetectorSeriesLength();

		int m_SpeedHackDetectorReckRatio; // 0~100

		virtual void SetSpeedHackDetectorReckRatioPercent(int newValue) PN_OVERRIDE;
		virtual bool EnableSpeedHackDetector(HostID clientID, bool enable) PN_OVERRIDE;

		virtual void SetMessageMaxLength(int value_s, int value_c) PN_OVERRIDE;

		virtual bool IsConnectedClient(HostID clientHostID) PN_OVERRIDE;

		virtual void TEST_SomeUnitTests();

		void TestUdpConnReset();
#if defined (_WIN32)
		// $$$; == > 본 작업이 완료되면 반드시 ifdef를 제거해주시기 바랍니다.
		// dev003에서 흘러온 것임. 이승철님 선에서 해결되어야.
		void TestReaderWriterMonitor();
#endif
		virtual void SetMaxDirectP2PConnectionCount(HostID clientID, int newVal) PN_OVERRIDE;
		virtual HostID GetMostSuitableSuperPeerInGroup(HostID groupID, const CSuperPeerSelectionPolicy& policy, const HostID* excludees, intptr_t excludeesLength) PN_OVERRIDE;
		virtual HostID GetMostSuitableSuperPeerInGroup(HostID groupID, const CSuperPeerSelectionPolicy& policy, HostID excludee) PN_OVERRIDE;

		virtual int GetSuitableSuperPeerRankListInGroup(HostID groupID, SuperPeerRating* ratings, int ratingsBufferCount, const CSuperPeerSelectionPolicy& policy, const CFastArray<HostID> &excludees) PN_OVERRIDE;
	private:
		void TouchSuitableSuperPeerCalcRequest(shared_ptr<CP2PGroup_S> group, const CSuperPeerSelectionPolicy &policy);
	public:

		virtual void GetUdpSocketAddrList(CFastArray<AddrPort> &output) PN_OVERRIDE;

		virtual int GetInternalVersion() PN_OVERRIDE;

		void TestStringW();
		void TestStringA();
		void TestFastArray();

		void EnqueueUnitTestFailEvent(String msg);
		virtual void EnqueError(ErrorInfoPtr info) PN_OVERRIDE;
		virtual void EnqueWarning(ErrorInfoPtr info) PN_OVERRIDE;

		void EnqueClientJoinApproveDetermine(const shared_ptr<CRemoteClient_S>& rc, const ByteArray& request);
		void ProcessOnClientJoinApproved(const shared_ptr<CRemoteClient_S>& rc, const ByteArray &response);
		void ProcessOnClientJoinRejected(const shared_ptr<CRemoteClient_S>& rc, const ByteArray &response);

		// C/S 첫 핑퐁 작업 : OnClientJoin에서도 Ping 얻는 함수를 사용할 수 있도록 하는 작업
		void SetClientPingTimeMs(const shared_ptr<CRemoteClient_S>& rc);

	private:
		// true시 빈 P2P 그룹도 허용함

		bool m_allowEmptyP2PGroup;
		bool m_startCreateP2PGroup;
	public:
		virtual void AllowEmptyP2PGroup(bool enable) PN_OVERRIDE;
		virtual bool IsEmptyP2PGroupAllowed() const PN_OVERRIDE;
	private:
		//void TestReliableUdpFrame();
		void TestSendBrake();

		String GetConfigString();
	public:
		void EnquePacketDefragWarning(shared_ptr<CSuperSocket> superSocket, AddrPort sender, const String& text) PN_OVERRIDE;

	private:
		int GetMessageMaxLength() PN_OVERRIDE;
		virtual HostID GetVolatileLocalHostID() const PN_OVERRIDE;
		virtual HostID GetLocalHostID() PN_OVERRIDE;

	public:
		virtual bool IsNagleAlgorithmEnabled() PN_OVERRIDE;

		//double m_lastPruneTooOldDefragBoardTime;

		int m_freqFailLogMostRank;
		String m_freqFailLogMostRankText;
		int64_t m_lastHolepunchFreqFailLoggedTime;
		volatile int32_t m_fregFailNeed;

	public:
		// 종료시에만 설정됨
		// 2010.07.20 delete by ulelio :  Stop()이 return하기 전까지는 이벤트 콜백이 있어야하므로 삭제한다.
		//volatile bool m_shutdowning;

		bool SendUserMessage(const HostID* remotes, int remoteCount, RmiContext &rmiContext, uint8_t* payload, int payloadLength) PN_OVERRIDE
		{
			return CNetCoreImpl::SendUserMessage(remotes,remoteCount,rmiContext,payload,payloadLength);
		}

		virtual ErrorType GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent) PN_OVERRIDE;
/*
		virtual bool IsEventCallbackPaused() PN_OVERRIDE;
		virtual void PauseEventCallback() PN_OVERRIDE;
		virtual void ResumeEventCallback() PN_OVERRIDE;*/

		virtual ErrorType SetCoalesceIntervalMs(HostID remote, int intervalMs) PN_OVERRIDE;
		virtual ErrorType SetCoalesceIntervalToAuto(HostID remote) PN_OVERRIDE;

	public:
		void LogFreqFailOnNeed();
		void LogFreqFailNow();

		virtual void TEST_SetOverSendSuspectingThresholdInBytes(int newVal) PN_OVERRIDE;
		virtual void TEST_SetTestping(HostID hostID0, HostID hostID1, int ping) PN_OVERRIDE;

		bool m_forceDenyTcpConnection_TEST;
		virtual void TEST_ForceDenyTcpConnection() PN_OVERRIDE;

		ServerUdpAssignMode m_udpAssignMode;

#ifdef VIZAGENT
		void EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey) PN_OVERRIDE;
#endif
	private:
		virtual CNetClientImpl* QueryNetClient() PN_OVERRIDE { return NULL; }
		virtual CNetServerImpl* QueryNetServer() PN_OVERRIDE { return this; }

#ifdef VIZAGENT
		virtual void Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext);
		virtual void Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString);
#endif
		void Unregister(const shared_ptr<CRemoteClient_S>& rc);

	public:
		void LockMain_AssertIsLockedByCurrentThreadImpl() PN_OVERRIDE
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}
		void LockMain_AssertIsNotLockedByCurrentThreadImpl() PN_OVERRIDE
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}
		bool IsSimplePacketMode() PN_OVERRIDE { return m_simplePacketMode; }

		//////////////////////////////////////////////////////////////////////////
		// 연결 유지 기능
		void RemoteClient_GarbageOrGoOffline(
			const shared_ptr<CRemoteClient_S>& rc,
			ErrorType errorType,
			ErrorType detailType,
			const ByteArray& shutdownCommentFromClient,
			const String& commentFromErrorInfo,
			const PNTCHAR* where,
			SocketErrorCode socketErrorCode);

		void FallbackP2PToRelay(const shared_ptr<CRemoteClient_S>& rc, ErrorType reason);
		void FallbackP2PToRelay(const shared_ptr<CRemoteClient_S>& rc, shared_ptr<P2PConnectionState> state, ErrorType reason);
		void IoCompletion_ProcessMessage_RequestAutoConnectionRecovery(
			CMessage &msg
			, const shared_ptr<CRemoteClient_S>& rc
			, const shared_ptr<CSuperSocket>& candidateSocket);
		void NotifyAutoConnectionRecoveryFailed(const shared_ptr<CSuperSocket>& socket, ErrorType reason);
		void RemoteClient_CleanupUdpSocket(const shared_ptr<CRemoteClient_S>& rc, bool garbageSocketNow);
		void RemoteClient_FallbackUdpSocketAndCleanup(const shared_ptr<CRemoteClient_S>& rc, bool garbageSocketNow);
		void RemoteClient_ChangeToAcrWaitingMode(
			const shared_ptr<CRemoteClient_S>& rc,
			ErrorType errorType,
			ErrorType detailType,
			const ByteArray& shutdownCommentFromClient,
			const String& commentFromErrorInfo,
			const PNTCHAR* where,
			SocketErrorCode socketErrorCode);
		void Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(const shared_ptr<CRemoteClient_S>& rc);
		void Heartbeat_AcrSendMessageIDAckOnNeed(const shared_ptr<CRemoteClient_S>& rc);

#ifndef PROUDSRCCOPY
		// 이 멤버 변수는 이 클래스의 맨 마지막에 선언되어야 한다. RetainedServer는 CNetServerImpl을 바로 액세스하는데,
		// 이게 맨 뒤가 아니면 멤버들 offset이 하나씩 밀려버린다.
		void* m_lic;
#endif
	};

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}


#include "NetServerImpl.inl"
#include "NetServer_WebSocket.inl"
