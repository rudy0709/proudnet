#pragma once

#include "../include/Enums.h"

namespace Proud
{
	// 주의: 이것들을 include 폴더로 옮기지 말 것. 해킹에 노출되니까.
	enum MessageType
	{
		MessageType_None = 0,

		MessageType_Rmi = 1,            // 통상 보내는 RMI
		MessageType_UserMessage,		// 엔진 사용자가 정의하는 자유로운 메시지

		MessageType_Hla, // HLA 계열 메시지들
		MessageType_ConnectServerTimedout,

		// S2C 최초 메시지. 서버에서 클라에게 공개키,환경 설정 등을 보낸다.
		MessageType_NotifyStartupEnvironment,
		// C2S 최초 메시지. 서버로부터 받은 공개키를 갖고 세션키 등을 보낸다.
		MessageType_RequestServerConnection,
		// S2C 위 메시지로 대체되었으므로 퇴역되어야 하지만, LC,LS때문에 일단 남김.
		MessageType_NotifyServerConnectionHint,
		// C2S 위 메시지로 대체되었으므로 퇴역되어야 하지만, LC,LS때문에 일단 남김.
		MessageType_NotifyCSEncryptedSessionKey,
		// S2C  위 메시지로 대체되었으므로 퇴역되어야 하지만, LC,LS때문에 일단 남김.
		MessageType_NotifyCSSessionKeySuccess,
		// C2S 위 메시지로 대체되었으므로 퇴역되어야 하지만, LC,LS때문에 일단 남김.
		MessageType_NotifyServerConnectionRequestData,
		// S2C NotifyRequestServerConnection의 응답.
		MessageType_NotifyProtocolVersionMismatch,
		// S2C NotifyRequestServerConnection의 응답.
		MessageType_NotifyServerDeniedConnection,
		// S2C NotifyRequestServerConnection의 응답.
		MessageType_NotifyServerConnectSuccess,

		// ACR(auto connection recovery)
		MessageType_RequestAutoConnectionRecovery, // C2S
		MessageType_NotifyAutoConnectionRecoverySuccess, // S2C
		MessageType_NotifyAutoConnectionRecoveryFailed, // S2C

		// S2C
		MessageType_RequestStartServerHolepunch,
		// C2S UDP
		MessageType_ServerHolepunch,
		// S2C UDP
		MessageType_ServerHolepunchAck,

		// C2S
		MessageType_NotifyHolepunchSuccess,
		// S2C
		MessageType_NotifyClientServerUdpMatched,

		// C2S UDP
		MessageType_PeerUdp_ServerHolepunch,
		// S2C UDP
		MessageType_PeerUdp_ServerHolepunchAck,
		// C2S
		MessageType_PeerUdp_NotifyHolepunchSuccess,

		// P2P or S2C(by relay layer, fake)
		MessageType_ReliableUdp_Frame,

		// C2S
		MessageType_ReliableRelay1,
		MessageType_UnreliableRelay1,
		MessageType_UnreliableRelay1_RelayDestListCompressed,	//MessageType_UnreliableRelay1와 같으나, HostID가 압축되어 있다.

		/* [C2S] ReliableRelay1은 브로드캐스트를 하지만 이건 이미 reliable UDP sender window에 릴레이 모드 전환 전에 이미
		있던 것들을 뒤늦게라도 릴레이로 상대에게 전송하는 역할을 한다.
		참고: reliable하게 가는거니까 ack는 굳이 보내지 않는다. */
		MessageType_LingerDataFrame1,

		// S2C
		MessageType_ReliableRelay2,
		MessageType_UnreliableRelay2,
		MessageType_LingerDataFrame2,

		// C2S
		MessageType_UnreliablePing,  // 서버 시간 동기화, latency 측정, keep-alive, 트래픽 수신 속도 등을 전송
		MessageType_SpeedHackDetectorPing,
		// S2C
		MessageType_UnreliablePong,
		MessageType_ArbitraryTouch,		// UDP 통신량이 한쪽이 일방적으로 많은 경우 UDP가 다른 연결(relay or TCP) fallback을 하는 현상을 막는다.

		// P2P
		MessageType_PeerUdp_PeerHolepunch,
		MessageType_PeerUdp_PeerHolepunchAck,

		MessageType_P2PUnreliablePing,	// 서버 시간 동기화, latency 측정, keep-alive, 트래픽 수신 속도 등을 전송
		MessageType_P2PUnreliablePong,

		// 서버로 부터의 브로드캐스팅 메세지를 Peer간의 릴레이를 통해서 해결하기 위해 추가
		// 서버의 브로드 캐스팅 메세지를 릴레이를 해줄 피어가 받는 메세지 타입
		MessageType_S2CRoutedMulticast1, // S2C
		// 피어를 통해 릴레이된 메세지를 받는다는 의미를 가진 메세지 타입
		MessageType_S2CRoutedMulticast2, //C2C

		// 암호화 계층
		// 예전에는 MessageType_Encrypted이었으나,
		// 1바이트를 더 아끼기 위해 암호 알고리즘 선택을 여기에 합쳐 버렸다.
		// 그래서 늘어난 것임.
		MessageType_Encrypted_Reliable_EM_Secure,
		MessageType_Encrypted_Reliable_EM_Fast,
		MessageType_Encrypted_UnReliable_EM_Secure,
		MessageType_Encrypted_UnReliable_EM_Fast,

		// 압축 계층
		MessageType_Compressed,
		//
		// 		// 수신측에서 계속 산출해왔던 '수신 속도'를 송신측에서 받고자 함 및 리플. C2S,S2C,P2P 모두.
		// 		// 더 이상 안쓰이지만 프로토콜 버전 바뀌는 것 두려워서 아직 안 제거함.
		// 		MessageType_RequestReceiveSpeedAtReceiverSide_NoRelay_DEPRECATED,
		// 		MessageType_ReplyReceiveSpeedAtReceiverSide_NoRelay_DEPRECATED,

		// P2P 연결된 Peer에게 인증관련 데이터를 보낸다.
		MessageType_NotifyConnectionPeerRequestData,
		// C2S 서버에게 인증이 실패했다고 noti한다.
		MessageType_NotifyCSP2PDisconnected,
		// P2P 연결된 피어에게 검증이 성공했다고 알림.
		MessageType_NotifyConnectPeerRequestDataSucess,
		// C2S Peer 커넥션이 완료 되었다.
		MessageType_NotifyCSConnectionPeerSuccess,

		// 수신측은 받아도 그냥 버리는 메시지
		// Windows 2003은 서버에서 한번 보낸 적이 있는 곳으로부터 오는 패킷만 받는다. 따라서 이걸로 구멍을 내준다.
		MessageType_Ignore,

		// c2s connectionHint를 요청한다.
		MessageType_RequestServerConnectionHint,

		MessageType_PolicyRequest,

		// p2p reliable ping 관련 패킷 추가
		MessageType_P2PReliablePing,
		MessageType_P2PReliablePong,

		// ProtocolMismatch가 라이선스 문제일 때도 떠서 다른 원인으로 뜰 때와 구분짓기 위해서 전용 에러메세지를 만듬
		MessageType_NotifyLicenseMismatch,

		// 주의: 이것도 바꾸면 ProudClr namespace의 동명 심볼도 수정해야 한다.

		MessageType_Last,
	};

	// 주의: 이것들을 include 폴더로 옮기지 말 것. 해킹에 노출되니까.
	enum _LocalEventType
	{
		LocalEventType_None = 0,
		LocalEventType_ConnectServerSuccess,
		LocalEventType_ConnectServerFail,	// C/S로 연결하는 과정이 실패했음
		LocalEventType_ClientServerDisconnect, // HostID도 유효한, 즉 연결 잘 된 상태 후 중도 C/S 해제됐음
		LocalEventType_ClientJoinCandidate,
		LocalEventType_ClientJoinApproved,
		LocalEventType_ClientLeaveAfterDispose,            // 서버에 연결되어있던 클라이언트 1개가 나갔음

		// ACR
		LocalEventType_ClientOffline,     // remote client가 TCP가 증발했고 ACR을 진행중인 상태.
		LocalEventType_ClientOnline,
		LocalEventType_ServerOffline,     // 넷클라에서 server와의 TCP가 증발했고 ACR을 진행중인 상태.
		LocalEventType_ServerOnline,
		LocalEventType_P2PMemberOffline,     // 넷클라에서 remote peer가 TCP가 증발했고 ACR을 진행중인 상태.
		LocalEventType_P2PMemberOnline,

		LocalEventType_AddMemberAckComplete,
		LocalEventType_AddMember,
		LocalEventType_DelMember,
		LocalEventType_DirectP2PEnabled,
		LocalEventType_RelayP2PEnabled,
		LocalEventType_GroupP2PEnabled,			// Lan 클섭에서 한 그룹의 P2P connection이 모두완료되면 뜨는 이벤트
		LocalEventType_ServerUdpChanged,
		LocalEventType_SynchronizeServerTime,
		LocalEventType_HackSuspected,
		LocalEventType_TcpListenFail,
		LocalEventType_P2PGroupRemoved,
		LocalEventType_P2PDisconnected,			// P2P 연결이 실패되었음
		LocalEventType_UnitTestFail,
		LocalEventType_Error,
		LocalEventType_Warning,
	};

	// 주의: 이것들을 include 폴더로 옮기지 말 것. 해킹에 노출되니까.
	// http://msdn.microsoft.com/en-us/library/aa366912(VS.85).aspx 에 의하면
	// 사용자가 할당할 수 없는 메모리 공간 주소로서의 값이 이걸로 쓰여야 한다.
	// 왜냐하면 OVERLAPPED 주소 값이 들어갈 곳에 이 값이 쓰이기 때문이다.
	enum CustomValueEvent
	{
		//CustomValueEvent_NewClient = -1, // IOCP에 넣는 custom value는 overlapped 포인터값이다. NULL이어서는 안된다. GQCS쪽에서 T/F 체크대신 overlapped를 체크하니까.
		//CustomValueEvent_NewPeerAccepted = -3, // 새로운 peer가 접속했다.
		//CustomValueEvent_Heartbeat = -4, // 일정 시간마다 PN 내부 처리를 한다.
		CustomValueEvent_OnTick = -5, // 일정 시간마다 사용자 코드 실행을 한다.
		//CustomValueEvent_DoUserTask = -6, 
		//CustomValueEvent_End = -7, //호스트 종료. completion에 종료를 알린다.
		// static assigned UDP socket이 처음 만들어질 때 net worker thread에서 최초의 issue recv를 걸기 위함.
		CustomValueEvent_IssueFirstRecv = -8,
		CustomValueEvent_GarbageCollect = -9, // 20ms 정도 되는 매우 짧은 시간
		CustomValueEvent_Last = -10
	};

	// 주의: 이것들을 include 폴더로 옮기지 말 것. 해킹에 노출되니까.
	// UnityEngine.RuntimePlatform enum과 같아야 함
	enum RuntimePlatform
	{
		RuntimePlatform_UOSXEditor, // U로 시작하는 것들은 Unity3D를 의미
		RuntimePlatform_UOSXPlayer,
		RuntimePlatform_UWindowsPlayer,
		RuntimePlatform_UOSXWebPlayer,
		RuntimePlatform_UOSXDashboardPlayer,
		RuntimePlatform_UWindowsWebPlayer,
		RuntimePlatform_UWiiPlayer,
		RuntimePlatform_UWindowsEditor,
		RuntimePlatform_UIPhonePlayer,
		RuntimePlatform_UXBOX360,
		RuntimePlatform_UPS3,
		RuntimePlatform_UAndroid,
		RuntimePlatform_UNaCl,
		RuntimePlatform_ULinuxPlayer,
		RuntimePlatform_UWebGL = 17,

		RuntimePlatform_Flash = 0x40000001 + RuntimePlatform_UWebGL,
		RuntimePlatform_C,
		RuntimePlatform_NIPhone,	// N으로 시작하는 것들은 Native를 의미
		RuntimePlatform_NAndroid,
		RuntimePlatform_Mobile_LAST,
		RuntimePlatform_Last,

		RuntimePlatform_Count = RuntimePlatform_Last - 0x40000001
	};

	//이 함수는 custom value queue가 따로 있는 이상 더 이상 호출할 일 없다. 지우자.
	// 	inline bool CustomValueEventInRange(intptr_t value)
	// 	{
	// 		// Overlapped Handle 값이 -값이 나올 수 있다.
	// 		// 따라서 여기서 Custom Value 범위값을 체그 해야한다.
	// 		return CustomValueEvent_Last < value && value < 0;
	// 	}

	inline bool MessageType_IsEncrypted(MessageType t)
	{
		return t == MessageType_Encrypted_Reliable_EM_Secure
			|| t == MessageType_Encrypted_Reliable_EM_Fast
			|| t == MessageType_Encrypted_UnReliable_EM_Secure
			|| t == MessageType_Encrypted_UnReliable_EM_Fast;
	}


	enum ReliableUdpFrameType
	{
		ReliableUdpFrameType_None = 0,
		ReliableUdpFrameType_Data,
		ReliableUdpFrameType_Ack,
		//Disconnect
	};

	enum {
		MessageTypeFieldLength = 1
	};

	typedef int8_t LocalEventType;

	enum DecryptResult
	{
		DecryptResult_NoCare = 0,	// 암호화가 안되어있거나 C2C로 되어있어 복호화하지 않고 payload를 그대로 전달
		DecryptResult_Success,		// 복호화 성공
		DecryptResult_Fail,			// 복호화 실패
	};

	enum LogLevel
	{
		// OK
		LogLevel_Ok,
		// 경고
		LogLevel_Warning,
		// 오류
		LogLevel_Error,
		// 존재하지 않는 HostID
		LogLevel_InvalidHostID,
	};
}