/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/NetClientInterface.h"
//#include "TypedPtr.h"
//#include "TimeAlarm.h"
#include "ReliableUDP.h"
#include "SendOpt.h"
#include "ISendDest_C.h"
#include "SuperSocket.h"
#include "RemoteBase.h"
#include "P2PGroup_C.h"
#include "../include/CryptoRsa.h"
#include "../include/Crypto.h"
#include "../include/MilisecTimer.h"

namespace Proud
{
	class CNetClientImpl;
	class CReceivedMessageList;
	class CReceivedMessage;
	class CSendFragRefs;
	class CRemotePeerReliableUdp;

	class CRemotePeer_C;

	// 이 remote peer에 직접 p2p 연결 시도를 상태를 담은 객체
	class CP2PConnectionTrialContext
	{
		// 어느 remote peer를 위한 context인지?
		CRemotePeer_C *m_owner;

		// 이 객체의 수명을 재기 위함
		int64_t m_startTime;
	public:
		enum State { S_ServerHolepunch, S_PeerHolepunch };
		class StateBase
		{
		public:
			virtual ~StateBase() {}
			State m_state; // dynamic_cast 즐
		};

		class ServerHolepunchState :public StateBase
		{
		public:
			int64_t m_sendTimeToDo;
			int m_ackReceiveCount;
			Guid m_holepunchMagicNumber;

			ServerHolepunchState()
			{
				m_state = S_ServerHolepunch;
				m_ackReceiveCount = 0;
				m_sendTimeToDo = 0;
				m_holepunchMagicNumber = Guid::RandomGuid();
			}
		};

		class PeerHolepunchState :public StateBase
		{
		public:
			int64_t m_sendTimeToDo;
			int m_sendTurn;
			int m_ackReceiveCount;
			Guid m_holepunchMagicNumber;
			int m_offsetShotgunCountdown;
			uint16_t m_shotgunMinPortNum;

			PeerHolepunchState()
			{
				m_shotgunMinPortNum = 1023;
				m_offsetShotgunCountdown = CNetConfig::ShotgunTrialCount;
				m_state = S_PeerHolepunch;
				m_ackReceiveCount = 0;
				m_sendTimeToDo = 0;
				m_sendTurn = 0;
				m_holepunchMagicNumber = Guid::RandomGuid();
			}
		};
	private:
		static uint16_t AdjustUdpPortNumber(uint16_t portNum);
	public:
		CHeldPtr<StateBase> m_state;
	private:
		AddrPort GetServerUdpAddr();

		void SendPeerHolepunch(AddrPort ABSendAddr, Guid magicNumber);
		void SendPeerHolepunchAck(AddrPort BASendAddr, Guid magicNumber, AddrPort ABSendAddr, AddrPort ABRecvAddr);
	public:
		bool Heartbeat();

		CNetClientImpl *GetClient();

		CP2PConnectionTrialContext(CRemotePeer_C *owner);

		AddrPort GetExternalAddr();
		AddrPort GetInternalAddr();

		static void ProcessPeerHolepunch(CNetClientImpl *main, CReceivedMessage &ri);
		static void ProcessPeerHolepunchAck(CNetClientImpl *main, CReceivedMessage &ri);

		void ProcessMessage_PeerUdp_ServerHolepunchAck(CReceivedMessage &ri, Guid magicNumber, AddrPort addrOfHereAtServer, HostID peerID);
	};

	typedef RefCount<CP2PConnectionTrialContext> CP2PConnectionTrialContextPtr;

	class IDisposeWaiter
	{
	public:
		static inline bool IsDisposeSafe(IDisposeWaiter& dw, int64_t currentTimeMs) {
			return dw.CanDispose() || dw.IsTimeout(currentTimeMs);
		}
		virtual bool IsTimeout(int64_t currentTimeMs) const = 0;
		virtual bool CanDispose() const = 0;
		virtual ~IDisposeWaiter(){}
	};

	class CRemotePeer_C
		: public CHostBase
		, public IP2PGroupMember
	{
	public:
		/* P2P member leave RMI를 받았지만
		그것에 대한 event callback이 아직 미처리된 것들이 있으면,
		remote peer가 garbage collect가 미뤄지게 한다. */
		class CDisposeWaiter_LeaveEventCount : public IDisposeWaiter
		{
			CNetClientImpl* m_owner;
			int m_leaveEventCount;

		public:
			CDisposeWaiter_LeaveEventCount(CNetClientImpl* owner);
			virtual bool IsTimeout(int64_t currentTimeMs) const PN_OVERRIDE;
			virtual bool CanDispose() const PN_OVERRIDE;
			void Increase();
			void Decrease();
		};

		/* P2P member join RMI를 받았지만
		그것에 대한 P2P connection recycle complete RMI를 아직 못 받은 것들이 있으면,
		remote peer가 garbage collect가 미뤄지게 한다. */
		class CDisposeWaiter_JoinProcessCount : public IDisposeWaiter
		{
			CNetClientImpl* m_owner;
			int m_count;
			int64_t m_lastStartTime;

		public:
			CDisposeWaiter_JoinProcessCount(CNetClientImpl* owner);
			virtual bool IsTimeout(int64_t currentTimeMs) const PN_OVERRIDE;
			virtual bool CanDispose() const PN_OVERRIDE;
			void Increase();
			void Decrease();
		};

		// 예전에는 Dispose waiter가 있었으나, 이제는 삭제됨.
		// remote peer 객체는 더 이상 안 쓰이더라도 수십초간 살아있는 방식으로 바뀌었기 때문이다.
		// remote peer는 수십초가 지났고, 더 이상 처리할 user work item이 없으면 소멸된다.


		/* per-peer UDP socket
		- UDP socket 생성은 CRemotePeer_C 생성 후 1~2초 뒤에 하도록 한다.
		(바로 만들면 지나친 소켓 생성/종료 문제가 야기되므로)
		UDP 소켓은 CRemotePeer_C.dtor까지 계속 사용된다. */
		CSuperSocketPtr m_udpSocket;

		// Relay 강제 플래그(이 플래그값이 true 이면 P2P 홀펀칭 결과보다 우선한다/defaut값 : false)
		bool m_forceRelayP2P;

		// 활성화 상태 즉 정상 사용중이면 false.
		// 최후의 P2P member join으로 인해, 재사용을 기대하며, 좀비 상태가 된 경우 true.
		// 수십초동안 true를 유지하면 결국 재사용 불가로 판단하여(홀펀칭 증발 등) 그냥 버려진다.
		// 홀펀칭이 안되어 있더라도 가령 아직 릴레이라 하더라도 P2P member join-leave를 자주 할 경우
		// 잦은 P2P 암호키 생성은 비효율적이므로, 역시 비활성화 상태로 만들어 수십초간 보관한다.
		bool m_recycled;

		// expire되는 시간. m_recycled=true일때만 유효하다.
		// 현재 시간이 이 시간을 넘으면 remote peer는 garbage 처분된다.
		int64_t m_recycleExpirationTime;

		// JIT P2P 기능.
		// 릴레이 모드인 피어에게 사용자가 P2P 메시징이나 시간 얻기 등을 걸면, 홀펀칭 하라는 조건이 만족된다.
		// 이건 그걸 발동시키는 시발점이다.
		bool m_jitDirectP2PNeeded;

		// 서버에게 'p2p를 하고자 한다'는 메시지를 딱 1번만 보내야 한다.
		// 그것을 위한 플래그.
		// 아직 홀펀칭을 개시하지는 않는다. 그건 m_newP2PConnectionNeeded 담당.
		bool m_jitDirectP2PTriggered;

		// 서버에서 '그래 니네 둘다 홀펀칭해라'라는 메시지가 오면 이게 true가 된다.
		// 그제서야 홀펀칭을 시작한다. 즉 CP2PConnectionTrialContext를 생성한다.
		bool m_newP2PConnectionNeeded;

		// 홀펀칭 상태기계. 처음에는 클라-서버 홀펀칭 후 외부 주소 노출되면 피어간 홀펀칭을 한다.
		// 홀펀칭 완료되면 사라진다.
		CP2PConnectionTrialContextPtr m_p2pConnectionTrialContext;

		// P2P hole punch 과정에서 잘못된 3rd peer로부터의 홀펀칭에 대한 에코를 걸러내기 위함
		Guid m_magicNumber;

		// NetClient가 해당 peer에게 realudp로 보낸 패킷 갯수
		uint32_t m_toRemotePeerSendUdpMessageTrialCount;
		// NetClient가 해당 peer에게 realudp로 받은(성공한) 패킷 갯수
		int m_toRemotePeerSendUdpMessageSuccessCount;

		// 내가(NetClient)가 이 peer에게 받은 패킷 갯수
		int m_receiveudpMessageSuccessCount;

		// P2P unreliable 송수신을 위한 객체
		class CUdpLayer
		{
			CRemotePeer_C *m_owner;
		public:
			CUdpLayer(CRemotePeer_C *owner)
			{
				m_owner = owner;
			}
			void SendWithSplitter_Copy(const CSendFragRefs &sendData, const SendOpt &sendOpt);
			int GetUdpSendBufferPacketFilledCount();
			bool IsUdpSendBufferPacketEmpty();
		};

		// recentPing과 같지만 reliableUdp로 측정된 핑
		int m_recentReliablePingMs;

		// 측정된 송신 대기량(UDP)
		int m_sendQueuedAmountInBytes;

		// 이 peer와 server와의 랙
		int m_peerToServerPingMs;

		// 이 peer 와 server 와의 packet loss
		int m_CSPacketLossPercent;

		// 이 UDP peer와 통신을 할 전용 소켓을 생성할 시간
		int64_t m_udpSocketCreationTimeMs;

		int64_t GetRenewalSocketCreationTimeMs();
		// 가장 마지막에 peer로부터 UDP 패킷을 direct로 받은 시간
		// relay fallback을 위한 검증 목적
		// direct p2p mode일때만 유효한 값이다.
		PROUDNET_VOLATILE_ALIGN int64_t m_lastDirectUdpPacketReceivedTimeMs;
		PROUDNET_VOLATILE_ALIGN int m_directUdpPacketReceiveCount;
		PROUDNET_VOLATILE_ALIGN int64_t m_lastUdpPacketReceivedInterval;

		/* 이 peer에게 local에서 가진 서버 시간차를 보내는 쿨타임 */
		int64_t m_ReliablePingDiffTimeMs;	// Reliable Ping 을 최근 마지막에 쏜 시간 기록
		int64_t m_UnreliablePingDiffTimeMs;	// Unreliable Ping 을 최근 마지막에 쏜 시간 기록

		// 이 값 대신 GetIndirectServerTimeDiff()를 써서 얻어라!!
		int64_t m_indirectServerTimeDiff;

		// remote peer와의 랙
		// 측정된 적이 있으면 1 이상의 값을, 그렇지 않으면 0을 갖는다.
		// Q: 실제 RTT가 <1인 경우는 어떻게 감지하나요? A: 그때 가서 ms or ns로 바꿉시다. RTT가 0이면 측정된 적 없음을 의미하는 것이 통상적입니다.
		int m_lastPingMs;

		// lastping과 같지만 reliableudp로 측정된 핑
		int m_lastReliablePingMs;

		// recent 혹은 last ping이 제대로 측정되었는지에 대한 여부 ( 릴레이핑여부 포함 )
		bool m_p2pPingChecked;

		// reliable ping 이 제대로 측정되었는가
		bool m_p2pReliablePingChecked;

		// remote Peer의 프레임 레이트
		double m_recentFrameRate;

		CSessionKey m_p2pSessionKey;
		CryptCount m_encryptCount, m_decryptCount;

		//HostID m_HostID;

		AddrPort m_UdpAddrFromServer;	// 상대 피어(이 호스트가 아니라!!!)가 서버와의 홀펀칭 후 얻어진 서버측에서 인식된 상대 피어의 외부 주소.
		AddrPort m_UdpAddrInternal; // 이건 상대 피어 내부(이 호스트가 아니라!!!)에서 인식된 내부주소

		/*		AddrPort m_UdpAddrInternal_old;
		AddrPort m_UdpAddrFromServer_old;
		AddrPort m_P2PHolepunchedLocalToRemoteAddr_old;
		AddrPort m_P2PHolepunchedRemoteToLocalAddr_old;*/


		// local에서 remote로 전송 가능한 홀펀칭된 주소 & remote에서 local로 전송 가능한 홀펀칭된 주소
		AddrPort m_P2PHolepunchedLocalToRemoteAddr, m_P2PHolepunchedRemoteToLocalAddr;

		P2PGroups_C m_joinedP2PGroups;

		// true이면 P2P가 server를 통해 우회하며 직접 P2P 가 안됨을 의미
		bool m_RelayedP2P_USE_FUNCTION;

		int64_t m_RelayedP2PDisabledTimeMs;

		// directp2p -> relay 로 바뀔때 last ping을 다시 계산하기 위한 조건 default true
		bool m_setToRelayedButLastPingIsNotCalulcatedYet;

		int m_repunchCount; // 총 re-holepunch 횟수
		int64_t m_repunchStartTimeMs; // 0이면 시작하지 말라는 뜻

		int64_t m_lastCheckSendQueueTimeMs;
		int64_t m_sendQueueHeavyStartTimeMs;

		bool IsRelayConditionByUdpFailure(int64_t currTime);
		bool IsRelayConditionByReliableUdpFailure();
		void SetRelayedP2P(bool flag);

		bool IsRelayMuchFasterThanDirectP2P(int serverUdpRecentPingMs, double forceRelayThresholdRatio) const;

		// 상대 피어와 reliable 메시징에 사용되는 reliable UDP 계층.
		// 피어와 direct or relayed P2P 모든 경우에 계속 사용된다. direct에서 relay로 바뀌는 순간에 스트림 유실을 하면 안되기 때문이다.
		CRemotePeerReliableUdp m_ToPeerReliableUdp;

		int64_t m_ToPeerReliableUdpHeartbeatLastTimeMs;
		CUdpLayer m_ToPeerUdp;
		CNetClientImpl *m_owner;

		// ctor,dtor
		CRemotePeer_C(CNetClientImpl *owner);
		~CRemotePeer_C();

		void ToNetPeerInfo(CNetPeerInfo &ret);
		inline bool IsBehindNat();
		inline bool IsRelayedP2P() { return m_RelayedP2P_USE_FUNCTION; }

		// 이 remote peer에 직접 p2p 연결 시도를 하는 과정을 시작한다.
		void CreateP2PConnectionTrialContext();

		void Heartbeat(int64_t currTime);

		void NewUdpSocketAndStartP2PHolepunch_OnNeed();
		void FallbackP2PToRelay(bool firstChance, ErrorType reason);
		int64_t GetIndirectServerTimeDiff();

		bool RecycleUdpSocketByHostID(HostID hostID);

		void ReserveRepunch();

		virtual HostID GetHostID() PN_OVERRIDE
		{
			return m_HostID;
		}

			virtual LeanType GetLeanType() PN_OVERRIDE
		{
			return LeanType_CRemotePeer_C;
		}

		AddrPort GetDirectP2PLocalToRemoteAddr() { return m_P2PHolepunchedLocalToRemoteAddr; }
		void GetDirectP2PInfo(CDirectP2PInfo& output);

		void UngarbageAndInit(CNetClientImpl *owner);
		bool IsSameLanToLocal();
		/*		bool IsDirectP2PInfoDifferent();
		void BackupDirectP2PInfo();*/

	};
}
