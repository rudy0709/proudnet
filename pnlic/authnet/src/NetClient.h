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

#include "../include/ConnectParam.h"
#include "../include/Crypto.h"
//#include "../include/Enums.h"
#include "../include/FastList.h"
//#include "../include/IRMIProxy.h"
//#include "../include/IRMIStub.h"
//#include "../include/HlaHost_C.h"
//#include "../include/Message.h"
//#include "../include/MilisecTimer.h"
//#include "../include/ReceivedMessage.h"
//#include "../include/RmiContext.h"
//#include "../include/FakeClr.h"
//#include "../include/RMIContext.h"
//#include "../include/MessageSummary.h"

//#include "IOCP.h"
#include "LocalEvent.h"
#include "NetCore.h"
#include "SendOpt.h"
//#include "FastSocket.h"
#include "TimeAlarm.h"
//#include "TypedPtr.h"
//#include "FinalUserWorkItem.h"
#include "RemoteServer.h"
#include "LoopbackHost.h"

#ifdef __ANDROID__
#include "NetClient_NDK.h"
#endif

using namespace Proud;

#include "RemotePeer.h"
//#include "P2PGroup_C.h"
#include "Networker_C.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif
#include "NetS2C_stub.h"
#include "NetC2C_stub.h"
#include "NetC2S_proxy.h"
#include "NetC2C_proxy.h"
#include "IVizAgentDg.h"
#include "EmergencyLogData.h"
//#include "SuperSocket.h"
#include "NetClientManager.h"
#include "HlaSessionHostImpl_S.h"

#include "acrimpl_c.h"
#include "../include/NetClientStats.h"

namespace Proud
{
	class CNetClientWorker;
	class CUdpSocket_C;
	class CFallbackableUdpLayer_C;
	class CNetClientImpl;
	class CRemotePeer_C;
	class MessageSummary;
	class CUpnpTcpPortMappingState;
	class CFastSocket;
	class CThreadPoolImpl;

	//class ReceivedMessage_C
	//{
	//    AddrPort m_from = AddrPort::Unassigned;
	//}

	class CNetClientImpl : public CNetCoreImpl,
		public IP2PGroupMember,
		public CNetClient,
		//		public IHlaSessionHostImplDelegate_C,
		public IVizAgentDg
	{
	public:
		/* 2013.03.06 : KJSA
		 * ProcessMessage_NotifyServerConnectionHint에서 DefaultTimeoutTime이 변경 될때
		 * m_ReliablePing_Timer의 주기도 변경되어야 하므로
		 * NetWorker_C 에서 접근하기 위해 private에서 public으로 변경
		 */
		CTimeAlarm m_ReliablePing_Timer;

	private:
		// TcpAndUdp_DoForShortInterval 실행 주기를 위해
		int64_t m_TcpAndUdp_DoForShortInterval_lastTime;

		// TCP unstable을 검사하는 루틴을 수행할 주기
		int64_t m_checkTcpUnstable_timeToDo;

	public:
		CSessionKey m_selfP2PSessionKey;
	private:
		CryptCount m_selfEncryptCount, m_selfDecryptCount;

	public:
		AddrPort Get_ToServerUdpSocketAddrAtServer();
		AddrPort Get_ToServerUdpSocketLocalAddr();

		AddrPort GetTcpLocalAddr();
		AddrPort GetUdpLocalAddr();

		virtual CriticalSection& GetCriticalSection() PN_OVERRIDE;
#ifdef _DEBUG
		void CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname);
#endif // _DEBUG

		// true인 경우 연결 해제시 연이어 도착하는 여러가지 연결 해제 이벤트를 잠재운다.
		// 서버와의 연결이 TCP, UDP 등이 있기 때문에 이렇게 해주는거다.
		bool m_supressSubsequentDisconnectionEvents;

		// RequestServerTime을 호출한 횟수
		uint32_t m_RequestServerTimeCount;

		// NC 각각을 보호함
		CriticalSection m_critSec;
		//bool m_FirstRecvDisregard;
		//intptr_t m_DisregardOffset;

	public:
		// 클라이언트와 서버의 시간 차이 (clientTime - serverTime)
		int64_t m_serverTimeDiff;

		// 서버 컴퓨터에 대한 레이턴시. TCP fallback mode인 경우 TCP로 핑퐁한 결과가 들어간다.
		// round trip의 반이다.
		int m_serverUdpRecentPingMs;
		int m_serverUdpLastPingMs;

		int m_serverTcpRecentPingMs;
		int m_serverTcpLastPingMs;

		// 클라-서버 연결 과정에서 주고받는 기초 메시징을 갖고 핑도 덤으로 구한다. 처음 핑 보낸 시간.
		// 퐁을 받으면 최초의 TCP latency다.
		int64_t m_firstTcpPingedTime;

		void ServerTcpPing_UpdateValues(int newLag);

		// 클라-서버간 핑퐁 확인이 1회 이상 일어났는가
		bool m_serverUdpPingChecked;
		bool m_serverTcpPingChecked;

		// 서버에게 보낸 udp packet count
		int m_toServerUdpSendCount;
		int64_t m_lastReportUdpCountTime;

		int m_ioPendingCount;

	private:
		/* connect, disconnect 과정만을 보호하는 객체
		- m_cs보다 우선 잠금을 요한다.
		- 이게 없으면 disconnect 과정중 다른 스레드에서 connect를 할 때 race condition이 발생한다.
		내부적으로 connect와 disconnect는 m_cs를 1회 이상 unlock하기 때문이다.
		*/
		CriticalSection m_connectDisconnectFramePhaseLock;

		// 사용자가 Connect or Disconnect 함수를 호출했던 횟수
		PROUDNET_VOLATILE_ALIGN int32_t m_disconnectInvokeCount, m_connectCount;

		// RequestServerTime을 마지막으로 보낸 시간
		int64_t m_lastRequestServerTimeTime;

		// Multi-Threaded 모델시 heartbeat 의 1-thread 처리를 보장해주는 atomic 변수
		PROUDNET_VOLATILE_ALIGN int32_t m_isProcessingHeartbeat;

	public:
		// 이 객체가 존재하는 경우 서버와 연결 시도중, 연결된 상태, 연결 해제중임을 의미.
		//CNetClientWorkerPtr m_worker;
		RefCount<CNetClientWorker> m_worker;

		CSingleton<CNetClientManager>::PtrType m_manager;
	private:
		P2PGroups_C m_P2PGroups;

	public:
		// 그룹 내의 모든 피어에서 핑을 보내줄 때 사용되는 시간
		int64_t m_enablePingTestEndTime;

		// 동시 진행중인 p2p connection 시도 횟수에 따라 이 값들은 신축한다.
		int64_t m_P2PConnectionTrialEndTime;

		// 동시 진행중인 p2p connection 시도 횟수에 따라 이 값들은 신축한다.
		int64_t m_P2PHolepunchInterval;

		/* 2013.03.06 : KJSA
		 * ProcessMessage_NotifyServerConnectionHint에서 DefaultTimeoutTime이 변경 될때
		 * m_ReliablePing_Timer의 주기도 변경되어야 하므로
		 * NetWorker_C 에서 접근하기 위해 private에서 public으로 변경
		 */
		int64_t GetReliablePingTimerIntervalMs();
	private:
		virtual bool AsyncCallbackMayOccur();

		void UpdateP2PGroup_MemberJoin(
			const HostID &groupHostID,
			const HostID &memberHostID,
			const ByteArray &customField,
			const uint32_t &eventID,
			const int &p2pFirstFrameNumber,
			const Guid &connectionMagicNumber,
			const ByteArray &p2pAESSessionKey,
			const ByteArray &p2pFastSessionKey,
			bool allowDirectP2P,
			bool pairRecycled);

	public:
		bool m_natDeviceNameDetected;
		// emergencylog때문에 하나 백업을 받아놓자.
		CNetClientStats m_recentBackedUpStats;

		CNetClientStats m_stats;

		// OnMessageSend는 겨우 stats update만 한다.
		// 이것때문에 main lock 걸면 오버다.
		// 그래서 이것이 따로 있다.
		CriticalSection m_statsCritSec;

		int m_internalVersion;

		// true이면 iocp를 써도 되는 플랫폼, 아니면 non-block으로 작동하는 플랫폼
		// UDP socket이 생성되기 전에 기록되어야 함.
		bool m_isProactorAsyncModel;

		// heartbeat thread가 막장 느린 상태임을 경고한 바가 있는지
		bool m_hasWarnedStarvation;

		// 서버 인스턴스의 GUID
		// 서버 연결 성공시 발급받는다.
		Guid m_serverInstanceGuid;

		CryptCount m_toServerEncryptCount, m_toServerDecryptCount;
		CSessionKey m_toServerSessionKey;

		Random m_random;
		//CSuperSocketPtr m_toServerUdpSocket;
		//bool				m_toServerUdpSocketFailed;

		// 사용자가 지정한 UDP 포트 리스트
		// 이것이 필요한 이유: Hackshield 등의 게임 보안 프로그램은, 지정한 포트만 사용하는 것을 허락하는 경우가 있어서다. 나원참.
		CFastSet<uint16_t> m_unusedUdpPorts;

		// 이미 사용한 UDP 포트 리스트
		CFastSet<uint16_t> m_usedUdpPorts;
	public:
		// 자기자신에게 송신한 메시지. 아직 user worker로 넘어간건 아니다.
		//CFastList2<CMessage, int> m_loopbackFinalReceivedMessageQueue;

		// CHeldPtr보다 변경 추적이 쉬우므로

		//CFallbackableUdpLayerPtr_C m_ToServerUdp_fallbackable;

		//사용하지 않는 기능이므로 제거.
		//double m_minExtraPing, m_extraPingVariance;

		// 주의: FastArray에 의해 사용되므로 생성자,파괴자,복사 연산자가 불필요해야 한다!!
		class RelayDest_C
		{
		public:
			CRemotePeer_C *m_remotePeer;
			int m_frameNumber;
		};

		class RelayDestList_C : public CFastArray < RelayDest_C, true, false, int >
		{
		public:
			void ToSerializable(RelayDestList &ret);
		};

		// 2011.09.21 add by ulelio : 패킷 최적화를 위해 unreliable relay를 할때 모든 hostid를 보내는것이 아니라 directp2p한 양이 적을경우
		// 서버에 direct된 hostId를 보내 최적화한다. ( 부분 집합이므로 subset이란 단어사용 )
		class P2PGroupSubset_C
		{
		public:
			HostIDArray m_excludeeHostIDList;	// 릴레이에서 제외되야할 hostid list
		};

		class CompressedRelayDestList_C
		{
		public:
			CFastMap2<HostID, P2PGroupSubset_C, int> m_p2pGroupList;
			HostIDArray m_includeeHostIDList;			// 어떠한 p2p 그룹에도 등록되어 있지 않은 개별 host id list

			void AddSubset(const HostIDArray &subsetGroupHostID, HostID hostID);	// 제외되어야할 hostid list에 추가한다.
			void AddIndividual(HostID hostID);							    // individual list에 추가한다.
			int GetAllHostIDCount();						// 모든 HostID의 갯수 ( 그룹 호스트ID까지 포함됨 )

			CompressedRelayDestList_C();
		};

		//CHeldPtr<CFastSocket> m_socket;

		CNetConnectionParam m_connectionParam;
		// Disconnect가 다 안끝났는데 사용자가 Connect를 호출하면 여기에 일단 저장된다.
		CNetConnectionParam *m_pendedConnectionParam;

		// 서버로의 연결
		CRemoteServer_C* m_remoteServer;

		/* 자기 자신으로의 연결.
		이것이 필요한 이유 중 하나: loopback의 user work 도 처리해야 하는 뭔가가 있어야 하므로. */
		CLoopbackHost* m_loopbackHost;

		// 가장 최근에 FrameMove를 호출한 시간
		// 0이면 한번도 호출 안했음, -1이면 경고를 더 이상 낼 필요가 없음
		int64_t m_lastFrameMoveInvokedTime;

		// DisconnectAsync()를 호출한 시간
		int64_t m_disconnectCallTime;
		// Disconnect()를 호출한 경우 True로 설정됩니다.
		bool m_isDisconnectCalled;

		// FrameMove를 호출한 경우에만 True로 설정됩니다.
		// 본 함수는 사용자가 LocalEvent, RMI 등에서 Disconnect함수의 호출을 막고자 사용되는 변수입니다.
		bool m_isFrameMoveCalled;

		//RemotePeers_C m_remotePeers;

		CTimeAlarm m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer;
		bool m_enableLog;

		// 가상 스피드핵. 얼마나 자주 ping 메시지를 보내느냐를 조작한다.
		// 1이면 스핵을 안쓴다는 의미다.
		int64_t m_virtualSpeedHackMultiplication;

		int64_t m_lastCheckSendQueueTime;
		int64_t m_sendQueueHeavyStartTime;

		// m_slowReliableP2P 의 값을 캐슁하고 이 값에 따라 값을 선택하는 부분을 매 수행하는 것보다는 멤버로 구해서 가지고 있는것이
		// 속도상 이득이 있을듯 하여서 값을 각각 캐슁하는 것으로 결정함
		int64_t m_ReliableUdpHeartbeatInterval_USE;
		int64_t m_StreamToSenderWindowCoalesceInterval_USE;

	public:
#if defined(_WIN32)
		// 2011.06.30 add by ulelio : CEmergencyLogServer에게 보낼 로그데이터
		CEmergencyLogData m_emergencyLogData;
		void UpdateNetClientStatClone();
		bool SendEmergencyLogData(String serverAddr, uint16_t serverPort);
		// 해당 메소드의 내부에서 Proxy를 호출하는데 Proxy의 프로토콜이 String이므로 아예 인자를 String으로 받음
		void AddEmergencyLogList(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction = _PNT(""), int logLine = 0);
#endif

	private:
		int64_t m_lastUpdateNetClientStatCloneTime;
#if defined(_WIN32)
		static DWORD WINAPI RunEmergencyLogClient(void* context);
#endif
		bool GetIntersectionOfHostIDListAndP2PGroupsOfRemotePeer(HostIDArray sortedHostIDList, CRemotePeer_C* rp, HostIDArray* outSubsetGroupHostIDList);
	public:
		class C2CProxy : public ProudC2C::Proxy
		{};
		class C2SProxy : public ProudC2S::Proxy
		{};
		class S2CStub : public ProudS2C::Stub
		{
		public:
			CNetClientImpl *m_owner;

			DECRMI_ProudS2C_P2PGroup_MemberJoin;
			DECRMI_ProudS2C_RequestP2PHolepunch;
			DECRMI_ProudS2C_NotifyDirectP2PEstablish;
			DECRMI_ProudS2C_P2PGroup_MemberLeave;
			DECRMI_ProudS2C_P2P_NotifyDirectP2PDisconnected2;
			DECRMI_ProudS2C_P2P_NotifyP2PMemberOffline;
			DECRMI_ProudS2C_P2P_NotifyP2PMemberOnline;
			DECRMI_ProudS2C_ReliablePong;
			DECRMI_ProudS2C_EnableLog;
			DECRMI_ProudS2C_DisableLog;
			DECRMI_ProudS2C_NotifyUdpToTcpFallbackByServer;
			DECRMI_ProudS2C_NotifySpeedHackDetectorEnabled;
			DECRMI_ProudS2C_ShutdownTcpAck;
			DECRMI_ProudS2C_NewDirectP2PConnection;
			DECRMI_ProudS2C_RequestMeasureSendSpeed;
			DECRMI_ProudS2C_RequestAutoPrune;
			DECRMI_ProudS2C_S2C_RequestCreateUdpSocket;
			DECRMI_ProudS2C_S2C_CreateUdpSocketAck;
		};

		class C2CStub : public ProudC2C::Stub
		{
		public:
			CNetClientImpl* m_owner;

			DECRMI_ProudC2C_HolsterP2PHolepunchTrial;
			DECRMI_ProudC2C_ReportUdpMessageCount;
			// 			DECRMI_ProudC2C_ReportServerTimeAndFrameRateAndPing;
			// 			DECRMI_ProudC2C_ReportServerTimeAndFrameRateAndPong;
		};
		C2CProxy m_c2cProxy;
		C2CStub m_c2cStub;
		C2SProxy m_c2sProxy;
		S2CStub m_s2cStub;

		CNetClientImpl();

		bool Connect(const CNetConnectionParam &param) PN_OVERRIDE;
		bool Connect(const CNetConnectionParam &param, ErrorInfoPtr& outError) PN_OVERRIDE;
	private:
		bool Connect_Internal(const CNetConnectionParam &param);
		void Connect_CheckStateAndParameters(const CNetConnectionParam &param);
	public:
		void OnAccepted(CSuperSocket* socket, AddrPort localAddr, AddrPort remoteAddr) PN_OVERRIDE;

		void OnConnectSuccess(CSuperSocket* socket) PN_OVERRIDE;
		void OnConnectFail(CSuperSocket* socket, SocketErrorCode code) PN_OVERRIDE;

		void StartupUpnpOnNeed();
		void AddUpnpTcpPortMappingOnNeed();
		void DeleteUpnpTcpPortMappingOnNeed();

		bool IsCalledByWorkerThread();

	private:
#ifdef _WIN32
		// true이면 upnp 포트매핑 명령을 던진 적이 있으므로 또 던지지 말라는 뜻.
		CHeldPtr<CUpnpTcpPortMappingState> m_upnpTcpPortMappingState;
#endif // _WIN32
	public:
		void SetEventSink(INetClientEvent *eventSink);
		void SetEventSink(INetClientEvent *eventSink, ErrorInfoPtr& outError);

		INetClientEvent* GetEventSink();

		//		void LogLastServerUdpPacketReceived();
		void EnqueueDisconnectionEvent(const ErrorType errorType, const ErrorType detailType, const String &comment);
		inline void EnqueueDisconnectionEvent(const ErrorType errorType, const ErrorType detailType = ErrorType_Ok)
		{
			String comment = _PNT("");
			EnqueueDisconnectionEvent(errorType, detailType, comment);
		}
		void EnqueueConnectFailEvent(ErrorType errorType, const String& comment, SocketErrorCode socketErrorCode = SocketErrorCode_Ok, ByteArrayPtr reply = ByteArrayPtr());
		void EnqueueConnectFailEvent(ErrorType errorType, ErrorInfoPtr errorInfo);
		void EnqueFallbackP2PToRelayEvent(HostID remotePeerID, ErrorType reason);
		void SetInitialTcpSocketParameters();

		virtual INetCoreEvent *GetEventSink_NOCSLOCK() PN_OVERRIDE;

		virtual void Disconnect() PN_OVERRIDE;
		virtual void Disconnect(const CDisconnectArgs &args) PN_OVERRIDE;
		virtual void Disconnect(ErrorInfoPtr& outError) PN_OVERRIDE;
		virtual void Disconnect(const CDisconnectArgs &args, ErrorInfoPtr& outError) PN_OVERRIDE;
		virtual void DisconnectAsync() PN_OVERRIDE;
		virtual void DisconnectAsync(const CDisconnectArgs &args) PN_OVERRIDE;
		virtual void DisconnectAsync(ErrorInfoPtr& outError) PN_OVERRIDE;
		virtual void DisconnectAsync(const CDisconnectArgs &args, ErrorInfoPtr& outError) PN_OVERRIDE;

		bool IsDisconnectAsyncEnded() PN_OVERRIDE;

	private:
		void New_ToServerUdpSocket();

	public:
		bool Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult) PN_OVERRIDE;
		void BadAllocException(Exception& ex);

		//void WaitForGarbageCleanup();
		void Cleanup();

		~CNetClientImpl();

		void OnCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue);

		// 넷클라 워커스레드의 기아화 감지용
		int64_t m_lastHeartbeatTime;
		// NOTE: volatile 키워드에만 의존했던 루틴 모두 수정하기.pptx
		int64_t m_recentElapsedTime;

		// coalesce interval, 즉 issue-send를 수행하는 주기.
		// Proud.CNetConfig.EveryRemoteIssueSendOnNeedIntervalMs이지만
		// 스트레스 테스트시 많은 NC 객체를 소화하기 위해 값을 수정할 경우를 위해 멤버로 떼놨다.
		int m_everyRemoteIssueSendOnNeedIntervalMs;

		//#if !defined(_WIN32)
		//		void EveryRemote_NonBlockedSendOnNeed(CIoEventStatus &comp);
		//#else
		//		//		void EveryRemote_IssueSendOnNeed();
		//#endif

		void OnSocketWarning(CFastSocket* socket, String msg);

		bool GetP2PGroupByHostID(HostID groupHostID, CP2PGroup &output);

		CP2PGroupPtr_C GetP2PGroupByHostID_Internal(HostID groupHostID);

		virtual bool Send(const CSendFragRefs &msg,
			const SendOpt& sendContext,
			const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength) PN_OVERRIDE;

		virtual bool Send_BroadcastLayer(
			const CSendFragRefs& payload,
			const CSendFragRefs* encryptedPayload,
			const SendOpt& sendContext,
			const HostID* sendTo, int numberOfsendTo);

		// void Send_ToServer_Directly_Copy(HostID destHostID, MessageReliability reliability, const CSendFragRefs& sendData2, const SendOpt& sendOpt);

		void ConvertGroupToIndividualsAndUnion(int numberOfsendTo, const HostID * sendTo, ISendDestArray &sendDestList);
		void ConvertGroupToIndividualsAndUnion(int numberOfsendTo, const HostID * sendTo, HostIDArray& output);
		// bool Send(CSendFragRefs &sendData, MessagePriority priority, MessageReliability reliability, char orderingChannel, HostID sendTo) PN_OVERRIDE;

		bool ConvertAndAppendP2PGroupToPeerList(HostID sendTo, ISendDestArray &sendTo2);

		//		CHostBase *GetSendDestByHostID(HostID peerHostID);

		virtual bool SetHostTag(HostID hostID, void* hostTag);
		void* GetHostTag(HostID hostID);

		CRemotePeer_C* GetPeerByHostID_NOLOCK(HostID peerHostID);
		AddrPort GetLocalUdpSocketAddr(HostID remotePeerID);

		CSessionKey *GetCryptSessionKey(HostID remote, String &errorOut, bool& outEnqueError);
		bool NextEncryptCount(HostID remote, CryptCount &output);
		void PrevEncryptCount(HostID remote);
		bool GetExpectedDecryptCount(HostID remote, CryptCount &output);
		bool NextDecryptCount(HostID remote);

		CRemotePeer_C* GetPeerByUdpAddr(AddrPort UdpAddr, bool findInRecycleToo);

		void SendServerHolepunch();

		void FrameMove(int maxWaitTimeMs, CFrameMoveResult *outResult);
		void FrameMove(int maxWaitTime, CFrameMoveResult* outResult, ErrorInfoPtr& outError);

		void SendServerHolePunchOnNeed();
		void ReportP2PPeerPingOnNeed();
		void ReportRealUdpCount();

		void GetStats(CNetClientStats &outVal);

	private:
		CApplicationHint m_applicationHint;

	public:
		virtual bool IsSimplePacketMode();

		virtual void SetApplicationHint(const CApplicationHint &hint);
		virtual void GetApplicationHint(CApplicationHint &hint);
		virtual HostID GetVolatileLocalHostID() const PN_OVERRIDE;
		virtual HostID GetLocalHostID() PN_OVERRIDE;
		bool GetPeerInfo(HostID remoteHostID, CNetPeerInfo &output);
		void GetLocalJoinedP2PGroups(HostIDArray &output);
#ifdef DEPRECATE_SIMLAG
		void SimulateBadTraffic(uint32_t minExtraPing, uint32_t extraPingVariance);
#endif // DEPRECATE_SIMLAG

		ConnectionState GetServerConnectionState(CServerConnectionState &output);
		void GetWorkerState(CClientWorkerInfo &output);

		virtual void LockMain_AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}

		virtual void LockMain_AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}

		virtual void LockRemote_AssertIsLockedByCurrentThread()
		{
			// NetClient에서는 Main 및 Remote lock의 구분이 없다. 따라서 Main의 Assert와 같다.
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}
		virtual void LockRemote_AssertIsNotLockedByCurrentThread()
		{
			// NetClient에서는 Main 및 Remote lock의 구분이 없다. 따라서 Main의 Assert와 같다.
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}

		CP2PGroupPtr_C CreateP2PGroupObject_INTERNAL(HostID groupHostID);

		// 일정 시간이 됐으면 서버에 서버 시간을 요청한다.
		void RequestServerTimeOnNeed();
		void SpeedHackPingOnNeed();
		void CheckSendQueue();

		int64_t GetServerTimeDiffMs()
		{
			return m_serverTimeDiff;
		}

		int64_t GetIndirectServerTimeMs(HostID peerHostID);

		int64_t GetServerTimeMs();

		void P2PPingOnNeed();

		String GetGroupStatus()
		{
			String ret = _PNT("<not yet>");
			//
			//{
			//CriticalSectionLock clk(m_critSecPtr, true);
			//    // NTTNTRACE를 안쓰고 1회의 OutputDebugString으로 때려버린다.
			//    // 이러면 여러 프로세스의 trace line이 겹쳐서 나오는 문제가 해결될라나?

			//    String a;
			//    ret += "=======================\n";
			//    //ret+="======================="; a.Format(" -- Time=%s\n",CT2A(CTime::GetCurrentTime().Format())); ret+=a;

			//    foreach (CRemotePeerPtr_C p in m_remotePeers.Values)
			//    {
			//        a.Format("*    peer=%d(%s)\n", p->m_HostID, ToString(p->m_externalID));
			//        ret += a;

			//        a.Format("*        indirect server time diff=%f\n", p->m_indirectServerTimeDiff);
			//        ret += a;

			//        for (CHostIDSet::iterator j = p->m_joinedP2PGroups.begin(); j != p->m_joinedP2PGroups.end(); j++)
			//        {
			//            a.Format("*        joined P2P group=%d\n", *j);
			//            ret += a;
			//        }
			//    }

			//    for (CP2PGroups::iterator i = m_P2PGroups.begin(); i != m_P2PGroups.end(); i++)
			//    {
			//        CP2PGroupPtr GP = i->second;
			//        a.Format("*    P2PGroup=%d\n", GP->m_groupHostID);
			//        ret += a;
			//        for (CHostIDSet::iterator j = GP->m_members.begin(); j != GP->m_members.end(); j++)
			//        {
			//            a.Format("*        member=%d\n", *j);
			//            ret += a;
			//        }
			//    }
			//    a.Format("=======================\n");
			//    ret += a;
			//}
			return ret;
		}

		void FallbackServerUdpToTcpOnNeed();
		void FirstChanceFallbackServerUdpToTcp(bool notifyToServer, ErrorType reasonToShow);

		void FirstChanceFallbackEveryPeerUdpToTcp(bool notifyToServer, ErrorType reason);
		int64_t GetP2PServerTimeMs(HostID groupHostID);

		void RemoveRemotePeerIfNoGroupRelationDetected(CRemotePeer_C* memberRC);

		HostID GetHostID()
		{
			return GetVolatileLocalHostID();
		}

		AddrPort GetServerAddrPort();

		int64_t GetIndirectServerTimeDiff()
		{
			return m_serverTimeDiff;
		}

		void TEST_FallbackUdpToTcp(FallbackMethod mode);

		virtual void TEST_SetPacketTruncatePercent(Proud::HostType hostType, int percent);

		bool RestoreUdpSocket(HostID peerID);
		bool InvalidateUdpSocket(HostID peerID, CDirectP2PInfo &outDirectP2PInfo);

		virtual bool GetDirectP2PInfo(HostID remotePeerID, CDirectP2PInfo &outInfo);

		//String DumpStatus();

		void GetGroupMembers(HostID groupHostID, HostIDArray &output);

		// modify by rekfkno1 - Bug가 나올 가능성이 있어 기능 쓰지 않음.
		//double GetSimLag();
		//{
		//	return m_minExtraPing + m_random.NextDouble() * m_extraPingVariance;
		//}

		void AttachProxy(IRmiProxy *proxy)
		{
			CNetCoreImpl::AttachProxy(proxy);
		}
		void AttachStub(IRmiStub *stub)
		{
			CNetCoreImpl::AttachStub(stub);
		}
		void DetachProxy(IRmiProxy *proxy)
		{
			CNetCoreImpl::DetachProxy(proxy);
		}
		void DetachStub(IRmiStub *stub)
		{
			CNetCoreImpl::DetachStub(stub);
		}

		void HlaFrameMove();
		void HlaAttachEntityTypes(CHlaEntityManagerBase_C* entityManager);

		void HlaSetDelegate(IHlaDelegate_C* dg);
#ifdef USE_HLA
		CHeldPtr<CHlaSessionHostImpl_C> m_hlaSessionHostImpl;
#endif // USE_HLA
	public:

		virtual String GetNatDeviceName();

		void ShowError_NOCSLOCK(ErrorInfoPtr errorInfo)
		{
			CNetCoreImpl::ShowError_NOCSLOCK(errorInfo);
		}

		virtual void EnqueError(ErrorInfoPtr info);
		virtual void EnqueWarning(ErrorInfoPtr info);

		void ShowNotImplementedRmiWarning(const PNTCHAR* RMIName)
		{
			CNetCoreImpl::ShowNotImplementedRmiWarning(RMIName);
		}

		void PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName)
		{
			CNetCoreImpl::PostCheckReadMessage(msg, RMIName);
		}

		int GetLastUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL);
		int GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* error = NULL);

		int GetLastReliablePingMs(HostID peerHostID, ErrorType* error = NULL);
		int GetRecentReliablePingMs(HostID peerHostID, ErrorType* error = NULL);

		//int GetFinalUserWotkItemCount();
		//void ClearFinalUserWorkItemList();

		virtual ErrorType SetCoalesceIntervalMs(HostID remote, int interval);
		virtual ErrorType SetCoalesceIntervalToAuto(HostID remote);

		bool IsTcpUnstable(int64_t interval);

		void TcpAndUdp_DoForLongInterval();

		void RemotePeerRecycles_GarbageTooOld();

		//void PruneTooOldDefragBoardOnNeed();

		// 최초 메시지 send issue를 일괄 수행하는 타이머
		//CTimeAlarm m_processSendReadyRemotes_Timer;

		void InduceDisconnect();

		bool GetPeerReliableUdpStats(HostID peerID, ReliableUdpHostStats &output);

		// 해당 메소드의 내부에서 Proxy를 호출하는데 Proxy의 프로토콜이 String이므로 아예 인자를 String으로 받음
		void Log(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction = _PNT(""), int logLine = 0);
		void LogHolepunchFreqFail(int rank, const PNTCHAR* format, ...);

		void TEST_EnableVirtualSpeedHack(int64_t multipliedSpeed);

		// 스핵이 걸려있으면 핑 주기가 짧아진다. 이를 통해 스핵 감지를 하는 핑 보내는 시간.
		// 엄청 큰 값을 넣어서 절대 부딪히지 않게 해도 된다.
		int64_t m_speedHackDetectorPingTime;

		bool IsLocalHostBehindNat(bool &output);

		String GetTrafficStatText();
		virtual String TEST_GetDebugText();

		// 		// worker thread에서 더 이상 사용하지 않음을 보장하는 변수

		// 		PROUDNET_VOLATILE_ALIGN bool m_tcpSendStopAcked,m_tcpRecvStopAcked;
		// 		PROUDNET_VOLATILE_ALIGN bool m_udpSendStopAcked,m_udpRecvStopAcked;
		//PROUDNET_VOLATILE_ALIGN bool m_heartbeatStopAcked;

		virtual void GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output) PN_OVERRIDE;
		virtual void GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output) PN_OVERRIDE;
		bool GetSocketInfo(HostID remoteHostID, CSocketInfo& output);

		//double m_lastPruneTooOldDefragBoardTime;

		virtual int GetInternalVersion();

		virtual void EnquePacketDefragWarning(CSuperSocket* superSocket, AddrPort addrPort, const PNTCHAR* text);

		virtual int GetMessageMaxLength();
		
		virtual AddrPort GetPublicAddress();

		CTestStats2 m_testStats2;

		virtual void TEST_GetTestStats(CTestStats2 &output);

		CNetClientWorker* GetWorker() { return m_worker; }

		////////////////////////////
		// CNetCoreImpl 구현

		void OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/);
		void OnMessageReceived(int doneBytes, SocketType socketType, CReceivedMessageList& messages, CSuperSocket* socket);

		void ProcessMessageOrMoveToFinalRecvQueue(SocketType socketType, CSuperSocket* socket, CReceivedMessageList &extractedMessages) PN_OVERRIDE;

		// disconncet 처리작업
		virtual void ProcessDisconnecting(CSuperSocket* socket, const ErrorInfo& errorInfo) PN_OVERRIDE;

		void AssociateSocket(CFastSocket* socket);

		//void SendReadyInstances_Add();

		bool SendUserMessage(const HostID* remotes, int remoteCount, const RmiContext &rmiContext, uint8_t* payload, int payloadLength)
		{
			return CNetCoreImpl::SendUserMessage(remotes, remoteCount, rmiContext, payload, payloadLength);
		}
		bool BindUdpSocketToAddrAndAnyUnusedPort(CSuperSocket* udpSocket, AddrPort &udpLocalAddr);

		void SetDefaultTimeoutTimeMs(int newValInMs);

		virtual ErrorType ForceP2PRelay(HostID remotePeerID, bool enable);
		virtual ErrorType GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent);

#ifdef TEST_DISCONNECT_CLEANUP
		virtual bool IsAllCleanup();
#endif

	private:
		virtual CNetClientImpl* QueryNetClient() { return this; }
		virtual CNetServerImpl* QueryNetServer() { return NULL; }

#ifdef VIZAGENT
		virtual void EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey) PN_OVERRIDE;

		virtual void Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext) PN_OVERRIDE;
		virtual void Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString) PN_OVERRIDE;
#endif

		bool IsBehindNat();

		bool IsRealUdpAlive() { return (bool)( m_remoteServer->m_ToServerUdp == NULL || m_remoteServer->GetToServerUdpFallbackable()->IsRealUdpEnabled() == false ); }

		virtual void ProcessOneLocalEvent(LocalEvent& e, CHostBase* subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult) PN_OVERRIDE;
		virtual void OnHostGarbaged(CHostBase* remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode) PN_OVERRIDE;
		virtual bool OnHostGarbageCollected(CHostBase* remote) PN_OVERRIDE;
		virtual void OnSocketGarbageCollected(CSuperSocket* socket) PN_OVERRIDE;
		virtual CHostBase* GetTaskSubjectByHostID_NOLOCK(HostID hostID) PN_OVERRIDE;
		virtual bool IsValidHostID_NOLOCK(HostID hostID) PN_OVERRIDE;

		// 주의: 새 멤버 추가시 CNetClientManager에 의해 접근할 수 있으므로 dtor에서 CS lock 필수! (primitive type은 예외)

		// NetClientWorker로부터 옮겨진 함수들입니다.
	private:
		// 반드시 콜러에서 하나의 쓰레드에서만 호출 된다는걸 보장 해야한다.
		void CacheLocalIpAddress_MustGuaranteeOneThreadCalledByCaller();
		void GetCachedLocalIpAddressSnapshot(CFastArray<String>& out);

		void Heartbeat();

		void Heartbeat_IssueConnect();
		bool Main_IssueConnect(SocketErrorCode* outCode);
		void Heartbeat_ConnectFailCase(SocketErrorCode code, const String& comment);

		void Heartbeat_Connecting();
		void ProcessFirstToServerTcpConnectOk();

		void Heartbeat_JustConnected();
#if defined(_WIN32)
		void IssueTcpFirstRecv();
#endif

		void Heartbeat_Connected();
		void Heartbeat_Connected_AfterLock();
		void Heartbeat_ConnectedCase();

		void Heartbeat_EveryRemotePeer();
		void Heartbeat_DetectNatDeviceName();

		void Heartbeat_Disconnecting();

		void Heartbeat_Disconnected();

		//////////////////////////////////////////////////////////////////////////
		// 연결 유지 기능 (ACR)
	public:
		// 연결 유지 기능이 허락되어 있는가
		bool m_enableAutoConnectionRecovery;

		// 연결 재접속 과정이 진행되는 동안 not null.
		CAutoConnectionRecoveryContextPtr m_autoConnectionRecoveryContext;

	private:
		// >0이면, 연결 불량 상태가 되더라도 일부러 여기 지정한 값만큼 늦게 ACR 을 시작한다.
		// 테스트할때 빼놓고는 항상 0이다.
		int64_t m_autoConnectionRecoveryDelayMs;

		// NOTE: AcrMessageRecovery는 RemoteServer가 직접 가진다.
		// 재연결 시도가 성공했을때 부여되는 새 세션키.
		// 구 세션키+1이다. 해커는 세션키를 모르므로 +1을 할 여지도 없다. 따라서 논리적으로 안전.
		CSessionKey m_acrCandidateSessionKey;
	private:
		/* 재접속 연결 후보 TCP connection 각각에 매치되는 remote server 객체의 목록.
		NOTE: tempRemoteServer 객체를 여러개 담아야 하는 이유
		=> 아주 짧은 순간에, 이러한 상황이 존재할 수 있습니다.
		- 재접속을 위해 만들었던 temp remote server 1이 garbaged이지만 garbage collect되지 않은 상태.
		- 새 재접속을 위해 만든 candidate socket의 TCP connect가 성공하여, temp remote server가 필요해진 상태.

		즉, 2개 이상의 temp remote server가 존재해야 할 수 있어야 한다.

		(CNetClient::OnHostGarbageCollected 참조)
		*/
		CFastMap<CHostBase*, CRemoteServer_C*> m_autoConnectionRecovery_temporaryRemoteServers;

	public:
		// ACR 연결에서, 세션키+1을 암호화해야 한다. 그래서 이것이 멤버 변수로 존재함.
		// Disconnected 상태가 되기 전까지는 안바뀜.
		ByteArray m_publicKeyBlob;
		
	private:
		// 일부 스마트폰은 망 전환이 일어날 때 TCP 연결을 끊지 않는다.
		// 따라서 일정 시간마다 NIC 변화를 감지해야 한다.
		// 이를 위한 타이머.
		int64_t m_checkNextTransitionNetworkTimeMs;

		void DisconnectOrStartAutoConnectionRecovery(const ErrorInfo& errorInfo);
		void StartAutoConnectionRecovery();

		void AutoConnectionRecovery_CleanupUdpSocket(CHostBase* host);
		bool AutoConnectionRecovery_OnTcpConnection(CSuperSocket* acrTcpSocket);
		void Heartbeat_AutoConnectionRecovery();

		void StopIoRequestToClosedSockets();

		void ProcessAcrCandidateFailure();
	public:
		// 네트워크 전환 감지를 유니캐스트 주소가 바뀐것으로 감지 할 것인지
		// 혹은 단순히 유니캐스트 주소가 나온것으로 감지 할 것인지의 인자
		// true -> 전자
		// false -> 후자
		bool IsTransitionNetwork(bool isTransitionUnicastAddress);

		void AutoConnectionRecovery_GarbageTempRemoteServerAndSocket(CRemoteServer_C* tempRemoteServer = NULL);
		void AutoConnectionRecovery_GarbageEveryTempRemoteServerAndSocket();
		void ProcessMessage_NotifyAutoConnectionRecoverySuccess(CMessage &msg);
		void ProcessMessage_NotifyAutoConnectionRecoveryFailed(CMessage &msg);

		virtual void TEST_FakeTurnOffTcpReceive();
		virtual void TEST_SetAutoConnectionRecoverySimulatedDelay(int timeMs);

	private:
		typedef CFastMap2<HostID, CRemotePeer_C*, int> HostIDToRemotePeerMap;

		// recycle 상태가 된 remote peer들. 수십초 지나면 garbage 처분된다.
		HostIDToRemotePeerMap m_remotePeerRecycles;
	public:
		void RemotePeerRecycles_Add(CRemotePeer_C* peer);
		CRemotePeer_C* RemotePeerRecycles_Pop(HostID peerHostID);
		
		virtual void GarbageAllHosts();
		virtual bool CanDeleteNow();

	private:
		uint32_t m_sendQueueTcpTotalBytes;
		uint32_t m_sendQueueUdpTotalBytes;
	};
}
