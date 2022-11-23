/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once
#include "../include/pnstdint.h"
#include "../include/BasicTypes.h"

namespace Proud
{
	class ReliableUdpConfig
	{
	public:
		static uint8_t FirstStreamValue;
		static int MaxRandomStreamLength;
		static int GetFrameLength();
		static int64_t FirstResendCoolTimeMs;
		static bool IsResendCoolTimeRelatedToPing;
		static int EnlargeResendCoolTimeRatioPercent;
		static int64_t MinResendCoolTimeMs;
		static int64_t MaxResendCoolTimeMs;
		static int64_t MinLag;
		static int64_t MaxLag;
		static double SimulatedUDPReliabilityRatio;
		static int TooOldFrameNumberThreshold;
		static int GetMaxAckCountInOneFrame();
		static int64_t FrameMoveInterval;
		static int64_t ShowUIInterval;
		static int ReceiveSpeedBeforeUpdate;
		static int64_t CalcRecentReceiveInterval;
		static int GetMaxSendSpeedInFrameCount();
		static int BrakeMaxSendSpeedThreshold;
		static int64_t StreamToSenderWindowCoalesceIntervalMs_Real;
		static int64_t StreamToSenderWindowCoalesceIntervalMs_ForDummyTest;
		static bool HighPriorityAckFrame;
		static bool HighPriorityDataFrame;
		static int ResendLimitRatio;
		static int	MinResendLimitCount;
		static int	MaxResendLimitCount;
		static int	MinThreadholdSlowStartRestartTimeout;
		static int DelayedAckIntervalMs;
	};
}
