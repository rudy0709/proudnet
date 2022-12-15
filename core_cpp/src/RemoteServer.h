#pragma once

#include "SuperSocket.h"
#include "RemoteBase.h"
#include "ISendDest_C.h"
#include "FallbackParam.h"


namespace Proud
{
	/* 서버와의 연결에 대한 host 객체.
	- 서버로부터 온 RMI나 서버와 관련된 event callback을 담당한다.
	- to-server TCP socket과 lifetime이 비슷하지만 동일하지는 않다. */
	class CRemoteServer_C
		: public CHostBase,
		public IP2PGroupMember,
		public enable_shared_from_this<CRemoteServer_C>
	{
	public:

		// socket, event 등의 객체가 같은 lifetime을 가지므로 이렇게 뜯어냈다.
		class CFallbackableUdpLayer_C
		{
			CRemoteServer_C* m_owner;
		public:
			Guid m_holepunchMagicNumber;
			// 클라에서 UDP 소켓을 생성하고(JIT UDP) 서버에 UDP 소켓 생성을 요청해서 응답을 대기중일 때만 true이다.
			volatile bool m_serverUdpReadyWaiting;

			// true이면, UDP가 실제로 작동중. false이면 TCP로 우회되고 있음. 홀펀칭 성공 전이라.
			volatile bool m_realUdpEnabled_USE_FUNCTION;
			// 서버와의 UDP 홀펀칭이 된 시간.
			volatile int64_t m_realUdpEnabledTimeMs;

			AddrPort m_serverAddr;

			// 서버 홀펀칭 시도 타임. 초기에는 영원한 미래가 지정됨
			volatile int64_t m_holepunchTimeMs;

			// 서버 홀펀칭 시도 횟수. 홀펀칭이 안 될 놈이 영원히 하는 것을 방지용.
			volatile int m_holepunchTrialCount;

			// UDP->TCP 후퇴한 횟수.
			// 일정량 이상 후퇴하게 되면, 이후부터는 홀펀칭을 더 하지 않는다. 쓸데없는 통신 낭비니까.
			volatile int m_fallbackCount;

			// 서버로부터 가장 마지막에 UDP로 패킷을 받은 시간
			// TCP fallback을 안한 경우에만 유효한 값이다.
			volatile int64_t m_lastServerUdpPacketReceivedTimeMs;
			volatile int m_lastServerUdpPacketReceivedCount;
			volatile int64_t m_lastUdpPacketReceivedIntervalMs; // 디버깅용

#ifdef UDP_PACKET_RECEIVE_LOG
			CFastArray<int64_t> m_lastServerUdpPacketReceivedQueue;
#endif // UDP_PACKET_RECEIVE_LOG

		public:

			void SendWithSplitterViaUdpOrTcp_Copy(HostID hostID, const CSendFragRefs& sendData, const SendOpt &sendOpt);
			bool IsRealUdpEnabled() { return m_realUdpEnabled_USE_FUNCTION; }
			void Set_RealUdpEnabled(bool flag);

			CFallbackableUdpLayer_C(CRemoteServer_C* owner);
		};

		typedef shared_ptr<CFallbackableUdpLayer_C> CFallbackableUdpLayerPtr_C;
// 		to - server의 final user work item을 위해 :
// 		NC, LC.Connect()에서는 m_candidateHosts에 new CRemoteServer_C or CLanRemoteServer_C를 add합시다.
// 			그리고 서버로부터 HostID를 발급받으면 m_candidateHosts로부터 m_authedHosts로 그걸 옮기면 됩니다.
//		 m_toServerTcp, m_toServerUdp_Fallbackable이 여기로 옮겨져야 하겠죠.
	public:
		CNetClientImpl* m_owner;
		virtual CriticalSection& GetOwnerCriticalSection() PN_OVERRIDE;
	private:
		CFallbackableUdpLayerPtr_C m_ToServerUdp_fallbackable;
	public:
		/* PN은 일반적인 TCP의 graceful shutdown (양쪽 모두 shutdown 호출 후 close하는 방식)이 아니라,
		NetC2S, NetS2C의 Shutdown,ShutdownACk RMI를 이용해서 shutdown 과정을 수행한다.
		이를 shutdown RMI 과정이라고 지칭하자.

		Q: 왜 이렇게 하나요?
		A: 사용자가 RMI를 호출하자마자 NetClient.Disconnect or NetServer.CloseConnection를 호출할 경우
		호출했던 RMI가 상대방에게 반드시 전송되게 하기 위해서다.
		TCP의 graceful shutdown 방식은 그렇게 되지 못한다. IP packet ordering이 어긋날 경우 precede되어 버리기 때문이다.

		대체로 shutdown RMI가 주고받아지고 나서 양측의 TCP socket을 닫으면 되겠지만,
		행여나 한쪽이 예외가 생겨 그 과정도 못하고 프로세스가 죽을 수 있다.
		이러면 나머지 한쪽은 오랫동안 멍 때리게 된다.
		이를 막기 위해 shutdown RMI가 일단 주고받아지면,
		일정 시간 안에 shutdown RMI가 못 끝나더라도 강제로 TCP close socket을 해야 한다.

		이 변수는 그러한 목적으로 사용된다.

		평소에는 0이고, shutdown RMI 과정이 시작하면 시작한 시간이 기재된다.
		이때에는 PN RMI를 제외하고 송신 금지.
		*/
		int64_t m_shutdownIssuedTime;

		shared_ptr<CSuperSocket> m_ToServerTcp;
		shared_ptr<CSuperSocket> m_ToServerUdp;

		// To-server UDP socket을 생성하는 것이 실패한 적이 있는지?
		bool m_toServerUdpSocketCreateHasBeenFailed;
	public:
		CRemoteServer_C(CNetClientImpl* owner, shared_ptr<CSuperSocket> toServerTcp = shared_ptr<CSuperSocket>());
		~CRemoteServer_C();

		CFallbackableUdpLayerPtr_C GetToServerUdpFallbackable(){ return m_ToServerUdp_fallbackable; }
		void SetToServerUdpFallbackable(AddrPort addrPort);

		bool IsRealUdpEnable();

		AddrPort Get_ToServerUdpSocketLocalAddr();
		void CleanupToServerUdpFallbackable() { m_ToServerUdp_fallbackable = CFallbackableUdpLayerPtr_C(); }

		void ResetUdpEnable();
		bool MustDoServerHolepunch();
		Guid GetServerUdpHolepunchMagicNumber() { return m_ToServerUdp_fallbackable->m_holepunchMagicNumber; }
		AddrPort GetServerUdpAddr() { return m_ToServerUdp_fallbackable->m_serverAddr; }

		bool FallbackServerUdpToTcpOnNeed(int64_t currTime);
		bool FirstChanceFallbackServerUdpToTcp_WITHOUT_NotifyToServer(const FallbackParam &param);
		void UpdateServerUdpReceivedTime();

		void Send_ToServer_Directly_Copy(HostID destHostID, MessageReliability reliability, const CSendFragRefs &sendData2, const SendOpt& sendOpt, bool simplePacketMode);
		void RequestServerUdpSocketReady_FirstTimeOnly();

		int64_t GetIndirectServerTimeDiff() PN_OVERRIDE { return 0; }

		void SetServerUdpReadyWaiting(bool flag) { m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting = flag; }

		virtual HostID GetHostID() PN_OVERRIDE
		{
			return m_HostID;
		}

		virtual LeanType GetLeanType() PN_OVERRIDE
		{
			return LeanType_CRemoteServer_C;
		}
	};

#if 0
	template<typename T>
	inline bool operator==(const shared_ptr<CRemoteServer_C>& a, const shared_ptr<T>& b)
	{
		return a.get() == b.get();
	}

	template<typename T>
	inline bool operator!=(const shared_ptr<CRemoteServer_C>& a, const shared_ptr<T>& b)
	{
		return a.get() != b.get();
	}
#endif

}