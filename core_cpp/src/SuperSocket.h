/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "ioevent.h"
#include "UseCount.h"
#include "FastSocket.h"
#include "RemoteBase.h"
//#include "TCPLayer.h"
//#include "StreamQueue.h"
#include "PacketFrag.h"
#include "acrimpl.h"
#include "CriticalSectImpl.h"
#include "TCPLayer.h"

//#define SUPERSOCKET_DETECT_HANG_ON_GARBAGE


#ifdef SUPERSOCKET_DETECT_HANG_ON_GARBAGE
#include "../UtilSrc/DebugUtil/DebugCommon.h"
#endif

namespace Proud
{
	class CThreadPoolImpl;
	class CNetCoreImpl;
	class CReferrerHeart;
	class CUdpPacketFragBoard;
	class CUdpPacketDefragBoard;
	class CUdpPacketFragBoardOutput;
	class CSendReadySockets;
	class CompletionPort;
	class CIoReactorEventNotifier;

#ifdef _WIN32
	typedef CompletionPort IoQueueType;
#else
	typedef CIoReactorEventNotifier IoQueueType;
#endif

	// 소켓 생성 결과 리턴값.
	struct SuperSocketCreateResult
	{
		// 얻은 소켓
		shared_ptr<CSuperSocket> socket;
		// 못 얻었으면 여기 사유가 기재된다.
		String errorText;
	};

	// 소켓 본연의 기능 뿐만 아니라 여러 프로토콜이나 송수신 데이터 처리 로직등이 
	// 들어간 덩치 큰 클래스. 덩치가 큰 놈이라 하여 super라는 이름이 붙었다.
	// socket, event 등의 객체가 같은 lifetime을 가지므로 이렇게 뜯어냈다.
	// listen socket의 역할도 함.
	// 역할: splitter, UDP frag, message 단위 처리, 초기 policy text 교환
	class CSuperSocket
		: public IIoFileDescriptor
		, public enable_shared_from_this<CSuperSocket>
	{
		friend class CompletionPort;
		friend class CIoReactorEventNotifier;
	public:
		// 소켓이슈가 완료된 직후 수행해야 할 절차
		enum ProcessType
		{
			ProcessType_None,
			ProcessType_OnMessageReceived,
			ProcessType_OnMessageSent,
			ProcessType_OnConnectStillInProgress,
			ProcessType_OnConnectSuccess,
			ProcessType_OnConnectFail,
			ProcessType_CloseSocketAndProcessDisconnecting,
			ProcessType_EnqueWarning,
			ProcessType_EnquePacketDefragWarning,
			ProcessType_OnAccept, // OnAccepted를 호출하고(accepted socket가 있으면) 다음 issue-accept을 걸어라.
		};

		// send queue외의 main lock로 보호되지 않는 모든 것을 보호하는 critical section
		// 이 lock 후 syscall을 자주 수행하므로 contention이 심하다.
		// 따라서 main lock 후 이것을 잠그면 암달의 저주를 겪는다.
		// Owner인 NetCore의 Lock과 이 Lock은 절대로 같이 잠기지 않는다.
		CriticalSection m_cs;

		/* socket lock은 syscall이 수반됨. 그러므로 상당한 시간을 먹음.
		사용자가 send queue에 뭔가를 넣을 때 socket lock을 걸면 contention의 확률 증가.
		따라서 send queue만을 보호하는 critsec을 둔다.

		lock order: socket lock => send queue lock.
		socket lock 없이 send queue lock하는 것은 허락되나, send queue lock 후 socket lock은 금지.

		주요 용도:
		사용자가 send하려고 하면 => socket lock !건너뛰고! send queue lock
		send event 발생하면=>socket lock->send queue lock->pop data->send queue unlock->syscall->socket unlock
		*/
		CriticalSection m_sendQueueCS;

		// 이 소켓 객체가, TCP 리스닝 전용이면, true이다.
		bool m_isListeningSocket;

		// 이 소켓 객체가, 아직 TCP 연결 진행중이면 true이다. 연결된 후 혹은 연결 시작 전이면 false.
		// TCP 연결 진행 중이면 send i/o event에 대해서 처리하는 방법이 달라져야 한다. 그래서 이게 있어야.
		bool m_isConnectingSocket;

		// 서버에서 인식한 클라 호스트의 주소. 클라가 공유기 뒤에 있으면 이 주소는 외부 주소임.
		// 서버에서 인식된 이 소켓의 주소, 즉 외부 인터넷 주소.
		// Proud.CRemotePeer_C.m_UdpAddrFromServer와 다름.
		AddrPort m_localAddrAtServer;

		// 클라 내부에서 인식된 호스트의 주소.클라가 공유기 뒤에 있으면 이 주소는 내부 주소이다.
		// 헤어핀 홀펀칭을 위함.
		AddrPort m_localAddr_USE_FUNCTION;

		// 재사용 기회를 잃는 시간. 즉 파괴되는 시간.
		// 0이면 재사용통에 안 들어갔음을 의미.
		// 소켓은 아직 닫지 않았음. 홀펀칭 재사용시 다시 사용되므로.
		int64_t m_timeToGarbage;

		CNetCoreImpl *m_owner;
		
//		CThreadPoolImpl* m_ownerThreadPool;

		// 이것에 대해 별도로 close를 호출하게 하지 말고,
		// 항상 stop io acked = true를 세팅하는 과정이 선결되어야 하므로
		// 필요한 기능만 제한 노출하기 위해 privat이다.
		shared_ptr<CFastSocket> m_fastSocket;

		// UDP 소켓이 보내거나 받는 데이터를 버리는 옵션.
		bool m_dropSendAndReceive;

	// 모의로 네트워크가 차단되었는지?
	// 네트워크 장치가 통신이 안되는 상황을 simulate하고자 하면, 즉 socket은 살아있지만 네트워크 장치가 오가는 것을 못하게 하고자 하려면,
	// 이 값을 true로 세팅하면 된다. 그러면 주고 받으려는 데이터가 모두 그냥 버려지게 된다.
		// TEST Case 95번 테스트 용도로써 랜선이 플러그 아웃 된 상황을 연출 하는 변수.
		bool m_turnOffSendAndReceive;

		/* #FAST_REACTION send ready list에 추가할 때, iocp or epoll이 즉시 깨개 할 것인가?
		클라에서는 true, 서버에서는 false이다.
		자세한 것은 위 hash tag 중에 설명되어 있다. */
		bool m_forceImmediateSend;


	public:
		 PROUD_API AddrPort GetSocketName();
	private:
		// Win32: AcceptEx를 호출할 때 동원되는 socket. AcceptEx의 sAcceptSocket에 준함.
		// AcceptEx의 completion이 발생하면 이것에 대해 FinalizeAcceptEx로 마무리되어야 한다.
		// unix: 평소에는 null이다가 non-block accept가 성공하면 여기에 채워진다.
		shared_ptr<CFastSocket> m_acceptCandidateSocket;

		// 위 socket에 대한 accept가 완료되면 아래가 채워진다.
		shared_ptr<CSuperSocket> m_acceptedSocket;

		// valid only if the variable above is valid.
		AddrPort m_acceptedSocket_tcpLocalAddr;
		AddrPort m_acceptedSocket_tcpRemoteAddr;

	public:
		// 이 값은 socket을 닫을 때는 어차피 무효하므로 iocp와의 deassoc시에도 값은 유지된다. 그때는 쓰지 말 것.
		// iocp가 여럿일 때, 이 socket을 다루는 iocp에 post event를 수동으로 할 때 사용되는 변수.
		IoQueueType* m_associatedIoQueue_accessByAssociatedSocketsOnly;

		inline IoQueueType* GetAssociatedIoQueue() { return m_associatedIoQueue_accessByAssociatedSocketsOnly; }

		 CriticalSection* GetAssociatedIoQueueCritSec();

	public:
#ifdef _WIN32
		// IoState_NotWorking,NotWorking,NoMoreWorkGuaranteed 중 하나이다.
		// epoll에서는 이들 변수를 쓰지 않는다.
		// overlapped send or connect용
		volatile IoState m_sendIssued;
		// overlapped recv or accept용
		volatile IoState m_recvIssued;
#else
		/* non-block i/o를 처리하는 스레드가 있는지 여부 등 상태.
		IoState_NotWorking,NotWorking,NoMoreWorkGuaranteed 중 하나이다. */
		//volatile int32_t m_ioState;
#endif

	private:
		/* 더 이상 overlapped io를 걸지 말라는 플래그.
		no lock 상태에서 여러 스레드에서 건드는 값이므로 volatile을 넣어야 함.
		socket을 닫으면서 이 값도 true로 세팅하자.
		이게 true가 되면 나머지 루틴에서 m_sendIssued,m_recvIssued를 2로 바꾸려 할 것이다.

		unix에서도 이것이 필요하다. ProcessDisconnecting이 중복 호출되는 것을 막기 위해.

		true이면, 더 이상 i/o 처리를 하지 말고 곧 닫을 소켓이라는 뜻이다.
		IoState_True or False 값이다.
		이것을 얻어 오는 값은 stale하지만, 잠금 없이 여러 스레드에서 액세스하므로, volatile이다.

		이 멤버 변수는 혼동의 여지가 있으므로 StopIoRequested(), RequestStopIo() 등을 이용하도록 하자.
		*/
		volatile IoState m_stopIoRequested_USE_FUNCTION;

// #ifdef _WIN32
// 		// 마지막으로 issue send를 건 시간. completion이 오면 0으로 바뀜에 주의.
// 		volatile int64_t m_lastSendIssuedTime;
// 		volatile int64_t m_lastRecvIssuedTime;
//
// 		volatile int64_t m_lastRecvissueWarningTime;
// 		volatile int64_t m_lastSendIssueWarningTime;
// #endif // _WIN32

	public:
		// 가장 마지막에 TCP 스트림을 받은 시간
		// 디스 여부를 감지하기 위함
		// 양쪽 호스트가 모두 감지해야 한다. TCP는 shutdown 직후 closesocket을 호출해도 상대가 못 받을 가능성이 있기 때문이다.
		volatile int64_t m_lastReceivedTime;

		// 패킷을 일정시간 이상 안보낼때 더미패킷을 보내기 위하여, 가장 마지막으로 이 소켓에 대해서 패킷을 보낸 시간을 기록해둔다.
		// 서버와 클라이언트 둘 다 다음과 같은 경우 패킷을 서로에게 패킷을 안보내게 된다.
		// 서버의 경우, 클라이언트와의 핑퐁은 항상 클라이언트가 먼저 핑을 보내야 되므로 만약에 클라이언트가 아무런 패킷도 안보낸다면 서버도 안보낸다.
		// 클라이언트의 경우, OnJoinServerComplete 호출 이전에는 Server가 Candidate 상태라서 아무런 패킷도 주고받지 않고 있게 된다.
		volatile int64_t m_tcpOnlyLastSentTime;

		/* 서버와의 연결이 끊어진 client socket의 peer name을 얻어봤자 Unassigned이다.
		그러므로 서버와의 연결이 성사됐을 때 미리 연결된 주소를 얻어둬야 나중에
		연결 종료 이벤트시 제대로 된 연결 정보를 유저에게 건넬 수 있다. */
		AddrPort m_remoteAddr;

		// 사용자가 NetClient.Disconnect or NetServer.CloseConnection을 호출한 경우에만 True로 설정됩니다.
		volatile bool m_userCalledDisconnectFunction;

	private:
		// 디버깅용
		// 총 issue send 시도한 바이트수
		uint64_t m_totalIssueSendBytes;
		uint64_t m_totalIssueSendCount;
		uint64_t m_totalIssueReceiveCount;

	public:
		int64_t m_firstIssueReceiveTime;
		int64_t m_firstReceiveEventTime;

		// send or receive event가 발생한 횟수
		// 디버깅용
		uint64_t m_totalSendEventCount;
		uint64_t m_totalReceiveEventCount;

		static int64_t m_firstReceiveDelay_TotalValue;
		static int64_t m_firstReceiveDelay_TotalCount;
		static int64_t m_firstReceiveDelay_MaxValue;
		static int64_t m_firstReceiveDelay_MinValue;

		//////////////////////////////////////////////////////////////////////////
		// CSendReadyList에 의해서만 액세스된다. 다른 곳에서는 액세스 금지!
		
		// pos in list. much faster than map.
		Position m_ACCESSED_ONLY_BY_SendReadySockets_PositionInList;
		// 검사용
		CSendReadySockets* m_ACCESSED_ONLY_BY_SendReadySockets_Owner;

	private:
		// 가장 마지막에 TCP 스트림을 받은 시간
		// 디스 여부를 감지하기 위함
		// 양쪽 호스트가 모두 감지해야 한다. TCP는 shutdown 직후 closesocket을 호출해도 상대가 못 받을 가능성이 있기 때문이다.
		//int64_t m_lastReceivedTime;

		// 그리고 이 객체에 대응 remote 객체가 중간에 사라지거나 다른 것으로 바뀌는 경우 멀티스레드에서 위험한 코딩이 될 수 있다.
		// 이를 예방하기 위해, CNetCoreImpl는 절대 파괴되지 않는 객체 가령 main 객체가 된다.
		// 그러나 이렇게 되면 이 객체에서 발생하는 이벤트가 어느 remote에 대한 것인지 구별하기 어렵다.
		// 이를 위해 CNetCoreImpl.m_socketToHostMap가 쓰임. 그러나 ABA problem을 막기 위해 serial number가 필요.
		intptr_t m_serialNumber;

		// TCP or UDP
		SocketType m_socketType;

		//////////////////////////////////////////////////////////////////////////
		// TCP 전용

		// 송신큐
		// send queue lock으로 보호된 후에 억세스해야 함
		CHeldPtr<CTcpSendQueue> m_sendQueue_needSendLock;

		// 수신큐
		// AddToSendQueue계열 함수가 다루는 변수가 아니므로 send queue lock과 무관함.
		CHeldPtr<CStreamQueue> m_recvStream;


		//////////////////////////////////////////////////////////////////////////
		// UDP 전용
	private:
		// 송신큐
		// 이게 queue가 아니고 map인 이유 - coalesce를 하기 위함
		// send queue lock으로 보호된 후에 억세스해야 함
		CHeldPtr<CUdpPacketFragBoard> m_udpPacketFragBoard_needSendLock;

		// 수신큐
		// AddToSendQueue계열 함수가 다루는 변수가 아니므로 send queue lock과 무관함.
		CHeldPtr<CUdpPacketDefragBoard> m_udpPacketDefragBoard;

		// 송신큐로부터 꺼내온 항목.
		// 로컬 변수처럼 쓰임. 이 안의 msgbuffer가 계속 재사용되어야 메모리 재할당을 최소화하니까.
		// m_udpPacketFragBoard보다 나중에 선언되어야 함! 이게 먼저 파괴되어야 하므로.
		CHeldPtr<CUdpPacketFragBoardOutput> m_sendIssuedFragment;
	public:
		// ReliableUDP packetLoss Test 를 위해서. 패킷 로스율.
		// data race가 허용되므로 send queue lock으로 보호할 필요 없음.
		int m_packetTruncatePercent;
	private:
		// ReliableUDP packetLoss Test 를 위해서. Proud::Random 의 경우 seed 값을 셋팅할 수 없다. CRandom 으로 변경.
		// send queue lock으로 보호된 후에 억세스해야 함
		Proud::Random m_packetTruncateRandom_needSendLock;

		/*위 send queue lock 후 건드리는 부분을 찾아, send queue lock에 대해 assert(IsLockedByCurrentThread)를 넣읍시다. 유지보수 중 방어 코딩 차원에서. */

		// 만약 이 socket이 invalid->restore를 막 했으면 그때만 true이다.
		//bool m_justRestored; => 리눅스 포팅 다 끝나면 P2P port 재사용 기능은 나중에 다시 다룰 것임. 별도 노트로도 이미 남겨놨음.

		//////////////////////////////////////////////////////////////////////////
		// 각 super socket은 sender의 addrPort를 근거로 remote hostID를 가지는 map을 갖도록 하자.

		// 수신된 UDP 데이터의 송신자 주소 즉 recvfrom으로 받은 주소가, 어느 HostID를 가졌는지에 대한 map.
		// filter tag 처리를 하는 용도로 쓰임.
		CFastMap2<AddrPort, HostID, int> m_receivedAddrPortToVolatileHostIDMap;

	public:
		 PROUD_API CSuperSocket(CNetCoreImpl *owner, SocketType socketType/*CThreadPoolImpl* ownerThreadPool, AddrPort remoteAddr*/);
		 PROUD_API ~CSuperSocket();

		 PROUD_API static SuperSocketCreateResult New(CNetCoreImpl* owner, const shared_ptr<CFastSocket>& fastSocket, SocketType socketType);
		 PROUD_API static SuperSocketCreateResult New(CNetCoreImpl* owner, SocketType socketType);

		PROUD_API void SetEnableNagleAlgorithm(bool enableNagleAlgorithm);
		PROUD_API  void AddToSendQueueWithoutSplitterAndSignal_Copy(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			const CSendFragRefs &sendData);
		 PROUD_API void AddToSendQueueWithSplitterAndSignal_Copy(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			const CSendFragRefs &sendData,
			const SendOpt& sendOpt,
			bool simplePacketMode = false);
		 PROUD_API void AddToSendQueueWithSplitterAndSignal_Copy(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			HostID finalDestHostID,
			FilterTag::Type filterTag,
			AddrPort sendTo,
			const CSendFragRefs &sendData,
			int64_t addedTime,
			const SendOpt &sendOpt);
		 PROUD_API void AddToSendQueueWithSplitterAndSignal_Copy(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			HostID finalDestHostID,
			FilterTag::Type filterTag,
			AddrPort sendTo,
			CMessage &msg,
			int64_t addedTime, // caller가 멀티캐스트를 하는 경우 이 함수를 자주 호출한다. 그때 GetPreciseCurrentTimeMs를 매번 호출하는 부하를 막으려고 이 변수가 있다.
			const SendOpt &sendOpt);

		// this가 server side websocket인 경우에 한해서 호출된다.
		 PROUD_API virtual void AddToSendQueueWithoutSplitterAndSignal_Copy_WebSocket(
			const CSendFragRefs& /*sendData*/) {  // 보낼 데이터
			assert(0); // 상속 함수에서나 호출해야 함
		}

	private:
		void MustTcpSocket();
		void RefreshLastReceivedTime();

	public:

		// destAddr이 가리키는 수신자에게 쏠 송신큐에 쌓인 UDP 패킷 수를 구k한다.
		uint32_t GetUdpSendQueuePacketCount(const AddrPort &destAddr);
		// destAddr이 가리키는 수신자에게 쏠 송신큐가 비었는가?
		bool IsUdpSendBufferPacketEmpty(const AddrPort &destAddr);

		PROUD_API int GetUdpSendQueueLength(const AddrPort &destAddr);
		int GetUdpSendQueueDestAddrCount();

		void ResetPacketFragState();

		PROUD_API bool RequestStopIo();
#if !defined(_WIN32)
		SocketErrorCode SetNonBlockingAndConnect(const AddrPort& hostAddrPort);
		bool NonBlockSendAndUnlock(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			bool calledByIoEvent, CriticalSectionLock& socketLock, CIoEventStatus &comp);
		void NonBlockRecvAndProcessUntilWouldBlock(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			CIoEventStatus &comp);
#else
	private:
		void ProcessOverlappedSendCompletion(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			CIoEventStatus &comp);
		void ProcessOverlappedReceiveCompletion(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			CIoEventStatus &comp);
	public:
		void IssueSendOnNeedAndUnlock(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			bool calledBySendCompletion, CriticalSectionLock& socketLock);
	private:
		SocketErrorCode IssueSendOnNeed_internal(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			bool calledBySendCompletion, ProcessType& outNextProcess);
	public:
		PROUD_API void IssueRecv(
			const shared_ptr<CSuperSocket>& param_shared_from_this,
			bool firstRecv = false);

		PROUD_API SocketErrorCode IssueAccept(bool calledBySendCompletion);
		SocketErrorCode IssueConnectEx(AddrPort hostAddrPort);
#endif
		PROUD_API void DoForLongInterval(int64_t currTime, uint32_t &queueLength);
		void DoForShortInterval(int64_t currTime);
		//int GetBrakedSendAmount(double currTime);
		bool RefreshLocalAddr();
		PROUD_API AddrPort GetLocalAddr();

		bool StopIoRequested();
		PROUD_API virtual bool StopIoAcked();


/*//3003!!!
		String m_tagName;*/




		inline CriticalSection& GetSendQueueCriticalSection(){ return m_sendQueueCS; }
		inline CriticalSection& GetCriticalSection(){ return m_cs; }

#ifdef PN_LOCK_OWNER_SHOWN
		inline bool IsSendQueueLockedByCurrentThread()
		{
			return Proud::IsCriticalSectionLockedByCurrentThread(GetSendQueueCriticalSection());
		}

		virtual bool IsLockedByCurrentThread() { return Proud::IsCriticalSectionLockedByCurrentThread(m_cs); }
#endif

		PROUD_API void SetCoalesceInterval(AddrPort addr, int interval);
		intptr_t GetSerialNumber() { return m_serialNumber; }

		// udpsocket_s
//		NamedAddrPort GetRemoteIdentifiableLocalAddr(CHostBase* rc);

//		void RequestReceiveSpeedAtReceiverSide_NoRelay(AddrPort dest);
		int GetOverSendSuspectingThresholdInBytes();
		int GetMessageMaxLength();

		int GetSendQueueLength() { return m_sendQueue_needSendLock->GetLength(); }

		int GetTcpSendQueueLength() { return m_sendQueue_needSendLock->GetTotalLength(); }
		PROUD_API uint32_t GetUdpSendQueueLength();

		uint64_t GetTotalIssueSendBytes() { return m_totalIssueSendBytes; }
		SocketType GetSocketType() { return m_socketType; }
		const shared_ptr<CFastSocket>& GetSocket() { return m_fastSocket; }
		CUdpPacketFragBoard* GetUdpPacketFragBoard() { return m_udpPacketFragBoard_needSendLock; }

		PROUD_API int64_t GetRecentReceiveSpeed(AddrPort src);
		PROUD_API void SetReceiveSpeedAtReceiverSide(AddrPort dest, int64_t speed, int packetLossPercent, int64_t curTime);
		void SetTcpUnstable(int64_t curTime, bool unstable);

		HostID ReceivedAddrPortToVolatileHostIDMap_Get(const AddrPort& senderAddr);
		PROUD_API void ReceivedAddrPortToVolatileHostIDMap_Set(const AddrPort& senderAddr, HostID remoteHostID);
		PROUD_API void ReceivedAddrPortToVolatileHostIDMap_Remove(const AddrPort& senderAddr);

		PROUD_API int GetUnreliableMessagingLossRatioPercent(AddrPort& senderPort);

		void SetSocketBlockingMode(bool flag);
		void SetSocketVerboseFlag(bool flag);
		int GetPacketQueueTotalLengthByAddr(AddrPort addr);

		PROUD_API void UdpPacketFragBoard_Remove(AddrPort addrPort);
		PROUD_API void UdpPacketDefragBoard_Remove(AddrPort addrPort);

		 PROUD_API SocketErrorCode Bind();
		 PROUD_API SocketErrorCode Bind(int port);
		 PROUD_API SocketErrorCode Bind(const PNTCHAR* addr, int port);
		 PROUD_API SocketErrorCode Bind(AddrPort localAddr);
	private:

		ProcessType GetNextProcessType_AfterRecv(const CIoEventStatus& comp);
		ProcessType GetNextProcessType_AfterSend(const CIoEventStatus& comp, SocketErrorCode& outError);

		ProcessType ExtractMessage(const CIoEventStatus& comp,
								   CReceivedMessageList& outMsgList,
								   ErrorInfoPtr& outErrInfo);

		void BuildDisconnectedErrorInfo(ErrorInfo& output,
			IoEventType type,
			int ioLength,
			SocketErrorCode errorCode);

	public:
		//////////////////////////////////////////////////////////////////////////
		// IIoEventContext 구현
#if defined (_WIN32)
		PROUD_API virtual void OnSocketIoCompletion(
			void* param_shared_from_this_ptr,
			CIoEventStatus &comp);
#else
		PROUD_API virtual void OnSocketIoAvail(
			void* param_shared_from_this_ptr,
			CIoEventStatus &comp);
#endif
		void SetCoalesceInteraval(const AddrPort &destAddr, int coalesceIntervalMs);
		//////////////////////////////////////////////////////////////////////////
		// ACR 연결 유지 기능

	private:
		/* 이 socket이 갖고 있는 AMR 객체.
		수신미보장 부분(local to remote)을 가진다.
		또한, 상대로부터 ack받은 최종 메시지+1 값(remote to local)을 보유한다.
		send queue lock으로 보호된다. syscall 중에 보호되면 너무 많은 시간이 병목이 되므로.
		스레딩 관련 자세한 것은 ACR 명세서에.
		주의: send queue lock으로 보호된 채 액세스할 것.
		*/
		RefCount<CAcrMessageRecovery> m_acrMessageRecovery;

/*

		// 3003 에러 잡기 위해 추가
		int m_receiveCount = 0;*/

	public:
		//************************************
		// ※ 경고 : Send Queue를 테스트 하기 위한 용도로 정의된 값입니다.
		// "m_test_isSendQueueLock = true"이면
		// Queue에서 Pop 하지 않고 실질적으로 데이터를 전송하지 않는다.
		// Send Queue에만 데이터가 계속 쌓이게 된다.
		//************************************
		static bool m_test_isSendQueueLock;
		
		/* RequestStopIo를 호출한 시간.
		unix에서 RMI 3003 문제 해결하기, 즉 "최후의 스레드가 액세스하고 나면 그제서야 socket을 지운다. fd값이 재사용되는 문제를 막기 위해"를
		해결하기 위해, Garbaged socket은 바로 close socket을 하지 않아 왔다. 
		하지만 만일에 버그로 인해 이것이 영원히 socket을 안 닫을 경우, 최악의 경우 socket fd가 계속 누적되는 상황이 될 수 있다.
		이에 대한 방어코드 목적으로, 10초같은 매우 큰 시간동안 garbaged socket이 안 닫히면 강제로 닫는다.
		이를 위해서 사용된다. */
		int64_t m_requestStopIoTime;
		
		bool m_warnTooLongGarbage;

#ifndef _WIN32
		// 이 socket이 epoll에 level trigger mode로 등록되어 있으면 true.
		// socket을 소유한 스레드가 죽어, 다른 스레드의 epoll에 재등록할 때
		volatile bool m_isLevelTrigger;
#endif

		//************************************
		// Method:    Test_IsSendQueueLock
		// Returns:   bool
		// Qualifier: 리턴값이 true이면 실제 데이터를 전송하지 않고 Queue에 계속 쌓인다
		//************************************
		static bool Test_IsSendQueueLock()
		{
			return m_test_isSendQueueLock;
		}

		//************************************
		// Method:    Test_SetSendQueueLock
		// FullName:  Proud::CSuperSocket::Test_SetSendQueueLock
		// Access:    public static
		// Returns:   void
		// Qualifier: Send Queue Lock을 설정/해제 함수, isLock에 true로 하면 그 이후 부터 Send Queue에 데이터가 쌓이게 된다.
		// Parameter: bool isLock
		//************************************
		static void Test_SetSendQueueLock(bool isLock)
		{
			m_test_isSendQueueLock = isLock;
		}

	public:
		PROUD_API static void AcrMessageRecovery_MoveSocketToSocket(shared_ptr<CSuperSocket> oldSocket, const shared_ptr<CSuperSocket>& newSocket);
		PROUD_API void AcrMessageRecovery_ResendUnguarantee(
			const shared_ptr<CSuperSocket>& param_shared_from_this);
		PROUD_API bool AcrMessageRecovery_PeekMessageIDToAck(int* output);
		PROUD_API void AcrMessageRecovery_RemoveBeforeAckedMessageID(int messageID);
		PROUD_API bool AcrMessageRecovery_ProcessReceivedMessageID(int messageID);
		PROUD_API void AcrMessageRecovery_Init(int LocalToRemoteNextMessageID, int RemoteToLocalNextMessageID);

		//////////////////////////////////////////////////////////////////////////

	public:
		void CanDeleteNow_DumpStatus();

		// 단선 감지를 위해 서버와 클라이언트가 공통으로 사용하는 더피패킷 송신 로직
		PROUD_API void SendDummyOnTcpOnTooLongUnsending(int64_t currTime);
	};

	class CSuperSocketArray : public CFastArray<shared_ptr<CSuperSocket>, true, false, int> {};

#if 0
	template<typename T>
	inline bool operator== (shared_ptr<CSuperSocket> const& a, const shared_ptr<T>& b)
	{
		return a.get() == b.get();
	}

	template<typename T>
	inline bool operator!= (shared_ptr<CSuperSocket> const& a, const shared_ptr<T>& b)
	{
		return a.get() != b.get();
	}
#endif

}
