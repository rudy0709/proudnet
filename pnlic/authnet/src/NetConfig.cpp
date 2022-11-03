/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/NetConfig.h"
#include "../include/sysutil.h"
#include "PNVersion.h"

namespace Proud
{
	CriticalSection CNetConfig::m_writeCritSec;

	/* syscall 횟수도 줄여야 하고 현대 컴퓨터 수준에 맞게 조절.
	너무 크면 서버 메모리를 너무 차지하므로 100KB 미만이어야.

	**아래는 오래된 메모**
	netio loop가 순간 못쫓아가는 경우 쌓일 수 있는 총량을 고려해서, 큰 패킷 10개 정도 까지는 받아야.
	(예전에는 50이었으나, 동접 4000 정도 붙으면 RAM을 기가 단위로도 먹는 바람에 10으로 하향.)
	최소 횟수의 io call로 패킷 뭉치를 한번에 받는게 좋으므로 recv buf length = issue max length이다.
	OS와 RAM 상황에 따라, 사용자가 변태 프로그램을 설치하거나 레지스트리 해킹에 따라 socket buffer가 원치 않는 크기로 설정될 수 있다.
	이를 위해 OS 상황 따라가지 않고 강제로 설정해주도록 하자.
	소켓 버퍼는 너무 크면 CPU에 큰 부담을 준다. 따라서 지나친 상향화는 즐. */
	int CNetConfig::TcpIssueRecvLength = 1024 * 30;

	/* 현대 컴퓨터 수준에 맞게 하고 sliding window에서 최대한 빨리 빠져나오게 하기 위해 200KB로 조절. 물론 syscall횟수도 소폭 줄임 */
	int CNetConfig::TcpRecvBufferLength = 1024 * 50;

	// UDP datagram은 1회의 수신에서 처리할 수 있는 총량이 MTU를 넘기지 못하므로, 더 커봤자 의미가 없다.
	int CNetConfig::UdpIssueRecvLength = MtuLength * 2;

	// syscall 횟수를 줄이기 위해 커야 한다. 특히 networker thread off시
	// io loop가 10Hz 수준까지 떨어지기도 한다. 
	// UDP는 socket buffer가 모자라면 패킷 드랍을 일으키므로 충분히 값이 커야 한다.
	// ios는 상한선이 1790000byte인데, 이값에는 못 미치더라도 
	// 10Hz에서 4MB/sec은 socket buffer 초과로 인한 패킷 드랍은 없어야 한다.
	int CNetConfig::UdpRecvBufferLength_Client = 1024 * 400;

	// 서버측도 syscall을 줄여야 하지만 메모리를 너무 먹으면 안되므로.
	// 그리고 서버는 networker thread off같은 것이 없으므로 1000Hz 정도는 충분히 처리한다고 가정하는 것이 낫다.
	// 30KB=>100KB: 클라에서 통신량이 매우 많은 경우 가령 case 37의 경우 간헐 실패를 막기 위해.
	int CNetConfig::UdpRecvBufferLength_Server = 1024 * 100;

	// 1개의 udp port로 대량의 클라이언트를 처리한다.
	// 따라서 io loop가 못 쫓아가더라도 패킷 유실을 최소화 하기 위해 가능한 커야 한다.
	// 512KB=>2MB: 클라우드 서버에서는 소켓의 수가 많지 않으므로 이것이 충분히 커야.
	int CNetConfig::UdpRecvBufferLength_ServerStaticAssigned = 1024 * 1024 * 2; 

	/* syscall 횟수를 줄이기 위해 키워야 하고 서버측 메모리 사용을 줄이기 위해 작아야 한다.
	*/
	int CNetConfig::TcpSendBufferLength = 1024 * 50;

	// UdpRecvBufferLength_Client 주석 참고
	int CNetConfig::UdpSendBufferLength_Client = 1024 * 400;

	// UdpRecvBufferLength_Server 주석 참고
	int CNetConfig::UdpSendBufferLength_Server = 1024 * 100;

	// 서버는 송신량이 많다. 게다가 1개 udp port로 여러 클라이언트를 처리한다면
	// 굉장한 크기가 아니면 패킷 드랍이 우려된다.
	// 물론 io loop가 못쫓아가더라도 패킷 드랍이 최소화되려면 가능한 커야 한다.
	// 1MB=>4MB: 클라우드 서버에서는 소켓의 수가 많지 않으므로 이것이 충분히 커야.
	int CNetConfig::UdpSendBufferLength_ServerStaticAssigned = 1024 * 1024 * 4;

	// true이면 SO_KEEPALIVE를 켠다.
	// ProudNet 자체가 동등한 기능이 있으므로 꺼도 된다만 찾기 힘든 문제 발생시 이를 시험해볼 수 있겠지.
	bool CNetConfig::EnableSocketTcpKeepAliveOption = false;

	// reliable UDP의 heartbeat 주기
	// 처음에는 1ms였으나 게임클라 CPU가 너무 먹는데다 사람이 못 느끼는 수준이므로 (200fps 플러스 알파)정도가 적당. 
	int64_t CNetConfig::ReliableUdpHeartbeatIntervalMs_Real = 1;
	int64_t CNetConfig::ReliableUdpHeartbeatIntervalMs_ForDummyTest = 5;

	// TCP connect를 하되, 일반적인 blocked socket의 연결 시도시간이 OS마다 다른 문제를 해결하기 위한 용도로 쓰인다.
	// 5->10: 서버에서 TCP accept 속도가 너무 느린 경우에 아예 접속 못한다는 문제로 이어지므로.
	// 10->30: 중국처럼 느린 곳에서는 5% 이내로 TCP 연결 성사 자체가 10초 이상 걸리는 경우가 있다. <=추후 UDP 혼잡 제어 기능 들어가면 해결될라나.
	int64_t CNetConfig::TcpSocketConnectTimeoutMs = 30000;

	/* 서버 입장에서, 클라가 TCP 연결은 했지만 connection hint 등을 주고 받아서 HostID가 발급되는 과정까지 끝날 때까지 remote는 candidate 상태이다.
	candidate 상태로 너무 오래 있으면 불량으로 간주하고 쫓아내야 한다. 그 역치다. */
	int64_t CNetConfig::ClientConnectServerTimeoutTimeMs = TcpSocketConnectTimeoutMs * 2 + 3000;

	// 사용자는 연결 디스 조건 타임아웃을 지정할 수 있는데, 미지정시 기본 설정되는 값.
	// 30으로 지정하면 핑이 오가는 주기가 20초 정도가 된다. 이정도면 트래픽도 충분히 적은 편.
	int64_t CNetConfig::DefaultConnectionTimeoutMs = 30000;

	// 사용자가 지정할 수 있는 연결 디스 조건 타임아웃의 최소값. 이 값 이하이면 false positive의 위험이 있다.
	// reliable ping으로 체크를 하므로 
	// 중국에서 막장 gateway dropout에서는 10초 정도 나온다. 따라서 그 값보다는 커야.
	int64_t CNetConfig::MinConnectionTimeoutMs = 15000;

	// 위와 반대로, 이것은 최대값. 이값을 넘어가면 TCP 자체가 디스를 검출하므로 의미가 없다. 
	int64_t CNetConfig::MaxConnectionTimeoutMs = 240000;

	// Proud.CNetConfig.EveryRemoteIssueSendOnNeedInterval와 관련됨
	// 10=>100: 10은 임베디드에서 지나치게 context switch를 일으켜 부하.
	// 너무 큰 값을 주고 싶지만 IOCP의 경우 특정 스레드만 awake하게 하는 방법이 없으므로
	// 그냥 이렇게 하자. 사람이 크게 느낄 정도의 시간은 아니다.
	// 만약 Disconnect or Stop()에서 너무 느리다고 나오면 이걸 고칠게 아니라
	// 스트레스테스트 프로그램에서 net worker thread and user worker thread가 여러 인스턴스에 의해 공유되게 만드는게 답이다.
	// 100=>10: 위에게 맞지만, 아직 StressTest까지 테스트 되지 않으면, 일단 롤백. 잘 되면 100으로 다시 바꾸자.
	uint32_t CNetConfig::WaitCompletionTimeoutMs = 10;

	// 10~1ms 사이로 결정해야 함. 너무 길면 정확한 매시 단위의 체크를 하지 못한다.
	// 예전엔 1이었으나 CritSec contention이 너무 심한 듯 해서 5로 수정
	uint32_t CNetConfig::ServerHeartbeatIntervalMs = 5;

	uint32_t CNetConfig::ServerUserWorkMaxWaitMs = 100;

	// 10~1ms 사이로 결정해야 함. 너무 길면 정확한 매시 단위의 체크를 하지 못한다.
	uint32_t CNetConfig::ClientHeartbeatIntervalMs = 1;

	int64_t CNetConfig::ServerHolepunchIntervalMs = UdpHolepunchIntervalMs;

	int64_t CNetConfig::ServerUdpRepunchIntervalMs = CNetConfig::ServerHolepunchIntervalMs * 3;

	// 이 횟수만큼 서버 UDP가 fallback하면 더 이상의 UDP 시도를 하지 않는다.
	// [VTG case] 이걸 계속하게 했더니 게임할때마다 10초 간격으로 퍽퍽 랙 생기는 짜증으로 이어지니까. 차라리 relay만 하는게 낫다. 그래서 3=>1.
	int CNetConfig::ServerUdpRepunchMaxTrialCount = 1;

	// 클라-서버간 UDP 홀펀칭을 할 수 있는 최대 횟수
	// ServerUdpRepunchMaxTrialCountMs는 펄백-재기 제한 횟수이며 이것은 일단 펄백이 일어난 후에도 그 안에서 몇번 할 수 있냐를 의미
	// 5 정도면 매우 충분. 패킷 드랍율이 50% 이하이면 게임 하지 말란 소리므로 0.5^5 = 0.03, 즉 이정도면 충충.
	int CNetConfig::ServerUdpHolepunchMaxTrialCount = 5;

	int64_t CNetConfig::P2PHolepunchIntervalMs = UdpHolepunchIntervalMs;

	/* UDP 홀펀칭 주기.

	피어 갯수가 100개 이상이면(SEOYU) 이것 자체도 꽤 트래픽을 먹는다. 이정도 길게 잡아도 괜찮은 것이,
	UDP가 적당하게 작동하려면 10% 미만의 패킷 로스가 있어야 하니까, 설령 홀펀칭이 되더라도 너무 패킷 로스가 심하면 그 자체로도 막장.
	그래서 3에서 상향.
	*/
	int64_t CNetConfig::UdpHolepunchIntervalMs = 7300;

	// 몇번째 턴에서부터 샷건 쏠까
	// IPTIME땜시롱
	int CNetConfig::P2PShotgunStartTurn = 10;

	// P2P 홀펀칭을 시도할 최대 횟수
	// 이 횟수까지도 홀펀칭 실패시 relayed 영구화
	int CNetConfig::P2PHolepunchMaxTurnCount = 30;

	int64_t CNetConfig::PurgeTooOldUnmatureClientTimeoutMs = 12200;

	// 2초는 너무 짧아서 엄한 타임아웃이 간혹 생긴다.
	int64_t CNetConfig::PurgeTooOldAddMemberAckTimeoutMs = 15300;

	int64_t CNetConfig::DisposeGarbagedHostsTimeoutMs = 200;

	uint32_t CNetConfig::LanClientHeartbeatIntervalMs = 5;

	//임시...
	int64_t CNetConfig::LanPeerConnectPeerTimeoutTimeMs = 20000;
	int64_t CNetConfig::PurgeTooOldJoiningTimeOutIntervalMs = 10000;
	int64_t CNetConfig::RemoveLookaheadMessageTimeoutIntervalMs = 1000;

	int CNetConfig::RecyclePairReuseTimeMs  = 10 * 1000; // 새로 개정된 P2P 관련 아티클에 의하면 십수초까지밖에 못 버티는 NAT 모델이 있다고 한다.
	int64_t CNetConfig::GarbageTooOldRecyclableUdpSocketsIntervalMs = 3 * 1000;

	// 위 값보다는 짧다. 혹시나 서버에서 너무 많은 포트가 잔존하면 포트 고갈이 우려되니까. 큰 차이가 아니더라도.
	int CNetConfig::ServerUdpSocketLatentCloseMs = 5000;

	// TCP 송신을 issue했으나 이 시간만큼 completion이 안오면 gateway dropout 위험을 의심해야.
	int64_t CNetConfig::TcpInDangerThresholdMs = 5000;

	// 서버와의 TCP 연결이 위험 상태인지 체크 하는 기능(IsTcpUnstable 함수) 에서 검사할때
	// 감지할, reliable ping-pong 플러스 알파 시간 값(밀리초).
	// 너무 값이 짧으면 정상적인 상황인데도 UDP 통신량이 강하해 버리고,
	// 너무 값이 길면 진짜 불량 상황에도 감지 못한다.
	int64_t CNetConfig::TcpUnstableDetectionWaitTimeMs = 3000;

	// 상기 위험 상황에서 UDP 송신을 일시 정지하는 최대 시간
	int64_t CNetConfig::PauseUdpSendDurationOnTcpInDangerMs = 2000;

	// modify by seungwhan: UDP패킷 복제 해킹 차단 기능. UDP Packet들의 ID를 저장하며, 이 시간에 한번씩 저장소를 제거한다.
	int64_t CNetConfig::RecentAssemblyingPacketIDsClearIntervalMs = 3000;

	// P2P 연결 시도가 너무 오랫동안 성사되지 못하면 P2P 연결 실패로 간주하고 relay mode로 우회한다. 이 값은 그 역치다.
	// symmetric NAT 예측 기법을 쓰면서 오랜 시도 시간이 필요하다. 그래서 30. 그러나 너무 길게 잡으면 동시 p2p 연결 시도에 비례해서 증가하는 폭이 너무 커진다.
	// 따라서 필요시 줄이도록 한다.
	int64_t CNetConfig::GetP2PHolepunchEndTimeMs()
	{
		return CNetConfig::P2PHolepunchIntervalMs * P2PHolepunchMaxTurnCount;
	}

	// Symmetric NAT에서 포트 예측 기법을 하되, UDP로 홀펀칭을 하는 것이므로 패킷 유실 횟수도 감안, 몇번이나 같은 포트 번호로
	// 시도할지를 지정한다.
	// 충분히 크게 잡아놓아야 한번에 많은 횟수를 시도하게 된다.
	int CNetConfig::ShotgunTrialCount = 3;

	// symmetric NAT를 위해 한번 시도에서 몇 개의 포트를 동시에 예측 시도를 하는지를 지정하는 값
	// 너무 넓게 쏘면 멀웨어 감지 위험. 따라서 3
	// => 클라-서버간 TCP 홀펀칭 증발 의혹차 0으로.
	int CNetConfig::ShotgunRange = 0;


	/* UDP 핑 주기.
	홀펀칭 매핑이 수십초 안에 증발해버리는 공유기에서는 빠른 fallback이 필요하므로 이 주기는 가급적 짧아야.
	그러나 피어 갯수가 백개가 넘으니 중국에서 이것 자체도 문제되는 트래픽. 따라서 5초로 잡았다.
	TCP라면 RTO+latency로 잡으면 되지만 unreliable에서는 못 가는 경우 때문에 RTO의 1/5 수준으로 잡아야 한다. 그것이 4.3초인 이유.
	*/
	int64_t CNetConfig::UnreliablePingIntervalMs = 4300;

	/* P2P reliable ping 주기.
	Q: 이건 왜 4.3초인가요?
	A: 많은 사용자는 P2P 연결을 맺은 후 이들의 회선 품질을 조기에 파악하고 싶어합니다.
	이때 패킷 로스율과 패킷 전송 시간(RTT)가 모두 고려된 최종 값을 원합니다.
	그것으로 reliable RTT를 쓰는 것이죠. 이를 조기에 파악하기 위해서 이러한 기능이 존재합니다.
	Q: 그렇다면 P2P 연결 맺으면 초기에 미리 파악해 두고 텀이 길어도 되지 않을까요? economic mode에 한해서요?
	A: 그...그렇긴 하네요. 추후 고려해 봅시다. 아직은 안 급함.
	*/
	int64_t CNetConfig::ReliablePingIntervalMs = 4300;

	/* UDP Packet Count를 측정하는 기능을 켜고 끕니다. */
	bool CNetConfig::UseReportRealUdpCount = true;

	/* UDP Packet Count를 측정하는 시간주기입니다. default 30초입니다. */
	int64_t CNetConfig::ReportRealUdpCountIntervalMs = 30000;

	const int64_t CNetConfig::SpeedHackDetectorPingIntervalMs = 590;

	/* 선형계획법(?)
	- 너무 작게 잡으면 값의 변화가 작아서 기존 값의 오류를 쉽게 극복 못하고
	너무 크게 잡으면 값의 변화가 커서 대체적 값의 중간점을 유지하기 어렵다.
	이 값의 범위는 0~100 사이다. 0에 가까울수록 좌항에 가깝게, 100에 가까울수록 우항에 가까워진다.
	*/
	int CNetConfig::LagLinearProgrammingFactorPercent = 80;

	int CNetConfig::StreamGrowBy = 1024;

	// ProudNet net 내부 버전
	// 과거에는 이것을 바꿀때마다 srcDotNet, srcJava, srcFlash도 동일하게 바꾸었으나, 
	// 현재는 SWIG native plugin을 적용하면서, 타 언어로의 구현은 퇴역되면서, 더 이상 바꿀 필요 없음.
	int CNetConfig::InternalNetVersion = 0x0003020a;

	// ProudNet Lan 내부 버젼.
	int CNetConfig::InternalLanVersion = 0x0000013a;

	// ProudNet Version 정보를 반환합니다.
	Proud::String CNetConfig::GetVersion()
	{
		return StringA2T(g_versionText);
	}

	/* UDP 패킷의 MTU 크기
	[과거1]http://en.wikipedia.org/wiki/Maximum_transmission_unit#Path_MTU_discovery 에 의하면 1500이 적당하다.

	[과거2]576은 1983년 스펙인데다 어차피 UDP 자체도 MTU보다 크면 fragment를 하기 때문에 문제될 것은 없다.
	1500으로 잡았더니 공유기 뒤 클라들의 홀펀칭 증발 고질이 생기는건가? 아무튼 그래서 1200으로 수정. 끙.

	[현재]어쩌면 더 줄여야 할지도. peer 사이에 엄하게 ICMP까지 막아버리는 방화벽이 있고 MTU가 모자란 라우터가 있으면
	path MTU failure로 인해서 낮은 MTU로의 조절을 못할 터인데 이런 경우 대책없이 증발할터이니.
	그래서 500이다.  그리고 이것만으로는 부족하다. PN 자체에서의 MTU 이하 패킷 통신을 권고한다.

	KIST: 이 값을 65400으로 올리니까 LAN에서 100MB/sec를 전송해도 CPU 사용량이 팍 준다!
	(1300일때 3%, 8000일때 0.5%)
	그러나 jumbo packet은 9K 정도. 따라서 이것을 올리는 것이 도움은 되지만 근본적 해결은 아니다.
	WSASendMsg를 써야 할라나? 벌크 송신을 할 수 있게 말이다...
	*/
	int CNetConfig::MtuLength = 1300;

	// 충분히 크게 잡아야 함. 송신 큐 자체를 없애는 일이니까.
	// 인터넷 환경에서의 UDP ping * 2 이상은 잡아야 한다.
	int64_t CNetConfig::RemoveTooOldUdpSendPacketQueueTimeoutMs = 3 * 60 * 1000;
	int64_t CNetConfig::AssembleFraggedPacketTimeoutMs = 10000;

	/* packet defrag 과정 중 잘못된 유형의 패킷이 오면 OnWarning으로 콜백을 뿌린다.
	실섭에서는 off를 권장. 해킹 패킷에 대해서 경고로 뿌리면 짜증나니까.
	디버깅 용도로 on을 잠깐 켜자. */
	bool CNetConfig::EnablePacketDefragWarning = false;

	// 서버와의 연결 종료시 클라이언트가 대기할 수 있는 최대 시간.
	// 이 시간을 넘어서도 연결 해제 처리가 끝나지 못하면 서버에서는 클라 연결 해제를 즉시 인식하지 못한다.
	int64_t CNetConfig::DefaultGracefulDisconnectTimeoutMs = 2000;

	// 주의: EnableTestSplitter를 켤 경우 RMI encryption 관련해서 아직 디버깅이 다 끝나지 않았다
	const bool CNetConfig::EnableTestSplitter = false;

	int CNetConfig::MaxS2CMulticastRouteCount = 4;

	/* 서버에서 클라이언트로 P2P routed multicast를 할 때 이 값 이상의 핑을 가진 p2p끼리는 route를 하지 않는다. */
	int CNetConfig::UnreliableS2CRoutedMulticastMaxPingDefaultMs = 100;

	/* 이 값이 true이면, 무조건 MessageType_UnreliableRelay1_RelayDestListCompressed를 사용한다.
	MessageType_UnreliableRelay1_RelayDestListCompressed는 MessageType_UnreliableRelay1보다 압축이 더 안될 경우 MessageType_UnreliableRelay1
	를 쓰게 되어 있다.
	MessageType_UnreliableRelay1_RelayDestListCompressed의 기능이 버그가 없는지 테스트 할 때 빼고는 이 값을 손대지 말 것.
	*/
	bool CNetConfig::ForceCompressedRelayDestListOnly = false;

	// 예전에는 64KB였으나 서버간 통신에서는 더 큰 값을 요구하므로
	//int CNetConfig::MessageMaxLength = 1024*1024; 

	bool CNetConfig::EnableMessagePriority = true;
	CriticalSection& CNetConfig::GetWriteCriticalSection()
	{
		return m_writeCritSec;
	}

	void CNetConfig::AssertTimeoutTimeAppropriate(int64_t ms)
	{
		if (ms < MinConnectionTimeoutMs)
		{
			// test 95,96이 ACR test를 위해 강제로 엄청 짧게 잡을 수도 있으므로
			ShowUserMisuseError(_PNT("WARNING: Too short timeout value. It may cause unfair disconnection."));
		}
#ifndef _DEBUG
		if (ms > MaxConnectionTimeoutMs)
		{
			//ShowUserMisuseError(_PNT("Too long timeout value. It may take a lot of time to detect lost connection."));
		}
#endif
	}

	int CNetConfig::DefaultMaxDirectP2PMulticastCount = INT_MAX;

	/** upnp SSDP를 이용한 공유기 탐색 기능

	디자인 타임에서 uPNP 기능을 쓰지 않으려면 이 값을 false로 세팅하면 된다.
	uPNP 기능이 부작용을 일으킬 때만 다룰 값이다.

	주의! 이걸 변경 전에 Proud.CStartServerParameter.m_enableUpnpUse 사용을 고려하라. */
	bool CNetConfig::UpnpDetectNatDeviceByDefault = true;

	/** upnp AddPortMapping을 클섭 TCP 연결에 적용하는 기능

	디자인 타임에서 uPNP 기능을 쓰지 않으려면 이 값을 false로 세팅하면 된다.
	uPNP 기능이 부작용을 일으킬 때만 다룰 값이다.

	주의! 이걸 변경 전에 Proud.CStartServerParameter.m_enableUpnpUse 사용을 고려하라. */
	bool CNetConfig::UpnpTcpAddPortMappingByDefault = false;

	// SEOYU때문에 3.3->10 상향. 그러나 나중에 고쳐야 하겠다.	
	int64_t CNetConfig::ElectSuperPeerIntervalMs = 10000;

	// 60에서 상향. 어차피 재는 간격 길게 잡아도 OK. 	
	int64_t CNetConfig::MeasureClientSendSpeedIntervalMs = 120000;

	int64_t CNetConfig::MeasureSendSpeedDurationMs = 500;

	DirectP2PStartCondition CNetConfig::DefaultDirectP2PStartCondition = DirectP2PStartCondition_Jit;

	bool CNetConfig::CatchUnhandledException = true;

	ErrorReaction CNetConfig::UserMisuseErrorReaction = ErrorReaction_MessageBox; // 이값 바꾸면 문서화도 손대야

	bool CNetConfig::NetworkerThreadPriorityIsHigh = true;

	// 너무 길면 수퍼 피어를 잘 선택하는데 너무 오래 걸린다.
	// 너무 짧으면 트래픽 과다.
	//double CNetConfig::ReportP2PGroupPingInterval = 4.2;

	// 이 시간에 한번씩 모든 피어간의 핑을 서버로 보내준다.
	// 3에서 상향. 어차피 천천히 줘도 됨.
	int64_t CNetConfig::ReportP2PPeerPingTestIntervalMs = 30 * 1000;

	// 이 시간에 한번씩 클라간에 서버의 시간 및 FrameRate 및 핑을 전달한다. ( direct p2p 인경우엔 내부적으로도 udp핑을 주고받는다.)
	// 이시간 마다 가는 패킷은 direct p2p 혹은 relay 둘다로도 전송된다. ( 내부 RMI 사용 )
	// 3에서 상향. 어차피 천천히 줘도 됨.
	int64_t CNetConfig::ReportServerTimeAndPingIntervalMs = 30000;

	// 너무 길면 수퍼 피어를 잘 선택하는데 너무 오래 걸린다.
	// 너무 짧으면 트래픽 과다.
	int64_t CNetConfig::ReportLanP2PPeerPingIntervalMs = 5000;

	// 매 1초마다 송신량 체크 등을 하므로
	int64_t CNetConfig::LongIntervalMs = 1000;

	// send brake에 의해서도 이 속도 이하는 불가능. 이 이하는 홀펀칭까지도 증발하는 사태 위험이 있으므로
	// 과거에는 5KB/sec였으나, 느린 3G나 열악 네트워크 환경에서는 총 피어의 총 통신량 합산이 13KB/sec를 넘지 못하기도 하므로
	// 0.3KB/sec로 하향.
	int64_t CNetConfig::MinSendSpeed = 300;

	// 15KB/sec 이상 쏘는 경우 LAN->공유기->WAN 속도를 감당하지 못함으로 공유기 성능 저하로 이어질 수 있다. 이를 예방하기 위함
	int CNetConfig::DefaultOverSendSuspectingThresholdInBytes = 15 * 1024;

	// send brake를 켜거나 끈다. 버그가 의심되면 시험삼아 끄자.
	// TODO: unreliable RMI만 send brake하도록 하는 기능이 들어가기 전까지는 false로 두자!!
	// TODO: SSE 작동 안하는 VM에서의 이슈를 해결 후 다시 true로 바꾸자. 혼잡 제어 기능도 한번 재정비하고 나서.
	bool CNetConfig::EnableSendBrake = false;

	// 두 호스트간 UDP 통신에서 패킷 로스율이 이 값을 넘으면
	// 송신측의 송신 속도를 급격히 다운시킨다.
	int CNetConfig::UdpCongestionControl_MinPacketLossPercent = 20;

	// unsafe heap이 문제를 일으키는 것 같으면 이 값을 true로 설정한 후 ProudNet의 각 API를 사용할 것!
	// (아직 문제 제보를 하는 업체가 있는바 true이다.)
	// Vista에서 성능 프로필링 결과 별 차이가 안난다 --;
	bool CNetConfig::ForceUnsafeHeapToSafeHeap = false;

	// VizServer로 연결 실패시 연결 재시도 시간
	int64_t CNetConfig::VizReconnectTryIntervalMs = 3000 + ClientConnectServerTimeoutTimeMs;

	// 수퍼피어로 이미 선정된 놈은 비등비등한 놈에게 수퍼피어를 내줘서는 안된다. 그랬다가는 지나친 수퍼피어 바뀌기로 이어지니까.
	// 이는 그것을 위한 역치값이다.
	int64_t CNetConfig::SuperPeerSelectionPremiumMs = 200;

	// 이값이 너무크면 송신렉이 발생할수 있다. 3ms ~ 10ms사이가 적당함. ( 초단위로 넣어야함. )
	int CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs = 1;

	// HostID가 재사용되려면 일정 시간 미사용 상태이어야 한다. 이 값은 그 최소 기간이다.
	// 이 값은 홀펀칭 재사용 가능 임계 시간(대략 30초)보다 훨씬 커야 한다.
	// ACR에 의해 복원되는 HostID가 하필 재사용된 놈이면 곤란. 따라서 ACR 최장 시간보다는 충분히 길어야 한다.
	int64_t CNetConfig::HostIDRecycleAllowTimeMs = 60 * 5 * 1000;

	int CNetConfig::SendQueueHeavyWarningCapacity = 1024 * 1024 * 10; // 10MB

	// 10->30 상향. 너무 로그가 많이 남아서.
	int64_t CNetConfig::SendQueueHeavyWarningTimeMs = 30000;
	int64_t CNetConfig::SendQueueHeavyWarningCheckCoolTimeMs = 10000;
	int64_t CNetConfig::UpdateNetClientStatCloneCoolTimeMs = 3000;

	bool CNetConfig::EnableErrorReportToNettention = false;

	int64_t CNetConfig::ManagerGarbageFreeTimeMs = 1000;

	unsigned int CNetConfig::ManagerAverageElapsedTimeCollectCount = 20;
	bool	CNetConfig::EnableStarvationWarning = false;

	// true이면, 기본적으로 packet frag board를 쓴다. false를 하면 안할 때와 서버 성능 차이를 비교하는 용도로 활용 가능.
	// 측정 결과, socket API i/o call 횟수가 늘어나서 오히려 서버 CPU를 더 먹는다능.
#ifdef TEST_WANGBA
	bool CNetConfig::FraggingOnNeedByDefault = false;
#else
	bool CNetConfig::FraggingOnNeedByDefault = true;
#endif

	bool CNetConfig::CheckDeadLock = false;

	// 내부 랜에서도 일정 갯수 이상의 직접 p2p 송신 갯수 제한을 걸려면 false로 설정하라. 웬만하면 이럴 일 없음.
	// SEOYU에서 테스트 차 꺼 보기도 하는 값이기도 함.
	bool CNetConfig::UseIsSameLanToLocalForMaxDirectP2PMulticast = true;

	// 웬만하면 켜 놓을 것. 트래픽이 부담돼서 이것도 끈다면 모를까.	
	// SEOYU에서 껐으나 트래픽 부담율이 적어서 다시 켰음. 클섭 통신 뿐이니까.
	// 2014.03.13 기본값 false로 변경
	bool CNetConfig::EnableSpeedHackDetectorByDefault = false;

	const int CNetConfig::TcpSendMaxAmount = 1024 * 1024 * 100;

	//kdh MessageOverload Warning 추가
	int CNetConfig::MessageOverloadWarningLimit = 100;
	int CNetConfig::MessageOverloadWarningLimitTimeMs = 5000;

	// 	int64_t CNetConfig::LanServerMessageOverloadTimerIntervalMs = 5000; 
	// 	int64_t CNetConfig::LanClientMessageOverloadTimerIntervalMs = 5000;
	// 	int64_t CNetConfig::NetServerMessageOverloadTimerIntervalMs = 5000;
	// 	int64_t CNetConfig::NetClientMessageOverloadTimerIntervalMs = 5000;
	int64_t CNetConfig::MessageOverloadTimerIntervalMs = 5000;

	int64_t CNetConfig::LanRemotePeerHeartBeatTimerIntervalMs = 5000;

	// reliable UDP에서 resend가 이 시간 이상 계속 실패하면 relay fallback을 한다. 
	// TCP retransmission timeout과 같은 역할. win32에서도 20초다.
	int	CNetConfig::P2PFallbackTcpRelayResendTimeIntervalMs = 20000;

	const bool CNetConfig::AllowUseTls = true;

	// 이 값이 true이면, NetClient.Disconnect 및 NetServer.Stop이 장시간 처리 못하고 데드락이 발생해도 
	// 일정 시간 지나면 무조건 루프를 빠져나온다. 당연히 heap은 망가질 위험이 있다.
	// 릴리즈할 때 데드락 문제가 생길때만 응급처치용이고, 평소에는 반드시 끌 것.
	bool CNetConfig::ConcealDeadlockOnDisconnect = false;

	// 메시지를 송신큐에 넣은지 이 시간만큼 지나면 보내지 않고 버린다.
	// 단, ring 0 or 1가 아닌 unreliable message에 한해서.
	// unreliable로 enque된 메시지가 모두 대응된다. UDP뿐만 아니라 **TCP**도.
	int CNetConfig::CleanUpOldPacketIntervalMs = 10000;

	int CNetConfig::NormalizePacketIntervalMs = 5000;

	const int CNetConfig::MessageMaxLength = 1024 * 1024 * 1024; // 1GB

	// MessageID가 0이 아닌 메시지들을 받은 적이 있으면 매 이 시간마다 delayed ack를 전송한다.
	double CNetConfig::MessageRecovery_MessageIDAckIntervalMs = 2200;

	void CNetConfig::ThrowExceptionIfMessageLengthOutOfRange(int length)
	{
		if (length <= CNetConfig::MessageMinLength || length > CNetConfig::MessageMaxLength)
			ThrowInvalidArgumentException();
	}

	// accept() 함수에서 EINVAL을 리턴 한 경우 재 bind() -> listen() -> accept()를 수행 하는 로직이 활성화 된다.
	// false로 설정 되어 있는 경우 위 작업 없이 에러 처리 된다.
	bool CNetConfig::ListenSocket_RetryOnInvalidArgError = true;

	bool CNetConfig::AllowOutputDebugString = true;

	// 이것을 false로 하면 속도가 빨라지는데 https://trello.com/c/thei0WfI/867-1-7 문제가 있다. 해결한 후 제거하자.
	bool CNetConfig::DefensiveSendReadyListAdd = true;
	bool CNetConfig::DefensiveCustomValueEvent = true;
}
