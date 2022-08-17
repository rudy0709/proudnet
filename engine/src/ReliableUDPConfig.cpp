/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "ReliableUDPConfig.h"
#include "../include/NetConfig.h"

namespace Proud
{
	// 무작위로 채울 stream의 초기 데이터 바이트 값
	uint8_t ReliableUdpConfig::FirstStreamValue = 33;

	// 무작위로 채울 stream 단위의 크기
	int ReliableUdpConfig::MaxRandomStreamLength = 2000;

	// 한번에 보내는 frame의 최대 크기
	int ReliableUdpConfig::GetFrameMaxLength()
	{
		// 과거에는 변수였으나, 사용자가 동적으로 MtuLength를 수정할 수 있으므로 함수이어야 한다.
		return max(1300, CNetConfig::MtuLength - 100);
	}

	/* 처음 resend할 때까지 대기하는 시간
	300ms는 인간이 크게 느끼지 못하는 텀이다. 따라서 이정도면 충분하다.
	너무 짧게 잡으면 ack를 저쪽에서 보냈지만 랙 때문에(먼거리 등) 늦는 경우에도 엄한 재송신이 있을 것이므로 즐. 특히 resend 폭주는 왕 즐 
	어차피 첫 핑 나오면 줄여지므로 크게 잡아도 된다.
	윈도 TCP는 3초가 기본이다. http://support.microsoft.com/kb/170359/en-us/ */
	int64_t ReliableUdpConfig::FirstResendCoolTimeMs = 2000; 

	/* 이 값이 true이면 RTT 값에 의해 RTO가 동적으로 변경된다.
	만약 resend과다로 트래픽 소모가 심함이 의심되면 시험삼아 이 값을 false로 바꿔보자. */
	bool ReliableUdpConfig::IsResendCoolTimeRelatedToPing = true;

	// 너무 짧은 RTO는 트래픽 대량 유발. fast retransmit으로 보완될거이므로 이게 필요.
	int64_t ReliableUdpConfig::MinResendCoolTimeMs =300;

	// 최대 resend 전 대기 시간. 이게 없으니 10초까지 길어지는데, 10초나 기다려서 resend하면 사실상 막장이나 다름없다.
	int64_t ReliableUdpConfig::MaxResendCoolTimeMs = 8000;

	// resend를 반복할 때마다 resend 빈도를 낮춘다. 이를 exponential backoff라 한다.
	// 이는 resend를 반복할 때마다 RTO를 증가시키는 비율이다.
	// 100%으로 두면 sliding window의 크기에 비례해서 resend 트래픽이 선형 증가하므로 즐. 반드시 100%는 초과해야 한다.
	int ReliableUdpConfig::EnlargeResendCoolTimeRatioPercent = 110;


	//// UDP 시뮬레이션에서 보내지는 패킷의 최소 랙
	//double ReliableUdpConfig::MinLag = 0.1;
	//// UDP 시뮬레이션에서 보내지는 패킷의 최고 랙
	//double ReliableUdpConfig::MaxLag = 0.4;
	// UDP 시뮬레이션에서 패킷이 1회 정상적으로 보내질 확률
	double ReliableUdpConfig::SimulatedUDPReliabilityRatio = .7;

	// 이건 DWORD가 아니라 int이어야 한다.
	int ReliableUdpConfig::TooOldFrameNumberThreshold = (2 << 23);
	int ReliableUdpConfig::GetMaxAckCountInOneFrame()
	{
		return max((GetFrameMaxLength() - 10) / 5,10);
	}
	int64_t ReliableUdpConfig::FrameMoveInterval = 1;
	int64_t ReliableUdpConfig::ShowUIInterval = 400;
	int ReliableUdpConfig::ReceiveSpeedBeforeUpdate = 100;
	int64_t ReliableUdpConfig::CalcRecentReceiveInterval = 1000;
	int ReliableUdpConfig::GetMaxSendSpeedInFrameCount()
	{
		return  1024 * 1024 * 10 / GetFrameMaxLength();
	}
	int ReliableUdpConfig::BrakeMaxSendSpeedThreshold = 8;

	// 최대 대기 한계 시간(체감으로 느끼지 못하는 랙 30ms)을 두도록 한다.
	// 0.03은 게임에서는 적합하나 대량 데이터 전송시에는 부적합. 나중에 트래픽량 따라서 조절하는 기능이
	// 들어가야 하겠다.
	int64_t ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_Real = 1;
	// 기존의 0.03 은 일반적인 게임환경에서는 적합하나 대량 데이터 전송시에는 부적합. Real 은 이제 새로운 값 0.001 로 사용해서, 기존 게임환경에서도 0.001 을 사용하고 대량 데이터 전송에도 0.001 을 사용할 수 있도록
	// dummy test 에서는 기존의 0.001 값을 사용하는 경우 cpu 과다 사용량으로 인해 더미 테스트를 진행할 수 없는 경우가 생길 수 있으므로 dummyTest 인 경우에는 기존에 사용했던 0.03값으로 셋팅
	int64_t ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_ForDummyTest = 30;

	// VTG case: unreliable RMI가 안가지는 현상 제보와 관련일 듯 해서 false로 바꾸었다.(...취소!)
	// VTG case: reliable UDP가 대량 UDP 통신 사용시 resend time out이 발생하는 경우를 최소화하기 위해 reliable UDP frame은 훨씬 높은 우선순위로 주고받게 바꾸었다.
	bool ReliableUdpConfig::HighPriorityAckFrame = true;
	bool ReliableUdpConfig::HighPriorityDataFrame = true;

	// add by rekfkno1 - resend가 너무 많이 일어나면, recv측에서 처리가 안되어 계속 delay되는 현상이 있습니다.
	// 그래서 resend양을 조절합니다.
	int ReliableUdpConfig::ResendLimitRatio = 10;

	// 1개의 호스트가 초당 보낼 수 있는 총 frame의 갯수.  
	int ReliableUdpConfig::MinResendLimitCount = 1000;

	// 1개의 호스트가 초당 보낼 수 있는 총 frame의 갯수.  
	int ReliableUdpConfig::MaxResendLimitCount = 3000;

	// slow start 를 재시작하기 위한 최소 임계치 값(밀리초)
	// resend 가 일어날때마다 slow start 를 재시작하지 않고, 대용량 전송시 전송속도를 보장하기 위해, 적극적 유지방법의 일환으로 어느 정도(초단위) 유지하는 최소 시간 임계치값
	int ReliableUdpConfig::MinThreadholdSlowStartRestartTimeout = 60000;

	// 마스터링 TCP/IP에 의하면 이 값은 0.2~0.5초가 적당.
	int ReliableUdpConfig::DelayedAckIntervalMs = 300;
}