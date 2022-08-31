#pragma once

#include "../include/Ptr.h"
#include "../include/AddrPort.h"
//#include "TypedPtr.h"

namespace Proud
{
	using namespace std;

	class CNetServerImpl;
	class CLanServerImpl;
	class CRemoteClient_S;
	class CLanRemoteClient_S;

	// 두 클라의 P2P 연결 상태
	// 릴레이인지, 홀펀칭된 상태이면 그 정보는 뭔지 등등.
	class P2PConnectionState
	{
		CNetServerImpl* m_owner;
		bool m_relayed_USE_FUNCTION;		// 직빵 p2p 연결을 못하는 중인가?

		// 각 클라의 홀펀칭 주소 정보
		struct PeerAddrInfo
		{
			HostID m_ID;
			// 각 클라는 상대 Peer마다 서로 다른 external, internal ADDR을 가지므로 필수
			AddrPort m_externalAddr;
			// 각 클라는 상대 Peer마다 서로 다른 external, internal ADDR을 가지므로 필수
			AddrPort m_internalAddr;
			// NOTE: 여기에 CRemoteClient_S를 넣지말것
			bool	m_memberJoinAcked;

			PeerAddrInfo()
			{
				m_ID = HostID_None;
				m_memberJoinAcked = false;
				m_externalAddr = AddrPort::Unassigned;
				m_internalAddr = AddrPort::Unassigned;
			}
		};

		// 각 클라의 홀펀칭 주소 정보.
		// 재사용되는 경우 보존된다.
		PeerAddrInfo m_peers[2];
	public:
		weak_ptr<CRemoteClient_S> m_firstClient, m_secondClient;

		bool m_jitDirectP2PRequested;  // JIT P2P를 요청받은 적이 있나?
		int m_dupCount;	// 쌍방 peer끼리 P2P group이 몇개 중복됐는가?

		// 자기 자신과의 연결인 경우 p2p connection count에서 제외해야 하므로
		// (참고: 자기 자신과의 연결에 대해서는 UDP 홀펀칭 관련 메시지가 안온다.)
		bool m_isLoopbackConnection;

		/* 예전 버전에서는 session key를 서버에서 만들어 두 peer간 session key를 직접 전송하는 방식이었다.
		그러나 Windows crypto api는 이것을 어렵게 한다. public key만 쌩 blob 추출이 허용되기 때문이다.
		이런 경우 random blob을 만들어서 그것을 클라에 전송하고, 클라는 그것을 crypto hash로 받아들인 후
		CryptDeriveKey를 통해 session key로 만들어 쓰도록 해야 한다.

		물론 서버에서 session key를 만들어 두 peer에게 전송할 수도 있다. 하지만 이런 경우 두 클라가 미리 받아 놓은
		public key를 위한 암호화 과정을 거쳐야 하는데, 이 부하도 만만치 않다.
		그러므로 hash를 만들어 전송한다.

		P2P pair 재사용되는 경우 보존된다. */
		ByteArray m_p2pAESSessionKey;
		ByteArray m_p2pFastSessionKey;

		// P2P pair 재사용되는 경우 보존되지 않는다.
		int m_p2pFirstFrameNumber;

		// P2P 홀펀칭 과정에서 사용될 값.
		// P2P pair 재사용되는 경우 보존된다. join-leave를 자주 반복할 경우 매번 만드는 부하도 무시 못하니까.
		Guid m_magicNumber;

		// 클라에서 보고한, 피어간 ping.
		// 0이면 아직 측정된 적 없음을 의미.
		int m_recentPingMs;

		// p2p 중복 연결 수가 0이 되어서 active pairs로부터 삭제되고 recyclable pairs에 들어간 시간
		// 0이면 recycle 상태가 아님.
		int64_t m_releaseTimeMs;

		// P2P memberjoin - ack 과정이 진행중인지?
		// 두 피어에게 P2P member join RMI를 보냈지만 아직 양쪽 모두로부터 그것의 ack RMI가 다 오지 않았다면 true이다.
		bool m_memberJoinAndAckInProgress;

		// pair간 udp message 시도,성공횟수
		uint32_t m_toRemotePeerSendUdpMessageSuccessCount;
		uint32_t m_toRemotePeerSendUdpMessageTrialCount;
//		int m_clientUdpRecycleSuccessCount = 0;

		bool ContainsHostID(HostID PeerID);
		PROUD_API shared_ptr<CRemoteClient_S> GetOppositePeer(const shared_ptr<CRemoteClient_S>& peer) const;

		bool MemberJoin_IsAckedCompletely();
		void MemberJoin_Acked(HostID peerID);

		void SetRelayed(bool val);
		bool IsRelayed();

		int GetServerHolepunchOkCount();
		void SetServerHolepunchOk(HostID peerID, AddrPort internalAddr,AddrPort externalAddr);
		AddrPort GetInternalAddr(HostID peerID);
		AddrPort GetExternalAddr(HostID peerID);

		void MemberJoin_Start(HostID peerIDA, HostID peerIDB);

		AddrPort GetHolepunchedSendToAddr(HostID peerID);
		AddrPort GetHolepunchedRecvFromAddr(HostID peerID);

		void ClearPeerInfo();

		P2PConnectionState(CNetServerImpl* owner,bool isLoopbackConnection);
		~P2PConnectionState();
	};


	typedef shared_ptr<P2PConnectionState> P2PConnectionStatePtr;

	// LanServer용 P2PConnectionState
	// 무조건 relayed 이다. ( TCP 연결만 가능 )
	class LanP2PConnectionState
	{
		CLanServerImpl* m_owner;

		struct PeerAddrInfo
		{
			HostID m_ID;
			AddrPort m_externalAddr;  // 각 클라는 상대 Peer마다 서로 다른 external ADDR을 가지므로 필수
			// NOTE: 여기에 CLanRemoteClient_S를 넣지말것

			PeerAddrInfo()
			{
				m_ID = HostID_None;
				m_externalAddr = AddrPort::Unassigned;
			}
		};
		PeerAddrInfo m_peers[2];
	public:

		CLanRemoteClient_S *m_firstClient,*m_secondClient;

		int m_dupCount;	// 쌍방 peer끼리 P2P group이 몇개 중복됐는가?

		/* 예전 버전에서는 session key를 서버에서 만들어 두 peer간 session key를 직접 전송하는 방식이었다.
		그러나 Windows crypto api는 이것을 어렵게 한다. public key만 쌩 blob 추출이 허용되기 때문이다.
		이런 경우 random blob을 만들어서 그것을 클라에 전송하고, 클라는 그것을 crypto hash로 받아들인 후
		CryptDeriveKey를 통해 session key로 만들어 쓰도록 해야 한다.

		물론 서버에서 session key를 만들어 두 peer에게 전송할 수도 있다. 하지만 이런 경우 두 클라가 미리 받아 놓은
		public key를 위한 암호화 과정을 거쳐야 하는데, 이 부하도 만만치 않다.
		그러므로 hash를 만들어 전송한다. */
		// 2010.2.17 By LSH - 각각의 리모트 피어의 공개키로 암호화 하여 보내주며, 이전과 같은 헤쉬를 거치지 않는다.
		// 암호화 체제를 AES로 변환 하면서 변경
		ByteArray m_p2pAESSessionKey;
		ByteArray m_p2pFastSessionKey;
		Guid m_magicNumber;	// P2P 인증에서 사용할 값 ( 해킹이나 다른 connection을 방지하기 위함. )

		enum P2PConnectState
		{
			None = 0,			// 초기화
			P2PJoining,			// P2P Member Join중
			P2PConnecting,		// P2P Tcp Connect중
			P2PConnected,		// P2P Connection 완료
		};

		P2PConnectState m_p2pConnectState; // 현재 P2P 연결상태 ( TCP )

		// 클라에서 보고한, 피어간 ping
		int m_recentPingMs;

		void SetExternalAddr(HostID peerID, AddrPort externalAddr);
		AddrPort GetExternalAddr(HostID peerID);

		LanP2PConnectionState(CLanServerImpl* owner);
		~LanP2PConnectionState();
	};

	typedef RefCount<LanP2PConnectionState> LanP2PConnectionStatePtr;
}
